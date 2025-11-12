modded class SCR_PlayerController : PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        // Check if we're taking control of a new entity (player spawn)
        if (to)
        {
            // Wait 5 seconds before running initialization code
            GetGame().GetCallqueue().CallLater(InitializePlayer, 5000, false, to);
        }
    }

    // Initialize player after delay
    void InitializePlayer(IEntity playerEntity)
    {
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
                Print("[Special] Cloutier detected - activating admin features");
                ActivateAdminFeatures(playerEntity);
				string message = string.Format("Cloutier sti!");
		        SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Master", 10.0);
                break;
            }
            default:
            {
                Print(string.Format("[Welcome] Regular player %1 (Steam ID: %2) has spawned", playerName, steamId));
				// Display welcome message
		        string message = string.Format("Salut, %1!", playerName);
		        SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Bienvenue", 10.0);
                break;
            }
        }
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

        SCR_HintManagerComponent.GetInstance().ShowCustomHint("Admin mode activated", "System", 3.0);
    }
}

/*
"d7e9113c-f075-41c5-a72a-9ee5187dc723": // Cloutier
*/