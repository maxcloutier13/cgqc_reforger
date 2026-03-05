modded class SCR_PlayerController : PlayerController
{
	protected bool m_bCGQC_Initialized = false;
	
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
			GetGame().GetCallqueue().CallLater(CGQC_Scripts.initializePlayer, 5000, false, to, playerId, playerIdentityId, m_bCGQC_Initialized);
		} else {
			
			
			// Skip if already initialized
			PrintFormat("[CGQC_OnControlledEntityChanged] Initialised already? : %1", m_bCGQC_Initialized);
	        if (m_bCGQC_Initialized)
	        {
	            Print("[CGQC_OnControlledEntityChanged] Already initialized, skipping");
	            return;
	        }
			PrintFormat("[CGQC_OnControlledEntityChanged] Init sanity check. Should be true: %1 - Proceeding with init", m_bCGQC_Initialized);
			// Init running on client
			PrintFormat("[CGQC_OnControlledEntityChanged] Valid spawn for playerId: %1 - Running client-side code", GetPlayerId());
			GetGame().GetCallqueue().CallLater(CGQC_Scripts.welcomePlayer, 5000, false, to, playerId, playerIdentityId);
		}
		
		m_bCGQC_Initialized = true;
	}
}