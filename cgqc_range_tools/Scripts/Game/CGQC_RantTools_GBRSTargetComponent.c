// ============================================================
// CGQC_RangeTools_GBRSTargetHitComponent.c
// GBRS Group circle target + Height-Over-Bore zone detection
// No scoring - just zone identification
// ============================================================


// ----------------------------------------------------------------
// Hit effect for GBRS target - fires CLIENT-SIDE
// ----------------------------------------------------------------
class CGQC_RangeTools_GBRSTargetHitEffect : BaseProjectileEffect
{
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, notnull Instigator instigator, string colliderName, float speed)
	{
		if (!pHitEntity)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		IEntity localPlayer = pc.GetControlledEntity();
		if (!localPlayer || instigator.GetInstigatorEntity() != localPlayer)
			return;

		CGQC_RangeTools_GBRSTargetHitComponent hitComp =
			CGQC_RangeTools_GBRSTargetHitComponent.Cast(pHitEntity.FindComponent(CGQC_RangeTools_GBRSTargetHitComponent));
		if (!hitComp)
			return;

		float distance = vector.Distance(localPlayer.GetOrigin(), pHitEntity.GetOrigin()) - 1.0;
		int distM  = (int)distance;
		int velMs  = (int)speed;

		SCR_HintManagerComponent hm = SCR_HintManagerComponent.GetInstance();
		if (hm)
			hm.ShowCustomHint("Distance: " + distM.ToString() + "m\nVelocite: " + velMs.ToString() + " m/s", "Impact!", 2.5);
	}
}


// ----------------------------------------------------------------
// Zone names
// ----------------------------------------------------------------
// TL = Top-Left group, TR = Top-Right group
// BL = Bottom-Left group, BR = Bottom-Right group
// HOB = Height-Over-Bore center rectangle
// Format: "TR-1i" = Top-Right circle 1 inner ring
//         "TR-1o" = Top-Right circle 1 outer ring
//         "M"     = Miss


// ----------------------------------------------------------------
// Per-shot data
// ----------------------------------------------------------------
class CGQC_RangeTools_GBRSHitData
{
	int		m_iPlayerID;
	float	m_fVelocityMS;
	float	m_fDistance;
	string	m_sZone;

	void CGQC_RangeTools_GBRSHitData(int pid, float vel, float dist, string zone)
	{
		m_iPlayerID   = pid;
		m_fVelocityMS = vel;
		m_fDistance   = dist;
		m_sZone       = zone;
	}
}


// ----------------------------------------------------------------
// Circle definition - center + outer radius + inner radius
// ----------------------------------------------------------------
class CGQC_RangeTools_GBRSCircle
{
	string	m_sName;
	float	m_fCX;		// local X center
	float	m_fCY;		// local Y center
	float	m_fROuter;	// outer radius
	float	m_fRInner;	// inner ring radius (0 = no inner)

	void CGQC_RangeTools_GBRSCircle(string name, float cx, float cy, float rOuter, float rInner)
	{
		m_sName   = name;
		m_fCX     = cx;
		m_fCY     = cy;
		m_fROuter = rOuter;
		m_fRInner = rInner;
	}
}


// ----------------------------------------------------------------
// Main component
// ----------------------------------------------------------------
class CGQC_RangeTools_GBRSTargetHitComponentClass : ScriptComponentClass {}

class CGQC_RangeTools_GBRSTargetHitComponent : ScriptComponent
{
	// --- Configurable per prefab ---
	[Attribute("10", UIWidgets.EditBox, "Max hits to record")]
	protected int m_iMaxHits;

	[Attribute("", UIWidgets.EditBox, "Prefab path for reset respawn")]
	protected string m_sPrefabPath;

	// Large circle outer radius (4.7in = 0.119m, radius = 0.060m)
	protected float OUTER_R  = 0.064;
	// Inner ring radius (2.35in = 0.060m, radius = 0.030m)
	protected float INNER_R  = 0.023;
	// Small circles 5/6 radius
	protected float SMALL_R  = 0.020;
	// HOB rectangle half-width and half-height
	protected float HOB_HW   = 0.055;
	protected float HOB_HH   = 0.100;

	// Anchor: top-LEFT circle 1 center from shot data
	// TL-C1: X=+0.281, Y=1.736 (calibrated from live hits)
	protected float TL1_X =  0.2834;
	protected float TL1_Y =  1.7825;

	// Offsets within a group
	protected float DX12  = -0.1206;	// C1 to C2 horizontal (left = more negative X)
	protected float DY13  = -0.1417;	// C1 to C3 vertical (down)
	protected float DY56  = -0.0933;	// C3 to C5/C6 vertical (below C3)

	// Group offsets
	protected float GRP_LR_DX = -0.450;	// TR group X offset from TL group (negative = right side)
	protected float GRP_TB_DY = -0.680;	// bottom group Y offset from top group

	// HOB center (calibrated: X=0.003, Y=1.340)
	protected float HOB_X =  0.003;
	protected float HOB_Y =  1.340;

	protected ref array<ref CGQC_RangeTools_GBRSHitData> m_aHits = new array<ref CGQC_RangeTools_GBRSHitData>();
	protected RplComponent m_RplComp;

	[RplProp()]
	protected string m_sLastReport = string.Empty;

	[RplProp()]
	protected string m_sLastReportTitle = string.Empty;

	// ----------------------------------------------------------------
	// Build circle list
	// ----------------------------------------------------------------
	protected ref array<ref CGQC_RangeTools_GBRSCircle> m_aCircles;

	protected void BuildCircles()
	{
		m_aCircles = new array<ref CGQC_RangeTools_GBRSCircle>();

		// Helper to add a group of 4+2 circles
		// grpName: "TR","TL","BR","BL"
		// baseX, baseY: center of circle 1
		// hasC4: top groups have C4, bottom groups don't
		// has56: top groups have 5/6, bottom groups don't

		// Top-Left
		AddGroup("TL", TL1_X, TL1_Y, true, true);
		// Top-Right (negative X from TL)
		AddGroup("TR", TL1_X + GRP_LR_DX, TL1_Y, true, true);
		// Bottom-Left
		AddGroup("BL", TL1_X, TL1_Y + GRP_TB_DY, false, false);
		// Bottom-Right
		AddGroup("BR", TL1_X + GRP_LR_DX, TL1_Y + GRP_TB_DY, false, false);
	}

	protected void AddGroup(string grp, float bx, float by, bool hasC4, bool has56)
	{
		// C1
		m_aCircles.Insert(new CGQC_RangeTools_GBRSCircle(grp + "-1", bx, by, OUTER_R, INNER_R));
		// C2
		m_aCircles.Insert(new CGQC_RangeTools_GBRSCircle(grp + "-2", bx + DX12, by, OUTER_R, INNER_R));
		// C3
		m_aCircles.Insert(new CGQC_RangeTools_GBRSCircle(grp + "-3", bx, by + DY13, OUTER_R, INNER_R));

		if (hasC4)
			m_aCircles.Insert(new CGQC_RangeTools_GBRSCircle(grp + "-4", bx + DX12, by + DY13, OUTER_R, INNER_R));

		if (has56)
		{
			float c56y = by + DY13 + DY56;
			m_aCircles.Insert(new CGQC_RangeTools_GBRSCircle(grp + "-5", bx,        c56y, SMALL_R, 0));
			m_aCircles.Insert(new CGQC_RangeTools_GBRSCircle(grp + "-6", bx + DX12, c56y, SMALL_R, 0));
		}
	}

	// ----------------------------------------------------------------
	// Init
	// ----------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_RplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
		BuildCircles();
	}

	// ----------------------------------------------------------------
	// Zone detection
	// ----------------------------------------------------------------
	protected string DetectZone(vector worldHitPos, IEntity owner)
	{
		vector mat[4];
		owner.GetWorldTransform(mat);

		vector localPos = worldHitPos - mat[3];
		float lx = vector.Dot(localPos, mat[0]);
		float ly = vector.Dot(localPos, mat[1]);


		// HOB rectangle check first
		if (Math.AbsFloat(lx - HOB_X) <= HOB_HW && Math.AbsFloat(ly - HOB_Y) <= HOB_HH)
			return "HOB";

		// Check each circle
		for (int i = 0; i < m_aCircles.Count(); i++)
		{
			CGQC_RangeTools_GBRSCircle c = m_aCircles[i];
			float dx   = lx - c.m_fCX;
			float dy   = ly - c.m_fCY;
			float dist = Math.Sqrt(dx * dx + dy * dy);

			if (dist <= c.m_fROuter)
			{
				if (c.m_fRInner > 0 && dist <= c.m_fRInner)
					return c.m_sName + "i";
				else
					return c.m_sName + "o";
			}
		}

		return "M";
	}

	// ----------------------------------------------------------------
	// Register hit (called from DamageManager, server only)
	// ----------------------------------------------------------------
	void RegisterHit(int playerID, float velocityMS, float distance, vector worldHitPos)
	{
		string zone = DetectZone(worldHitPos, GetOwner());

		// RPC hit hint to shooter - only if distance > 100m
		if (playerID > 0 && distance > 100.0)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerID);
			if (pc)
			{
				SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
				if (spc)
				{
					int distInt = (int)distance;
					int velInt  = (int)velocityMS;
					spc.CGQC_Rpc_ShowHitHint(0, zone, 0, 0, distInt, velInt, false);
				}
			}
		}
	}

	// ----------------------------------------------------------------
	// Build report
	// ----------------------------------------------------------------
	protected string BuildReportTitle(int pid)
	{
		string playerName = "Inconnu";
		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm) playerName = pm.GetPlayerName(pid);

		return playerName + " | GBRS";
	}

	protected string BuildReport(int pid)
	{
		int count = m_aHits.Count();
		if (count == 0)
			return "Aucun impact enregistre.";

		string sep = "--------------------------------";
		string body = sep + "\n";

		for (int i = 0; i < count; i++)
		{
			CGQC_RangeTools_GBRSHitData h = m_aHits[i];
			int velMs  = (int)h.m_fVelocityMS;
			int distM  = (int)h.m_fDistance;
			int num    = i + 1;

			string line = " " + num.ToString() + ". " + h.m_sZone
				+ " - " + distM.ToString() + "m"
				+ " - " + velMs.ToString() + " m/s";
			body = body + line + "\n";
		}

		body = body + sep + "\n";
		body = body + count.ToString() + " tirs";

		return body;
	}

	// ----------------------------------------------------------------
	// Check / Reset
	// ----------------------------------------------------------------
	void Server_Check(int requestingPlayerID)
	{
		m_sLastReport      = BuildReport(requestingPlayerID);
		m_sLastReportTitle = BuildReportTitle(requestingPlayerID);
		Replication.BumpMe();

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(requestingPlayerID);
		if (!pc) return;
		SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
		if (!spc) return;
		spc.CGQC_Rpc_ShowTargetReport(m_sLastReport, m_sLastReportTitle);
	}

	void Server_Reset(int requestingPlayerID)
	{
		m_aHits.Clear();
		m_sLastReport      = string.Empty;
		m_sLastReportTitle = string.Empty;
		Replication.BumpMe();

		// Respawn target
		if (m_sPrefabPath.IsEmpty())
			return;

		IEntity owner = GetOwner();
		if (!owner) return;

		vector mat[4];
		owner.GetWorldTransform(mat);

		CGQC_RangeTools_ResetHelper.s_sPrefab    = m_sPrefabPath;
		CGQC_RangeTools_ResetHelper.s_vTransform = mat;

		GetGame().GetCallqueue().CallLater(CGQC_RangeTools_ResetHelper.DoReset, 100, false);
		SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}

	string GetLastReport()      { return m_sLastReport; }
	string GetLastReportTitle() { return m_sLastReportTitle; }
}


// ----------------------------------------------------------------
// Damage manager for GBRS target
// ----------------------------------------------------------------
class CGQC_RangeTools_GBRSTargetDamageManagerClass : SCR_DamageManagerComponentClass {}

class CGQC_RangeTools_GBRSTargetDamageManager : SCR_DamageManagerComponent
{
	override void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		if (!Replication.IsServer())
			return;

		IEntity owner = GetOwner();
		if (!owner) return;

		CGQC_RangeTools_GBRSTargetHitComponent hitComp =
			CGQC_RangeTools_GBRSTargetHitComponent.Cast(owner.FindComponent(CGQC_RangeTools_GBRSTargetHitComponent));
		if (!hitComp) return;

		float vel = damageContext.impactVelocity.Length();
		vector hitPos = damageContext.hitPosition;

		Instigator instigator = damageContext.instigator;
		IEntity instigatorEnt = instigator.GetInstigatorEntity();
		if (!instigatorEnt) return;

		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(instigatorEnt);
		if (pid <= 0) return;

		float dist = vector.Distance(instigatorEnt.GetOrigin(), owner.GetOrigin());


		hitComp.RegisterHit(pid, vel, dist, hitPos);
	}
}


// ----------------------------------------------------------------
// Check action for GBRS target
// ----------------------------------------------------------------
class CGQC_RangeTools_GBRSCheckAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_RangeTools_GBRSTargetHitComponent hitComp =
			CGQC_RangeTools_GBRSTargetHitComponent.Cast(pOwnerEntity.FindComponent(CGQC_RangeTools_GBRSTargetHitComponent));
		if (!hitComp) return;

		if (Replication.IsServer())
		{
			int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
			if (pid <= 0) return;
			hitComp.Server_Check(pid);
		}
		else
		{
			string report = hitComp.GetLastReport();
			string title  = hitComp.GetLastReportTitle();
			if (report.IsEmpty()) report = "Aucun impact enregistre.";
			SCR_HintManagerComponent hm = SCR_HintManagerComponent.GetInstance();
			if (hm) hm.ShowCustomHint(report, title, 15.0);
		}
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return true; }
}


// ----------------------------------------------------------------
// Reset action for GBRS target
// ----------------------------------------------------------------
class CGQC_RangeTools_GBRSResetAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer()) return;

		CGQC_RangeTools_GBRSTargetHitComponent hitComp =
			CGQC_RangeTools_GBRSTargetHitComponent.Cast(pOwnerEntity.FindComponent(CGQC_RangeTools_GBRSTargetHitComponent));
		if (!hitComp) return;

		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		hitComp.Server_Reset(pid);
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}