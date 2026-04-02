// CGQC_ShootingRange.c
// Place CQB_TargetBlank_CGQC.et in Workbench at the 3m position.
// Set "Respawn Prefab" on CGQC_TargetMoverComponent to CQB_TargetBlank_CGQC.et

class CGQC_TargetMoverComponentClass : ScriptComponentClass {}

class CGQC_TargetMoverComponent : ScriptComponent
{
	protected static const float DIST_0 = 3.0;
	protected static const float DIST_1 = 5.0;
	protected static const float DIST_2 = 10.0;
	protected static const float DIST_3 = 15.0;
	protected static const float DIST_4 = 20.0;
	protected static const float DIST_5 = 25.0;
	protected static const float DIST_6 = 30.0;
	protected static const float CHECK_DISTANCE = 1.2;
	protected static const int DIST_COUNT = 7;
	protected static const float MOVE_SPEED = 4.0; // metres per second

	[RplProp(onRplName: "OnStateReplicated")]
	protected int m_iDistanceIndex = 0;

	[RplProp(onRplName: "OnStateReplicated")]
	protected bool m_bCheckMode = false;

	// True while sliding — locks controls
	[RplProp(onRplName: "OnMovingReplicated")]
	protected bool m_bMoving = false;

	protected float m_fBaseX = 0;
	protected float m_fTargetX = 0; // world X we are sliding toward

	[Attribute(defvalue: "", desc: "Prefab to spawn on reset (CQB_TargetBlank_CGQC.et)", params: "et")]
	protected ResourceName m_sRespawnPrefab;

	//------------------------------------------------------------------------------------------------
	void CGQC_TargetMoverComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		SetEventMask(ent, EntityEvent.FRAME);
		ent.SetFlags(EntityFlags.ACTIVE, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_fBaseX = owner.GetOrigin()[0];
		m_fTargetX = m_fBaseX;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bMoving)
			return;

		vector pos = owner.GetOrigin();
		float currentX = pos[0];
		float diff = m_fTargetX - currentX;
		float step = MOVE_SPEED * timeSlice;

		if (Math.AbsFloat(diff) <= step)
		{
			// Arrived
			pos[0] = m_fTargetX;
			owner.SetOrigin(pos);

			Physics phys = owner.GetPhysics();
			if (phys)
				phys.SetVelocity(vector.Zero);

			if (Replication.IsServer())
			{
				m_bMoving = false;
				Replication.BumpMe();
			}
		}
		else
		{
			if (diff > 0)
				pos[0] = currentX + step;
			else
				pos[0] = currentX - step;
			owner.SetOrigin(pos);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Called on clients when RplProps arrive
	protected void OnStateReplicated()
	{
		if (Replication.IsServer())
			return;

		// Compute target X from new state so clients slide too
		m_fTargetX = ComputeTargetX();
		m_bMoving = true;
	}

	protected void OnMovingReplicated()
	{
		// When server signals movement done, snap to exact position on client
		if (!m_bMoving)
		{
			IEntity owner = GetOwner();
			if (!owner)
				return;
			vector pos = owner.GetOrigin();
			pos[0] = m_fTargetX;
			owner.SetOrigin(pos);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsCheckMode() { return m_bCheckMode; }
	bool IsMoving() { return m_bMoving; }
	float GetBaseX() { return m_fBaseX; }
	int GetDistanceIndex() { return m_iDistanceIndex; }
	ResourceName GetPrefabName() { return m_sRespawnPrefab; }

	void SetBaseX(float val) { m_fBaseX = val; m_fTargetX = val; }
	void SetRespawnPrefab(ResourceName val) { m_sRespawnPrefab = val; }

	//------------------------------------------------------------------------------------------------
	protected float ComputeTargetX()
	{
		float dist;
		if (m_bCheckMode)
			dist = CHECK_DISTANCE;
		else
			dist = GetDistanceAtIndex(m_iDistanceIndex);
		return m_fBaseX - (dist - DIST_0);
	}

	//------------------------------------------------------------------------------------------------
	void CycleDistance()
	{
		if (!Replication.IsServer() || m_bCheckMode || m_bMoving)
			return;

		m_iDistanceIndex = (m_iDistanceIndex + 1) % DIST_COUNT;
		m_fTargetX = ComputeTargetX();
		m_bMoving = true;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void ToggleCheck()
	{
		if (!Replication.IsServer() || m_bMoving)
			return;

		m_bCheckMode = !m_bCheckMode;
		m_fTargetX = ComputeTargetX();
		m_bMoving = true;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void Reset()
	{
		if (!Replication.IsServer())
			return;

		IEntity owner = GetOwner();
		if (!owner || m_sRespawnPrefab.IsEmpty())
		{
			Print("[CGQC_Reset] No owner or respawn prefab not set.", LogLevel.WARNING);
			return;
		}

		Resource res = Resource.Load(m_sRespawnPrefab);
		if (!res.IsValid())
		{
			Print("[CGQC_Reset] Resource.Load failed: " + m_sRespawnPrefab, LogLevel.WARNING);
			return;
		}

		string entName      = owner.GetName();
		float baseX         = m_fBaseX;
		ResourceName prefab = m_sRespawnPrefab;

		vector transform[4];
		owner.GetTransform(transform);
		transform[3][0] = baseX;

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

		CGQC_TargetMoverComponent newMover = CGQC_TargetMoverComponent.Cast(
			newEnt.FindComponent(CGQC_TargetMoverComponent)
		);
		if (newMover)
		{
			newMover.SetBaseX(baseX);
			newMover.SetRespawnPrefab(prefab);
		}

		SCR_EntityHelper.DeleteEntityAndChildren(owner);
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
		if (idx == 4) return DIST_4;
		if (idx == 5) return DIST_5;
		return DIST_6;
	}

	//------------------------------------------------------------------------------------------------
	// Snap to position immediately (used after reset)
	void SnapToPosition()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		vector pos = owner.GetOrigin();
		pos[0] = m_fTargetX;
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
		Print("[" + tag + "] Target entity name not set.", LogLevel.WARNING);
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

// Silent version for CanBePerformedScript — no log spam
static CGQC_TargetMoverComponent CGQC_FindMoverSilent(string entityName)
{
	if (entityName.IsEmpty())
		return null;

	IEntity targetEnt = GetGame().GetWorld().FindEntityByName(entityName);
	if (!targetEnt)
		return null;

	return CGQC_TargetMoverComponent.Cast(targetEnt.FindComponent(CGQC_TargetMoverComponent));
}

//------------------------------------------------------------------------------------------------
// Distance cycle
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

		SCR_HintManagerComponent hintMgr = SCR_HintManagerComponent.GetInstance();
		if (hintMgr)
			hintMgr.ShowCustomHint("Distance — " + label, "", 2.0);
	}

	override bool CanBeShownScript(IEntity user) { return true; }

	override bool CanBePerformedScript(IEntity user)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMoverSilent(m_sTargetEntityName);
		if (!mover)
			return true;
		return !mover.IsCheckMode() && !mover.IsMoving();
	}

	override bool HasLocalEffectOnlyScript() { return false; }
}

//------------------------------------------------------------------------------------------------
// Check toggle
//------------------------------------------------------------------------------------------------

class CGQC_TargetCheckAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of this bay's target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMover(m_sTargetEntityName, "CGQC_TargetCheckAction");
		if (!mover)
			return;

		mover.ToggleCheck();

		SCR_HintManagerComponent hintMgr = SCR_HintManagerComponent.GetInstance();
		if (hintMgr)
			hintMgr.ShowCustomHint("Vérification de la cible", "", 2.0);
	}

	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMoverSilent(m_sTargetEntityName);
		if (!mover)
			return true;
		return !mover.IsMoving();
	}
	override bool HasLocalEffectOnlyScript() { return false; }
}

//------------------------------------------------------------------------------------------------
// Reset
//------------------------------------------------------------------------------------------------

class CGQC_TargetResetAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Exact scene name of this bay's target entity")]
	protected string m_sTargetEntityName;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMover(m_sTargetEntityName, "CGQC_TargetResetAction");
		if (!mover)
			return;

		mover.Reset();

		SCR_HintManagerComponent hintMgr = SCR_HintManagerComponent.GetInstance();
		if (hintMgr)
			hintMgr.ShowCustomHint("Reset de toute la patente", "", 2.0);
	}

	override bool CanBeShownScript(IEntity user) { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript() { return false; }
}