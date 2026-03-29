modded class BON_TapShoulderAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(pOwnerEntity.FindComponent(SCR_CharacterControllerComponent));
		if (characterController)
			characterController.AskCameraShake();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			return;
		}

		int tapperPlayerId = playerManager.GetPlayerIdFromControlledEntity(pUserEntity);
		if (tapperPlayerId == 0)
			return;
		
		string tapperName = CGQC_Scripts.GetCustomPlayerName(tapperPlayerId);
		
		if (tapperName.IsEmpty())
			return;

		int ownerPlayerId = playerManager.GetPlayerIdFromControlledEntity(pOwnerEntity);
		
		if (ownerPlayerId == 0)
			return;

		BON_TapShoulderReceiverComponent receiver = BON_TapShoulderReceiverComponent.Cast(pOwnerEntity.FindComponent(BON_TapShoulderReceiverComponent));

		if (!receiver)
			return;

		BON_TapShoulderReceiverComponent tapperReceiver = BON_TapShoulderReceiverComponent.Cast(pUserEntity.FindComponent(BON_TapShoulderReceiverComponent));

		if (tapperReceiver)
			tapperReceiver.ShowTapNotification("You tapped " + tapperName);

		receiver.ShowTapNotification(tapperName + " is on you");
	}
}

class BON_TapShoulderReceiverComponentClass : ScriptComponentClass {}

class BON_TapShoulderReceiverComponent : ScriptComponent
{
	void ShowTapNotification(string message)
	{
		Rpc(RpcDo_ShowTapNotification, message);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_ShowTapNotification(string message)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
		{
			return;
		}

		popup.PopupMsg(message, 1.0);
	}
}