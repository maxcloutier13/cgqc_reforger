// CGQC_TargetHitComponent.c

// ----------------------------------------------------------------
// Self-contained reset helper for steel targets
// ----------------------------------------------------------------
class CGQC_TargetSelfResetHelper
{
	static string s_sName;
	static float s_fBaseX;
	static ResourceName s_sPrefab;
	static vector s_vTransform[4];

	static void DoReset()
	{
		Resource res = Resource.Load(s_sPrefab);
		if (!res || !res.IsValid())
		{
			Print("[CGQC_TargetSelfResetHelper] Resource.Load failed: " + s_sPrefab, LogLevel.WARNING);
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
			Print("[CGQC_TargetSelfResetHelper] SpawnEntityPrefab failed.", LogLevel.ERROR);
			return;
		}

		if (!s_sName.IsEmpty())
			newEnt.SetName(s_sName);
	}
}


//------------------------------------------------------------------------------------------------
class CGQC_TargetHitEffect : BaseProjectileEffect
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

		CGQC_TargetHitComponent hitComp = CGQC_TargetHitComponent.Cast(
			pHitEntity.FindComponent(CGQC_TargetHitComponent)
		);

		if (!hitComp)
			return;

		float distance = vector.Distance(localPlayer.GetOrigin(), pHitEntity.GetOrigin()) - 1.0;

		string msg = string.Format(
			"Distance: %1 m | Vélocité@Impact: %2 m/s",
			Math.Round(distance).ToString(),
			Math.Round(speed).ToString()
		);

		SCR_HintManagerComponent hint = SCR_HintManagerComponent.GetInstance();
		if (hint)
			hint.ShowCustomHint(msg, "Impact", 4.0);
	}
}


//------------------------------------------------------------------------------------------------
// Marker component — placed on the target prefab, no logic required
class CGQC_TargetHitComponentClass : ScriptComponentClass {}

class CGQC_TargetHitComponent : ScriptComponent
{
}


//------------------------------------------------------------------------------------------------
// Reset action — placed directly on the target prefab
//------------------------------------------------------------------------------------------------
class CGQC_TargetSelfResetAction : ScriptedUserAction
{
	[Attribute(defvalue: "", desc: "Prefab to respawn (e.g. CQB_TargetBlank_CGQC.et)", params: "et")]
	protected ResourceName m_sRespawnPrefab;

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		if (!pOwnerEntity || m_sRespawnPrefab.IsEmpty())
		{
			Print("[CGQC_TargetSelfResetAction] No owner or respawn prefab not set.", LogLevel.WARNING);
			return;
		}

		CGQC_TargetSelfResetHelper.s_sName   = pOwnerEntity.GetName();
		CGQC_TargetSelfResetHelper.s_fBaseX  = pOwnerEntity.GetOrigin()[0];
		CGQC_TargetSelfResetHelper.s_sPrefab = m_sRespawnPrefab;
		pOwnerEntity.GetTransform(CGQC_TargetSelfResetHelper.s_vTransform);

		SCR_EntityHelper.DeleteEntityAndChildren(pOwnerEntity);
		GetGame().GetCallqueue().CallLater(CGQC_TargetSelfResetHelper.DoReset, 100, false);

		SCR_HintManagerComponent hint = SCR_HintManagerComponent.GetInstance();
		if (hint)
			hint.ShowCustomHint("Reset de la cible", "", 2.0);
	}

	override bool CanBeShownScript(IEntity user)     { return true; }
	override bool CanBePerformedScript(IEntity user) { return true; }
	override bool HasLocalEffectOnlyScript()         { return false; }
}