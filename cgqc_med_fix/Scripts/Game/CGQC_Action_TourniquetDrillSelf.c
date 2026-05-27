// scripts/GameCode/CGQC_Action_TourniquetDrillSelf.c

class CGQC_Action_TourniquetDrillSelfClass : ScriptAndConfig {}

class CGQC_Action_TourniquetDrillSelf : SCR_ScriptedUserAction
{
    [Attribute("0", UIWidgets.ComboBox, "Difficulty", "", ParamEnumArray.FromEnum(CGQC_EDifficulty))]
    protected CGQC_EDifficulty m_Difficulty;

    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        AF_CasualtyDamageComponent damageComp = AF_CasualtyDamageComponent.Cast(pOwnerEntity.FindComponent(AF_CasualtyDamageComponent));
        if (!damageComp) return;

        damageComp.ApplyCGQCScenario(pUserEntity, CGQC_SCENARIO_TOURNIQUET_DRILL_SELF, m_Difficulty);
    }

    override bool CanBePerformedScript(IEntity user)
    {
        return true;
    }
}