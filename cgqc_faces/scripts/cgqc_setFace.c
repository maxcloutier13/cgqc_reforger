/* modded class SCR_PlayerController : PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);
        
        if (to)
        {
            GetGame().GetCallqueue().CallLater(ForceHeadOnEntity, 100, false, to);
        }
    }
    
    void ForceHeadOnEntity(IEntity entity)
    {
        if (!entity)
            return;
        
        SCR_CharacterIdentityComponent identityComp = SCR_CharacterIdentityComponent.Cast(entity.FindComponent(SCR_CharacterIdentityComponent));
        if (!identityComp)
            return;
        
        // Get SteamID from player controller
        string steamId = GetGame().GetBackendApi().GetPlayerIdentityId(GetPlayerId());
        
        // Map SteamID to head resource path
        ResourceName headPath = GetHeadPathForSteamId(steamId);
        
        // Only set head if player is in the list
        if (headPath != "")
        {
            // Get current body to preserve it
            ResourceName currentBody = identityComp.GetBodyType();
            identityComp.SetIdentity(headPath, currentBody);
        }
    }
    
    ResourceName GetHeadPathForSteamId(string steamId)
    {
        // Add your SteamID-to-head path mappings here
        // Use the full path to the head prefab
        switch (steamId)
        {
            case "76561198024730191": 
                return "{65FB5234B4A8E20B}\Prefabs\Characters\Heads\Head_Livonian_Head_9.et"; // Replace with actual path
            case "76561198087654321": 
                return "{ANOTHER_PATH}Head_US_01.et";
        }
        
        // Return empty string if not in list (skip head setting)
        return "";
    }
}

*/