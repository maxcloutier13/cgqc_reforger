// CGQC_ShootingRange.c
// Place the target in Workbench at the 3m position.
//
// CGQC_TargetMoverComponent  — attach to the CQB_TargetBlank entity
// CGQC_TargetCycleAction     — distance switch > ActionsManagerComponent > Additional Actions
// CGQC_TargetCheckAction     — check switch > ActionsManagerComponent > Additional Actions
// CGQC_TargetResetAction     — also on check switch > Additional Actions

//------------------------------------------------------------------------------------------------
class CGQC_TargetMoverComponentClass : ScriptComponentClass {}

class CGQC_TargetMoverComponent : ScriptComponent
{
	protected static const float DIST_0 = 3.0;
	protected static const float DIST_1 = 5.0;
	protected static const float DIST_2 = 10.0;
	protected static const float DIST_3 = 15.0;
	protected static const float DIST_4 = 25.0;
	protected static const float CHECK_DISTANCE = 1.2;
	protected static const int DIST_COUNT = 5;

	[RplProp()]
	protected int m_iDistanceIndex = 0;

	[RplProp()]
	protected bool m_bCheckMode = false;

	protected float m_fBaseX = 0;

	// Set to the source prefab path of this target (e.g. CQB_TargetBlank.et)
	// Used by Reset to respawn a fresh copy. Set once in Workbench per target entity.
	[Attribute(defvalue: "", desc: "Prefab to spawn on reset", params: "et")]
	protected ResourceName m_sRespawnPrefab;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		m_fBaseX = owner.GetOrigin()[0];
	}

	//------------------------------------------------------------------------------------------------
	bool IsCheckMode() { return m_bCheckMode; }

	void SetBaseX(float val) { m_fBaseX = val; }
	void SetDistanceIndex(int val) { m_iDistanceIndex = val; }
	void SetCheckMode(bool val) { m_bCheckMode = val; }

	ResourceName GetPrefabName() { return m_sRespawnPrefab; }
	void SetRespawnPrefab(ResourceName val) { m_sRespawnPrefab = val; }
	float GetBaseX() { return m_fBaseX; }
	int GetDistanceIndex() { return m_iDistanceIndex; }

	//------------------------------------------------------------------------------------------------
	void CycleDistance()
	{
		if (!Replication.IsServer() || m_bCheckMode)
			return;

		m_iDistanceIndex = (m_iDistanceIndex + 1) % DIST_COUNT;
		Replication.BumpMe();
		ApplyPosition();
	}

	//------------------------------------------------------------------------------------------------
	void ToggleCheck()
	{
		if (!Replication.IsServer())
			return;

		m_bCheckMode = !m_bCheckMode;
		Replication.BumpMe();
		ApplyPosition();
	}

	//------------------------------------------------------------------------------------------------
	string GetNextCycleLabel()
	{
		int nextIndex = (m_iDistanceIndex + 1) % DIST_COUNT;
		return GetDistanceAtIndex(nextIndex).ToString() + "m";
	}

	//------------------------------------------------------------------------------------------------
	protected float GetDistanceAtIndex(int idx)
	{
		if (idx == 0) return DIST_0;
		if (idx == 1) return DIST_1;
		if (idx == 2) return DIST_2;
		if (idx == 3) return DIST_3;
		return DIST_4;
	}

	//------------------------------------------------------------------------------------------------
	void ApplyPosition()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		float dist;
		if (m_bCheckMode)
			dist = CHECK_DISTANCE;
		else
			dist = GetDistanceAtIndex(m_iDistanceIndex);

		float newX = m_fBaseX - (dist - DIST_0);

		vector pos = owner.GetOrigin();
		pos[0] = newX;
		owner.SetOrigin(pos);

		Physics phys = owner.GetPhysics();
		if (phys)
			phys.SetVelocity(vector.Zero);
	}
}

//------------------------------------------------------------------------------------------------
static CGQC_TargetMoverComponent CGQC_FindMover(string entityName, string tag)
{
	if (entityName.IsEmpty())
	{
		Print("[" + tag + "] m_sTargetEntityName not set.", LogLevel.WARNING);
		return null;
	}

	IEntity targetEnt = GetGame().GetWorld().FindEntityByName(entityName);
	if (!targetEnt)
	{
		Print("[" + tag + "] Cannot find entity: " + entityName, LogLevel.WARNING);
		return null;
	}

	CGQC_TargetMoverComponent mover = CGQC_TargetMoverComponent.Cast(
		targetEnt.FindComponent(CGQC_TargetMoverComponent)
	);

	if (!mover)
		Print("[" + tag + "] No CGQC_TargetMoverComponent on: " + entityName, LogLevel.WARNING);

	return mover;
}

static void CGQC_Notify(string msg)
{
	SCR_PopUpNotification notif = SCR_PopUpNotification.GetInstance();
	if (!notif)
		return;

	notif.ClearMsg();
	notif.PopupMsg(msg);
}

//------------------------------------------------------------------------------------------------
class CGQC_TargetCycleAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of this bay's target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMover(m_sTargetEntityName, "CGQC_TargetCycleAction");
		if (!mover)
			return;

		string label = mover.GetNextCycleLabel();
		mover.CycleDistance();
		CGQC_Notify("Distance — " + label);
	}

	override bool CanBeShownScript(IEntity user) { return true; }

	override bool CanBePerformedScript(IEntity user)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMover(m_sTargetEntityName, "CGQC_TargetCycleAction");
		if (!mover)
			return true; // don't lock if entity temporarily missing (e.g. post-reset)
		return !mover.IsCheckMode();
	}

	override bool HasLocalEffectOnlyScript() { return false; }
}

//------------------------------------------------------------------------------------------------
class CGQC_TargetCheckAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of this bay's target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMover(m_sTargetEntityName, "CGQC_TargetCheckAction");
		if (mover)
			mover.ToggleCheck();
	}

	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript() { return false; }
}

//------------------------------------------------------------------------------------------------
class CGQC_TargetResetAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of this bay's target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		IEntity targetEnt = GetGame().GetWorld().FindEntityByName(m_sTargetEntityName);
		if (!targetEnt)
		{
			Print("[CGQC_Reset] Cannot find entity: " + m_sTargetEntityName, LogLevel.WARNING);
			return;
		}

		CGQC_TargetMoverComponent mover = CGQC_TargetMoverComponent.Cast(
			targetEnt.FindComponent(CGQC_TargetMoverComponent)
		);
		if (!mover)
			return;

		ResourceName prefab = mover.GetPrefabName();
		if (prefab.IsEmpty())
		{
			Print("[CGQC_Reset] Respawn prefab not set on mover component.", LogLevel.WARNING);
			return;
		}

		Resource res = Resource.Load(prefab);
		if (!res.IsValid())
		{
			Print("[CGQC_Reset] Resource.Load failed: " + prefab, LogLevel.WARNING);
			return;
		}

		string entName  = targetEnt.GetName();
		float baseX     = mover.GetBaseX();
		ResourceName rp = prefab;

		// Build spawn transform from current rotation/position but force X to 3m baseline
		vector transform[4];
		targetEnt.GetTransform(transform);
		transform[3][0] = baseX; // Move to 3m position along X

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = transform;

		IEntity newEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!newEnt)
		{
			Print("[CGQC_Reset] SpawnEntityPrefab failed.", LogLevel.ERROR);
			return;
		}

		newEnt.SetName(entName);

		// OnPostInit records baseX from spawn position (which is already baseX).
		// Explicitly set it anyway to be safe, and copy respawn prefab for future resets.
		CGQC_TargetMoverComponent newMover = CGQC_TargetMoverComponent.Cast(
			newEnt.FindComponent(CGQC_TargetMoverComponent)
		);
		if (newMover)
		{
			newMover.SetBaseX(baseX);
			newMover.SetRespawnPrefab(rp);
		}

		SCR_EntityHelper.DeleteEntityAndChildren(targetEnt);
	}

	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user) { return CGQC_FindMover(m_sTargetEntityName, "CGQC_TargetResetAction") != null; }
	override bool HasLocalEffectOnlyScript() { return false; }
}