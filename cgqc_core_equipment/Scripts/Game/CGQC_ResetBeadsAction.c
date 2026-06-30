/*
class CGQC_ResetBeadsAction : ScriptedUserAction
{
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        SCR_PaceDistanceComponent tracker = SCR_PaceDistanceComponent.Cast(
            pUserEntity.FindComponent(SCR_PaceDistanceComponent)
        );

        if (!tracker)
            return;

        tracker.ResetDistance();

        SCR_HintManagerComponent.GetInstance().ShowCustomHint(
            "Pace count reset.",
            "Ranger Beads",
            2.0
        );
    }

    override bool CanBeShownScript(IEntity user)
    {
        return true;
    }

    override bool HasLocalEffectOnlyScript()
    {
        return true;
    }

    override bool CanBePerformedScript(IEntity user)
    {
        return true;
    }
}
*/