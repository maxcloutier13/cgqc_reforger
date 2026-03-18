modded class RandomGearComponent : ScriptComponent
{
    override void RandomizeGear(IEntity owner)
    {
        if (IsConfigEmpty(m_TopsConfig)) m_TopsConfig = null;
        if (IsConfigEmpty(m_HeadConfig)) m_HeadConfig = null;
        if (IsConfigEmpty(m_FaceConfig)) m_FaceConfig = null;
        if (IsConfigEmpty(m_ArmorConfig)) m_ArmorConfig = null;
        if (IsConfigEmpty(m_VestsConfig)) m_VestsConfig = null;
        if (IsConfigEmpty(m_BackpacksConfig)) m_BackpacksConfig = null;
        if (IsConfigEmpty(m_PantsConfig)) m_PantsConfig = null;
        if (IsConfigEmpty(m_BootsConfig)) m_BootsConfig = null;
        if (IsConfigEmpty(m_GlovesConfig)) m_GlovesConfig = null;

        super.RandomizeGear(owner);
    }

    bool IsConfigEmpty(SCR_GearPoolConfig cfg)
    {
        return cfg && cfg.m_aGearPool.IsEmpty() && cfg.m_aAdvancedPool.IsEmpty() && cfg.m_aGlobalItems.IsEmpty();
    }
}