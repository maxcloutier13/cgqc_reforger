// scripts/GameCode/CGQC_CasualtyDamageComponent.c

modded class AF_CasualtyDamageComponent : ScriptComponent
{
    // Debug Logging
    static bool s_bDebugEnabled = false;

    static void Log(string msg)
    {
        if (!s_bDebugEnabled)
            return;

        Print(string.Format("[CGQC_Casualty] %1", msg));
    }

    // Entry point for CGQC scenarios
    void ApplyCGQCScenario(IEntity casualty, int scenario, CGQC_EDifficulty difficulty = CGQC_EDifficulty.EASY)
    {
        SCR_CharacterDamageManagerComponent dmgMgr = SCR_CharacterDamageManagerComponent.Cast(casualty.FindComponent(SCR_CharacterDamageManagerComponent));
        ACE_Medical_VitalsComponent vitals = ACE_Medical_VitalsComponent.Cast(casualty.FindComponent(ACE_Medical_VitalsComponent));

        if (!dmgMgr || !vitals)
        {
            Log("ApplyCGQCScenario: missing dmgMgr or vitals");
            return;
        }

        Log(string.Format("ApplyCGQCScenario: scenario=%1 difficulty=%2", scenario, difficulty));

        switch (scenario)
        {
            case CGQC_SCENARIO_TOURNIQUET_DRILL:
                ApplyCGQC_TourniquetDrill(dmgMgr, vitals, difficulty);
                break;
            case CGQC_SCENARIO_TOURNIQUET_DRILL_SELF:
                ApplyCGQC_TourniquetDrillSelf(dmgMgr, vitals, difficulty);
                break;
            default:
                Log(string.Format("ApplyCGQCScenario: unknown scenario %1", scenario));
                break;
        }
    }

    // -------------------------------------------------------------------------
    // SHARED HELPERS
    // -------------------------------------------------------------------------

  protected void ApplyCGQC_LimbBleed(SCR_CharacterDamageManagerComponent dmgMgr, CGQC_EDifficulty difficulty, bool applyHealthDmg = true)
	{
	    float bleedScale;
	    int woundCount;
	
	    switch (difficulty)
	    {
	        case CGQC_EDifficulty.EASY:   bleedScale = 0.2; woundCount = 1; break;
	        case CGQC_EDifficulty.NORMAL: bleedScale = 2.0; woundCount = 2; break;
	        case CGQC_EDifficulty.HARD:   bleedScale = 4.0; woundCount = 3; break;
	        default:                      bleedScale = 0.2; woundCount = 1; break;
	    }
	
	    array<string> limbs = new array<string>();
	    limbs.Insert("LArm");
	    limbs.Insert("RArm");
	    limbs.Insert("LForearm");
	    limbs.Insert("RForearm");
	    limbs.Insert("LCalf");
	    limbs.Insert("RCalf");
	
	    dmgMgr.SetBleedingScale(bleedScale, true);
	
	    for (int i = 0; i < woundCount; i++)
	    {
	        string pick = limbs.GetRandomElement();
	        SCR_CharacterHitZone hz = SCR_CharacterHitZone.Cast(dmgMgr.GetHitZoneByName(pick));
	
	        Log(string.Format("LimbBleed[%1]: picked=%2 resolved=%3", i, pick, hz));
	
	        if (!hz) continue;
	
	        if (applyHealthDmg)
	        {
	            float newHealth = Math.Clamp(hz.GetHealthScaled() - 0.35, 0.1, 0.95);
	            hz.SetHealthScaled(newHealth);
	        }
	
	        dmgMgr.AddBleedingEffectOnHitZone(hz, -1);
	    }
	}
	
	protected void ApplyCGQC_TourniquetDrill(SCR_CharacterDamageManagerComponent dmgMgr, ACE_Medical_VitalsComponent vitals, CGQC_EDifficulty difficulty)
	{
	    Log(string.Format("Applying CGQC_TourniquetDrill difficulty=%1", difficulty));
	    dmgMgr.SetPermitUnconsciousness(true, true);
	    dmgMgr.ForceUnconsciousness(0);
	    ApplyCGQC_LimbBleed(dmgMgr, difficulty);
	}
	
	protected void ApplyCGQC_TourniquetDrillSelf(SCR_CharacterDamageManagerComponent dmgMgr, ACE_Medical_VitalsComponent vitals, CGQC_EDifficulty difficulty)
	{
	    Log(string.Format("Applying CGQC_TourniquetDrillSelf difficulty=%1", difficulty));
	    ApplyCGQC_LimbBleed(dmgMgr, difficulty);
	}
}