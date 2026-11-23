// -----------------------------------------------------------------------
// CGQC_BorrowRecord
// Tracks a single active borrow: which kit was borrowed, who borrowed it,
// the casualty's original storage slot for accurate return,
// and a snapshot of the provider's own kit at borrow-time for reconciliation.
// -----------------------------------------------------------------------
class CGQC_BorrowRecord
{
	IEntity m_BorrowedKit;
	IEntity m_Borrower;

	// The storage slot on the casualty the kit came from.
	// Used to push the kit back to the exact original slot on return,
	// bypassing inventory state restrictions on the unconscious casualty.
	BaseInventoryStorageComponent m_CasualtyOriginalStorage;

	// Provider's original kit and its contents at borrow-time.
	// Used to detect which items the provider consumed from their own kit
	// during the borrow window and reimburse them from the casualty's kit on return.
	IEntity m_ProviderKit;
	ref array<ResourceName> m_ProviderStartSnapshot;
};

// -----------------------------------------------------------------------
class CGQC_CasualtyComponentClass : ScriptComponentClass
{
};

// -----------------------------------------------------------------------
// Added to every character prefab that can be a borrow casualty.
// Manages the borrowable state flag (replicated), the active borrow list,
// and a throttled think loop that enforces return conditions.
// -----------------------------------------------------------------------
class CGQC_CasualtyComponent : ScriptComponent
{
	// Think loop intervals
	protected const int THINK_IDLE_MS		= 1000;  // No active borrows
	protected const int THINK_ACTIVE_MS		= 250;   // Active borrows: range / state guard
	protected const int RESYNC_EVERY_MS		= 5000;  // Idle borrowability resync interval

	protected int m_iResyncCountdownMs;

	protected ref array<ref CGQC_BorrowRecord> m_aBorrows = new array<ref CGQC_BorrowRecord>();

	protected bool m_bInitialized;
	protected bool m_bLastUnconsciousState;
	protected bool m_bDeathProcessed;

	// Replicated: clients read this to show/hide the borrow action.
	[RplProp()]
	protected bool m_bBorrowable = true;

	// Replicated: player ID of the current borrower (-1 = no active borrow).
	// Clients use this to show/hide the manual return action for the correct player.
	[RplProp()]
	protected int m_iBorrowerPlayerId = -1;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!Replication.IsServer())
			return;

		m_bInitialized			= true;
		m_bLastUnconsciousState	= CGQC_MedCore.IsCasualtyUnconscious(owner);
		m_bDeathProcessed		= false;

		// Hide action immediately if character has no kit.
		m_bBorrowable = (CGQC_MedCore.FindMedKitOnCharacter(owner) != null);
		Replication.BumpMe();

		m_iResyncCountdownMs = RESYNC_EVERY_MS;

		ScheduleThink(owner, THINK_IDLE_MS);
	}

	//------------------------------------------------------------------------------------------------
	protected void ScheduleThink(IEntity owner, int delayMs)
	{
		if (!owner)
			return;

		GetGame().GetCallqueue().CallLater(ThinkTick, delayMs, false, owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void ThinkTick(IEntity casualty)
	{
		if (!Replication.IsServer() || !m_bInitialized || !casualty)
			return;

		CGQC_MedCore.Log(string.Format("ThinkTick: borrows=%1 dead=%2 uncon=%3",
			m_aBorrows.Count(),
			CGQC_MedCore.IsActorDead(casualty),
			CGQC_MedCore.IsCasualtyUnconscious(casualty)
		));

		// ---- Death flow ----
		if (CGQC_MedCore.IsActorDead(casualty))
		{
			if (!m_bDeathProcessed)
			{
				CGQC_MedCore.Log("Casualty died: returning borrowed kits then dumping contents.");

				if (!m_aBorrows.IsEmpty())
					ReturnAllBorrows();

				CGQC_MedCore.DumpKitContentsOnDeath(casualty);
				Server_SetBorrowable(false);
				Server_SetBorrowerPlayerId(-1);

				m_bDeathProcessed = true;
			}

			// Component becomes inert after death; do not reschedule.
			return;
		}

		// ---- Idle (no active borrows) ----
		if (m_aBorrows.IsEmpty())
		{
			m_iResyncCountdownMs -= THINK_IDLE_MS;
			if (m_iResyncCountdownMs <= 0)
			{
				bool hasKit = (CGQC_MedCore.FindMedKitOnCharacter(casualty) != null);
				Server_SetBorrowable(hasKit);
				m_iResyncCountdownMs = RESYNC_EVERY_MS;
			}

			ScheduleThink(casualty, THINK_IDLE_MS);
			return;
		}

		bool nowUnconscious = CGQC_MedCore.IsCasualtyUnconscious(casualty);

		// ---- Casualty regained consciousness: return everything ----
		if (m_bLastUnconsciousState && !nowUnconscious)
		{
			CGQC_MedCore.Log("Casualty conscious: returning all borrowed kits.");
			ReturnAllBorrows();

			bool hasKit = (CGQC_MedCore.FindMedKitOnCharacter(casualty) != null);
			Server_SetBorrowable(hasKit);
			Server_SetBorrowerPlayerId(-1);

			m_bLastUnconsciousState = nowUnconscious;
			ScheduleThink(casualty, THINK_IDLE_MS);
			return;
		}

		m_bLastUnconsciousState = nowUnconscious;

		// Not unconscious with no borrows to enforce (should not occur, but safe)
		if (!nowUnconscious)
		{
			ScheduleThink(casualty, THINK_IDLE_MS);
			return;
		}

		// ---- Unconscious with active borrows: enforce guards ----
		EnforceGuards(casualty);

		ScheduleThink(casualty, THINK_ACTIVE_MS);
	}

	//------------------------------------------------------------------------------------------------
	// Read by the borrow action on clients (via [RplProp] replication).
	bool IsBorrowable()
	{
		return m_bBorrowable;
	}

	//------------------------------------------------------------------------------------------------
	void Server_SetBorrowable(bool state)
	{
		if (!Replication.IsServer())
			return;

		if (m_bBorrowable == state)
			return;

		m_bBorrowable = state;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void Server_SetBorrowerPlayerId(int playerId)
	{
		if (!Replication.IsServer())
			return;

		if (m_iBorrowerPlayerId == playerId)
			return;

		m_iBorrowerPlayerId = playerId;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	// Returns true if the given entity is the current active borrower.
	// Works on both server and client since m_iBorrowerPlayerId is replicated.
	bool HasActiveBorrowByEntity(IEntity borrower)
	{
		if (!borrower)
			return false;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return false;

		int userId = pm.GetPlayerIdFromControlledEntity(borrower);
		return userId > 0 && userId == m_iBorrowerPlayerId;
	}

	//------------------------------------------------------------------------------------------------
	// Called by CGQC_BorrowAction after a successful ExecuteBorrow.
	// Stores the original storage slot and snapshots the provider's own kit
	// at this moment as the reconciliation baseline.
	void RegisterBorrow(IEntity borrowedKit, IEntity borrower, BaseInventoryStorageComponent originalStorage)
	{
		if (!Replication.IsServer() || !borrowedKit)
			return;

		ref CGQC_BorrowRecord rec = new CGQC_BorrowRecord();
		rec.m_BorrowedKit				= borrowedKit;
		rec.m_Borrower					= borrower;
		rec.m_CasualtyOriginalStorage	= originalStorage;
		rec.m_ProviderKit				= null;
		rec.m_ProviderStartSnapshot		= new array<ResourceName>();

		if (borrower)
		{
			SCR_InventoryStorageManagerComponent borInv = CGQC_MedCore.GetInvMgr(borrower);
			if (borInv)
			{
				// The provider now has the borrowed kit in their inventory.
				// Find their ORIGINAL kit (not the borrowed one) for reconciliation.
				array<IEntity> allItems = {};
				borInv.GetItems(allItems, EStoragePurpose.PURPOSE_ANY);

				IEntity providerKit = null;
				foreach (IEntity it : allItems)
				{
					if (!it)
						continue;

					if (!CGQC_MedCore.IsMedKitItem(it))
						continue;

					if (it == borrowedKit)
						continue;

					providerKit = it;
					break;
				}

				rec.m_ProviderKit = providerKit;

				if (providerKit)
					CGQC_MedCore.SnapshotKitContents(providerKit, borInv, rec.m_ProviderStartSnapshot);
			}

			// Replicate borrower player ID so clients can show/hide the return action.
			PlayerManager pm = GetGame().GetPlayerManager();
			if (pm)
				Server_SetBorrowerPlayerId(pm.GetPlayerIdFromControlledEntity(borrower));
		}

		m_aBorrows.Insert(rec);

		// Kit is out – mark as not borrowable until it comes back.
		Server_SetBorrowable(false);

		CGQC_MedCore.Log("RegisterBorrow: borrow registered.");
	}

	//------------------------------------------------------------------------------------------------
	// Returns all active borrows unconditionally (consciousness regained, death, etc.)
	void ReturnAllBorrows()
	{
		IEntity casualty = GetOwner();
		if (!casualty)
			return;

		for (int i = m_aBorrows.Count() - 1; i >= 0; i--)
		{
			CGQC_BorrowRecord rec = m_aBorrows[i];
			if (!rec)
			{
				m_aBorrows.Remove(i);
				continue;
			}

			ReturnBorrow(rec, casualty);
			m_aBorrows.Remove(i);
		}

		Server_SetBorrowerPlayerId(-1);
	}

	//------------------------------------------------------------------------------------------------
	// Range, state, and inventory guard loop. Runs every THINK_ACTIVE_MS while borrows are active.
	protected void EnforceGuards(IEntity casualty)
	{
		CGQC_MedCore.Log(string.Format("EnforceGuards: threshold=%1", CGQC_MedCore.MAX_BORROW_DISTANCE));

		for (int i = m_aBorrows.Count() - 1; i >= 0; i--)
		{
			CGQC_BorrowRecord rec = m_aBorrows[i];
			if (!rec)
			{
				m_aBorrows.Remove(i);
				continue;
			}

			IEntity borrowedKit = rec.m_BorrowedKit;
			IEntity borrower	= rec.m_Borrower;

			// Borrower disconnected / gone
			if (!borrower)
			{
				CGQC_MedCore.Log("Borrower missing/disconnected -> returning kit.");
				ReturnBorrow(rec, casualty);
				m_aBorrows.Remove(i);
				Server_SetBorrowerPlayerId(-1);
				continue;
			}

			// Borrower went down or died
			if (CGQC_MedCore.IsActorDeadOrUnconscious(borrower))
			{
				CGQC_MedCore.Log("Borrower dead/unconscious -> returning kit.");
				ReturnBorrow(rec, casualty);
				m_aBorrows.Remove(i);
				Server_SetBorrowerPlayerId(-1);
				continue;
			}

			// Range guard
			vector delta = borrower.GetOrigin() - casualty.GetOrigin();
			float dist = Math.Sqrt(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
			if (dist > CGQC_MedCore.MAX_BORROW_DISTANCE)
			{
				CGQC_MedCore.Log(string.Format("Borrower too far (%1 m) -> returning kit.", dist));
				ReturnBorrow(rec, casualty);
				m_aBorrows.Remove(i);
				Server_SetBorrowerPlayerId(-1);
				continue;
			}

			// Kit left borrower inventory (dropped, traded, etc.)
			if (borrowedKit && !CGQC_MedCore.IsMedKitInInventory(borrower, borrowedKit))
			{
				CGQC_MedCore.Log("Borrowed kit no longer in borrower inventory -> returning kit.");
				ReturnBorrow(rec, casualty);
				m_aBorrows.Remove(i);
				Server_SetBorrowerPlayerId(-1);
				continue;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Reconcile + return for a single borrow record.
	protected void ReturnBorrow(CGQC_BorrowRecord rec, IEntity casualty)
	{
		if (!rec || !Replication.IsServer())
			return;

		// Reimburse the provider for any items consumed from their own kit during the borrow window.
		ReconcileConsumption(rec);

		IEntity borrower = rec.m_Borrower;

		// If borrower is gone, try to resolve current carrier from the kit itself.
		if (!borrower)
			borrower = GetCurrentCarrier(rec.m_BorrowedKit);

		if (!borrower)
			return;

		bool returned = CGQC_MedCore.ExecuteReturn(borrower, casualty, rec.m_BorrowedKit, rec.m_CasualtyOriginalStorage);
		CGQC_MedCore.Log(string.Format("ReturnBorrow: ExecuteReturn result=%1", returned));

		if (returned)
		{
			// Notify borrower via their own component
			CGQC_CasualtyComponent borrowerComp = CGQC_CasualtyComponent.Cast(
				borrower.FindComponent(CGQC_CasualtyComponent)
			);
			if (borrowerComp)
				borrowerComp.ShowNotification("IFAK remis au blessé.");

			// Notify casualty — this IS the casualty's component
			ShowNotification("Ton IFAK t'a été retourné.");
		}
	}

	//------------------------------------------------------------------------------------------------
	// Triggered by CGQC_ReturnAction. Finds and returns the borrow record belonging to the
	// requesting entity, then resyncs borrowability and clears the borrower ID.
	void RequestReturn(IEntity borrower)
	{
		if (!Replication.IsServer() || !borrower)
			return;

		IEntity casualty = GetOwner();
		if (!casualty)
			return;

		for (int i = m_aBorrows.Count() - 1; i >= 0; i--)
		{
			CGQC_BorrowRecord rec = m_aBorrows[i];
			if (!rec || rec.m_Borrower != borrower)
				continue;

			ReturnBorrow(rec, casualty);
			m_aBorrows.Remove(i);
			break;
		}

		bool hasKit = (CGQC_MedCore.FindMedKitOnCharacter(casualty) != null);
		Server_SetBorrowable(hasKit);
		Server_SetBorrowerPlayerId(-1);
	}

	//------------------------------------------------------------------------------------------------
	// Reimburses the provider for medical items consumed from their own kit while they had the
	// borrowed kit. For each consumed item (start snapshot - end snapshot), we look for a matching
	// prefab in the borrowed (casualty) kit and move it to the provider's kit.
	//
	// If the casualty's kit has no match, the provider simply eats the loss — which is correct:
	// you can't be reimbursed for something the casualty never had.
	protected void ReconcileConsumption(CGQC_BorrowRecord rec)
	{
		if (!Replication.IsServer() || !rec)
			return;

		IEntity borrower	= rec.m_Borrower;
		IEntity borrowedKit = rec.m_BorrowedKit;
		IEntity providerKit = rec.m_ProviderKit;

		if (!borrower || !borrowedKit || !providerKit)
			return;

		// Skip reconciliation if the borrowed kit is no longer in borrower inventory.
		if (!CGQC_MedCore.IsMedKitInInventory(borrower, borrowedKit))
			return;

		SCR_InventoryStorageManagerComponent borInv = CGQC_MedCore.GetInvMgr(borrower);
		if (!borInv)
			return;

		// Current state of provider's own kit
		ref array<ResourceName> providerEndSnapshot = new array<ResourceName>();
		CGQC_MedCore.SnapshotKitContents(providerKit, borInv, providerEndSnapshot);

		// Build unique prefab list from start snapshot, restricted to tracked med items
		ref array<ResourceName> uniquePrefabs = new array<ResourceName>();
		foreach (ResourceName rn : rec.m_ProviderStartSnapshot)
		{
			if (rn == "" || !CGQC_MedUtils.IsMedItem(rn))
				continue;

			bool already = false;
			foreach (ResourceName u : uniquePrefabs)
			{
				if (u == rn)
				{
					already = true;
					break;
				}
			}

			if (!already)
				uniquePrefabs.Insert(rn);
		}

		BaseInventoryStorageComponent borrowedStorage = GetKitStorage(borrowedKit);
		BaseInventoryStorageComponent providerStorage = GetKitStorage(providerKit);
		if (!borrowedStorage || !providerStorage)
			return;

		foreach (ResourceName rn : uniquePrefabs)
		{
			int startCount	= CountPrefab(rec.m_ProviderStartSnapshot, rn);
			int endCount	= CountPrefab(providerEndSnapshot, rn);
			int used		= startCount - endCount;

			if (used <= 0)
				continue;

			// PERF: scan borrowed kit once per prefab, then null consumed entries.
			array<IEntity> borrowedItems = {};
			borInv.GetAllItems(borrowedItems, borrowedStorage);

			for (int i = 0; i < used; i++)
			{
				IEntity match = null;

				for (int j = 0; j < borrowedItems.Count(); j++)
				{
					IEntity it = borrowedItems[j];
					if (!it || it == borrowedKit)
						continue;

					if (CGQC_MedUtils.GetPrefabName(it) != rn)
						continue;

					match = it;
					borrowedItems[j] = null;
					break;
				}

				if (!match)
					break; // No more matching items in borrowed kit – provider eats the loss.

				if (!borInv.TryMoveItemToStorage(match, providerStorage, -1, null))
					break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected BaseInventoryStorageComponent GetKitStorage(IEntity kit)
	{
		if (!kit)
			return null;

		return BaseInventoryStorageComponent.Cast(kit.FindComponent(BaseInventoryStorageComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected int CountPrefab(array<ResourceName> list, ResourceName rn)
	{
		if (!list)
			return 0;

		int c = 0;
		foreach (ResourceName it : list)
		{
			if (it == rn)
				c++;
		}

		return c;
	}

	//------------------------------------------------------------------------------------------------
	// Resolves the current inventory owner of an item via its parent storage slot.
	// Allows returning a kit even if the original borrower entity is gone.
	protected IEntity GetCurrentCarrier(IEntity item)
	{
		BaseInventoryStorageComponent parentStorage = CGQC_MedCore.GetParentStorageOfItem(item);
		if (!parentStorage)
			return null;

		return parentStorage.GetOwner();
	}

	//------------------------------------------------------------------------------------------------
	void ShowNotification(string message, float duration = 3.0)
	{
		Rpc(RpcDo_ShowNotification, message, duration);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_ShowNotification(string message, float duration)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
			return;

		popup.PopupMsg(message, duration);
	}
}