modded class GunBuilderUI_SlotsUIComponent
{
    protected ref set<string> m_SavedAreas = new set<string>();
    
    override void OnClicked(GunBuilderUI_MultifunctionSlotUIComponent comp, int button)
    {
        Bacon_GunBuilderUI_SlotChoice choice = Bacon_GunBuilderUI_SlotChoice.Cast(comp.GetData());
        if (choice && choice.slotType == Bacon_GunBuilderUI_SlotType.CHARACTER_LOADOUT && !choice.prefab.IsEmpty())
        {
            Bacon_GunBuilderUI ui = Bacon_GunBuilderUI.GetInstance();
            if (ui && ui.m_CharacterEntity)
            {
                CGQC_ClothingStash.SaveSlotIfNew(ui.m_CharacterEntity, choice.prefab, m_SavedAreas);
                
                if (m_SavedAreas.Count() == 1)
                    CGQC_ClothingStash.StartWatchingUI(ui);
            }
        }
        
        super.OnClicked(comp, button);
    }
}

class CGQC_ClothingStash
{
    static ref map<string, ref array<ResourceName>> s_SavedContents = new map<string, ref array<ResourceName>>();
    static IEntity s_Character;
    static Bacon_GunBuilderUI s_UI;

    static void StartWatchingUI(Bacon_GunBuilderUI ui)
    {
        s_UI = ui;
        GetGame().GetCallqueue().CallLater(CheckUIAlive, 100, true);
    }

    static void CheckUIAlive()
    {
        if (!s_UI || !Bacon_GunBuilderUI.GetInstance())
        {
            RestoreIfNeeded();
            GetGame().GetCallqueue().Remove(CheckUIAlive);
        }
    }

    static void RestoreIfNeeded()
    {
        if (s_Character)
            Restore(s_Character);
        
        s_Character = null;
        s_UI = null;
        s_SavedContents.Clear();
    }

    static void SaveSlotIfNew(IEntity character, ResourceName newPrefab, set<string> savedAreas)
    {
        s_Character = character;
        
        // For remove ("empty"), we need to find the area type from the currently worn item
        // We do this by checking which slot is focused — but since we can't access that,
        // we save all slots that haven't been saved yet
        if (newPrefab == "empty")
        {
			Print("CGQC_Clothing_Switcher | Remove detected, saving all unsaved slots", LogLevel.WARNING);
			
            SCR_CharacterInventoryStorageComponent charStorage = SCR_CharacterInventoryStorageComponent.Cast(
                character.FindComponent(SCR_CharacterInventoryStorageComponent));
            if (!charStorage) return;
            
            for (int i = 0; i < charStorage.GetSlotsCount(); i++)
            {
                LoadoutSlotInfo slot = LoadoutSlotInfo.Cast(charStorage.GetSlot(i));
                if (!slot) continue;
                
                LoadoutAreaType areaType = slot.GetAreaType();
                if (!areaType) continue;
                
                string areaKey = areaType.Type().ToString();
                if (savedAreas.Contains(areaKey)) continue;
                
                IEntity clothItem = slot.GetAttachedEntity();
                if (!clothItem) continue;
                
                SaveClothItemContents(clothItem, areaKey, savedAreas);
            }
            return;
        }
        
        IEntity tempEnt = GetGame().SpawnEntityPrefabLocal(
            Resource.Load(newPrefab), GetGame().GetWorld(), null);
        if (!tempEnt) return;
        
        BaseLoadoutClothComponent cloth = BaseLoadoutClothComponent.Cast(
            tempEnt.FindComponent(BaseLoadoutClothComponent));
        
        if (!cloth || !cloth.GetAreaType())
        {
            SCR_EntityHelper.DeleteEntityAndChildren(tempEnt);
            return;
        }
        
        typename targetArea = cloth.GetAreaType().Type();
        string areaKey = targetArea.ToString();
        SCR_EntityHelper.DeleteEntityAndChildren(tempEnt);
        
        if (savedAreas.Contains(areaKey))
            return;
        
        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout) return;
        
        IEntity wornItem = loadout.GetClothFromArea(targetArea);
        if (!wornItem) return;
        
        SaveClothItemContents(wornItem, areaKey, savedAreas);
    }
    
    static void SaveClothItemContents(IEntity clothItem, string areaKey, set<string> savedAreas)
    {
        BaseInventoryStorageComponent wornStorage = BaseInventoryStorageComponent.Cast(
            clothItem.FindComponent(BaseInventoryStorageComponent));
        if (!wornStorage) return;
        
        array<IEntity> items = {};
        wornStorage.GetAll(items);
        
        savedAreas.Insert(areaKey);
        
        if (items.IsEmpty()) return;
        
        array<ResourceName> prefabs = {};
        foreach (IEntity item : items)
        {
            BaseInventoryStorageComponent itemStorage = BaseInventoryStorageComponent.Cast(
                item.FindComponent(BaseInventoryStorageComponent));
            
            if (itemStorage)
            {
                array<IEntity> subItems = {};
                itemStorage.GetAll(subItems);
                foreach (IEntity subItem : subItems)
                {
                    ResourceName subPrefab;
                    if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(subItem, subPrefab))
                        prefabs.Insert(subPrefab);
                }
                continue;
            }
            
            ResourceName prefab;
            if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(item, prefab))
                prefabs.Insert(prefab);
        }
        
        if (!prefabs.IsEmpty())
            s_SavedContents.Set(areaKey, prefabs);
        
        Print("CGQC_Clothing_Switcher | Saved " + prefabs.Count() + " items from slot " + areaKey, LogLevel.WARNING);
    }
    
    static void Restore(IEntity character)
	{
	    if (s_SavedContents.IsEmpty()) return;
	    
	    SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(
	        character.FindComponent(SCR_InventoryStorageManagerComponent));
	    if (!invManager) return;
	    
	    EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
	        character.FindComponent(EquipedLoadoutStorageComponent));
	    if (!loadout) return;
	    
	    SCR_CharacterInventoryStorageComponent charStorage = SCR_CharacterInventoryStorageComponent.Cast(
	        character.FindComponent(SCR_CharacterInventoryStorageComponent));
	    
	    int droppedCount = 0;
	    int movedCount = 0;
	    
		Print("CGQC_Clothing_Switcher | Restoring " + s_SavedContents.Count() + " slots", LogLevel.WARNING);
	    foreach (string areaKey, array<ResourceName> prefabs : s_SavedContents)
	    {
			Print("CGQC_Clothing_Switcher | Slot " + areaKey + " has " + prefabs.Count() + " items", LogLevel.WARNING);
	        typename areaType = areaKey.ToType();
	        IEntity clothItem = loadout.GetClothFromArea(areaType);
	        
	        BaseInventoryStorageComponent clothStorage = null;
	        if (clothItem)
	            clothStorage = BaseInventoryStorageComponent.Cast(
	                clothItem.FindComponent(BaseInventoryStorageComponent));
	        
	        foreach (ResourceName prefab : prefabs)
	        {
	            if (clothStorage && invManager.TrySpawnPrefabToStorage(prefab, clothStorage))
	            {
	                movedCount++;
	                continue;
	            }
	            
	            bool inserted = false;
	            if (clothStorage)
	            {
	                array<BaseInventoryStorageComponent> subStorages = {};
	                clothStorage.GetOwnedStorages(subStorages, 1, false);
	                foreach (BaseInventoryStorageComponent subStorage : subStorages)
	                {
	                    if (invManager.TrySpawnPrefabToStorage(prefab, subStorage))
	                    {
	                        inserted = true;
	                        break;
	                    }
	                }
	            }
	            if (inserted)
	            {
	                movedCount++;
	                continue;
	            }
	            
	            if (invManager.TrySpawnPrefabToStorage(prefab))
				{
				    movedCount++;
				    continue;
				}
	            
	            if (charStorage)
	            {
	                for (int i = 0; i < charStorage.GetSlotsCount(); i++)
	                {
	                    LoadoutSlotInfo slot = LoadoutSlotInfo.Cast(charStorage.GetSlot(i));
	                    if (!slot) continue;
	                    
	                    IEntity otherCloth = slot.GetAttachedEntity();
	                    if (!otherCloth) continue;
	                    
	                    BaseInventoryStorageComponent otherStorage = BaseInventoryStorageComponent.Cast(
	                        otherCloth.FindComponent(BaseInventoryStorageComponent));
	                    if (!otherStorage) continue;
	                    
	                    if (invManager.TrySpawnPrefabToStorage(prefab, otherStorage))
	                    {
	                        inserted = true;
	                        break;
	                    }
	                    
	                    array<BaseInventoryStorageComponent> otherSubStorages = {};
	                    otherStorage.GetOwnedStorages(otherSubStorages, 1, false);
	                    foreach (BaseInventoryStorageComponent otherSubStorage : otherSubStorages)
	                    {
	                        if (invManager.TrySpawnPrefabToStorage(prefab, otherSubStorage))
	                        {
	                            inserted = true;
	                            break;
	                        }
	                    }
	                    
	                    if (inserted) break;
	                }
	            }
	            if (inserted)
	            {
	                movedCount++;
	                continue;
	            }
	            
	            EntitySpawnParams params = new EntitySpawnParams();
	            params.TransformMode = ETransformMode.WORLD;
	            Math3D.MatrixIdentity4(params.Transform);
	            params.Transform[3] = character.GetOrigin() + Vector(Math.RandomFloat(-0.5, 0.5), 0.1, Math.RandomFloat(-0.5, 0.5));
	            GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
	            droppedCount++;
	        }
	    }
	    
	    string hint = string.Format("%1 item(s) moved to new clothing.", movedCount);
	    if (droppedCount > 0)
	        hint = hint + string.Format(" %1 item(s) didn't fit and were dropped nearby.", droppedCount);
	    
	    if (movedCount > 0 || droppedCount > 0)
	        SCR_HintManagerComponent.GetInstance().ShowCustomHint(hint, "Clothing Swap", 4.0);
	    
	    Print("CGQC_Clothing_Switcher | Restored clothing contents", LogLevel.WARNING);
	}
}