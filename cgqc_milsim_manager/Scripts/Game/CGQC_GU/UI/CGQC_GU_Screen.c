// Gestion d'unité - screens. Every screen is opened from the pause menu and lives inside it as widgets:
// the pause menu already shows the cursor and blocks game input, so no chimeraMenus.conf change and no
// input context are needed (decision 2 of the plan). Échap closes the pause menu, and our screen with it.

//! Host: the open pause menu's root widget, plus the one screen currently shown on top of it.
class CGQC_GU_Ui
{
	protected static Widget s_wHostRoot;
	protected static ref CGQC_GU_Screen s_Current;
	protected static ref CGQC_GU_Screen s_Pending;
	protected static ref array<Widget> s_aHidden = new array<Widget>();
	// True while the player is inside Gestion d'unité: FERMER on a paper screen goes back to it.
	protected static bool s_bMenuSession;

	//------------------------------------------------------------------------------------------------
	//! Opens the Gestion d'unité menu and starts a menu session.
	static void OpenUnitMenu()
	{
		s_bMenuSession = true;
		OpenNextFrame(new CGQC_GU_SituationScreen());
	}

	//------------------------------------------------------------------------------------------------
	//! FERMER on a paper screen reached from the Gestion d'unité navigation: back to Situation.
	//! RETOUR on a dark screen, or FERMER outside the menu: back to the pause menu.
	static void Back()
	{
		if (s_bMenuSession && !CGQC_GU_DarkScreen.Cast(s_Current))
		{
			OpenNextFrame(new CGQC_GU_SituationScreen());
			return;
		}

		s_bMenuSession = false;
		CloseCurrent();
	}

	//------------------------------------------------------------------------------------------------
	static void OnHostOpened(Widget hostRoot)
	{
		s_wHostRoot = hostRoot;
	}

	//------------------------------------------------------------------------------------------------
	static void OnHostClosed()
	{
		s_Pending = null;
		s_bMenuSession = false;
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
		HidePauseMenu();
		s_Current = screen;
		screen.Build(s_wHostRoot);
	}

	//------------------------------------------------------------------------------------------------
	//! Switch screens from a button of the current screen: the switch runs on the next frame, so the
	//! pressed button is not destroyed while its own click is still being handled.
	static void OpenNextFrame(CGQC_GU_Screen screen)
	{
		s_Pending = screen;
		GetGame().GetCallqueue().Remove(OpenPending);
		GetGame().GetCallqueue().CallLater(OpenPending, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OpenPending()
	{
		CGQC_GU_Screen screen = s_Pending;
		s_Pending = null;
		Open(screen);
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
		RestorePauseMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! While a screen is open, the pause menu's own widgets (vanilla buttons, overview panel) are hidden:
	//! hidden widgets take no clicks and no keyboard focus, so nothing reacts behind the paper.
	protected static void HidePauseMenu()
	{
		if (!s_wHostRoot)
			return;

		Widget child = s_wHostRoot.GetChildren();
		while (child)
		{
			if (child.IsVisible())
			{
				child.SetVisible(false);
				s_aHidden.Insert(child);
			}

			child = child.GetSibling();
		}

		GetGame().GetWorkspace().SetFocusedWidget(null);
	}

	//------------------------------------------------------------------------------------------------
	protected static void RestorePauseMenu()
	{
		foreach (Widget w : s_aHidden)
		{
			if (w)
				w.SetVisible(true);
		}

		s_aHidden.Clear();
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

//! Tabs of the paper rail, in the mockup's order. Tabs of later steps and phases are greyed out.
enum CGQC_GU_ERailTab
{
	NONE,
	// Événements
	OPERATIONS,
	DEPLOYMENTS,
	TRAININGS,
	// Dossier
	REGISTRY,
	GRADE,
	QUALS,
	SESSIONS,
	PROGRESSION,
	INSTRUCTOR,
	BADGES,
	AWARDS,
	// Unité
	MEMBERS,
	ORBAT,
	CAT_QUALS,
	CAT_EXERCISES,
	CAT_AWARDS
}

//! Rail entry: a side tab that tells its screen which tab was pressed.
class CGQC_GU_RailEntry
{
	protected CGQC_GU_PaperScreen m_Screen;
	protected CGQC_GU_ERailTab m_eTab;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_RailEntry(CGQC_GU_PaperScreen screen, CGQC_GU_ERailTab tab)
	{
		m_Screen = screen;
		m_eTab = tab;
	}

	//------------------------------------------------------------------------------------------------
	void OnActivated()
	{
		if (m_Screen)
			m_Screen.OnRailSelected(m_eTab);
	}
}

//! Paper screens (Dossier, Unité, Événements): parchment shell with a header, a content stack and the
//! right rail, which is the same on every paper screen.
class CGQC_GU_PaperScreen : CGQC_GU_Screen
{
	protected TextWidget m_wCap;
	protected TextWidget m_wTitle;
	protected Widget m_wHeaderTools;	// right side of the header (search)
	protected Widget m_wContent;
	protected Widget m_wRail;
	protected ref map<int, CGQC_GU_Button> m_mRail = new map<int, CGQC_GU_Button>();

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
		m_wHeaderTools = Find("HeaderTools");
		m_wContent = Find("ContentStack");
		m_wRail = Find("Rail");

		if (m_wCap)
			m_wCap.SetColor(CGQC_GU_Style.InkMuted());

		if (m_wTitle)
			m_wTitle.SetColor(CGQC_GU_Style.Ink());

		OnBuildPaper();
		BuildRail();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBuildPaper();

	//------------------------------------------------------------------------------------------------
	//! Which rail tab is shown by this screen (drawn as the active one).
	protected CGQC_GU_ERailTab GetRailTab()
	{
		return CGQC_GU_ERailTab.NONE;
	}

	//------------------------------------------------------------------------------------------------
	//! A rail tab was pressed. Screens override this to switch in place; the default opens the tab's screen.
	void OnRailSelected(CGQC_GU_ERailTab tab)
	{
		if (tab == GetRailTab())
			return;

		switch (tab)
		{
			// The Dossier tabs open one's own dossier.
			case CGQC_GU_ERailTab.REGISTRY:
			case CGQC_GU_ERailTab.GRADE:
			case CGQC_GU_ERailTab.QUALS:
			case CGQC_GU_ERailTab.SESSIONS:
			case CGQC_GU_ERailTab.BADGES:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_DossierScreen("", tab));
				break;

			case CGQC_GU_ERailTab.MEMBERS:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_MembersScreen());
				break;

			case CGQC_GU_ERailTab.CAT_QUALS:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_QualCatalogueScreen());
				break;

			case CGQC_GU_ERailTab.CAT_EXERCISES:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_ExerciseCatalogueScreen());
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetHeader(string cap, string title)
	{
		if (m_wCap)
			m_wCap.SetText(cap);

		if (m_wTitle)
			m_wTitle.SetText(title);
	}

	//------------------------------------------------------------------------------------------------
	//! The mockup's full rail: Événements / Dossier / Unité, then FERMER at the bottom.
	protected void BuildRail()
	{
		if (!m_wRail)
			return;

		RailSection("ÉVÉNEMENTS", 0);
		RailTab("OPÉRATIONS", CGQC_GU_ERailTab.OPERATIONS);
		RailTab("DÉPLOIEMENTS", CGQC_GU_ERailTab.DEPLOYMENTS);
		RailTab("ENTRAÎNEMENTS", CGQC_GU_ERailTab.TRAININGS);

		RailSection("DOSSIER");
		RailTab("REGISTRE", CGQC_GU_ERailTab.REGISTRY, true);
		RailTab("GRADE & ACCÈS", CGQC_GU_ERailTab.GRADE, true);
		RailTab("QUALIFICATIONS", CGQC_GU_ERailTab.QUALS, true);
		RailTab("SÉANCES / FTX", CGQC_GU_ERailTab.SESSIONS, true);
		RailTab("PROGRESSION", CGQC_GU_ERailTab.PROGRESSION);
		RailTab("INSTRUCTEUR", CGQC_GU_ERailTab.INSTRUCTOR);
		RailTab("INSIGNES", CGQC_GU_ERailTab.BADGES, true);
		RailTab("DISTINCTIONS", CGQC_GU_ERailTab.AWARDS);

		RailSection("UNITÉ");
		// Everyone reads the member list; the screen offers the edits to members.edit holders only.
		RailTab("MEMBRES", CGQC_GU_ERailTab.MEMBERS, true);
		RailTab("ORBAT", CGQC_GU_ERailTab.ORBAT);
		RailTab("LISTE DES QUALIFICATIONS", CGQC_GU_ERailTab.CAT_QUALS, true);
		RailTab("LISTE DES EXERCICES", CGQC_GU_ERailTab.CAT_EXERCISES, true);
		RailTab("LISTE DES DISTINCTIONS", CGQC_GU_ERailTab.CAT_AWARDS);

		Widget spacer = CGQC_GU_W.VBox(m_wRail);
		if (spacer)
			LayoutSlot.SetSizeMode(spacer, LayoutSizeMode.Fill);

		CGQC_GU_Button close = AddButton(m_wRail, "FERMER", CGQC_GU_EButtonKind.PLAIN);
		if (close)
		{
			close.SetRail();
			close.GetOnActivated().Insert(CGQC_GU_Ui.Back);
		}

		SelectRailTab();
	}

	//------------------------------------------------------------------------------------------------
	protected void RailSection(string title, float padTop = 10)
	{
		TextWidget t = CGQC_GU_W.Text(m_wRail, title, CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.InkMuted());
		if (t)
			LayoutSlot.SetPadding(t, 4, padTop, 0, 0);

		CGQC_GU_W.Rule(m_wRail, 2, 6);
	}

	//------------------------------------------------------------------------------------------------
	//! built = false: the tab is greyed out until its screen exists.
	protected void RailTab(string text, CGQC_GU_ERailTab tab, bool built = false)
	{
		CGQC_GU_Button b = AddButton(m_wRail, text, CGQC_GU_EButtonKind.PLAIN);
		if (!b)
			return;

		b.SetRail();
		b.SetEnabled(built);
		CGQC_GU_RailEntry entry = new CGQC_GU_RailEntry(this, tab);
		m_aKeepAlive.Insert(entry);
		b.GetOnActivated().Insert(entry.OnActivated);
		m_mRail.Set(tab, b);
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectRailTab()
	{
		CGQC_GU_ERailTab current = GetRailTab();
		foreach (int tab, CGQC_GU_Button b : m_mRail)
		{
			if (b)
				b.SetSelected(tab == current);
		}
	}
}
