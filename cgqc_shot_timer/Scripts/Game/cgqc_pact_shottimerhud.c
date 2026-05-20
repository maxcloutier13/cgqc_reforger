modded class PACT_ShotTimerHUD
{
    protected const string W_HINT_LABEL = "Text0";
    protected TextWidget m_wHint;

    override void SetState(PACT_TimerState state)
    {
        if (!m_wHint && m_wRoot)
            m_wHint = TextWidget.Cast(m_wRoot.FindAnyWidget(W_HINT_LABEL));

        if (!m_wStatus) return;

        if (m_wRoot)
            m_wRoot.SetVisible(state != PACT_TimerState.IDLE);

        switch (state)
        {
            case PACT_TimerState.IDLE:
                m_wStatus.SetText("IDLE");
                if (m_wDot) m_wDot.SetColor(Color.FromRGBA(100, 100, 100, 255));
                if (m_wLiveTimer) m_wLiveTimer.SetText("");
                if (m_wElapsed) m_wElapsed.SetText("");
                if (m_wHint) m_wHint.SetText("");
                break;
            case PACT_TimerState.ARMED:
                m_wStatus.SetText("ARMED");
                if (m_wDot) m_wDot.SetColor(Color.FromRGBA(255, 200, 50, 255));
                if (m_wLiveTimer) m_wLiveTimer.SetText("");
                if (m_wElapsed) m_wElapsed.SetText("");
                if (m_wHint) m_wHint.SetText("[ start ]");
                break;
            case PACT_TimerState.RUNNING:
                m_wStatus.SetText("RUN");
                if (m_wDot) m_wDot.SetColor(Color.FromRGBA(61, 220, 132, 255));
                if (m_wHint) m_wHint.SetText("");
                break;
            case PACT_TimerState.PAUSED:
                m_wStatus.SetText("STOP");
                if (m_wDot) m_wDot.SetColor(Color.FromRGBA(230, 100, 50, 255));
                if (m_wHint) m_wHint.SetText("[ reset ]");
                break;
        }
    }

    override void ClearShots()
    {
        super.ClearShots();
        if (m_wTotal) m_wTotal.SetText("");
    }

    // Replaces the big "ARMED" status text with "STANDBY" until SetState is called again.
    void SetStandby()
    {
        if (m_wStatus) m_wStatus.SetText("STANDBY");
        if (m_wDot) m_wDot.SetColor(Color.FromRGBA(255, 200, 50, 255));
    }
}