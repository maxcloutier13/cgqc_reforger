class CGQC_MedCore
{
	// Debug Logging
	static bool s_bDebugEnabled  = false;
	
	//------------------------------------------------------------------------------------------------
	static void Log(string msg)
	{
		if (!s_bDebugEnabled )
			return;

		Print(string.Format("[CGQC_Medical] %1", msg));
	}


	// All borrowable kit container prefabs. Must match CGQC_MedUtils constants.
	static ref array<ResourceName> MED_KIT_PREFABS = {
		"{CE262EF537F2E47A}Prefabs/Items/Equipment/Accessories/IFAK/IFAK.et",
		"{25569C2962C8F381}Prefabs/Items/Equipment/Accessories/IFAK/Trauma_IFAK_Tan.et",
		"{AE578EEA4244D41F}Prefabs/Items/Equipment/Kits/MedicalKit_01/MedicalKit_01_US.et"
	};
	
	static BaseInventoryStorageComponent FindNonKitStorage(SCR_InventoryStorageManagerComponent invMgr)
	{
	    array<IEntity> items = {};
	    invMgr.GetItems(items, EStoragePurpose.PURPOSE_ANY);
	
	    foreach (IEntity it : items)
	    {
	        if (!it)
	            continue;
	
	        if (CGQC_MedUtils.IsMedKitContainer(GetPrefabNameSafe(it)))
	            continue;
	
	        BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(
	            it.FindComponent(BaseInventoryStorageComponent)
	        );
	
	        if (storage)
	            return storage;
	    }
	
	    return null;
	}

	// Distance guard (meters). Keep in sync with CGQC_CasualtyComponent.
	static const float MAX_BORROW_DISTANCE = 5.0;

	//------------------------------------------------------------------------------------------------
	// Server-side: returns true only for real player-controlled characters.
	// Avoids handing kits to AI. Uses PlayerManager directly to handle early-possession
	// timing on dedicated servers (SCR_PlayerController cast can fail during that window).
	static bool IsPlayerCharacter_Server(IEntity actor)
	{
		if (!Replication.IsServer())
			return false;

		if (!actor)
			return false;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return false;

		int playerId = pm.GetPlayerIdFromControlledEntity(actor);
		if (playerId <= 0)
			return false;

		PlayerController pc = pm.GetPlayerController(playerId);
		if (!pc)
			return false;

		return (pc.GetControlledEntity() == actor);
	}

	//------------------------------------------------------------------------------------------------
	// Returns true if the item entity is one of the borrowable kit containers.
	static bool IsMedKitItem(IEntity item)
	{
		if (!item)
			return false;

		EntityPrefabData pd = item.GetPrefabData();
		if (!pd)
			return false;

		ResourceName prefab = pd.GetPrefabName();
		foreach (ResourceName rn : MED_KIT_PREFABS)
		{
			if (prefab == rn)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsActorDead(IEntity actor)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(actor);
		if (!ch)
			return false;

		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (!ctrl)
			return false;

		return (ctrl.GetLifeState() == ECharacterLifeState.DEAD);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCasualtyUnconscious(IEntity casualty)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(casualty);
		if (!ch)
			return false;

		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (!ctrl)
			return false;

		return (ctrl.GetLifeState() == ECharacterLifeState.INCAPACITATED);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsActorDeadOrUnconscious(IEntity actor)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(actor);
		if (!ch)
			return true;

		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (!ctrl)
			return true;

		ECharacterLifeState ls = ctrl.GetLifeState();
		return (ls == ECharacterLifeState.DEAD || ls == ECharacterLifeState.INCAPACITATED);
	}

	//------------------------------------------------------------------------------------------------
	static SCR_InventoryStorageManagerComponent GetInvMgr(IEntity ent)
	{
		if (!ent)
			return null;

		return SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName GetPrefabNameSafe(IEntity ent)
	{
		if (!ent)
			return "";

		EntityPrefabData pd = ent.GetPrefabData();
		if (!pd)
			return "";

		return pd.GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	// Returns the first borrowable med kit found in the character's inventory.
	static IEntity FindMedKitOnCharacter(IEntity character)
	{
		SCR_InventoryStorageManagerComponent invMgr = GetInvMgr(character);
		if (!invMgr)
			return null;

		array<IEntity> items = {};
		invMgr.GetItems(items, EStoragePurpose.PURPOSE_ANY);

		foreach (IEntity it : items)
		{
			if (!it)
				continue;

			if (IsMedKitItem(it))
				return it;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	// Returns true if the specific kit entity is currently in the character's inventory.
	static bool IsMedKitInInventory(IEntity character, IEntity kit)
	{
		if (!character || !kit)
			return false;

		SCR_InventoryStorageManagerComponent invMgr = GetInvMgr(character);
		if (!invMgr)
			return false;

		array<IEntity> items = {};
		invMgr.GetItems(items, EStoragePurpose.PURPOSE_ANY);

		foreach (IEntity it : items)
		{
			if (it == kit)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Returns the parent storage component of an item (e.g. the vest pocket storage that holds it).
	static BaseInventoryStorageComponent GetParentStorageOfItem(IEntity item)
	{
		if (!item)
			return null;

		InventoryItemComponent invItemComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!invItemComp)
			return null;

		InventoryStorageSlot parentSlot = invItemComp.GetParentSlot();
		if (!parentSlot)
			return null;

		return parentSlot.GetStorage();
	}

	//------------------------------------------------------------------------------------------------
	// Snapshot the ResourceNames of all children inside a kit container.
	// Used to duplicate kit contents during borrow and to diff provider consumption on return.
	static void SnapshotKitContents(IEntity kit, SCR_InventoryStorageManagerComponent invMgr, out array<ResourceName> outPrefabs)
	{
		outPrefabs.Clear();

		if (!kit || !invMgr)
			return;

		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(kit.FindComponent(BaseInventoryStorageComponent));
		if (!storage)
		{
			Log("SnapshotKitContents: kit has no BaseInventoryStorageComponent.");
			return;
		}

		array<IEntity> children = {};
		invMgr.GetAllItems(children, storage);

		foreach (IEntity it : children)
		{
			if (!it || it == kit)
				continue;

			ResourceName rn = GetPrefabNameSafe(it);
			if (rn != "")
				outPrefabs.Insert(rn);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Deletes all children inside a kit container (used to wipe default contents of a freshly spawned copy).
	static void ClearKitContents(IEntity kit, SCR_InventoryStorageManagerComponent invMgr)
	{
		if (!kit || !invMgr)
			return;

		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(kit.FindComponent(BaseInventoryStorageComponent));
		if (!storage)
		{
			Log("ClearKitContents: kit has no BaseInventoryStorageComponent.");
			return;
		}

		array<IEntity> children = {};
		invMgr.GetAllItems(children, storage);

		int deleted = 0;

		foreach (IEntity it : children)
		{
			if (!it || it == kit)
				continue;

			if (invMgr.TryDeleteItem(it))
				deleted++;
		}

		Log(string.Format("ClearKitContents: deleted %1 items.", deleted));
	}

	//------------------------------------------------------------------------------------------------
	// Spawns a list of prefabs into a kit container (used to restore a snapshot into a fresh copy).
	static void FillKitContents(IEntity kit, SCR_InventoryStorageManagerComponent invMgr, array<ResourceName> prefabs)
	{
		if (!kit || !invMgr)
			return;

		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(kit.FindComponent(BaseInventoryStorageComponent));
		if (!storage)
		{
			Log("FillKitContents: kit has no BaseInventoryStorageComponent.");
			return;
		}

		foreach (ResourceName rn : prefabs)
		{
			if (rn == "")
				continue;

			invMgr.TrySpawnPrefabToStorage(rn, storage, -1, EStoragePurpose.PURPOSE_ANY);
		}
	}

	//------------------------------------------------------------------------------------------------
	// After spawning a new kit into an inventory, finds the newly added entity by comparing before/after.
	static IEntity FindNewlySpawnedKit(SCR_InventoryStorageManagerComponent invMgr, array<IEntity> beforeItems, ResourceName kitPrefab)
	{
		array<IEntity> afterItems = {};
		invMgr.GetItems(afterItems, EStoragePurpose.PURPOSE_ANY);

		foreach (IEntity it : afterItems)
		{
			if (!it)
				continue;

			if (GetPrefabNameSafe(it) != kitPrefab)
				continue;

			bool existedBefore = false;
			foreach (IEntity b : beforeItems)
			{
				if (b == it)
				{
					existedBefore = true;
					break;
				}
			}

			if (!existedBefore)
				return it;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	// Executes a borrow: snapshots the casualty's kit, spawns an identical copy in the borrower's
	// inventory (same prefab, same contents), then deletes the original from the casualty.
	// Server-only.
	static bool ExecuteBorrow(IEntity casualty, IEntity borrower, IEntity casualtyKit, out IEntity outBorrowedKit)
	{
		outBorrowedKit = null;

		if (!Replication.IsServer())
			return false;

		if (!casualty || !borrower || !casualtyKit)
			return false;

		SCR_InventoryStorageManagerComponent casInv = GetInvMgr(casualty);
		SCR_InventoryStorageManagerComponent borInv = GetInvMgr(borrower);
		if (!casInv || !borInv)
			return false;

		ResourceName kitPrefab = GetPrefabNameSafe(casualtyKit);
		if (kitPrefab == "")
			return false;

		// 1. Snapshot existing contents
		array<ResourceName> snapshot = {};
		SnapshotKitContents(casualtyKit, casInv, snapshot);
		Log(string.Format("ExecuteBorrow: snapped %1 items from casualty kit.", snapshot.Count()));

		// 2. Spawn a fresh copy of the same kit prefab into borrower inventory
		array<IEntity> before = {};
		borInv.GetItems(before, EStoragePurpose.PURPOSE_ANY);
		
		//bool spawned = borInv.TrySpawnPrefabToStorage(kitPrefab, null, -1, EStoragePurpose.PURPOSE_ANY);
		BaseInventoryStorageComponent targetStorage = FindNonKitStorage(borInv);
		bool spawned = borInv.TrySpawnPrefabToStorage(kitPrefab, targetStorage, -1, EStoragePurpose.PURPOSE_ANY);

		if (!spawned)
		{
			array<IEntity> dbgItems = {};
		    borInv.GetItems(dbgItems, EStoragePurpose.PURPOSE_ANY);
		    CGQC_MedCore.Log(string.Format("ExecuteBorrow: GetItems returned %1 items after spawn.", dbgItems.Count()));
		    
		    foreach (IEntity dbgIt : dbgItems)
		    {
		        if (!dbgIt) continue;
		        CGQC_MedCore.Log(string.Format("  - %1", GetPrefabNameSafe(dbgIt)));
		    }
			Log("ExecuteBorrow: failed to spawn borrowed kit into borrower inventory.");
			return false;
		}

		IEntity borrowedKit = FindNewlySpawnedKit(borInv, before, kitPrefab);
		if (!borrowedKit)
		{
		    array<IEntity> dbgItems = {};
		    borInv.GetItems(dbgItems, EStoragePurpose.PURPOSE_ANY);
		    Log(string.Format("ExecuteBorrow: GetItems sees %1 items after spawn.", dbgItems.Count()));
		    foreach (IEntity dbgIt : dbgItems)
		    {
		        if (!dbgIt)
		            continue;
		        Log(string.Format("  - %1", GetPrefabNameSafe(dbgIt)));
		    }
		
		    Log("ExecuteBorrow: could not locate newly spawned kit in borrower inventory.");
		    return false;
		}

		// 3. Clear default fill, restore casualty's snapshot
		ClearKitContents(borrowedKit, borInv);
		FillKitContents(borrowedKit, borInv, snapshot);

		// 4. Remove the original from the casualty
		if (!casInv.TryDeleteItem(casualtyKit))
			Log("ExecuteBorrow: failed to delete casualty's original kit.");

		outBorrowedKit = borrowedKit;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Executes a return: snapshots the borrowed kit's remaining contents, spawns an identical copy
	// back on the casualty, then deletes the borrowed kit from the borrower.
	// Server-only.
	static bool ExecuteReturn(IEntity borrower, IEntity casualty, IEntity borrowedKit)
	{
		if (!Replication.IsServer())
			return false;

		if (!casualty || !borrowedKit)
			return false;

		SCR_InventoryStorageManagerComponent casInv = GetInvMgr(casualty);
		if (!casInv)
			return false;

		ResourceName kitPrefab = GetPrefabNameSafe(borrowedKit);
		if (kitPrefab == "")
			return false;

		// 1. Snapshot what's left in the borrowed kit
		array<ResourceName> snapshot = {};
		SCR_InventoryStorageManagerComponent borInv = null;

		if (borrower)
			borInv = GetInvMgr(borrower);

		if (borInv)
			SnapshotKitContents(borrowedKit, borInv, snapshot);

		Log(string.Format("ExecuteReturn: snapped %1 remaining items from borrowed kit.", snapshot.Count()));

		// 2. Spawn a fresh copy of the same kit on the casualty
		array<IEntity> before = {};
		casInv.GetItems(before, EStoragePurpose.PURPOSE_ANY);

		//bool spawned = casInv.TrySpawnPrefabToStorage(kitPrefab, null, -1, EStoragePurpose.PURPOSE_ANY);
		
		BaseInventoryStorageComponent targetStorage = FindNonKitStorage(casInv);
		bool spawned = casInv.TrySpawnPrefabToStorage(kitPrefab, targetStorage, -1, EStoragePurpose.PURPOSE_ANY);
		if (!spawned)
		{
			Log("ExecuteReturn: failed to spawn kit back onto casualty.");
			return false;
		}

		IEntity returnedKit = FindNewlySpawnedKit(casInv, before, kitPrefab);
		if (!returnedKit)
		{
			Log("ExecuteReturn: could not locate newly spawned kit on casualty.");
			return false;
		}

		// 3. Clear default fill, restore snapshot
		ClearKitContents(returnedKit, casInv);
		FillKitContents(returnedKit, casInv, snapshot);

		// 4. Delete borrowed copy from borrower
		if (borInv)
		{
			if (!borInv.TryDeleteItem(borrowedKit))
				Log("ExecuteReturn: could not delete borrowed kit from borrower (may have been dropped).");
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// On death: moves all items out of the casualty's kit into the surrounding storage layer
	// (pants, vest) so they are easily lootable, then deletes the empty container.
	static void DumpKitContentsOnDeath(IEntity casualty)
	{
		if (!Replication.IsServer())
			return;

		if (!casualty)
			return;

		SCR_InventoryStorageManagerComponent casInv = GetInvMgr(casualty);
		if (!casInv)
			return;

		IEntity kit = FindMedKitOnCharacter(casualty);
		if (!kit)
			return;

		BaseInventoryStorageComponent kitStorage = BaseInventoryStorageComponent.Cast(kit.FindComponent(BaseInventoryStorageComponent));
		if (!kitStorage)
		{
			Log("DumpKitContentsOnDeath: kit has no BaseInventoryStorageComponent.");
			return;
		}

		BaseInventoryStorageComponent parentStorage = GetParentStorageOfItem(kit);

		array<IEntity> children = {};
		casInv.GetAllItems(children, kitStorage);

		int moved = 0;

		foreach (IEntity child : children)
		{
			if (!child || child == kit)
				continue;

			bool ok = false;

			if (parentStorage)
				ok = casInv.TryMoveItemToStorage(child, parentStorage, -1, null);

			if (!ok)
				ok = casInv.TryMoveItemToStorage(child, null, -1, null);

			if (ok)
				moved++;
		}

		Log(string.Format("DumpKitContentsOnDeath: moved %1 items out of kit.", moved));

		if (!casInv.TryDeleteItem(kit))
			Log("DumpKitContentsOnDeath: failed to delete kit container.");
	}
}