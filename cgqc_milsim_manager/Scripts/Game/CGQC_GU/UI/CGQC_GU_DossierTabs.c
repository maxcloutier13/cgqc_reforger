// Gestion d'unité - Dossier → Qualifications and Séances / FTX (paper), Phase 3d, after the approved
// mockup (Dossier-Qualifs, Dossier-Seances, Dossier-Insignes). The badges are on the Qualifications page,
// above the tables, and the whole page scrolls (owner, 3f test). Read-only: everything shown comes
// from the server's "dossier" reply. The tabs draw into the body region of the Dossier screen.

class CGQC_GU_DossierTabs
{
	static const int SESSION_PAGE = 10;
	static const int BADGES_PER_ROW = 6;
	static const float BADGE_WIDTH = 178;
	static const float BADGE_HEIGHT = 84;
	static const float BADGE_FRAME = 5;		// room around a badge for the outline of the chosen one

	protected CGQC_GU_DossierScreen m_Screen;
	protected CGQC_GU_Region m_Region;
	protected string m_sSessionKind;		// filter: "" = Tout, "ftx", "training"
	protected int m_iSessionPage;
	protected string m_sBadge;				// chosen qualification code

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_DossierTabs(CGQC_GU_DossierScreen screen, CGQC_GU_Region region)
	{
		m_Screen = screen;
		m_Region = region;
	}

	//------------------------------------------------------------------------------------------------
	//! Another member is shown: forget the page and the chosen badge.
	void Reset()
	{
		m_sSessionKind = string.Empty;
		m_iSessionPage = 0;
		m_sBadge = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! True when the body must be rebuilt.
	bool OnAction(string kind, string value)
	{
		switch (kind)
		{
			case "session.filter":
				m_sSessionKind = value;
				m_iSessionPage = 0;
				return true;

			case "session.page":
				m_iSessionPage = value.ToInt();
				return true;

			case "badge.pick":
				m_sBadge = value;
				return true;

			case "badge.qual":
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_QualCatalogueScreen(value));
				break;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Qualifications
	//------------------------------------------------------------------------------------------------
	void BuildQuals(Widget body, CGQC_GU_DossierDto dto)
	{
		Widget list = CGQC_GU_W.Scroll(body);
		if (!list)
			return;

		m_Screen.Section(list, "QUALIFICATIONS");
		Badges(list, dto);

		array<float> widths = { 120, 0, 130, 150, 100, 200 };
		Widget gap = CGQC_GU_W.Rule(list, 24, 12);
		if (gap)
			gap.SetOpacity(0.5);

		CGQC_GU_Table.Head(m_Region, list, { "CODE", "QUALIFICATION", "NIVEAU", "RÉSULTAT", "DATE", "ÉVALUÉ PAR" }, widths);
		foreach (CGQC_GU_DossierQualLine q : dto.quals)
		{
			Widget cells = CGQC_GU_Table.Row(m_Region, list);
			CGQC_GU_Table.Value(cells, 0, q.code, widths[0], true);
			CGQC_GU_Table.Value(cells, 1, q.title, widths[1]);
			CGQC_GU_Table.Value(cells, 2, q.level, widths[2]);

			Color tag = CGQC_GU_Style.InkSoft();
			if (q.kind == "ok")
				tag = CGQC_GU_Style.Blue();
			else if (q.kind == "ko")
				tag = CGQC_GU_Style.Red();

			CGQC_GU_W.Tag(CGQC_GU_Table.Stack(cells, 3, widths[3]), q.result, tag);
			CGQC_GU_Table.Value(cells, 4, q.date, widths[4]);
			CGQC_GU_Table.Value(cells, 5, q.evaluator, widths[5]);
		}

		if (dto.quals.IsEmpty())
			Empty(list, "Aucune qualification au dossier.");

		Widget title = m_Screen.Section(list, "EXERCICES RÉUSSIS");
		if (title)
			LayoutSlot.SetPadding(title, 0, 24, 0, 2);

		array<float> exWidths = { 130, 0, 150, 100, 200 };
		CGQC_GU_Table.Head(m_Region, list, { "CODE", "EXERCICE", "QUALIFICATION", "DATE", "INSTRUCTEUR" }, exWidths);
		foreach (CGQC_GU_DossierExerciseLine e : dto.exercisesDone)
		{
			Widget exCells = CGQC_GU_Table.Row(m_Region, list);
			CGQC_GU_Table.Value(exCells, 0, e.code, exWidths[0], true);
			CGQC_GU_Table.Value(exCells, 1, e.title, exWidths[1]);
			CGQC_GU_Table.Value(exCells, 2, e.qual, exWidths[2]);
			CGQC_GU_Table.Value(exCells, 3, e.date, exWidths[3]);
			CGQC_GU_Table.Value(exCells, 4, e.instructor, exWidths[4]);
		}

		if (dto.exercisesDone.IsEmpty())
			Empty(list, "Aucun exercice réussi.");
	}

	//------------------------------------------------------------------------------------------------
	protected void Empty(Widget parent, string text)
	{
		CGQC_GU_W.Para(parent, text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 8);
	}

	//------------------------------------------------------------------------------------------------
	// Séances / FTX
	//------------------------------------------------------------------------------------------------
	void BuildSessions(Widget body, CGQC_GU_DossierDto dto)
	{
		// Title, with the filter on its right
		Widget head = CGQC_GU_W.HBox(body);
		if (head)
		{
			LayoutSlot.SetHorizontalAlign(head, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(head, 0, 6, 0, 0);
			TextWidget title = CGQC_GU_W.Text(head, "SÉANCES ET FTX", CGQC_GU_Style.FONT_TITLE, 22, CGQC_GU_Style.Ink());
			if (title)
			{
				LayoutSlot.SetSizeMode(title, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(title, LayoutVerticalAlign.Bottom);
			}

			Filter(head, "TOUT", "");
			Filter(head, "FTX", "ftx");
			Filter(head, "ENTRAÎNEMENT", "training");
		}

		CGQC_GU_W.Rule(body, 6, 8);

		Widget list = CGQC_GU_W.Scroll(body);
		if (!list)
			return;

		array<CGQC_GU_DossierSessionLine> shown = new array<CGQC_GU_DossierSessionLine>();
		foreach (CGQC_GU_DossierSessionLine s : dto.sessionLines)
		{
			if (m_sSessionKind.IsEmpty() || s.kind == m_sSessionKind)
				shown.Insert(s);
		}

		int pages = Math.Max(1, (shown.Count() + SESSION_PAGE - 1) / SESSION_PAGE);
		m_iSessionPage = Math.ClampInt(m_iSessionPage, 0, pages - 1);
		int from = m_iSessionPage * SESSION_PAGE;
		int to = Math.Min(from + SESSION_PAGE, shown.Count());

		array<float> widths = { 0, 80, 100, 120, 200, 190 };
		CGQC_GU_Table.Head(m_Region, list, { "SÉANCE", "TYPE", "DATE", "QUALIF.", "EXERCICES", "INSTRUCTEUR" }, widths);
		for (int i = from; i < to; i++)
		{
			CGQC_GU_DossierSessionLine line = shown[i];
			Widget cells = CGQC_GU_Table.Row(m_Region, list);

			// The session, with the member's position under it
			Widget what = CGQC_GU_Table.Stack(cells, 0, widths[0]);
			CGQC_GU_W.Para(what, line.title, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
			CGQC_GU_W.Para(what, line.sub, CGQC_GU_Style.FONT_LABEL, 14, CGQC_GU_Style.InkMuted(), 2);

			CGQC_GU_Table.Value(cells, 1, line.type, widths[1]);
			CGQC_GU_Table.Value(cells, 2, line.date, widths[2]);
			CGQC_GU_Table.Value(cells, 3, line.qual, widths[3]);
			CGQC_GU_Table.Value(cells, 4, line.exercises, widths[4]);
			CGQC_GU_Table.Value(cells, 5, line.instructor, widths[5]);
		}

		if (shown.IsEmpty())
			Empty(list, "Aucune séance au dossier.");

		m_Screen.Pager(m_Region, list, m_iSessionPage, pages, "session.page");
	}

	//------------------------------------------------------------------------------------------------
	protected void Filter(Widget row, string text, string kind)
	{
		CGQC_GU_Button b = m_Screen.Btn(m_Region, row, text, "session.filter", kind);
		if (!b)
			return;

		b.SetSelected(kind == m_sSessionKind);
		LayoutSlot.SetPadding(b.GetRootWidget(), 8, 0, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	// Insignes (top of the Qualifications page)
	//------------------------------------------------------------------------------------------------
	protected void Badges(Widget body, CGQC_GU_DossierDto dto)
	{
		Widget legend = CGQC_GU_W.HBox(body);
		if (legend)
		{
			LayoutSlot.SetPadding(legend, 0, 0, 0, 10);
			Legend(legend, CGQC_GU_Quals.STATE_OBTAINED, "Obtenu", 16);
			Legend(legend, CGQC_GU_Quals.STATE_RUNNING, "En cours", 14);
			Legend(legend, CGQC_GU_Quals.STATE_RETAKE, "Reprise", 16);
			Legend(legend, CGQC_GU_Quals.STATE_TODO, "À venir", 14);
		}

		// The chosen badge; at first a retake, else one in progress, else the first of the list.
		CGQC_GU_DossierLadderItem chosen = FindBadge(dto, m_sBadge);
		if (!chosen)
			chosen = FirstInState(dto, CGQC_GU_Quals.STATE_RETAKE);
		if (!chosen)
			chosen = FirstInState(dto, CGQC_GU_Quals.STATE_RUNNING);
		if (!chosen && !dto.ladder.IsEmpty())
			chosen = dto.ladder[0];

		if (!chosen)
		{
			Empty(body, "Aucun insigne au catalogue.");
			return;
		}

		m_sBadge = chosen.code;

		Widget row;
		for (int i = 0; i < dto.ladder.Count(); i++)
		{
			if (i % BADGES_PER_ROW == 0)
				row = CGQC_GU_W.HBox(body);

			if (!row)
				break;

			Badge(row, dto.ladder[i], dto.ladder[i].code == m_sBadge);
		}

		BadgeDetail(body, chosen);

		Widget buttons = CGQC_GU_W.HBox(body);
		if (!buttons)
			return;

		LayoutSlot.SetPadding(buttons, 0, 12, 0, 0);
		m_Screen.Btn(m_Region, buttons, "VOIR LA PROGRESSION", "badge.progress", chosen.code, false, CGQC_GU_EButtonKind.PRIMARY);

		CGQC_GU_Button qual = m_Screen.Btn(m_Region, buttons, "VOIR LA QUALIFICATION", "badge.qual", chosen.code);
		if (qual)
			LayoutSlot.SetPadding(qual.GetRootWidget(), 8, 0, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_DossierLadderItem FindBadge(CGQC_GU_DossierDto dto, string code)
	{
		foreach (CGQC_GU_DossierLadderItem item : dto.ladder)
		{
			if (item.code == code)
				return item;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_DossierLadderItem FirstInState(CGQC_GU_DossierDto dto, string state)
	{
		foreach (CGQC_GU_DossierLadderItem item : dto.ladder)
		{
			if (item.state == state)
				return item;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static string StateLabel(string state)
	{
		if (state == CGQC_GU_Quals.STATE_OBTAINED)
			return "OBTENU";

		if (state == CGQC_GU_Quals.STATE_RUNNING)
			return "EN COURS";

		if (state == CGQC_GU_Quals.STATE_RETAKE)
			return "REPRISE";

		return "À VENIR";
	}

	//------------------------------------------------------------------------------------------------
	//! The look of the four states: obtained and retake are filled, the others are outlines.
	protected static Color StateFill(string state)
	{
		if (state == CGQC_GU_Quals.STATE_OBTAINED)
			return CGQC_GU_Style.Blue();

		if (state == CGQC_GU_Quals.STATE_RETAKE)
			return CGQC_GU_Style.Red();

		return CGQC_GU_Style.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected static Color StateEdge(string state)
	{
		if (state == CGQC_GU_Quals.STATE_RUNNING)
			return CGQC_GU_Style.Rule();

		if (state == CGQC_GU_Quals.STATE_TODO)
			return CGQC_GU_Style.BadgeDim();

		return StateFill(state);
	}

	//------------------------------------------------------------------------------------------------
	protected static Color StateText(string state)
	{
		if (state == CGQC_GU_Quals.STATE_OBTAINED)
			return CGQC_GU_Style.PaperText();

		if (state == CGQC_GU_Quals.STATE_RETAKE)
			return CGQC_GU_Style.White();

		if (state == CGQC_GU_Quals.STATE_RUNNING)
			return CGQC_GU_Style.InkSoft();

		return CGQC_GU_Style.BadgeDim();
	}

	//------------------------------------------------------------------------------------------------
	//! A square of the given state: pressable when an action is given. Returns the stack inside it.
	protected Widget StateBox(Widget parent, float width, float height, string state, CGQC_GU_Action action, float dash)
	{
		Widget holder = CGQC_GU_W.Sized(parent, width, height);
		if (!holder)
			return null;

		LayoutSlot.SetVerticalAlign(holder.GetParent(), LayoutVerticalAlign.Center);

		// « À venir » has a dashed outline, drawn from small squares.
		bool dashed = state == CGQC_GU_Quals.STATE_TODO;
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		if (dashed)
			edges.Clear();

		Color edge = StateEdge(state);
		CGQC_GU_ClickRow box = CGQC_GU_ClickRow.Create(holder, action, StateFill(state), edge, edges);
		if (!box)
			return null;

		m_Region.m_aRows.Insert(box);
		LayoutSlot.SetSizeMode(box.GetRootWidget(), LayoutSizeMode.Fill);

		if (dashed)
		{
			Widget overlay = box.GetRootWidget().FindAnyWidget("Body");
			DashLine(overlay, width, dash, edge, true, false);
			DashLine(overlay, width, dash, edge, true, true);
			DashLine(overlay, height, dash, edge, false, false);
			DashLine(overlay, height, dash, edge, false, true);
		}

		return box.GetCells();
	}

	//------------------------------------------------------------------------------------------------
	//! One side of a dashed outline. far: the bottom or the right side.
	protected void DashLine(Widget overlay, float length, float dash, Color color, bool horizontal, bool far)
	{
		Widget line;
		if (horizontal)
			line = CGQC_GU_W.HBox(overlay);
		else
			line = CGQC_GU_W.VBox(overlay);

		if (!line)
			return;

		AlignableSlot.SetHorizontalAlign(line, LayoutHorizontalAlign.Left);
		AlignableSlot.SetVerticalAlign(line, LayoutVerticalAlign.Top);
		if (far && horizontal)
			AlignableSlot.SetVerticalAlign(line, LayoutVerticalAlign.Bottom);
		else if (far)
			AlignableSlot.SetHorizontalAlign(line, LayoutHorizontalAlign.Right);

		// As many dashes as fit with a gap of about two thirds of a dash, spread over the whole side.
		float gap = dash * 0.66;
		int count = Math.Max(2, Math.Floor((length + gap) / (dash + gap)));
		gap = (length - count * dash) / (count - 1);

		for (int i = 0; i < count; i++)
		{
			ImageWidget r = ImageWidget.Cast(CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, line));
			if (!r)
				return;

			if (horizontal)
				r.SetSize(dash, 1.5);
			else
				r.SetSize(1.5, dash);

			r.SetColor(color);
			if (i == count - 1)
				continue;

			if (horizontal)
				LayoutSlot.SetPadding(r, 0, 0, gap, 0);
			else
				LayoutSlot.SetPadding(r, 0, 0, 0, gap);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Legend(Widget row, string state, string label, float size)
	{
		StateBox(row, size, size, state, null, 3);

		TextWidget t = CGQC_GU_W.Text(row, label, CGQC_GU_Style.FONT_VALUE, 15, CGQC_GU_Style.InkSoft());
		if (!t)
			return;

		LayoutSlot.SetVerticalAlign(t, LayoutVerticalAlign.Center);
		LayoutSlot.SetPadding(t, 6, 0, 18, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void Badge(Widget row, CGQC_GU_DossierLadderItem item, bool chosen)
	{
		Widget holder = CGQC_GU_W.Sized(row, BADGE_WIDTH + 2 * BADGE_FRAME, BADGE_HEIGHT + 2 * BADGE_FRAME);
		Widget frame = CGQC_GU_W.Panel(holder);
		if (!frame)
			return;

		// The outline of the chosen badge, a little away from it
		Widget panel = frame.GetParent();
		LayoutSlot.SetSizeMode(panel, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(panel, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetPadding(frame, BADGE_FRAME, BADGE_FRAME, BADGE_FRAME, BADGE_FRAME);
		array<string> sides = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string side : sides)
		{
			ImageWidget e = ImageWidget.Cast(panel.FindAnyWidget(side));
			if (!e)
				continue;

			e.SetSize(3, 3);
			e.SetColor(CGQC_GU_Style.InkSoft());
			e.SetVisible(chosen);
		}

		CGQC_GU_DossierAction pick = new CGQC_GU_DossierAction(m_Screen, "badge.pick", item.code);
		Widget cells = StateBox(frame, BADGE_WIDTH, BADGE_HEIGHT, item.state, pick, 6);
		Widget col = CGQC_GU_W.VBox(cells);
		if (!col)
			return;

		LayoutSlot.SetSizeMode(col, LayoutSizeMode.Fill);
		LayoutSlot.SetVerticalAlign(col, LayoutVerticalAlign.Center);

		Color text = StateText(item.state);
		TextWidget code = CGQC_GU_W.Text(col, item.code, CGQC_GU_Style.FONT_TITLE, 18, text);
		if (code)
			LayoutSlot.SetHorizontalAlign(code, LayoutHorizontalAlign.Center);

		TextWidget state = CGQC_GU_W.Text(col, StateLabel(item.state), CGQC_GU_Style.FONT_LABEL, 12, text);
		if (state)
		{
			LayoutSlot.SetHorizontalAlign(state, LayoutHorizontalAlign.Center);
			LayoutSlot.SetPadding(state, 0, 4, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BadgeDetail(Widget body, CGQC_GU_DossierLadderItem item)
	{
		Widget d = CGQC_GU_W.Panel(body);
		if (!d)
			return;

		LayoutSlot.SetHorizontalAlign(d.GetParent(), LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(d.GetParent(), 0, 14, 0, 0);

		CGQC_GU_W.Para(d, item.code + " — " + item.title, CGQC_GU_Style.FONT_TITLE, 26, CGQC_GU_Style.Ink());

		string tag = StateLabel(item.state);
		Color color = CGQC_GU_Style.InkSoft();
		if (item.state == CGQC_GU_Quals.STATE_OBTAINED)
		{
			color = CGQC_GU_Style.Blue();
			if (!item.stateDate.IsEmpty())
				tag = tag + " — RÉUSSI LE " + item.stateDate;
		}
		else if (item.state == CGQC_GU_Quals.STATE_RETAKE)
		{
			color = CGQC_GU_Style.Red();
			if (!item.stateDate.IsEmpty())
				tag = tag + " — ÉCHEC LE " + item.stateDate;
		}
		else if (item.state == CGQC_GU_Quals.STATE_TODO)
		{
			color = CGQC_GU_Style.InkMuted();
		}

		Widget box = CGQC_GU_W.Tag(d, tag, color, 14);
		if (box)
			LayoutSlot.SetPadding(box, 0, 8, 0, 4);

		string level = item.ppLabel;
		if (!item.levelLabel.IsEmpty())
			level = item.levelLabel + " · " + level;

		CGQC_GU_W.Para(d, "Niveau : " + level, CGQC_GU_Style.FONT_VALUE, 18, CGQC_GU_Style.InkSoft(), 4);

		string teachers = item.teachers;
		if (teachers.IsEmpty())
			teachers = "—";

		CGQC_GU_W.Para(d, "Enseignée par : " + teachers, CGQC_GU_Style.FONT_VALUE, 18, CGQC_GU_Style.InkSoft(), 6);
	}
}
