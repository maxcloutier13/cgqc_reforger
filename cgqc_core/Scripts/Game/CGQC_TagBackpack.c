[ComponentEditorProps(category: "CGQC", description: "Renames backpack to last owner's name")]
class CGQC_BackpackNametagClass : ScriptComponentClass {}

class CGQC_BackpackNametag : ScriptComponent
{
    // Store the last known owner name so we don't lose it
    string m_sLastOwnerName;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        InventoryItemComponent invItem = InventoryItemComponent.Cast(
            owner.FindComponent(InventoryItemComponent)
        );

        if (!invItem)
            return;

        invItem.m_OnParentSlotChangedInvoker.Insert(OnParentSlotChanged);
    }
	
	// Get name from mod CustomName
	string GetCustomPlayerName(int playerID)
	{
	    // Try CustomNamesManager first
	    CustomNamesManager cnm = CustomNamesManager.GetInstance();
	    if (cnm)
	    {
	        string customName = cnm.GetCustomName(playerID);
	        if (!customName.IsEmpty())
	            return customName;
	    }
	    
	    // Fallback to Steam name
	    return GetGame().GetPlayerManager().GetPlayerName(playerID);
	}

    protected void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
    {
		
		// Skip if already named
		if (!m_sLastOwnerName.IsEmpty())
        	return;
		 
        // We care about who HAD it — that's the dropper
        if (!oldSlot)
            return;

        // Walk up from the old slot's storage to find the root entity
        BaseInventoryStorageComponent oldStorage = oldSlot.GetStorage();
        if (!oldStorage)
            return;

        IEntity storageOwner = oldStorage.GetOwner();
        if (!storageOwner)
            return;

        // Get the root parent — backpacks are often slotted inside sub-storages
        IEntity rootOwner = storageOwner.GetRootParent();
        if (!rootOwner)
            rootOwner = storageOwner;

        // Check if this root entity is a player-controlled character
        PlayerManager pm = GetGame().GetPlayerManager();
        if (!pm)
            return;

        int playerID = pm.GetPlayerIdFromControlledEntity(rootOwner);
        if (playerID == 0)
            return; // Not a player (could be AI, vehicle, etc.)

        //string playerName = pm.GetPlayerName(playerID);
		string playerName = GetCustomPlayerName(playerID);
        if (playerName.IsEmpty())
            return;

        m_sLastOwnerName = playerName;

        // Now rename the bag
        InventoryItemComponent invItem = InventoryItemComponent.Cast(
            GetOwner().FindComponent(InventoryItemComponent)
        );
        if (!invItem)
            return;

        UIInfo uiInfo = invItem.GetUIInfo();
        if (!uiInfo)
            return;
		
		string m_oldName = uiInfo.GetName();
        uiInfo.SetName(m_sLastOwnerName + "'s " + m_oldName);
        
    }
}

[ComponentEditorProps(category: "CGQC", description: "Shows owner name as action label")]

class CGQC_BackpackNameAction : ScriptedUserAction
{
    override bool GetActionNameScript(out string outName)
    {
         InventoryItemComponent invItem = InventoryItemComponent.Cast(
        GetOwner().FindComponent(InventoryItemComponent)
   		 );
	    if (!invItem)
	        return false;
	
	    UIInfo uiInfo = invItem.GetUIInfo();
	    if (!uiInfo)
	        return false;
	
	    outName = uiInfo.GetName();
	    return true;
    }

    override bool CanBeShownScript(IEntity user)
    {
        CGQC_BackpackNametag nametag = CGQC_BackpackNametag.Cast(
            GetOwner().FindComponent(CGQC_BackpackNametag)
        );
        // Only show the action if the bag has been named
        return nametag && !nametag.m_sLastOwnerName.IsEmpty();
    }

    override bool CanBePerformedScript(IEntity user) { return true; }
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) {}
}