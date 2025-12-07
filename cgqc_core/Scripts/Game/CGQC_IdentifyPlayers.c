modded class SCR_PlayerController : PlayerController
{
	
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		if (!to)
			return;
		
		// Check if the controlled entity is the player's main entity (not a possessed unit)
		IEntity mainEntity = GetMainEntity();
		if (to != mainEntity)
		{
			Print("[CGQC_OnControlledEntityChanged] Possessing another entity, skipping initialization");
			return;
		}
		int playerId = GetPlayerId();
		string playerIdentityId = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		
		// Check where code is running
		if (Replication.IsServer())
		{
			// Init running on server
			PrintFormat("[CGQC_OnControlledEntityChanged] Valid spawn for playerId: %1 - Running server-side code", GetPlayerId());
			GetGame().GetCallqueue().CallLater(CGQC_Scripts.initializePlayer, 5000, false, to, playerId, playerIdentityId);
		} else {
			// Init running on client
			PrintFormat("[CGQC_OnControlledEntityChanged] Valid spawn for playerId: %1 - Running client-side code", GetPlayerId());
			GetGame().GetCallqueue().CallLater(CGQC_Scripts.welcomePlayer, 5000, false, to, playerId, playerIdentityId);
		}
	}
}