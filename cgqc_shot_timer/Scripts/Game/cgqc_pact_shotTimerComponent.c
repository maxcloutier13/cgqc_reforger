modded class PACT_ShotTimerComponent
{
    override void StartTimer(IEntity owner)
    {
        if (m_bRunning || m_bStartPending)
            return;

        m_bArmed = true;
        m_bRunning = false;
        m_bStopped = false;
        m_bStartPending = true;
        m_iLastAmmoCount = -1;

        if (m_HUD)
        {
            m_HUD.SetState(PACT_TimerState.ARMED);
            m_HUD.SetStandby(); // big visible "STANDBY" in the status row
        }
		AudioSystem.PlaySound("{5FFE043DE4AD561B}"); // standby cue

        int delay = Math.RandomInt(2500, 3001);
        GetGame().GetCallqueue().CallLater(BeginTimerAfterBeep, delay, false);
        Print(string.Format("[PACT ShotTimer] CGQC: STANDBY, beep in %1 ms (tick %2).",
            delay, System.GetTickCount()));
    }

    override void BeginTimerAfterBeep()
    {
        if (!m_bStartPending)
            return;

        PlayBeep(m_OwnerEntity);
        Print(string.Format("[PACT ShotTimer] CGQC: PlayBeep called at tick %1.",
            System.GetTickCount()));

        // Delay between beep queue and timer start. Tune in 50ms steps.
        GetGame().GetCallqueue().CallLater(CommitTimerStart, 975, false);
    }

    protected void CommitTimerStart()
    {
        if (!m_bStartPending)
            return;

        m_bStartPending = false;
        m_bRunning = true;
        m_bArmed = true;
        m_bStopped = false;

        m_fStartTime = System.GetTickCount();
        m_fLastShot = m_fStartTime;
        m_iLastAmmoCount = -1;

        if (m_HUD)
            m_HUD.SetState(PACT_TimerState.RUNNING);

        Print(string.Format("[PACT ShotTimer] CGQC: RUNNING at tick %1.",
            System.GetTickCount()));
    }
}
