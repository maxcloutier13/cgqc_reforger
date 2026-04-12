// ============================================================
// CGQC_RangeTools_TargetHitComponent.c
//
// Tracks projectile hits on an IPSC target.
// Records: shooter playerID, impact velocity (m/s), distance (m),
//          world-space hit position.
//
// Two user actions:
//   Reset       — saves transform, deletes old entity, spawns fresh prefab
//   Check Target — sends a hit-group report hint to the requesting player
//
// Fully server-authoritative. Hit logic, spawning, and report
// building all run on authority. Hint delivery is RPC'd to the
// requesting client's owned PlayerController component.
//
// ---- Prefab setup ----
// On CGQC_Target_IPSC.et:
//   1. Replace SCR_DamageManagerComponent with CGQC_RangeTools_TargetDamageManager
//   2. Add CGQC_RangeTools_TargetHitComponent (set m_sPrefabPath to this .et)
//   3. ActionsManagerComponent -> Additional Actions:
//        CGQC_RangeTools_ResetTargetAction
//        CGQC_RangeTools_CheckTargetAction
//
// On PlayerController.et (modded SCR_PlayerController):
//   4. Add CGQC_RangeTools_PlayerReportComponent
// ============================================================


// ----------------------------------------------------------------
// Static helper for reset — stores spawn parameters across the
// entity deletion boundary (same pattern as CGQC_TargetMoverComponent)
// ----------------------------------------------------------------
class CGQC_RangeTools_ResetHelper
{
	static ResourceName s_sPrefab;
	static vector s_vTransform[4];

	static void DoReset()
	{
		Resource res = Resource.Load(s_sPrefab);
		if (!res || !res.IsValid())
		{
			Print("[CGQC_RangeTools_ResetHelper] Resource.Load failed: " + s_sPrefab, LogLevel.ERROR);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = s_vTransform[0];
		params.Transform[1] = s_vTransform[1];
		params.Transform[2] = s_vTransform[2];
		params.Transform[3] = s_vTransform[3];

		IEntity newEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (newEnt)
			Print("[CGQC_RangeTools_ResetHelper] Spawned new target at " + s_vTransform[3].ToString(), LogLevel.NORMAL);
		else
			Print("[CGQC_RangeTools_ResetHelper] SpawnEntityPrefab failed!", LogLevel.ERROR);
	}
}


// ----------------------------------------------------------------
// Per-shot data record
// ----------------------------------------------------------------
class CGQC_RangeTools_HitData
{
	int		m_iPlayerID;
	float	m_fVelocity;	// m/s at impact
	float	m_fDistance;	// metres from shooter to target at time of shot
	vector	m_vWorldPos;	// world-space impact point

	void CGQC_RangeTools_HitData(int pid, float vel, float dist, vector wpos)
	{
		m_iPlayerID = pid;
		m_fVelocity = vel;
		m_fDistance = dist;
		m_vWorldPos = wpos;
	}
}


// ----------------------------------------------------------------
// Component class descriptor
// ----------------------------------------------------------------
class CGQC_RangeTools_TargetHitComponentClass : ScriptComponentClass {}


// ----------------------------------------------------------------
// Main hit-tracking component
// ----------------------------------------------------------------
class CGQC_RangeTools_TargetHitComponent : ScriptComponent
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab to re-spawn on Reset — point to this target's own .et",
		params: "et")]
	protected ResourceName m_sPrefabPath;

	[Attribute(defvalue: "20", uiwidget: UIWidgets.EditBox,
		desc: "Maximum shots tracked before ignoring new ones")]
	protected int m_iMaxHits;

	// authority-only runtime state
	protected ref array<ref CGQC_RangeTools_HitData> m_aHits = new array<ref CGQC_RangeTools_HitData>();
	protected RplComponent m_RplComp;

	// ----------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		m_RplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
		Print("[CGQC_TargetHit] OnPostInit | entity=" + owner.GetName()
			+ " | IsProxy=" + (m_RplComp && m_RplComp.IsProxy()).ToString(), LogLevel.NORMAL);
	}

	// ----------------------------------------------------------------
	protected bool IsAuthority()
	{
		return !m_RplComp || !m_RplComp.IsProxy();
	}

	// ----------------------------------------------------------------
	// Called by GetOnDamage() invoker on the entity's damage manager.
	// Runs on authority only (proxy guard inside).
	// ----------------------------------------------------------------
	protected void OnDamageReceived(BaseDamageContext damageContext)
	{
		if (!IsAuthority())
			return;

		if (!damageContext)
			return;

		// hitPosition and impactVelocity are direct fields on BaseDamageContext
		vector impactPos = damageContext.hitPosition;
		float velocity = damageContext.impactVelocity.Length();

		// instigator is also a direct field
		int playerID = -1;
		Instigator instigator = damageContext.instigator;
		if (instigator)
		{
			IEntity instigEnt = instigator.GetInstigatorEntity();
			if (instigEnt)
			{
				playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(instigEnt);
				if (playerID <= 0)
				{
					IEntity parent = instigEnt.GetParent();
					if (parent)
						playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(parent);
				}
			}
		}

		Print("[CGQC_TargetHit] OnDamageReceived | damage=" + damageContext.damageValue.ToString()
			+ " | vel=" + velocity.ToString()
			+ " | pid=" + playerID.ToString()
			+ " | pos=" + impactPos.ToString(), LogLevel.NORMAL);

		RegisterHit(playerID, velocity, impactPos);
	}

	// ================================================================
	// RegisterHit — called by OnDamageReceived or external damage manager
	// ================================================================
	void RegisterHit(int playerID, float velocityMS, vector worldHitPos)
	{
		if (!IsAuthority())
		{
			Print("[CGQC_TargetHit] RegisterHit called on proxy — ignoring.", LogLevel.WARNING);
			return;
		}

		if (m_aHits.Count() >= m_iMaxHits)
		{
			Print("[CGQC_TargetHit] Hit limit " + m_iMaxHits.ToString() + " reached — ignoring.", LogLevel.NORMAL);
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return;

		float dist = 0;
		IEntity shooterChar = GetCharacterForPlayer(playerID);
		if (shooterChar)
			dist = vector.Distance(shooterChar.GetOrigin(), owner.GetOrigin());

		m_aHits.Insert(new CGQC_RangeTools_HitData(playerID, velocityMS, dist, worldHitPos));

		Print("[CGQC_TargetHit] Hit #" + m_aHits.Count().ToString()
			+ " | pid=" + playerID.ToString()
			+ " | vel=" + velocityMS.ToString()
			+ " | dist=" + dist.ToString()
			+ " | pos=" + worldHitPos.ToString(), LogLevel.NORMAL);
	}

	// ================================================================
	// Server_Reset
	// ================================================================
	void Server_Reset()
	{
		if (!IsAuthority())
		{
			Print("[CGQC_TargetHit] Server_Reset called on proxy — ignoring.", LogLevel.WARNING);
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return;

		Print("[CGQC_TargetHit] Server_Reset | clearing " + m_aHits.Count().ToString() + " hits.", LogLevel.NORMAL);

		if (m_sPrefabPath == string.Empty)
		{
			Print("[CGQC_TargetHit] Server_Reset: m_sPrefabPath not set — clearing hits only.", LogLevel.WARNING);
			m_aHits.Clear();
			return;
		}

		// Store everything in static helper before delete — same pattern as CGQC_TargetMoverComponent
		vector mat[4];
		owner.GetWorldTransform(mat);

		CGQC_RangeTools_ResetHelper.s_sPrefab     = m_sPrefabPath;
		CGQC_RangeTools_ResetHelper.s_vTransform  = mat;

		m_aHits.Clear();

		// Delete first, then spawn via CallLater — owner/this are gone after this line
		SCR_EntityHelper.DeleteEntityAndChildren(owner);
		GetGame().GetCallqueue().CallLater(CGQC_RangeTools_ResetHelper.DoReset, 100, false);
	}

	// DeferredDelete no longer needed — kept as stub for safety
	protected void DeferredDelete(IEntity ent) {}

	// ================================================================
	// Server_Check
	// ================================================================
	void Server_Check(int requestingPlayerID)
	{
		if (!IsAuthority())
		{
			Print("[CGQC_TargetHit] Server_Check called on proxy — ignoring.", LogLevel.WARNING);
			return;
		}

		Print("[CGQC_TargetHit] Server_Check | pid=" + requestingPlayerID.ToString()
			+ " | hits=" + m_aHits.Count().ToString(), LogLevel.NORMAL);

		string reportText = BuildReport(requestingPlayerID);

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(requestingPlayerID);
		if (!pc)
		{
			Print("[CGQC_TargetHit] Server_Check: no PlayerController for pid " + requestingPlayerID.ToString(), LogLevel.ERROR);
			return;
		}

		CGQC_RangeTools_PlayerReportComponent prc =
			CGQC_RangeTools_PlayerReportComponent.Cast(pc.FindComponent(CGQC_RangeTools_PlayerReportComponent));
		if (!prc)
		{
			Print("[CGQC_TargetHit] Server_Check: CGQC_RangeTools_PlayerReportComponent not found on PlayerController "
				+ "for pid " + requestingPlayerID.ToString()
				+ " — add it to the PlayerController prefab.", LogLevel.ERROR);
			return;
		}

		prc.Rpc_ShowTargetReport(reportText);
	}

	// ================================================================
	// BuildReport
	// ================================================================
	protected string BuildReport(int requestingPlayerID)
	{
		if (m_aHits.IsEmpty())
		{
			Print("[CGQC_TargetHit] BuildReport: no hits.", LogLevel.NORMAL);
			return "CGQC Range Tools\nNo hits recorded on this target.";
		}

		// Header uses last hit's player
		int lastPID = m_aHits[m_aHits.Count() - 1].m_iPlayerID;
		string playerName = GetGame().GetPlayerManager().GetPlayerName(lastPID);
		string weaponName = "Unknown";
		string magName    = "Unknown";

		IEntity shooterChar = GetCharacterForPlayer(lastPID);
		if (shooterChar)
		{
			BaseWeaponManagerComponent wm =
				BaseWeaponManagerComponent.Cast(shooterChar.FindComponent(BaseWeaponManagerComponent));
			if (wm)
			{
				WeaponSlotComponent ws = wm.GetCurrentSlot();
				if (ws)
				{
					IEntity weaponEnt = ws.GetWeaponEntity();
					BaseWeaponComponent weaponComp = null;
					if (weaponEnt)
						weaponComp = BaseWeaponComponent.Cast(weaponEnt.FindComponent(BaseWeaponComponent));
					if (weaponComp)
					{
						// GetUIInfo() is on BaseWeaponComponent directly
						UIInfo wUI = weaponComp.GetUIInfo();
						if (wUI) weaponName = wUI.GetName();

						// GetCurrentMagazine() is also on BaseWeaponComponent directly
						BaseMagazineComponent magComp = weaponComp.GetCurrentMagazine();
						if (magComp)
						{
							InventoryItemComponent mInv =
								InventoryItemComponent.Cast(magComp.GetOwner().FindComponent(InventoryItemComponent));
							if (mInv)
							{
								UIInfo mUI = mInv.GetUIInfo();
								if (mUI) magName = mUI.GetName();
							}
						}
					}
				}
			}
		}

		Print("[CGQC_TargetHit] BuildReport | player=" + playerName
			+ " | weapon=" + weaponName + " | mag=" + magName, LogLevel.NORMAL);

		float shotDistM = m_aHits[0].m_fDistance;

		// Project hits onto target local right/up axes for group size
		IEntity owner = GetOwner();
		vector targetRight = vector.Right;
		vector targetUp    = vector.Up;
		vector targetPos   = vector.Zero;

		if (owner)
		{
			vector mat[4];
			owner.GetWorldTransform(mat);
			targetRight = mat[0];
			targetUp    = mat[1];
			targetPos   = mat[3];
		}

		float minR =  99999.0;
		float maxR = -99999.0;
		float minU =  99999.0;
		float maxU = -99999.0;

		int hitCount = m_aHits.Count();
		for (int i = 0; i < hitCount; i++)
		{
			vector offset = m_aHits[i].m_vWorldPos - targetPos;
			float r = vector.Dot(offset, targetRight);
			float u = vector.Dot(offset, targetUp);
			if (r < minR) minR = r;
			if (r > maxR) maxR = r;
			if (u < minU) minU = u;
			if (u > maxU) maxU = u;
		}

		float groupW  = maxR - minR;
		float groupH  = maxU - minU;
		float groupM  = Math.Sqrt(groupW * groupW + groupH * groupH);
		float groupIn = groupM * 39.3701;

		float moaValue = 0.0;
		if (shotDistM > 0.1)
		{
			float distYards = shotDistM / 0.9144;
			if (distYards > 0.0)
				moaValue = (groupIn / distYards) * 100.0;
		}

		Print("[CGQC_TargetHit] BuildReport | dist=" + shotDistM.ToString()
			+ " | groupM=" + groupM.ToString()
			+ " | groupIn=" + groupIn.ToString()
			+ " | MOA=" + moaValue.ToString(), LogLevel.NORMAL);

		// Assemble report string — no method chaining on float returns
		int distRounded = (int)shotDistM;
		string nl = "\n";
		string reportText = "=== CGQC Range Report ===" + nl;
		reportText = reportText + playerName + " | " + weaponName + " | " + magName + nl;
		reportText = reportText + "Distance: " + distRounded.ToString() + " m | Group: " + FloatStr(groupIn, 2) + " in" + nl;
		reportText = reportText + FloatStr(moaValue, 2) + " MOA" + nl;
		reportText = reportText + "------------------------" + nl;

		int maxShow = hitCount;
		if (maxShow > 10)
			maxShow = 10;

		for (int i = 0; i < maxShow; i++)
		{
			CGQC_RangeTools_HitData h = m_aHits[i];
			int shotNum = i + 1;
			int velWhole = (int)h.m_fVelocity;
			int velFrac  = (int)((h.m_fVelocity - velWhole) * 10.0);
			if (velFrac < 0) velFrac = -velFrac;
			reportText = reportText + "Hit " + shotNum.ToString()
				+ " | " + velWhole.ToString() + "." + velFrac.ToString() + " m/s" + nl;
		}

		if (hitCount > maxShow)
		{
			int remaining = hitCount - maxShow;
			reportText = reportText + "(+" + remaining.ToString() + " more)" + nl;
		}

		return reportText;
	}

	// ================================================================
	// Helpers
	// ================================================================
	protected IEntity GetCharacterForPlayer(int playerID)
	{
		if (playerID <= 0) return null;
		return GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
	}

	// Float to N decimal places without chaining on return value
	protected string FloatStr(float v, int decimals)
	{
		int whole = (int)v;
		float factor = 1.0;
		int d = decimals;
		while (d > 0)
		{
			factor = factor * 10.0;
			d--;
		}
		int frac = (int)((v - whole) * factor);
		if (frac < 0) frac = -frac;
		string fracStr = frac.ToString();
		while (fracStr.Length() < decimals)
			fracStr = "0" + fracStr;
		return whole.ToString() + "." + fracStr;
	}

	protected string VecStr(vector v)
	{
		int x = (int)v[0];
		int y = (int)v[1];
		int z = (int)v[2];
		return "(" + x.ToString() + ", " + y.ToString() + ", " + z.ToString() + ")";
	}
}


// ============================================================
// Damage manager
// Replaces SCR_DamageManagerComponent on the target prefab.
// Subscribes to the damage started invoker to capture bullet hits.
// ============================================================
class CGQC_RangeTools_TargetDamageManagerClass : SCR_DamageManagerComponentClass {}

class CGQC_RangeTools_TargetDamageManager : SCR_DamageManagerComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode())
			return;
		Print("[CGQC_TargetDmg] OnPostInit on: " + owner.GetName(), LogLevel.NORMAL);
	}

	// Override OnDamage — called during damage processing, context fully populated
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rpl && rpl.IsProxy())
			return;

		// hitPosition and impactVelocity are live here
		vector impactPos = damageContext.hitPosition;
		float velocity   = damageContext.impactVelocity.Length();

		int playerID = -1;
		Instigator instigator = damageContext.instigator;
		if (instigator)
		{
			IEntity instigEnt = instigator.GetInstigatorEntity();
			if (instigEnt)
			{
				playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(instigEnt);
				if (playerID <= 0)
				{
					IEntity parent = instigEnt.GetParent();
					if (parent)
						playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(parent);
				}
			}
		}

		Print("[CGQC_TargetDmg] OnDamage | vel=" + velocity.ToString()
			+ " | pid=" + playerID.ToString()
			+ " | pos=" + impactPos.ToString(), LogLevel.NORMAL);

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(GetOwner().FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (hitComp)
			hitComp.RegisterHit(playerID, velocity, impactPos);
		else
			Print("[CGQC_TargetDmg] TargetHitComponent not found on owner!", LogLevel.ERROR);
	}
}

// ============================================================
// User action: Reset Target
// ============================================================
class CGQC_RangeTools_ResetTargetAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		Print("[CGQC_ResetAction] PerformAction called.", LogLevel.NORMAL);

		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		if (pid <= 0)
		{
			Print("[CGQC_ResetAction] Could not resolve playerID.", LogLevel.WARNING);
			return;
		}

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(pOwnerEntity.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp)
		{
			Print("[CGQC_ResetAction] TargetHitComponent not found on owner!", LogLevel.ERROR);
			return;
		}

		RplComponent rpl = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		bool isAuthority = (!rpl || !rpl.IsProxy());
		Print("[CGQC_ResetAction] isAuthority=" + isAuthority.ToString(), LogLevel.NORMAL);

		if (isAuthority)
		{
			hitComp.Server_Reset();
		}
		else
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(pid);
			if (!pc)
			{
				Print("[CGQC_ResetAction] No PlayerController for pid " + pid.ToString(), LogLevel.ERROR);
				return;
			}
			CGQC_RangeTools_PlayerReportComponent prc =
				CGQC_RangeTools_PlayerReportComponent.Cast(pc.FindComponent(CGQC_RangeTools_PlayerReportComponent));
			if (!prc)
			{
				Print("[CGQC_ResetAction] PlayerReportComponent missing on PlayerController!", LogLevel.ERROR);
				return;
			}
			RplId targetId = Replication.FindId(pOwnerEntity);
			prc.RpcAsk_ResetTarget(targetId);
		}
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}


// ============================================================
// User action: Check Target
// ============================================================
class CGQC_RangeTools_CheckTargetAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		Print("[CGQC_CheckAction] PerformAction called.", LogLevel.NORMAL);

		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		if (pid <= 0)
		{
			Print("[CGQC_CheckAction] Could not resolve playerID.", LogLevel.WARNING);
			return;
		}

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(pOwnerEntity.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp)
		{
			Print("[CGQC_CheckAction] TargetHitComponent not found on owner!", LogLevel.ERROR);
			return;
		}

		RplComponent rpl = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		bool isAuthority = (!rpl || !rpl.IsProxy());
		Print("[CGQC_CheckAction] pid=" + pid.ToString() + " | isAuthority=" + isAuthority.ToString(), LogLevel.NORMAL);

		if (isAuthority)
		{
			hitComp.Server_Check(pid);
		}
		else
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(pid);
			if (!pc)
			{
				Print("[CGQC_CheckAction] No PlayerController for pid " + pid.ToString(), LogLevel.ERROR);
				return;
			}
			CGQC_RangeTools_PlayerReportComponent prc =
				CGQC_RangeTools_PlayerReportComponent.Cast(pc.FindComponent(CGQC_RangeTools_PlayerReportComponent));
			if (!prc)
			{
				Print("[CGQC_CheckAction] PlayerReportComponent missing on PlayerController!", LogLevel.ERROR);
				return;
			}
			RplId targetId = Replication.FindId(pOwnerEntity);
			prc.RpcAsk_CheckTarget(targetId);
		}
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}


// ============================================================
// PlayerReportComponent
// Plain ScriptComponent on the PlayerController entity.
// Carries client->server and server->owner RPCs for this system.
// ============================================================
class CGQC_RangeTools_PlayerReportComponentClass : ScriptComponentClass {}

class CGQC_RangeTools_PlayerReportComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode())
			return;
		Print("[CGQC_PlayerReport] Attached to: " + owner.GetName(), LogLevel.NORMAL);
	}

	// ---- Client -> Server: reset ----
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ResetTarget(RplId targetId)
	{
		Print("[CGQC_PlayerReport] RpcAsk_ResetTarget received on server.", LogLevel.NORMAL);

		IEntity targetEnt = IEntity.Cast(Replication.FindItem(targetId));
		if (!targetEnt)
		{
			Print("[CGQC_PlayerReport] RpcAsk_ResetTarget: entity not found for RplId.", LogLevel.ERROR);
			return;
		}

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(targetEnt.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp)
		{
			Print("[CGQC_PlayerReport] RpcAsk_ResetTarget: no TargetHitComponent on entity.", LogLevel.ERROR);
			return;
		}
		hitComp.Server_Reset();
	}

	// ---- Client -> Server: check ----
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_CheckTarget(RplId targetId)
	{
		Print("[CGQC_PlayerReport] RpcAsk_CheckTarget received on server.", LogLevel.NORMAL);

		IEntity targetEnt = IEntity.Cast(Replication.FindItem(targetId));
		if (!targetEnt)
		{
			Print("[CGQC_PlayerReport] RpcAsk_CheckTarget: entity not found.", LogLevel.ERROR);
			return;
		}

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(targetEnt.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp)
		{
			Print("[CGQC_PlayerReport] RpcAsk_CheckTarget: no TargetHitComponent on entity.", LogLevel.ERROR);
			return;
		}

		int pid = -1;
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (pc)
		{
			IEntity controlled = pc.GetControlledEntity();
			if (controlled)
				pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(controlled);
		}

		Print("[CGQC_PlayerReport] RpcAsk_CheckTarget: resolved pid=" + pid.ToString(), LogLevel.NORMAL);

		if (pid > 0)
			hitComp.Server_Check(pid);
	}

	// ---- Server -> Owner client: show hint ----
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void Rpc_ShowTargetReport(string reportText)
	{
		Print("[CGQC_PlayerReport] Rpc_ShowTargetReport received on client.", LogLevel.NORMAL);

		SCR_HintManagerComponent hm = SCR_HintManagerComponent.GetInstance();
		if (!hm)
		{
			Print("[CGQC_PlayerReport] SCR_HintManagerComponent instance is null!", LogLevel.ERROR);
			return;
		}
		hm.ShowCustomHint(reportText, "CGQC Range Report", 15.0);
	}
}