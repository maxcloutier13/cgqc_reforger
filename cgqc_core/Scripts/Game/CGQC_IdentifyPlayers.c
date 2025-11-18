modded class SCR_PlayerController : PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        // Check if we're taking control of a new entity (player spawn)
        if (to)
        {
			PrintFormat("[CGQC_TakingControl] Waiting %1s", 10);
            // Wait 5 seconds before running initialization code
            GetGame().GetCallqueue().CallLater(CGQC_InitializePlayer, 10000, false, to);
        }
    }

    // Initialize player after delay
    void CGQC_InitializePlayer(IEntity playerEntity)
    {
        Print("[CGQC_InitializePlayer] Starting ->");
        // Get player's Steam ID
        int playerId = GetPlayerId();
        string steamId = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
        // Get player's name
        string playerName = GetPlayerName();
        PrintFormat("[CGQC_InitializePlayer] playerId: %1s - steamId: %2s playerName:%3 ", playerId, steamId, playerName);

        // Check Steam ID and activate specific features for certain players
        switch (steamId)
        {
            case "d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier
            {
                Print("[CGQC_InitializePlayer] Cloutier detected - activating admin features");
				GetGame().GetCallqueue().CallLater(ActivateAdminFeatures, 5000, false, playerEntity);
                break;
            }
            default:
            {
                PrintFormat("[CGQC_InitializePlayer] Unrecognized player %1 (Steam ID: %2) has spawned", playerName, steamId);
				// Display welcome message
		        string message = string.Format("Salut, %1! Bienvenue sur le serveur CGQC. Have fun.", playerName);
		        SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Salut!", 10.0);
                break;
            }
        }



        Print("[CGQC_InitializePlayer] Done <-");
    }

    // Helper method to get player name
    string GetPlayerName()
    {
        int playerId = GetPlayerId();
        return GetGame().GetPlayerManager().GetPlayerName(playerId);
    }

    // Admin features activation
    void ActivateAdminFeatures(IEntity playerEntity)
    {
		Print("[CGQC_InitializePlayer] Admin features running");
		// Cloutier time
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{F723BDF891EFECAE}Prefabs/Items/Smokeables/Smokeable_Joint.et");
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{E513AC48A65855AA}Prefabs/Items/Smokeables/Smokeable_Cigar.et");
        CGQC_Scripts.CheckAndGiveItem(playerEntity, "{33CBDE73AB48172A}Prefabs/Weapons/Explosives/DemoBlock_M112/DemoBlock_M112.et");
        SCR_HintManagerComponent.GetInstance().ShowCustomHint("Admin mode activated", "System", 3.0);
		string message = string.Format("Cloutier sti!");
		SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Master", 10.0);
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && !editorManager.IsOpened())
		{
		    editorManager.Open();
		}
    }




}





/*
"d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier
*/