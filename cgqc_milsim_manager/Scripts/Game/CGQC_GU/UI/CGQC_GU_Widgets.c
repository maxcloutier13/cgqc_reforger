// Gestion d'unité - small building blocks for paper screens, built from one-widget layouts so screens
// can be composed in script: stacks, ruled panels, edit fields, checkboxes and the drawn rank insignia.

class CGQC_GU_W
{
	//------------------------------------------------------------------------------------------------
	static Widget Create(ResourceName layout, Widget parent)
	{
		if (!parent)
			return null;

		Widget w = GetGame().GetWorkspace().CreateWidgets(layout, parent);
		if (!w)
			CGQC_GU_Log.Error("Layout introuvable : " + layout);

		return w;
	}

	//------------------------------------------------------------------------------------------------
	static Widget VBox(Widget parent)
	{
		return Create(CGQC_GU_Style.LAYOUT_VBOX, parent);
	}

	//------------------------------------------------------------------------------------------------
	static Widget HBox(Widget parent)
	{
		return Create(CGQC_GU_Style.LAYOUT_HBOX, parent);
	}

	//------------------------------------------------------------------------------------------------
	//! Fixed-size box (0 = not fixed on that axis). Returns its inner vertical stack.
	static Widget Sized(Widget parent, float width, float height = 0)
	{
		SizeLayoutWidget s = SizeLayoutWidget.Cast(Create(CGQC_GU_Style.LAYOUT_SIZED, parent));
		if (!s)
			return null;

		if (width > 0)
		{
			s.EnableWidthOverride(true);
			s.SetWidthOverride(width);
			// A stretched slot would ignore the width: keep the box at its own size.
			LayoutSlot.SetHorizontalAlign(s, LayoutHorizontalAlign.Left);
		}

		if (height > 0)
		{
			s.EnableHeightOverride(true);
			s.SetHeightOverride(height);
		}

		return s.FindAnyWidget("Inner");
	}

	//------------------------------------------------------------------------------------------------
	//! Vertical scroll area filling what is left of the parent stack. Returns its inner vertical stack.
	static Widget Scroll(Widget parent)
	{
		Widget s = Create(CGQC_GU_Style.LAYOUT_SCROLL, parent);
		if (!s)
			return null;

		LayoutSlot.SetSizeMode(s, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(s, LayoutHorizontalAlign.Stretch);
		return s.FindAnyWidget("Inner");
	}

	//------------------------------------------------------------------------------------------------
	//! Wrapped paragraph across the parent stack.
	static TextWidget Para(Widget parent, string text, ResourceName font, int size, Color color, float padTop = 0)
	{
		TextWidget t = Text(parent, text, font, size, color, true);
		if (!t)
			return null;

		LayoutSlot.SetHorizontalAlign(t, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(t, 0, padTop, 0, 0);
		return t;
	}

	//------------------------------------------------------------------------------------------------
	//! Small boxed tag (« REQUIS »), outline and text in the same colour.
	static void Tag(Widget parent, string text, Color color)
	{
		Widget inner = Panel(parent);
		if (!inner)
			return;

		Widget box = inner.GetParent();
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Left);
		LayoutSlot.SetVerticalAlign(box, LayoutVerticalAlign.Center);
		AlignableSlot.SetPadding(inner, 8, 2, 8, 2);
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string edge : edges)
		{
			Widget e = box.FindAnyWidget(edge);
			if (e)
				e.SetColor(color);
		}

		Text(inner, text, CGQC_GU_Style.FONT_TITLE, 12, color);
	}

	//------------------------------------------------------------------------------------------------
	//! Ruled box on paper. Returns its inner vertical stack.
	static Widget Panel(Widget parent, bool tinted = false)
	{
		Widget p = Create(CGQC_GU_Style.LAYOUT_PANEL, parent);
		if (!p)
			return null;

		Widget bg = p.FindAnyWidget("Bg");
		if (bg)
		{
			if (tinted)
				bg.SetColor(CGQC_GU_Style.Translucent(0x3A, 0x31, 0x23, 0.06));
			else
				bg.SetColor(CGQC_GU_Style.Clear());
		}

		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string edge : edges)
		{
			Widget e = p.FindAnyWidget(edge);
			if (e)
				e.SetColor(CGQC_GU_Style.Rule());
		}

		return p.FindAnyWidget("Inner");
	}

	//------------------------------------------------------------------------------------------------
	//! Translucent dark panel for the dark screens (no outline). Returns its inner vertical stack.
	static Widget DarkPanel(Widget parent, Color color = null, float padX = 28, float padY = 22)
	{
		Widget p = Create(CGQC_GU_Style.LAYOUT_PANEL, parent);
		if (!p)
			return null;

		// A Color created here must be held by a local: assigning a new one to the parameter
		// left it null at SetColor (VM exception, 2c test).
		Color fill = color;
		if (!fill)
			fill = CGQC_GU_Style.DarkCard();

		Widget bg = p.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(fill);

		Widget innerStack = p.FindAnyWidget("Inner");
		if (innerStack)
			AlignableSlot.SetPadding(innerStack, padX, padY, padX, padY);

		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string edge : edges)
		{
			Widget e = p.FindAnyWidget(edge);
			if (e)
				e.SetVisible(false);
		}

		return p.FindAnyWidget("Inner");
	}

	//------------------------------------------------------------------------------------------------
	static TextWidget Text(Widget parent, string text, ResourceName font, int size, Color color, bool wrap = false)
	{
		TextWidget t = TextWidget.Cast(Create(CGQC_GU_Style.LAYOUT_TEXT, parent));
		if (!t)
			return null;

		t.SetText(text);
		t.SetFont(font);
		t.SetExactFontSize(size);
		t.SetColor(color);
		if (wrap)
			t.SetTextWrapping(true);

		return t;
	}

	//------------------------------------------------------------------------------------------------
	//! Small caps caption above a value or a section.
	static TextWidget Caption(Widget parent, string text, float padTop = 10)
	{
		TextWidget t = Text(parent, text, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
		if (t)
			LayoutSlot.SetPadding(t, 0, padTop, 0, 3);

		return t;
	}

	//------------------------------------------------------------------------------------------------
	//! Thin rule across the parent stack.
	static Widget Rule(Widget parent, float padTop = 8, float padBottom = 8)
	{
		ImageWidget r = ImageWidget.Cast(Create(CGQC_GU_Style.LAYOUT_RECT, parent));
		if (!r)
			return null;

		r.SetSize(2, 1.5);
		r.SetColor(CGQC_GU_Style.Rule());
		LayoutSlot.SetHorizontalAlign(r, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(r, 0, padTop, 0, padBottom);
		return r;
	}

	//------------------------------------------------------------------------------------------------
	//! Solid square (online dot, tags).
	static ImageWidget Dot(Widget parent, float size, Color color)
	{
		ImageWidget r = ImageWidget.Cast(Create(CGQC_GU_Style.LAYOUT_RECT, parent));
		if (!r)
			return null;

		r.SetSize(size, size);
		r.SetColor(color);
		LayoutSlot.SetVerticalAlign(r, LayoutVerticalAlign.Center);
		return r;
	}

	//------------------------------------------------------------------------------------------------
	//! Single-line edit field of a fixed width.
	static EditBoxWidget Field(Widget parent, string text, float width)
	{
		Widget box = Sized(parent, width);
		Widget f = Create(CGQC_GU_Style.LAYOUT_FIELD, box);
		if (!f)
			return null;

		LayoutSlot.SetHorizontalAlign(f, LayoutHorizontalAlign.Stretch);
		StyleField(f);
		EditBoxWidget e = EditBoxWidget.Cast(f.FindAnyWidget("Edit"));
		if (e)
		{
			e.SetText(text);
			e.SetColor(CGQC_GU_Style.InkSoft());
		}

		return e;
	}

	//------------------------------------------------------------------------------------------------
	//! Multi-line edit field (bio).
	static MultilineEditBoxWidget FieldMulti(Widget parent, string text, float width, float height)
	{
		Widget box = Sized(parent, width);
		SizeLayoutWidget f = SizeLayoutWidget.Cast(Create(CGQC_GU_Style.LAYOUT_FIELD_MULTI, box));
		if (!f)
			return null;

		LayoutSlot.SetHorizontalAlign(f, LayoutHorizontalAlign.Stretch);
		f.EnableHeightOverride(true);
		f.SetHeightOverride(height);
		StyleField(f);
		MultilineEditBoxWidget e = MultilineEditBoxWidget.Cast(f.FindAnyWidget("Edit"));
		if (e)
		{
			e.SetText(text);
			e.SetColor(CGQC_GU_Style.InkSoft());
		}

		return e;
	}

	//------------------------------------------------------------------------------------------------
	protected static void StyleField(Widget f)
	{
		Widget bg = f.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(CGQC_GU_Style.Field());

		Widget edge = f.FindAnyWidget("EdgeB");
		if (edge)
			edge.SetColor(CGQC_GU_Style.Rule());
	}

	//------------------------------------------------------------------------------------------------
	static void Clear(Widget w)
	{
		if (!w)
			return;

		while (w.GetChildren())
		{
			w.GetChildren().RemoveFromHierarchy();
		}
	}
}

//! Checkbox on paper: a ruled square with a blue tick, and a label.
class CGQC_GU_Check : ScriptedWidgetEventHandler
{
	protected Widget m_wRoot;
	protected Widget m_wTick;
	protected TextWidget m_wLabel;
	protected bool m_bChecked;
	protected bool m_bEnabled = true;
	protected string m_sId;

	protected ref ScriptInvokerVoid m_OnChanged = new ScriptInvokerVoid();

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Check Create(Widget parent, string id, string label, bool checked)
	{
		Widget root = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_CHECK, parent);
		if (!root)
			return null;

		CGQC_GU_Check c = new CGQC_GU_Check();
		c.Init(root, id, label, checked);
		return c;
	}

	//------------------------------------------------------------------------------------------------
	protected void Init(Widget root, string id, string label, bool checked)
	{
		m_wRoot = root;
		m_sId = id;
		m_bChecked = checked;
		m_wTick = root.FindAnyWidget("Tick");
		m_wLabel = TextWidget.Cast(root.FindAnyWidget("Label"));

		Widget outer = root.FindAnyWidget("Outer");
		if (outer)
			outer.SetColor(CGQC_GU_Style.Rule());

		Widget inner = root.FindAnyWidget("Inner");
		if (inner)
			inner.SetColor(CGQC_GU_Style.Field());

		if (m_wTick)
			m_wTick.SetColor(CGQC_GU_Style.Blue());

		if (m_wLabel)
		{
			m_wLabel.SetText(label);
			m_wLabel.SetColor(CGQC_GU_Style.InkSoft());
		}

		LayoutSlot.SetHorizontalAlign(root, LayoutHorizontalAlign.Left);
		root.AddHandler(this);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	//! Checkbox on a dark screen (mockup: light square, blue tick, light label).
	void SetDark(int fontSize = 17)
	{
		if (!m_wRoot)
			return;

		Widget outer = m_wRoot.FindAnyWidget("Outer");
		if (outer)
			outer.SetColor(CGQC_GU_Style.DarkSoft());

		Widget inner = m_wRoot.FindAnyWidget("Inner");
		if (inner)
			inner.SetColor(CGQC_GU_Style.DarkButton());

		if (m_wTick)
			m_wTick.SetColor(CGQC_GU_Style.BlueLight());

		if (m_wLabel)
		{
			m_wLabel.SetColor(CGQC_GU_Style.DarkText());
			m_wLabel.SetExactFontSize(fontSize);
		}
	}

	//------------------------------------------------------------------------------------------------
	Widget GetRootWidget()
	{
		return m_wRoot;
	}

	//------------------------------------------------------------------------------------------------
	void SetChecked(bool checked)
	{
		m_bChecked = checked;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	string GetId()
	{
		return m_sId;
	}

	//------------------------------------------------------------------------------------------------
	bool IsChecked()
	{
		return m_bChecked;
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvokerVoid GetOnChanged()
	{
		return m_OnChanged;
	}

	//------------------------------------------------------------------------------------------------
	void SetEnabled(bool enabled)
	{
		m_bEnabled = enabled;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_bEnabled)
			return true;

		m_bChecked = !m_bChecked;
		Refresh();
		m_OnChanged.Invoke();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (m_wTick)
			m_wTick.SetVisible(m_bChecked);

		if (!m_wRoot)
			return;

		if (m_bEnabled)
			m_wRoot.SetOpacity(1);
		else
			m_wRoot.SetOpacity(0.6);
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_wRoot)
		{
			m_wRoot.RemoveHandler(this);
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
	}
}

//! Calls back when the text of an edit field changes (search box).
class CGQC_GU_FieldWatch : ScriptedWidgetEventHandler
{
	protected ref ScriptInvokerVoid m_OnChanged = new ScriptInvokerVoid();

	//------------------------------------------------------------------------------------------------
	ScriptInvokerVoid GetOnChanged()
	{
		return m_OnChanged;
	}

	//------------------------------------------------------------------------------------------------
	override bool OnChange(Widget w, bool finished)
	{
		m_OnChanged.Invoke();
		return false;
	}
}

//! Placeholder rank insignia (spec §2): a slip-on with plain bars and squares, picked by the rank's
//! "insignia" key from ranks.json. Real images made by the owner replace this later.
//! Bars = chevrons, small square = maple leaf, large square = crown, pairs of squares = stars.
class CGQC_GU_Insignia
{
	static const float W = 84;
	static const float H = 112;

	//------------------------------------------------------------------------------------------------
	static Widget Draw(Widget parent, string key, float scale = 1)
	{
		SizeLayoutWidget s = SizeLayoutWidget.Cast(CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_CANVAS, parent));
		if (!s)
			return null;

		s.EnableWidthOverride(true);
		s.SetWidthOverride(W * scale);
		s.EnableHeightOverride(true);
		s.SetHeightOverride(H * scale);

		Widget c = s.FindAnyWidget("Canvas");
		if (!c)
			return s;

		Rect(c, scale, 0, 0, W, H, CGQC_GU_Style.Slip());

		switch (key)
		{
			case "sdt":		Bars(c, scale, 1, 40); break;
			case "cpl":		Bars(c, scale, 2, 40); break;
			case "cplc":	Leaf(c, scale); Bars(c, scale, 2, 50); break;
			case "sgt":		Leaf(c, scale); Bars(c, scale, 3, 50); break;
			case "adju":	Crown(c, scale, 40); break;
			case "adjum":	Crown(c, scale, 32); Rect(c, scale, 18, 72, 48, 4, CGQC_GU_Style.SlipMark()); break;
			case "adjuc":	Crown(c, scale, 24); Rect(c, scale, 26, 62, 32, 26, CGQC_GU_Style.SlipMark()); break;
			case "slt":		Stars(c, scale, 1, 0); break;
			case "lt":		Stars(c, scale, 2, 0); break;
			case "capt":	Stars(c, scale, 3, 0); break;
			case "maj":		Crown(c, scale, 40); break;
			case "lcol":	Crown(c, scale, 20); Stars(c, scale, 1, 50); break;
			case "col":		Crown(c, scale, 14); Stars(c, scale, 2, 40); Rect(c, scale, 0, H - 10, W, 10, CGQC_GU_Style.Gorget()); break;
			case "vet":		Rect(c, scale, 18, 50, 48, 6, CGQC_GU_Style.SlipMark()); break;
		}

		return s;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Rect(Widget canvas, float scale, float x, float y, float w, float h, Color color)
	{
		Widget r = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, canvas);
		if (!r)
			return;

		FrameSlot.SetPos(r, x * scale, y * scale);
		FrameSlot.SetSize(r, w * scale, h * scale);
		r.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Bars(Widget c, float scale, int count, float top)
	{
		for (int i = 0; i < count; i++)
		{
			Rect(c, scale, 14, top + i * 14, W - 28, 7, CGQC_GU_Style.SlipMark());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Leaf(Widget c, float scale)
	{
		Rect(c, scale, W / 2 - 7, 22, 14, 14, CGQC_GU_Style.SlipMark());
	}

	//------------------------------------------------------------------------------------------------
	protected static void Crown(Widget c, float scale, float y)
	{
		Rect(c, scale, W / 2 - 15, y, 30, 24, CGQC_GU_Style.SlipMark());
	}

	//------------------------------------------------------------------------------------------------
	protected static void Stars(Widget c, float scale, int count, float offset)
	{
		for (int i = 0; i < count; i++)
		{
			Rect(c, scale, W / 2 - 8, 20 + offset + i * 26, 16, 16, CGQC_GU_Style.SlipMark());
		}
	}
}
