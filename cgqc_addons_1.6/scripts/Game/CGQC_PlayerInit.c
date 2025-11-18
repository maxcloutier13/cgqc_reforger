modded class SCR_PlayerController : PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        // Check if we're taking control of a new entity (player spawn)
        if (to)
        {
			PrintFormat("[CGQC_Addons_TakingControl] Waiting %1s", 5);
            // Wait 5 seconds before running initialization code
            GetGame().GetCallqueue().CallLater(CGQC_Addons_InitializePlayer, 5000, false, to);
        }
    }

    // Initialize player after delay
    void CGQC_Addons_InitializePlayer(IEntity playerEntity)
    {
        Print("[CGQC_Addons_InitializePlayer] Starting ->");
	
		
        Print("[CGQC_Addons_InitializePlayer] Done <-");
    }
	
}


/*
"d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier
*/