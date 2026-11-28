// Gestion d'unité - pause menu overview panel (spec §4), Phase 1 content: header, command notice,
// unit news, next rank and OUVRIR MON DOSSIER. The operation section stays empty until Phase 5;
// seat · role · section and the team leader come from the ORBAT (4f).

class CGQC_GU_OverviewPanel
{
	static const ResourceName LAYOUT = "{CEB2C111D1A4EEE0}UI/layouts/CGQC_GU_Overview.layout";

	// Placement inside the pause menu: just past the end of its separator lines (~320 units).
	// Offsets are in reference units, scaled with the screen like the vanilla button column.
	static const float LEFT = 360;
	static const float MARGIN = 30;

	protected Widget m_wRoot;
	protected TextWidget m_wName;
	protected TextWidget m_wSub;
	protected TextWidget m_wLeaderLabel;	// « CHEF D'ÉQUIPE » (4f)
	protected TextWidget m_wLeaderName;		// « S1-0 Adju. Tremblay »
	protected TextWidget m_wPhase;
	protected TextWidget m_wOnline;
	protected TextWidget m_wNotice;
	protected Widget m_wNoticeBar;
	protected Widget m_wNewsList;
	protected TextWidget m_wNextRank;
	protected TextWidget m_wNextCriteria;
	protected Widget m_wNextPanel;
	protected ref CGQC_GU_Button m_DossierButton;
	protected ref CGQC_GU_Button m_CalendarButton;
	protected ref CGQC_GU_Button m_NoticeEdit;
	protected ref array<ref CGQC_GU_Button> m_aNoticeButtons = new array<ref CGQC_GU_Button>();
	protected Widget m_wNextEvent;
	protected TextWidget m_wNextEventText;
	protected Widget m_wNoticeEditor;
	protected EditBoxWidget m_wNoticeField;
	protected bool m_bEditingNotice;
	protected string m_sNotice;
	protected int m_iNoticeReq;
	protected int m_iRequestId;

	//------------------------------------------------------------------------------------------------
	void Create(Widget host)
	{
		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT, host);
		if (!m_wRoot)
		{
			CGQC_GU_Log.Error("Layout du panneau introuvable : " + LAYOUT);
			return;
		}

		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, LEFT, MARGIN, MARGIN, MARGIN);

		m_wName = TextWidget.Cast(m_wRoot.FindAnyWidget("Name"));
		m_wSub = TextWidget.Cast(m_wRoot.FindAnyWidget("Sub"));
		m_wLeaderLabel = TextWidget.Cast(m_wRoot.FindAnyWidget("LeaderLabel"));
		m_wLeaderName = TextWidget.Cast(m_wRoot.FindAnyWidget("LeaderName"));
		m_wPhase = TextWidget.Cast(m_wRoot.FindAnyWidget("Phase"));
		m_wOnline = TextWidget.Cast(m_wRoot.FindAnyWidget("Online"));
		m_wNotice = TextWidget.Cast(m_wRoot.FindAnyWidget("NoticeText"));
		m_wNoticeBar = m_wRoot.FindAnyWidget("Notice");
		m_wNewsList = m_wRoot.FindAnyWidget("NewsList");
		m_wNextRank = TextWidget.Cast(m_wRoot.FindAnyWidget("NextRank"));
		m_wNextCriteria = TextWidget.Cast(m_wRoot.FindAnyWidget("NextCriteria"));
		m_wNextPanel = m_wRoot.FindAnyWidget("NextPanel");

		// Colours from script (sRGB -> linear), like the paper shell: the layout's values are only placeholders.
		array<string> panels = { "HeaderBg", "OpBg", "NewsBg", "NextBg" };
		foreach (string panel : panels)
		{
			Widget bg = m_wRoot.FindAnyWidget(panel);
			if (bg)
				bg.SetColor(CGQC_GU_Style.DarkPanel());
		}

		Widget phaseBg = m_wRoot.FindAnyWidget("PhaseBg");
		if (phaseBg)
			phaseBg.SetColor(CGQC_GU_Style.PhaseTag());

		Widget noticeBg = m_wRoot.FindAnyWidget("NoticeBg");
		if (noticeBg)
			noticeBg.SetColor(CGQC_GU_Style.NoticeBlue());

		// Hidden until the server says there is a next rank.
		if (m_wNextPanel)
			m_wNextPanel.SetVisible(false);

		if (m_wNotice)
			m_wNotice.SetTextWrapping(true);

		if (m_wNextCriteria)
			m_wNextCriteria.SetTextWrapping(true);

		// OUVRIR MON DOSSIER, with CALENDRIER beside it (owner decision 3, Phase 2e).
		Widget dossierSlot = m_wRoot.FindAnyWidget("DossierSlot");
		if (dossierSlot)
		{
			Widget buttons = CGQC_GU_W.HBox(dossierSlot);
			m_DossierButton = CGQC_GU_Button.Create(buttons, "OUVRIR MON DOSSIER", CGQC_GU_EButtonKind.PRIMARY);
			if (m_DossierButton)
			{
				m_DossierButton.GetOnActivated().Insert(OnOpenDossier);
				LayoutSlot.SetSizeMode(m_DossierButton.GetRootWidget(), LayoutSizeMode.Fill);
				LayoutSlot.SetFillWeight(m_DossierButton.GetRootWidget(), 1);
			}

			m_CalendarButton = CGQC_GU_Button.Create(buttons, "CALENDRIER", CGQC_GU_EButtonKind.PLAIN);
			if (m_CalendarButton)
			{
				m_CalendarButton.SetDark();
				m_CalendarButton.GetOnActivated().Insert(OnOpenCalendar);
				LayoutSlot.SetPadding(m_CalendarButton.GetRootWidget(), 10, 0, 0, 0);
			}
		}

		BuildNextEvent();
		BuildNoticeEditor();

		ApplyLightState();
		CGQC_GU_Client.GetOnLightState().Insert(ApplyLightState);
		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		CGQC_GU_Ui.GetOnBackToPause().Insert(Refresh);
		m_iRequestId = CGQC_GU_Client.Request(CGQC_GU_Requests.OVERVIEW);
	}

	//------------------------------------------------------------------------------------------------
	//! Back from a Gestion d'unité screen: the seat or the leader may have changed there (4f).
	void Refresh()
	{
		if (m_wRoot)
			m_iRequestId = CGQC_GU_Client.Request(CGQC_GU_Requests.OVERVIEW);
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		GetGame().GetCallqueue().Remove(RefreshNotice);
		CGQC_GU_Client.GetOnLightState().Remove(ApplyLightState);
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		CGQC_GU_Ui.GetOnBackToPause().Remove(Refresh);
		if (m_DossierButton)
		{
			m_DossierButton.Destroy();
			m_DossierButton = null;
		}

		if (m_CalendarButton)
		{
			m_CalendarButton.Destroy();
			m_CalendarButton = null;
		}

		if (m_NoticeEdit)
		{
			m_NoticeEdit.Destroy();
			m_NoticeEdit = null;
		}

		ClearNoticeButtons();

		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpenDossier()
	{
		CGQC_GU_Ui.Open(new CGQC_GU_DossierScreen());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOpenCalendar()
	{
		CGQC_GU_Ui.Open(new CGQC_GU_CalendarScreen());
	}

	//------------------------------------------------------------------------------------------------
	//! « PROCHAIN AU CALENDRIER » panel, put at the top of the right column (spec §4). Hidden when empty.
	protected void BuildNextEvent()
	{
		Widget right = m_wRoot.FindAnyWidget("Right");
		if (!right)
			return;

		m_wNextEvent = CGQC_GU_W.DarkPanel(right, CGQC_GU_Style.DarkPanel(), 22, 16);
		if (!m_wNextEvent)
			return;

		Widget panel = m_wNextEvent.GetParent();
		LayoutSlot.SetPadding(panel, 0, 0, 0, 10);

		// First in the column: renumber the others after it.
		array<ref Widget> children = {};
		SCR_WidgetHelper.GetAllChildren(right, children);
		int order = 1;
		foreach (Widget w : children)
		{
			if (w == panel)
				continue;

			w.SetZOrder(order);
			order++;
		}

		panel.SetZOrder(0);
		CGQC_GU_W.Text(m_wNextEvent, "PROCHAIN AU CALENDRIER", CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.DarkMuted());
		m_wNextEventText = CGQC_GU_W.Text(m_wNextEvent, string.Empty, CGQC_GU_Style.FONT_LABEL, 19, CGQC_GU_Style.DarkText(), true);
		panel.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetNextEvent(string text)
	{
		if (!m_wNextEvent)
			return;

		if (m_wNextEventText)
			m_wNextEventText.SetText(text);

		m_wNextEvent.GetParent().SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	//! Notice edit button and inline editor (owner decision 5): next to the notice, for notice.edit only.
	protected void BuildNoticeEditor()
	{
		Widget row = m_wRoot.FindAnyWidget("NoticeRow");
		if (!row)
			return;

		m_NoticeEdit = CGQC_GU_Button.Create(row, "MODIFIER", CGQC_GU_EButtonKind.PLAIN);
		if (m_NoticeEdit)
		{
			m_NoticeEdit.SetDark();
			m_NoticeEdit.SetCompact();
			LayoutSlot.SetPadding(m_NoticeEdit.GetRootWidget(), 16, 0, 0, 0);
			LayoutSlot.SetVerticalAlign(m_NoticeEdit.GetRootWidget(), LayoutVerticalAlign.Center);
			m_NoticeEdit.GetOnActivated().Insert(OnNoticeEdit);
		}

		m_wNoticeEditor = CGQC_GU_W.HBox(row);
		if (m_wNoticeEditor)
		{
			LayoutSlot.SetSizeMode(m_wNoticeEditor, LayoutSizeMode.Fill);
			LayoutSlot.SetFillWeight(m_wNoticeEditor, 1);
			LayoutSlot.SetVerticalAlign(m_wNoticeEditor, LayoutVerticalAlign.Center);
			m_wNoticeEditor.SetVisible(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearNoticeButtons()
	{
		foreach (CGQC_GU_Button b : m_aNoticeButtons)
		{
			b.Destroy();
		}

		m_aNoticeButtons.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnNoticeEdit()
	{
		m_bEditingNotice = true;
		GetGame().GetCallqueue().CallLater(RefreshNotice, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnNoticeCancel()
	{
		m_bEditingNotice = false;
		GetGame().GetCallqueue().CallLater(RefreshNotice, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnNoticeSave()
	{
		string text;
		if (m_wNoticeField)
			text = m_wNoticeField.GetText();

		SendNotice(text);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnNoticeClear()
	{
		SendNotice(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendNotice(string text)
	{
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		p.Set("text", text);
		m_iNoticeReq = CGQC_GU_Client.RequestWith(CGQC_GU_Requests.NOTICE_SET, p);
	}

	//------------------------------------------------------------------------------------------------
	//! Live values pushed by the server (phase, status, notice, players online).
	protected void ApplyLightState()
	{
		CGQC_GU_LightState s = CGQC_GU_Client.GetLightState();
		if (s.m_iSerial < 0)
			return;

		if (m_wOnline)
			m_wOnline.SetText(string.Format("%1 en ligne", s.m_iOnline));

		SetNotice(s.m_sNotice);

		// A calendar change also re-sends the light state: read the panel again for the next event.
		m_iRequestId = CGQC_GU_Client.Request(CGQC_GU_Requests.OVERVIEW);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetNotice(string text)
	{
		m_sNotice = text;
		RefreshNotice();
	}

	//------------------------------------------------------------------------------------------------
	//! Notice bar: hidden when empty, except for editors, who always see it with MODIFIER.
	protected void RefreshNotice()
	{
		if (!m_wRoot)
			return;

		bool canEdit = CGQC_GU_Client.GetLightState().Can(CGQC_GU_LightState.CAN_NOTICE);
		if (!canEdit)
			m_bEditingNotice = false;

		if (m_wNotice)
		{
			m_wNotice.SetText(m_sNotice);
			m_wNotice.SetVisible(!m_bEditingNotice);
		}

		if (m_wNoticeBar)
			m_wNoticeBar.SetVisible(!m_sNotice.IsEmpty() || canEdit);

		if (m_NoticeEdit && m_NoticeEdit.GetRootWidget())
			m_NoticeEdit.GetRootWidget().SetVisible(canEdit && !m_bEditingNotice);

		if (!m_wNoticeEditor)
			return;

		// Already in the right state: keep what is being typed.
		if (m_bEditingNotice == (m_wNoticeField != null))
			return;

		ClearNoticeButtons();
		CGQC_GU_W.Clear(m_wNoticeEditor);
		m_wNoticeField = null;
		m_wNoticeEditor.SetVisible(m_bEditingNotice);
		if (!m_bEditingNotice)
			return;

		m_wNoticeField = CGQC_GU_W.Field(m_wNoticeEditor, m_sNotice, 620);
		NoticeButton("ENREGISTRER", true, CGQC_GU_EButtonKind.PRIMARY).GetOnActivated().Insert(OnNoticeSave);
		NoticeButton("EFFACER", true, CGQC_GU_EButtonKind.PLAIN).GetOnActivated().Insert(OnNoticeClear);
		NoticeButton("ANNULER", false, CGQC_GU_EButtonKind.PLAIN).GetOnActivated().Insert(OnNoticeCancel);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button NoticeButton(string text, bool confirm, CGQC_GU_EButtonKind kind)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(m_wNoticeEditor, text, kind, confirm);
		m_aNoticeButtons.Insert(b);
		if (kind == CGQC_GU_EButtonKind.PLAIN)
			b.SetDark();

		b.SetCompact();
		b.SetConfirmText("CONFIRMER");
		LayoutSlot.SetPadding(b.GetRootWidget(), 10, 0, 0, 0);
		LayoutSlot.SetVerticalAlign(b.GetRootWidget(), LayoutVerticalAlign.Center);
		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot)
			return;

		// Notice saved or cleared: the light state brings the new text; leave the editor.
		if (requestId == m_iNoticeReq)
		{
			if (ok)
			{
				m_bEditingNotice = false;
				GetGame().GetCallqueue().CallLater(RefreshNotice, 0, false);
			}

			return;
		}

		if (requestId != m_iRequestId)
			return;

		if (!ok)
		{
			if (m_wName)
				m_wName.SetText("Dossier introuvable");

			return;
		}

		CGQC_GU_OverviewDto dto = new CGQC_GU_OverviewDto();
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(payload) || !ctx.ReadValue("o", dto) || !dto)
			return;

		if (m_wName) m_wName.SetText(dto.name);
		if (m_wSub) m_wSub.SetText(dto.sub);
		if (m_wLeaderLabel) m_wLeaderLabel.SetText(dto.leaderLabel);
		if (m_wLeaderName) m_wLeaderName.SetText(dto.leader);
		if (m_wLeaderLabel) m_wLeaderLabel.SetVisible(!dto.leaderLabel.IsEmpty());
		if (m_wLeaderName) m_wLeaderName.SetVisible(!dto.leaderLabel.IsEmpty());
		if (m_wPhase) m_wPhase.SetText(dto.phase);
		if (m_wOnline) m_wOnline.SetText(string.Format("%1 en ligne", dto.online));
		SetNotice(dto.notice);

		if (m_wNextPanel)
			m_wNextPanel.SetVisible(!dto.nextRank.IsEmpty());

		SetNextEvent(dto.nextEvent);
		if (m_wNextRank) m_wNextRank.SetText(dto.nextRank);
		if (m_wNextCriteria) m_wNextCriteria.SetText(dto.nextCriteria);

		FillNews(dto.news);
	}

	//------------------------------------------------------------------------------------------------
	protected void FillNews(array<ref CGQC_GU_NewsLine> news)
	{
		if (!m_wNewsList)
			return;

		while (m_wNewsList.GetChildren())
		{
			m_wNewsList.GetChildren().RemoveFromHierarchy();
		}

		if (!news)
			return;

		foreach (CGQC_GU_NewsLine line : news)
		{
			TextWidget t = TextWidget.Cast(GetGame().GetWorkspace().CreateWidgets(CGQC_GU_Style.LAYOUT_TEXT, m_wNewsList));
			if (!t)
				continue;

			t.SetFont(CGQC_GU_Style.FONT_LABEL);
			t.SetExactFontSize(17);
			t.SetColor(CGQC_GU_Style.DarkText());
			t.SetTextWrapping(true);
			t.SetText(line.date + "    " + line.text);
			LayoutSlot.SetPadding(t, 0, 0, 0, 9);
		}
	}
}
