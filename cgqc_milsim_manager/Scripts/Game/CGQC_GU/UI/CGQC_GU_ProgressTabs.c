// Gestion d'unité - Dossier → Progression and Instructeur (paper), Phase 3e, after the approved mockup
// (Dossier-Prochains, Dossier-Instructeur). Everything shown comes from the server's "dossier" reply.
// Every write is a double press and is checked again on the server; the controls only show to a viewer who
// may use them (« evaluable » for Progression, instructors.manage for Instructeur).

class CGQC_GU_ProgressTabs
{
	static const float PANEL_WIDTH = 520;
	static const float PANEL_INNER = 484;		// panel width less its padding
	static const int SPECIALTIES_PER_ROW = 5;
	static const int PICKS_PER_ROW = 8;

	protected CGQC_GU_DossierScreen m_Screen;
	protected CGQC_GU_Region m_Region;
	protected CGQC_GU_DossierDto m_Dto;
	protected string m_sQual;		// Progression: chosen qualification
	protected string m_sTeach;		// Instructeur: chosen qualification

	// Widgets of the open panel, kept alive by the region
	protected ref array<CGQC_GU_Check> m_aExerciseChecks = new array<CGQC_GU_Check>();
	protected ref array<CGQC_GU_Check> m_aCriteriaChecks = new array<CGQC_GU_Check>();
	protected MultilineEditBoxWidget m_wNotes;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_ProgressTabs(CGQC_GU_DossierScreen screen, CGQC_GU_Region region)
	{
		m_Screen = screen;
		m_Region = region;
	}

	//------------------------------------------------------------------------------------------------
	//! Another member is shown: forget the chosen qualifications.
	void Reset()
	{
		m_sQual = string.Empty;
		m_sTeach = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! The qualification Progression opens on (VOIR LA PROGRESSION on Insignes).
	void Choose(string qual)
	{
		m_sQual = qual;
	}

	//------------------------------------------------------------------------------------------------
	//! True when the body must be rebuilt.
	bool OnAction(string kind, string value)
	{
		switch (kind)
		{
			case "prog.pick":
				m_sQual = value;
				return true;

			case "prog.save":
				SendProgress(CGQC_GU_QualRequests.PROGRESS_SAVE, string.Empty);
				break;

			case "prog.eval":
				SendProgress(CGQC_GU_QualRequests.QUAL_EVALUATE, value);
				break;

			case "teach.pick":
				m_sTeach = value;
				return true;

			case "teach.set":
				SendInstructor(CGQC_GU_QualRequests.INSTRUCTOR_SET);
				break;

			case "teach.remove":
				SendInstructor(CGQC_GU_QualRequests.INSTRUCTOR_REMOVE);
				break;

			case "teach.list":
				OpenList();
				break;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	//! The panel's ticks and notes; result is "" for a partial save.
	protected void SendProgress(string action, string result)
	{
		if (m_sQual.IsEmpty() || !m_wNotes)
			return;

		CGQC_GU_Payload p = m_Screen.ForThisMember();
		p.Set("qual", m_sQual);
		p.Set("exercises", Ticked(m_aExerciseChecks));
		p.Set("criteria", Ticked(m_aCriteriaChecks));
		p.Set("notes", m_wNotes.GetText());
		if (!result.IsEmpty())
			p.Set("result", result);

		m_Screen.Write(action, p);
	}

	//------------------------------------------------------------------------------------------------
	protected string Ticked(array<CGQC_GU_Check> checks)
	{
		array<string> ids = new array<string>();
		foreach (CGQC_GU_Check c : checks)
		{
			if (c && c.IsChecked())
				ids.Insert(c.GetId());
		}

		return CGQC_GU_Str.Join(ids, ",");
	}

	//------------------------------------------------------------------------------------------------
	protected void SendInstructor(string action)
	{
		if (m_sTeach.IsEmpty())
			return;

		CGQC_GU_Payload p = m_Screen.ForThisMember();
		p.Set("qual", m_sTeach);
		m_Screen.Write(action, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenList()
	{
		if (!m_Dto)
			return;

		array<string> codes = new array<string>();
		foreach (CGQC_GU_DossierLadderItem item : m_Dto.ladder)
		{
			codes.Insert(item.code);
		}

		CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_InstructorsDialog(m_Dto.uid, codes));
	}

	//------------------------------------------------------------------------------------------------
	// Progression
	//------------------------------------------------------------------------------------------------
	void BuildProgress(Widget body, CGQC_GU_DossierDto dto)
	{
		m_Dto = dto;
		m_aExerciseChecks.Clear();
		m_aCriteriaChecks.Clear();
		m_wNotes = null;

		m_Screen.Section(body, "PROGRESSION");

		CGQC_GU_DossierLadderItem chosen = Find(dto, m_sQual);
		if (!chosen)
			chosen = FirstInState(dto, CGQC_GU_Quals.STATE_RETAKE);
		if (!chosen)
			chosen = FirstInState(dto, CGQC_GU_Quals.STATE_RUNNING);
		if (!chosen)
			chosen = FirstInState(dto, CGQC_GU_Quals.STATE_TODO);
		if (!chosen && !dto.ladder.IsEmpty())
			chosen = dto.ladder[0];

		if (!chosen)
		{
			CGQC_GU_W.Para(body, "Aucune qualification au catalogue.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 8);
			return;
		}

		m_sQual = chosen.code;

		Widget row = CGQC_GU_W.HBox(body);
		if (!row)
			return;

		LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);

		// Left: next promotion, then the PP ladder
		Widget left = CGQC_GU_W.VBox(row);
		if (left)
		{
			LayoutSlot.SetSizeMode(left, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(left, LayoutVerticalAlign.Stretch);
			NextPromotion(left, dto);
			Ladder(left, dto, chosen);
		}

		// Right: the chosen qualification
		Widget right = CGQC_GU_W.Sized(row, PANEL_WIDTH);
		if (!right)
			return;

		LayoutSlot.SetVerticalAlign(right.GetParent(), LayoutVerticalAlign.Stretch);
		LayoutSlot.SetPadding(right.GetParent(), 16, 0, 0, 0);
		Panel(right, dto, chosen);
	}

	//------------------------------------------------------------------------------------------------
	//! « Prochaine promotion »: each criterion with a read-only box.
	protected void NextPromotion(Widget parent, CGQC_GU_DossierDto dto)
	{
		if (dto.nextRank.IsEmpty())
			return;

		Widget box = CGQC_GU_W.Panel(parent);
		if (!box)
			return;

		LayoutSlot.SetHorizontalAlign(box.GetParent(), LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(box.GetParent(), 0, 0, 0, 10);
		AlignableSlot.SetPadding(box, 14, 10, 14, 10);

		string title = "PROCHAINE PROMOTION — " + CGQC_GU_Str.UpperFr(dto.nextRank);
		if (dto.nextAutomatic)
			title = title + " · AUTOMATIQUE";

		CGQC_GU_W.Para(box, title, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.Ink());

		foreach (CGQC_GU_CriterionLine line : dto.criteria)
		{
			// « Q-MB à faire » reads « Q-MB — Militaire de base »: the box says whether it is met.
			string text = line.text;
			int space = text.IndexOf(" ");
			if (space > 0)
			{
				CGQC_GU_DossierLadderItem item = Find(dto, text.Substring(0, space));
				if (item)
					text = item.code + " — " + item.title;
			}

			CGQC_GU_Check c = CGQC_GU_Check.Create(box, "", text, line.met);
			if (!c)
				continue;

			c.SetReadOnly();
			c.SetPanelStyle(CGQC_GU_Style.FONT_VALUE, 16, true);
			m_Region.m_aChecks.Insert(c);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Ladder(Widget parent, CGQC_GU_DossierDto dto, CGQC_GU_DossierLadderItem chosen)
	{
		Widget list = CGQC_GU_W.Scroll(parent);
		if (!list)
			return;

		array<float> widths = { 100, 0, 70, 120 };
		CGQC_GU_Table.Head(m_Region, list, { "CODE", "QUALIFICATION", "GRADE", "STATUT" }, widths);

		for (int pp = 1; pp <= 3; pp++)
		{
			bool headed = false;
			array<string> groups = new array<string>();
			foreach (CGQC_GU_DossierLadderItem item : dto.ladder)
			{
				if (item.pp != pp)
					continue;

				if (!headed)
				{
					PpHead(list, pp, item.ppLabel);
					headed = true;
				}

				if (item.group.IsEmpty())
				{
					LadderRow(list, widths, item.code, item.title, item.levelAbbr, item.state, item.code == chosen.code, item.code);
					continue;
				}

				// A group of specialties is one row; the panel picks the specialty.
				if (groups.Contains(item.group))
					continue;

				groups.Insert(item.group);
				array<CGQC_GU_DossierLadderItem> members = GroupOf(dto, item.group);
				array<string> names = new array<string>();
				foreach (CGQC_GU_DossierLadderItem m : members)
				{
					names.Insert(Specialty(m));
				}

				// The row shows the best of its specialties: one is enough.
				CGQC_GU_DossierLadderItem best = BestOf(members);
				bool selected = chosen.group == item.group;
				string pick = best.code;
				if (selected)
					pick = chosen.code;

				string title = string.Format("1 sur %1 : %2", members.Count(), CGQC_GU_Str.Join(names, " / "));
				LadderRow(list, widths, item.group + "-x", title, item.levelAbbr, best.state, selected, pick);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Heading of one PP level, with its colour bar.
	protected void PpHead(Widget list, int pp, string label)
	{
		array<string> edges = { "EdgeB", "EdgeL", "EdgeR" };
		CGQC_GU_ClickRow row = CGQC_GU_ClickRow.Create(list, null, CGQC_GU_Style.Translucent(0x3A, 0x31, 0x23, 0.07), CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return;

		m_Region.m_aRows.Insert(row);
		Widget cells = row.GetCells();

		ImageWidget bar = ImageWidget.Cast(CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, cells));
		if (bar)
		{
			bar.SetSize(6, 2);
			bar.SetColor(CGQC_GU_Style.PpColor(pp));
			LayoutSlot.SetVerticalAlign(bar, LayoutVerticalAlign.Stretch);
		}

		CGQC_GU_W.Para(CGQC_GU_Table.Stack(cells, 0, 0), CGQC_GU_Str.UpperFr(label), CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.Ink());
	}

	//------------------------------------------------------------------------------------------------
	protected void LadderRow(Widget list, array<float> widths, string code, string title, string level, string state, bool selected, string pick)
	{
		Color fill = CGQC_GU_Style.Clear();
		if (selected)
			fill = CGQC_GU_Style.RowSelected();

		array<string> edges = { "EdgeB", "EdgeL", "EdgeR" };
		CGQC_GU_DossierAction action = new CGQC_GU_DossierAction(m_Screen, "prog.pick", pick);
		CGQC_GU_ClickRow row = CGQC_GU_ClickRow.Create(list, action, fill, CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return;

		m_Region.m_aRows.Insert(row);
		Widget cells = row.GetCells();

		// « À venir » rows are dimmed, unless chosen.
		Color text = CGQC_GU_Style.InkSoft();
		if (state == CGQC_GU_Quals.STATE_TODO && !selected)
			text = CGQC_GU_Style.BadgeDim();

		CGQC_GU_W.Para(CGQC_GU_Table.Stack(cells, 0, widths[0]), code, CGQC_GU_Style.FONT_TITLE, 15, text);
		CGQC_GU_W.Para(CGQC_GU_Table.Stack(cells, 1, widths[1]), title, CGQC_GU_Style.FONT_VALUE, 15, text);
		CGQC_GU_W.Para(CGQC_GU_Table.Stack(cells, 2, widths[2]), level, CGQC_GU_Style.FONT_VALUE, 14, text);
		CGQC_GU_W.Tag(CGQC_GU_Table.Stack(cells, 3, widths[3]), CGQC_GU_DossierTabs.StateLabel(state), StateColor(state));
	}

	//------------------------------------------------------------------------------------------------
	//! The chosen qualification: exercises, criteria and notes; the buttons stay at the bottom.
	protected void Panel(Widget parent, CGQC_GU_DossierDto dto, CGQC_GU_DossierLadderItem item)
	{
		Widget inner = CGQC_GU_W.Panel(parent);
		if (!inner)
			return;

		Widget box = inner.GetParent();
		LayoutSlot.SetSizeMode(box, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Stretch);
		Widget bg = box.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(CGQC_GU_Style.PaperLight());

		AlignableSlot.SetPadding(inner, 16, 14, 16, 14);

		// The controls show to an authorized evaluator, until the qualification is obtained.
		bool obtained = item.state == CGQC_GU_Quals.STATE_OBTAINED;
		bool edit = !obtained && dto.evaluable.Contains(item.code);

		Widget d = CGQC_GU_W.Scroll(inner);
		if (!d)
			return;

		CGQC_GU_W.Text(d, "QUALIFICATION CHOISIE", CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.InkMuted());
		CGQC_GU_W.Para(d, item.code + " — " + item.title, CGQC_GU_Style.FONT_TITLE, 22, CGQC_GU_Style.Ink(), 4);

		if (!item.group.IsEmpty())
			Specialties(d, dto, item);

		StateTag(d, item);

		PanelLabel(d, "EXERCICES");
		if (item.exercises.IsEmpty())
			Upcoming(d);

		foreach (CGQC_GU_DossierCheck e : item.exercises)
		{
			ExerciseRow(d, e, edit);
		}

		PanelLabel(d, "CRITÈRES");
		if (item.criteria.IsEmpty())
			Upcoming(d);

		foreach (CGQC_GU_DossierCheck c : item.criteria)
		{
			CGQC_GU_Check check = CGQC_GU_Check.Create(d, c.id, c.text, c.met);
			if (!check)
				continue;

			if (!edit)
				check.SetReadOnly();

			check.SetPanelStyle(CGQC_GU_Style.FONT_VALUE, 16, true);
			m_Region.m_aChecks.Insert(check);
			m_aCriteriaChecks.Insert(check);
		}

		PanelLabel(d, "NOTES");
		if (edit)
		{
			m_wNotes = CGQC_GU_W.FieldMulti(d, item.notes, PANEL_INNER - 12, 76);
		}
		else
		{
			string notes = item.notes;
			if (notes.IsEmpty())
				notes = "—";

			CGQC_GU_W.Para(d, notes, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		}

		if (!item.saved.IsEmpty())
			CGQC_GU_W.Para(d, item.saved, CGQC_GU_Style.FONT_LABEL, 13, CGQC_GU_Style.InkMuted(), 6);

		if (!edit)
			return;

		Widget buttons = CGQC_GU_W.HBox(inner);
		if (!buttons)
			return;

		LayoutSlot.SetHorizontalAlign(buttons, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(buttons, 0, 10, 0, 0);

		CGQC_GU_Button save = m_Screen.Btn(m_Region, buttons, "ENREGISTRER LA PROGRESSION", "prog.save", "", true);
		if (save)
		{
			LayoutSlot.SetSizeMode(save.GetRootWidget(), LayoutSizeMode.Fill);
			LayoutSlot.SetHorizontalAlign(save.GetRootWidget(), LayoutHorizontalAlign.Stretch);
		}

		Verdict(buttons, "RÉUSSIR", CGQC_GU_Quals.RESULT_PASSED, CGQC_GU_EButtonKind.PRIMARY);
		Verdict(buttons, "ÉCHOUER", CGQC_GU_Quals.RESULT_FAILED, CGQC_GU_EButtonKind.DANGER);
	}

	//------------------------------------------------------------------------------------------------
	//! RÉUSSIR / ÉCHOUER, in a box wide enough for the armed label so the row does not move.
	protected void Verdict(Widget row, string text, string result, CGQC_GU_EButtonKind look)
	{
		Widget box = CGQC_GU_W.Sized(row, 112);
		if (!box)
			return;

		LayoutSlot.SetPadding(box.GetParent(), 8, 0, 0, 0);
		CGQC_GU_Button b = m_Screen.Btn(m_Region, box, text, "prog.eval", result, true, look);
		if (!b)
			return;

		b.SetConfirmText("CONFIRMER");
		LayoutSlot.SetHorizontalAlign(b.GetRootWidget(), LayoutHorizontalAlign.Stretch);
	}

	//------------------------------------------------------------------------------------------------
	protected void Specialties(Widget d, CGQC_GU_DossierDto dto, CGQC_GU_DossierLadderItem item)
	{
		array<CGQC_GU_DossierLadderItem> members = GroupOf(dto, item.group);
		Widget row;
		for (int i = 0; i < members.Count(); i++)
		{
			if (i % SPECIALTIES_PER_ROW == 0)
			{
				row = CGQC_GU_W.HBox(d);
				if (row)
					LayoutSlot.SetPadding(row, 0, 6, 0, 0);
			}

			if (!row)
				break;

			CGQC_GU_Button b = m_Screen.Btn(m_Region, row, Specialty(members[i]), "prog.pick", members[i].code);
			if (!b)
				continue;

			b.SetSelected(members[i].code == item.code);
			LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Obtained and retake qualifications say so, with the date of the result.
	protected void StateTag(Widget d, CGQC_GU_DossierLadderItem item)
	{
		string what;
		if (item.state == CGQC_GU_Quals.STATE_OBTAINED)
			what = "OBTENU — RÉUSSI LE ";
		else if (item.state == CGQC_GU_Quals.STATE_RETAKE)
			what = "REPRISE — ÉCHEC LE ";

		if (what.IsEmpty() || item.stateDate.IsEmpty())
			return;

		Widget tag = CGQC_GU_W.Tag(d, what + item.stateDate, StateColor(item.state), 13);
		if (tag)
			LayoutSlot.SetPadding(tag, 0, 8, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Box and code (pressable together), title, REQUIS.
	protected void ExerciseRow(Widget d, CGQC_GU_DossierCheck e, bool edit)
	{
		Widget row = CGQC_GU_W.HBox(d);
		if (!row)
			return;

		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);

		Widget codeBox = CGQC_GU_W.Sized(row, 140);
		if (codeBox)
		{
			LayoutSlot.SetVerticalAlign(codeBox.GetParent(), LayoutVerticalAlign.Center);
			CGQC_GU_Check c = CGQC_GU_Check.Create(codeBox, e.id, e.id, e.met);
			if (c)
			{
				if (!edit)
					c.SetReadOnly();

				c.SetPanelStyle(CGQC_GU_Style.FONT_TITLE, 15, false);
				m_Region.m_aChecks.Insert(c);
				m_aExerciseChecks.Insert(c);
			}
		}

		TextWidget title = CGQC_GU_W.Text(row, e.text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft(), true);
		if (title)
		{
			LayoutSlot.SetSizeMode(title, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(title, LayoutVerticalAlign.Center);
		}

		Widget tagBox = CGQC_GU_W.Sized(row, 76);
		if (tagBox)
		{
			LayoutSlot.SetVerticalAlign(tagBox.GetParent(), LayoutVerticalAlign.Center);
			if (e.required)
				CGQC_GU_W.Tag(tagBox, "REQUIS", CGQC_GU_Style.Blue());
		}

		Widget sep = CGQC_GU_W.Rule(d, 0, 0);
		if (sep)
			sep.SetOpacity(0.35);
	}

	//------------------------------------------------------------------------------------------------
	protected void PanelLabel(Widget d, string text)
	{
		CGQC_GU_W.Caption(d, text, 12);
	}

	//------------------------------------------------------------------------------------------------
	protected void Upcoming(Widget d)
	{
		CGQC_GU_W.Text(d, "à venir", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
	}

	//------------------------------------------------------------------------------------------------
	// Instructeur
	//------------------------------------------------------------------------------------------------
	void BuildInstructor(Widget body, CGQC_GU_DossierDto dto)
	{
		m_Dto = dto;

		// Title, with the instructor list on its right
		Widget head = CGQC_GU_W.HBox(body);
		if (head)
		{
			LayoutSlot.SetHorizontalAlign(head, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(head, 0, 6, 0, 0);
			TextWidget title = CGQC_GU_W.Text(head, "INSTRUCTEUR", CGQC_GU_Style.FONT_TITLE, 22, CGQC_GU_Style.Ink());
			if (title)
			{
				LayoutSlot.SetSizeMode(title, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(title, LayoutVerticalAlign.Bottom);
			}

			m_Screen.Btn(m_Region, head, "LISTE DES INSTRUCTEURS", "teach.list", "", false, CGQC_GU_EButtonKind.PRIMARY);
		}

		CGQC_GU_W.Rule(body, 6, 8);

		// The chosen qualification: at first the first one taught, else the first of the catalogue.
		if (!Find(dto, m_sTeach))
		{
			m_sTeach = string.Empty;
			if (!dto.teach.IsEmpty())
				m_sTeach = dto.teach[0].qual;
			else if (!dto.ladder.IsEmpty())
				m_sTeach = dto.ladder[0].code;
		}

		Widget list = CGQC_GU_W.Scroll(body);
		if (!list)
			return;

		array<float> widths = { 0, 300, 220, 100 };
		CGQC_GU_Table.Head(m_Region, list, { "QUALIFICATION", "AUTORISATION", "AUTORISÉ PAR", "ÉLÈVES" }, widths);

		bool held;
		foreach (CGQC_GU_DossierTeachLine t : dto.teach)
		{
			bool selected = dto.canInstruct && t.qual == m_sTeach;
			if (t.qual == m_sTeach)
				held = true;

			TeachRow(list, widths, t, selected, dto.canInstruct);
		}

		if (dto.teach.IsEmpty())
			CGQC_GU_W.Para(list, "Aucune autorisation d'enseigner.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 8);

		if (!dto.canInstruct)
			return;

		// Qualification picker and the two writes, at the bottom of the page
		CGQC_GU_W.Caption(body, "QUALIFICATION", 12);
		Widget row;
		for (int i = 0; i < dto.ladder.Count(); i++)
		{
			if (i % PICKS_PER_ROW == 0)
			{
				row = CGQC_GU_W.HBox(body);
				if (row)
					LayoutSlot.SetPadding(row, 0, 0, 0, 8);
			}

			if (!row)
				break;

			string code = dto.ladder[i].code;
			CGQC_GU_Button b = m_Screen.Btn(m_Region, row, code, "teach.pick", code);
			if (!b)
				continue;

			b.SetSelected(code == m_sTeach);
			LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
		}

		Widget buttons = CGQC_GU_W.HBox(body);
		if (!buttons)
			return;

		LayoutSlot.SetPadding(buttons, 0, 6, 0, 0);
		CGQC_GU_Button grant = m_Screen.Btn(m_Region, buttons, "AUTORISER À ENSEIGNER", "teach.set", "", true);
		if (grant)
			grant.SetEnabled(!held);

		CGQC_GU_Button remove = m_Screen.Btn(m_Region, buttons, "RETIRER L'AUTORISATION", "teach.remove", "", true);
		if (remove)
		{
			remove.SetEnabled(held);
			LayoutSlot.SetPadding(remove.GetRootWidget(), 8, 0, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A manager picks a line by pressing it.
	protected void TeachRow(Widget list, array<float> widths, CGQC_GU_DossierTeachLine t, bool selected, bool pressable)
	{
		Color fill = CGQC_GU_Style.Clear();
		if (selected)
			fill = CGQC_GU_Style.RowSelected();

		CGQC_GU_DossierAction action;
		if (pressable)
			action = new CGQC_GU_DossierAction(m_Screen, "teach.pick", t.qual);

		array<string> edges = { "EdgeB", "EdgeL", "EdgeR" };
		CGQC_GU_ClickRow row = CGQC_GU_ClickRow.Create(list, action, fill, CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return;

		m_Region.m_aRows.Insert(row);
		Widget cells = row.GetCells();

		Widget line = CGQC_GU_W.HBox(CGQC_GU_Table.Stack(cells, 0, widths[0]));
		CGQC_GU_W.Text(line, t.qual, CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.InkSoft());
		TextWidget title = CGQC_GU_W.Text(line, " — " + t.title, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		if (title)
			LayoutSlot.SetPadding(title, 4, 0, 0, 0);

		CGQC_GU_Table.Value(cells, 1, "Autorisé par le commandement", widths[1]);
		CGQC_GU_Table.Value(cells, 2, t.by, widths[2]);
		CGQC_GU_Table.Value(cells, 3, t.students.ToString(), widths[3]);
	}

	//------------------------------------------------------------------------------------------------
	// Ladder helpers
	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_DossierLadderItem Find(CGQC_GU_DossierDto dto, string code)
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
	//! The specialties of one group (QSP1, QSP2), in catalogue order.
	protected array<CGQC_GU_DossierLadderItem> GroupOf(CGQC_GU_DossierDto dto, string group)
	{
		array<CGQC_GU_DossierLadderItem> members = new array<CGQC_GU_DossierLadderItem>();
		foreach (CGQC_GU_DossierLadderItem item : dto.ladder)
		{
			if (item.group == group)
				members.Insert(item);
		}

		return members;
	}

	//------------------------------------------------------------------------------------------------
	//! Obtained first, then retake, then in progress, else the first of the group.
	protected CGQC_GU_DossierLadderItem BestOf(array<CGQC_GU_DossierLadderItem> members)
	{
		array<string> order = { CGQC_GU_Quals.STATE_OBTAINED, CGQC_GU_Quals.STATE_RETAKE, CGQC_GU_Quals.STATE_RUNNING };
		foreach (string state : order)
		{
			foreach (CGQC_GU_DossierLadderItem item : members)
			{
				if (item.state == state)
					return item;
			}
		}

		return members[0];
	}

	//------------------------------------------------------------------------------------------------
	//! "QSP1-MED" -> "MED"
	protected string Specialty(CGQC_GU_DossierLadderItem item)
	{
		int skip = item.group.Length() + 1;
		if (item.code.Length() <= skip)
			return item.code;

		return item.code.Substring(skip, item.code.Length() - skip);
	}

	//------------------------------------------------------------------------------------------------
	protected Color StateColor(string state)
	{
		if (state == CGQC_GU_Quals.STATE_OBTAINED)
			return CGQC_GU_Style.Blue();

		if (state == CGQC_GU_Quals.STATE_RETAKE)
			return CGQC_GU_Style.Red();

		if (state == CGQC_GU_Quals.STATE_RUNNING)
			return CGQC_GU_Style.InkSoft();

		return CGQC_GU_Style.BadgeDim();
	}
}
