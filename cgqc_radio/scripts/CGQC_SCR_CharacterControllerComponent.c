modded class SCR_CharacterControllerComponent
{
    //------------------------------------------------------------------------------------------------
    override void OnConsciousnessChanged(bool conscious)
    {
        super.OnConsciousnessChanged(conscious);

        // Only care about waking up
        if (!conscious)
            return;

        // Only process on the owning client — compare local controlled entity to this character
        PlayerController localPC = GetGame().GetPlayerController();
        if (!localPC || localPC.GetControlledEntity() != GetOwner())
            return;

        SCR_PlayerController pc = SCR_PlayerController.Cast(localPC);
        if (!pc)
            return;

        GetGame().GetCallqueue().CallLater(CGQC_EnableVONAfterWakeup, 3000, false, pc);
    }

    //------------------------------------------------------------------------------------------------
    protected void CGQC_EnableVONAfterWakeup(SCR_PlayerController pc)
    {
        if (!pc)
            return;

        SCR_VONController vonController = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
        if (!vonController)
            return;

        vonController.ForceEnableVONDirectToggle();
    }
}