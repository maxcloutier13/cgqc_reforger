// ============================================================
// CGQC_SilentKillAction.c (cgqc_scripts)
// Achever Silencieusement — visible only on unconscious, living targets.
// Runs client-local, then explicitly asks the server to kill,
// rather than relying on ScriptedUserAction's own server-forward.
//
// SETUP:
//   Add CGQC_SilentKillAction to the character prefab's
//   ActionsManagerComponent in Workbench.
// ============================================================

class CGQC_SilentKillAction : ScriptedUserAction
{
	override bool GetActionNameScript(out string outName)
	{
		outName = "Achever Silencieusement";
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return TargetIsValidForSilentKill();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return TargetIsValidForSilentKill();
	}

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		RplId targetId = Replication.FindId(pOwnerEntity);
		if (!targetId.IsValid())
			return;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		int checkerPlayerId = pm.GetPlayerIdFromControlledEntity(pUserEntity);
		SCR_PlayerController checkerPC = SCR_PlayerController.Cast(pm.GetPlayerController(checkerPlayerId));
		if (!checkerPC)
			return;

		checkerPC.CGQC_AskSilentKill(targetId);
	}

	protected bool TargetIsValidForSilentKill()
	{
		CharacterControllerComponent charController = CharacterControllerComponent.Cast(
			GetOwner().FindComponent(CharacterControllerComponent));
		if (!charController)
			return false;

		if (charController.GetLifeState() == ECharacterLifeState.DEAD)
			return false;

		return charController.IsUnconscious();
	}
}