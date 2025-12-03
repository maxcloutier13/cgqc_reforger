modded class SCR_PlayerController : PlayerController
{
	
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		if (!to)
			return;
		
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		// Check if the controlled entity is the player's main entity (not a possessed unit)
		IEntity mainEntity = GetMainEntity();
		if (to != mainEntity)
		{
			Print("[CGQC_OnControlledEntityChanged] Possessing another entity, skipping initialization");
			return;
		}
		
		PrintFormat("[CGQC_OnControlledEntityChanged] Valid spawn for playerId: %1", GetPlayerId());
		int playerId = GetPlayerId();
		string playerIdentityId = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		GetGame().GetCallqueue().CallLater(CGQC_Scripts.initializePlayer, 10000, false, to, playerId, playerIdentityId);
	}
}


/*
modded class SCR_PlayerController : PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        // Check if we're taking control of a new entity (player spawn)
        if (to)
        {
			PrintFormat("[CGQC_TakingControl] Waiting %1s", 10);
            // Wait 10 seconds before running initialization code
			int playerId = GetPlayerId();
			// Bohemia ID
        	string playerIdentityId = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			PrintFormat("[CGQC_identifyPlayer] playerId: %1 - playerIdentityId: %2 - playerName: %3", playerId, playerIdentityId);
            GetGame().GetCallqueue().CallLater(CGQC_Scripts.initializePlayer, 10000, false, to, playerId, playerIdentityId);
        }
    }
}
*/