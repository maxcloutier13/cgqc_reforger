// Gestion d'unité - screens. Every screen is opened from the pause menu and lives inside it as widgets:
// the pause menu already shows the cursor and blocks game input, so no chimeraMenus.conf change and no
// input context are needed (decision 2 of the plan). Échap closes the pause menu, and our screen with it.

//! Host: the open pause menu's root widget, plus the one screen currently shown on top of it.
class CGQC_GU_Ui
{
	protected static Widget s_wHostRoot;
	protected static ref CGQC_GU_Screen s_Current;

	//------------------------------------------------------------------------------------------------
	static void OnHostOpened(Widget hostRoot)
	{
		s_wHostRoot = hostRoot;
	}

	//------------------------------------------------------------------------------------------------
	static void OnHostClosed()
	{
		CloseCurrent();
		s_wHostRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsHostOpen()
	{
		return s_wHostRoot != null;
	}

	//------------------------------------------------------------------------------------------------
	static void Open(CGQC_GU_Screen screen)
	{
		if (!s_wHostRoot || !screen)
			return;

		CloseCurrent();
		s_Current = screen;
		screen.Build(s_wHostRoot);
	}

	//------------------------------------------------------------------------------------------------
	//! Back to the pause menu underneath.
	static void CloseCurrent()
	{
		if (!s_Current)
			return;

		CGQC_GU_Screen screen = s_Current;
		s_Current = null;
		screen.Teardown();
	}
}

//! Base screen: owns its root widget and the buttons created on it.
class CGQC_GU_Screen
{
	protected Widget m_wRoot;
	protected ref array<ref CGQC_GU_Button> m_aButtons = new array<ref CGQC_GU_Button>();
	protected ref array<ref Managed> m_aKeepAlive = new array<ref Managed>();

	static const int Z_ORDER = 1000;

	//------------------------------------------------------------------------------------------------
	//! Layout used as the screen's root.
	protected ResourceName GetLayout();

	//------------------------------------------------------------------------------------------------
	//! Fill the screen once its root exists.
	protected void OnBuild();

	//------------------------------------------------------------------------------------------------
	protected void OnTeardown();

	//------------------------------------------------------------------------------------------------
	void Build(Widget host)
	{
		m_wRoot = GetGame().GetWorkspace().CreateWidgets(GetLayout(), host);
		if (!m_wRoot)
		{
			CGQC_GU_Log.Error("Layout d'écran introuvable : " + GetLayout());
			return;
		}

		// A layout's root has no slot of its own: stretch it over the whole host (the pause menu's frame).
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wRoot.SetZOrder(Z_ORDER);
		OnBuild();
	}

	//------------------------------------------------------------------------------------------------
	void Teardown()
	{
		OnTeardown();
		foreach (CGQC_GU_Button b : m_aButtons)
		{
			b.Destroy();
		}

		m_aButtons.Clear();
		m_aKeepAlive.Clear();
		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button AddButton(Widget parent, string text, CGQC_GU_EButtonKind kind = CGQC_GU_EButtonKind.PLAIN, bool confirm = false)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text, kind, confirm);
		if (b)
			m_aButtons.Insert(b);

		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected TextWidget AddText(Widget parent, string text, ResourceName font, int size, Color color)
	{
		TextWidget t = TextWidget.Cast(GetGame().GetWorkspace().CreateWidgets(CGQC_GU_Style.LAYOUT_TEXT, parent));
		if (!t)
			return null;

		t.SetText(text);
		t.SetFont(font);
		t.SetExactFontSize(size);
		t.SetColor(color);
		return t;
	}

	//------------------------------------------------------------------------------------------------
	protected Widget Find(string name)
	{
		if (!m_wRoot)
			return null;

		return m_wRoot.FindAnyWidget(name);
	}
}

//! Rail entry: a side tab that tells its screen which index was pressed.
class CGQC_GU_RailEntry
{
	protected CGQC_GU_PaperScreen m_Screen;
	protected int m_iIndex;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_RailEntry(CGQC_GU_PaperScreen screen, int index)
	{
		m_Screen = screen;
		m_iIndex = index;
	}

	//------------------------------------------------------------------------------------------------
	void OnActivated()
	{
		if (m_Screen)
			m_Screen.OnRailSelected(m_iIndex);
	}
}

//! Paper screens (Dossier, Unité, Événements): parchment shell with a header, a content stack and a right rail.
class CGQC_GU_PaperScreen : CGQC_GU_Screen
{
	protected TextWidget m_wCap;
	protected TextWidget m_wTitle;
	protected Widget m_wContent;
	protected Widget m_wRail;

	//------------------------------------------------------------------------------------------------
	override protected ResourceName GetLayout()
	{
		return CGQC_GU_Style.LAYOUT_PAPER_SHELL;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuild()
	{
		ImageWidget bg = ImageWidget.Cast(Find("Bg"));
		if (bg)
			bg.SetColor(CGQC_GU_Style.Parchment());

		ImageWidget rule = ImageWidget.Cast(Find("Rule"));
		if (rule)
			rule.SetColor(CGQC_GU_Style.Rule());

		m_wCap = TextWidget.Cast(Find("Cap"));
		m_wTitle = TextWidget.Cast(Find("Title"));
		m_wContent = Find("ContentStack");
		m_wRail = Find("Rail");

		if (m_wCap)
			m_wCap.SetColor(CGQC_GU_Style.InkMuted());

		if (m_wTitle)
			m_wTitle.SetColor(CGQC_GU_Style.Ink());

		OnBuildPaper();

		CGQC_GU_Button close = AddButton(m_wRail, "FERMER", CGQC_GU_EButtonKind.PLAIN);
		if (close)
			close.GetOnActivated().Insert(CGQC_GU_Ui.CloseCurrent);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBuildPaper();

	//------------------------------------------------------------------------------------------------
	void OnRailSelected(int index);

	//------------------------------------------------------------------------------------------------
	protected void SetHeader(string cap, string title)
	{
		if (m_wCap)
			m_wCap.SetText(cap);

		if (m_wTitle)
			m_wTitle.SetText(title);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button AddRailEntry(string text, int index)
	{
		CGQC_GU_Button b = AddButton(m_wRail, text, CGQC_GU_EButtonKind.PLAIN);
		if (!b)
			return null;

		CGQC_GU_RailEntry entry = new CGQC_GU_RailEntry(this, index);
		m_aKeepAlive.Insert(entry);
		b.GetOnActivated().Insert(entry.OnActivated);
		return b;
	}
}
