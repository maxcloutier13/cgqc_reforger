modded class GunBuilderUI_SlotsUIComponent
{
    protected ref set<string> m_SavedAreas = new set<string>();
    protected bool m_bWatchingStarted = false;
    
    override void OnClicked(GunBuilderUI_MultifunctionSlotUIComponent comp, int button)
    {
        Print("[CGQC_CS] OnClicked fired. button=" + button);
        
        Bacon_GunBuilderUI_SlotChoice choice = Bacon_GunBuilderUI_SlotChoice.Cast(comp.GetData());
        if (!choice)
        {
            Print("[CGQC_CS] OnClicked: choice cast failed, bailing");
            super.OnClicked(comp, button);
            return;
        }
        
        Print("[CGQC_CS] OnClicked: slotType=" + choice.slotType + " prefab=" + choice.prefab);
        
        if (choice.slotType == Bacon_GunBuilderUI_SlotType.CHARACTER_LOADOUT ||
             choice.slotType == Bacon_GunBuilderUI_SlotType.REMOVE)
        {
            if (choice.prefab.IsEmpty())
            {
                Print("[CGQC_CS] OnClicked: prefab is empty, skipping save");
            }
            else
            {
                Bacon_GunBuilderUI ui = Bacon_GunBuilderUI.GetInstance();             
                
                if (ui && ui.m_CharacterEntity)
                {
                    Print("[CGQC_CS] OnClicked: calling SaveSlotIfNew. savedAreas.Count=" + m_SavedAreas.Count());
                    CGQC_ClothingStash.SaveSlotIfNew(ui.m_CharacterEntity, choice.prefab, m_SavedAreas);
                    Print("[CGQC_CS] OnClicked: after SaveSlotIfNew. savedAreas.Count=" + m_SavedAreas.Count());
                    
                    if (!m_bWatchingStarted && m_SavedAreas.Count() > 0)
                    {
                        Print("[CGQC_CS] OnClicked: starting UI watcher");
                        CGQC_ClothingStash.StartWatchingUI(ui);
                        m_bWatchingStarted = true;
                    }
                }
                else
                {
                    Print("[CGQC_CS] OnClicked: ui or m_CharacterEntity is null, skipping");
                }
            }
        }
        else
        {
            Print("[CGQC_CS] OnClicked: slotType not CHARACTER_LOADOUT or REMOVE, skipping");
        }
        
        super.OnClicked(comp, button);
    }
}

class CGQC_ClothingStash
{
    static ref map<string, ref array<ResourceName>> s_SavedContents = new map<string, ref array<ResourceName>>();
    static ref set<string> s_RemovalAreas = new set<string>();
    static IEntity s_Character;
    static Bacon_GunBuilderUI s_UI;

    static void StartWatchingUI(Bacon_GunBuilderUI ui)
    {
        Print("[CGQC_CS] StartWatchingUI called. ui=" + ui);
        s_UI = ui;
        GetGame().GetCallqueue().CallLater(CheckUIAlive, 100, true);
    }

    static void CheckUIAlive()
    {
        bool uiNull = !s_UI;
        bool instanceNull = !Bacon_GunBuilderUI.GetInstance();
        Print("[CGQC_CS] CheckUIAlive: s_UI null=" + uiNull + " GetInstance null=" + instanceNull);
        
        if (uiNull || instanceNull)
        {
            Print("[CGQC_CS] CheckUIAlive: UI gone, calling RestoreIfNeeded");
            RestoreIfNeeded();
            GetGame().GetCallqueue().Remove(CheckUIAlive);
        }
    }

    static void RestoreIfNeeded()
    {
        Print("[CGQC_CS] RestoreIfNeeded: s_Character=" + s_Character + " savedContents.Count=" + s_SavedContents.Count());
        
        if (s_Character)
            Restore(s_Character);
        
        s_Character = null;
        s_UI = null;
        s_SavedContents.Clear();
        s_RemovalAreas.Clear();
        Print("[CGQC_CS] RestoreIfNeeded: state cleared");
    }

    static void SaveSlotIfNew(IEntity character, ResourceName newPrefab, set<string> savedAreas)
    {
        Print("[CGQC_CS] SaveSlotIfNew: prefab=" + newPrefab + " savedAreas.Count=" + savedAreas.Count());
        s_Character = character;
        
        if (newPrefab == "empty")
        {
            Print("[CGQC_CS] SaveSlotIfNew: prefab is 'empty', scanning all clothing slots for contents");
            
            SCR_CharacterInventoryStorageComponent charStorage = SCR_CharacterInventoryStorageComponent.Cast(
                character.FindComponent(SCR_CharacterInventoryStorageComponent));
            if (!charStorage)
            {
                Print("[CGQC_CS] SaveSlotIfNew: charStorage cast failed");
                return;
            }
            
            Print("[CGQC_CS] SaveSlotIfNew: charStorage found, slotsCount=" + charStorage.GetSlotsCount());
            
            for (int i = 0; i < charStorage.GetSlotsCount(); i++)
            {
                LoadoutSlotInfo slot = LoadoutSlotInfo.Cast(charStorage.GetSlot(i));
                if (!slot) continue;
                
                LoadoutAreaType areaType = slot.GetAreaType();
                if (!areaType) continue;
                
                string areaKey = areaType.Type().ToString();
                if (savedAreas.Contains(areaKey))
                {
                    Print("[CGQC_CS] SaveSlotIfNew: area " + areaKey + " already saved, skipping");
                    continue;
                }
                
                IEntity clothItem = slot.GetAttachedEntity();
                if (!clothItem)
                {
                    Print("[CGQC_CS] SaveSlotIfNew: slot " + areaKey + " has no attached entity");
                    continue;
                }
                
                Print("[CGQC_CS] SaveSlotIfNew: saving area=" + areaKey + " from entity=" + clothItem);
                SaveClothItemContents(clothItem, areaKey, savedAreas);
                s_RemovalAreas.Insert(areaKey);
            }
            return;
        }
        
        Print("[CGQC_CS] SaveSlotIfNew: spawning temp entity for prefab=" + newPrefab);
        IEntity tempEnt = GetGame().SpawnEntityPrefabLocal(
            Resource.Load(newPrefab), GetGame().GetWorld(), null);
        if (!tempEnt)
        {
            Print("[CGQC_CS] SaveSlotIfNew: SpawnEntityPrefabLocal returned null");
            return;
        }
        
        BaseLoadoutClothComponent cloth = BaseLoadoutClothComponent.Cast(
            tempEnt.FindComponent(BaseLoadoutClothComponent));
        
        if (!cloth || !cloth.GetAreaType())
        {
            Print("[CGQC_CS] SaveSlotIfNew: temp entity has no BaseLoadoutClothComponent or no AreaType, deleting. cloth=" + cloth);
            SCR_EntityHelper.DeleteEntityAndChildren(tempEnt);
            return;
        }
        
        typename targetArea = cloth.GetAreaType().Type();
        string areaKey = targetArea.ToString();
        Print("[CGQC_CS] SaveSlotIfNew: temp entity area=" + areaKey);
        SCR_EntityHelper.DeleteEntityAndChildren(tempEnt);
        
        if (savedAreas.Contains(areaKey))
        {
            Print("[CGQC_CS] SaveSlotIfNew: area already saved, skipping");
            return;
        }
        
        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout)
        {
            Print("[CGQC_CS] SaveSlotIfNew: EquipedLoadoutStorageComponent cast failed");
            return;
        }
        
        IEntity wornItem = loadout.GetClothFromArea(targetArea);
        if (!wornItem)
        {
            Print("[CGQC_CS] SaveSlotIfNew: no worn item in area=" + areaKey + ", nothing to save");
            return;
        }
        
        Print("[CGQC_CS] SaveSlotIfNew: found worn item=" + wornItem + " in area=" + areaKey);
        SaveClothItemContents(wornItem, areaKey, savedAreas);
    }
    
    static void SaveClothItemContents(IEntity clothItem, string areaKey, set<string> savedAreas)
    {
        Print("[CGQC_CS] SaveClothItemContents: areaKey=" + areaKey + " entity=" + clothItem);
        
        BaseInventoryStorageComponent wornStorage = BaseInventoryStorageComponent.Cast(
            clothItem.FindComponent(BaseInventoryStorageComponent));
        if (!wornStorage)
        {
            Print("[CGQC_CS] SaveClothItemContents: no BaseInventoryStorageComponent on item, skipping");
            return;
        }
        
        array<IEntity> items = {};
        wornStorage.GetAll(items);
        Print("[CGQC_CS] SaveClothItemContents: found " + items.Count() + " item(s) in " + areaKey);
        
        savedAreas.Insert(areaKey);
        
        if (items.IsEmpty())
        {
            Print("[CGQC_CS] SaveClothItemContents: item is empty, nothing to stash");
            return;
        }
        
        array<ResourceName> prefabs = {};
        foreach (IEntity item : items)
		{
		    BaseInventoryStorageComponent itemStorage = BaseInventoryStorageComponent.Cast(
		        item.FindComponent(BaseInventoryStorageComponent));
		    
		    if (itemStorage)
			{
			    InventoryItemComponent invItem = InventoryItemComponent.Cast(
			        item.FindComponent(InventoryItemComponent));
			    BaseLoadoutClothComponent clothComp = BaseLoadoutClothComponent.Cast(
			        item.FindComponent(BaseLoadoutClothComponent));
			    
			    if (invItem && !clothComp)
			    {
			        // Carriable container (cig pack, IFAK) — save as-is
			        ResourceName prefab;
			        if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(item, prefab))
			            prefabs.Insert(prefab);
			    }
			    else
			    {
			        // Structural pouch — save its contents
			        array<IEntity> subItems = {};
			        itemStorage.GetAll(subItems);
			        foreach (IEntity subItem : subItems)
			        {
			            ResourceName subPrefab;
			            if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(subItem, subPrefab))
			                prefabs.Insert(subPrefab);
			        }
			    }
			    continue;
			}
		    
		    // Plain item
		    ResourceName prefab;
		    if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(item, prefab))
		        prefabs.Insert(prefab);
		}
		        
        Print("[CGQC_CS] SaveClothItemContents: total prefabs stashed=" + prefabs.Count() + " for area=" + areaKey);
        
        if (!prefabs.IsEmpty())
            s_SavedContents.Set(areaKey, prefabs);
        else
            Print("[CGQC_CS] SaveClothItemContents: no prefabs resolved, nothing stored for " + areaKey);
    }
    
    static void Restore(IEntity character)
    {
        Print("[CGQC_CS] Restore: starting. savedContents.Count=" + s_SavedContents.Count() + " removalAreas.Count=" + s_RemovalAreas.Count());
        
        if (s_SavedContents.IsEmpty())
        {
            Print("[CGQC_CS] Restore: nothing saved, early out");
            return;
        }
        
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(
            character.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager)
        {
            Print("[CGQC_CS] Restore: invManager cast failed");
            return;
        }
        
        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout)
        {
            Print("[CGQC_CS] Restore: loadout cast failed");
            return;
        }
        
        SCR_CharacterInventoryStorageComponent charStorage = SCR_CharacterInventoryStorageComponent.Cast(
            character.FindComponent(SCR_CharacterInventoryStorageComponent));
        Print("[CGQC_CS] Restore: charStorage=" + charStorage);
        
        int droppedCount = 0;
        int movedCount = 0;
        
        foreach (string areaKey, array<ResourceName> prefabs : s_SavedContents)
        {
            typename areaType = areaKey.ToType();
            IEntity clothItem = loadout.GetClothFromArea(areaType);
            bool isRemoval = s_RemovalAreas.Contains(areaKey);
            
            Print("[CGQC_CS] Restore: processing area=" + areaKey + " isRemoval=" + isRemoval + " clothItem=" + clothItem + " prefabs.Count=" + prefabs.Count());
            
            if (isRemoval && clothItem)
            {
                Print("[CGQC_CS] Restore: removal area but slot still occupied, skipping");
                continue;
            }
            
            if (!isRemoval && !clothItem)
            {
                Print("[CGQC_CS] Restore: swap area but slot is empty after swap, skipping");
                continue;
            }
            
            BaseInventoryStorageComponent clothStorage = null;
            if (clothItem)
                clothStorage = BaseInventoryStorageComponent.Cast(
                    clothItem.FindComponent(BaseInventoryStorageComponent));
            
            Print("[CGQC_CS] Restore: clothStorage=" + clothStorage);
            
            foreach (ResourceName prefab : prefabs)
            {
                Print("[CGQC_CS] Restore: trying to restore prefab=" + prefab);
                
                if (clothStorage && invManager.TrySpawnPrefabToStorage(prefab, clothStorage))
                {
                    Print("[CGQC_CS] Restore: placed in clothStorage directly");
                    movedCount++;
                    continue;
                }
                
                bool inserted = false;
                if (clothStorage)
                {
                    array<BaseInventoryStorageComponent> subStorages = {};
                    clothStorage.GetOwnedStorages(subStorages, 1, false);
                    Print("[CGQC_CS] Restore: checking " + subStorages.Count() + " sub-storages of new clothing");
                    foreach (BaseInventoryStorageComponent subStorage : subStorages)
                    {
                        if (invManager.TrySpawnPrefabToStorage(prefab, subStorage))
                        {
                            Print("[CGQC_CS] Restore: placed in sub-storage of new clothing");
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
                
                if (charStorage)
                {
                    Print("[CGQC_CS] Restore: trying other clothing slots. slotsCount=" + charStorage.GetSlotsCount());
                    for (int i = 0; i < charStorage.GetSlotsCount(); i++)
                    {
                        LoadoutSlotInfo slot = LoadoutSlotInfo.Cast(charStorage.GetSlot(i));
                        if (!slot) continue;
                        
                        IEntity otherCloth = slot.GetAttachedEntity();
                        if (!otherCloth || otherCloth == clothItem) continue;
                        
                        BaseInventoryStorageComponent otherStorage = BaseInventoryStorageComponent.Cast(
                            otherCloth.FindComponent(BaseInventoryStorageComponent));
                        if (!otherStorage) continue;
                        
                        if (invManager.TrySpawnPrefabToStorage(prefab, otherStorage))
                        {
                            Print("[CGQC_CS] Restore: placed in other clothing slot storage, entity=" + otherCloth);
                            inserted = true;
                            break;
                        }
                        
                        array<BaseInventoryStorageComponent> otherSubStorages = {};
                        otherStorage.GetOwnedStorages(otherSubStorages, 1, false);
                        foreach (BaseInventoryStorageComponent otherSubStorage : otherSubStorages)
                        {
                            if (invManager.TrySpawnPrefabToStorage(prefab, otherSubStorage))
                            {
                                Print("[CGQC_CS] Restore: placed in sub-storage of other clothing, entity=" + otherCloth);
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
                
                Print("[CGQC_CS] Restore: no storage fit, dropping prefab=" + prefab + " near character");
                EntitySpawnParams params = new EntitySpawnParams();
                params.TransformMode = ETransformMode.WORLD;
                Math3D.MatrixIdentity4(params.Transform);
                params.Transform[3] = character.GetOrigin() + Vector(Math.RandomFloat(-0.5, 0.5), 0.1, Math.RandomFloat(-0.5, 0.5));
                GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
                droppedCount++;
            }
        }
        
        Print("[CGQC_CS] Restore: done. movedCount=" + movedCount + " droppedCount=" + droppedCount);
        
        string hint = string.Format("%1 item(s) moved to new clothing.", movedCount);
        if (droppedCount > 0)
            hint = hint + string.Format(" %1 item(s) didn't fit and were dropped nearby.", droppedCount);
        
        if (movedCount > 0 || droppedCount > 0)
            SCR_HintManagerComponent.GetInstance().ShowCustomHint(hint, "Clothing Swap", 4.0);
    }
}