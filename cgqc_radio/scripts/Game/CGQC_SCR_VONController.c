modded class SCR_VONController
{
    protected float m_fCGQC_WatchdogTimer = 0;
    protected static const float CGQC_WATCHDOG_INTERVAL = 3.0; // check every 3 seconds

    //------------------------------------------------------------------------------------------------
    void ForceEnableVONDirectToggle()
    {
        AssignVONComponent(); // Fix ghost state: re-fetch component before toggling
        SetVONProximityToggle(true);
    }

    //------------------------------------------------------------------------------------------------
    override void Update(float timeSlice)
    {
        super.Update(timeSlice);

        m_fCGQC_WatchdogTimer += timeSlice;
        if (m_fCGQC_WatchdogTimer < CGQC_WATCHDOG_INTERVAL)
            return;

        m_fCGQC_WatchdogTimer = 0;
        CGQC_CheckVONState();
    }

    //------------------------------------------------------------------------------------------------
    protected void CGQC_CheckVONState()
	{
	    if (m_bIsDisabled || m_bIsPauseDisabled || m_bIsUnconscious)
	        return;
	
	    if (!m_bIsToggledDirect)
	    {
			CGQC_ShowVONHint("VON DIRECT désactivé !");
	        Print("[CGQC_VON] WATCHDOG: toggle off inattendu", LogLevel.ERROR);
	        return;
	    }
	
	    if (!m_VONComp)
	    {
			CGQC_ShowVONHint("VON fantôme — réinitialisation...");
	        Print("[CGQC_VON] WATCHDOG: ghost state détecté, réinitialisation", LogLevel.ERROR);
	        ForceEnableVONDirectToggle();
	        return;
	    }
	}

    //------------------------------------------------------------------------------------------------
    protected void CGQC_ShowVONHint(string message)
    {
        SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
        if (hintManager)
            hintManager.ShowCustomHint(message, "VON", 4.0);
    }
}