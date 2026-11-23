// scripts/GameCode/CGQC_Action_TourniquetDrill.c

class CGQC_Action_TourniquetDrillClass : ScriptAndConfig {}

class CGQC_Action_TourniquetDrill : SCR_ScriptedUserAction
{
    [Attribute("0", UIWidgets.ComboBox, "Difficulty", "", ParamEnumArray.FromEnum(CGQC_EDifficulty))]
    protected CGQC_EDifficulty m_Difficulty;

    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        AF_SingleCasualtySpawnerComponent spawner = AF_SingleCasualtySpawnerComponent.Cast(pOwnerEntity.FindComponent(AF_SingleCasualtySpawnerComponent));
        if (!spawner) return;

        spawner.SpawnTourniquetDrill(m_Difficulty);
    }

    override bool CanBePerformedScript(IEntity user)
    {
        return true;
    }
}