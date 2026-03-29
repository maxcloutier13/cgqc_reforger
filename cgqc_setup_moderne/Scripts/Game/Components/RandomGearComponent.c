// =========================================================================
// 1. UNIFIED MODULAR CONFIG CLASS WITH WEIGHTS
// =========================================================================

[BaseContainerProps()]
class SCR_AdvancedGearConfig
{
    [Attribute(desc: "The .et path for this specific item.", params: "et")]
    ResourceName m_Prefab;
    
    // --- OLD LOGIC (Keep for Attachments/Backpack Contents) ---
    [Attribute(desc: "ATTACHMENTS/INTERNAL: Spawns ON/INSIDE this item (e.g. Scopes, Suppressors, or items inside a Backpack).", params: "et")]
    ref array<ResourceName> m_aSpecificItems;

    // --- NEW LOGIC (For Ammo/Meds) ---
    [Attribute(desc: "POCKET ITEMS: Spawns in VEST/POCKETS (e.g. Magazines, Grenades, Meds associated with this item).", params: "et")]
    ref array<ResourceName> m_aOverflowItems;

    // --- WEIGHTS ---
    [Attribute(defvalue: "0", desc: "Enable custom spawn chance for this specific item?", category: "Spawn Weights")]
    bool m_bUseCustomWeight;

    [Attribute(defvalue: "10", desc: "Custom spawn chance in % (e.g., 5 for 5%).", uiwidget: UIWidgets.Slider, params: "0 100 0.1", category: "Spawn Weights")]
    float m_fCustomWeight;
}

[BaseContainerProps(configRoot: true)]
class SCR_GearPoolConfig
{
    [Attribute(desc: "SIMPLE POOL: Standard list of .et paths.", params: "et")]
    ref array<ResourceName> m_aGearPool;

    [Attribute(desc: "ADVANCED POOL: Bundle specific items and custom % weights with specific gear.")]
    ref array<ref SCR_AdvancedGearConfig> m_aAdvancedPool;
    
    [Attribute(desc: "GLOBAL ITEMS: Spawns these items into THIS specific piece of gear WHENEVER this config is used.", params: "et")]
    ref array<ResourceName> m_aGlobalItems;
}

// =========================================================================
// 2. THE COMPONENT
// =========================================================================
[ComponentEditorProps(category: "Custom Modding", description: "Lightning Fast Gear Randomizer: Weighted Spawns, Targeted Pockets, Auto-Equip.")]
class RandomGearComponentClass : ScriptComponentClass {}

class RandomGearComponent : ScriptComponent
{
    // =====================================================================
    // --- WEAPONS ---
    // =====================================================================
    [Attribute(desc: "Primary Weapon (Slot 0 - Auto Equips to Hands).", UIWidgets.Object, category: "Weapons")]
    ref SCR_GearPoolConfig m_PrimaryWeaponHandsConfig;

    [Attribute(desc: "Primary Weapon (Slot 1 - Spawns on Back).", UIWidgets.Object, category: "Weapons")]
    ref SCR_GearPoolConfig m_PrimaryWeaponBackConfig;

    [Attribute(desc: "Secondary Weapon (Handgun/Holster).", UIWidgets.Object, category: "Weapons")]
    ref SCR_GearPoolConfig m_SecondaryWeaponConfig;

    // =====================================================================
    // --- VANILLA SLOTS ---
    // =====================================================================
    [Attribute(desc: "Helmets and basic headwear.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_HeadConfig;

    [Attribute(desc: "Facewear, masks, and balaclavas.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_FaceConfig;

    [Attribute(desc: "Jackets and Tops.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_TopsConfig;

    [Attribute(desc: "Armored Vests (Plate carriers).", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_ArmorConfig;

    [Attribute(desc: "Chest Rigs and Webbing.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_VestsConfig;
    
    [Attribute(desc: "Backpacks.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_BackpacksConfig;

    [Attribute(desc: "Pants and Trousers.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_PantsConfig;
    
    [Attribute(desc: "Boots and footwear.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_BootsConfig;
    
    [Attribute(desc: "Gloves and handwear.", UIWidgets.Object, category: "Vanilla Slots")]
    ref SCR_GearPoolConfig m_GlovesConfig;

    // =====================================================================
    // --- ZEL CUSTOM SLOTS ---
    // =====================================================================
    [Attribute(desc: "Custom Hat slot.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_HatConfig;

    [Attribute(desc: "Custom Eyes (Glasses/Goggles) slot.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_EyesConfig;
    
    [Attribute(desc: "Custom Ears (Headsets/Comms) slot.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_EarsConfig;

    [Attribute(desc: "Custom Neck (Scarves/Shemaghs) slot.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_NeckConfig;
    
    [Attribute(desc: "Custom Waist (Battle belts) slot.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_WaistConfig;

    [Attribute(desc: "Extra Custom Area 1.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_Extra1Config;

    [Attribute(desc: "Extra Custom Area 2.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_Extra2Config;
    
    [Attribute(desc: "Extra Custom Area 3.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_Extra3Config;

    [Attribute(desc: "Extra Custom Area 4.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_Extra4Config;

    [Attribute(desc: "Extra Custom Area 5.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_Extra5Config;
    
    [Attribute(desc: "Extra Custom Area 6.", UIWidgets.Object, category: "Zel Custom Slots")]
    ref SCR_GearPoolConfig m_Extra6Config;

    // --- Thread-Safe Memory Maps ---
    ref map<IEntity, ref array<ResourceName>> m_PendingAmmo = new map<IEntity, ref array<ResourceName>>(); 
    ref map<IEntity, ref array<ResourceName>> m_PendingGlobalItems = new map<IEntity, ref array<ResourceName>>();
    ref map<IEntity, ref map<ResourceName, ref array<ResourceName>>> m_AllPendingPockets = new map<IEntity, ref map<ResourceName, ref array<ResourceName>>>();

    override void OnPostInit(IEntity owner)
    {
        SetEventMask(owner, EntityEvent.INIT);
    }

    override void EOnInit(IEntity owner)
    {
        GetGame().GetCallqueue().CallLater(RandomizeGear, 100, false, owner);
    }

    void RandomizeGear(IEntity owner)
    {
        if (!owner) return; 
        
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;

        array<IEntity> allItems = new array<IEntity>();
        invManager.GetItems(allItems);
        
        ref array<ResourceName> savedAmmo = new array<ResourceName>();
        m_PendingAmmo.Insert(owner, savedAmmo);

        bool shouldReplaceWeapons = (m_PrimaryWeaponHandsConfig != null) || (m_PrimaryWeaponBackConfig != null) || (m_SecondaryWeaponConfig != null);

        foreach (IEntity item : allItems)
        {
            if (!item) continue; 
            
            if (item.FindComponent(BaseWeaponComponent))
            {
                if (shouldReplaceWeapons)
                {
                    invManager.TryRemoveItemFromInventory(item);
                    SCR_EntityHelper.DeleteEntityAndChildren(item);
                }
                continue;
            }

            BaseLoadoutClothComponent clothComp = BaseLoadoutClothComponent.Cast(item.FindComponent(BaseLoadoutClothComponent));
            if (clothComp)
            {
                string areaName = clothComp.GetAreaType().ToString();
                bool shouldReplace = false;
                
                if (m_HeadConfig && areaName.Contains("Head")) shouldReplace = true;
                if (m_FaceConfig && areaName.Contains("Face")) shouldReplace = true;
                if (m_TopsConfig && areaName.Contains("Jacket")) shouldReplace = true;
                if (m_ArmorConfig && areaName.Contains("ArmoredVest")) shouldReplace = true;
                if (m_VestsConfig && areaName.Contains("Vest") && !areaName.Contains("ArmoredVest")) shouldReplace = true;
                if (m_BackpacksConfig && (areaName.Contains("Backpack") || areaName.Contains("Back"))) shouldReplace = true;
                if (m_PantsConfig && areaName.Contains("Pants")) shouldReplace = true;
                if (m_BootsConfig && areaName.Contains("Boot")) shouldReplace = true;
                if (m_GlovesConfig && areaName.Contains("Handwear")) shouldReplace = true;

                if (areaName.Contains("Hat") || areaName.Contains("Eyes") || areaName.Contains("Ears") || 
                    areaName.Contains("Neck") || areaName.Contains("Waist") || areaName.Contains("Extra")) 
                {
                    shouldReplace = true;
                }

                if (!shouldReplace) continue;

                array<IEntity> processedEntities = new array<IEntity>();
                ExtractAmmoAndMeds(item, savedAmmo, processedEntities, true);
                
                invManager.TryRemoveItemFromInventory(item);
                SCR_EntityHelper.DeleteEntityAndChildren(item);
            }
        }

        GetGame().GetCallqueue().CallLater(SpawnNewGear, 150, false, owner);
    }

    void SpawnNewGear(IEntity owner)
    {
        if (!owner) return;
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;

        ref array<ResourceName> globalItems = new array<ResourceName>();
        m_PendingGlobalItems.Insert(owner, globalItems);

        ref map<ResourceName, ref array<ResourceName>> pendingPockets = new map<ResourceName, ref array<ResourceName>>();
        m_AllPendingPockets.Insert(owner, pendingPockets);

        SpawnItemAndQueueContents(owner, invManager, m_HeadConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_FaceConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_TopsConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_ArmorConfig, pendingPockets); 
        SpawnItemAndQueueContents(owner, invManager, m_VestsConfig, pendingPockets); 
        SpawnItemAndQueueContents(owner, invManager, m_BackpacksConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_PantsConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_BootsConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_GlovesConfig, pendingPockets);

        SpawnItemAndQueueContents(owner, invManager, m_HatConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_EyesConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_EarsConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_NeckConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_WaistConfig, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_Extra1Config, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_Extra2Config, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_Extra3Config, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_Extra4Config, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_Extra5Config, pendingPockets);
        SpawnItemAndQueueContents(owner, invManager, m_Extra6Config, pendingPockets);

        GetGame().GetCallqueue().CallLater(InjectItemsIntoPockets, 200, false, owner);
        GetGame().GetCallqueue().CallLater(SpawnWeapons, 350, false, owner);
    }

    void InjectItemsIntoPockets(IEntity owner)
    {
        if (!owner) return;
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;

        map<ResourceName, ref array<ResourceName>> myPockets = m_AllPendingPockets.Get(owner);
        if (!myPockets || myPockets.Count() == 0) return;

        array<IEntity> allItems = new array<IEntity>();
        invManager.GetItems(allItems);

        foreach (IEntity item : allItems)
        {
            ResourceName itemPrefab = item.GetPrefabData().GetPrefabName();

            if (myPockets.Contains(itemPrefab))
            {
                array<ResourceName> itemsToForce = myPockets.Get(itemPrefab);
                
                array<Managed> storages = new array<Managed>();
                item.FindComponents(BaseInventoryStorageComponent, storages);

                foreach (ResourceName prefabToForce : itemsToForce)
                {
                    if (prefabToForce.IsEmpty()) continue;
                    
                    bool inserted = false;

                    Resource res = Resource.Load(prefabToForce);
                    if (res && res.IsValid())
                    {
                        EntitySpawnParams spawnParams = new EntitySpawnParams();
                        spawnParams.TransformMode = ETransformMode.WORLD;
                        owner.GetWorldTransform(spawnParams.Transform);
                        IEntity spawnedItem = GetGame().SpawnEntityPrefab(res, owner.GetWorld(), spawnParams);

                        if (spawnedItem)
                        {
                            // Try to insert INSIDE the item (Backpack/Vest/Attachment)
                            foreach (Managed comp : storages)
                            {
                                BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(comp);
                                if (storage && invManager.TryInsertItemInStorage(spawnedItem, storage))
                                {
                                    inserted = true;
                                    break;
                                }
                            }
                            
                            // NOTE: If we can't put it in the storage, we DELETE it.
                            // We do NOT put it in general inventory here, because this list is for ATTACHMENTS/CONTENTS.
                            if (!inserted) 
                            {
                                SCR_EntityHelper.DeleteEntityAndChildren(spawnedItem);
                            }
                        }
                    }
                }
                
                myPockets.Remove(itemPrefab);
            }
        }
        
        m_AllPendingPockets.Remove(owner);
    }

    void SpawnWeapons(IEntity owner)
    {
        if (!owner) return;
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;

        ref map<ResourceName, ref array<ResourceName>> dummyPockets = new map<ResourceName, ref array<ResourceName>>();

        // Note: dummyPockets is passed for attachments logic.
        // Ammo/Overflow logic will handle the global map internally.
        SpawnItemAndQueueContents(owner, invManager, m_PrimaryWeaponHandsConfig, dummyPockets);
        SpawnItemAndQueueContents(owner, invManager, m_PrimaryWeaponBackConfig, dummyPockets);
        SpawnItemAndQueueContents(owner, invManager, m_SecondaryWeaponConfig, dummyPockets);
        
        // Handle attachments for weapons (if any in specific list)
        GetGame().GetCallqueue().CallLater(InjectItemsIntoPockets, 50, false, owner); 

        GetGame().GetCallqueue().CallLater(SpawnGlobalContentsAndRestoreAmmo, 200, false, owner);
        GetGame().GetCallqueue().CallLater(ForceWeaponToHands, 350, false, owner);
    }

    void ForceWeaponToHands(IEntity owner)
    {
        if (!owner) return;
        
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;

        array<IEntity> items = new array<IEntity>();
        invManager.GetItems(items);

        IEntity weaponToEquip = null;

        foreach (IEntity item : items)
        {
            if (item.FindComponent(BaseWeaponComponent))
            {
                weaponToEquip = item;
                break; 
            }
        }

        if (weaponToEquip)
        {
            CharacterControllerComponent charController = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
            if (charController)
            {
                charController.TryEquipRightHandItem(weaponToEquip, EEquipItemType.EEquipTypeWeapon);
            }
        }
    }

    void SpawnGlobalContentsAndRestoreAmmo(IEntity owner)
    {
        if (!owner) return;
        SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invManager) return;

        array<ResourceName> globalItemsToSpawn = m_PendingGlobalItems.Get(owner);
        if (globalItemsToSpawn)
        {
            foreach (ResourceName item : globalItemsToSpawn)
            {
                if (!item.IsEmpty()) invManager.TrySpawnPrefabToStorage(item);
            }
        }
        m_PendingGlobalItems.Remove(owner);

        array<ResourceName> ammoToRestore = m_PendingAmmo.Get(owner);
        if (ammoToRestore)
        {
            foreach (ResourceName res : ammoToRestore)
            {
                if (!res.IsEmpty()) invManager.TrySpawnPrefabToStorage(res);
            }
        }
        m_PendingAmmo.Remove(owner);
    }

    // --- HELPER FUNCTIONS ---

    SCR_AdvancedGearConfig GetWeightedSelection(array<ref SCR_AdvancedGearConfig> pool)
    {
        if (!pool || pool.Count() == 0) return null;
        if (pool.Count() == 1) return pool[0];

        float totalCustomWeight = 0;
        int unweightedCount = 0;

        foreach (SCR_AdvancedGearConfig item : pool)
        {
            if (item.m_bUseCustomWeight)
                totalCustomWeight += item.m_fCustomWeight;
            else
                unweightedCount++;
        }

        float remainingWeight = Math.Max(0.0, 100.0 - totalCustomWeight);
        float defaultWeight = 0;
        
        if (unweightedCount > 0)
            defaultWeight = remainingWeight / unweightedCount;

        float randomVal = Math.RandomFloat(0, 100);
        float cumulative = 0;

        foreach (SCR_AdvancedGearConfig item : pool)
        {
            float currentWeight = defaultWeight;
            if (item.m_bUseCustomWeight)
                currentWeight = item.m_fCustomWeight;

            cumulative += currentWeight;
            if (randomVal <= cumulative)
                return item;
        }

        return pool[pool.Count() - 1]; 
    }

    void SpawnItemAndQueueContents(IEntity owner, SCR_InventoryStorageManagerComponent invManager, SCR_GearPoolConfig config, map<ResourceName, ref array<ResourceName>> pendingPockets)
    {
        if (!config) return;

        ResourceName selectedPrefab = ResourceName.Empty;
        
        // Arrays to hold the two types of items
        ref array<ResourceName> attachmentsToAttach = new array<ResourceName>();
        ref array<ResourceName> itemsForGlobalInv = new array<ResourceName>();

        // 1. Check Advanced Pool (Weighted)
        if (config.m_aAdvancedPool && config.m_aAdvancedPool.Count() > 0)
        {
            SCR_AdvancedGearConfig selectedGear = GetWeightedSelection(config.m_aAdvancedPool);
            if (selectedGear && !selectedGear.m_Prefab.IsEmpty())
            {
                selectedPrefab = selectedGear.m_Prefab;
                
                // --- OLD LIST: Attachments / Internal Items ---
                if (selectedGear.m_aSpecificItems)
                {
                    foreach (ResourceName spec : selectedGear.m_aSpecificItems) attachmentsToAttach.Insert(spec);
                }

                // --- NEW LIST: Overflow / Pockets Items ---
                if (selectedGear.m_aOverflowItems)
                {
                    foreach (ResourceName over : selectedGear.m_aOverflowItems) itemsForGlobalInv.Insert(over);
                }
            }
        }
        // 2. Check Simple Pool (Random)
        else if (config.m_aGearPool && config.m_aGearPool.Count() > 0)
        {
            selectedPrefab = config.m_aGearPool.Get(Math.RandomInt(0, config.m_aGearPool.Count()));
        }

        // 3. Global Items logic (ALWAYS applied to Global/Pockets now)
        if (config.m_aGlobalItems && !selectedPrefab.IsEmpty())
        {
            foreach (ResourceName glob : config.m_aGlobalItems) 
            {
                // We assume global items config meant "add to inventory" generally
                itemsForGlobalInv.Insert(glob); 
            }
        }

        if (!selectedPrefab.IsEmpty())
        {
            invManager.TrySpawnPrefabToStorage(selectedPrefab);
            
            // Logic A: Attachments (attach to this specific item)
            if (attachmentsToAttach.Count() > 0)
            {
                pendingPockets.Insert(selectedPrefab, attachmentsToAttach);
            }

            // Logic B: Ammo/Overflow (send to general inventory queue)
            if (itemsForGlobalInv.Count() > 0)
            {
                array<ResourceName> globalList = m_PendingGlobalItems.Get(owner);
                if (globalList)
                {
                    foreach(ResourceName item : itemsForGlobalInv)
                    {
                        globalList.Insert(item);
                    }
                }
            }
        }
    }

    void ExtractAmmoAndMeds(IEntity entityToSearch, array<ResourceName> savedAmmo, array<IEntity> processedEntities, bool isRoot)
    {
        if (!entityToSearch) return;

        if (processedEntities.Contains(entityToSearch)) return;
        processedEntities.Insert(entityToSearch);

        if (!isRoot)
        {
            bool isAtomicItem = false;
            if (entityToSearch.FindComponent(BaseWeaponComponent)) isAtomicItem = true;
            if (entityToSearch.FindComponent(BaseMagazineComponent)) isAtomicItem = true;
            if (entityToSearch.FindComponent(SCR_ConsumableItemComponent)) isAtomicItem = true;
            if (entityToSearch.FindComponent(SCR_GadgetComponent)) isAtomicItem = true;
            if (!entityToSearch.FindComponent(BaseInventoryStorageComponent)) isAtomicItem = true;

            if (isAtomicItem)
            {
                ResourceName prefabName = entityToSearch.GetPrefabData().GetPrefabName();
                if (!prefabName.IsEmpty()) savedAmmo.Insert(prefabName);
                return; 
            }
        }

        BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(entityToSearch.FindComponent(BaseInventoryStorageComponent));
        if (storage)
        {
            array<IEntity> storedItems = new array<IEntity>();
            storage.GetAll(storedItems);
            foreach (IEntity storedItem : storedItems)
            {
                ExtractAmmoAndMeds(storedItem, savedAmmo, processedEntities, false);
            }
        }

        IEntity child = entityToSearch.GetChildren();
        while (child)
        {
            if (child.FindComponent(BaseInventoryStorageComponent))
            {
                ExtractAmmoAndMeds(child, savedAmmo, processedEntities, false);
            }
            child = child.GetSibling();
        }
    }
}