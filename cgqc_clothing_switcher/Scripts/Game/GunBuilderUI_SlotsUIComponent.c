modded class GunBuilderUI_SlotsUIComponent
{
    override void OnClicked(GunBuilderUI_MultifunctionSlotUIComponent comp, int button)
    {
        Bacon_GunBuilderUI ui = Bacon_GunBuilderUI.GetInstance();
        
        if (ui && ui.m_CharacterEntity)
        {
            Bacon_GunBuilderUI_SlotInfo slotInfo = Bacon_GunBuilderUI_SlotInfo.Cast(comp.GetData());
            if (slotInfo && slotInfo.slotType == Bacon_GunBuilderUI_SlotType.CHARACTER_LOADOUT)
            {
                CGQC_ClothingStash.OnLeftItemPicker(ui.m_CharacterEntity);
            }
            else
            {
                Bacon_GunBuilderUI_SlotChoice choice = Bacon_GunBuilderUI_SlotChoice.Cast(comp.GetData());
                if (choice && 
                    (choice.slotType == Bacon_GunBuilderUI_SlotType.CHARACTER_LOADOUT ||
                     choice.slotType == Bacon_GunBuilderUI_SlotType.REMOVE) &&
                    !choice.prefab.IsEmpty())
                {
                    CGQC_ClothingStash.TrySave(ui.m_CharacterEntity, choice.prefab);
                }
            }
        }
        
        super.OnClicked(comp, button);
    }
}

class CGQC_ClothingStash
{
    static ref map<string, ref array<ResourceName>> s_SavedContents = new map<string, ref array<ResourceName>>();
    static ref map<string, EntityID> s_OldEntityIDs = new map<string, EntityID>();
    static ref map<EntityID, ref array<ResourceName>> s_PendingRestores = new map<EntityID, ref array<ResourceName>>();
    static ref array<ResourceName> s_SpawnQueue = new array<ResourceName>();
    static ref array<EntityID> s_SpawnTargetQueue = new array<EntityID>();
    static IEntity s_Character;
    static bool s_bWatching = false;
    static bool s_bPendingSave = false;
    static int s_iMovedCount = 0;
    static int s_iDroppedCount = 0;

    static void TrySave(IEntity character, ResourceName newPrefab)
    {
        s_Character = character;
        
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
        
        // Skip armor slot — plates are part of the prefab, restoring causes duplication
        if (areaKey == "LoadoutArmoredVestSlotArea") return;
		if (areaKey == "LoadoutHeadCoverArea") return;
        
        if (s_OldEntityIDs.Contains(areaKey)) return;
        
        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout) return;
        
        IEntity wornItem = loadout.GetClothFromArea(targetArea);
        if (!wornItem) return;
        
        array<ResourceName> prefabs = {};
        SaveClothItemContents(wornItem, areaKey, prefabs);
        
        s_OldEntityIDs.Set(areaKey, wornItem.GetID());
        
        if (!prefabs.IsEmpty())
            s_SavedContents.Set(areaKey, prefabs);
        else
            s_SavedContents.Set(areaKey, null);
        
        s_bPendingSave = true;
        
        if (!s_bWatching)
        {
            s_bWatching = true;
            GetGame().GetCallqueue().Remove(WatchForBLEClose);
            GetGame().GetCallqueue().CallLater(WatchForBLEClose, 100, true);
        }
    }
    
    static void OnLeftItemPicker(IEntity character)
    {
        if (!s_bPendingSave) return;
        s_Character = character;
        s_bPendingSave = false;
        
        GetGame().GetCallqueue().Remove(WatchForBLEClose);
        s_bWatching = false;
        
        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout) return;
        
        array<string> completedAreas = {};
        foreach (string areaKey, EntityID oldID : s_OldEntityIDs)
        {
            typename areaType = areaKey.ToType();
            IEntity current = loadout.GetClothFromArea(areaType);
            if (!current || current.GetID() != oldID)
                completedAreas.Insert(areaKey);
        }
        
        if (!completedAreas.IsEmpty())
        {
            foreach (string areaKey : completedAreas)
            {
                typename areaType = areaKey.ToType();
                IEntity newCloth = loadout.GetClothFromArea(areaType);
                array<ResourceName> prefabs = s_SavedContents.Get(areaKey);
                if (prefabs && newCloth)
                    s_PendingRestores.Set(newCloth.GetID(), prefabs);
                s_OldEntityIDs.Remove(areaKey);
                s_SavedContents.Remove(areaKey);
            }
            
            if (s_OldEntityIDs.IsEmpty())
                RunPendingRestores();
        }
        else
        {
            GetGame().GetCallqueue().Remove(CheckSwapComplete);
            GetGame().GetCallqueue().CallLater(CheckSwapComplete, 100, true);
        }
    }
    
    static void WatchForBLEClose()
    {
        if (Bacon_GunBuilderUI.GetInstance()) return;
        
        GetGame().GetCallqueue().Remove(WatchForBLEClose);
        s_bWatching = false;
        
        if (s_bPendingSave && s_Character)
        {
            s_bPendingSave = false;
            GetGame().GetCallqueue().Remove(CheckSwapComplete);
            GetGame().GetCallqueue().CallLater(CheckSwapComplete, 100, true);
        }
    }
    
    static void CheckSwapComplete()
    {
        if (!s_Character)
        {
            GetGame().GetCallqueue().Remove(CheckSwapComplete);
            s_SavedContents.Clear();
            s_OldEntityIDs.Clear();
            s_PendingRestores.Clear();
            s_bWatching = false;
            s_bPendingSave = false;
            return;
        }
        
        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            s_Character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout)
        {
            GetGame().GetCallqueue().Remove(CheckSwapComplete);
            s_SavedContents.Clear();
            s_OldEntityIDs.Clear();
            s_PendingRestores.Clear();
            s_bWatching = false;
            s_bPendingSave = false;
            return;
        }
        
        array<string> completedAreas = {};
        
        foreach (string areaKey, EntityID oldID : s_OldEntityIDs)
        {
            typename areaType = areaKey.ToType();
            IEntity current = loadout.GetClothFromArea(areaType);
            if (!current || current.GetID() != oldID)
                completedAreas.Insert(areaKey);
        }
        
        if (completedAreas.IsEmpty()) return;
        
        foreach (string areaKey : completedAreas)
        {
            typename areaType = areaKey.ToType();
            IEntity newCloth = loadout.GetClothFromArea(areaType);
            array<ResourceName> prefabs = s_SavedContents.Get(areaKey);
            if (prefabs && newCloth)
                s_PendingRestores.Set(newCloth.GetID(), prefabs);
            s_OldEntityIDs.Remove(areaKey);
            s_SavedContents.Remove(areaKey);
        }
        
        if (s_OldEntityIDs.IsEmpty())
        {
            GetGame().GetCallqueue().Remove(CheckSwapComplete);
            s_bWatching = false;
            RunPendingRestores();
        }
    }
    
    static void RunPendingRestores()
    {
        if (!s_Character)
        {
            s_PendingRestores.Clear();
            return;
        }
        
        foreach (EntityID id, array<ResourceName> prefabs : s_PendingRestores)
        {
            IEntity cloth = GetGame().GetWorld().FindEntityByID(id);
            if (!cloth) continue;
            
            foreach (ResourceName prefab : prefabs)
            {
                s_SpawnQueue.Insert(prefab);
                s_SpawnTargetQueue.Insert(id);
            }
        }
        
        s_PendingRestores.Clear();
        
        if (!s_SpawnQueue.IsEmpty())
        {
            s_iMovedCount = 0;
            s_iDroppedCount = 0;
            GetGame().GetCallqueue().Remove(SpawnNextItem);
            GetGame().GetCallqueue().CallLater(SpawnNextItem, 100, true);
        }
        else
        {
            s_Character = null;
        }
    }
    
    static void SpawnNextItem()
    {
        if (s_SpawnQueue.IsEmpty() || !s_Character)
        {
            GetGame().GetCallqueue().Remove(SpawnNextItem);
            s_SpawnQueue.Clear();
            s_SpawnTargetQueue.Clear();
            
            if (s_iMovedCount > 0 || s_iDroppedCount > 0)
            {
                string hint = string.Format("%1 item(s) moved to new clothing.", s_iMovedCount);
                if (s_iDroppedCount > 0)
                    hint = hint + string.Format(" %1 item(s) didn't fit and were dropped nearby.", s_iDroppedCount);
                SCR_HintManagerComponent.GetInstance().ShowCustomHint(hint, "Clothing Swap", 4.0);
            }
            
            s_iMovedCount = 0;
            s_iDroppedCount = 0;
            s_Character = null;
            return;
        }
        
        ResourceName prefab = s_SpawnQueue[0];
        s_SpawnQueue.RemoveOrdered(0);
        
        EntityID targetID = s_SpawnTargetQueue[0];
        s_SpawnTargetQueue.RemoveOrdered(0);
        
        IEntity clothItem = GetGame().GetWorld().FindEntityByID(targetID);
        
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(
            s_Character.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;
        
        SCR_CharacterInventoryStorageComponent charStorage = SCR_CharacterInventoryStorageComponent.Cast(
            s_Character.FindComponent(SCR_CharacterInventoryStorageComponent));
        
        BaseInventoryStorageComponent clothStorage = null;
        if (clothItem)
            clothStorage = BaseInventoryStorageComponent.Cast(
                clothItem.FindComponent(BaseInventoryStorageComponent));
        
        bool inserted = false;
        
        if (clothStorage && invManager.TrySpawnPrefabToStorage(prefab, clothStorage))
        {
            s_iMovedCount++;
            return;
        }
        
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
        if (inserted) { s_iMovedCount++; return; }
        
        if (charStorage)
        {
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
        if (inserted) { s_iMovedCount++; return; }
        
        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        Math3D.MatrixIdentity4(params.Transform);
        params.Transform[3] = s_Character.GetOrigin() + Vector(Math.RandomFloat(-0.5, 0.5), 0.1, Math.RandomFloat(-0.5, 0.5));
        GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
        s_iDroppedCount++;
    }
    
    static void SaveClothItemContents(IEntity clothItem, string areaKey, out array<ResourceName> prefabs)
    {
        BaseInventoryStorageComponent wornStorage = BaseInventoryStorageComponent.Cast(
            clothItem.FindComponent(BaseInventoryStorageComponent));
        if (!wornStorage) return;
        
        array<IEntity> items = {};
        wornStorage.GetAll(items);
        if (items.IsEmpty()) return;
        
        bool isVest = (areaKey == "LoadoutVestArea");
        
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
                
                if (isVest && invItem && !clothComp)
                {
                    // Vest: carriable container (IFAK, cig pack) — save as-is
                    ResourceName prefab;
                    if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(item, prefab))
                        prefabs.Insert(prefab);
                }
                else if (isVest)
                {
                    // Vest: structural pouch — recurse into contents
                    array<IEntity> subItems = {};
                    itemStorage.GetAll(subItems);
                    foreach (IEntity subItem : subItems)
                    {
                        ResourceName subPrefab;
                        if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(subItem, subPrefab))
                            prefabs.Insert(subPrefab);
                    }
                }
                else
                {
                    // Non-vest: always save container as-is
                    ResourceName prefab;
                    if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(item, prefab))
                        prefabs.Insert(prefab);
                }
                continue;
            }
            
            // Plain item
            ResourceName prefab;
            if (Bacon_GunBuilderUI_Helpers.GetResourceNameFromEntity(item, prefab))
                prefabs.Insert(prefab);
        }
    }
}