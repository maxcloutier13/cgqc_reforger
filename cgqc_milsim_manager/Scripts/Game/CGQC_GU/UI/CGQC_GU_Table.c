// Gestion d'unité - ruled tables and pressable rows of the paper screens (catalogue lists, Dossier tabs).

//! Something a pressed row does.
class CGQC_GU_Action : Managed
{
	void Fire();
}

//! A row that can be pressed over its whole width: the button layout without its label; the cells go into
//! GetCells(). The action runs on the next frame, because it rebuilds the row's own list.
class CGQC_GU_ClickRow : ScriptedWidgetEventHandler
{
	protected Widget m_wRoot;
	protected Widget m_wCells;
	protected ref CGQC_GU_Action m_Action;
	protected ref CGQC_GU_Action m_DoubleAction;
	protected ref CGQC_GU_Action m_RightAction;

	//------------------------------------------------------------------------------------------------
	//! action may be null (a row that only shows). edges: names of the outline sides to draw.
	static CGQC_GU_ClickRow Create(Widget parent, CGQC_GU_Action action, Color fill, Color edge, array<string> edges)
	{
		Widget root = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_BUTTON, parent);
		if (!root)
			return null;

		CGQC_GU_ClickRow row = new CGQC_GU_ClickRow();
		row.m_wRoot = root;
		row.m_Action = action;
		LayoutSlot.SetHorizontalAlign(root, LayoutHorizontalAlign.Stretch);

		Widget bg = root.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(fill);

		array<string> sides = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string side : sides)
		{
			Widget e = root.FindAnyWidget(side);
			if (!e)
				continue;

			e.SetVisible(edges.Contains(side));
			e.SetColor(edge);
		}

		Widget label = root.FindAnyWidget("Label");
		if (label)
			label.SetVisible(false);

		row.m_wCells = CGQC_GU_W.HBox(root.FindAnyWidget("Body"));
		if (row.m_wCells)
		{
			AlignableSlot.SetHorizontalAlign(row.m_wCells, LayoutHorizontalAlign.Stretch);
			AlignableSlot.SetVerticalAlign(row.m_wCells, LayoutVerticalAlign.Stretch);
		}

		root.AddHandler(row);
		return row;
	}

	//------------------------------------------------------------------------------------------------
	Widget GetRootWidget()
	{
		return m_wRoot;
	}

	//------------------------------------------------------------------------------------------------
	Widget GetCells()
	{
		return m_wCells;
	}

	//------------------------------------------------------------------------------------------------
	//! Optional: a double click (after the click's own action) and a right click (instead of it).
	void SetExtraActions(CGQC_GU_Action doubleAction, CGQC_GU_Action rightAction)
	{
		m_DoubleAction = doubleAction;
		m_RightAction = rightAction;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_RightAction && button != 0)
			return true;

		if (m_Action)
			GetGame().GetCallqueue().CallLater(Activate, 0, false);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (!m_DoubleAction || button != 0)
			return false;

		GetGame().GetCallqueue().CallLater(ActivateDouble, 0, false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (!m_RightAction || button != 1)
			return false;

		GetGame().GetCallqueue().CallLater(ActivateRight, 0, false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Activate()
	{
		if (m_Action)
			m_Action.Fire();
	}

	//------------------------------------------------------------------------------------------------
	protected void ActivateDouble()
	{
		if (m_DoubleAction)
			m_DoubleAction.Fire();
	}

	//------------------------------------------------------------------------------------------------
	protected void ActivateRight()
	{
		if (m_RightAction)
			m_RightAction.Fire();
	}

	//------------------------------------------------------------------------------------------------
	//! Detaches the row while its widget is alive, then removes it. Never do this from a destructor: the
	//! widget releases its handlers while it is being destroyed, and removing one then crashes the game.
	void Destroy()
	{
		if (!m_wRoot)
			return;

		m_wRoot.RemoveHandler(this);
		m_wRoot.RemoveFromHierarchy();
		m_wRoot = null;
	}
}

//! A read-only ruled table: a heading line, then one line per entry. A column of width 0 takes what is left;
//! the same widths are given to the heading and to every line, so the columns stay aligned.
class CGQC_GU_Table
{
	static const float PAD_X = 10;
	static const float PAD_Y = 8;

	//------------------------------------------------------------------------------------------------
	//! One empty line, kept by the region. Returns the stack its cells go into.
	static Widget Row(CGQC_GU_Region region, Widget list, bool heading = false)
	{
		array<string> edges = { "EdgeB", "EdgeL", "EdgeR" };
		Color fill = CGQC_GU_Style.Clear();
		if (heading)
		{
			edges.Insert("EdgeT");
			fill = CGQC_GU_Style.Translucent(0x3A, 0x31, 0x23, 0.07);
		}

		CGQC_GU_ClickRow row = CGQC_GU_ClickRow.Create(list, null, fill, CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return null;

		region.m_aRows.Insert(row);
		return row.GetCells();
	}

	//------------------------------------------------------------------------------------------------
	static void Head(CGQC_GU_Region region, Widget list, array<string> titles, array<float> widths)
	{
		Widget cells = Row(region, list, true);
		for (int i = 0; i < titles.Count(); i++)
		{
			CGQC_GU_W.Para(Stack(cells, i, widths[i]), titles[i], CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.Ink());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The next cell of a line, empty: a vertical stack for the content. column > 0 draws the separator.
	static Widget Stack(Widget cells, int column, float width)
	{
		if (column > 0)
			Separator(cells);

		Widget box;
		Widget slot;
		if (width > 0)
		{
			box = CGQC_GU_W.Sized(cells, width);
			if (!box)
				return null;

			slot = box.GetParent();
		}
		else
		{
			box = CGQC_GU_W.VBox(cells);
			if (!box)
				return null;

			slot = box;
			LayoutSlot.SetSizeMode(slot, LayoutSizeMode.Fill);
		}

		LayoutSlot.SetVerticalAlign(slot, LayoutVerticalAlign.Center);
		LayoutSlot.SetPadding(slot, PAD_X, PAD_Y, PAD_X, PAD_Y);
		return box;
	}

	//------------------------------------------------------------------------------------------------
	//! A text cell. bold: the line's key column (code).
	static void Value(Widget cells, int column, string text, float width, bool bold = false)
	{
		ResourceName font = CGQC_GU_Style.FONT_VALUE;
		if (bold)
			font = CGQC_GU_Style.FONT_TITLE;

		CGQC_GU_W.Para(Stack(cells, column, width), text, font, 16, CGQC_GU_Style.InkSoft());
	}

	//------------------------------------------------------------------------------------------------
	static void Separator(Widget row)
	{
		ImageWidget line = ImageWidget.Cast(CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, row));
		if (!line)
			return;

		line.SetSize(1, 2);
		line.SetColor(CGQC_GU_Style.RowEdge());
		LayoutSlot.SetVerticalAlign(line, LayoutVerticalAlign.Stretch);
	}
}
