[ComponentEditorProps(category: "CGQC", description: "Renames backpack to last owner's name")]
class CGQC_BackpackNametagClass : ScriptComponentClass {}

class CGQC_BackpackNametag : ScriptComponent
{
    // Store the last known owner name so we don't lose it
    string m_sLastOwnerName;
	string m_sOriginalName;

    override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Store the original name once on init, before any renaming
		InventoryItemComponent invItem = InventoryItemComponent.Cast(
			owner.FindComponent(InventoryItemComponent)
		);
		if (invItem)
		{
			UIInfo uiInfo = invItem.GetUIInfo();
			if (uiInfo)
				m_sOriginalName = uiInfo.GetName();
		}

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
		
		// Only fire when item is dropped to the world (newSlot = null)
		if (newSlot)
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

        string playerName = GetCustomPlayerName(playerID);
        if (playerName.IsEmpty())
            return;

        m_sLastOwnerName = playerName;
		
		/* Bad. Causes issue

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
        */
    }
	
	// Returns the tagged display name for use in the action
	string GetTaggedName()
	{
		if (m_sLastOwnerName.IsEmpty())
			return m_sOriginalName;

		return m_sLastOwnerName + "'s " + m_sOriginalName;
	}
}


// Custom action with name included
[ComponentEditorProps(category: "CGQC", description: "Shows owner name as action label")]
class CGQC_BackpackNameAction : ScriptedUserAction
{
	override bool GetActionNameScript(out string outName)
	{
		CGQC_BackpackNametag nametag = CGQC_BackpackNametag.Cast(
			GetOwner().FindComponent(CGQC_BackpackNametag)
		);
		if (!nametag || nametag.m_sLastOwnerName.IsEmpty())
			return false;

		outName = nametag.GetTaggedName();
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		CGQC_BackpackNametag nametag = CGQC_BackpackNametag.Cast(
			GetOwner().FindComponent(CGQC_BackpackNametag)
		);
		return nametag && !nametag.m_sLastOwnerName.IsEmpty();
	}

	override bool CanBePerformedScript(IEntity user) { return true; }
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity) {}
}

//----------------------------------------------------------------------
// override to inject tagged name
modded class SCR_InventoryMenuUI
{
    override protected void SetFocusedSlotEffects()
    {
        if (!m_pFocusedSlotUI)
            return;

        InventoryItemComponent invItemComp = m_pFocusedSlotUI.GetInventoryItemComponent();
        if (!invItemComp)
        {
            super.SetFocusedSlotEffects();
            return;
        }

        CGQC_BackpackNametag nametag = CGQC_BackpackNametag.Cast(
            invItemComp.GetOwner().FindComponent(CGQC_BackpackNametag)
        );

        if (!nametag || nametag.m_sLastOwnerName.IsEmpty())
        {
            super.SetFocusedSlotEffects();
            return;
        }

        // Has a nametag — build custom tooltip
        SCR_ItemAttributeCollection attribs = SCR_ItemAttributeCollection.Cast(invItemComp.GetAttributes());
        if (!attribs)
        {
            super.SetFocusedSlotEffects();
            return;
        }

        UIInfo itemInfo = attribs.GetUIInfo();
        if (!itemInfo)
        {
            super.SetFocusedSlotEffects();
            return;
        }

        SCR_InventoryUIInfo inventoryInfo = SCR_InventoryUIInfo.Cast(itemInfo);
        string desc;
		if (inventoryInfo)
		    desc = inventoryInfo.GetInventoryItemDescription(invItemComp);
		else
		    desc = itemInfo.GetDescription();

        ShowItemInfo(nametag.GetTaggedName(), desc, invItemComp.GetTotalWeight(), inventoryInfo);
        NavigationBarUpdate();
    }
}


// Highlight the player's items
class CGQC_NametagHighlightHelper
{
    static void ApplyHighlight(InventoryItemComponent pItem, SCR_InventoryStorageBaseUI pStorageUI, Widget widget)
    {
        if (!pStorageUI)
            return;

        if (pStorageUI.IsInherited(SCR_InventoryStoragesListUI))
            return;

        if (!pItem)
            return;

        IEntity item = pItem.GetOwner();
        if (!item)
            return;

        CGQC_BackpackNametag nametag = CGQC_BackpackNametag.Cast(
            item.FindComponent(CGQC_BackpackNametag));

        if (!nametag || nametag.m_sLastOwnerName.IsEmpty())
            return;

        PlayerController pc = GetGame().GetPlayerController();
        if (!pc)
            return;

        string localName = nametag.GetCustomPlayerName(pc.GetPlayerId());
        if (nametag.m_sLastOwnerName != localName)
            return;

        if (!widget)
            return;

        ImageWidget img = ImageWidget.Cast(widget.FindAnyWidget("BackgroundColor"));
        if (img)
            img.SetColor(new Color(12/255.0, 96/255.0, 255/255.0, 0.25));
    }
}

modded class SCR_InventorySlotUI
{
     override protected void Init()
    {
        super.Init();
        CGQC_NametagHighlightHelper.ApplyHighlight(m_pItem, m_pStorageUI, m_widget);
    }
}
