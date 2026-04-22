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

	    // Check unit prefab name for "training"
	    EntityPrefabData unitPrefabData = playerEntity.GetPrefabData();
	    if (!unitPrefabData)
	    {
	        Print("[CGQC_getBeret] ERROR: No prefab data on playerEntity");
	        return;
	    }

	    string unitPrefabName = unitPrefabData.GetPrefabName();

	    if (!unitPrefabName.Contains("Training") && !unitPrefabName.Contains("training"))
	    {
	        return;
	    }


	    // Find the loadout storage on the character
	    EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(playerEntity.FindComponent(EquipedLoadoutStorageComponent));
	    if (!loadoutStorage)
	    {
	        return;
	    }
	    Print("[CGQC_getBeret] EquipedLoadoutStorageComponent found");

	    // Find whatever is currently in the head slot
	    IEntity currentHead = loadoutStorage.GetClothFromArea(LoadoutHeadCoverArea);
	    if (currentHead)
	    {
	        EntityPrefabData headPrefabData = currentHead.GetPrefabData();
	        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	        if (!invManager)
	        {
	            return;
	        }

	        bool removed = invManager.TryRemoveItemFromInventory(currentHead);
	        SCR_EntityHelper.DeleteEntityAndChildren(currentHead);
	    }

	    // Pick the beret for this rank
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
				beretPrefab = "{0365853D19E60AE6}Prefabs/Characters/HeadGear/Beret/CGQC_beret_1_yellow.et"; 
	            break;
	        }
	    }
	    CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
	}
}