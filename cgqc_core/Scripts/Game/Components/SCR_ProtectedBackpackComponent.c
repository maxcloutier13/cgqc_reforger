[ComponentEditorProps(category: "GameScripted/Inventory", description: "Debug backpack protection")]
class CGQC_ProtectedBackpackComponentClass : ScriptComponentClass
{
}

class CGQC_ProtectedBackpackComponent : ScriptComponent
{
	protected bool m_bIsProtected = false;
	
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		Print(string.Format("[CGQC_ProtectedBackpack] Owner: %1", owner), LogLevel.NORMAL);
		
		// Get the InventoryItemComponent
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(
			owner.FindComponent(InventoryItemComponent)
		);
		
		if (!itemComp)
		{
			return;
		}
		
		// Check if the invoker exists
		if (!itemComp.m_OnParentSlotChangedInvoker)
		{
			return;
		}
				
		// Subscribe to the event
		itemComp.m_OnParentSlotChangedInvoker.Insert(OnParentSlotChanged);
		
		// Also check current slot state
		InventoryStorageSlot currentSlot = itemComp.GetParentSlot();
		if (currentSlot)
		{
			Print("[CGQC_ProtectedBackpack] Ruck is currently IN a slot", LogLevel.NORMAL);
		}
		else
		{
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] Ruck is currently ON THE GROUND");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		CGQC_Log.Dbg("[CGQC_ProtectedBackpack] OnDelete called");
		
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(
			owner.FindComponent(InventoryItemComponent)
		);
		
		if (itemComp && itemComp.m_OnParentSlotChangedInvoker)
		{
			itemComp.m_OnParentSlotChangedInvoker.Remove(OnParentSlotChanged);
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] Unsubscribed from events");
		}
		
		super.OnDelete(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
	{
		
		// If newSlot is null, the item was dropped to the ground
		if (!newSlot && !m_bIsProtected)
		{
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] Ruck has been DROPPED to ground!");
			
			IEntity owner = GetOwner();
			if (owner)
			{
				ProtectFromGarbage(owner);
			}
		}
		else if (newSlot)
		{
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] Ruck was picked up or moved");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ProtectFromGarbage(IEntity owner)
	{
		CGQC_Log.Dbg("[CGQC_ProtectedBackpack] ProtectFromGarbage called");
		
		SCR_GarbageSystem garbageSystem = SCR_GarbageSystem.GetByEntityWorld(owner);
		if (!garbageSystem)
		{
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] ERROR: GarbageSystem not found!");
			return;
		}
		
		// Check if already inserted
		if (garbageSystem.IsInserted(owner))
		{
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] Item is ALREADY tracked by garbage system, withdrawing...");
			garbageSystem.Withdraw(owner);
		}
		else
		{
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] Item is NOT tracked by garbage system");
		}
		
		// Blacklist permanently
		bool success = garbageSystem.UpdateBlacklist(owner, true);
		
		if (success)
		{
			m_bIsProtected = true;
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] SUCCESS! Backpack protected from garbage collection");
		}
		else
		{
			CGQC_Log.Dbg("[CGQC_ProtectedBackpack] FAILED to blacklist backpack");
		}
	}
	
}