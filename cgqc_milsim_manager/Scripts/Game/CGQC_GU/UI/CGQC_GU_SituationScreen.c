// Gestion d'unité - Situation (Phase 2c, spec §5), after the approved mockup: header with the current
// phase, « depuis », STATUT and EN LIGNE tiles; the 5 phase cards with ACTIVER (double press) and the
// Qualification box on ENTRAÎNEMENT; STATUT DE L'UNITÉ; JOURNAL DES CHANGEMENTS. Redraws live.

class CGQC_GU_SituationAction
{
	protected CGQC_GU_SituationScreen m_Screen;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_SituationAction(CGQC_GU_SituationScreen screen, string kind, string value)
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

class CGQC_GU_SituationScreen : CGQC_GU_DarkScreen
{
	static const int LOG_PAGE = 9;
	static const float CARD_HEIGHT = 230;

	protected ref CGQC_GU_SituationDto m_Dto;
	protected ref CGQC_GU_Region m_Body = new CGQC_GU_Region();
	protected CGQC_GU_Check m_QualCheck;
	protected bool m_bQualPick;
	protected int m_iLogPage;
	protected int m_iReadReq;
	protected int m_iWriteReq;
	protected string m_sMessage;

	//------------------------------------------------------------------------------------------------
	override protected int GetNavId()
	{
		return NAV_SITUATION;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildDark()
	{
		m_Body.m_wRoot = m_wContent;
		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		CGQC_GU_Client.GetOnLightState().Insert(OnLightState);
		RequestSituation();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		CGQC_GU_Client.GetOnLightState().Remove(OnLightState);
		GetGame().GetCallqueue().Remove(BuildAll);
		m_Body.Reset();
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestSituation()
	{
		m_iReadReq = CGQC_GU_Client.Request(CGQC_GU_Requests.SITUATION);
	}

	//------------------------------------------------------------------------------------------------
	//! Another Zeus changed something, or players came and went: read again.
	protected void OnLightState()
	{
		RequestSituation();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot)
			return;

		if (requestId != m_iReadReq && requestId != m_iWriteReq)
			return;

		if (!ok)
		{
			m_sMessage = CGQC_GU_DossierScreen.ErrorText(code);
			if (m_Dto)
				BuildAll();

			return;
		}

		JsonLoadContext ctx = new JsonLoadContext();
		CGQC_GU_SituationDto dto = new CGQC_GU_SituationDto();
		if (!ctx.LoadFromString(payload) || !ctx.ReadValue("s", dto))
			return;

		bool first = !m_Dto;
		m_Dto = dto;
		if (first || requestId == m_iWriteReq)
		{
			m_bQualPick = dto.qualification;
			m_sMessage = string.Empty;
		}

		BuildAll();
	}

	//------------------------------------------------------------------------------------------------
	protected void Write(CGQC_GU_Payload p)
	{
		m_iWriteReq = CGQC_GU_Client.RequestWith(CGQC_GU_Requests.SITUATION_SET, p);
	}

	//------------------------------------------------------------------------------------------------
	void OnAction(string kind, string value)
	{
		if (!m_Dto)
			return;

		switch (kind)
		{
			case "phase":
			{
				CGQC_GU_Payload p = new CGQC_GU_Payload();
				p.Set("phase", value);
				if (value == "entrainement")
					p.Set("qualification", BoolText(m_bQualPick));

				Write(p);
				break;
			}

			case "qual":
			{
				CGQC_GU_Payload q = new CGQC_GU_Payload();
				q.Set("qualification", BoolText(m_bQualPick));
				Write(q);
				break;
			}

			case "status":
			{
				CGQC_GU_Payload s = new CGQC_GU_Payload();
				s.Set("unitStatus", value);
				Write(s);
				break;
			}

			case "log":
				m_iLogPage = value.ToInt();
				ScheduleBuild();
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string BoolText(bool v)
	{
		if (v)
			return "1";

		return "0";
	}

	//------------------------------------------------------------------------------------------------
	protected void OnQualToggled()
	{
		if (m_QualCheck)
			m_bQualPick = m_QualCheck.IsChecked();

		// Owner (2c test): while ENTRAÎNEMENT runs, the box switches to / from QUALIFICATION at once.
		// Otherwise it is only the value ACTIVER sends.
		if (m_Dto && m_Dto.canPhase && m_Dto.phase == "entrainement" && m_bQualPick != m_Dto.qualification)
		{
			OnAction("qual", string.Empty);
			return;
		}

		ScheduleBuild();
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuild on the next frame: the widget that was clicked must not be destroyed in its own click.
	protected void ScheduleBuild()
	{
		GetGame().GetCallqueue().Remove(BuildAll);
		GetGame().GetCallqueue().CallLater(BuildAll, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	// Drawing
	//------------------------------------------------------------------------------------------------
	protected void BuildAll()
	{
		if (!m_wRoot || !m_Dto || !m_wContent)
			return;

		m_Body.Reset();
		m_QualCheck = null;

		SetNavNext(m_Dto.nextEvent);
		BuildHeader();

		TextWidget phasesCap = Caption(m_wContent, "PHASES");
		if (phasesCap)
			LayoutSlot.SetPadding(phasesCap, 0, 20, 0, 8);

		BuildPhases();
		BuildBottom();
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildHeader()
	{
		Widget head = CGQC_GU_W.DarkPanel(m_wContent, CGQC_GU_Style.DarkCard(), 28, 24);
		Widget row = CGQC_GU_W.HBox(head);

		Widget left = CGQC_GU_W.VBox(row);
		LayoutSlot.SetSizeMode(left, LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(left, 1);
		LayoutSlot.SetVerticalAlign(left, LayoutVerticalAlign.Center);

		Caption(left, "SITUATION");
		TextWidget phase = CGQC_GU_W.Text(left, CGQC_GU_SituationBuilder.DisplayPhase(m_Dto.phase, m_Dto.qualification), CGQC_GU_Style.FONT_TITLE, 44, CGQC_GU_Style.DarkText());
		if (phase)
			LayoutSlot.SetPadding(phase, 0, 4, 0, 4);

		if (!m_Dto.since.IsEmpty())
			CGQC_GU_W.Text(left, m_Dto.since, CGQC_GU_Style.FONT_LABEL, 19, CGQC_GU_Style.DarkSoft());

		if (!m_sMessage.IsEmpty())
		{
			TextWidget msg = CGQC_GU_W.Text(left, m_sMessage, CGQC_GU_Style.FONT_LABEL, 18, Color.FromSRGBA(0xE0, 0x7A, 0x6E, 255));
			if (msg)
				LayoutSlot.SetPadding(msg, 0, 6, 0, 0);
		}

		Tile(row, CGQC_GU_SituationBuilder.UnitStatusName(m_Dto.unitStatus), "STATUT", CGQC_GU_Style.BlueLighter());
		Tile(row, m_Dto.online.ToString(), "EN LIGNE", CGQC_GU_Style.DarkText());
	}

	//------------------------------------------------------------------------------------------------
	protected void Tile(Widget row, string value, string caption, Color color)
	{
		Widget box = CGQC_GU_W.Sized(row, 170, 100);
		if (!box)
			return;

		LayoutSlot.SetPadding(box.GetParent(), 12, 0, 0, 0);
		LayoutSlot.SetVerticalAlign(box.GetParent(), LayoutVerticalAlign.Center);
		Widget tile = CGQC_GU_W.DarkPanel(box, CGQC_GU_Style.DarkTile(), 16, 18);
		Stretch(tile);
		CGQC_GU_W.Text(tile, value, CGQC_GU_Style.FONT_TITLE, 30, color);
		CGQC_GU_W.Text(tile, caption, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.DarkMuted());
	}

	//------------------------------------------------------------------------------------------------
	//! Makes a DarkPanel fill its parent stack vertically.
	protected void Stretch(Widget panelInner)
	{
		if (!panelInner || !panelInner.GetParent())
			return;

		LayoutSlot.SetSizeMode(panelInner.GetParent(), LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(panelInner.GetParent(), 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildPhases()
	{
		Widget row = CGQC_GU_W.HBox(m_wContent);
		int count = m_Dto.phases.Count();
		for (int i = 0; i < count; i++)
		{
			string code = m_Dto.phases[i];
			bool current = code == m_Dto.phase;

			Widget box = CGQC_GU_W.Sized(row, 0, CARD_HEIGHT);
			if (!box)
				continue;

			Widget sized = box.GetParent();
			LayoutSlot.SetSizeMode(sized, LayoutSizeMode.Fill);
			LayoutSlot.SetFillWeight(sized, 1);
			if (i < count - 1)
				LayoutSlot.SetPadding(sized, 0, 0, 10, 0);

			Color bg = CGQC_GU_Style.DarkCard();
			Color nameColor = CGQC_GU_Style.DarkText();
			if (current)
			{
				bg = CGQC_GU_Style.DarkActiveCard();
				nameColor = CGQC_GU_Style.BlueLighter();
			}

			Widget card = CGQC_GU_W.DarkPanel(box, bg, 20, 20);
			Stretch(card);

			CGQC_GU_W.Text(card, m_Dto.phaseLabels[i], CGQC_GU_Style.FONT_TITLE, 24, nameColor);
			TextWidget desc = CGQC_GU_W.Text(card, m_Dto.phaseTexts[i], CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkSoft(), true);
			if (desc)
			{
				LayoutSlot.SetPadding(desc, 0, 10, 0, 10);
				LayoutSlot.SetSizeMode(desc, LayoutSizeMode.Fill);
				LayoutSlot.SetFillWeight(desc, 1);
			}

			bool training = code == "entrainement";
			if (training)
			{
				m_QualCheck = CGQC_GU_Check.Create(card, "qual", "Qualification", m_bQualPick);
				if (m_QualCheck)
				{
					m_Body.m_aChecks.Insert(m_QualCheck);
					m_QualCheck.SetDark();
					m_QualCheck.SetEnabled(m_Dto.canPhase);
					m_QualCheck.GetOnChanged().Insert(OnQualToggled);
					LayoutSlot.SetPadding(m_QualCheck.GetRootWidget(), 0, 0, 0, 10);
				}
			}

			if (current)
			{
				CGQC_GU_Button on = CardButton(card, "EN COURS", string.Empty, code, false);
				if (on)
					on.SetSelected(true);

				continue;
			}

			CGQC_GU_Button b = CardButton(card, "ACTIVER", "phase", code, true);
			if (b)
			{
				b.SetConfirmText("CONFIRMER");
				if (!m_Dto.canPhase)
					b.SetEnabled(false);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button CardButton(Widget parent, string text, string kind, string value, bool confirm)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text, CGQC_GU_EButtonKind.PLAIN, confirm);
		if (!b)
			return null;

		m_Body.m_aButtons.Insert(b);
		b.SetDark();
		if (!kind.IsEmpty())
		{
			CGQC_GU_SituationAction a = new CGQC_GU_SituationAction(this, kind, value);
			m_Body.m_aKeep.Insert(a);
			b.GetOnActivated().Insert(a.Fire);
		}

		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildBottom()
	{
		Widget row = CGQC_GU_W.HBox(m_wContent);
		LayoutSlot.SetPadding(row, 0, 14, 0, 0);
		LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(row, 1);

		// STATUT DE L'UNITÉ
		Widget left = HalfPanel(row, true);
		Caption(left, "STATUT DE L'UNITÉ");
		Widget buttons = CGQC_GU_W.HBox(left);
		LayoutSlot.SetPadding(buttons, 0, 16, 0, 16);
		StatusButton(buttons, "garnison");
		StatusButton(buttons, "deploye");
		CGQC_GU_W.Text(left, "Garnison : l'unité est à la base. Déployé : l'unité est en campagne, les règles de tenue en déploiement s'appliquent.", CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkSoft(), true);

		// JOURNAL DES CHANGEMENTS
		Widget right = HalfPanel(row, false);
		Caption(right, "JOURNAL DES CHANGEMENTS");
		Line(right, 12, 12);

		int total = m_Dto.log.Count();
		int pages = (total + LOG_PAGE - 1) / LOG_PAGE;
		if (m_iLogPage >= pages)
			m_iLogPage = Math.Max(pages - 1, 0);

		int start = m_iLogPage * LOG_PAGE;
		for (int i = start; i < total && i < start + LOG_PAGE; i++)
		{
			CGQC_GU_SituationLogLine line = m_Dto.log[i];
			Widget lr = CGQC_GU_W.HBox(right);
			LayoutSlot.SetPadding(lr, 0, 0, 0, 10);
			Widget whenBox = CGQC_GU_W.Sized(lr, 130);
			CGQC_GU_W.Text(whenBox, line.when, CGQC_GU_Style.FONT_LABEL, 18, CGQC_GU_Style.DarkMuted());
			CGQC_GU_W.Text(lr, line.text, CGQC_GU_Style.FONT_LABEL, 18, CGQC_GU_Style.DarkText());
		}

		if (pages > 1)
		{
			Widget pager = CGQC_GU_W.HBox(right);
			LayoutSlot.SetPadding(pager, 0, 6, 0, 0);
			CGQC_GU_Button prev = PagerButton(pager, "<", m_iLogPage - 1);
			if (prev && m_iLogPage == 0)
				prev.SetEnabled(false);

			TextWidget pg = CGQC_GU_W.Text(pager, string.Format("%1 / %2", m_iLogPage + 1, pages), CGQC_GU_Style.FONT_LABEL, 16, CGQC_GU_Style.DarkMuted());
			if (pg)
			{
				LayoutSlot.SetPadding(pg, 12, 0, 12, 0);
				LayoutSlot.SetVerticalAlign(pg, LayoutVerticalAlign.Center);
			}

			CGQC_GU_Button next = PagerButton(pager, ">", m_iLogPage + 1);
			if (next && m_iLogPage >= pages - 1)
				next.SetEnabled(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected Widget HalfPanel(Widget row, bool first)
	{
		Widget holder = CGQC_GU_W.VBox(row);
		LayoutSlot.SetSizeMode(holder, LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(holder, 1);
		LayoutSlot.SetVerticalAlign(holder, LayoutVerticalAlign.Stretch);
		if (first)
			LayoutSlot.SetPadding(holder, 0, 0, 14, 0);

		Widget panel = CGQC_GU_W.DarkPanel(holder, CGQC_GU_Style.DarkCard(), 28, 22);
		Stretch(panel);
		return panel;
	}

	//------------------------------------------------------------------------------------------------
	protected void StatusButton(Widget row, string code)
	{
		Widget box = CGQC_GU_W.Sized(row, 170);
		if (!box)
			return;

		LayoutSlot.SetPadding(box.GetParent(), 0, 0, 10, 0);
		bool current = code == m_Dto.unitStatus;
		string kind;
		if (!current)
			kind = "status";

		CGQC_GU_Button b = CardButton(box, CGQC_GU_SituationBuilder.UnitStatusName(code), kind, code, !current);
		if (!b)
			return;

		b.SetConfirmText("CONFIRMER");
		b.SetSelected(current);
		if (!current && !m_Dto.canPhase)
			b.SetEnabled(false);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button PagerButton(Widget row, string text, int page)
	{
		Widget box = CGQC_GU_W.Sized(row, 56);
		if (!box)
			return null;

		return CardButton(box, text, "log", page.ToString(), false);
	}
}
