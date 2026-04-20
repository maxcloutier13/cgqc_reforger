// ============================================================
// CGQC_RangeTools_TargetHitComponent.c
// ============================================================


class CGQC_RangeTools_TargetHitEffect : BaseProjectileEffect
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

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(pHitEntity.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp)
			return;

		float distance = vector.Distance(localPlayer.GetOrigin(), pHitEntity.GetOrigin()) - 1.0;

		if (distance <= 100.0)
			return;

		// Nothing to do client-side - server RPC handles the full hint
	}
}


// ----------------------------------------------------------------
// Static helper for reset
// ----------------------------------------------------------------
class CGQC_RangeTools_ResetHelper
{
	static ResourceName s_sPrefab;
	static vector s_vTransform[4];
	static string s_sName;
	static float s_fBaseX;
	static int s_iDistanceIndex;
	static bool s_bCheckMode;

	static void DoReset()
	{
		Resource res = Resource.Load(s_sPrefab);
		if (!res || !res.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = s_vTransform[0];
		params.Transform[1] = s_vTransform[1];
		params.Transform[2] = s_vTransform[2];
		params.Transform[3] = s_vTransform[3];

		IEntity newEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!newEnt)
			return;

		if (!s_sName.IsEmpty())
			newEnt.SetName(s_sName);

		CGQC_TargetMoverComponent newMover = CGQC_TargetMoverComponent.Cast(
			newEnt.FindComponent(CGQC_TargetMoverComponent)
		);
		if (newMover)
		{
			newMover.SetBaseX(s_fBaseX);
			newMover.SetDistanceIndex(s_iDistanceIndex);
			newMover.SetCheckMode(s_bCheckMode);
			
		}
	}
}


// ----------------------------------------------------------------
// Per-shot data record
// ----------------------------------------------------------------
class CGQC_RangeTools_HitData
{
	int		m_iPlayerID;
	float	m_fVelocity;
	float	m_fDistance;
	vector	m_vWorldPos;
	string	m_sZone;
	int		m_iPoints;

	void CGQC_RangeTools_HitData(int pid, float vel, float dist, vector wpos, string zone, int points)
	{
		m_iPlayerID = pid;
		m_fVelocity = vel;
		m_fDistance = dist;
		m_vWorldPos = wpos;
		m_sZone     = zone;
		m_iPoints   = points;
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
		desc: "Prefab to re-spawn on Reset", params: "et")]
	protected ResourceName m_sPrefabPath;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Maximum shots tracked")]
	protected int m_iMaxHits;

	[Attribute(defvalue: "0.105", uiwidget: UIWidgets.EditBox, desc: "A2 torso half-width (X)")]
	protected float m_fAZoneHalfX;

	[Attribute(defvalue: "1.22", uiwidget: UIWidgets.EditBox, desc: "A2 torso bottom Y")]
	protected float m_fATorsoYMin;

	[Attribute(defvalue: "1.62", uiwidget: UIWidgets.EditBox, desc: "A2 torso top Y")]
	protected float m_fATorsoYMax;

	[Attribute(defvalue: "1.68", uiwidget: UIWidgets.EditBox, desc: "A1 head bottom Y")]
	protected float m_fAHeadYMin;

	[Attribute(defvalue: "1.88", uiwidget: UIWidgets.EditBox, desc: "A1 head top Y")]
	protected float m_fAHeadYMax;

	[Attribute(defvalue: "0.105", uiwidget: UIWidgets.EditBox, desc: "A1 head half-width (X)")]
	protected float m_fAHeadHalfX;

	[Attribute(defvalue: "0.28", uiwidget: UIWidgets.EditBox, desc: "C upper half-width (X)")]
	protected float m_fCZoneHalfX;

	[Attribute(defvalue: "1.62", uiwidget: UIWidgets.EditBox, desc: "C upper bottom Y")]
	protected float m_fCZoneYMin;

	[Attribute(defvalue: "1.88", uiwidget: UIWidgets.EditBox, desc: "C upper top Y")]
	protected float m_fCZoneYMax;

	[Attribute(defvalue: "0.19", uiwidget: UIWidgets.EditBox, desc: "C lower half-width (X)")]
	protected float m_fCLowerHalfX;

	[Attribute(defvalue: "0.92", uiwidget: UIWidgets.EditBox, desc: "C lower bottom Y")]
	protected float m_fCLowerYMin;

	[Attribute(defvalue: "1.62", uiwidget: UIWidgets.EditBox, desc: "C lower top Y")]
	protected float m_fCLowerYMax;

	[Attribute(defvalue: "0.32", uiwidget: UIWidgets.EditBox, desc: "D zone half-width (X)")]
	protected float m_fDZoneHalfX;

	[Attribute(defvalue: "0.92", uiwidget: UIWidgets.EditBox, desc: "D zone bottom Y")]
	protected float m_fDZoneYMin;

	[Attribute(defvalue: "1.62", uiwidget: UIWidgets.EditBox, desc: "D zone top Y")]
	protected float m_fDZoneYMax;

	protected ref array<ref CGQC_RangeTools_HitData> m_aHits = new array<ref CGQC_RangeTools_HitData>();
	protected RplComponent m_RplComp;

	[RplProp()]
	protected string m_sLastReport = string.Empty;

	[RplProp()]
	protected string m_sLastReportTitle = string.Empty;

	// ----------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode())
			return;
		m_RplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
	}

	protected bool IsAuthority()
	{
		return !m_RplComp || !m_RplComp.IsProxy();
	}

	// ----------------------------------------------------------------
	// Accessors for BaseProjectileEffect (client-side, reads proxy data)
	// ----------------------------------------------------------------
	int GetLocalHitCount()
	{
		return m_aHits.Count();
	}

	int GetLocalTotalPoints()
	{
		int total = 0;
		for (int i = 0; i < m_aHits.Count(); i++)
			total += m_aHits[i].m_iPoints;
		return total;
	}

	string GetLastReport()      { return m_sLastReport; }
	string GetLastReportTitle() { return m_sLastReportTitle; }

	// ================================================================
	// GetZone
	// ================================================================
	string GetZone(vector worldHitPos)
	{
		IEntity owner = GetOwner();
		if (!owner)
			return "M";

		vector mat[4];
		owner.GetWorldTransform(mat);
		vector offset = worldHitPos - mat[3];
		float localX = vector.Dot(offset, mat[0]);
		float localY = vector.Dot(offset, mat[1]);

		if (localY >= m_fAHeadYMin && localY <= m_fAHeadYMax && Math.AbsFloat(localX) <= m_fAHeadHalfX)
			return "A1";
		if (localY >= m_fATorsoYMin && localY <= m_fATorsoYMax && Math.AbsFloat(localX) <= m_fAZoneHalfX)
			return "A2";
		if (localY >= m_fCZoneYMin && localY <= m_fCZoneYMax && Math.AbsFloat(localX) <= m_fCZoneHalfX)
			return "C";
		if (localY >= m_fCLowerYMin && localY <= m_fCLowerYMax && Math.AbsFloat(localX) <= m_fCLowerHalfX)
			return "C";
		if (localY >= m_fDZoneYMin && localY <= m_fDZoneYMax && Math.AbsFloat(localX) <= m_fDZoneHalfX)
			return "D";

		return "M";
	}

	// ================================================================
	// GetZonePoints
	// ================================================================
	int GetZonePoints(string zone)
	{
		if (zone == "A1" || zone == "A2") return 5;
		if (zone == "C") return 3;
		if (zone == "D") return 1;
		return 0;
	}

	// ================================================================
	// RegisterHit - server only
	// ================================================================
	void RegisterHit(int playerID, float velocityMS, vector worldHitPos)
	{
		if (!IsAuthority())
			return;

		if (m_aHits.Count() >= m_iMaxHits)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		float dist = 0;
		IEntity shooterChar = GetCharacterForPlayer(playerID);
		if (shooterChar)
			dist = vector.Distance(shooterChar.GetOrigin(), owner.GetOrigin());

		string zone = GetZone(worldHitPos);
		int points = GetZonePoints(zone);
		m_aHits.Insert(new CGQC_RangeTools_HitData(playerID, velocityMS, dist, worldHitPos, zone, points));

		int hitNum = m_aHits.Count();
		int totalPoints = 0;
		for (int i = 0; i < hitNum; i++)
			totalPoints += m_aHits[i].m_iPoints;

		// Zone debug
		vector matDbg[4];
		owner.GetWorldTransform(matDbg);
		vector offsetDbg = worldHitPos - matDbg[3];
		float lx = vector.Dot(offsetDbg, matDbg[0]);
		float ly = vector.Dot(offsetDbg, matDbg[1]);

		// Rebuild replicated report so client can read it immediately on Check
		m_sLastReport      = BuildReport(playerID);
		m_sLastReportTitle = BuildReportTitle(playerID);
		Replication.BumpMe();

		// RPC hit hint to shooter - only if distance > 100m
		if (playerID > 0 && dist > 100.0)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerID);
			if (pc)
			{
				SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
				Print("[CGQC_HitHint] spc null=" + (spc == null).ToString() + " pid=" + playerID.ToString(), LogLevel.WARNING);
				if (spc)
				{
					int distInt = (int)dist;
					int velInt  = (int)velocityMS;
					spc.CGQC_Rpc_ShowHitHint(hitNum, zone, points, totalPoints, distInt, velInt, hitNum >= m_iMaxHits);
				}
			}
		}
	}

	// ================================================================
	// Server_Reset
	// ================================================================
	void Server_Reset(int requestingPlayerID = -1)
	{
		if (!IsAuthority())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		if (m_sPrefabPath == string.Empty)
		{
			m_aHits.Clear();
			return;
		}

		vector mat[4];
		owner.GetWorldTransform(mat);
		CGQC_RangeTools_ResetHelper.s_sPrefab    = m_sPrefabPath;
		CGQC_RangeTools_ResetHelper.s_vTransform = mat;
		CGQC_RangeTools_ResetHelper.s_sName      = owner.GetName();

		// Capture mover state before deletion so bay buttons and check mode survive respawn
		CGQC_TargetMoverComponent mover = CGQC_TargetMoverComponent.Cast(
			owner.FindComponent(CGQC_TargetMoverComponent)
		);
		if (mover)
		{
			CGQC_RangeTools_ResetHelper.s_fBaseX         = mover.GetBaseX();
			CGQC_RangeTools_ResetHelper.s_iDistanceIndex = mover.GetDistanceIndex();
			CGQC_RangeTools_ResetHelper.s_bCheckMode     = mover.IsCheckMode();
		}
		else
		{
			CGQC_RangeTools_ResetHelper.s_fBaseX         = mat[3][0];
			CGQC_RangeTools_ResetHelper.s_iDistanceIndex = 0;
			CGQC_RangeTools_ResetHelper.s_bCheckMode     = false;
		}

		m_aHits.Clear();

		// RPC reset hint before entity deletion
		if (requestingPlayerID > 0)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(requestingPlayerID);
			if (pc)
			{
				SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
				if (spc)
					spc.CGQC_Rpc_ShowResetHint();
			}
		}

		SCR_EntityHelper.DeleteEntityAndChildren(owner);
		GetGame().GetCallqueue().CallLater(CGQC_RangeTools_ResetHelper.DoReset, 100, false);
	}

	// ================================================================
	// Server_Check
	// ================================================================
	void Server_Check(int requestingPlayerID)
	{
		if (!IsAuthority())
			return;

		m_sLastReport      = BuildReport(requestingPlayerID);
		m_sLastReportTitle = BuildReportTitle(requestingPlayerID);
		Replication.BumpMe();

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(requestingPlayerID);
		if (!pc)
		{
			return;
		}

		SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
		if (!spc)
		{
			return;
		}

		spc.CGQC_Rpc_ShowTargetReport(m_sLastReport, m_sLastReportTitle);
	}

	// ================================================================
	// BuildReportTitle
	// ================================================================
	protected string BuildReportTitle(int pid)
	{
		string playerName = GetGame().GetPlayerManager().GetPlayerName(pid);
		string weaponName = "Unknown";
		string magName    = "Unknown";

		IEntity ch = GetCharacterForPlayer(pid);
		if (ch)
		{
			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ch.FindComponent(BaseWeaponManagerComponent));
			if (wm)
			{
				WeaponSlotComponent ws = wm.GetCurrentSlot();
				if (ws)
				{
					IEntity wEnt = ws.GetWeaponEntity();
					BaseWeaponComponent wComp = null;
					if (wEnt) wComp = BaseWeaponComponent.Cast(wEnt.FindComponent(BaseWeaponComponent));
					if (wComp)
					{
						UIInfo wUI = wComp.GetUIInfo();
						if (wUI) weaponName = wUI.GetName();
						BaseMagazineComponent mag = wComp.GetCurrentMagazine();
						if (mag)
						{
							InventoryItemComponent mInv = InventoryItemComponent.Cast(mag.GetOwner().FindComponent(InventoryItemComponent));
							if (mInv) { UIInfo mUI = mInv.GetUIInfo(); if (mUI) magName = mUI.GetName(); }
						}
					}
				}
			}
		}

		return playerName + " | " + weaponName;
	}

	// ================================================================
	// BuildReport
	// ================================================================
	protected string BuildReport(int requestingPlayerID)
	{
		if (m_aHits.IsEmpty())
			return "Aucun impact enregistre.";

		float shotDistM = m_aHits[0].m_fDistance;
		int hitCount = m_aHits.Count();

		// Get mag name for first line
		string magName = "Unknown";
		IEntity ch = GetCharacterForPlayer(requestingPlayerID);
		if (ch)
		{
			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ch.FindComponent(BaseWeaponManagerComponent));
			if (wm)
			{
				WeaponSlotComponent ws = wm.GetCurrentSlot();
				if (ws)
				{
					IEntity wEnt = ws.GetWeaponEntity();
					BaseWeaponComponent wComp = null;
					if (wEnt) wComp = BaseWeaponComponent.Cast(wEnt.FindComponent(BaseWeaponComponent));
					if (wComp)
					{
						BaseMagazineComponent mag = wComp.GetCurrentMagazine();
						if (mag)
						{
							InventoryItemComponent mInv = InventoryItemComponent.Cast(mag.GetOwner().FindComponent(InventoryItemComponent));
							if (mInv) { UIInfo mUI = mInv.GetUIInfo(); if (mUI) magName = mUI.GetName(); }
						}
					}
				}
			}
		}

		// Group size
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

		float minR = 99999.0, maxR = -99999.0, minU = 99999.0, maxU = -99999.0;
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

		float groupM  = Math.Sqrt((maxR - minR) * (maxR - minR) + (maxU - minU) * (maxU - minU));
		float groupIn = groupM * 39.3701;
		float groupCm = groupM * 100.0;
		float moaValue = 0.0;
		if (shotDistM > 0.1)
		{
			float distYards = shotDistM / 0.9144;
			if (distYards > 0.0) moaValue = (groupIn / distYards) * 100.0;
		}

		// Chrono stats
		float velSum = 0, velMin = 999999.0, velMax = 0.0;
		int totalScore = 0;
		for (int i = 0; i < hitCount; i++)
		{
			float v = m_aHits[i].m_fVelocity;
			velSum = velSum + v;
			if (v < velMin) velMin = v;
			if (v > velMax) velMax = v;
			totalScore += m_aHits[i].m_iPoints;
		}
		float velAvg  = velSum / hitCount;
		int velAvgMs  = (int)velAvg;
		int velAvgFts = (int)(velAvg * 3.28084);
		int velMinMs  = (int)velMin;
		int velMaxMs  = (int)velMax;
		int extSpread = velMaxMs - velMinMs;
		float variance = 0;
		for (int i = 0; i < hitCount; i++)
		{
			float dv = m_aHits[i].m_fVelocity - velAvg;
			variance = variance + dv * dv;
		}
		int stdDev = (int)Math.Sqrt(variance / hitCount);

		// Date/time
		int year, month, day, hour, minute, second;
		System.GetHourMinuteSecond(hour, minute, second);
		System.GetYearMonthDay(year, month, day);
		string yy = (year % 100).ToString(); if (year % 100 < 10) yy = "0" + yy;
		string mm = month.ToString();        if (month < 10)       mm = "0" + mm;
		string dd = day.ToString();          if (day < 10)         dd = "0" + dd;
		string hh = hour.ToString();         if (hour < 10)        hh = "0" + hh;
		string mn = minute.ToString();       if (minute < 10)      mn = "0" + mn;
		int distRounded = (int)shotDistM;
		string datetime = yy + mm + dd + " - " + hh + ":" + mn + " - Dist: " + distRounded.ToString() + "m - Tgt:IPSC";

		string nl  = "\n";
		string sep = "--------------------------------" + nl;

		string r = datetime + nl;
		r = r + magName + nl;
		r = r + sep;
		r = r + "Groupe: " + FloatStr(groupIn, 2) + " in | " + FloatStr(groupCm, 2) + " cm | " + FloatStr(moaValue, 2) + " MOA" + nl;
		r = r + "Score: " + totalScore.ToString() + " / " + (hitCount * 5).ToString() + " pts" + nl;
		r = r + sep;

		int maxShow = hitCount;
		if (maxShow > 10) maxShow = 10;

		for (int i = 0; i < maxShow; i++)
		{
			CGQC_RangeTools_HitData h = m_aHits[i];
			int shotNum  = i + 1;
			int velWhole = (int)h.m_fVelocity;
			int distM    = (int)h.m_fDistance;

			string numStr = shotNum.ToString();
			if (shotNum < 10) numStr = " " + numStr;

			r = r + numStr + ". " + h.m_sZone + " - " + distM.ToString() + "m - " + velWhole.ToString() + " m/s - " + h.m_iPoints.ToString() + " pts" + nl;
		}

		if (hitCount > maxShow)
		{
			int remaining = hitCount - maxShow;
			r = r + "(+" + remaining.ToString() + " more)" + nl;
		}

		r = r + sep;
		r = r + hitCount.ToString() + " tirs  | MuzzleVel. Moy: " + velAvgMs.ToString() + " m/s | " + velAvgFts.ToString() + " ft/s" + nl;
		r = r + "Min: " + velMinMs.ToString() + " m/s | Max: " + velMaxMs.ToString() + " m/s" + nl;
		r = r + "Spread: " + extSpread.ToString() + " m/s | Ecart type: " + stdDev.ToString() + " m/s" + nl;

		return r;
	}

	// ================================================================
	// Helpers
	// ================================================================
	protected IEntity GetCharacterForPlayer(int playerID)
	{
		if (playerID <= 0) return null;
		return GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
	}

	protected string FloatStr(float v, int decimals)
	{
		int whole = (int)v;
		float factor = 1.0;
		int d = decimals;
		while (d > 0) { factor = factor * 10.0; d--; }
		int frac = (int)((v - whole) * factor);
		if (frac < 0) frac = -frac;
		string fracStr = frac.ToString();
		while (fracStr.Length() < decimals) fracStr = "0" + fracStr;
		return whole.ToString() + "." + fracStr;
	}
}


// ============================================================
// Damage manager - server-side hit tracking only
// ============================================================
class CGQC_RangeTools_TargetDamageManagerClass : SCR_DamageManagerComponentClass {}

class CGQC_RangeTools_TargetDamageManager : SCR_DamageManagerComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode()) return;
	}

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
	    super.OnDamage(damageContext);
	
	    RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
	    if (rpl && rpl.IsProxy()) return;
	
	    vector impactPos = damageContext.hitPosition;
	
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
	                if (parent) playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(parent);
	            }
	        }
	    }
	
	    float muzzleVelocity = 0.0;
	    if (playerID > 0)
	    {
	        IEntity shooterChar = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
	        if (shooterChar)
	        {
	            CGQC_RangeTools_ChronographComponent chrono = CGQC_RangeTools_ChronographComponent.Cast(
	                shooterChar.FindComponent(CGQC_RangeTools_ChronographComponent));
	            if (chrono)
	                muzzleVelocity = chrono.GetLastMuzzleVelocity();
	        }
	    }
	
	    CGQC_RangeTools_TargetHitComponent hitComp =
	        CGQC_RangeTools_TargetHitComponent.Cast(GetOwner().FindComponent(CGQC_RangeTools_TargetHitComponent));
	    if (hitComp)
	        hitComp.RegisterHit(playerID, muzzleVelocity, impactPos);
	}
}


// ============================================================
// User action: Reset
// ============================================================
class CGQC_RangeTools_ResetTargetAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(pOwnerEntity.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp) { return; }

		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		hitComp.Server_Reset(pid);
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}


// ============================================================
// User action: Check
// ============================================================
class CGQC_RangeTools_CheckTargetAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_RangeTools_TargetHitComponent hitComp =
			CGQC_RangeTools_TargetHitComponent.Cast(pOwnerEntity.FindComponent(CGQC_RangeTools_TargetHitComponent));
		if (!hitComp) { return; }

		if (Replication.IsServer())
		{
			// Server: build and replicate the report
			int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
			if (pid <= 0) { return; }
			hitComp.Server_Check(pid);
		}
		else
		{
			// Client: show the last replicated report directly
			string report = hitComp.GetLastReport();
			string title  = hitComp.GetLastReportTitle();
			if (report.IsEmpty()) report = "Aucun impact enregistre.";
			CGQC_RangeTools_HitDisplay.Show(title, report, 15.0);
		}
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return true; }
}