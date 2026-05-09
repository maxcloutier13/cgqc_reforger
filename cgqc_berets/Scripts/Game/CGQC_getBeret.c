class CGQC_getBeret
{
	//! If the player's unit prefab name contains "Training":
	//! - Head slot empty   -> equip ranked beret directly
	//! - Head slot occupied -> check inventory for any beret, add to inventory if none found
	static void SwapBeretIfTraining(IEntity playerEntity, SCR_ECharacterRank rank)
	{
	    if (!playerEntity)
	    {
	        return;
	    }

	    // Check unit prefab name for "Training" / "training"
	    EntityPrefabData unitPrefabData = playerEntity.GetPrefabData();
	    if (!unitPrefabData)
	    {
	        return;
	    }

	    string unitPrefabName = unitPrefabData.GetPrefabName();

	    if (!unitPrefabName.Contains("Training") && !unitPrefabName.Contains("training") && !unitPrefabName.Contains("Moderne"))
	    {
	        return;
	    }


	    // Resolve beret prefab for this rank
	    ResourceName beretPrefab = CGQC_getBeret.GetBeretForRank(rank);
	    if (beretPrefab.IsEmpty())
	    {
	        return;
	    }

	    // Find loadout storage
	    EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(playerEntity.FindComponent(EquipedLoadoutStorageComponent));
	    if (!loadoutStorage)
	    {
	        return;
	    }

	    SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	    if (!invManager)
	    {
	        return;
	    }

	    IEntity currentHead = loadoutStorage.GetClothFromArea(LoadoutHeadCoverArea);

	    if (!currentHead)
	    {
	        // Head slot is empty -> equip beret directly
	        CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
	    }
	    else
	    {
	        // Head slot occupied -> check if player already has any beret in inventory
	        if (!CGQC_getBeret.HasAnyBeret(invManager))
	        {
	            CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
	        }
	    }
	}

	//! Returns the ranked beret prefab ResourceName
	static ResourceName GetBeretForRank(SCR_ECharacterRank rank)
	{
	    switch (rank)
	    {
	        case SCR_ECharacterRank.MAJOR:
	            return "{3BF0961398A4EB40}Prefabs/Characters/HeadGear/Beret/CGQC_beret_6_red.et";
	        case SCR_ECharacterRank.CAPTAIN:
	            return "{6BD04A140B05B38E}Prefabs/Characters/HeadGear/Beret/CGQC_beret_5_blue.et";
	        case SCR_ECharacterRank.LIEUTENANT:
	            return "{CB0DEB239416979A}Prefabs/Characters/HeadGear/Beret/CGQC_beret_3_green.et";
	        case SCR_ECharacterRank.SERGEANT:
	            return "{33E55CE26ECCDC7F}Prefabs/Characters/HeadGear/Beret/CGQC_beret_2_black.et";
	        case SCR_ECharacterRank.PRIVATE:
	            return "{0365853D19E60AE6}Prefabs/Characters/HeadGear/Beret/CGQC_beret_1_yellow.et";
	        default:
	            return "{0365853D19E60AE6}Prefabs/Characters/HeadGear/Beret/CGQC_beret_1_yellow.et";
	    }
	    return "{0365853D19E60AE6}Prefabs/Characters/HeadGear/Beret/CGQC_beret_1_yellow.et";
	}

	//! Returns true if any beret from the CGQC set is found anywhere in the player's inventory
	static bool HasAnyBeret(SCR_InventoryStorageManagerComponent invManager)
	{
	    if (!invManager)
	        return false;

	    array<ResourceName> berets = {
	        "{3BF0961398A4EB40}Prefabs/Characters/HeadGear/Beret/CGQC_beret_6_red.et",
	        "{6BD04A140B05B38E}Prefabs/Characters/HeadGear/Beret/CGQC_beret_5_blue.et",
	        "{CB0DEB239416979A}Prefabs/Characters/HeadGear/Beret/CGQC_beret_3_green.et",
	        "{33E55CE26ECCDC7F}Prefabs/Characters/HeadGear/Beret/CGQC_beret_2_black.et",
	        "{0365853D19E60AE6}Prefabs/Characters/HeadGear/Beret/CGQC_beret_1_yellow.et"
	    };

	    array<IEntity> items = {};
	    invManager.GetAllItems(items, null);

	    foreach (IEntity item : items)
	    {
	        if (!item)
	            continue;

	        EntityPrefabData prefabData = item.GetPrefabData();
	        if (!prefabData)
	            continue;

	        if (berets.Contains(prefabData.GetPrefabName()))
	        {
	            return true;
	        }
	    }

	    return false;
	}
}