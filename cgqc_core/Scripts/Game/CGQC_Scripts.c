class CGQC_Scripts
{
	// Get name from mod CustomName
	static string GetCustomPlayerName(int playerID)
	{
		/*
	    // Try CustomNamesManager first
	    CustomNamesManager cnm = CustomNamesManager.GetInstance();
	    if (cnm)
	    {
	        string customName = cnm.GetCustomName(playerID);
	        if (!customName.IsEmpty())
	            return customName;
	    }*/

	    // Fallback to Steam name
	    return GetGame().GetPlayerManager().GetPlayerName(playerID);
	}

	// Init for FreedomFighters
	static void initializeFFPlayer(IEntity playerEntity)
    {
        Print("[CGQC_InitializeFFPlayer] Giving radio/armband to player");
        CGQC_Scripts.CheckAndGiveItem(playerEntity, "{E82129A6CC014809}Prefabs/Items/Equipment/Radios Base/CGQC_Radio_3_152.et");
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{81A4D576093ED337}Prefabs/Characters/Bandeau/Brassard_Green.et");
    }

	static void swapRadios(IEntity playerEntity)
	{
		// Modcheck. If cgqc_radio missing: skip
		ResourceName cgqcRadio = "{E82129A6CC014809}Prefabs/Items/Equipment/Radios Base/CGQC_Radio_3_152.et";
		Resource res = Resource.Load(cgqcRadio);
		if (!res.IsValid())
		{
		    Print("[CGQC_swapRadios] CGQC radio mod not present, skipping swap");
		    return;
		}

	    SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	    if (!invManager)
	        return;

	    array<ResourceName> radiosToClear = {
	        "{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et",
	        "{E1A5D4B878AA8980}Prefabs/Items/Equipment/Radios/Radio_R148.et",
	        "{540C08AD5F21A5FA}Prefabs/Items/Equipment/Radios/Radio_R148_FIA.et",
	        "{C55821E8E86C074E}Prefabs/Items/Equipment/Radios/Radio_ANPRC152.et",
	        "{9CB82F893D614FF0}Prefabs/Items/Equipment/Radios/Radio_ANPRC152A.et",
	        "{D69684663E89D6EA}Prefabs/Items/Equipment/Radios/Radio_ANPRC152A_OLD.et",
	        "{B343AB76B0725657}Prefabs/Items/Equipment/Radios/Radio_R187P1_OLD.et"
	    };

	    array<IEntity> items = {};
	    invManager.GetAllItems(items, null);

	    foreach (IEntity item : items)
	    {
	        EntityPrefabData prefabData = item.GetPrefabData();
	        if (!prefabData)
	            continue;

	        if (radiosToClear.Contains(prefabData.GetPrefabName()))
	        {
	            PrintFormat("[CGQC_swapRadios] Found radio: %1 -> Removing", prefabData.GetPrefabName());
	            invManager.TryRemoveItemFromInventory(item);
	            PrintFormat("[CGQC_swapRadios] Swapping to CGQC radio");
	            CGQC_Scripts.CheckAndGiveItem(playerEntity, "{E82129A6CC014809}Prefabs/Items/Equipment/Radios Base/CGQC_Radio_3_152.et");
	            return;
	        }
	    }

	    PrintFormat("[CGQC_swapRadios] No swappable radio found");
	}

	static void welcomePlayer(IEntity playerEntity, int playerId, string playerIdentityId)
    {
		Print("[CGQC_welcomePlayer] Starting ->");
        // Get player's name
        string playerName = GetPlayerName(playerId);
        PrintFormat("[CGQC_welcomePlayer] playerId: %1 - playerIdentityId: %2 - playerName: %3", playerId, playerIdentityId, playerName);
		// CGQC flag
		bool isCGQC = false;

        // Check Steam name and activate specific features for certain players
        switch (playerName)
        {
			case "silent1":
            {
                Print("[CGQC_welcomePlayer] CGQC Cloutier detected - activating admin features");
				isCGQC = true;
                break;
            }
			case "ElButteur":
            {
                Print("[CGQC_welcomePlayer] CGQC Genest detected");
				isCGQC = true;
                break;
            }
			case "Darkangel898":
            {
                Print("[CGQC_welcomePlayer] CGQC Tremblay detected");
				isCGQC = true;
                break;
            }
			case "HellRik":
            {
                Print("[CGQC_welcomePlayer] CGQC Valiquette detected");
				isCGQC = true;
                break;
            }
			case "Dubaille":
            {
                Print("[CGQC_welcomePlayer] CGQC Dubé detected");
				isCGQC = true;
                break;
            }
			case "Pig'sPeels":
            {
                Print("[CGQC_welcomePlayer] CGQC Comeau detected");
				isCGQC = true;
                break;
            }
			case "poolerpol":
            {
                Print("[CGQC_welcomePlayer] CGQC JeuneComeau detected");
				isCGQC = true;
                break;
            }
			case "Psyck0u":
            {
                Print("[CGQC_welcomePlayer] CGQC Fournier detected");
				isCGQC = true;
				break;
            }
        }

		if(isCGQC)
		{
			PrintFormat("[CGQC_welcomePlayer] CGQC player %1 has spawned", playerName);
			// Display CGQC message
			GetGame().GetCallqueue().CallLater(CGQC_Scripts.ActivateCGQCFeatures, 5000, false, playerEntity, playerName);
		}
		else {
            PrintFormat("[CGQC_welcomePlayer] Unrecognized player %1 has spawned", playerName);
			// Display welcome message
	        string message = string.Format("Salut, %1! Bienvenue sur le serveur CGQC. Rejoins-nous sur discord: cgqc.ca - Have fun!", playerName);
	        SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Salut!", 10.0);
		}
        Print("[CGQC_InitializePlayer] Done <-");

	}

	// Roster stuff
	static const ResourceName CGQC_ROSTER_CONFIG = "{262B3F77A01CFAE4}Configs/CGQC/CGQC_Roster.conf";

	static CGQC_PlayerRosterConfig LoadRosterConfig()
	{
		Resource res = Resource.Load(CGQC_ROSTER_CONFIG);
		if (!res.IsValid())
		{
			Print("[CGQC] LoadRosterConfig: Failed to load roster config - resource invalid");
			return null;
		}
		return CGQC_PlayerRosterConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
	}

	static void initializePlayer(IEntity playerEntity, int playerId, string playerIdentityId, bool isInitialised)
	{
		Print("[CGQC_InitializePlayer] Starting ->");

		string playerName = GetPlayerName(playerId);
		PrintFormat("[CGQC_InitializePlayer] playerId: %1 - playerIdentityId: %2 - playerName: %3", playerId, playerIdentityId, playerName);

		CGQC_PlayerRosterConfig roster = CGQC_Scripts.LoadRosterConfig();
		if (!roster || !roster.m_aRankRosters)
		{
			Print("[CGQC_InitializePlayer] No valid roster config found, skipping.");
			return;
		}

		foreach (CGQC_RankRosterConfig rankConfig : roster.m_aRankRosters)
		{
			if (!rankConfig || !rankConfig.m_aPlayers)
				continue;

			foreach (CGQC_PlayerEntry entry : rankConfig.m_aPlayers)
			{
				if (!entry || entry.m_sIdentityId != playerIdentityId)
					continue;

				PrintFormat("[CGQC_InitializePlayer] Matched: %1", entry.m_sLabel);

				// Admin features
				if (!entry.m_sAdminTag.IsEmpty())
					GetGame().GetCallqueue().CallLater(CGQC_Scripts.ActivateAdminFeatures, 5000, false, playerEntity, isInitialised, entry.m_sAdminTag);

				// Custom identity
				if (!entry.m_sHeadPrefab.IsEmpty() && !entry.m_sBodyPrefab.IsEmpty())
					GetGame().GetCallqueue().CallLater(CGQC_Scripts.SetPlayerIdentity, 1000, false, playerEntity, entry.m_sHeadPrefab, entry.m_sBodyPrefab);

				// Rank
				SCR_CharacterRankComponent rankComp = SCR_CharacterRankComponent.Cast(playerEntity.FindComponent(SCR_CharacterRankComponent));
				if (!rankComp)
				{
					Print("[CGQC] AssignRank: No SCR_CharacterRankComponent found on entity");
					return;
				}

				SCR_ECharacterRank desiredRank = entry.m_iRank;
				rankComp.SetCharacterRank(desiredRank);
				PrintFormat("[CGQC] AssignRank: Set rank %1", desiredRank);
				GetGame().GetCallqueue().CallLater(CGQC_getBeret.SwapBeretIfTraining, 2000, false, playerEntity, desiredRank);

				return;
			}
		}

		Print("[CGQC_InitializePlayer] Player not in CGQC roster, no action taken.");
	}


		/*
		{"d7e9113c-f075-41c5-a72a-9ee5187dc723", SCR_ECharacterRank.MAJOR}, // Cloutier
		{"c3fcb72f-2f87-4937-8847-1fb62b39a40c", SCR_ECharacterRank.MAJOR}, // Darkangel898 / Tremblay

		{"6538de9f-32ce-41cc-bc6f-844a6a3b2ce8", SCR_ECharacterRank.CAPTAIN}, // InsanelyCisMale / Lafo

		{"e1c8cdf2-953a-4db0-be45-2cd8e79556f2", SCR_ECharacterRank.LIEUTENANT}, // HellRik / Valiquette
		{"120786ec-60cd-4a96-9e10-c846578745f2", SCR_ECharacterRank.LIEUTENANT}, // ElButteur / Genest
		{"e98c24ac-31b8-4647-94f8-c4ffa5bec5fe", SCR_ECharacterRank.LIEUTENANT}, // cluelessCanadian / Trépanier

		{"dce42da0-351f-4d03-821b-e89e46b70002", SCR_ECharacterRank.SERGEANT}, // ServerError / Turcotte
		{"fdeca5b5-5cba-403a-8e3a-d598325dbccc", SCR_ECharacterRank.SERGEANT}, // Dubaille / Dubé
		{"789954f6-cdb6-4f69-8095-af635b9a78a1", SCR_ECharacterRank.SERGEANT}, // Lauzon / Lauzon
		{"c6643f04-50cb-461a-b591-b6e6deb63ed9", SCR_ECharacterRank.SERGEANT}, // psykou / Fournier
		{"dd7969a6-722f-4ae6-a9e7-898ee9e6fea5", SCR_ECharacterRank.SERGEANT}, // Epicdudejo / Pike
		{"60e84a60-c12f-4b4a-91ed-c294bab6046e", SCR_ECharacterRank.SERGEANT}, // Pig'sPeels / Comeau

		{"c40d4907-67a4-4a53-b7c0-49ae4f3d3121", SCR_ECharacterRank.PRIVATE}, // poolerPol / JeuneComeau
		{"daa9fb24-0729-45bb-b41f-324d22ca632b", SCR_ECharacterRank.PRIVATE}, // Technical / Forgues
		{"f86ee5bc-39bd-4a59-98b7-9caf0ee061b6", SCR_ECharacterRank.PRIVATE}, // P-Hell / Audet
		{"72c38387-e5fd-46f4-806e-ce1e7c0ce16e", SCR_ECharacterRank.PRIVATE} // Kevin Walker the 7th / Walker
		*/
        /* ex:
		{"", SCR_ECharacterRank.PRIVATE}, // Discord / Actual

		Ranks possibles par défaut:
			PRIVATE,
			CORPORAL,
			SERGEANT,
			LIEUTENANT,
			CAPTAIN,
			MAJOR,
			COLONEL,
			GENERAL,
		*/

	// Set player identity (works on both client and server)
	static void SetPlayerIdentity(IEntity playerEntity, string headPath, string bodyPath)
	{
	    if (!playerEntity)
	        return;

	    SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(playerEntity);
	    if (!playerCharacter)
	        return;

	    CharacterIdentityComponent idComp = CharacterIdentityComponent.Cast(playerCharacter.FindComponent(CharacterIdentityComponent));
	    if (!idComp)
	        return;

	    Identity playerID = idComp.GetIdentity();
	    VisualIdentity newVisID = playerID.GetVisualIdentity();
	    newVisID.SetHead(headPath);
	    newVisID.SetBody(bodyPath);
	    playerID.SetVisualIdentity(newVisID);
	    idComp.CommitChanges();
	    idComp.SetIdentity(playerID);
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
			"BAN dans 3,2,1...", "What's cookin'?", "Wassup homie?", "Greetings and salutations!",
			"Comment qu'il va?"
		};

		// Picks random greetings
		string init_WelcomeTxt = string.Format("%1, %2", i_welcome_list.GetRandomElement(), name);
		string init_WelcomeMsg = i_message_list.GetRandomElement();
		PrintFormat("[CGQC_ActivateCGQCFeatures] txt:%1/msg:%2 is the random msg", init_WelcomeTxt, init_WelcomeMsg);
		// Show welcome
		SCR_HintManagerComponent.GetInstance().ShowCustomHint(init_WelcomeMsg, init_WelcomeTxt, 10.0);
	}

    // Admin features activation
    static void ActivateAdminFeatures(IEntity playerEntity, bool isInitialised, string target)
    {
		// Skip if already initialized
		PrintFormat("[CGQC_InitializePlayer] Initialised already? : %1", isInitialised);
        if (isInitialised)
        {
            Print("[CGQC_InitializePlayer] Already initialized, skipping");
            return;
        }
		PrintFormat("[CGQC_InitializePlayer] Init sanity check. Should be true: %1 - Proceeding with init", isInitialised);



		Print("[CGQC_InitializePlayer] Admin features running");
		switch (target)
        {
            case "clou":
            {
				// Cloutier time
				//CGQC_Scripts.CheckAndGiveItem(playerEntity, "{8FD0DBFBF0AB213B}Prefabs/Characters/HeadGear/Hat_FlatCap_01/S10MASK.et");
				//CGQC_Scripts.CheckAndGiveItem(playerEntity, "{E513AC48A65855AA}Prefabs/Items/Smokeables/Smokeable_Cigar.et");
				//CGQC_Scripts.CheckAndGiveItem(playerEntity, "{F723BDF891EFECAE}Prefabs/Items/Smokeables/Smokeable_Joint.et");
				//CGQC_Scripts.CheckAndGiveItem(playerEntity, "{C918F658F962C669}Prefabs/Characters/HeadGear/Helmet_M1_01/BeardV2Blonde.et");
		        //CGQC_Scripts.CheckAndGiveItem(playerEntity, "{33CBDE73AB48172A}Prefabs/Weapons/Explosives/DemoBlock_M112/DemoBlock_M112.et");
				//CGQC_Scripts.CheckAndGiveItem(playerEntity, "{EEEDC5D1AC2CE09F}Prefabs/Items/Equipment/Radios Base/CGQC_Radio_4_163a.et");
				break;
			}
			case "lafo":
            {
				// Lafo time
				CGQC_Scripts.CheckAndGiveItem(playerEntity, "{8FD0DBFBF0AB213B}Prefabs/Characters/HeadGear/Hat_FlatCap_01/S10MASK.et");
				break;
			}
			case "vali":
            {
				// Vali time
				//CGQC_Scripts.CheckAndGiveItem(playerEntity, "{C918F658F962C669}Prefabs/Characters/HeadGear/Helmet_M1_01/BeardV2Blonde.et");
				break;
			}
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

