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
			Print("[CGQC_ProtectedBackpack] Ruck is currently ON THE GROUND", LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		Print("[CGQC_ProtectedBackpack] OnDelete called", LogLevel.NORMAL);
		
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(
			owner.FindComponent(InventoryItemComponent)
		);
		
		if (itemComp && itemComp.m_OnParentSlotChangedInvoker)
		{
			itemComp.m_OnParentSlotChangedInvoker.Remove(OnParentSlotChanged);
			Print("[CGQC_ProtectedBackpack] Unsubscribed from events", LogLevel.NORMAL);
		}
		
		super.OnDelete(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
	{
		
		// If newSlot is null, the item was dropped to the ground
		if (!newSlot && !m_bIsProtected)
		{
			Print("[CGQC_ProtectedBackpack] Ruck has been DROPPED to ground!", LogLevel.NORMAL);
			
			IEntity owner = GetOwner();
			if (owner)
			{
				ProtectFromGarbage(owner);
			}
		}
		else if (newSlot)
		{
			Print("[CGQC_ProtectedBackpack] Ruck was picked up or moved", LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ProtectFromGarbage(IEntity owner)
	{
		Print("[CGQC_ProtectedBackpack] ProtectFromGarbage called", LogLevel.NORMAL);
		
		SCR_GarbageSystem garbageSystem = SCR_GarbageSystem.GetByEntityWorld(owner);
		if (!garbageSystem)
		{
			Print("[CGQC_ProtectedBackpack] ERROR: GarbageSystem not found!", LogLevel.ERROR);
			return;
		}
		
		// Check if already inserted
		if (garbageSystem.IsInserted(owner))
		{
			Print("[CGQC_ProtectedBackpack] Item is ALREADY tracked by garbage system, withdrawing...", LogLevel.NORMAL);
			garbageSystem.Withdraw(owner);
		}
		else
		{
			Print("[CGQC_ProtectedBackpack] Item is NOT tracked by garbage system", LogLevel.NORMAL);
		}
		
		// Blacklist permanently
		bool success = garbageSystem.UpdateBlacklist(owner, true);
		
		if (success)
		{
			m_bIsProtected = true;
			Print("[CGQC_ProtectedBackpack] SUCCESS! Backpack protected from garbage collection", LogLevel.NORMAL);
		}
		else
		{
			Print("[CGQC_ProtectedBackpack] FAILED to blacklist backpack", LogLevel.ERROR);
		}
	}
}