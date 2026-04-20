// ============================================================
// CGQC_RangeTools_HitDisplay.c
// Custom popup display replacing SCR_HintManagerComponent
// for range tool hit feedback. Fades out after duration.
// Call CGQC_RangeTools_HitDisplay.Show(...) from client side.
// ============================================================

class CGQC_RangeTools_HitDisplay
{
	protected static const ResourceName LAYOUT = "{CF3057165B961942}UI/layouts/CGQC_RangeTools_HitDisplay.layout";
	protected static const float FADE_DURATION  = 0.4;	// seconds for fade out animation
	protected static const float SHOW_DURATION  = 3.0;	// seconds before fade starts

	protected static Widget			s_wRoot;
	protected static RichTextWidget	s_wTitle;
	protected static RichTextWidget	s_wBody;
	protected static bool			s_bInitialized = false;

	// ----------------------------------------------------------------
	// Show - call this from RPC receiver (client only)
	// ----------------------------------------------------------------
	static void Show(string title, string body, float duration = SHOW_DURATION)
	{
		if (!EnsureWidget())
			return;

		s_wTitle.SetText(title);
		s_wBody.SetText(body);

		// Make visible and fully opaque
		s_wRoot.SetVisible(true);
		s_wRoot.SetOpacity(1.0);

		// Cancel any pending fade and schedule new one
		GetGame().GetCallqueue().Remove(CGQC_RangeTools_HitDisplay.StartFade);
		GetGame().GetCallqueue().CallLater(CGQC_RangeTools_HitDisplay.StartFade, (int)(duration * 1000), false);
	}

	// ----------------------------------------------------------------
	// StartFade - begins opacity animation to 0
	// ----------------------------------------------------------------
	static void StartFade()
	{
		if (!s_wRoot)
			return;

		AnimateWidget.Opacity(s_wRoot, 0.0, 1.0 / FADE_DURATION);
		GetGame().GetCallqueue().CallLater(CGQC_RangeTools_HitDisplay.Hide, (int)(FADE_DURATION * 1000) + 50, false);
	}

	// ----------------------------------------------------------------
	// Hide - called after fade completes
	// ----------------------------------------------------------------
	static void Hide()
	{
		if (s_wRoot)
			s_wRoot.SetVisible(false);
	}

	// ----------------------------------------------------------------
	// EnsureWidget - creates layout on first call
	// ----------------------------------------------------------------
	protected static bool EnsureWidget()
	{
		if (s_bInitialized && s_wRoot)
			return true;

		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return false;

		s_wRoot = ws.CreateWidgets(LAYOUT);
		if (!s_wRoot)
		{
			Print("[CGQC_HitDisplay] CreateWidgets failed! Layout: " + LAYOUT, LogLevel.ERROR);
			return false;
		}

		s_wTitle = RichTextWidget.Cast(s_wRoot.FindAnyWidget("CGQC_HitDisplay_Title"));
		s_wBody  = RichTextWidget.Cast(s_wRoot.FindAnyWidget("CGQC_HitDisplay_Body"));

		if (!s_wTitle || !s_wBody)
		{
			Print("[CGQC_HitDisplay] Widget lookup failed!", LogLevel.ERROR);
			return false;
		}

		s_wRoot.SetVisible(false);
		s_bInitialized = true;
		return true;
	}
}