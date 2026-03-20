class CGQC_ArsenalGrabAction : ScriptedUserAction
{
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        if (!Replication.IsServer())
            return;

        SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(pUserEntity);
        if (!character)
            return;

        InventoryStorageManagerComponent invManager = InventoryStorageManagerComponent.Cast(
            character.FindComponent(InventoryStorageManagerComponent));
        if (!invManager)
            return;

        EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(
            character.FindComponent(EquipedLoadoutStorageComponent));
        if (!loadout)
            return;

        RplComponent rpl = RplComponent.Cast(pUserEntity.FindComponent(RplComponent));
        if (!rpl)
            return;
        RplId playerId = rpl.Id();

        CGQC_ArsenalStashManagerComponent stashManager = CGQC_ArsenalStashManagerComponent.Cast(
            pOwnerEntity.FindComponent(CGQC_ArsenalStashManagerComponent));
        if (!stashManager || !stashManager.HasItems(playerId))
            return;

        int inserted = 0;
        int dropped  = 0;

        foreach (CGQC_ArsenalStashEntry entry : stashManager.GetStash(playerId))
        {
            // Try correct clothing slot first
            IEntity clothEntity = loadout.GetClothFromArea(entry.m_tAreaType);
            if (clothEntity)
            {
                BaseInventoryStorageComponent clothStorage = BaseInventoryStorageComponent.Cast(
                    clothEntity.FindComponent(BaseInventoryStorageComponent));
                if (clothStorage && invManager.TrySpawnPrefabToStorage(entry.m_sPrefab, clothStorage))
                {
                    inserted++;
                    continue;
                }
            }

            // Fallback — try anywhere in player inventory
            if (invManager.TrySpawnPrefabToStorage(entry.m_sPrefab))
            {
                inserted++;
                continue;
            }

            // Doesn't fit — spawn on the ground near player
            Resource res = Resource.Load(entry.m_sPrefab);
            if (res.IsValid())
            {
                EntitySpawnParams spawnParams = new EntitySpawnParams();
                spawnParams.TransformMode = ETransformMode.WORLD;
                Math3D.MatrixIdentity4(spawnParams.Transform);
                vector dropPos = pUserEntity.GetOrigin() + Vector(Math.RandomFloat(-0.5, 0.5), 0.1, Math.RandomFloat(-0.5, 0.5));
                spawnParams.Transform[3] = dropPos;
                GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), spawnParams);
            }
            dropped++;
        }

        stashManager.Clear(playerId);

        string msg = string.Format("%1 item(s) retrieved.", inserted);
        if (dropped > 0)
            msg = msg + string.Format(" %1 item(s) dropped nearby (no space).", dropped);

        SCR_HintManagerComponent.GetInstance().ShowCustomHint(msg, "Arsenal", 4.0);
    }

    override bool CanBeShownScript(IEntity user)
    {
        RplComponent rpl = RplComponent.Cast(user.FindComponent(RplComponent));
        if (!rpl)
            return false;

        CGQC_ArsenalStashManagerComponent stashManager = CGQC_ArsenalStashManagerComponent.Cast(
            GetOwner().FindComponent(CGQC_ArsenalStashManagerComponent));
        if (!stashManager)
            return false;

        return stashManager.HasItems(rpl.Id());
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
        outName = "Grab your items";
        return true;
    }
}