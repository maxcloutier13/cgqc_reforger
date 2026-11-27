// Gestion d'unité - pause menu overview panel (spec §4), Phase 1 content: header, command notice,
// unit news, next rank and OUVRIR MON DOSSIER. The operation section stays empty until Phase 5;
// seat / role / team leader arrive with the ORBAT (Phase 4).

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
	protected TextWidget m_wPhase;
	protected TextWidget m_wOnline;
	protected TextWidget m_wNotice;
	protected Widget m_wNoticeBar;
	protected Widget m_wNewsList;
	protected TextWidget m_wNextRank;
	protected TextWidget m_wNextCriteria;
	protected Widget m_wNextPanel;
	protected ref CGQC_GU_Button m_DossierButton;
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

		Widget dossierSlot = m_wRoot.FindAnyWidget("DossierSlot");
		if (dossierSlot)
		{
			m_DossierButton = CGQC_GU_Button.Create(dossierSlot, "OUVRIR MON DOSSIER", CGQC_GU_EButtonKind.PRIMARY);
			if (m_DossierButton)
				m_DossierButton.GetOnActivated().Insert(OnOpenDossier);
		}

		ApplyLightState();
		CGQC_GU_Client.GetOnLightState().Insert(ApplyLightState);
		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		m_iRequestId = CGQC_GU_Client.Request(CGQC_GU_Requests.OVERVIEW);
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		CGQC_GU_Client.GetOnLightState().Remove(ApplyLightState);
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		if (m_DossierButton)
		{
			m_DossierButton.Destroy();
			m_DossierButton = null;
		}

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
	//! Live values pushed by the server (phase, status, notice, players online).
	protected void ApplyLightState()
	{
		CGQC_GU_LightState s = CGQC_GU_Client.GetLightState();
		if (s.m_iSerial < 0)
			return;

		if (m_wOnline)
			m_wOnline.SetText(string.Format("%1 en ligne", s.m_iOnline));

		SetNotice(s.m_sNotice);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetNotice(string text)
	{
		if (m_wNotice)
			m_wNotice.SetText(text);

		// No notice: the bar is not shown at all.
		if (m_wNoticeBar)
			m_wNoticeBar.SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (requestId != m_iRequestId || !m_wRoot)
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
		if (m_wPhase) m_wPhase.SetText(dto.phase);
		if (m_wOnline) m_wOnline.SetText(string.Format("%1 en ligne", dto.online));
		SetNotice(dto.notice);

		if (m_wNextPanel)
			m_wNextPanel.SetVisible(!dto.nextRank.IsEmpty());

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
