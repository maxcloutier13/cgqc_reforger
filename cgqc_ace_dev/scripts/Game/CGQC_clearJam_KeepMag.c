//------------------------------------------------------------------------------------------------
// Callback that fires after the mag is removed from the weapon,
// then immediately inserts it back into the character's deposit storage.
class CGQC_ReinsertMagCallback : ScriptedInventoryOperationCallback
{
    InventoryStorageManagerComponent m_pInventoryManager;

    //------------------------------------------------------------------------------------------------
    override protected void OnComplete()
    {
        if (!m_pInventoryManager)
            return;

        // GetItem() returns the RplId of the item that was just removed.
        // We need the actual IEntity - find it via RplComponent.
        RplId magId = GetItem();
        RplComponent rpl = RplComponent.Cast(Replication.FindItem(magId));
        if (!rpl)
            return;

        IEntity mag = rpl.GetEntity();
        if (!mag)
            return;

        // Put it back into deposit (vest/backpack/uniform), not back into the weapon.
        m_pInventoryManager.TryInsertItem(mag, EStoragePurpose.PURPOSE_DEPOSIT);
    }
}

//------------------------------------------------------------------------------------------------
modded class ACE_Overheating_RemoveMagState : ACE_FSM_IState<ACE_Overheating_WeaponAnimContext>
{
    //------------------------------------------------------------------------------------------------
    override void OnEnter(ACE_Overheating_WeaponAnimContext context)
    {
        context.m_pCharController.OverrideMaxSpeed(0.25);

        BaseMagazineComponent currentMagComponent = context.m_pWeapon.GetCurrentMagazine();
        if (currentMagComponent)
        {
            IEntity currentMag = currentMagComponent.GetOwner();
            InventoryItemComponent magInventory = InventoryItemComponent.Cast(currentMag.FindComponent(InventoryItemComponent));
            BaseInventoryStorageComponent magStorage = magInventory.GetParentSlot().GetStorage();

            // Build the callback that will re-insert the mag after removal
            CGQC_ReinsertMagCallback cb = new CGQC_ReinsertMagCallback();
            cb.m_pInventoryManager = context.m_pInventoryManager;

            bool result = context.m_pInventoryManager.TryRemoveItemFromStorage(currentMag, magStorage, cb);
            context.m_bIsStartRemovingMag = result;
        }
    }

    //------------------------------------------------------------------------------------------------
    override void OnExit(ACE_Overheating_WeaponAnimContext context)
    {
        context.m_pCharController.OverrideMaxSpeed(1);
    }

    //------------------------------------------------------------------------------------------------
    override void OnUpdate(ACE_Overheating_WeaponAnimContext context, float timeSlice)
    {
        context.m_pCharController.SetWeaponADS(false);

        if (context.m_bIsStartRemovingMag && context.IsReloading())
            context.m_bIsStartRemovingMag = false;
    }
}