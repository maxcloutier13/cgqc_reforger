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
        // Get player's id
        int playerId = GetPlayerId();
		// Bohemia ID
		
        string playerIdentityId = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerEntity);
        // Get player's name
        string cgqc_playerName = GetPlayerName();
        PrintFormat("[CGQC_InitializePlayer] playerId: %1s - playerIdentityId: %2s %3 ", playerId, playerIdentityId, cgqc_playerName);
		// CGQC flag
		bool isCGQC = false;

        // Check Steam ID and activate specific features for certain players
        switch (playerIdentityId)
        {
            case "d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier local
            {
                Print("[CGQC_InitializePlayer] CGQC Cloutier LOCAL detected - activating admin features");
				GetGame().GetCallqueue().CallLater(ActivateAdminFeatures, 5000, false, playerEntity);
				isCGQC = true;
                break;
            }
			case "d7e9113c-f075-41c5-a72a-9ee5187dc723s":
            {
                Print("[CGQC_InitializePlayer] CGQC Cloutier detected - activating admin features");
				GetGame().GetCallqueue().CallLater(ActivateAdminFeatures, 5000, false, playerEntity);
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
            default:
            {
                PrintFormat("[CGQC_InitializePlayer] Unrecognized player %1 (Steam ID: %2) has spawned", cgqc_playerName, playerIdentityId);
				// Display welcome message
		        string message = string.Format("Salut, %1! Bienvenue sur le serveur CGQC. Rejoins-nous sur discord: cgqc.ca - Have fun!", cgqc_playerName);
		        SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Salut!", 10.0);
                break;
            }
        }
		if(isCGQC)
		{
			GetGame().GetCallqueue().CallLater(ActivateCGQCFeatures, 5000, false, playerEntity, cgqc_playerName);
		}
        Print("[CGQC_InitializePlayer] Done <-");
    }

    // Helper method to get player name
    string GetPlayerName()
    {
        int playerId = GetPlayerId();
        return GetGame().GetPlayerManager().GetPlayerName(playerId);
    }
	
	void ActivateCGQCFeatures(IEntity playerEntity, string name)
    {
		Print("[CGQC_ActivateCGQCFeatures] Setup up for CGQC player");
		// We have a group member
		array<string> _welcome_list = {
		    "What's good", "Howdy", "Hiya", "Wassup", "Yo", "R’gard", "Allo", "Hello", "Ooooh",
			"Coucou", "Bonsoir", "Konnichiwa", "Hola", "Hallo", "Nǐ hǎo", "Hoi", "Merhaba", "Vitayu"
		};
		array<string> _message_list = {
		   "what's up?", "what's up buddy?", "Time to fuck shit up",
			"Asti que t'es beau", "Ça roule ma poule?", "As-tu couché ta blonde?",
			"BAN dans 3,2,1...", "What's cookin'?", "Wassup homie?", "Greetings and salutations!"
		};
		
		// Picks random greetings
		string init_WelcomeTxt = string.Format("%1, %2", _welcome_list.GetRandomElement(), name);
		string init_WelcomeMsg = _message_list.GetRandomElement();
		
		// Show welcome
		SCR_HintManagerComponent.GetInstance().ShowCustomHint(init_WelcomeMsg, init_WelcomeTxt, 10.0);
	}

    // Admin features activation
    void ActivateAdminFeatures(IEntity playerEntity)
    {
		Print("[CGQC_InitializePlayer] Admin features running");
		// Cloutier time
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{F723BDF891EFECAE}Prefabs/Items/Smokeables/Smokeable_Joint.et");
		CGQC_Scripts.CheckAndGiveItem(playerEntity, "{E513AC48A65855AA}Prefabs/Items/Smokeables/Smokeable_Cigar.et");
        CGQC_Scripts.CheckAndGiveItem(playerEntity, "{33CBDE73AB48172A}Prefabs/Weapons/Explosives/DemoBlock_M112/DemoBlock_M112.et");
        
		/* Tried to add GM but it auto opens it? Fuuuck that.
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && !editorManager.IsOpened())
		{
		    editorManager.Open();
		}*/
    }
}





/*
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