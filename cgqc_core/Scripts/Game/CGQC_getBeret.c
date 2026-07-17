class CGQC_getBeret
{
	//! If the player's unit prefab name contains both "CGQC" and "Base":
	//! - Head slot empty    -> equip ranked beret directly
	//! - Head slot occupied -> check inventory for any beret, add to inventory if none found
	//! If the player's unit prefab name contains only "CGQC" (no "Base"):
	//! - Insert beret into pants storage (falls back to general inventory if no pants/storage)
	static void getBeret(IEntity playerEntity, SCR_ECharacterRank rank)
	{
	    if (!playerEntity)
	        return;

	    EntityPrefabData unitPrefabData = playerEntity.GetPrefabData();
	    if (!unitPrefabData)
	        return;

	    string unitPrefabName = unitPrefabData.GetPrefabName();

	    //bool hasCGQC = unitPrefabName.Contains("CGQC") || unitPrefabName.Contains("cgqc");
	    //bool hasBase = unitPrefabName.Contains("Base") || unitPrefabName.Contains("base");

	    
	    ResourceName beretPrefab = CGQC_getBeret.GetBeretForRank(rank);
	    if (beretPrefab.IsEmpty())
	        return;

	    EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(playerEntity.FindComponent(EquipedLoadoutStorageComponent));
	    if (!loadoutStorage)
	        return;

	    SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	    if (!invManager)
	        return;

	   // Check if something on head
       IEntity currentHead = loadoutStorage.GetClothFromArea(LoadoutHeadCoverArea);
       if (!currentHead)
       {
		// Nothing on head, putting beret on
           CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
       }
       else
       {
		// Got something on head
           if (!CGQC_getBeret.HasAnyBeret(invManager))
           {
			// No beret in pockets: Adding it
			CGQC_getBeret.GiveBeretToPants(playerEntity, invManager, loadoutStorage, beretPrefab);
               //CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
           }
       }
	  
	}

	//! Spawns the beret and inserts it into the pants item's storage.
	//! Falls back to general inventory if pants are missing or have no storage.
	static void GiveBeretToPants(IEntity playerEntity, SCR_InventoryStorageManagerComponent invManager, EquipedLoadoutStorageComponent loadoutStorage, ResourceName beretPrefab)
	{
	    // LoadoutPantsArea confirmed in official Arma Reforger Script API
	    IEntity pants = loadoutStorage.GetClothFromArea(LoadoutPantsArea);
	    if (!pants)
	    {
	        CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
	        return;
	    }

	    BaseInventoryStorageComponent pantsStorage = BaseInventoryStorageComponent.Cast(pants.FindComponent(BaseInventoryStorageComponent));
	    if (!pantsStorage)
	    {
	        CGQC_Scripts.CheckAndGiveItem(playerEntity, beretPrefab);
	        return;
	    }

	    Resource beretResource = Resource.Load(beretPrefab);
	    if (!beretResource || !beretResource.IsValid())
	        return;

	    EntitySpawnParams spawnParams = new EntitySpawnParams();
	    spawnParams.TransformMode = ETransformMode.WORLD;
	    playerEntity.GetTransform(spawnParams.Transform);

	    IEntity beret = GetGame().SpawnEntityPrefab(beretResource, GetGame().GetWorld(), spawnParams);
	    if (!beret)
	        return;

	    if (!invManager.TryInsertItemInStorage(beret, pantsStorage))
	    {
	        invManager.TryInsertItem(beret, EStoragePurpose.PURPOSE_ANY);
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
	    return "{4A1C1BF9AB830BA3}Prefabs/Characters/Berets/CGQC_1_yellow.et"; // required by EnforceScript
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
	            return true;
	    }

	    return false;
	}
}