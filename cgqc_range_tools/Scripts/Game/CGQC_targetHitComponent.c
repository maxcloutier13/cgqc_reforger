// CGQC_TargetHitComponent.c
//
// Two parts:
//
// 1. CGQC_TargetHitEffect — a BaseProjectileEffect assigned to ammo configs.
//    Fires on the shooter's client on every bullet hit.
//    Checks if the hit entity has CGQC_TargetHitComponent and shows the hint locally.
//
// 2. CGQC_TargetHitComponent — a marker component placed on CQB_TargetBlank_CGQC.et.
//    No logic needed — just used as an identifier tag.
//
// Setup:
//   - Add CGQC_TargetHitComponent to CQB_TargetBlank_CGQC.et in Workbench
//   - Remove SCR_DamageManagerComponent from CQB_TargetBlank_CGQC.et (not needed)
//   - For each ammo type you want to detect (e.g. ProjBullet_556x45_Ball.et),
//     create an inherited prefab in cgqc_training, add CGQC_TargetHitEffect
//     to its Projectile Effects array in the ammo config

//------------------------------------------------------------------------------------------------
class CGQC_TargetHitEffect : BaseProjectileEffect
{
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, notnull Instigator instigator, string colliderName, float speed)
	{
		if (!pHitEntity)
			return;

		// Check if the shooter is the local player
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		IEntity localPlayer = pc.GetControlledEntity();
		if (!localPlayer || instigator.GetInstigatorEntity() != localPlayer)
			return;

		// Check if hit entity is a training target
		CGQC_TargetHitComponent hitComp = CGQC_TargetHitComponent.Cast(
			pHitEntity.FindComponent(CGQC_TargetHitComponent)
		);

		if (!hitComp)
			return;

		// Subtract ~1m to account for eye-level vs entity origin offset
		float distance = vector.Distance(localPlayer.GetOrigin(), pHitEntity.GetOrigin()) - 1.0;

		string msg = string.Format(
			"Distance: %1 m  |  Vélocité: %2 m/s",
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
// Reset action — placed directly on CQB_TargetBlank_CGQC.et
// Reads the prefab resource from CGQC_TargetHitComponent on the same entity
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

        CGQC_ResetHelper.s_sName = pOwnerEntity.GetName();
        CGQC_ResetHelper.s_fBaseX = pOwnerEntity.GetOrigin()[0];
        CGQC_ResetHelper.s_sPrefab = m_sRespawnPrefab;
        pOwnerEntity.GetTransform(CGQC_ResetHelper.s_vTransform);

        SCR_EntityHelper.DeleteEntityAndChildren(pOwnerEntity);
        GetGame().GetCallqueue().CallLater(CGQC_ResetHelper.DoReset, 100, false);

        SCR_HintManagerComponent hint = SCR_HintManagerComponent.GetInstance();
        if (hint)
            hint.ShowCustomHint("Reset de la cible", "", 2.0);
    }

    override bool CanBeShownScript(IEntity user) { return true; }
    override bool CanBePerformedScript(IEntity user) { return true; }
    override bool HasLocalEffectOnlyScript() { return false; }
}