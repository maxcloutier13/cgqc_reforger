//------------------------------------------------------------------------------------------------
// CGQC Personal Patch
// Puts the player's roster patch (CGQC_PlayerEntry.m_sPatch) on their jacket. Server only.
// Slots are matched by name, so cgqc_core doesn't need GRS as a dependency.
// Does nothing if there is no jacket, no matching slot, or the patch mod isn't loaded.
//------------------------------------------------------------------------------------------------

class CGQC_PersonalPatch
{
	// Slot that receives the personal patch
	static const string PATCH_SLOT = "Lower_Right_Patch";

	// Attribute default of CGQC_PlayerEntry.m_sPatch. Not a personal patch: only fills an empty slot
	static const ResourceName DEFAULT_PATCH = "{F20A13563AFED116}Prefabs/Rectangle_Patches/spartan_camo.et";

	//------------------------------------------------------------------------------------------------
	static void ApplyPatch(IEntity playerEntity, ResourceName patchPrefab)
	{
		if (!playerEntity || patchPrefab.IsEmpty() || !Replication.IsServer())
			return;

		EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(playerEntity.FindComponent(EquipedLoadoutStorageComponent));
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!loadoutStorage || !invManager)
			return;

		IEntity jacket = loadoutStorage.GetClothFromArea(LoadoutJacketArea);
		if (!jacket)
			return;

		BaseInventoryStorageComponent storage;
		InventoryStorageSlot slot;
		if (!FindSlot(jacket, PATCH_SLOT, storage, slot))
			return;

		IEntity currentPatch = slot.GetAttachedEntity();
		if (!currentPatch)
		{
			InsertPatch(invManager, storage, slot.GetID(), patchPrefab);
			return;
		}

		// A personal patch replaces whatever is there, the default one doesn't
		EntityPrefabData prefabData = currentPatch.GetPrefabData();
		if (!prefabData)
			return;

		ResourceName currentPrefab = prefabData.GetPrefabName();
		if (IsSamePrefab(currentPrefab, patchPrefab) || IsSamePrefab(patchPrefab, DEFAULT_PATCH))
			return;

		// Don't remove the old patch if the new one can't be loaded
		Resource patchResource = Resource.Load(patchPrefab);
		if (!patchResource || !patchResource.IsValid())
			return;

		CGQC_PatchInsertAfterDelete cb = new CGQC_PatchInsertAfterDelete();
		cb.m_InvManager = invManager;
		cb.m_Storage = storage;
		cb.m_iSlotId = slot.GetID();
		cb.m_sPatch = patchPrefab;
		invManager.TryDeleteItem(currentPatch, cb);
	}

	//------------------------------------------------------------------------------------------------
	static void InsertPatch(SCR_InventoryStorageManagerComponent invManager, BaseInventoryStorageComponent storage, int slotId, ResourceName patchPrefab)
	{
		if (!invManager || !storage)
			return;

		Resource patchResource = Resource.Load(patchPrefab);
		if (!patchResource || !patchResource.IsValid())
		{
			PrintFormat("[CGQC_PersonalPatch] Patch prefab not available, skipping: %1", patchPrefab);
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		storage.GetOwner().GetTransform(spawnParams.Transform);

		IEntity patch = GetGame().SpawnEntityPrefab(patchResource, GetGame().GetWorld(), spawnParams);
		if (!patch)
			return;

		CGQC_PatchInsert cb = new CGQC_PatchInsert();
		cb.m_Patch = patch;
		cb.m_sPatch = patchPrefab;

		if (!invManager.TryInsertItemInStorage(patch, storage, slotId, cb))
			cb.Cleanup();
	}

	//------------------------------------------------------------------------------------------------
	//! Finds a named slot on any of the item's storages
	protected static bool FindSlot(IEntity item, string slotName, out BaseInventoryStorageComponent outStorage, out InventoryStorageSlot outSlot)
	{
		array<Managed> storages = {};
		item.FindComponents(BaseInventoryStorageComponent, storages);

		foreach (Managed component : storages)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage)
				continue;

			for (int i = 0, count = storage.GetSlotsCount(); i < count; i++)
			{
				InventoryStorageSlot slot = storage.GetSlot(i);
				if (!slot || slot.GetSourceName() != slotName)
					continue;

				outStorage = storage;
				outSlot = slot;
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Compares the "{GUID}" part only, since paths in configs can be outdated
	protected static bool IsSamePrefab(string prefabA, string prefabB)
	{
		if (prefabA.Length() < 18 || prefabB.Length() < 18)
			return false;

		return prefabA.Substring(0, 18) == prefabB.Substring(0, 18);
	}
}

//------------------------------------------------------------------------------------------------
//! Deletes the spawned patch if it could not be inserted, so it isn't left on the ground
class CGQC_PatchInsert : ScriptedInventoryOperationCallback
{
	IEntity m_Patch;
	ResourceName m_sPatch;

	override void OnComplete()
	{
		PrintFormat("[CGQC_PersonalPatch] Applied: %1", m_sPatch);
	}

	override void OnFailed()
	{
		Cleanup();
	}

	void Cleanup()
	{
		if (!m_Patch)
			return;

		PrintFormat("[CGQC_PersonalPatch] Could not insert: %1", m_sPatch);
		SCR_EntityHelper.DeleteEntityAndChildren(m_Patch);
		m_Patch = null;
	}
}

//------------------------------------------------------------------------------------------------
//! Inserts the personal patch once the previous one has been removed
class CGQC_PatchInsertAfterDelete : ScriptedInventoryOperationCallback
{
	SCR_InventoryStorageManagerComponent m_InvManager;
	BaseInventoryStorageComponent m_Storage;
	int m_iSlotId;
	ResourceName m_sPatch;

	override void OnComplete()
	{
		CGQC_PersonalPatch.InsertPatch(m_InvManager, m_Storage, m_iSlotId, m_sPatch);
	}

	override void OnFailed()
	{
		PrintFormat("[CGQC_PersonalPatch] Could not remove current patch for: %1", m_sPatch);
	}
}
