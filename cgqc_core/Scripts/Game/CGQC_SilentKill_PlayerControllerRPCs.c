// ============================================================
// CGQC_SilentKill_PlayerControllerRPCs.c (cgqc_scripts)
// Adds silent-kill request RPC to SCR_PlayerController.
// Client -> Server: server re-validates and performs the kill.
// ============================================================

modded class SCR_PlayerController
{
	// Public wrapper - called normally by the client. Triggers the real
	// network dispatch internally, since Rpc() is protected and can only
	// be called from within this class.
	void CGQC_AskSilentKill(RplId targetId)
	{
		Rpc(CGQC_RpcAsk_SilentKill, targetId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void CGQC_RpcAsk_SilentKill(RplId targetId)
	{
		IEntity targetEntity = IEntity.Cast(Replication.FindItem(targetId));
		if (!targetEntity)
			return;

		CharacterControllerComponent charController = CharacterControllerComponent.Cast(
			targetEntity.FindComponent(CharacterControllerComponent));
		if (!charController)
			return;

		if (charController.GetLifeState() == ECharacterLifeState.DEAD)
			return;

		if (!charController.IsUnconscious())
			return;

		SCR_DamageManagerComponent dmgMgr = SCR_DamageManagerComponent.GetDamageManager(targetEntity);
		if (!dmgMgr)
			return;

		Instigator instigator = Instigator.CreateInstigator(GetControlledEntity());
		dmgMgr.Kill(instigator);
	}
}