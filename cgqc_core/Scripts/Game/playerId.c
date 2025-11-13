modded class SCR_PlayerController : PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        // Check if we're taking control of a new entity (player spawn)
        if (to)
        {
			PrintFormat("[CGQC_TakingControl] Waiting %1s", 5);
            // Wait 5 seconds before running initialization code
            GetGame().GetCallqueue().CallLater(InitializePlayer, 5000, false, to);
        }
    }

    // Initialize player after delay
    void InitializePlayer(IEntity playerEntity)
    {
        Print("[CGQC_InitializePlayer] Starting ->");
        // Get player's Steam ID
        int playerId = GetPlayerId();
        string steamId = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
        // Get player's name
        string playerName = GetPlayerName();

        // Check Steam ID and activate specific features for certain players
        switch (steamId)
        {
            case "d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier
            {
                Print("[CGQC_InitializePlayer] Cloutier detected - activating admin features");
                ActivateAdminFeatures(playerEntity);
                break;
            }
            default:
            {
                PrintFormat("[CGQC_InitializePlayer] Unrecognized player %1 (Steam ID: %2) has spawned", playerName, steamId);
				// Display welcome message
		        string message = string.Format("Salut, %1!", playerName);
		        SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Bienvenue", 10.0);
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
		// Cig time
		GiveItemToPlayer(playerEntity, "{F723BDF891EFECAE}Prefabs/Items/Smokeables/Smokeable_Joint.et");
		GiveItemToPlayer(playerEntity, "{E513AC48A65855AA}Prefabs/Items/Smokeables/Smokeable_Cigar.et");
        GiveItemToPlayer(playerEntity, "{33CBDE73AB48172A}Prefabs/Weapons/Explosives/DemoBlock_M112/DemoBlock_M112.et");
        SCR_HintManagerComponent.GetInstance().ShowCustomHint("Admin mode activated", "System", 3.0);
		string message = string.Format("Cloutier sti!");
		SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Master", 10.0);
    }

	// Give item to player
	void GiveItemToPlayer(IEntity playerEntity, string itemToAdd)
    {
        PrintFormat("[CGQC_GiveItemToPlayer] called for item: %1", itemToAdd);

        SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));

        if (!inventoryManager)
        {
            PrintFormat("[CGQC_GiveItemToPlayer] ERROR: No inventory manager");
            return;
        }

        // Let the system choose the best storage automatically by passing null
        bool success = inventoryManager.TrySpawnPrefabToStorage(itemToAdd, null, -1, EStoragePurpose.PURPOSE_ANY);

        PrintFormat("[CGQC_GiveItemToPlayer] Result: %1", success);
    }
}

/*
"d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier
*/