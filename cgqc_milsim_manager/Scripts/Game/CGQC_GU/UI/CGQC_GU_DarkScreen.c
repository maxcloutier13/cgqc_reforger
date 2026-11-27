// Gestion d'unité - dark screens (Situation, Calendrier), styled after the approved mockup (NavGU):
// the CGQC background darkened, the Gestion d'unité navigation on the left (logo, sections, entries,
// PROCHAIN AU CALENDRIER, RETOUR), and the screen's own panels on the right.

class CGQC_GU_NavEntry
{
	protected int m_iTarget;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_NavEntry(int target)
	{
		m_iTarget = target;
	}

	//------------------------------------------------------------------------------------------------
	void OnActivated()
	{
		CGQC_GU_DarkScreen.OpenTarget(m_iTarget);
	}
}

class CGQC_GU_DarkScreen : CGQC_GU_Screen
{
	// Navigation targets. NAV_NONE: a later phase, shown dimmed.
	static const int NAV_NONE = 0;
	static const int NAV_SITUATION = 1;
	static const int NAV_CALENDAR = 2;
	static const int NAV_DOSSIER = 3;
	static const int NAV_MEMBERS = 4;
	static const int NAV_QUALS = 5;
	static const int NAV_EXERCISES = 6;

	protected Widget m_wContent;
	protected Widget m_wNavItems;
	protected Widget m_wNavBottom;
	protected Widget m_wNavNext;
	protected TextWidget m_wNavNextText;

	//------------------------------------------------------------------------------------------------
	override protected ResourceName GetLayout()
	{
		return CGQC_GU_Style.LAYOUT_DARK_SHELL;
	}

	//------------------------------------------------------------------------------------------------
	//! Which navigation entry is this screen (drawn as the active one).
	protected int GetNavId()
	{
		return NAV_NONE;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuild()
	{
		ImageWidget bg = ImageWidget.Cast(Find("Bg"));
		if (bg)
		{
			// Without the image the screen keeps a plain dark background.
			if (bg.LoadImageTexture(0, CGQC_GU_Style.BACKGROUND))
			{
				bg.SetImage(0);
				bg.SetColor(CGQC_GU_Style.White());
			}
			else
			{
				bg.SetColor(Color.FromSRGBA(0x0E, 0x11, 0x16, 255));
			}
		}

		Widget veil = Find("Veil");
		if (veil)
			veil.SetColor(CGQC_GU_Style.DarkVeil());

		Widget navBg = Find("NavBg");
		if (navBg)
			navBg.SetColor(CGQC_GU_Style.DarkNav());

		ImageWidget logo = ImageWidget.Cast(Find("Logo"));
		if (logo && logo.LoadImageTexture(0, CGQC_GU_Style.LOGO))
			logo.SetImage(0);

		Widget rule = Find("Rule");
		if (rule)
			rule.SetColor(CGQC_GU_Style.DarkLine());

		TextWidget title = TextWidget.Cast(Find("Title"));
		if (title)
			title.SetColor(CGQC_GU_Style.DarkText());

		TextWidget cap = TextWidget.Cast(Find("Cap"));
		if (cap)
			cap.SetColor(CGQC_GU_Style.BlueLighter());

		m_wContent = Find("ContentStack");
		m_wNavItems = Find("NavItems");
		m_wNavBottom = Find("NavBottom");

		BuildNav();
		OnBuildDark();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBuildDark();

	//------------------------------------------------------------------------------------------------
	protected void BuildNav()
	{
		// The Calendrier is for everyone (owner decision 3); the Gestion d'unité entries are not.
		if (CGQC_GU_Client.GetLightState().Can(CGQC_GU_LightState.CAN_MENU))
		{
			NavItem("SITUATION", NAV_SITUATION, string.Empty);
			NavItem("OPÉRATIONS", NAV_NONE, "ÉVÉNEMENTS");
			NavItem("DÉPLOIEMENTS", NAV_NONE, string.Empty);
			NavItem("ENTRAÎNEMENTS", NAV_NONE, string.Empty);
			NavItem("CALENDRIER", NAV_CALENDAR, string.Empty);
			NavItem("MON DOSSIER", NAV_DOSSIER, "DOSSIER");
			NavItem("MEMBRES", NAV_MEMBERS, "UNITÉ");
			NavItem("ORBAT", NAV_NONE, string.Empty);
			NavItem("QUALIFICATIONS", NAV_QUALS, string.Empty);
			NavItem("EXERCICES", NAV_EXERCISES, string.Empty);
			NavItem("DISTINCTIONS", NAV_NONE, string.Empty);
		}

		// « PROCHAIN AU CALENDRIER » (filled by SetNavNext once the screen's data arrives).
		m_wNavNext = CGQC_GU_W.DarkPanel(m_wNavBottom, CGQC_GU_Style.DarkTile(), 16, 12);
		if (m_wNavNext)
		{
			CGQC_GU_W.Text(m_wNavNext, "PROCHAIN AU CALENDRIER", CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.DarkMuted());
			m_wNavNextText = CGQC_GU_W.Text(m_wNavNext, string.Empty, CGQC_GU_Style.FONT_LABEL, 19, CGQC_GU_Style.DarkText(), true);
			m_wNavNext.GetParent().SetVisible(false);
		}

		CGQC_GU_Button back = AddDarkButton(m_wNavBottom, "RETOUR");
		if (back)
		{
			LayoutSlot.SetPadding(back.GetRootWidget(), 0, 8, 0, 0);
			back.GetOnActivated().Insert(CGQC_GU_Ui.Back);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! « PROCHAIN AU CALENDRIER » box at the bottom of the navigation; hidden when nothing is coming.
	protected void SetNavNext(string text)
	{
		if (!m_wNavNext)
			return;

		if (m_wNavNextText)
			m_wNavNextText.SetText(text);

		m_wNavNext.GetParent().SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	//! Section caption (13 px, spaced, ruled under) then the entry.
	protected void NavItem(string text, int target, string section)
	{
		if (!m_wNavItems)
			return;

		if (!section.IsEmpty())
		{
			TextWidget sec = CGQC_GU_W.Text(m_wNavItems, section, CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.DarkMuted());
			if (sec)
				LayoutSlot.SetPadding(sec, 2, 12, 0, 2);

			Line(m_wNavItems, 0, 6);
		}

		CGQC_GU_Button b = AddDarkButton(m_wNavItems, text);
		if (!b)
			return;

		LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 0, 6);
		if (target == NAV_NONE)
		{
			b.SetEnabled(false);
			return;
		}

		if (target == GetNavId())
		{
			b.SetSelected(true);
			return;
		}

		CGQC_GU_NavEntry entry = new CGQC_GU_NavEntry(target);
		m_aKeepAlive.Insert(entry);
		b.GetOnActivated().Insert(entry.OnActivated);
	}

	//------------------------------------------------------------------------------------------------
	//! 1 px separator (#4A4E50).
	protected Widget Line(Widget parent, float padTop, float padBottom)
	{
		Widget r = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, parent);
		if (!r)
			return null;

		r.SetColor(CGQC_GU_Style.DarkLine());
		ImageWidget.Cast(r).SetSize(8, 1);
		LayoutSlot.SetHorizontalAlign(r, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(r, 0, padTop, 0, padBottom);
		return r;
	}

	//------------------------------------------------------------------------------------------------
	//! Plain button styled for the dark screens.
	protected CGQC_GU_Button AddDarkButton(Widget parent, string text, bool confirm = false)
	{
		CGQC_GU_Button b = AddButton(parent, text, CGQC_GU_EButtonKind.PLAIN, confirm);
		if (b)
			b.SetDark();

		return b;
	}

	//------------------------------------------------------------------------------------------------
	//! Small spaced caption (14 px, #A3A7AA), as « SITUATION », « PHASES ».
	protected TextWidget Caption(Widget parent, string text)
	{
		return CGQC_GU_W.Text(parent, text, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.DarkMuted());
	}

	//------------------------------------------------------------------------------------------------
	static void OpenTarget(int target)
	{
		switch (target)
		{
			case NAV_SITUATION:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_SituationScreen());
				break;

			case NAV_CALENDAR:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_CalendarScreen());
				break;

			case NAV_DOSSIER:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_DossierScreen());
				break;

			case NAV_MEMBERS:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_MembersScreen());
				break;

			case NAV_QUALS:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_QualCatalogueScreen());
				break;

			case NAV_EXERCISES:
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_ExerciseCatalogueScreen());
				break;
		}
	}
}
