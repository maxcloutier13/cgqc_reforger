class CGQC_ArsenalStashAction : ScriptedUserAction
{
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        if (!Replication.IsServer())
            return;

        SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(pUserEntity);
        if (!character)
            return;

        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout)
            return;

        CGQC_ArsenalStashManagerComponent stashManager = CGQC_ArsenalStashManagerComponent.Cast(
            pOwnerEntity.FindComponent(CGQC_ArsenalStashManagerComponent));
        if (!stashManager)
            return;

        RplComponent rpl = RplComponent.Cast(pUserEntity.FindComponent(RplComponent));
        if (!rpl)
            return;
        RplId playerId = rpl.Id();

        IEntity jacket   = loadout.GetClothFromArea(LoadoutJacketArea);
        IEntity pants    = loadout.GetClothFromArea(LoadoutPantsArea);
        IEntity vest     = loadout.GetClothFromArea(LoadoutVestArea);
        IEntity backpack = loadout.GetClothFromArea(LoadoutBackpackArea);

        array<IEntity> toDelete      = {};
        array<typename> toDeleteAreas = {};

        if (jacket)
        {
            BaseInventoryStorageComponent s = BaseInventoryStorageComponent.Cast(jacket.FindComponent(BaseInventoryStorageComponent));
            if (s)
            {
                array<IEntity> items = {};
                s.GetAll(items, true);
                foreach (IEntity item : items) { toDelete.Insert(item); toDeleteAreas.Insert(LoadoutJacketArea); }
            }
        }

        if (pants)
        {
            BaseInventoryStorageComponent s = BaseInventoryStorageComponent.Cast(pants.FindComponent(BaseInventoryStorageComponent));
            if (s)
            {
                array<IEntity> items = {};
                s.GetAll(items, true);
                foreach (IEntity item : items) { toDelete.Insert(item); toDeleteAreas.Insert(LoadoutPantsArea); }
            }
        }

        if (vest)
        {
            BaseInventoryStorageComponent s = BaseInventoryStorageComponent.Cast(vest.FindComponent(BaseInventoryStorageComponent));
            if (s)
            {
                array<IEntity> directItems = {};
                s.GetAll(directItems, false);
                foreach (IEntity directItem : directItems)
                {
                    BaseInventoryStorageComponent pouchStorage = BaseInventoryStorageComponent.Cast(
                        directItem.FindComponent(BaseInventoryStorageComponent));
                    if (pouchStorage)
                    {
                        array<IEntity> pouchContents = {};
                        pouchStorage.GetAll(pouchContents, true);
                        foreach (IEntity item : pouchContents) { toDelete.Insert(item); toDeleteAreas.Insert(LoadoutVestArea); }
                    }
                    else
                    {
                        toDelete.Insert(directItem);
                        toDeleteAreas.Insert(LoadoutVestArea);
                    }
                }
            }
        }

        if (backpack)
        {
            BaseInventoryStorageComponent s = BaseInventoryStorageComponent.Cast(backpack.FindComponent(BaseInventoryStorageComponent));
            if (s)
            {
                array<IEntity> items = {};
                s.GetAll(items, true);
                foreach (IEntity item : items) { toDelete.Insert(item); toDeleteAreas.Insert(LoadoutBackpackArea); }
            }
        }

        if (toDelete.IsEmpty())
        {
            SCR_HintManagerComponent.GetInstance().ShowCustomHint("No items to stash.", "Arsenal", 3.0);
            return;
        }

        array<ref CGQC_ArsenalStashEntry> stash = stashManager.GetStash(playerId);

        for (int i = 0; i < toDelete.Count(); i++)
        {
            stash.Insert(new CGQC_ArsenalStashEntry(
                toDelete[i].GetPrefabData().GetPrefabName(), toDeleteAreas[i]));
            SCR_EntityHelper.DeleteEntityAndChildren(toDelete[i]);
        }

        SCR_HintManagerComponent.GetInstance().ShowCustomHint(
            string.Format("%1 item(s) stashed. You can now change your uniform.", toDelete.Count()), "Arsenal", 4.0);
    }

    override bool CanBeShownScript(IEntity user)
    {
        RplComponent rpl = RplComponent.Cast(user.FindComponent(RplComponent));
        if (!rpl)
            return true;

        CGQC_ArsenalStashManagerComponent stashManager = CGQC_ArsenalStashManagerComponent.Cast(
            GetOwner().FindComponent(CGQC_ArsenalStashManagerComponent));
        if (!stashManager)
            return true;

        return !stashManager.HasItems(rpl.Id());
    }

    override bool CanBePerformedScript(IEntity user)
    {
        return true;
    }

    override bool HasLocalEffectOnlyScript()
    {
        return false;
    }

    override bool GetActionNameScript(out string outName)
    {
        outName = "Stash your items";
        return true;
    }
}