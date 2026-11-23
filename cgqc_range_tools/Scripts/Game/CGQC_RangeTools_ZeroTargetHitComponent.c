// ============================================================
// CGQC_RangeTools_ZeroTarget.c
// ============================================================


// ----------------------------------------------------------------
// Per-shot data
// ----------------------------------------------------------------
class CGQC_RangeTools_ZeroHitData
{
	int    m_iPlayerID;
	float  m_fVelocity;
	float  m_fDistance;
	vector m_vWorldPos;
	float  m_fMradX;
	float  m_fMradY;
	float  m_fMradTotal;

	void CGQC_RangeTools_ZeroHitData(int pid, float vel, float dist, vector wpos,
	                                  float mradX, float mradY, float mradTotal)
	{
		m_iPlayerID  = pid;
		m_fVelocity  = vel;
		m_fDistance  = dist;
		m_vWorldPos  = wpos;
		m_fMradX     = mradX;
		m_fMradY     = mradY;
		m_fMradTotal = mradTotal;
	}
}


// ----------------------------------------------------------------
// Static helper for zero target reset (clears hits)
// ----------------------------------------------------------------
class CGQC_RangeTools_ZeroResetHelper
{
	static ResourceName s_sPrefab;
	static vector s_vTransform[4];
	static string s_sName;

	static void DoReset()
	{
		Resource res = Resource.Load(s_sPrefab);
		if (!res || !res.IsValid()) return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = s_vTransform[0];
		params.Transform[1] = s_vTransform[1];
		params.Transform[2] = s_vTransform[2];
		params.Transform[3] = s_vTransform[3];

		IEntity newEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!newEnt) return;

		if (!s_sName.IsEmpty())
			newEnt.SetName(s_sName);
	}
}


// ----------------------------------------------------------------
// Static helper for zero target move (preserves hit data)
// ----------------------------------------------------------------
class CGQC_RangeTools_ZeroMoveHelper
{
	static ResourceName s_sPrefab;
	static vector s_vTransform[4];
	static string s_sName;
	static ref array<ref CGQC_RangeTools_ZeroHitData> s_aHits = new array<ref CGQC_RangeTools_ZeroHitData>();

	static void DoMove()
	{
		Resource res = Resource.Load(s_sPrefab);
		if (!res || !res.IsValid()) return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = s_vTransform[0];
		params.Transform[1] = s_vTransform[1];
		params.Transform[2] = s_vTransform[2];
		params.Transform[3] = s_vTransform[3];

		IEntity newEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!newEnt) return;

		if (!s_sName.IsEmpty())
			newEnt.SetName(s_sName);

		CGQC_RangeTools_ZeroTargetHitComponent newComp =
			CGQC_RangeTools_ZeroTargetHitComponent.Cast(
				newEnt.FindComponent(CGQC_RangeTools_ZeroTargetHitComponent));
		if (newComp)
			newComp.RestoreHits(s_aHits);
	}
}


// ----------------------------------------------------------------
// Component class descriptor
// ----------------------------------------------------------------
class CGQC_RangeTools_ZeroTargetHitComponentClass : ScriptComponentClass {}


// ================================================================
// CGQC_RangeTools_ZeroTargetHitComponent — on the target entity
// ================================================================
class CGQC_RangeTools_ZeroTargetHitComponent : ScriptComponent
{
	// Height of bullseye center above entity origin (measured in-game)
	protected static const float BULLSEYE_Y = 1.40167;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab to re-spawn on Reset", params: "et")]
	protected ResourceName m_sPrefabPath;

	protected ref array<ref CGQC_RangeTools_ZeroHitData> m_aHits =
		new array<ref CGQC_RangeTools_ZeroHitData>();

	protected RplComponent m_RplComp;

	[RplProp()]
	protected string m_sLastReport      = string.Empty;

	[RplProp()]
	protected string m_sLastReportTitle = string.Empty;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode()) return;
		m_RplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
	}

	protected bool IsAuthority()
	{
		return !m_RplComp || !m_RplComp.IsProxy();
	}

	string GetLastReport()      { return m_sLastReport; }
	string GetLastReportTitle() { return m_sLastReportTitle; }

	void RestoreHits(array<ref CGQC_RangeTools_ZeroHitData> hits)
	{
		m_aHits.Clear();
		for (int i = 0; i < hits.Count(); i++)
			m_aHits.Insert(hits[i]);
	}

	// ================================================================
	// RegisterHit
	// ================================================================
	void RegisterHit(int playerID, float velocityMS, vector worldHitPos)
	{
		if (!IsAuthority()) return;

		IEntity owner = GetOwner();
		if (!owner) return;

		float dist = 0.0;
		IEntity shooterChar = GetCharacterForPlayer(playerID);
		if (shooterChar)
			dist = vector.Distance(shooterChar.GetOrigin(), owner.GetOrigin());

		vector mat[4];
		owner.GetWorldTransform(mat);

		float mradX = 0.0, mradY = 0.0, mradTotal = 0.0;
		if (dist > 0.1)
		{
			vector offset = worldHitPos - mat[3];
			float localX  = -vector.Dot(offset, mat[0]);
			float localY  = vector.Dot(offset, mat[1]) - BULLSEYE_Y;
			mradX     = (localX / dist) * 1000.0;
			mradY     = (localY / dist) * 1000.0;
			mradTotal = Math.Sqrt(mradX * mradX + mradY * mradY);
		}

		m_aHits.Insert(new CGQC_RangeTools_ZeroHitData(
			playerID, velocityMS, dist, worldHitPos, mradX, mradY, mradTotal));

		m_sLastReport      = BuildReport(playerID);
		m_sLastReportTitle = BuildReportTitle(playerID);
		Replication.BumpMe();

		if (playerID > 0)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerID);
			if (pc)
			{
				SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
				if (spc)
				{
					string mradStr = FormatMradHint(mradX, mradY, mradTotal);
					float localXm = (mradX * dist) / 1000.0;
					float localYm = (mradY * dist) / 1000.0;
					string cmStr = FormatCmMm(localXm) + " / " + FormatCmMm(localYm);
					spc.CGQC_Rpc_ShowZeroHitHint(m_aHits.Count(), mradStr, cmStr, (int)dist, (int)velocityMS);
				}
			}
		}
	}

	// ================================================================
	// Server_Reset
	// ================================================================
	void Server_Reset(int requestingPlayerID = -1)
	{
		if (!IsAuthority()) return;

		IEntity owner = GetOwner();
		if (!owner) return;

		if (m_sPrefabPath == string.Empty)
		{
			m_aHits.Clear();
			m_sLastReport      = string.Empty;
			m_sLastReportTitle = string.Empty;
			Replication.BumpMe();
			return;
		}

		vector mat[4];
		owner.GetTransform(mat);

		CGQC_RangeTools_ZeroResetHelper.s_sPrefab    = m_sPrefabPath;
		CGQC_RangeTools_ZeroResetHelper.s_vTransform = mat;
		CGQC_RangeTools_ZeroResetHelper.s_sName      = owner.GetName();

		m_aHits.Clear();
		m_sLastReport      = string.Empty;
		m_sLastReportTitle = string.Empty;

		if (requestingPlayerID > 0)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(requestingPlayerID);
			if (pc)
			{
				SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
				if (spc) spc.CGQC_Rpc_ShowResetHint();
			}
		}

		SCR_EntityHelper.DeleteEntityAndChildren(owner);
		GetGame().GetCallqueue().CallLater(CGQC_RangeTools_ZeroResetHelper.DoReset, 100, false);
	}

	// ================================================================
	// Server_Check
	// ================================================================
	void Server_Check(int requestingPlayerID)
	{
		if (!IsAuthority()) return;

		m_sLastReport      = BuildReport(requestingPlayerID);
		m_sLastReportTitle = BuildReportTitle(requestingPlayerID);
		Replication.BumpMe();

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(requestingPlayerID);
		if (!pc) return;
		SCR_PlayerController spc = SCR_PlayerController.Cast(pc);
		if (!spc) return;
		spc.CGQC_Rpc_ShowTargetReport(m_sLastReport, m_sLastReportTitle);
	}

	// ================================================================
	// Movement — delete+respawn, preserving hits
	// ================================================================
	protected static const float DIST_CLOSE = 2.0;
	protected static ref array<float> DIST_STEPS = {25.0, 50.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0};
	protected int m_iDistStep = 0;

	protected int GetCurrentDistStep(float distFromPlayer)
	{
		int best = 0;
		float bestDiff = Math.AbsFloat(distFromPlayer - DIST_STEPS[0]);
		for (int i = 1; i < DIST_STEPS.Count(); i++)
		{
			float diff = Math.AbsFloat(distFromPlayer - DIST_STEPS[i]);
			if (diff < bestDiff) { bestDiff = diff; best = i; }
		}
		return best;
	}

	protected void RespawnAt(IEntity control, float dist)
	{
		IEntity owner = GetOwner();
		if (!owner || !control) return;

		vector controlMat[4];
		control.GetWorldTransform(controlMat);
		vector dir = controlMat[2];
		dir[1] = 0.0;
		dir.Normalize();
		dir = dir * -1.0;

		vector mat[4];
		owner.GetTransform(mat);
		mat[3] = controlMat[3] + dir * dist;
		mat[3][1] = owner.GetOrigin()[1];

		CGQC_RangeTools_ZeroMoveHelper.s_sPrefab    = m_sPrefabPath;
		CGQC_RangeTools_ZeroMoveHelper.s_vTransform = mat;
		CGQC_RangeTools_ZeroMoveHelper.s_sName      = owner.GetName();
		CGQC_RangeTools_ZeroMoveHelper.s_aHits.Clear();
		for (int i = 0; i < m_aHits.Count(); i++)
			CGQC_RangeTools_ZeroMoveHelper.s_aHits.Insert(m_aHits[i]);

		SCR_EntityHelper.DeleteEntityAndChildren(owner);
		GetGame().GetCallqueue().CallLater(CGQC_RangeTools_ZeroMoveHelper.DoMove, 100, false);
	}

	void Server_MoveClose(IEntity control)
	{
		if (!Replication.IsServer()) return;
		m_iDistStep = 0;
		RespawnAt(control, DIST_CLOSE);
	}

	void Server_MoveAway(IEntity control)
	{
		if (!Replication.IsServer()) return;
		IEntity owner = GetOwner();
		if (!owner || !control) return;

		float currentDist = vector.Distance(control.GetOrigin(), owner.GetOrigin());
		int currentStep = GetCurrentDistStep(currentDist);

		int nextStep = currentStep + 1;
		if (nextStep >= DIST_STEPS.Count())
			nextStep = DIST_STEPS.Count() - 1;
		if (currentDist < DIST_STEPS[0] - 5.0)
			nextStep = 0;

		m_iDistStep = nextStep;
		RespawnAt(control, DIST_STEPS[nextStep]);
	}

	void Server_MoveCloser(IEntity control)
	{
		if (!Replication.IsServer()) return;
		IEntity owner = GetOwner();
		if (!owner || !control) return;

		float currentDist = vector.Distance(control.GetOrigin(), owner.GetOrigin());

		if (currentDist <= DIST_STEPS[0] + 5.0)
		{
			Server_MoveClose(control);
			return;
		}

		int currentStep = GetCurrentDistStep(currentDist);
		int prevStep = currentStep - 1;
		if (prevStep < 0) prevStep = 0;

		m_iDistStep = prevStep;
		RespawnAt(control, DIST_STEPS[prevStep]);
	}

	// ================================================================
	// BuildReportTitle
	// ================================================================
	protected string BuildReportTitle(int pid)
	{
		string playerName = GetGame().GetPlayerManager().GetPlayerName(pid);
		string weaponName = "Unknown";

		IEntity ch = GetCharacterForPlayer(pid);
		if (ch)
		{
			BaseWeaponManagerComponent wm =
				BaseWeaponManagerComponent.Cast(ch.FindComponent(BaseWeaponManagerComponent));
			if (wm)
			{
				WeaponSlotComponent ws = wm.GetCurrentSlot();
				if (ws)
				{
					IEntity wEnt = ws.GetWeaponEntity();
					if (wEnt)
					{
						BaseWeaponComponent wComp =
							BaseWeaponComponent.Cast(wEnt.FindComponent(BaseWeaponComponent));
						if (wComp) { UIInfo wUI = wComp.GetUIInfo(); if (wUI) weaponName = wUI.GetName(); }
					}
				}
			}
		}

		return playerName + " | " + weaponName + " | ZERO";
	}

	// ================================================================
	// BuildReport
	// ================================================================
	protected string BuildReport(int requestingPlayerID)
	{
		int hitCount = m_aHits.Count();
		if (hitCount == 0)
			return "Aucun impact enregistre.";

		string magName = "Unknown";
		IEntity ch = GetCharacterForPlayer(requestingPlayerID);
		if (ch)
		{
			BaseWeaponManagerComponent wm =
				BaseWeaponManagerComponent.Cast(ch.FindComponent(BaseWeaponManagerComponent));
			if (wm)
			{
				WeaponSlotComponent ws = wm.GetCurrentSlot();
				if (ws)
				{
					IEntity wEnt = ws.GetWeaponEntity();
					if (wEnt)
					{
						BaseWeaponComponent wComp =
							BaseWeaponComponent.Cast(wEnt.FindComponent(BaseWeaponComponent));
						if (wComp)
						{
							BaseMagazineComponent mag = wComp.GetCurrentMagazine();
							if (mag)
							{
								InventoryItemComponent mInv =
									InventoryItemComponent.Cast(
										mag.GetOwner().FindComponent(InventoryItemComponent));
								if (mInv) { UIInfo mUI = mInv.GetUIInfo(); if (mUI) magName = mUI.GetName(); }
							}
						}
					}
				}
			}
		}

		float shotDistM = m_aHits[0].m_fDistance;

		// Group bounding box + centroid in target-local space
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

		float minR =  99999.0, maxR = -99999.0;
		float minU =  99999.0, maxU = -99999.0;
		float sumR = 0.0, sumU = 0.0;

		for (int i = 0; i < hitCount; i++)
		{
			vector offset = m_aHits[i].m_vWorldPos - targetPos;
			float r = -vector.Dot(offset, targetRight);
			float u = vector.Dot(offset, targetUp) - BULLSEYE_Y;
			if (r < minR) minR = r;
			if (r > maxR) maxR = r;
			if (u < minU) minU = u;
			if (u > maxU) maxU = u;
			sumR = sumR + r;
			sumU = sumU + u;
		}

		float groupM    = Math.Sqrt((maxR - minR) * (maxR - minR) + (maxU - minU) * (maxU - minU));
		float groupCm   = groupM * 100.0;
		float groupMrad = 0.0;
		if (shotDistM > 0.1)
			groupMrad = (groupM / shotDistM) * 1000.0;

		float centroidR = sumR / hitCount;
		float centroidU = sumU / hitCount;
		float centroidMradX = 0.0, centroidMradY = 0.0, centroidMradTotal = 0.0;
		if (shotDistM > 0.1)
		{
			centroidMradX     = (centroidR / shotDistM) * 1000.0;
			centroidMradY     = (centroidU / shotDistM) * 1000.0;
			centroidMradTotal = Math.Sqrt(centroidMradX * centroidMradX +
			                              centroidMradY * centroidMradY);
		}

		// Velocity stats
		float velSum = 0.0, velMin = 999999.0, velMax = 0.0;
		for (int i = 0; i < hitCount; i++)
		{
			float v = m_aHits[i].m_fVelocity;
			velSum = velSum + v;
			if (v < velMin) velMin = v;
			if (v > velMax) velMax = v;
		}
		float velAvg  = velSum / hitCount;
		int velAvgMs  = (int)velAvg;
		int velAvgFts = (int)(velAvg * 3.28084);
		int velMinMs  = (int)velMin;
		int velMaxMs  = (int)velMax;
		int extSpread = velMaxMs - velMinMs;
		float variance = 0.0;
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
		string datetime = yy + mm + dd + " - " + hh + ":" + mn +
		                  " - Dist: " + ((int)shotDistM).ToString() + "m - Tgt:ZERO";

		string nl  = "\n";
		string sep = "--------------------------------" + nl;

		string r = datetime + nl;
		r = r + magName + nl;
		r = r + sep;
		r = r + "Groupe: " + FloatStr(groupCm, 1) + " cm | " + FloatStr(groupMrad, 2) + " mrad" + nl;
		r = r + sep;

		float centroidCmX = centroidR * 100.0;
		float centroidCmY = centroidU * 100.0;
		r = r + "Centre groupe:" + nl;
		r = r + "  " + FloatStr(centroidMradX, 2) + " / " + FloatStr(centroidMradY, 2) + " mrad" + nl;
		r = r + "  " + FloatStr(centroidCmX, 1) + " / " + FloatStr(centroidCmY, 1) + " cm" + nl;
		r = r + sep;

		for (int i = 0; i < hitCount; i++)
		{
			CGQC_RangeTools_ZeroHitData h = m_aHits[i];
			int shotNum = i + 1;
			string numStr = shotNum.ToString();
			if (shotNum < 10) numStr = " " + numStr;

			float cmX = (h.m_fMradX * h.m_fDistance) / 1000.0 * 100.0;
			float cmY = (h.m_fMradY * h.m_fDistance) / 1000.0 * 100.0;

			string shotLine = numStr + ". ";
			shotLine = shotLine + FloatStr(h.m_fMradX, 2) + " / " + FloatStr(h.m_fMradY, 2) + " mrad";
			shotLine = shotLine + "  [" + FloatStr(cmX, 1) + " / " + FloatStr(cmY, 1) + " cm]";
			shotLine = shotLine + "  " + ((int)h.m_fDistance).ToString() + "m";
			shotLine = shotLine + "  " + ((int)h.m_fVelocity).ToString() + " m/s";
			r = r + shotLine + nl;
		}

		r = r + sep;
		r = r + hitCount.ToString() + " tirs | MuzzleVel. Moy: " +
		    velAvgMs.ToString() + " m/s | " + velAvgFts.ToString() + " ft/s" + nl;
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
		bool neg = v < 0.0;
		if (neg) v = -v;
		int whole = (int)v;
		float factor = 1.0;
		int d = decimals;
		while (d > 0) { factor = factor * 10.0; d--; }
		int frac = (int)((v - whole) * factor);
		if (frac < 0) frac = -frac;
		string fracStr = frac.ToString();
		while (fracStr.Length() < decimals) fracStr = "0" + fracStr;
		string s = whole.ToString() + "." + fracStr;
		if (neg) s = "-" + s;
		return s;
	}

	protected string FormatMradHint(float mradX, float mradY, float mradTotal)
	{
		return FloatStr(mradX, 2) + " / " + FloatStr(mradY, 2) + " mrad (" + FloatStr(mradTotal, 2) + ")";
	}

	protected string FormatCmMm(float metres)
	{
		float cm = metres * 100.0;
		if (Math.AbsFloat(cm) < 1.0)
		{
			float mm = metres * 1000.0;
			int mmInt = (int)mm;
			return mmInt.ToString() + " mm";
		}
		return FloatStr(cm, 1) + " cm";
	}
}


// ================================================================
// Damage manager
// ================================================================
class CGQC_RangeTools_ZeroTargetDamageManagerClass : SCR_DamageManagerComponentClass {}

class CGQC_RangeTools_ZeroTargetDamageManager : SCR_DamageManagerComponent
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
					if (parent)
						playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(parent);
				}
			}
		}

		float muzzleVelocity = 0.0;
		if (playerID > 0)
		{
			IEntity shooterChar =
				GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
			if (shooterChar)
			{
				CGQC_RangeTools_ChronographComponent chrono =
					CGQC_RangeTools_ChronographComponent.Cast(
						shooterChar.FindComponent(CGQC_RangeTools_ChronographComponent));
				if (chrono)
					muzzleVelocity = chrono.GetLastMuzzleVelocity();
			}
		}

		CGQC_RangeTools_ZeroTargetHitComponent hitComp =
			CGQC_RangeTools_ZeroTargetHitComponent.Cast(
				GetOwner().FindComponent(CGQC_RangeTools_ZeroTargetHitComponent));
		if (hitComp)
			hitComp.RegisterHit(playerID, muzzleVelocity, impactPos);
	}
}


// ================================================================
// User action: Reset — on the target entity directly
// ================================================================
class CGQC_RangeTools_ZeroResetAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_RangeTools_ZeroTargetHitComponent hitComp =
			CGQC_RangeTools_ZeroTargetHitComponent.Cast(
				pOwnerEntity.FindComponent(CGQC_RangeTools_ZeroTargetHitComponent));
		if (!hitComp) return;

		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		hitComp.Server_Reset(pid);
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}


// ================================================================
// User action: Check — on the target entity directly
// ================================================================
class CGQC_RangeTools_ZeroCheckAction : ScriptedUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_RangeTools_ZeroTargetHitComponent hitComp =
			CGQC_RangeTools_ZeroTargetHitComponent.Cast(
				pOwnerEntity.FindComponent(CGQC_RangeTools_ZeroTargetHitComponent));
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
			CGQC_BasicDisplay.Show(title, report, 15.0);
		}
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return true; }
}


// ================================================================
// zeroControl movement actions
// ================================================================

class CGQC_RangeTools_ZeroCloseAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of the zero target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		IEntity targetEnt = GetGame().GetWorld().FindEntityByName(m_sTargetEntityName);
		if (!targetEnt) return;
		CGQC_RangeTools_ZeroTargetHitComponent hitComp =
			CGQC_RangeTools_ZeroTargetHitComponent.Cast(
				targetEnt.FindComponent(CGQC_RangeTools_ZeroTargetHitComponent));
		if (!hitComp) return;
		hitComp.Server_MoveClose(pOwnerEntity);
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}

class CGQC_RangeTools_ZeroMoveAwayAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of the zero target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		IEntity targetEnt = GetGame().GetWorld().FindEntityByName(m_sTargetEntityName);
		if (!targetEnt) return;
		CGQC_RangeTools_ZeroTargetHitComponent hitComp =
			CGQC_RangeTools_ZeroTargetHitComponent.Cast(
				targetEnt.FindComponent(CGQC_RangeTools_ZeroTargetHitComponent));
		if (!hitComp) return;
		hitComp.Server_MoveAway(pOwnerEntity);
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}

class CGQC_RangeTools_ZeroMoveCloserAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of the zero target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		IEntity targetEnt = GetGame().GetWorld().FindEntityByName(m_sTargetEntityName);
		if (!targetEnt) return;
		CGQC_RangeTools_ZeroTargetHitComponent hitComp =
			CGQC_RangeTools_ZeroTargetHitComponent.Cast(
				targetEnt.FindComponent(CGQC_RangeTools_ZeroTargetHitComponent));
		if (!hitComp) return;
		hitComp.Server_MoveCloser(pOwnerEntity);
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}