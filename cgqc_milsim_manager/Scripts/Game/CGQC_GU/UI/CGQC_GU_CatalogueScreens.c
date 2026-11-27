// Gestion d'unité - Unité → Liste des qualifications and Liste des exercices (paper), Phase 3c, after the
// approved mockup (Catalogue-Qualifs, Catalogue-Exercices). Read-only, for every member. Everything shown
// comes from the server's "quals.catalogue" and "exercises.catalogue" replies.

//! Binds a row or a button to a named action of a catalogue screen, with a value (code).
class CGQC_GU_CatalogueAction : CGQC_GU_Action
{
	protected CGQC_GU_CatalogueScreen m_Screen;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_CatalogueAction(CGQC_GU_CatalogueScreen screen, string kind, string value)
	{
		m_Screen = screen;
		m_sKind = kind;
		m_sValue = value;
	}

	//------------------------------------------------------------------------------------------------
	override void Fire()
	{
		if (m_Screen)
			m_Screen.OnAction(m_sKind, m_sValue);
	}
}

//! Common part of the two lists: header with the search box, one read of the catalogue, two regions.
class CGQC_GU_CatalogueScreen : CGQC_GU_PaperScreen
{
	protected int m_iReadReq;
	protected EditBoxWidget m_wSearch;
	protected ref CGQC_GU_FieldWatch m_SearchWatch;
	protected TextWidget m_wMessage;
	protected ref CGQC_GU_Region m_List = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Detail = new CGQC_GU_Region();
	protected Widget m_wDetailBox;		// hidden while there is nothing to show

	//------------------------------------------------------------------------------------------------
	protected string GetTitle();
	protected string GetReadAction();
	//! Fixed widgets of the screen, before any data.
	protected void BuildFrame(Widget content);
	//! Reads the server's reply; false when it can't be used.
	protected bool Parse(string payload);
	//! Rebuilds everything that depends on the data, the filters and the selection.
	protected void Rebuild();
	void OnAction(string kind, string value);

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildPaper()
	{
		SetHeader("UNITÉ · RÉFÉRENCE COMMUNE", GetTitle());

		if (m_wHeaderTools)
		{
			TextWidget cap = CGQC_GU_W.Text(m_wHeaderTools, "RECHERCHE", CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
			if (cap)
			{
				LayoutSlot.SetVerticalAlign(cap, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(cap, 0, 0, 10, 0);
			}

			m_wSearch = CGQC_GU_W.Field(m_wHeaderTools, "", 380);
			if (m_wSearch)
			{
				m_SearchWatch = new CGQC_GU_FieldWatch();
				m_SearchWatch.GetOnChanged().Insert(OnSearchChanged);
				m_wSearch.AddHandler(m_SearchWatch);
			}
		}

		if (!m_wContent)
			return;

		m_wMessage = CGQC_GU_W.Text(m_wContent, "Chargement…", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
		BuildFrame(m_wContent);

		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		m_iReadReq = CGQC_GU_Client.Request(GetReadAction());
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		if (m_wSearch && m_SearchWatch)
			m_wSearch.RemoveHandler(m_SearchWatch);

		m_List.Reset();
		m_Detail.Reset();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot || requestId != m_iReadReq)
			return;

		if (!ok || !Parse(payload))
		{
			if (!m_wMessage)
				return;

			string text = "Liste illisible.";
			if (!ok)
				text = CGQC_GU_DossierScreen.ErrorText(code);

			m_wMessage.SetText(text);
			m_wMessage.SetColor(CGQC_GU_Style.Red());
			return;
		}

		if (m_wMessage)
			m_wMessage.SetVisible(false);

		Rebuild();
	}

	//------------------------------------------------------------------------------------------------
	//! Typing in the search box refreshes the lists only, so the field keeps its focus.
	protected void OnSearchChanged()
	{
		Rebuild();
	}

	//------------------------------------------------------------------------------------------------
	protected string Needle()
	{
		if (!m_wSearch)
			return string.Empty;

		return CGQC_GU_Str.Lower(m_wSearch.GetText().Trim());
	}

	//------------------------------------------------------------------------------------------------
	protected bool Found(string needle, string hay)
	{
		if (needle.IsEmpty())
			return true;

		return CGQC_GU_Str.Lower(hay).Contains(needle);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_ClickRow Row(CGQC_GU_Region region, Widget parent, string kind, string value, Color fill, Color edge, array<string> edges)
	{
		CGQC_GU_CatalogueAction action;
		if (!kind.IsEmpty())
			action = new CGQC_GU_CatalogueAction(this, kind, value);

		CGQC_GU_ClickRow row = CGQC_GU_ClickRow.Create(parent, action, fill, edge, edges);
		if (row)
			region.m_aRows.Insert(row);

		return row;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button Btn(CGQC_GU_Region region, Widget parent, string text, string kind, string value)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text);
		if (!b)
			return null;

		region.m_aButtons.Insert(b);
		b.SetCompact();
		CGQC_GU_CatalogueAction a = new CGQC_GU_CatalogueAction(this, kind, value);
		region.m_aKeep.Insert(a);
		b.GetOnActivated().Insert(a.Fire);
		return b;
	}

	//------------------------------------------------------------------------------------------------
	//! One cell of a horizontal row; width 0 = takes what is left. Text wraps inside the cell.
	protected TextWidget Cell(Widget row, string text, float width, ResourceName font, int size, Color color, float padX = 0, float padY = 0)
	{
		Widget box;
		Widget slot;
		if (width > 0)
		{
			box = CGQC_GU_W.Sized(row, width);
			if (!box)
				return null;

			slot = box.GetParent();
		}
		else
		{
			box = CGQC_GU_W.VBox(row);
			if (!box)
				return null;

			slot = box;
			LayoutSlot.SetSizeMode(slot, LayoutSizeMode.Fill);
		}

		LayoutSlot.SetVerticalAlign(slot, LayoutVerticalAlign.Center);
		LayoutSlot.SetPadding(slot, padX, padY, padX, padY);
		return CGQC_GU_W.Para(box, text, font, size, color);
	}

	//------------------------------------------------------------------------------------------------
	//! Heading inside a detail box.
	protected void Section(Widget parent, string title)
	{
		TextWidget t = CGQC_GU_W.Text(parent, title, CGQC_GU_Style.FONT_TITLE, 19, CGQC_GU_Style.Ink());
		if (t)
			LayoutSlot.SetPadding(t, 0, 14, 0, 0);

		CGQC_GU_W.Rule(parent, 2, 6);
	}

	//------------------------------------------------------------------------------------------------
	//! Ruled detail box with a light fill. Returns its inner stack.
	protected Widget DetailBox(Widget parent)
	{
		Widget inner = CGQC_GU_W.Panel(parent);
		if (!inner)
			return null;

		Widget box = inner.GetParent();
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Stretch);
		Widget bg = box.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(CGQC_GU_Style.PaperLight());

		AlignableSlot.SetPadding(inner, 22, 18, 8, 18);
		return inner;
	}

	//------------------------------------------------------------------------------------------------
	protected void ScrollToTop(CGQC_GU_Region region)
	{
		if (!region.m_wRoot)
			return;

		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(region.m_wRoot.GetParent());
		if (scroll)
			scroll.SetSliderPos(0, 0);
	}
}

//! Unité → Liste des qualifications: the PP1 / PP2 / PP3 groups on the left, the chosen one on the right.
class CGQC_GU_QualCatalogueScreen : CGQC_GU_CatalogueScreen
{
	static const float LIST_WIDTH = 620;
	static const int CHIPS_PER_ROW = 3;

	protected ref array<ref CGQC_GU_QualDto> m_aQuals;
	protected ref array<ref CGQC_GU_LabelItem> m_aNotes;
	protected string m_sSelected;

	//------------------------------------------------------------------------------------------------
	//! selected: the qualification to show first ("" = the first of the list).
	void CGQC_GU_QualCatalogueScreen(string selected = "")
	{
		m_sSelected = selected;
	}

	//------------------------------------------------------------------------------------------------
	override protected CGQC_GU_ERailTab GetRailTab()
	{
		return CGQC_GU_ERailTab.CAT_QUALS;
	}

	override protected string GetTitle()
	{
		return "QUALIFICATIONS";
	}

	override protected string GetReadAction()
	{
		return CGQC_GU_QualRequests.QUALS_CATALOGUE;
	}

	//------------------------------------------------------------------------------------------------
	override protected void BuildFrame(Widget content)
	{
		Widget row = CGQC_GU_W.HBox(content);
		if (!row)
			return;

		LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);

		Widget left = CGQC_GU_W.Sized(row, LIST_WIDTH);
		if (left)
		{
			LayoutSlot.SetVerticalAlign(left.GetParent(), LayoutVerticalAlign.Stretch);
			m_List.m_wRoot = CGQC_GU_W.Scroll(left);
		}

		Widget detail = DetailBox(row);
		if (detail)
		{
			Widget box = detail.GetParent();
			LayoutSlot.SetSizeMode(box, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(box, LayoutVerticalAlign.Stretch);
			LayoutSlot.SetPadding(box, 16, 0, 0, 0);
			box.SetVisible(false);
			m_wDetailBox = box;
			m_Detail.m_wRoot = CGQC_GU_W.Scroll(detail);
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Parse(string payload)
	{
		JsonLoadContext ctx = new JsonLoadContext();
		array<ref CGQC_GU_QualDto> quals = new array<ref CGQC_GU_QualDto>();
		if (!ctx.LoadFromString(payload) || !ctx.ReadValue("quals", quals))
			return false;

		array<ref CGQC_GU_LabelItem> notes = new array<ref CGQC_GU_LabelItem>();
		ctx.ReadValue("ppNotes", notes);
		m_aQuals = quals;
		m_aNotes = notes;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnAction(string kind, string value)
	{
		if (kind != "qual")
			return;

		m_sSelected = value;
		BuildList();
		BuildDetail();
		ScrollToTop(m_Detail);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Rebuild()
	{
		if (!m_aQuals)
			return;

		// The chosen qualification stays while the search still shows it.
		CGQC_GU_QualDto chosen = FindQual(m_sSelected);
		string needle = Needle();
		if (!chosen || !Matches(chosen, needle))
		{
			m_sSelected = string.Empty;
			foreach (CGQC_GU_QualDto q : m_aQuals)
			{
				if (Matches(q, needle))
				{
					m_sSelected = q.code;
					break;
				}
			}
		}

		BuildList();
		BuildDetail();
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_QualDto FindQual(string code)
	{
		if (!m_aQuals)
			return null;

		foreach (CGQC_GU_QualDto q : m_aQuals)
		{
			if (q.code == code)
				return q;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Matches(CGQC_GU_QualDto q, string needle)
	{
		return Found(needle, q.code + " " + q.title);
	}

	//------------------------------------------------------------------------------------------------
	//! The specialties of one group (QSP1, QSP2), in catalogue order.
	protected array<CGQC_GU_QualDto> GroupOf(string group)
	{
		array<CGQC_GU_QualDto> members = new array<CGQC_GU_QualDto>();
		foreach (CGQC_GU_QualDto q : m_aQuals)
		{
			if (q.group == group)
				members.Insert(q);
		}

		return members;
	}

	//------------------------------------------------------------------------------------------------
	//! "QSP1-MED" -> "MED"
	protected string Specialty(CGQC_GU_QualDto q)
	{
		int skip = q.group.Length() + 1;
		if (q.code.Length() <= skip)
			return q.code;

		return q.code.Substring(skip, q.code.Length() - skip);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildList()
	{
		m_List.Reset();
		Widget list = m_List.m_wRoot;
		if (!list)
			return;

		string needle = Needle();
		CGQC_GU_QualDto chosen = FindQual(m_sSelected);
		if (!chosen)
		{
			CGQC_GU_W.Text(list, "Aucune qualification.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
			return;
		}

		for (int pp = 1; pp <= 3; pp++)
		{
			Widget col;
			array<string> groups = new array<string>();
			foreach (CGQC_GU_QualDto q : m_aQuals)
			{
				if (q.pp != pp || !Matches(q, needle))
					continue;

				if (!col)
					col = PpGroup(list, pp, q.ppLabel);

				if (q.group.IsEmpty())
				{
					ListRow(col, q.code, q.title, q.levelAbbr, q.code == chosen.code, q.code);
					continue;
				}

				// A group of specialties is one row, as in the mockup; the detail picks the specialty.
				if (groups.Contains(q.group))
					continue;

				groups.Insert(q.group);
				array<CGQC_GU_QualDto> members = GroupOf(q.group);
				array<string> names = new array<string>();
				foreach (CGQC_GU_QualDto m : members)
				{
					names.Insert(Specialty(m));
				}

				bool selected = chosen.group == q.group;
				string pick = q.code;
				if (selected)
					pick = chosen.code;

				string title = string.Format("1 sur %1 : %2", members.Count(), CGQC_GU_Str.Join(names, " / "));
				ListRow(col, q.group + "-x", title, q.levelAbbr, selected, pick);
			}

			if (!col)
				continue;

			foreach (CGQC_GU_LabelItem note : m_aNotes)
			{
				if (note.id == pp.ToString())
					CGQC_GU_W.Para(col, note.label, CGQC_GU_Style.FONT_LABEL, 14, CGQC_GU_Style.InkMuted(), 2);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Colour bar and heading of one PP level. Returns the stack its rows go into.
	protected Widget PpGroup(Widget list, int pp, string label)
	{
		Widget row = CGQC_GU_W.HBox(list);
		if (!row)
			return null;

		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(row, 0, 0, 0, 14);

		ImageWidget bar = ImageWidget.Cast(CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, row));
		if (bar)
		{
			bar.SetSize(6, 2);
			bar.SetColor(CGQC_GU_Style.PpColor(pp));
			LayoutSlot.SetVerticalAlign(bar, LayoutVerticalAlign.Stretch);
		}

		Widget col = CGQC_GU_W.VBox(row);
		if (!col)
			return null;

		LayoutSlot.SetSizeMode(col, LayoutSizeMode.Fill);
		LayoutSlot.SetPadding(col, 12, 0, 0, 0);
		TextWidget t = CGQC_GU_W.Text(col, CGQC_GU_Str.UpperFr(label), CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Ink());
		if (t)
			LayoutSlot.SetPadding(t, 0, 0, 0, 6);

		return col;
	}

	//------------------------------------------------------------------------------------------------
	protected void ListRow(Widget col, string code, string title, string level, bool selected, string pick)
	{
		Color fill = CGQC_GU_Style.Clear();
		Color edge = CGQC_GU_Style.RowEdge();
		if (selected)
		{
			fill = CGQC_GU_Style.RowSelected();
			edge = CGQC_GU_Style.Blue();
		}

		CGQC_GU_ClickRow row = Row(m_List, col, "qual", pick, fill, edge, { "EdgeT", "EdgeB", "EdgeL", "EdgeR" });
		if (!row)
			return;

		LayoutSlot.SetPadding(row.GetRootWidget(), 0, 0, 0, 6);
		Widget cells = row.GetCells();
		AlignableSlot.SetPadding(cells, 10, 7, 10, 7);
		Cell(cells, code, 110, CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.Ink());
		Cell(cells, title, 0, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		Cell(cells, level, 60, CGQC_GU_Style.FONT_LABEL, 14, CGQC_GU_Style.InkMuted());
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildDetail()
	{
		m_Detail.Reset();
		Widget d = m_Detail.m_wRoot;
		if (!d)
			return;

		CGQC_GU_QualDto q = FindQual(m_sSelected);
		if (m_wDetailBox)
			m_wDetailBox.SetVisible(q != null);

		if (!q)
			return;

		string cap = q.ppLabel;
		if (!q.levelLabel.IsEmpty())
			cap = cap + " · " + q.levelLabel;

		CGQC_GU_W.Text(d, CGQC_GU_Str.UpperFr(cap), CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
		CGQC_GU_W.Para(d, q.code + " — " + q.title, CGQC_GU_Style.FONT_TITLE, 28, CGQC_GU_Style.Ink(), 6);

		if (!q.group.IsEmpty())
		{
			Widget picks = CGQC_GU_W.HBox(d);
			if (picks)
			{
				LayoutSlot.SetPadding(picks, 0, 6, 0, 2);
				array<CGQC_GU_QualDto> members = GroupOf(q.group);
				foreach (CGQC_GU_QualDto m : members)
				{
					CGQC_GU_Button b = Btn(m_Detail, picks, Specialty(m), "qual", m.code);
					if (!b)
						continue;

					b.SetSelected(m.code == q.code);
					LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
				}
			}
		}

		if (!q.description.IsEmpty())
			CGQC_GU_W.Para(d, q.description, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkSoft(), 6);

		if (q.aVenir)
		{
			CGQC_GU_W.Para(d, "« à venir »", CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkMuted(), 8);
		}
		else
		{
			BuildExercises(d, q);

			Section(d, "CRITÈRES");
			if (q.criteria.IsEmpty())
				Upcoming(d);

			foreach (CGQC_GU_LabelItem c : q.criteria)
			{
				Bullet(d, c.label);
			}

			Section(d, "À LA RÉUSSITE");
			if (q.onSuccess.IsEmpty())
				Upcoming(d);

			foreach (string line : q.onSuccess)
			{
				Bullet(d, line);
			}

			Section(d, "TEXTE COMPLET");
			if (q.text.IsEmpty())
				Upcoming(d);
			else
				CGQC_GU_W.Para(d, q.text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		}

		BuildInstructors(d, q);
	}

	//------------------------------------------------------------------------------------------------
	protected void Upcoming(Widget d)
	{
		CGQC_GU_W.Text(d, "à venir", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
	}

	//------------------------------------------------------------------------------------------------
	protected void Bullet(Widget d, string text)
	{
		CGQC_GU_W.Para(d, "• " + text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft(), 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildExercises(Widget d, CGQC_GU_QualDto q)
	{
		Section(d, "EXERCICES");
		if (q.exercises.IsEmpty())
			Upcoming(d);

		foreach (CGQC_GU_ExerciseDto e : q.exercises)
		{
			Widget row = CGQC_GU_W.HBox(d);
			if (!row)
				continue;

			LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(row, 0, 3, 0, 3);
			Cell(row, e.code, 120, CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.Ink());
			Cell(row, e.title, 0, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
			Widget tagBox = CGQC_GU_W.Sized(row, 90);
			if (tagBox && e.required)
				CGQC_GU_W.Tag(tagBox, "REQUIS", CGQC_GU_Style.Blue());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Who may teach it, with the online dot. Sent by the server with the most students evaluated first;
	//! the count itself is not shown (owner, 3e test).
	protected void BuildInstructors(Widget d, CGQC_GU_QualDto q)
	{
		Section(d, string.Format("INSTRUCTEURS QUALIFIÉS · %1", q.instructors.Count()));
		if (q.instructors.IsEmpty())
		{
			CGQC_GU_W.Text(d, "Aucun instructeur autorisé.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
			return;
		}

		Widget row;
		for (int i = 0; i < q.instructors.Count(); i++)
		{
			if (i % CHIPS_PER_ROW == 0)
			{
				row = CGQC_GU_W.HBox(d);
				if (row)
					LayoutSlot.SetPadding(row, 0, 0, 0, 8);
			}

			if (row)
				Chip(row, q.instructors[i]);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Chip(Widget row, CGQC_GU_InstructorChip chip)
	{
		Widget inner = CGQC_GU_W.Panel(row);
		if (!inner)
			return;

		LayoutSlot.SetPadding(inner.GetParent(), 0, 0, 8, 0);
		AlignableSlot.SetPadding(inner, 10, 6, 10, 6);
		Widget line = CGQC_GU_W.HBox(inner);
		if (!line)
			return;

		Color dot = CGQC_GU_Style.Offline();
		if (chip.online)
			dot = CGQC_GU_Style.Online();

		ImageWidget d = CGQC_GU_W.Dot(line, 10, dot);
		if (d)
			LayoutSlot.SetPadding(d, 0, 0, 8, 0);

		CGQC_GU_W.Text(line, chip.name, CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Ink());
	}
}

//! Unité → Liste des exercices: a qualification filter, the table, and the chosen exercise's full text.
class CGQC_GU_ExerciseCatalogueScreen : CGQC_GU_CatalogueScreen
{
	static const int FILTERS_PER_ROW = 10;
	static const float DETAIL_HEIGHT = 320;

	protected ref array<ref CGQC_GU_ExerciseDto> m_aExercises;
	protected ref array<ref CGQC_GU_LabelItem> m_aQuals;
	protected string m_sQual;			// filter: qualification code, "" = Toutes
	protected bool m_bFilterOpen;
	protected string m_sSelected;		// exercise code
	protected ref CGQC_GU_Region m_Filter = new CGQC_GU_Region();

	//------------------------------------------------------------------------------------------------
	override protected CGQC_GU_ERailTab GetRailTab()
	{
		return CGQC_GU_ERailTab.CAT_EXERCISES;
	}

	override protected string GetTitle()
	{
		return "EXERCICES";
	}

	override protected string GetReadAction()
	{
		return CGQC_GU_QualRequests.EXERCISES_CATALOGUE;
	}

	//------------------------------------------------------------------------------------------------
	override protected void BuildFrame(Widget content)
	{
		m_Filter.m_wRoot = CGQC_GU_W.VBox(content);
		if (m_Filter.m_wRoot)
		{
			LayoutSlot.SetHorizontalAlign(m_Filter.m_wRoot, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(m_Filter.m_wRoot, 0, 0, 0, 10);
		}

		m_List.m_wRoot = CGQC_GU_W.Scroll(content);

		Widget sized = CGQC_GU_W.Sized(content, 0, DETAIL_HEIGHT);
		if (!sized)
			return;

		LayoutSlot.SetHorizontalAlign(sized.GetParent(), LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(sized.GetParent(), 0, 12, 0, 0);
		Widget detail = DetailBox(sized);
		if (!detail)
			return;

		LayoutSlot.SetSizeMode(detail.GetParent(), LayoutSizeMode.Fill);
		m_wDetailBox = sized.GetParent();
		m_wDetailBox.SetVisible(false);
		m_Detail.m_wRoot = CGQC_GU_W.Scroll(detail);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		m_Filter.Reset();
		super.OnTeardown();
	}

	//------------------------------------------------------------------------------------------------
	override protected bool Parse(string payload)
	{
		JsonLoadContext ctx = new JsonLoadContext();
		array<ref CGQC_GU_ExerciseDto> exercises = new array<ref CGQC_GU_ExerciseDto>();
		array<ref CGQC_GU_LabelItem> quals = new array<ref CGQC_GU_LabelItem>();
		if (!ctx.LoadFromString(payload) || !ctx.ReadValue("exercises", exercises) || !ctx.ReadValue("quals", quals))
			return false;

		m_aExercises = exercises;
		m_aQuals = quals;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnAction(string kind, string value)
	{
		switch (kind)
		{
			case "filter.toggle":
				m_bFilterOpen = !m_bFilterOpen;
				BuildFilter();
				break;

			case "filter.pick":
				m_sQual = value;
				m_bFilterOpen = false;
				Rebuild();
				ScrollToTop(m_List);
				ScrollToTop(m_Detail);
				break;

			case "exercise":
				m_sSelected = value;
				BuildTable();
				BuildDetail();
				ScrollToTop(m_Detail);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void Rebuild()
	{
		if (!m_aExercises)
			return;

		// The chosen exercise stays while the filters still show it.
		array<CGQC_GU_ExerciseDto> shown = Shown();
		bool kept;
		foreach (CGQC_GU_ExerciseDto e : shown)
		{
			if (e.code == m_sSelected)
				kept = true;
		}

		if (!kept)
		{
			m_sSelected = string.Empty;
			if (!shown.IsEmpty())
				m_sSelected = shown[0].code;
		}

		BuildFilter();
		BuildTable();
		BuildDetail();
	}

	//------------------------------------------------------------------------------------------------
	//! Exercises that pass the qualification filter and the search, in catalogue order.
	protected array<CGQC_GU_ExerciseDto> Shown()
	{
		string needle = Needle();
		array<CGQC_GU_ExerciseDto> shown = new array<CGQC_GU_ExerciseDto>();
		foreach (CGQC_GU_ExerciseDto e : m_aExercises)
		{
			if (!m_sQual.IsEmpty() && e.qual != m_sQual)
				continue;

			if (Found(needle, e.code + " " + e.title + " " + e.qual + " " + e.description))
				shown.Insert(e);
		}

		return shown;
	}

	//------------------------------------------------------------------------------------------------
	//! Qualifications with no exercise yet, as shown in the table: the QSP specialties as one line.
	protected array<string> WithoutExercises()
	{
		string needle = Needle();
		array<string> codes = new array<string>();
		foreach (CGQC_GU_LabelItem q : m_aQuals)
		{
			if (!m_sQual.IsEmpty() && q.id != m_sQual)
				continue;

			bool has = false;
			foreach (CGQC_GU_ExerciseDto e : m_aExercises)
			{
				if (e.qual == q.id)
					has = true;
			}

			if (has || !Found(needle, q.id + " " + q.label))
				continue;

			string code = q.id;
			int dash = code.IndexOf("-");
			if (m_sQual.IsEmpty() && code.IndexOf("QSP") == 0 && dash > 0)
				code = code.Substring(0, dash) + "-x";

			if (!codes.Contains(code))
				codes.Insert(code);
		}

		return codes;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildFilter()
	{
		m_Filter.Reset();
		Widget f = m_Filter.m_wRoot;
		if (!f || !m_aQuals)
			return;

		Widget row = CGQC_GU_W.HBox(f);
		if (!row)
			return;

		TextWidget cap = CGQC_GU_W.Text(row, "QUALIFICATION", CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Ink());
		if (cap)
		{
			LayoutSlot.SetVerticalAlign(cap, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(cap, 0, 0, 12, 0);
		}

		string label = m_sQual;
		if (label.IsEmpty())
			label = "Toutes";

		if (m_bFilterOpen)
			label = label + "  −";
		else
			label = label + "  +";

		Btn(m_Filter, row, label, "filter.toggle", "");

		if (!m_bFilterOpen)
			return;

		array<string> codes = { "" };
		foreach (CGQC_GU_LabelItem q : m_aQuals)
		{
			codes.Insert(q.id);
		}

		Widget line;
		for (int i = 0; i < codes.Count(); i++)
		{
			if (i % FILTERS_PER_ROW == 0)
			{
				line = CGQC_GU_W.HBox(f);
				if (line)
					LayoutSlot.SetPadding(line, 0, 8, 0, 0);
			}

			if (!line)
				break;

			string text = codes[i];
			if (text.IsEmpty())
				text = "Toutes";

			CGQC_GU_Button b = Btn(m_Filter, line, text, "filter.pick", codes[i]);
			if (!b)
				continue;

			b.SetSelected(codes[i] == m_sQual);
			LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildTable()
	{
		m_List.Reset();
		Widget list = m_List.m_wRoot;
		if (!list)
			return;

		Color head = CGQC_GU_Style.Translucent(0x3A, 0x31, 0x23, 0.07);
		TableRow(list, "", "", head, { "CODE", "EXERCICE", "QUALIF.", "DESCRIPTION", "REQUIS" }, true, false);

		array<CGQC_GU_ExerciseDto> shown = Shown();
		foreach (CGQC_GU_ExerciseDto e : shown)
		{
			Color fill = CGQC_GU_Style.Clear();
			if (e.code == m_sSelected)
				fill = CGQC_GU_Style.RowSelected();

			TableRow(list, "exercise", e.code, fill, { e.code, e.title, e.qual, e.description, "" }, false, e.required);
		}

		array<string> empty = WithoutExercises();
		foreach (string code : empty)
		{
			TableRow(list, "", "", CGQC_GU_Style.Clear(), { code + "-E…", "[à venir]", code, "", "" }, false, false, true);
		}

		if (shown.IsEmpty() && empty.IsEmpty())
			CGQC_GU_W.Para(list, "Aucun exercice.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 8);
	}

	//------------------------------------------------------------------------------------------------
	//! One line of the table: five cells (code, exercise, qualification, description, Requis).
	protected void TableRow(Widget list, string kind, string value, Color fill, array<string> texts, bool heading, bool required, bool muted = false)
	{
		array<string> edges = { "EdgeB", "EdgeL", "EdgeR" };
		if (heading)
			edges.Insert("EdgeT");

		CGQC_GU_ClickRow row = Row(m_List, list, kind, value, fill, CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return;

		Widget cells = row.GetCells();
		array<float> widths = { 150, 280, 130, 0, 150 };
		for (int i = 0; i < widths.Count(); i++)
		{
			if (i > 0)
				CGQC_GU_Table.Separator(cells);

			ResourceName font = CGQC_GU_Style.FONT_VALUE;
			int size = 16;
			Color color = CGQC_GU_Style.InkSoft();
			if (heading)
			{
				font = CGQC_GU_Style.FONT_TITLE;
				size = 14;
				color = CGQC_GU_Style.Ink();
			}
			else if (muted)
			{
				color = CGQC_GU_Style.InkMuted();
			}
			else if (i == 0)
			{
				font = CGQC_GU_Style.FONT_TITLE;
			}

			if (i == 4 && required)
			{
				Widget tagBox = CGQC_GU_W.Sized(cells, widths[i]);
				if (tagBox)
				{
					LayoutSlot.SetVerticalAlign(tagBox.GetParent(), LayoutVerticalAlign.Center);
					LayoutSlot.SetPadding(tagBox.GetParent(), 10, 6, 10, 6);
					CGQC_GU_W.Tag(tagBox, "REQUIS", CGQC_GU_Style.Blue());
				}

				continue;
			}

			Cell(cells, texts[i], widths[i], font, size, color, 10, 8);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildDetail()
	{
		m_Detail.Reset();
		Widget d = m_Detail.m_wRoot;
		if (!d)
			return;

		CGQC_GU_ExerciseDto chosen;
		foreach (CGQC_GU_ExerciseDto e : m_aExercises)
		{
			if (e.code == m_sSelected)
				chosen = e;
		}

		if (m_wDetailBox)
			m_wDetailBox.SetVisible(chosen != null);

		if (!chosen)
			return;

		CGQC_GU_W.Text(d, "EXERCICE CHOISI · " + chosen.qual, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
		CGQC_GU_W.Para(d, chosen.code + " — " + chosen.title, CGQC_GU_Style.FONT_TITLE, 26, CGQC_GU_Style.Ink(), 6);
		if (!chosen.description.IsEmpty())
			CGQC_GU_W.Para(d, chosen.description, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkSoft(), 6);

		Section(d, "TEXTE COMPLET");
		string text = chosen.text;
		if (text.IsEmpty())
			text = "à venir";

		CGQC_GU_W.Para(d, text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
	}
}
