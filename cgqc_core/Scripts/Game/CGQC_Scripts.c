class CGQC_Scripts
{
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
	    
	    // Get the character's inventory storage manager
	    SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	    if (!invManager)
	        return;
	    
	    // Check if player already has this item
	    if (HasItemInInventory(invManager, itemPrefab))
		{
			PrintFormat("[CGQC_CheckAndGiveItem] Player already has item: %1", itemPrefab);

	        return; // Already has it, don't give
		}
	    // Player doesn't have it, give it
		PrintFormat("[CGQC_CheckAndGiveItem] Player doesn't have item: %1 -> Giving", itemPrefab);
	    GiveItemToPlayer(playerEntity, itemPrefab);
	}
	
	static bool HasItemInInventory(SCR_InventoryStorageManagerComponent invManager, ResourceName itemPrefab)
	{
	    if (!invManager)
	        return false;
	    
	    array<IEntity> items = {};
	    invManager.GetAllItems(items, null);
	    
	    foreach (IEntity item : items)
	    {
	        if (!item)
	            continue;
	            
	        EntityPrefabData prefabData = item.GetPrefabData();
	        if (!prefabData)
	            continue;
	            
	        // Compare prefab paths
	        if (prefabData.GetPrefabName() == itemPrefab)
	            return true;
	    }
	    
	    return false;
	}
}