modded class BON_TapShoulderAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		int tapperPlayerId = playerManager.GetPlayerIdFromControlledEntity(pUserEntity);
		if (tapperPlayerId == 0)
			return;

		string tapperName = CGQC_Scripts.GetCustomPlayerName(tapperPlayerId);
		if (tapperName.IsEmpty())
			return;

		int ownerPlayerId = playerManager.GetPlayerIdFromControlledEntity(pOwnerEntity);
		if (ownerPlayerId == 0)
			return;

		string ownerName = CGQC_Scripts.GetCustomPlayerName(ownerPlayerId);
		if (ownerName.IsEmpty())
			return;

		BON_TapShoulderReceiverComponent receiver = BON_TapShoulderReceiverComponent.Cast(pOwnerEntity.FindComponent(BON_TapShoulderReceiverComponent));
		if (!receiver)
			return;

		BON_TapShoulderReceiverComponent tapperReceiver = BON_TapShoulderReceiverComponent.Cast(pUserEntity.FindComponent(BON_TapShoulderReceiverComponent));

		// Tapped player gets the shake + notification
		receiver.ShowTapNotification(tapperName + " is on you", true);

		// Tapper gets notification only
		if (tapperReceiver)
			tapperReceiver.ShowTapNotification("You tapped " + ownerName, false);
	}
}

class BON_TapShoulderReceiverComponentClass : ScriptComponentClass {}

class BON_TapShoulderReceiverComponent : ScriptComponent
{
	void ShowTapNotification(string message, bool shake)
	{
		Rpc(RpcDo_ShowTapNotification, message, shake);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_ShowTapNotification(string message, bool shake)
	{
		if (shake)
		{
			SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(GetOwner().FindComponent(SCR_CharacterControllerComponent));
			if (cc)
				cc.AskCameraShake();
		}

		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
			return;

		popup.PopupMsg(message, 1.0);
	}
}