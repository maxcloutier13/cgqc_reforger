[BaseContainerProps()]
modded class ACE_ConsumableFacepaint : SCR_ConsumableEffectBase
{
    protected static const ResourceName CGQC_HEAD_BASE   = "{1B1F4BCCC3A54549}Prefabs/Characters/Heads/Head_Cloutier.et";
    protected static const ResourceName CGQC_HEAD_CAMO_1 = "{4966111F3904A8FC}Prefabs/Characters/Heads/Head_Cloutier_camo_01.et";
    protected static const ResourceName CGQC_HEAD_CAMO_2 = "{253225FF85C027D2}Prefabs/Characters/Heads/Head_Cloutier_camo_02.et";

    override void ApplyEffect(notnull IEntity target, notnull IEntity user, IEntity item, ItemUseParameters animParams)
    {
        ChimeraCharacter char = ChimeraCharacter.Cast(target);
        if (char)
        {
            SCR_CharacterIdentityComponent identityComponent = SCR_CharacterIdentityComponent.Cast(char.FindComponent(SCR_CharacterIdentityComponent));
            if (identityComponent)
            {
                VisualIdentity visualIdentity = identityComponent.GetIdentity().GetVisualIdentity();
                if (visualIdentity)
                {
                    ResourceName currentHead = visualIdentity.GetHead();

                    if (currentHead == CGQC_HEAD_BASE)
                    {
                        ResourceName camoHead;
                        if (Math.RandomInt(0, 2) == 0)
                            camoHead = CGQC_HEAD_CAMO_1;
                        else
                            camoHead = CGQC_HEAD_CAMO_2;
                        visualIdentity.SetHead(camoHead);
                        identityComponent.CommitChanges();
                        identityComponent.SetIdentity(identityComponent.GetIdentity());
                        super.ApplyEffect(target, user, item, animParams);
                        return;
                    }
                    else if (currentHead == CGQC_HEAD_CAMO_1 || currentHead == CGQC_HEAD_CAMO_2)
                    {
                        visualIdentity.SetHead(CGQC_HEAD_BASE);
                        identityComponent.CommitChanges();
                        identityComponent.SetIdentity(identityComponent.GetIdentity());
                        super.ApplyEffect(target, user, item, animParams);
                        return;
                    }
                }
            }
        }

        super.ApplyEffect(target, user, item, animParams);
    }
}