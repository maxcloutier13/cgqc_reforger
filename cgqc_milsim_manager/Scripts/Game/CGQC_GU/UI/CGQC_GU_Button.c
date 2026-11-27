// Gestion d'unité - button built from CGQC_GU_Button.layout.
// Confirm buttons (save, delete, publish, award, merge) need two presses (spec §1): the first turns the
// button red with « APPUYER DE NOUVEAU », the second executes; it resets after CONFIRM_TIMEOUT.

enum CGQC_GU_EButtonKind
{
	PRIMARY,	// CGQC blue, light text
	PLAIN,		// paper button, dark text
	DANGER		// red
}

class CGQC_GU_Button : ScriptedWidgetEventHandler
{
	protected Widget m_wRoot;
	protected ImageWidget m_wBg;
	protected TextWidget m_wLabel;
	protected ref array<Widget> m_aEdges = new array<Widget>();
	protected bool m_bSelected;
	protected ref Color m_TextColor;
	protected bool m_bTextColor;
	protected string m_sText;
	protected string m_sConfirmText = CGQC_GU_Style.CONFIRM_TEXT;
	protected CGQC_GU_EButtonKind m_eKind;
	protected bool m_bConfirm;
	protected bool m_bArmed;
	protected bool m_bEnabled = true;
	protected bool m_bDark;

	// Only one confirm button may be armed at a time: arming another one disarms the previous.
	protected static CGQC_GU_Button s_Armed;

	protected ref ScriptInvokerVoid m_OnActivated = new ScriptInvokerVoid();

	//------------------------------------------------------------------------------------------------
	//! Creates the button inside parent (usually a layout widget). Keep the returned ref alive.
	static CGQC_GU_Button Create(Widget parent, string text, CGQC_GU_EButtonKind kind = CGQC_GU_EButtonKind.PLAIN, bool confirm = false)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(CGQC_GU_Style.LAYOUT_BUTTON, parent);
		if (!root)
		{
			CGQC_GU_Log.Error("Layout bouton introuvable : " + CGQC_GU_Style.LAYOUT_BUTTON);
			return null;
		}

		CGQC_GU_Button b = new CGQC_GU_Button();
		b.Init(root, text, kind, confirm);
		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected void Init(Widget root, string text, CGQC_GU_EButtonKind kind, bool confirm)
	{
		m_wRoot = root;
		m_wBg = ImageWidget.Cast(root.FindAnyWidget("Bg"));
		m_wLabel = TextWidget.Cast(root.FindAnyWidget("Label"));
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string edge : edges)
		{
			Widget e = root.FindAnyWidget(edge);
			if (e)
				m_aEdges.Insert(e);
		}

		m_sText = text;
		m_eKind = kind;
		m_bConfirm = confirm;
		root.AddHandler(this);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvokerVoid GetOnActivated()
	{
		return m_OnActivated;
	}

	//------------------------------------------------------------------------------------------------
	Widget GetRootWidget()
	{
		return m_wRoot;
	}

	//------------------------------------------------------------------------------------------------
	void SetText(string text)
	{
		m_sText = text;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void SetEnabled(bool enabled)
	{
		m_bEnabled = enabled;
		if (!enabled)
			Disarm();

		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Shorter armed label for narrow buttons (default « APPUYER DE NOUVEAU »).
	void SetConfirmText(string text)
	{
		m_sConfirmText = text;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Label colour of a plain button at rest (status colour code). Armed and selected keep theirs.
	void SetTextColor(Color color)
	{
		m_TextColor = color;
		m_bTextColor = true;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Plain button on a dark screen: translucent dark fill, light outline and label.
	void SetDark()
	{
		m_bDark = true;
		if (m_wLabel)
		{
			// Mockup: 46 px tall entries, 17 px label.
			m_wLabel.SetExactFontSize(17);
			AlignableSlot.SetPadding(m_wLabel, 18, 13, 18, 13);
		}

		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Selected tab / current choice: drawn in CGQC blue.
	void SetSelected(bool selected)
	{
		m_bSelected = selected;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Tab of the paper rail: full width, tighter than a plain button so the whole rail fits the screen.
	void SetRail()
	{
		if (m_wLabel)
		{
			m_wLabel.SetExactFontSize(15);
			AlignableSlot.SetPadding(m_wLabel, 6, 9, 6, 9);
		}

		if (m_wRoot)
		{
			LayoutSlot.SetHorizontalAlign(m_wRoot, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(m_wRoot, 0, 0, 0, 5);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Button inside page content: smaller label and padding, sized to its text instead of the full width.
	//! The parent must be a layout (vertical or horizontal).
	void SetCompact()
	{
		if (m_wLabel)
		{
			m_wLabel.SetExactFontSize(15);
			AlignableSlot.SetPadding(m_wLabel, 12, 5, 12, 5);
		}

		if (m_wRoot)
			LayoutSlot.SetHorizontalAlign(m_wRoot, LayoutHorizontalAlign.Left);
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_bEnabled)
			return true;

		if (!m_bConfirm)
		{
			m_OnActivated.Invoke();
			return true;
		}

		if (!m_bArmed)
		{
			if (s_Armed && s_Armed != this)
				s_Armed.Disarm();

			s_Armed = this;
			m_bArmed = true;
			Refresh();
			GetGame().GetCallqueue().Remove(Disarm);
			GetGame().GetCallqueue().CallLater(Disarm, CGQC_GU_Style.CONFIRM_TIMEOUT * 1000, false);
			return true;
		}

		Disarm();
		m_OnActivated.Invoke();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Disarm()
	{
		GetGame().GetCallqueue().Remove(Disarm);
		if (s_Armed == this)
			s_Armed = null;

		if (!m_bArmed)
			return;

		m_bArmed = false;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_wRoot)
			return;

		Color bg;
		Color fg;
		bool outline;
		if (m_bArmed)
		{
			bg = CGQC_GU_Style.ConfirmRed();
			fg = CGQC_GU_Style.PaperText();
		}
		else if (m_bDark && m_bSelected)
		{
			// Mockup: the active entry is #5B82E0 with dark text.
			bg = CGQC_GU_Style.BlueLight();
			fg = CGQC_GU_Style.DarkOnBlue();
		}
		else if (m_bSelected || m_eKind == CGQC_GU_EButtonKind.PRIMARY)
		{
			bg = CGQC_GU_Style.Blue();
			fg = CGQC_GU_Style.PaperText();
		}
		else if (m_eKind == CGQC_GU_EButtonKind.DANGER)
		{
			bg = CGQC_GU_Style.Red();
			fg = CGQC_GU_Style.White();
		}
		else if (m_bDark)
		{
			// Mockup: solid #3A3E40, no outline.
			bg = CGQC_GU_Style.DarkButton();
			fg = CGQC_GU_Style.DarkText();
			if (m_bTextColor)
				fg = m_TextColor;
		}
		else
		{
			// Plain button on paper: transparent with a thin ruled outline (owner's 1d visual fix).
			bg = CGQC_GU_Style.Clear();
			fg = CGQC_GU_Style.InkSoft();
			if (m_bTextColor)
				fg = m_TextColor;
			outline = true;
		}

		if (m_wBg)
			m_wBg.SetColor(bg);

		foreach (Widget edge : m_aEdges)
		{
			edge.SetVisible(outline);
			if (outline)
				edge.SetColor(CGQC_GU_Style.Rule());
		}

		if (m_wLabel)
		{
			m_wLabel.SetColor(fg);
			if (m_bArmed)
				m_wLabel.SetText(m_sConfirmText);
			else
				m_wLabel.SetText(m_sText);
		}

		if (m_bEnabled)
			m_wRoot.SetOpacity(1);
		else
			m_wRoot.SetOpacity(0.45);
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		GetGame().GetCallqueue().Remove(Disarm);
		if (s_Armed == this)
			s_Armed = null;

		if (m_wRoot)
		{
			m_wRoot.RemoveHandler(this);
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
	}
}
