// Gestion d'unité - « Instructeurs de l'unité » (paper dialog on a dark ground), Phase 3e, after the
// approved mockup (Instructeurs). Read-only, for every member; opened from Dossier → Instructeur, and X
// goes back there. Everything shown comes from the server's "instructors" reply.

//! Binds a button to a named action of the dialog, with a value (qualification code).
class CGQC_GU_InstructorsAction : CGQC_GU_Action
{
	protected CGQC_GU_InstructorsDialog m_Dialog;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_InstructorsAction(CGQC_GU_InstructorsDialog dialog, string kind, string value)
	{
		m_Dialog = dialog;
		m_sKind = kind;
		m_sValue = value;
	}

	//------------------------------------------------------------------------------------------------
	override void Fire()
	{
		if (m_Dialog)
			m_Dialog.OnAction(m_sKind, m_sValue);
	}
}

class CGQC_GU_InstructorsDialog : CGQC_GU_Screen
{
	static const float WIDTH = 1300;
	static const float HEIGHT = 940;
	static const int TAGS_PER_ROW = 6;
	static const int PICKS_PER_ROW = 8;

	protected string m_sUid;		// the dossier to go back to
	protected ref array<string> m_aCodes = new array<string>();	// every qualification, for the filter
	protected string m_sFilter;		// "" = all
	protected bool m_bFilterOpen;
	protected int m_iReadReq;
	protected ref array<ref CGQC_GU_InstructorRow> m_aRows;

	protected ref CGQC_GU_Region m_Head = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_List = new CGQC_GU_Region();

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_InstructorsDialog(string uid, array<string> codes)
	{
		m_sUid = uid;
		if (codes)
			m_aCodes.Copy(codes);
	}

	//------------------------------------------------------------------------------------------------
	override protected ResourceName GetLayout()
	{
		return CGQC_GU_Style.LAYOUT_PAPER_SHELL;
	}

	//------------------------------------------------------------------------------------------------
	//! The paper shell is only used for its full-screen ground: its page and rail are hidden, and the
	//! dialog is a sheet in the middle.
	override protected void OnBuild()
	{
		ImageWidget ground = ImageWidget.Cast(Find("Bg"));
		if (ground)
			ground.SetColor(Color.FromSRGBA(0x1A, 0x1C, 0x17, 255));

		Widget main = Find("Main");
		if (main)
			main.SetVisible(false);

		Widget rail = Find("Rail");
		if (rail)
			rail.SetVisible(false);

		Widget sheet = CGQC_GU_W.Panel(m_wRoot);
		if (!sheet)
			return;

		Widget box = sheet.GetParent();
		FrameSlot.SetAnchorMin(box, 0.5, 0.5);
		FrameSlot.SetAnchorMax(box, 0.5, 0.5);
		FrameSlot.SetAlignment(box, 0.5, 0.5);
		FrameSlot.SetSizeToContent(box, false);
		FrameSlot.SetPos(box, 0, 0);
		FrameSlot.SetSize(box, WIDTH, HEIGHT);

		Widget bg = box.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(CGQC_GU_Style.Parchment());

		AlignableSlot.SetPadding(sheet, 40, 32, 40, 32);

		m_Head.m_wRoot = CGQC_GU_W.VBox(sheet);
		if (m_Head.m_wRoot)
			LayoutSlot.SetHorizontalAlign(m_Head.m_wRoot, LayoutHorizontalAlign.Stretch);

		m_List.m_wRoot = CGQC_GU_W.Scroll(sheet);

		BuildHead();
		BuildList();

		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		m_iReadReq = CGQC_GU_Client.Request(CGQC_GU_QualRequests.INSTRUCTORS);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		m_Head.Reset();
		m_List.Reset();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot || requestId != m_iReadReq)
			return;

		array<ref CGQC_GU_InstructorRow> rows = new array<ref CGQC_GU_InstructorRow>();
		JsonLoadContext ctx = new JsonLoadContext();
		if (ok && ctx.LoadFromString(payload) && ctx.ReadValue("instructors", rows))
		{
			m_aRows = rows;
			BuildList();
			return;
		}

		string text = "Liste illisible.";
		if (!ok)
			text = CGQC_GU_DossierScreen.ErrorText(code);

		m_List.Reset();
		CGQC_GU_W.Para(m_List.m_wRoot, text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.Red());
	}

	//------------------------------------------------------------------------------------------------
	void OnAction(string kind, string value)
	{
		switch (kind)
		{
			case "close":
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_DossierScreen(m_sUid, CGQC_GU_ERailTab.INSTRUCTOR));
				break;

			case "filter.toggle":
				m_bFilterOpen = !m_bFilterOpen;
				BuildHead();
				break;

			case "filter.pick":
				m_sFilter = value;
				m_bFilterOpen = false;
				BuildHead();
				BuildList();
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button Btn(CGQC_GU_Region region, Widget parent, string text, string kind, string value = "")
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text);
		if (!b)
			return null;

		region.m_aButtons.Insert(b);
		b.SetCompact();
		CGQC_GU_InstructorsAction a = new CGQC_GU_InstructorsAction(this, kind, value);
		region.m_aKeep.Insert(a);
		b.GetOnActivated().Insert(a.Fire);
		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildHead()
	{
		m_Head.Reset();
		Widget head = m_Head.m_wRoot;
		if (!head)
			return;

		Widget row = CGQC_GU_W.HBox(head);
		if (row)
		{
			LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);

			Widget titles = CGQC_GU_W.VBox(row);
			if (titles)
			{
				LayoutSlot.SetSizeMode(titles, LayoutSizeMode.Fill);
				CGQC_GU_W.Text(titles, "UNITÉ", CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.InkMuted());
				CGQC_GU_W.Text(titles, "INSTRUCTEURS DE L'UNITÉ", CGQC_GU_Style.FONT_TITLE, 32, CGQC_GU_Style.Ink());
			}

			string label = "TOUTES LES QUALIFICATIONS";
			if (!m_sFilter.IsEmpty())
				label = m_sFilter;

			if (m_bFilterOpen)
				label = label + "  −";
			else
				label = label + "  +";

			CGQC_GU_Button filter = Btn(m_Head, row, label, "filter.toggle");
			if (filter)
				LayoutSlot.SetVerticalAlign(filter.GetRootWidget(), LayoutVerticalAlign.Center);

			CGQC_GU_Button close = Btn(m_Head, row, "X", "close");
			if (close)
			{
				LayoutSlot.SetVerticalAlign(close.GetRootWidget(), LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(close.GetRootWidget(), 16, 0, 0, 0);
			}
		}

		CGQC_GU_W.Rule(head, 10, 12);

		if (!m_bFilterOpen)
			return;

		Widget picks;
		for (int i = -1; i < m_aCodes.Count(); i++)
		{
			if ((i + 1) % PICKS_PER_ROW == 0)
			{
				picks = CGQC_GU_W.HBox(head);
				if (picks)
					LayoutSlot.SetPadding(picks, 0, 0, 0, 8);
			}

			if (!picks)
				break;

			// The first choice is « Toutes ».
			string code;
			string text = "TOUTES";
			if (i >= 0)
			{
				code = m_aCodes[i];
				text = code;
			}

			CGQC_GU_Button b = Btn(m_Head, picks, text, "filter.pick", code);
			if (!b)
				continue;

			b.SetSelected(code == m_sFilter);
			LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildList()
	{
		m_List.Reset();
		Widget list = m_List.m_wRoot;
		if (!list)
			return;

		if (!m_aRows)
		{
			CGQC_GU_W.Para(list, "Chargement…", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
			return;
		}

		array<float> widths = { 260, 120, 0, 150 };
		CGQC_GU_Table.Head(m_List, list, { "INSTRUCTEUR", "GRADE", "QUALIFICATIONS ENSEIGNÉES", "ÉLÈVES ÉVALUÉS" }, widths);

		int shown;
		foreach (CGQC_GU_InstructorRow r : m_aRows)
		{
			if (!m_sFilter.IsEmpty() && !r.quals.Contains(m_sFilter))
				continue;

			shown++;
			Widget cells = CGQC_GU_Table.Row(m_List, list);
			CGQC_GU_Table.Value(cells, 0, r.name, widths[0], true);
			CGQC_GU_Table.Value(cells, 1, r.rankAbbr, widths[1]);

			Widget tags = CGQC_GU_Table.Stack(cells, 2, widths[2]);
			Widget line;
			for (int i = 0; i < r.quals.Count(); i++)
			{
				if (i % TAGS_PER_ROW == 0)
				{
					line = CGQC_GU_W.HBox(tags);
					if (line)
						LayoutSlot.SetPadding(line, 0, 2, 0, 2);
				}

				Widget tag = CGQC_GU_W.Tag(line, r.quals[i], CGQC_GU_Style.Blue(), 13);
				if (tag)
					LayoutSlot.SetPadding(tag, 0, 0, 8, 0);
			}

			CGQC_GU_Table.Value(cells, 3, r.students.ToString(), widths[3]);
		}

		if (shown == 0)
			CGQC_GU_W.Para(list, "Aucun instructeur autorisé.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 8);
	}
}
