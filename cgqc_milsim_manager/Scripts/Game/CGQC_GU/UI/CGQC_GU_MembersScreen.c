// Gestion d'unité - Unité → Membres (paper), spec §5 "Unité (paper)", Phase 1e.
// The member table (online, name, rank, access, last connection, status, profile), the filters,
// « + Nouveau dossier », the manual status and « Lier un profil » (typed UID or merge of an auto-created
// Cadet dossier). Everything shown comes from the server's "member.list" reply; every write is a double
// press and is checked again on the server. After a write the list is read again.

//! Binds a button to a named action of the Membres screen, with an optional value.
class CGQC_GU_MembersAction
{
	protected CGQC_GU_MembersScreen m_Screen;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_MembersAction(CGQC_GU_MembersScreen screen, string kind, string value)
	{
		m_Screen = screen;
		m_sKind = kind;
		m_sValue = value;
	}

	//------------------------------------------------------------------------------------------------
	void Fire()
	{
		if (m_Screen)
			m_Screen.OnAction(m_sKind, m_sValue);
	}
}

class CGQC_GU_MembersScreen : CGQC_GU_PaperScreen
{
	static const int ROW_PAGE = 12;
	static const int MERGE_PAGE = 8;

	static const int PANEL_NONE = 0;
	static const int PANEL_CREATE = 1;
	static const int PANEL_LINK = 2;
	static const int PANEL_STATUS = 3;

	// Filters: id and label, in display order.
	protected ref array<string> m_aFilters = { "tous", "enligne", "actif", "inactif", "mia", "kia" };
	protected ref array<string> m_aFilterLabels = { "TOUS", "EN LIGNE", "ACTIFS", "INACTIFS", "MIA", "KIA" };
	protected ref array<string> m_aStatuses = { "auto", "actif", "inactif", "mia", "kia" };

	protected ref array<ref CGQC_GU_MemberSummary> m_aMembers;
	protected ref array<ref CGQC_GU_RankChoice> m_aRanks;
	protected bool m_bCanEdit;
	protected string m_sEntryRank;

	protected int m_iListReq;
	protected int m_iWriteReq;

	protected string m_sFilter = "tous";
	protected int m_iPage;
	protected int m_iPanel = PANEL_NONE;
	protected string m_sTarget;		// member the open panel acts on (link / status)
	protected int m_iMergePage;

	// Text kept across panel rebuilds (rank pick and paging rebuild the panel)
	protected string m_sCreateRank;
	protected string m_sCreateLastName;
	protected string m_sCreateDiscord;
	protected string m_sCreateEnlisted;
	protected string m_sLinkUid;

	protected ref CGQC_GU_Region m_Filters = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Actions = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Panel = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_List = new CGQC_GU_Region();
	protected TextWidget m_wMessage;
	protected CGQC_GU_Button m_RailMembers;
	protected ref CGQC_GU_FieldWatch m_SearchWatch;

	protected EditBoxWidget m_wSearch;
	protected EditBoxWidget m_wLastName;
	protected EditBoxWidget m_wDiscord;
	protected EditBoxWidget m_wEnlisted;
	protected EditBoxWidget m_wLinkUid;

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildPaper()
	{
		SetHeader("UNITÉ", "MEMBRES");

		Widget c = m_wContent;
		if (!c)
			return;

		// Toolbar: filters | search | + Nouveau dossier
		Widget bar = CGQC_GU_W.HBox(c);
		if (bar)
		{
			LayoutSlot.SetHorizontalAlign(bar, LayoutHorizontalAlign.Stretch);
			m_Filters.m_wRoot = CGQC_GU_W.HBox(bar);
			if (m_Filters.m_wRoot)
				LayoutSlot.SetVerticalAlign(m_Filters.m_wRoot, LayoutVerticalAlign.Center);

			TextWidget cap = CGQC_GU_W.Text(bar, "RECHERCHE", CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
			if (cap)
			{
				LayoutSlot.SetVerticalAlign(cap, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(cap, 28, 0, 10, 0);
			}

			Widget searchBox = CGQC_GU_W.VBox(bar);
			if (searchBox)
				LayoutSlot.SetVerticalAlign(searchBox, LayoutVerticalAlign.Center);

			m_wSearch = CGQC_GU_W.Field(searchBox, "", 300);
			if (m_wSearch)
			{
				m_SearchWatch = new CGQC_GU_FieldWatch();
				m_SearchWatch.GetOnChanged().Insert(OnSearchChanged);
				m_wSearch.AddHandler(m_SearchWatch);
			}

			m_Actions.m_wRoot = CGQC_GU_W.HBox(bar);
			if (m_Actions.m_wRoot)
			{
				LayoutSlot.SetVerticalAlign(m_Actions.m_wRoot, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(m_Actions.m_wRoot, 28, 0, 0, 0);
			}
		}

		CGQC_GU_W.Rule(c, 10, 4);

		m_wMessage = CGQC_GU_W.Text(c, "", CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.Red());
		if (m_wMessage)
			LayoutSlot.SetPadding(m_wMessage, 0, 4, 0, 6);

		m_Panel.m_wRoot = CGQC_GU_W.VBox(c);
		if (m_Panel.m_wRoot)
			LayoutSlot.SetHorizontalAlign(m_Panel.m_wRoot, LayoutHorizontalAlign.Stretch);

		m_List.m_wRoot = CGQC_GU_W.VBox(c);
		if (m_List.m_wRoot)
			LayoutSlot.SetHorizontalAlign(m_List.m_wRoot, LayoutHorizontalAlign.Stretch);

		// Same rail as the Dossier: REGISTRE and GRADE & ACCÈS open the own dossier on that tab.
		AddRailEntry("REGISTRE", CGQC_GU_DossierScreen.TAB_REGISTRE);
		AddRailEntry("GRADE & ACCÈS", CGQC_GU_DossierScreen.TAB_GRADE);
		m_RailMembers = AddRailEntry("MEMBRES", CGQC_GU_DossierScreen.TAB_MEMBERS);
		if (m_RailMembers)
			m_RailMembers.SetSelected(true);

		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		RequestList();
		BuildAll();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		if (m_wSearch && m_SearchWatch)
			m_wSearch.RemoveHandler(m_SearchWatch);

		m_Filters.Reset();
		m_Actions.Reset();
		m_Panel.Reset();
		m_List.Reset();
	}

	//------------------------------------------------------------------------------------------------
	override void OnRailSelected(int index)
	{
		if (index != CGQC_GU_DossierScreen.TAB_MEMBERS)
		{
			CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_DossierScreen("", index));
			return;
		}

		// MEMBRES: back to the plain list
		m_iPanel = PANEL_NONE;
		SetMessage("", false);
		BuildAll();
	}

	//------------------------------------------------------------------------------------------------
	// Requests
	//------------------------------------------------------------------------------------------------
	protected void RequestList()
	{
		m_iListReq = CGQC_GU_Client.Request(CGQC_GU_Requests.MEMBER_LIST);
	}

	//------------------------------------------------------------------------------------------------
	protected void Write(string action, CGQC_GU_Payload p)
	{
		SetMessage("", false);
		m_iWriteReq = CGQC_GU_Client.RequestWith(action, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot)
			return;

		if (requestId == m_iListReq)
		{
			if (!ok)
			{
				SetMessage(CGQC_GU_DossierScreen.ErrorText(code), false);
				return;
			}

			JsonLoadContext ctx = new JsonLoadContext();
			array<ref CGQC_GU_MemberSummary> members = new array<ref CGQC_GU_MemberSummary>();
			array<ref CGQC_GU_RankChoice> ranks = new array<ref CGQC_GU_RankChoice>();
			if (!ctx.LoadFromString(payload) || !ctx.ReadValue("members", members))
				return;

			ctx.ReadValue("ranks", ranks);
			bool canEdit;
			ctx.ReadValue("canEdit", canEdit);
			string entryRank;
			ctx.ReadValue("entryRank", entryRank);

			m_aMembers = members;
			m_aRanks = ranks;
			m_bCanEdit = canEdit;
			m_sEntryRank = entryRank;
			if (!m_bCanEdit)
				m_iPanel = PANEL_NONE;

			// The target of an open panel may be gone (merged) or now linked.
			if (m_iPanel == PANEL_LINK || m_iPanel == PANEL_STATUS)
			{
				CGQC_GU_MemberSummary t = FindMember(m_sTarget);
				if (!t || (m_iPanel == PANEL_LINK && t.uidLinked))
					m_iPanel = PANEL_NONE;
			}

			BuildAll();
			return;
		}

		if (requestId == m_iWriteReq)
		{
			if (!ok)
			{
				SetMessage(CGQC_GU_DossierScreen.ErrorText(code), false);
				return;
			}

			string done = "Enregistré.";
			if (m_iPanel == PANEL_CREATE)
			{
				done = "Dossier créé.";
				m_sCreateLastName = string.Empty;
				m_sCreateDiscord = string.Empty;
				m_sCreateEnlisted = string.Empty;
			}
			else if (m_iPanel == PANEL_LINK)
			{
				done = "Profil lié.";
				m_sLinkUid = string.Empty;
			}

			m_iPanel = PANEL_NONE;
			SetMessage(done, true);
			RequestList();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetMessage(string text, bool good)
	{
		if (!m_wMessage)
			return;

		m_wMessage.SetText(text);
		if (good)
			m_wMessage.SetColor(CGQC_GU_Style.Online());
		else
			m_wMessage.SetColor(CGQC_GU_Style.Red());
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_MemberSummary FindMember(string uid)
	{
		if (!m_aMembers || uid.IsEmpty())
			return null;

		foreach (CGQC_GU_MemberSummary m : m_aMembers)
		{
			if (m.uid == uid)
				return m;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected string NameOf(CGQC_GU_MemberSummary m)
	{
		if (m.rankAbbr.IsEmpty())
			return m.displayName;

		return m.rankAbbr + " " + m.displayName;
	}

	//------------------------------------------------------------------------------------------------
	// Actions
	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button Btn(CGQC_GU_Region region, Widget parent, string text, string kind, string value = "", bool confirm = false, CGQC_GU_EButtonKind look = CGQC_GU_EButtonKind.PLAIN)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text, look, confirm);
		if (!b)
			return null;

		region.m_aButtons.Insert(b);
		b.SetCompact();
		if (!kind.IsEmpty())
		{
			CGQC_GU_MembersAction a = new CGQC_GU_MembersAction(this, kind, value);
			region.m_aKeep.Insert(a);
			b.GetOnActivated().Insert(a.Fire);
		}

		return b;
	}

	//------------------------------------------------------------------------------------------------
	void OnAction(string kind, string value)
	{
		switch (kind)
		{
			case "filter":
				m_sFilter = value;
				m_iPage = 0;
				BuildFilters();
				FillList();
				break;

			case "page":
				m_iPage = value.ToInt();
				FillList();
				break;

			case "open":
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_DossierScreen(value));
				break;

			case "panel.close":
				KeepPanelText();
				m_iPanel = PANEL_NONE;
				BuildAll();
				break;

			case "create.open":
				KeepPanelText();
				m_iPanel = PANEL_CREATE;
				if (m_sCreateRank.IsEmpty())
					m_sCreateRank = m_sEntryRank;
				SetMessage("", false);
				BuildAll();
				break;

			case "create.rank":
				KeepPanelText();
				m_sCreateRank = value;
				BuildPanel();
				break;

			case "create.save":
				Create();
				break;

			case "link.open":
				KeepPanelText();
				if (m_sTarget != value)
					m_sLinkUid = string.Empty;
				m_iPanel = PANEL_LINK;
				m_sTarget = value;
				m_iMergePage = 0;
				SetMessage("", false);
				BuildAll();
				break;

			case "link.uid":
			{
				KeepPanelText();
				CGQC_GU_Payload lp = new CGQC_GU_Payload();
				lp.Set("uid", m_sTarget);
				lp.Set("bohemiaUid", m_sLinkUid.Trim());
				Write(CGQC_GU_Requests.MEMBER_LINK, lp);
				break;
			}

			case "link.merge":
			{
				CGQC_GU_Payload mp = new CGQC_GU_Payload();
				mp.Set("source", value);
				mp.Set("target", m_sTarget);
				Write(CGQC_GU_Requests.MEMBER_MERGE, mp);
				break;
			}

			case "merge.page":
				KeepPanelText();
				m_iMergePage = value.ToInt();
				BuildPanel();
				break;

			case "status.open":
				KeepPanelText();
				m_iPanel = PANEL_STATUS;
				m_sTarget = value;
				SetMessage("", false);
				BuildAll();
				break;

			case "status.set":
			{
				CGQC_GU_Payload sp = new CGQC_GU_Payload();
				sp.Set("uid", m_sTarget);
				sp.Set("status", value);
				Write(CGQC_GU_Requests.STATUS_SET, sp);
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Typed text survives a rebuild of the panel (rank pick, paging).
	protected void KeepPanelText()
	{
		if (m_wLastName) m_sCreateLastName = m_wLastName.GetText();
		if (m_wDiscord) m_sCreateDiscord = m_wDiscord.GetText();
		if (m_wEnlisted) m_sCreateEnlisted = m_wEnlisted.GetText();
		if (m_wLinkUid) m_sLinkUid = m_wLinkUid.GetText();
	}

	//------------------------------------------------------------------------------------------------
	protected void Create()
	{
		KeepPanelText();
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		p.Set("lastName", m_sCreateLastName);
		p.Set("rank", m_sCreateRank);
		p.Set("enlistedOn", m_sCreateEnlisted);
		p.Set("discordName", m_sCreateDiscord);
		Write(CGQC_GU_Requests.MEMBER_CREATE, p);
	}

	//------------------------------------------------------------------------------------------------
	//! Typing in the search box refreshes the list only, so the field keeps its focus.
	protected void OnSearchChanged()
	{
		m_iPage = 0;
		FillList();
	}

	//------------------------------------------------------------------------------------------------
	// Building
	//------------------------------------------------------------------------------------------------
	protected void BuildAll()
	{
		BuildFilters();
		BuildActions();
		BuildPanel();
		FillList();
	}

	//------------------------------------------------------------------------------------------------
	protected bool Matches(CGQC_GU_MemberSummary m, string filter)
	{
		switch (filter)
		{
			case "tous":	return true;
			case "enligne":	return m.online;
		}

		return m.status == filter;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildFilters()
	{
		m_Filters.Reset();
		Widget row = m_Filters.m_wRoot;
		if (!row)
			return;

		for (int i = 0; i < m_aFilters.Count(); i++)
		{
			string label = m_aFilterLabels[i];
			if (m_aMembers)
			{
				int n = 0;
				foreach (CGQC_GU_MemberSummary m : m_aMembers)
				{
					if (Matches(m, m_aFilters[i]))
						n++;
				}

				label = string.Format("%1 (%2)", label, n);
			}

			CGQC_GU_Button b = Btn(m_Filters, row, label, "filter", m_aFilters[i]);
			if (!b)
				continue;

			b.SetSelected(m_aFilters[i] == m_sFilter);
			if (i > 0)
				LayoutSlot.SetPadding(b.GetRootWidget(), 8, 0, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildActions()
	{
		m_Actions.Reset();
		if (!m_Actions.m_wRoot || !m_bCanEdit)
			return;

		CGQC_GU_Button b = Btn(m_Actions, m_Actions.m_wRoot, "+ NOUVEAU DOSSIER", "create.open", "", false, CGQC_GU_EButtonKind.PRIMARY);
		if (b)
			b.SetEnabled(m_iPanel != PANEL_CREATE);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildPanel()
	{
		m_Panel.Reset();
		m_wLastName = null;
		m_wDiscord = null;
		m_wEnlisted = null;
		m_wLinkUid = null;

		if (!m_Panel.m_wRoot || m_iPanel == PANEL_NONE || !m_aMembers)
			return;

		Widget panel = CGQC_GU_W.Panel(m_Panel.m_wRoot);
		if (!panel)
			return;

		LayoutSlot.SetHorizontalAlign(panel.GetParent(), LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(panel.GetParent(), 0, 0, 0, 14);

		switch (m_iPanel)
		{
			case PANEL_CREATE:	BuildCreate(panel); break;
			case PANEL_LINK:	BuildLink(panel); break;
			case PANEL_STATUS:	BuildStatus(panel); break;
		}

		CGQC_GU_Button close = Btn(m_Panel, panel, "ANNULER", "panel.close");
		if (close)
			LayoutSlot.SetPadding(close.GetRootWidget(), 0, 14, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void PanelTitle(Widget panel, string title, string who)
	{
		Widget row = CGQC_GU_W.HBox(panel);
		if (!row)
			return;

		CGQC_GU_W.Text(row, title, CGQC_GU_Style.FONT_TITLE, 22, CGQC_GU_Style.Ink());
		if (who.IsEmpty())
			return;

		TextWidget w = CGQC_GU_W.Text(row, who, CGQC_GU_Style.FONT_LABEL, 20, CGQC_GU_Style.InkSoft());
		if (w)
		{
			LayoutSlot.SetPadding(w, 16, 0, 0, 0);
			LayoutSlot.SetVerticalAlign(w, LayoutVerticalAlign.Bottom);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildCreate(Widget panel)
	{
		PanelTitle(panel, "NOUVEAU DOSSIER", "");

		Widget row = CGQC_GU_W.HBox(panel);
		if (row)
		{
			LayoutSlot.SetPadding(row, 0, 4, 0, 0);
			Widget a = CGQC_GU_W.VBox(row);
			CGQC_GU_W.Caption(a, "NOM DE FAMILLE");
			m_wLastName = CGQC_GU_W.Field(a, m_sCreateLastName, 320);

			Widget b = CGQC_GU_W.VBox(row);
			if (b)
				LayoutSlot.SetPadding(b, 20, 0, 0, 0);
			CGQC_GU_W.Caption(b, "NOM DISCORD");
			m_wDiscord = CGQC_GU_W.Field(b, m_sCreateDiscord, 320);

			Widget d = CGQC_GU_W.VBox(row);
			if (d)
				LayoutSlot.SetPadding(d, 20, 0, 0, 0);
			CGQC_GU_W.Caption(d, "ENRÔLÉ LE (AAAA-MM-JJ)");
			m_wEnlisted = CGQC_GU_W.Field(d, m_sCreateEnlisted, 200);
		}

		CGQC_GU_W.Caption(panel, "GRADE", 14);
		Widget ranks;
		int inRow;
		foreach (CGQC_GU_RankChoice r : m_aRanks)
		{
			if (!ranks || inRow == 8)
			{
				ranks = CGQC_GU_W.HBox(panel);
				inRow = 0;
				if (ranks)
					LayoutSlot.SetPadding(ranks, 0, 0, 0, 6);
			}

			if (!ranks)
				break;

			CGQC_GU_Button rb = Btn(m_Panel, ranks, r.abbr, "create.rank", r.id);
			if (rb)
			{
				rb.SetSelected(r.id == m_sCreateRank);
				if (inRow > 0)
					LayoutSlot.SetPadding(rb.GetRootWidget(), 6, 0, 0, 0);
			}

			inRow++;
		}

		CGQC_GU_Button save = Btn(m_Panel, panel, "CRÉER LE DOSSIER", "create.save", "", true, CGQC_GU_EButtonKind.PRIMARY);
		if (save)
			LayoutSlot.SetPadding(save.GetRootWidget(), 0, 12, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildLink(Widget panel)
	{
		CGQC_GU_MemberSummary target = FindMember(m_sTarget);
		if (!target)
			return;

		PanelTitle(panel, "LIER UN PROFIL", NameOf(target));

		// 1. By typed UID
		CGQC_GU_W.Caption(panel, "UID BOHEMIA", 14);
		Widget row = CGQC_GU_W.HBox(panel);
		if (row)
		{
			Widget f = CGQC_GU_W.VBox(row);
			m_wLinkUid = CGQC_GU_W.Field(f, m_sLinkUid, 440);
			CGQC_GU_Button link = Btn(m_Panel, row, "LIER LE PROFIL", "link.uid", "", true, CGQC_GU_EButtonKind.PRIMARY);
			if (link)
			{
				LayoutSlot.SetPadding(link.GetRootWidget(), 14, 0, 0, 0);
				LayoutSlot.SetVerticalAlign(link.GetRootWidget(), LayoutVerticalAlign.Center);
			}
		}

		// 2. Or merge a dossier created on first connection
		CGQC_GU_W.Caption(panel, "OU FUSIONNER UN DOSSIER CADET", 18);
		array<CGQC_GU_MemberSummary> candidates = new array<CGQC_GU_MemberSummary>();
		foreach (CGQC_GU_MemberSummary m : m_aMembers)
		{
			if (m.autoCreated && m.uid != target.uid)
				candidates.Insert(m);
		}

		if (candidates.IsEmpty())
		{
			CGQC_GU_W.Text(panel, "Aucun dossier cadet à fusionner.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
			return;
		}

		int pages = Math.Max(1, (candidates.Count() + MERGE_PAGE - 1) / MERGE_PAGE);
		m_iMergePage = Math.ClampInt(m_iMergePage, 0, pages - 1);
		int from = m_iMergePage * MERGE_PAGE;
		int to = Math.Min(from + MERGE_PAGE, candidates.Count());

		for (int i = from; i < to; i++)
		{
			CGQC_GU_MemberSummary c = candidates[i];
			Widget line = CGQC_GU_W.HBox(panel);
			if (!line)
				continue;

			LayoutSlot.SetPadding(line, 0, 2, 0, 2);
			OnlineDot(line, c.online);

			string seen = "Jamais connecté";
			if (c.online)
				seen = "EN LIGNE";
			else if (!c.lastSeen.IsEmpty())
				seen = "Vu le " + c.lastSeen;

			Cell(line, NameOf(c), 320, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
			Cell(line, seen, 200, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());

			CGQC_GU_Button merge = Btn(m_Panel, line, "FUSIONNER", "link.merge", c.uid, true, CGQC_GU_EButtonKind.PRIMARY);
			if (merge)
				merge.SetConfirmText("CONFIRMER");
		}

		Pager(m_Panel, panel, m_iMergePage, pages, "merge.page");
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildStatus(Widget panel)
	{
		CGQC_GU_MemberSummary target = FindMember(m_sTarget);
		if (!target)
			return;

		PanelTitle(panel, "STATUT", NameOf(target));
		TextWidget now = CGQC_GU_W.Text(panel, StatusText(target), CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		if (now)
			LayoutSlot.SetPadding(now, 0, 6, 0, 8);

		Widget row = CGQC_GU_W.HBox(panel);
		if (!row)
			return;

		bool manual = !target.statusBy.IsEmpty();
		for (int i = 0; i < m_aStatuses.Count(); i++)
		{
			string s = m_aStatuses[i];
			string label = CGQC_GU_DossierBuilder.StatusLabel(s);
			if (s == "auto")
				label = "AUTOMATIQUE";

			CGQC_GU_Button b = Btn(m_Panel, row, label, "status.set", s, true);
			if (!b)
				continue;

			b.SetConfirmText("CONFIRMER");
			if (s == "auto")
				b.SetSelected(!manual);
			else
				b.SetSelected(manual && target.status == s);

			if (i > 0)
				LayoutSlot.SetPadding(b.GetRootWidget(), 8, 0, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Status only; who changed it and when is in the member's Registre.
	protected string StatusText(CGQC_GU_MemberSummary m)
	{
		return CGQC_GU_DossierBuilder.StatusLabel(m.status);
	}

	//------------------------------------------------------------------------------------------------
	// Table
	//------------------------------------------------------------------------------------------------
	protected void FillList()
	{
		m_List.Reset();
		Widget list = m_List.m_wRoot;
		if (!list)
			return;

		// A panel replaces the table, as the member selector does in the Dossier.
		if (m_iPanel != PANEL_NONE)
			return;

		if (!m_aMembers)
		{
			CGQC_GU_W.Text(list, "Chargement…", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
			return;
		}

		string needle;
		if (m_wSearch)
			needle = CGQC_GU_Str.Lower(m_wSearch.GetText().Trim());

		array<CGQC_GU_MemberSummary> shown = new array<CGQC_GU_MemberSummary>();
		foreach (CGQC_GU_MemberSummary m : m_aMembers)
		{
			if (!Matches(m, m_sFilter))
				continue;

			if (!needle.IsEmpty())
			{
				string hay = CGQC_GU_Str.Lower(m.rankAbbr + " " + m.displayName + " " + m.lastName);
				if (!hay.Contains(needle))
					continue;
			}

			shown.Insert(m);
		}

		int pages = Math.Max(1, (shown.Count() + ROW_PAGE - 1) / ROW_PAGE);
		m_iPage = Math.ClampInt(m_iPage, 0, pages - 1);
		int from = m_iPage * ROW_PAGE;
		int to = Math.Min(from + ROW_PAGE, shown.Count());

		HeadingRow(list);
		if (shown.IsEmpty())
			CGQC_GU_W.Text(list, "Aucun membre.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());

		for (int i = from; i < to; i++)
		{
			MemberRow(list, shown[i]);
		}

		Pager(m_List, list, m_iPage, pages, "page");
	}

	//------------------------------------------------------------------------------------------------
	protected void HeadingRow(Widget list)
	{
		Widget row = CGQC_GU_W.HBox(list);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 2, 0, 6);
		ResourceName f = CGQC_GU_Style.FONT_TITLE;
		Color c = CGQC_GU_Style.InkMuted();
		Cell(row, "", 26, f, 14, c);
		Cell(row, "NOM", 340, f, 14, c);
		Cell(row, "GRADE", 110, f, 14, c);
		Cell(row, "ACCÈS", 180, f, 14, c);
		Cell(row, "DERNIÈRE CONNEXION", 200, f, 14, c);
		Cell(row, "STATUT", 300, f, 14, c);
		Cell(row, "PROFIL", 220, f, 14, c);
		CGQC_GU_W.Rule(list, 0, 2);
	}

	//------------------------------------------------------------------------------------------------
	protected void MemberRow(Widget list, CGQC_GU_MemberSummary m)
	{
		Widget row = CGQC_GU_W.HBox(list);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 3, 0, 3);

		// Online
		Widget dotBox = CGQC_GU_W.Sized(row, 26);
		if (dotBox)
		{
			LayoutSlot.SetVerticalAlign(dotBox.GetParent(), LayoutVerticalAlign.Center);
			OnlineDot(dotBox, m.online);
		}

		// Name: opens the dossier
		Widget nameBox = CellBox(row, 340);
		Btn(m_List, nameBox, m.displayName, "open", m.uid);

		CellText(row, m.rankAbbr, 110, CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.Ink());
		CellText(row, CGQC_GU_Str.Upper(m.accessLabel), 180, CGQC_GU_Style.FONT_LABEL, 16, CGQC_GU_Style.InkSoft());

		string seen = "—";
		if (m.online)
			seen = "EN LIGNE";
		else if (!m.lastSeen.IsEmpty())
			seen = m.lastSeen;

		CellText(row, seen, 200, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());

		// Status: staff change it here (manual override, KIA included)
		Color statusColor = CGQC_GU_Style.StatusColor(m.status);
		if (m_bCanEdit)
		{
			Widget statusBox = CellBox(row, 300);
			CGQC_GU_Button sb = Btn(m_List, statusBox, StatusText(m), "status.open", m.uid);
			if (sb)
				sb.SetTextColor(statusColor);
		}
		else
		{
			CellText(row, StatusText(m), 300, CGQC_GU_Style.FONT_TITLE, 15, statusColor);
		}

		// Profile: PROFIL NON LIÉ opens « Lier un profil » (staff)
		if (!m.uidLinked)
		{
			if (m_bCanEdit)
			{
				Widget linkBox = CellBox(row, 220);
				Btn(m_List, linkBox, "PROFIL NON LIÉ", "link.open", m.uid, false, CGQC_GU_EButtonKind.DANGER);
			}
			else
			{
				CellText(row, "PROFIL NON LIÉ", 220, CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Red());
			}
		}

		Widget sep = CGQC_GU_W.Rule(list, 0, 0);
		if (sep)
			sep.SetOpacity(0.25);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnlineDot(Widget parent, bool online)
	{
		Color dot = CGQC_GU_Style.Offline();
		if (online)
			dot = CGQC_GU_Style.Online();

		ImageWidget d = CGQC_GU_W.Dot(parent, 10, dot);
		if (d)
			LayoutSlot.SetPadding(d, 0, 0, 12, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected Widget CellBox(Widget row, float width)
	{
		Widget box = CGQC_GU_W.Sized(row, width);
		if (!box)
			return null;

		LayoutSlot.SetPadding(box.GetParent(), 0, 0, 12, 0);
		LayoutSlot.SetVerticalAlign(box.GetParent(), LayoutVerticalAlign.Center);
		return box;
	}

	//------------------------------------------------------------------------------------------------
	protected void CellText(Widget row, string text, float width, ResourceName font, int size, Color color)
	{
		Widget box = CellBox(row, width);
		if (!box)
			return;

		TextWidget t = CGQC_GU_W.Text(box, text, font, size, color, true);
		if (t)
			LayoutSlot.SetHorizontalAlign(t, LayoutHorizontalAlign.Stretch);
	}

	//------------------------------------------------------------------------------------------------
	protected void Cell(Widget row, string text, float width, ResourceName font, int size, Color color)
	{
		CellText(row, text, width, font, size, color);
	}

	//------------------------------------------------------------------------------------------------
	protected void Pager(CGQC_GU_Region region, Widget parent, int page, int pages, string kind)
	{
		if (pages <= 1)
			return;

		Widget row = CGQC_GU_W.HBox(parent);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 10, 0, 0);
		CGQC_GU_Button prev = Btn(region, row, "PRÉCÉDENTE", kind, (page - 1).ToString());
		if (prev)
			prev.SetEnabled(page > 0);

		TextWidget t = CGQC_GU_W.Text(row, string.Format("Page %1 / %2", page + 1, pages), CGQC_GU_Style.FONT_LABEL, 16, CGQC_GU_Style.InkMuted());
		if (t)
		{
			LayoutSlot.SetVerticalAlign(t, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(t, 14, 0, 14, 0);
		}

		CGQC_GU_Button next = Btn(region, row, "SUIVANTE", kind, (page + 1).ToString());
		if (next)
			next.SetEnabled(page < pages - 1);
	}
}
