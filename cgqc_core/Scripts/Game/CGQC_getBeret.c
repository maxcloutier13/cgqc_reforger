class CGQC_getBeret
{
	//! If the player's unit prefab name contains "training", remove whatever is in the
	//! head slot and give the rank-appropriate beret.
	static void SwapBeretIfTraining(IEntity playerEntity, SCR_ECharacterRank rank)
	{
	    if (!playerEntity)
	    {
	        Print("[CGQC_getBeret] ERROR: playerEntity is null");
	        return;
	    }

	    // Show the rank passed in so we can confirm it's correct
	    PrintFormat("[CGQC_getBeret] Rank passed in: %1", rank);
	    string hintMsg = string.Format("rank passé = %1", rank);
	    SCR_HintManagerComponent.GetInstance().ShowCustomHint(hintMsg, "DEBUG getBeret", 10.0);

	    // Check unit prefab name for "training"
	    EntityPrefabData unitPrefabData = playerEntity.GetPrefabData();
	    if (!unitPrefabData)
	    {
	        Print("[CGQC_getBeret] ERROR: No prefab data on playerEntity");
	        return;
	    }

	    string unitPrefabName = unitPrefabData.GetPrefabName();
	    PrintFormat("[CGQC_getBeret] Unit prefab name: %1", unitPrefabName);

	    if (!unitPrefabName.Contains("Training") && !unitPrefabName.Contains("training"))
	    {
	        PrintFormat("[CGQC_getBeret] 'training' not found in prefab name -> skipping. Full name: %1", unitPrefabName);
	        return;
	    }

	    Print("[CGQC_getBeret] Training prefab confirmed -> proceeding with head slot swap");

	    // Find the loadout storage on the character
	    EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(playerEntity.FindComponent(EquipedLoadoutStorageComponent));
	    if (!loadoutStorage)
	    {
	        Print("[CGQC_getBeret] ERROR: No EquipedLoadoutStorageComponent found");
	        return;
	    }
	    Print("[CGQC_getBeret] EquipedLoadoutStorageComponent found");

	    // Find whatever is currently in the head slot
	    IEntity currentHead = loadoutStorage.GetClothFromArea(LoadoutHeadCoverArea);
	    if (currentHead)
	    {
	        EntityPrefabData headPrefabData = currentHead.GetPrefabData();
	        if (headPrefabData)
	            PrintFormat("[CGQC_getBeret] Current head item: %1 -> removing", headPrefabData.GetPrefabName());
	        else
	            Print("[CGQC_getBeret] Current head item has no prefab data -> removing anyway");

	        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	        if (!invManager)
	        {
	            Print("[CGQC_getBeret] ERROR: No SCR_InventoryStorageManagerComponent found");
	            return;
	        }

	        bool removed = invManager.TryRemoveItemFromInventory(currentHead);
	        PrintFormat("[CGQC_getBeret] TryRemoveItemFromInventory result: %1", removed);
	        SCR_EntityHelper.DeleteEntityAndChildren(currentHead);
	        Print("[CGQC_getBeret] Head item deleted");
	    }
	    else
	    {
	        Print("[CGQC_getBeret] Head slot is empty, skipping removal");
	    }

	    // Pick the beret for this rank
	    // !! Fill in GUIDs from Workbench for each beret prefab !!
	    ResourceName beretPrefab = "";
	    switch (rank)
	    {
	        case SCR_ECharacterRank.MAJOR:
	        {
	            beretPrefab = "{3BF0961398A4EB40}Prefabs/Characters/HeadGear/Beret/CGQC_beret_6_red.et";        // Red
	            break;
	        }
	        case SCR_ECharacterRank.CAPTAIN:
	        {
	            beretPrefab = "{6BD04A140B05B38E}Prefabs/Characters/HeadGear/Beret/CGQC_beret_5_blue.et";       // Blue
	            break;
	        }
	        case SCR_ECharacterRank.LIEUTENANT:
	        {
	            beretPrefab = "{CB0DEB239416979A}Prefabs/Characters/HeadGear/Beret/CGQC_beret_3_green.et";      // Green
	            break;
	        }
	        case SCR_ECharacterRank.SERGEANT:
	        {
	            beretPrefab = "{33E55CE26ECCDC7F}Prefabs/Characters/HeadGear/Beret/CGQC_beret_2_black.et";      // Black
	            break;
	        }
	        case SCR_ECharacterRank.PRIVATE:
	        {
	            beretPrefab = "{0365853D19E60AE6}Prefabs/Characters/HeadGear/Beret/CGQC_beret_1_yellow.et";     // Yellow
	            break;
	        }
	        default:
	        {
	            PrintFormat("[CGQC_getBeret] No beret defined for rank %1 -> skipping", rank);
	            return;
	        }
	    }

	    PrintFormat("[CGQC_getBeret] Giving beret: %1", beretPrefab);
	    CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
	    Print("[CGQC_getBeret] Done <-");
	}
}