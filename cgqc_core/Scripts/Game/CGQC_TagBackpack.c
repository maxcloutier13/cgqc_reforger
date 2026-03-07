[ComponentEditorProps(category: "CGQC", description: "Renames backpack to last owner's name")]
class CGQC_BackpackNametagClass : ScriptComponentClass {}

class CGQC_BackpackNametag : ScriptComponent
{
    // Store the last known owner name so we don't lose it
    protected string m_sLastOwnerName;

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

    protected void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
    {
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

        string playerName = pm.GetPlayerName(playerID);
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

        uiInfo.SetName(m_sLastOwnerName + "'s Bag");
        // ⚠️ Remember: SetName is LOCAL only — needs RPC broadcast for MP
    }
}