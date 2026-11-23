// CGQC_targetMoverComponent.c
// Place CQB_TargetBlank_CGQC.et in Workbench at the 3m position.
// Set "Respawn Prefab" on CGQC_TargetMoverComponent to CQB_TargetBlank_CGQC.et

// ----------------------------------------------------------------
// Self-contained reset helper for the mover/bay system
// ----------------------------------------------------------------
class CGQC_ResetHelper
{
	static string s_sName;
	static float s_fBaseX;
	static ResourceName s_sPrefab;
	static vector s_vTransform[4];

	static void DoReset()
	{
		Resource res = Resource.Load(s_sPrefab);
		if (!res.IsValid())
		{
			Print("[CGQC_ResetHelper] Resource.Load failed: " + s_sPrefab, LogLevel.WARNING);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = s_vTransform[0];
		params.Transform[1] = s_vTransform[1];
		params.Transform[2] = s_vTransform[2];
		params.Transform[3] = s_vTransform[3];

		IEntity newEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!newEnt)
		{
			Print("[CGQC_ResetHelper] SpawnEntityPrefab failed.", LogLevel.ERROR);
			return;
		}

		if (!s_sName.IsEmpty())
			newEnt.SetName(s_sName);

		CGQC_TargetMoverComponent newMover = CGQC_TargetMoverComponent.Cast(
			newEnt.FindComponent(CGQC_TargetMoverComponent)
		);
		if (newMover)
		{
			newMover.SetBaseX(s_fBaseX);
			newMover.SetRespawnPrefab(s_sPrefab);
			// Explicitly push checkMode=false and distIndex=0 to clients
			// even though they're default — forces OnStateReplicated to fire
			newMover.SetCheckMode(false);
			newMover.SetDistanceIndex(0);
		}
	}
}


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

	[RplProp(onRplName: "OnMovingReplicated")]
	protected bool m_bMoving = false;

	protected float m_fBaseX = 0;
	protected float m_fTargetX = 0;

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
	protected void OnStateReplicated()
	{
		if (Replication.IsServer())
			return;
		m_fTargetX = ComputeTargetX();
		m_bMoving = true;
	}

	protected void OnMovingReplicated()
	{
		if (!m_bMoving)
		{
			IEntity owner = GetOwner();
			if (!owner) return;
			vector pos = owner.GetOrigin();
			pos[0] = m_fTargetX;
			owner.SetOrigin(pos);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsCheckMode()           { return m_bCheckMode; }
	bool IsMoving()              { return m_bMoving; }
	float GetBaseX()             { return m_fBaseX; }
	int GetDistanceIndex()       { return m_iDistanceIndex; }
	ResourceName GetPrefabName() { return m_sRespawnPrefab; }

	void SetBaseX(float val)                { m_fBaseX = val; m_fTargetX = val; }
	void SetRespawnPrefab(ResourceName val) { m_sRespawnPrefab = val; }

	void SetDistanceIndex(int val)
	{
		m_iDistanceIndex = val;
		m_fTargetX = ComputeTargetX();
		Replication.BumpMe();
	}

	void SetCheckMode(bool val)
	{
		m_bCheckMode = val;
		m_fTargetX = ComputeTargetX();
		Replication.BumpMe();
	}

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

		CGQC_ResetHelper.s_sName = owner.GetName();
		CGQC_ResetHelper.s_fBaseX = m_fBaseX;
		CGQC_ResetHelper.s_sPrefab = m_sRespawnPrefab;
		owner.GetTransform(CGQC_ResetHelper.s_vTransform);
		CGQC_ResetHelper.s_vTransform[3][0] = m_fBaseX;

		SCR_EntityHelper.DeleteEntityAndChildren(owner);
		GetGame().GetCallqueue().CallLater(CGQC_ResetHelper.DoReset, 100, false);
	}

	//------------------------------------------------------------------------------------------------
	// Returns the label for the current distance state.
	// Call AFTER CycleDistance() to get the label for the move that just happened.
	//------------------------------------------------------------------------------------------------
	string GetCurrentLabel()
	{
		if (m_bCheckMode)
			return "Vérification";
		return GetDistanceAtIndex(m_iDistanceIndex).ToString() + "m";
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
	void SnapToPosition()
	{
		IEntity owner = GetOwner();
		if (!owner) return;

		vector pos = owner.GetOrigin();
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
}

//------------------------------------------------------------------------------------------------
// Helper: get SCR_PlayerController from interacting character entity
//------------------------------------------------------------------------------------------------
static SCR_PlayerController CGQC_GetPlayerController(IEntity userEntity)
{
	if (!userEntity) return null;
	int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(userEntity);
	if (pid <= 0) return null;
	return SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(pid));
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

static CGQC_TargetMoverComponent CGQC_FindMoverSilent(string entityName)
{
	if (entityName.IsEmpty()) return null;
	IEntity targetEnt = GetGame().GetWorld().FindEntityByName(entityName);
	if (!targetEnt) return null;
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
		if (!mover) return;

		// Cycle first, then read the current label so hint matches the actual move
		mover.CycleDistance();
		string label = mover.GetCurrentLabel();

		SCR_PlayerController pc = CGQC_GetPlayerController(pUserEntity);
		if (pc)
			pc.CGQC_Rpc_ShowMoverHint("Champ de tir", "Distance — " + label);
	}

	override bool CanBeShownScript(IEntity user) { return true; }

	override bool CanBePerformedScript(IEntity user)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMoverSilent(m_sTargetEntityName);
		if (!mover) return true;
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

	override bool CanBePerformedScript(IEntity user)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMoverSilent(m_sTargetEntityName);
		if (!mover) return true;

		UIInfo ui = GetUIInfo();
		if (ui)
		{
			if (mover.IsCheckMode())
				ui.SetName("Retour");
			else
				ui.SetName("Vérification");
		}

		return !mover.IsMoving();
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CGQC_TargetMoverComponent mover = CGQC_FindMover(m_sTargetEntityName, "CGQC_TargetCheckAction");
		if (!mover) return;

		// Toggle first, then read state so hint matches the actual move
		mover.ToggleCheck();
		string label = mover.GetCurrentLabel();

		SCR_PlayerController pc = CGQC_GetPlayerController(pUserEntity);
		if (pc)
			pc.CGQC_Rpc_ShowMoverHint("Champ de tir", label);
	}

	override bool CanBeShownScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()     { return false; }
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
		if (!mover) return;

		mover.Reset();

		SCR_PlayerController pc = CGQC_GetPlayerController(pUserEntity);
		if (pc)
			pc.CGQC_Rpc_ShowResetHint();
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}