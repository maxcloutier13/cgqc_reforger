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
			
		Print("[CGQC_FF_TakingControl] Waiting a bit");
		int playerId = GetPlayerId();
        GetGame().GetCallqueue().CallLater(CGQC_Scripts.initializeFFPlayer, 10000, false, to, playerId);
        
    }
}