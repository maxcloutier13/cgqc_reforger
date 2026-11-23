class CGQC_MedCore
{
	// Toggle debug prints (default OFF for server efficiency)
	static bool s_bDebugEnabled = false;

	// All borrowable kit container prefabs. Must match CGQC_MedUtils constants.
	static ref array<ResourceName> MED_KIT_PREFABS = {
		"{CE262EF537F2E47A}Prefabs/Items/Equipment/Accessories/IFAK/IFAK.et",
		"{25569C2962C8F381}Prefabs/Items/Equipment/Accessories/IFAK/Trauma_IFAK_Tan.et",
		"{AE578EEA4244D41F}Prefabs/Items/Equipment/Kits/MedicalKit_01/MedicalKit_01_US.et"
	};

	// Distance guard in meters. Keep in sync with CGQC_CasualtyComponent.
	static const float MAX_BORROW_DISTANCE = 5.0;

	//------------------------------------------------------------------------------------------------
	static void Log(string msg)
	{
		if (!s_bDebugEnabled)
			return;

		Print(string.Format("[CGQC_Medical] %1", msg));
	}

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
	// Returns the parent storage component of an item (e.g. the vest attachment storage that holds it).
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
	// Used by reconciliation to diff provider consumption during the borrow window.
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
	// Moves the casualty's actual kit entity into the borrower's inventory using InsertItem.
	// Captures the kit's original storage on the casualty before moving, so it can be
	// returned to the exact same slot later.
	// Server-only.
	static bool ExecuteBorrow(IEntity casualty, IEntity borrower, IEntity casualtyKit, out IEntity outBorrowedKit, out BaseInventoryStorageComponent outOriginalStorage)
	{
		outBorrowedKit = null;
		outOriginalStorage = null;

		if (!Replication.IsServer())
			return false;

		if (!casualty || !borrower || !casualtyKit)
			return false;

		SCR_InventoryStorageManagerComponent borInv = GetInvMgr(borrower);
		if (!borInv)
			return false;

		// Capture the casualty's original storage slot BEFORE moving.
		// Used on return to push the kit back to the exact same slot,
		// bypassing inventory state checks on the unconscious casualty.
		outOriginalStorage = GetParentStorageOfItem(casualtyKit);

		Log("ExecuteBorrow: inserting kit into borrower inventory.");

		// InsertItem uses multiple fallback strategies and handles clothing-type items
		// (BaseLoadoutClothComponent) more reliably than TrySpawnPrefabToStorage.
		borInv.InsertItem(casualtyKit);

		if (!IsMedKitInInventory(borrower, casualtyKit))
		{
			Log("ExecuteBorrow: InsertItem failed - kit not found in borrower inventory after insert.");
			return false;
		}

		outBorrowedKit = casualtyKit;
		Log("ExecuteBorrow: kit inserted successfully.");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Moves the borrowed kit entity back to the casualty using the borrower's inventory manager,
	// targeting the casualty's original storage slot directly.
	// Using the borrower's (active) inv manager bypasses inventory state restrictions
	// on the unconscious casualty.
	// Server-only.
	static bool ExecuteReturn(IEntity borrower, IEntity casualty, IEntity borrowedKit, BaseInventoryStorageComponent originalStorage)
	{
		if (!Replication.IsServer())
			return false;

		if (!casualty || !borrowedKit)
			return false;

		SCR_InventoryStorageManagerComponent borInv = GetInvMgr(borrower);
		if (!borInv)
			return false;

		Log("ExecuteReturn: pushing kit back to casualty original storage.");

		// Push from borrower's active inv manager to the casualty's original slot.
		// Targeting the specific original storage bypasses the unconscious inv state check.
		bool moved = borInv.TryMoveItemToStorage(borrowedKit, originalStorage, -1, null);
		if (!moved)
		{
			// Fallback: try without targeting a specific storage, in case original slot is gone.
			Log("ExecuteReturn: original storage failed, trying casualty inv manager fallback.");
			SCR_InventoryStorageManagerComponent casInv = GetInvMgr(casualty);
			if (casInv)
			{
				casInv.InsertItem(borrowedKit);
				moved = IsMedKitInInventory(casualty, borrowedKit);
			}
		}

		if (!moved)
		{
			Log("ExecuteReturn: all return strategies failed.");
			return false;
		}

		Log("ExecuteReturn: kit returned successfully.");
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