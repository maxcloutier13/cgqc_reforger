modded class PACT_ShotTimerHUD
{
    protected const string W_HINT_LABEL = "Text0";
    protected const string W_SHOT_LIST  = "ShotsVBox";
    protected const string W_SHOT_ROW   = "{1785387979A2B2E0}";
    protected TextWidget         m_wHint;
    protected ref array<Widget>  m_aEventRows = new array<Widget>();

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
        // Delete our dynamically created event rows before base clears its own
        foreach (Widget row : m_aEventRows)
        {
            if (row) delete row;
        }
        m_aEventRows.Clear();

        super.ClearShots();
        if (m_wTotal) m_wTotal.SetText("");
    }

    void SetStandby()
    {
        if (m_wStatus) m_wStatus.SetText("STANDBY");
        if (m_wDot) m_wDot.SetColor(Color.FromRGBA(255, 200, 50, 255));
    }

    // Called by component after every real shot to ensure total is always correct
    void SetTotal(float ms)
    {
        if (m_wTotal)
            m_wTotal.SetText(string.Format("%1 s", (ms / 1000.0).ToString(6)));
    }

    void AddEventRow(string label, float splitMs, float elapsedMs)
    {
        if (!m_wRoot) return;

        Widget container = m_wRoot.FindAnyWidget(W_SHOT_LIST);
        if (!container) return;

        Widget row = GetGame().GetWorkspace().CreateWidgets(W_SHOT_ROW, container);
        if (!row) return;

        m_aEventRows.Insert(row); // Track for cleanup

        TextWidget wNum     = TextWidget.Cast(row.FindAnyWidget("RowNum"));
        TextWidget wSplit   = TextWidget.Cast(row.FindAnyWidget("RowSplit"));
        TextWidget wElapsed = TextWidget.Cast(row.FindAnyWidget("RowElapsed"));

        if (wNum)
        {
            wNum.SetText(label);
            wNum.SetColor(Color.FromRGBA(255, 200, 50, 255));
        }
        if (wSplit)   wSplit.SetText(string.Format("%1s",   (splitMs   / 1000.0).ToString(2)));
        if (wElapsed) wElapsed.SetText(string.Format("%1s", (elapsedMs / 1000.0).ToString(2)));
    }
}