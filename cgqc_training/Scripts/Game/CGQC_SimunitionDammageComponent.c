// CGQC_SimuDamageManagerComponent.c

modded class SCR_CharacterDamageManagerComponent
{
    protected static const string CGQC_SIMU_AMMO_TAG = "Simunition";
    protected static const float CGQC_KO_DURATION = 30.0;

    //------------------------------------------------------------------------------------------------
    override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
    {
        if (!Replication.IsServer())
            return super.HijackDamageHandling(damageContext);

        if (damageContext.damageSource)
        {
            EntityPrefabData prefabData = damageContext.damageSource.GetPrefabData();
            if (prefabData && prefabData.GetPrefabName().Contains(CGQC_SIMU_AMMO_TAG))
            {
                // Zero damage
                damageContext.damageValue = 0;

                // Knock out
                ForceUnconsciousness(0);

                // Schedule wake-up — no EOnFrame needed
                GetGame().GetCallqueue().CallLater(CGQC_WakeUp, CGQC_KO_DURATION * 1000, false);

                return false;
            }
        }

        return super.HijackDamageHandling(damageContext);
    }

    //------------------------------------------------------------------------------------------------
    protected void CGQC_WakeUp()
    {
        // FullHeal restores all hitzones including resilience
        FullHeal();
        UpdateConsciousness();
    }
}