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
			// Subalternes
			case SCR_ECharacterRank.CGQC_SOLDAT:
	            return "{4A1C1BF9AB830BA3}Prefabs/Characters/Berets/CGQC_1_yellow.et";
			case SCR_ECharacterRank.CGQC_CAPORAL:
	            return "{2ED3710551B83182}Prefabs/Characters/Berets/CGQC_2_black.et";	        
			case SCR_ECharacterRank.CGQC_CAPORAL_CHEF:
	            return "{FF6A3AE2B9F64309}Prefabs/Characters/Berets/CGQC_3_Green.et";			
	       // Sous officiers
	        case SCR_ECharacterRank.CGQC_SERGENT:
	            return "{B927727621FBF2CB}Prefabs/Characters/Berets/CGQC_4_Gray.et";
			case SCR_ECharacterRank.CGQC_ADJUDENT:
	            return "{FF6A3AE2B9F64309}Prefabs/Characters/Berets/CGQC_3_Green.et";
			case SCR_ECharacterRank.CGQC_ADJUDENT_MAITRE:
	            return "{FF6A3AE2B9F64309}Prefabs/Characters/Berets/CGQC_3_Green.et";
			case SCR_ECharacterRank.CGQC_ADJUDENT_CHEF:
	            return "{FF6A3AE2B9F64309}Prefabs/Characters/Berets/CGQC_3_Green.et";
			// Officiers
			case SCR_ECharacterRank.CGQC_SOUS_LIEUTENANT:
	            return "{F8C081F5D0BAF251}Prefabs/Characters/Berets/CGQC_5_Blue.et";
			case SCR_ECharacterRank.CGQC_LIEUTENANT:
	            return "{F8C081F5D0BAF251}Prefabs/Characters/Berets/CGQC_5_Blue.et";
	        case SCR_ECharacterRank.CGQC_CAPITAINE:
	            return "{F8C081F5D0BAF251}Prefabs/Characters/Berets/CGQC_5_Blue.et";        
			// État Major
	        case SCR_ECharacterRank.CGQC_MAJOR:
	            return "{4C0D703CEF585E02}Prefabs/Characters/Berets/CGQC_6_Red.et";
			default:
	        	return "{4A1C1BF9AB830BA3}Prefabs/Characters/Berets/CGQC_1_yellow.et";	
	    }
		return "{4A1C1BF9AB830BA3}Prefabs/Characters/Berets/CGQC_1_yellow.et";
	}

	//! Returns true if any beret from the CGQC set is found anywhere in the player's inventory
	static bool HasAnyBeret(SCR_InventoryStorageManagerComponent invManager)
	{
	    if (!invManager)
	        return false;

	    array<ResourceName> berets = {
			"{4A1C1BF9AB830BA3}Prefabs/Characters/Berets/CGQC_1_yellow.et",
			"{2ED3710551B83182}Prefabs/Characters/Berets/CGQC_2_black.et",	        
			"{FF6A3AE2B9F64309}Prefabs/Characters/Berets/CGQC_3_Green.et",			
			 "{B927727621FBF2CB}Prefabs/Characters/Berets/CGQC_4_Gray.et",
			"{F8C081F5D0BAF251}Prefabs/Characters/Berets/CGQC_5_Blue.et",     
			"{4C0D703CEF585E02}Prefabs/Characters/Berets/CGQC_6_Red.et"
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