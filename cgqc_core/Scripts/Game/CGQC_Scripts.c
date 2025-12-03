class CGQC_Scripts
{
	
	// Init for FreedomFighters
	static void initializeFFPlayer(IEntity playerEntity)
    {
        Print("[CGQC_InitializeFFPlayer] Giving radio/armband to player");
        CGQC_Scripts.CheckAndGiveItem(playerEntity, "{540C08AD5F21A5FA}Prefabs/Items/Equipment/Radios/Radio_R148_FIA.et");
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{81A4D576093ED337}Prefabs/Characters/Bandeau/Brassard_Green.et");
    }
	
	// Basic init for player 
    static void initializePlayer(IEntity playerEntity, int playerId, string playerIdentityId)
    {
        Print("[CGQC_InitializePlayer] Starting ->");
        // Get player's name
        string cgqc_playerName = GetPlayerName(playerId);
        PrintFormat("[CGQC_InitializePlayer] playerId: %1 - playerIdentityId: %2 - playerName: %3", playerId, playerIdentityId, cgqc_playerName);
		// CGQC flag
		bool isCGQC = false;
		// Identity
		string cgqc_head = "";
		string cgqc_body = "";

        // Check Steam ID and activate specific features for certain players
        switch (playerIdentityId)
        {
            case "d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier local
            {
                Print("[CGQC_InitializePlayer] CGQC Cloutier LOCAL detected - activating admin features");
				GetGame().GetCallqueue().CallLater(CGQC_Scripts.ActivateAdminFeatures, 5000, false, playerEntity);
				cgqc_head = "{BB234A3ADB246C1C}Prefabs/Characters/Heads/Head_livonianHead_9.et";
				cgqc_body = "{000A00972B3D8EF5}Prefabs/Characters/Basebody/CharacterBasebody_03.et";				
				isCGQC = true;
                break;
            }
			case "d7e9113c-f075-41c5-a72a-9ee5187dc723s":
            {
                Print("[CGQC_InitializePlayer] CGQC Cloutier detected - activating admin features");
				GetGame().GetCallqueue().CallLater(CGQC_Scripts.ActivateAdminFeatures, 5000, false, playerEntity);
				cgqc_head = "{BB234A3ADB246C1C}Prefabs/Characters/Heads/Head_livonianHead_9.et";
				cgqc_body = "{000A00972B3D8EF5}Prefabs/Characters/Basebody/CharacterBasebody_03.et";
				isCGQC = true;
                break;
            }
			case "120786ec-60cd-4a96-9e10-c846578745f2s":
            {
                Print("[CGQC_InitializePlayer] CGQC Genest detected");
				isCGQC = true;
                break;
            }
			case "c3fcb72f-2f87-4937-8847-1fb62b39a40cs":
            {
                Print("[CGQC_InitializePlayer] CGQC Tremblay detected");
				isCGQC = true;
                break;
            }
			case "e1c8cdf2-953a-4db0-be45-2cd8e79556f2s":
            {
                Print("[CGQC_InitializePlayer] CGQC Valiquette detected");
				isCGQC = true;
                break;
            }
			case "fdeca5b5-5cba-403a-8e3a-d598325dbcccs":
            {
                Print("[CGQC_InitializePlayer] CGQC Dubé detected");
				isCGQC = true;
                break;
            }
			case "60e84a60-c12f-4b4a-91ed-c294bab6046es":
            {
                Print("[CGQC_InitializePlayer] CGQC Comeau detected");
				isCGQC = true;
                break;
            }
			case "c6643f04-50cb-461a-b591-b6e6deb63ed9s":
            {
                Print("[CGQC_InitializePlayer] CGQC Fournier detected");
				isCGQC = true;
               
            }
        }
		
		/* Player id's of CGQC
		"d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier local
		"120786ec-60cd-4a96-9e10-c846578745f2s" : ElButteur / Genest
		"c3fcb72f-2f87-4937-8847-1fb62b39a40cs" : Darkangel898 / Tremblay
		"d7e9113c-f075-41c5-a72a-9ee5187dc723s" : silent1 / Cloutier
		"fdeca5b5-5cba-403a-8e3a-d598325dbcccs" : Dubaille / Dubé
		"60e84a60-c12f-4b4a-91ed-c294bab6046es" : Pig'sPeels / Comeau
		"e1c8cdf2-953a-4db0-be45-2cd8e79556f2s" : HellRik / Valiquette
		"c6643f04-50cb-461a-b591-b6e6deb63ed9s" : Psyck0u / Fournier		
		"78216978-6757-4e04-83c5-f05a7c1ed058s" :inmateQc / Winters
		*/
		
		
		if(isCGQC)
		{
			// Display CGQC message
			GetGame().GetCallqueue().CallLater(CGQC_Scripts.ActivateCGQCFeatures, 5000, false, playerEntity, cgqc_playerName);
			// Apply identity if head and body are set
			if (!cgqc_head.IsEmpty() && !cgqc_body.IsEmpty())
			{
				Print("[CGQC_InitializePlayer] Applying custom identity");
				SetPlayerIdentity(playerEntity, cgqc_head, cgqc_body);
			}
		} else {
            PrintFormat("[CGQC_InitializePlayer] Unrecognized player %1 has spawned", cgqc_playerName);
			// Display welcome message
	        string message = string.Format("Salut, %1! Bienvenue sur le serveur CGQC. Rejoins-nous sur discord: cgqc.ca - Have fun!", cgqc_playerName);
	        SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Salut!", 10.0);

		}
        Print("[CGQC_InitializePlayer] Done <-");
    }
	
	// Set player identity (works on both client and server)
	static void SetPlayerIdentity(IEntity playerEntity, string headPath, string bodyPath)
	{
		if (!playerEntity)
		{
			Print("[CGQC_SetPlayerIdentity] Invalid player entity", LogLevel.ERROR);
			return;
		}

		SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(playerEntity);
		if (!playerCharacter)
		{
			Print("[CGQC_SetPlayerIdentity] Failed to cast to SCR_ChimeraCharacter", LogLevel.ERROR);
			return;
		}

		// Find the identity selector component if it exists
		MFN_IdentitySelectorCharacterComponent idSelector = MFN_IdentitySelectorCharacterComponent.Cast(
			playerCharacter.FindComponent(MFN_IdentitySelectorCharacterComponent)
		);

		if (idSelector)
		{
			// Use the existing component's RPC system
			PrintFormat("[CGQC_SetPlayerIdentity] Using identity selector component: %1 | %2", headPath, bodyPath);
			idSelector.RemoteServerSetIdentity(headPath, bodyPath);
		}
		else
		{
			// Fallback: set identity directly on server
			PrintFormat("[CGQC_SetPlayerIdentity] Setting identity directly: %1 | %2", headPath, bodyPath);
			
			if (Replication.IsServer())
			{
				SetIdentityDirect(playerCharacter, headPath, bodyPath);
			}
		}
	}

	// Direct identity setting (server-side only)
	static void SetIdentityDirect(SCR_ChimeraCharacter playerCharacter, string headPath, string bodyPath)
	{
		CharacterIdentityComponent idComponent = CharacterIdentityComponent.Cast(
			playerCharacter.FindComponent(CharacterIdentityComponent)
		);
		
		if (!idComponent)
		{
			Print("[CGQC_SetIdentityDirect] CharacterIdentityComponent not found", LogLevel.ERROR);
			return;
		}

		Identity playerID = idComponent.GetIdentity();
		if (!playerID)
		{
			Print("[CGQC_SetIdentityDirect] Identity not found", LogLevel.ERROR);
			return;
		}

		VisualIdentity newVisID = playerID.GetVisualIdentity();
		if (!newVisID)
		{
			newVisID = new VisualIdentity();
		}

		newVisID.SetHead(headPath);
		newVisID.SetBody(bodyPath);
		playerID.SetVisualIdentity(newVisID);
		idComponent.SetIdentity(playerID);

		Print("[CGQC_SetIdentityDirect] Identity applied successfully");
	}

    // Helper method to get player name
    static string GetPlayerName(int playerId)
    {
        
        return GetGame().GetPlayerManager().GetPlayerName(playerId);
    }
	
	static void ActivateCGQCFeatures(IEntity playerEntity, string name)
    {
		Print("[CGQC_ActivateCGQCFeatures] Setup up for CGQC player");
		// We have a group member
		array<string> i_welcome_list = {
		    "What's good", "Howdy", "Hiya", "Wassup", "Yo", "R’gard", "Allo", "Hello", "Ooooh",
			"Coucou", "Bonsoir", "Konnichiwa", "Hola", "Hallo", "Nǐ hǎo", "Hoi", "Merhaba", "Vitayu"
		};
		array<string> i_message_list = {
		   "what's up?", "what's up buddy?", "Time to fuck shit up",
			"Asti que t'es beau", "Ça roule ma poule?", "As-tu couché ta blonde?",
			"BAN dans 3,2,1...", "What's cookin'?", "Wassup homie?", "Greetings and salutations!"
		};
		
		// Picks random greetings
		string init_WelcomeTxt = string.Format("%1, %2", i_welcome_list.GetRandomElement(), name);
		string init_WelcomeMsg = i_message_list.GetRandomElement();
		PrintFormat("[CGQC_ActivateCGQCFeatures] txt:%1/msg:%2 is the random msg", init_WelcomeTxt, init_WelcomeMsg);
		// Show welcome
		SCR_HintManagerComponent.GetInstance().ShowCustomHint(init_WelcomeMsg, init_WelcomeTxt, 10.0);
	}

    // Admin features activation
    static void ActivateAdminFeatures(IEntity playerEntity)
    {
		Print("[CGQC_InitializePlayer] Admin features running");
		// Cloutier time
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{F723BDF891EFECAE}Prefabs/Items/Smokeables/Smokeable_Joint.et");
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{E513AC48A65855AA}Prefabs/Items/Smokeables/Smokeable_Cigar.et");
        CGQC_Scripts.CheckAndGiveItem(playerEntity, "{33CBDE73AB48172A}Prefabs/Weapons/Explosives/DemoBlock_M112/DemoBlock_M112.et");
        
		
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && !editorManager.IsOpened())
		{
		    editorManager.Open();
		}
		
    }
	
	
	// Give item to player. No checks. 
	static void GiveItemToPlayer(IEntity playerEntity, string itemToAdd)
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
	
	// Give item if none is present
	static void CheckAndGiveItem(IEntity playerEntity, ResourceName itemPrefab)
	{
	    PrintFormat("[CGQC_CheckAndGiveItem] called for item: %1", itemPrefab);
	    if (!playerEntity)
	        return;
	
	    SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	    if (!invManager)
	        return;
	
	    // Use CallLater to delay the check slightly so inventory updates
	    GetGame().GetCallqueue().CallLater(CGQC_Scripts.CheckAndGiveItemDelayed, 50, false, playerEntity, invManager, itemPrefab);
	}
	
	static void CheckAndGiveItemDelayed(IEntity playerEntity, SCR_InventoryStorageManagerComponent invManager, ResourceName itemPrefab)
	{
	    // Check if player already has this item
	    if (CGQC_Scripts.HasItemInInventory(invManager, itemPrefab))
	    {
	        PrintFormat("[CGQC_CheckAndGiveItem] Player already has item: %1", itemPrefab);
	        return;
	    }
	    
	    PrintFormat("[CGQC_CheckAndGiveItem] Player doesn't have item: %1 -> Giving", itemPrefab);
	    GiveItemToPlayer(playerEntity, itemPrefab);
	}
	
	static bool HasItemInInventory(SCR_InventoryStorageManagerComponent invManager, ResourceName itemPrefab)
	{
	    if (!invManager)
	        return false;
	
	    PrintFormat("[HasItemInInventory] Searching for: %1", itemPrefab);
	    
	    // Get all storages first
	    array<BaseInventoryStorageComponent> storages = {};
	    invManager.GetStorages(storages);
	    
	    PrintFormat("[HasItemInInventory] Found %1 storages", storages.Count());
	    
	    // Check each storage
	    foreach (BaseInventoryStorageComponent storage : storages)
	    {
	        if (!storage)
	            continue;
	        
	        array<IEntity> items = {};
	        invManager.GetAllItems(items, storage);
	        
	        foreach (IEntity item : items)
	        {
	            if (!item)
	                continue;
	            
	            EntityPrefabData prefabData = item.GetPrefabData();
	            if (!prefabData)
	                continue;
	            
	            string itemPrefabName = prefabData.GetPrefabName();
	            
	            if (itemPrefabName == itemPrefab)
	            {
	                PrintFormat("[HasItemInInventory] MATCH FOUND!");
	                return true;
	            }
	        }
	    }
	    
	    return false;
	}
}

