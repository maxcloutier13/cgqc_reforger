// Gestion d'unité - Calendrier (Phase 2e, spec §5), after the approved mockup: month header with < > and
// AUJOURD'HUI, a 6-week grid with coloured marks, the selected day's details on the right, then « À VENIR ».
// Everyone may open it (owner decision 3). Calendar editors (calendar.edit) add and remove manual events.

class CGQC_GU_CalendarAction
{
	protected CGQC_GU_CalendarScreen m_Screen;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_CalendarAction(CGQC_GU_CalendarScreen screen, string kind, string value)
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

//! One day of the grid: a button-layout widget whose whole area selects the day.
class CGQC_GU_CalCell : ScriptedWidgetEventHandler
{
	protected Widget m_wRoot;
	protected ref CGQC_GU_CalendarAction m_Action;

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_CalCell Create(Widget parent, CGQC_GU_CalendarAction action, Color fill, bool outlined)
	{
		Widget root = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_BUTTON, parent);
		if (!root)
			return null;

		CGQC_GU_CalCell c = new CGQC_GU_CalCell();
		c.m_wRoot = root;
		c.m_Action = action;

		Widget bg = root.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(fill);

		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string edge : edges)
		{
			Widget e = root.FindAnyWidget(edge);
			if (!e)
				continue;

			e.SetVisible(outlined);
			e.SetColor(CGQC_GU_Style.BlueLight());
		}

		TextWidget label = TextWidget.Cast(root.FindAnyWidget("Label"));
		if (label)
			label.SetVisible(false);

		root.AddHandler(c);
		return c;
	}

	//------------------------------------------------------------------------------------------------
	Widget GetRootWidget()
	{
		return m_wRoot;
	}

	//------------------------------------------------------------------------------------------------
	//! Where the day number and marks go (top-left of the cell).
	Widget GetBody()
	{
		if (!m_wRoot)
			return null;

		return m_wRoot.FindAnyWidget("Body");
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Action)
			GetGame().GetCallqueue().CallLater(m_Action.Fire, 0, false);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_wRoot)
		{
			m_wRoot.RemoveHandler(this);
			m_wRoot = null;
		}
	}
}

class CGQC_GU_CalendarScreen : CGQC_GU_DarkScreen
{
	static const int MARKS_PER_DAY = 2;
	static const int MARK_CHARS = 18;
	static const float ASIDE_WIDTH = 400;

	protected ref CGQC_GU_CalendarDto m_Dto;
	protected ref CGQC_GU_Region m_Body = new CGQC_GU_Region();
	protected ref array<ref CGQC_GU_CalCell> m_aCells = new array<ref CGQC_GU_CalCell>();
	protected string m_sSelected;		// AAAA-MM-JJ
	protected bool m_bForm;				// « AJOUTER UN ÉVÉNEMENT » form open
	protected EditBoxWidget m_wTitle;
	protected CGQC_GU_DateField m_Date;
	protected EditBoxWidget m_wTime;
	protected MultilineEditBoxWidget m_wNote;
	protected CGQC_GU_Check m_Yearly;
	protected int m_iReadReq;
	protected int m_iWriteReq;
	protected string m_sMessage;
	protected TextWidget m_wClock;

	//------------------------------------------------------------------------------------------------
	override protected int GetNavId()
	{
		return NAV_CALENDAR;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildDark()
	{
		m_Body.m_wRoot = m_wContent;
		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		GetGame().GetCallqueue().CallLater(UpdateClock, 5000, true);
		Read(string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		GetGame().GetCallqueue().Remove(UpdateClock);
		m_wClock = null;
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		ClearAll();
	}

	//------------------------------------------------------------------------------------------------
	protected void Read(string month)
	{
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		p.Set("month", month);
		m_iReadReq = CGQC_GU_Client.RequestWith(CGQC_GU_Requests.CALENDAR, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot || (requestId != m_iReadReq && requestId != m_iWriteReq))
			return;

		if (!ok)
		{
			m_sMessage = CGQC_GU_DossierScreen.ErrorText(code);
			if (m_Dto)
				BuildAll();

			return;
		}

		JsonLoadContext ctx = new JsonLoadContext();
		CGQC_GU_CalendarDto dto = new CGQC_GU_CalendarDto();
		if (!ctx.LoadFromString(payload) || !ctx.ReadValue("c", dto))
			return;

		if (requestId == m_iWriteReq)
		{
			m_bForm = false;
			m_sMessage = string.Empty;
		}

		m_Dto = dto;
		if (m_sSelected.IsEmpty() || !m_sSelected.StartsWith(dto.month))
		{
			if (dto.today.StartsWith(dto.month))
				m_sSelected = dto.today;
			else
				m_sSelected = dto.month + "-01";
		}

		BuildAll();
	}

	//------------------------------------------------------------------------------------------------
	void OnAction(string kind, string value)
	{
		if (!m_Dto)
			return;

		switch (kind)
		{
			case "month":
				m_bForm = false;
				m_sMessage = string.Empty;
				Read(value);
				break;

			case "day":
				m_sSelected = value;
				m_bForm = false;
				m_sMessage = string.Empty;
				BuildAll();
				break;

			case "form.open":
				m_bForm = true;
				m_sMessage = string.Empty;
				BuildAll();
				break;

			case "form.close":
				m_bForm = false;
				m_sMessage = string.Empty;
				BuildAll();
				break;

			case "add":
			{
				CGQC_GU_Payload p = new CGQC_GU_Payload();
				if (m_wTitle) p.Set("title", m_wTitle.GetText());
				if (m_Date) p.Set("date", m_Date.GetValue());
				if (m_wTime) p.Set("time", m_wTime.GetText());
				if (m_wNote) p.Set("note", m_wNote.GetText());
				if (m_Yearly && m_Yearly.IsChecked())
					p.Set("yearly", "1");

				m_iWriteReq = CGQC_GU_Client.RequestWith(CGQC_GU_Requests.CALENDAR_ADD, p);
				break;
			}

			case "remove":
			{
				CGQC_GU_Payload r = new CGQC_GU_Payload();
				r.Set("id", value);
				r.Set("month", m_Dto.month);
				m_iWriteReq = CGQC_GU_Client.RequestWith(CGQC_GU_Requests.CALENDAR_REMOVE, r);
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Drawing
	//------------------------------------------------------------------------------------------------
	protected void ClearAll()
	{
		foreach (CGQC_GU_CalCell c : m_aCells)
		{
			c.Destroy();
		}

		m_aCells.Clear();
		m_Body.Reset();
		m_wTitle = null;
		m_Date = null;
		m_wTime = null;
		m_wNote = null;
		m_Yearly = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildAll()
	{
		if (!m_wRoot || !m_Dto || !m_wContent)
			return;

		ClearAll();
		if (!m_Dto.upcoming.IsEmpty())
			SetNavNext(UpcomingLine(m_Dto.upcoming[0], true));
		else
			SetNavNext(string.Empty);

		BuildHeader();

		Widget row = CGQC_GU_W.HBox(m_wContent);
		LayoutSlot.SetPadding(row, 0, 14, 0, 0);
		LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(row, 1);

		BuildGrid(row);
		BuildAside(row);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildHeader()
	{
		Widget head = CGQC_GU_W.DarkPanel(m_wContent, CGQC_GU_Style.DarkCard(), 28, 20);
		Widget row = CGQC_GU_W.HBox(head);

		Widget left = CGQC_GU_W.VBox(row);
		LayoutSlot.SetSizeMode(left, LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(left, 1);
		LayoutSlot.SetVerticalAlign(left, LayoutVerticalAlign.Center);
		Caption(left, "CALENDRIER");
		CGQC_GU_W.Text(left, m_Dto.monthLabel, CGQC_GU_Style.FONT_TITLE, 40, CGQC_GU_Style.DarkText());
		if (!m_sMessage.IsEmpty())
			CGQC_GU_W.Text(left, m_sMessage, CGQC_GU_Style.FONT_LABEL, 18, Color.FromSRGBA(0xE0, 0x7A, 0x6E, 255));

		int y, m;
		CGQC_GU_CalendarBuilder.ParseMonth(m_Dto.month, y, m);
		HeaderButton(row, "<", "month", ShiftMonth(y, m, -1), 56);
		HeaderButton(row, ">", "month", ShiftMonth(y, m, 1), 56);
		HeaderButton(row, "AUJOURD'HUI", "month", m_Dto.today.Substring(0, 7), 0);

		// Owner (2e): the current date and time (HE), live, instead of naming the time zone.
		Widget info = CGQC_GU_W.Sized(row, 260);
		LayoutSlot.SetPadding(info.GetParent(), 16, 0, 0, 0);
		LayoutSlot.SetVerticalAlign(info.GetParent(), LayoutVerticalAlign.Center);
		m_wClock = CGQC_GU_W.Text(info, string.Empty, CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkSoft());
		UpdateClock();
	}

	//------------------------------------------------------------------------------------------------
	//! « Aujourd'hui JEU 26 NOV · 22:10 », Eastern time, from this machine's UTC clock.
	protected void UpdateClock()
	{
		if (!m_wClock)
			return;

		string date, hhmm;
		CGQC_GU_Time.ToEastern(CGQC_GU_Time.NowUtc(), date, hhmm);
		int y = date.Substring(0, 4).ToInt();
		int m = date.Substring(5, 2).ToInt();
		int d = date.Substring(8, 2).ToInt();
		m_wClock.SetText(string.Format("Aujourd'hui %1 %2 %3 · %4", CGQC_GU_CalendarBuilder.DayName(CGQC_GU_Time.DayOfWeek(y, m, d)), d, MonthShort(m), hhmm));
	}

	//------------------------------------------------------------------------------------------------
	protected void HeaderButton(Widget row, string text, string kind, string value, float width)
	{
		Widget parent = row;
		if (width > 0)
		{
			parent = CGQC_GU_W.Sized(row, width);
			LayoutSlot.SetPadding(parent.GetParent(), 10, 0, 0, 0);
			LayoutSlot.SetVerticalAlign(parent.GetParent(), LayoutVerticalAlign.Center);
		}

		CGQC_GU_Button b = Btn(parent, text, kind, value, false);
		if (b && width <= 0)
		{
			LayoutSlot.SetPadding(b.GetRootWidget(), 10, 0, 0, 0);
			LayoutSlot.SetVerticalAlign(b.GetRootWidget(), LayoutVerticalAlign.Center);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button Btn(Widget parent, string text, string kind, string value, bool confirm)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text, CGQC_GU_EButtonKind.PLAIN, confirm);
		if (!b)
			return null;

		m_Body.m_aButtons.Insert(b);
		b.SetDark();
		if (confirm)
			b.SetConfirmText("CONFIRMER");

		if (!kind.IsEmpty())
		{
			CGQC_GU_CalendarAction a = new CGQC_GU_CalendarAction(this, kind, value);
			m_Body.m_aKeep.Insert(a);
			b.GetOnActivated().Insert(a.Fire);
		}

		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ShiftMonth(int y, int m, int delta)
	{
		m += delta;
		if (m < 1)
		{
			m = 12;
			y--;
		}
		else if (m > 12)
		{
			m = 1;
			y++;
		}

		return CGQC_GU_CalendarBuilder.MonthKey(y, m);
	}

	//------------------------------------------------------------------------------------------------
	protected static string MonthShort(int m)
	{
		array<string> names = { "JANV", "FÉVR", "MARS", "AVR", "MAI", "JUIN", "JUIL", "AOÛT", "SEPT", "OCT", "NOV", "DÉC" };
		return names[(m - 1) % 12];
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildGrid(Widget row)
	{
		Widget col = CGQC_GU_W.VBox(row);
		LayoutSlot.SetSizeMode(col, LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(col, 1);
		LayoutSlot.SetVerticalAlign(col, LayoutVerticalAlign.Stretch);
		LayoutSlot.SetPadding(col, 0, 0, 14, 0);

		// Weekday names
		Widget names = CGQC_GU_W.HBox(col);
		LayoutSlot.SetPadding(names, 0, 0, 0, 6);
		for (int d = 0; d < 7; d++)
		{
			Widget cellName = CGQC_GU_W.VBox(names);
			LayoutSlot.SetSizeMode(cellName, LayoutSizeMode.Fill);
			LayoutSlot.SetFillWeight(cellName, 1);
			TextWidget t = CGQC_GU_W.Text(cellName, CGQC_GU_CalendarBuilder.DayName(d), CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.DarkMuted());
			if (t)
				LayoutSlot.SetPadding(t, 10, 0, 0, 0);
		}

		int y, m;
		CGQC_GU_CalendarBuilder.ParseMonth(m_Dto.month, y, m);
		int index;
		for (int week = 0; week < 6; week++)
		{
			Widget wr = CGQC_GU_W.HBox(col);
			LayoutSlot.SetSizeMode(wr, LayoutSizeMode.Fill);
			LayoutSlot.SetFillWeight(wr, 1);
			LayoutSlot.SetPadding(wr, 0, 0, 0, 6);
			for (int dow = 0; dow < 7; dow++)
			{
				int day = index - m_Dto.firstDow + 1;
				index++;
				DayCell(wr, y, m, day, dow < 6);
			}
		}

		// Legend
		Widget legend = CGQC_GU_W.HBox(col);
		LayoutSlot.SetPadding(legend, 4, 4, 0, 0);
		LegendItem(legend, MarkColor("event"), "Événement");
		LegendItem(legend, MarkColor("enlist"), "Anniversaire");
		LegendItem(legend, CGQC_GU_Style.BlueLight(), "Opération");
		LegendItem(legend, CGQC_GU_Style.Training(), "Entraînement / FTX");
	}

	//------------------------------------------------------------------------------------------------
	protected void LegendItem(Widget row, Color color, string text)
	{
		ImageWidget dot = CGQC_GU_W.Dot(row, 16, color);
		if (dot)
		{
			LayoutSlot.SetVerticalAlign(dot, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(dot, 0, 0, 8, 0);
		}

		TextWidget t = CGQC_GU_W.Text(row, text, CGQC_GU_Style.FONT_LABEL, 15, CGQC_GU_Style.DarkSoft());
		if (t)
			LayoutSlot.SetPadding(t, 0, 0, 24, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void DayCell(Widget row, int y, int m, int day, bool gap)
	{
		bool inMonth = day >= 1 && day <= m_Dto.days;
		string date;
		if (inMonth)
			date = string.Format("%1-%2-%3", y, CGQC_GU_Time.Pad2(m), CGQC_GU_Time.Pad2(day));

		bool today = inMonth && date == m_Dto.today;
		bool picked = inMonth && date == m_sSelected;

		Color fill = CGQC_GU_Style.DarkTile();
		if (today)
			fill = CGQC_GU_Style.DarkActiveCard();
		else if (!inMonth)
			fill = CGQC_GU_Style.Translucent(0x28, 0x2B, 0x2C, 0.3);

		CGQC_GU_CalendarAction action;
		if (inMonth)
		{
			action = new CGQC_GU_CalendarAction(this, "day", date);
			m_Body.m_aKeep.Insert(action);
		}

		CGQC_GU_CalCell cell = CGQC_GU_CalCell.Create(row, action, fill, picked);
		if (!cell)
			return;

		m_aCells.Insert(cell);
		Widget root = cell.GetRootWidget();
		LayoutSlot.SetSizeMode(root, LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(root, 1);
		LayoutSlot.SetVerticalAlign(root, LayoutVerticalAlign.Stretch);
		if (gap)
			LayoutSlot.SetPadding(root, 0, 0, 6, 0);

		if (!inMonth)
			return;

		Widget body = cell.GetBody();
		if (!body)
			return;

		Widget stack = CGQC_GU_W.VBox(body);
		AlignableSlot.SetHorizontalAlign(stack, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetVerticalAlign(stack, LayoutVerticalAlign.Top);
		AlignableSlot.SetPadding(stack, 10, 8, 8, 6);

		Color numColor = CGQC_GU_Style.DarkText();
		ResourceName numFont = CGQC_GU_Style.FONT_LABEL;
		if (today)
		{
			numColor = CGQC_GU_Style.BlueLighter();
			numFont = CGQC_GU_Style.FONT_TITLE;
		}

		CGQC_GU_W.Text(stack, day.ToString(), numFont, 18, numColor);

		int shown;
		int more;
		foreach (CGQC_GU_CalItem it : m_Dto.items)
		{
			if (it.date != date)
				continue;

			if (shown >= MARKS_PER_DAY)
			{
				more++;
				continue;
			}

			Mark(stack, it);
			shown++;
		}

		if (more > 0)
			CGQC_GU_W.Text(stack, string.Format("+%1", more), CGQC_GU_Style.FONT_LABEL, 13, CGQC_GU_Style.DarkSoft());
	}

	//------------------------------------------------------------------------------------------------
	protected void Mark(Widget stack, CGQC_GU_CalItem it)
	{
		Widget chip = CGQC_GU_W.DarkPanel(stack, MarkColor(it.kind), 6, 3);
		if (!chip)
			return;

		LayoutSlot.SetPadding(chip.GetParent(), 0, 4, 0, 0);
		string title = it.title;
		if (title.Length() > MARK_CHARS)
			title = title.Substring(0, MARK_CHARS - 1) + ".";

		CGQC_GU_W.Text(chip, title, CGQC_GU_Style.FONT_LABEL, 13, CGQC_GU_Style.White());
	}

	//------------------------------------------------------------------------------------------------
	protected static Color MarkColor(string kind)
	{
		switch (kind)
		{
			case "event":		return Color.FromSRGBA(0x8E, 0x6C, 0x2A, 255);	// amber
			case "enlist":		return Color.FromSRGBA(0x4A, 0x55, 0x6A, 255);	// slate
			case "birthday":	return Color.FromSRGBA(0x4A, 0x55, 0x6A, 255);
			case "operation":	return CGQC_GU_Style.BlueLight();
			case "training":	return CGQC_GU_Style.Training();
		}

		return CGQC_GU_Style.DarkButton();
	}

	//------------------------------------------------------------------------------------------------
	protected static string KindLabel(string kind)
	{
		switch (kind)
		{
			case "event":		return "ÉVÉNEMENT";
			case "enlist":		return "ANNIVERSAIRE D'ENRÔLEMENT";
			case "birthday":	return "ANNIVERSAIRE";
			case "operation":	return "OPÉRATION";
			case "training":	return "ENTRAÎNEMENT";
		}

		return CGQC_GU_Str.Upper(kind);
	}

	//------------------------------------------------------------------------------------------------
	//! "MER 12-02 · Q-MB (séance 2)" (withTime: with the time HE when set).
	protected string UpcomingLine(CGQC_GU_CalItem it, bool withTime)
	{
		string text = DayLabelShort(it.date);
		if (withTime && !it.time.IsEmpty())
			text += " · " + CGQC_GU_SituationBuilder.FrenchTime(it.time);

		return text + " · " + it.title;
	}

	//------------------------------------------------------------------------------------------------
	//! "MER 12-02"
	protected static string DayLabelShort(string date)
	{
		int y = date.Substring(0, 4).ToInt();
		int m = date.Substring(5, 2).ToInt();
		int d = date.Substring(8, 2).ToInt();
		return CGQC_GU_CalendarBuilder.DayName(CGQC_GU_Time.DayOfWeek(y, m, d)) + " " + CGQC_GU_OverviewBuilder.ShortDate(date);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildAside(Widget row)
	{
		Widget box = CGQC_GU_W.Sized(row, ASIDE_WIDTH);
		if (!box)
			return;

		LayoutSlot.SetVerticalAlign(box.GetParent(), LayoutVerticalAlign.Stretch);
		Widget aside = CGQC_GU_W.DarkPanel(box, CGQC_GU_Style.DarkCard(), 24, 22);
		LayoutSlot.SetSizeMode(aside.GetParent(), LayoutSizeMode.Fill);
		LayoutSlot.SetFillWeight(aside.GetParent(), 1);

		if (m_bForm)
		{
			BuildForm(aside);
			return;
		}

		Caption(aside, "JOUR CHOISI");
		int y = m_sSelected.Substring(0, 4).ToInt();
		int m = m_sSelected.Substring(5, 2).ToInt();
		int d = m_sSelected.Substring(8, 2).ToInt();
		string dayText = string.Format("%1 %2 %3", CGQC_GU_CalendarBuilder.DayName(CGQC_GU_Time.DayOfWeek(y, m, d)), d, MonthShort(m));
		TextWidget dt = CGQC_GU_W.Text(aside, dayText, CGQC_GU_Style.FONT_TITLE, 28, CGQC_GU_Style.DarkText());
		if (dt)
			LayoutSlot.SetPadding(dt, 0, 4, 0, 10);

		int count;
		foreach (CGQC_GU_CalItem it : m_Dto.items)
		{
			if (it.date != m_sSelected)
				continue;

			ItemCard(aside, it);
			count++;
		}

		if (count == 0)
			CGQC_GU_W.Text(aside, "Rien de prévu.", CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkSoft());

		if (m_Dto.canEdit)
		{
			CGQC_GU_Button add = Btn(aside, "AJOUTER UN ÉVÉNEMENT", "form.open", string.Empty, false);
			if (add)
				LayoutSlot.SetPadding(add.GetRootWidget(), 0, 12, 0, 0);
		}

		Line(aside, 16, 10);
		Caption(aside, "À VENIR");
		if (m_Dto.upcoming.IsEmpty())
		{
			CGQC_GU_W.Text(aside, "Rien de prévu.", CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkSoft());
			return;
		}

		foreach (CGQC_GU_CalItem up : m_Dto.upcoming)
		{
			Widget line = CGQC_GU_W.HBox(aside);
			LayoutSlot.SetPadding(line, 0, 8, 0, 0);
			Widget dateBox = CGQC_GU_W.Sized(line, 96);
			CGQC_GU_W.Text(dateBox, DayLabelShort(up.date), CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkMuted());
			Widget titleBox = CGQC_GU_W.Sized(line, ASIDE_WIDTH - 96 - 48);
			CGQC_GU_W.Text(titleBox, up.title, CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkText(), true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ItemCard(Widget aside, CGQC_GU_CalItem it)
	{
		Widget holder = CGQC_GU_W.VBox(aside);
		LayoutSlot.SetPadding(holder, 0, 0, 0, 10);

		// Coloured top edge, as in the mockup.
		Widget strip = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, holder);
		if (strip)
		{
			strip.SetColor(MarkColor(it.kind));
			ImageWidget.Cast(strip).SetSize(8, 4);
			LayoutSlot.SetHorizontalAlign(strip, LayoutHorizontalAlign.Stretch);
		}

		Widget card = CGQC_GU_W.DarkPanel(holder, CGQC_GU_Style.DarkTile(), 16, 14);
		string cap = KindLabel(it.kind);
		if (it.yearly)
			cap += " · CHAQUE ANNÉE";

		CGQC_GU_W.Text(card, cap, CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.BlueLighter());
		CGQC_GU_W.Text(card, it.title, CGQC_GU_Style.FONT_TITLE, 22, CGQC_GU_Style.DarkText(), true);
		if (!it.time.IsEmpty())
			CGQC_GU_W.Text(card, CGQC_GU_SituationBuilder.FrenchTime(it.time) + " (HE)", CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkSoft());

		if (!it.note.IsEmpty())
			CGQC_GU_W.Text(card, it.note, CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.DarkSoft(), true);

		if (m_Dto.canEdit && !it.id.IsEmpty())
		{
			CGQC_GU_Button rm = Btn(card, "RETIRER", "remove", it.id, true);
			if (rm)
			{
				rm.SetCompact();
				LayoutSlot.SetPadding(rm.GetRootWidget(), 0, 8, 0, 0);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! « AJOUTER UN ÉVÉNEMENT »: title, date (calendar), time HE (optional), note, every year.
	protected void BuildForm(Widget aside)
	{
		Caption(aside, "AJOUTER UN ÉVÉNEMENT");
		float width = ASIDE_WIDTH - 48;

		FormLabel(aside, "TITRE");
		m_wTitle = CGQC_GU_W.Field(aside, string.Empty, width);

		FormLabel(aside, "DATE");
		m_Date = CGQC_GU_DateField.Create(m_Body, aside, m_sSelected, false, true);

		FormLabel(aside, "HEURE HE (HH:MM, FACULTATIVE)");
		m_wTime = CGQC_GU_W.Field(aside, string.Empty, 100);

		FormLabel(aside, "NOTE");
		m_wNote = CGQC_GU_W.FieldMulti(aside, string.Empty, width, 110);

		m_Yearly = CGQC_GU_Check.Create(aside, "yearly", "Chaque année", false);
		if (m_Yearly)
		{
			m_Body.m_aChecks.Insert(m_Yearly);
			m_Yearly.SetDark();
			LayoutSlot.SetPadding(m_Yearly.GetRootWidget(), 0, 12, 0, 0);
		}

		CGQC_GU_Button save = Btn(aside, "ENREGISTRER", "add", string.Empty, true);
		if (save)
			LayoutSlot.SetPadding(save.GetRootWidget(), 0, 16, 0, 0);

		CGQC_GU_Button cancel = Btn(aside, "ANNULER", "form.close", string.Empty, false);
		if (cancel)
			LayoutSlot.SetPadding(cancel.GetRootWidget(), 0, 8, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void FormLabel(Widget parent, string text)
	{
		TextWidget t = CGQC_GU_W.Text(parent, text, CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.DarkMuted());
		if (t)
			LayoutSlot.SetPadding(t, 0, 12, 0, 4);
	}
}
