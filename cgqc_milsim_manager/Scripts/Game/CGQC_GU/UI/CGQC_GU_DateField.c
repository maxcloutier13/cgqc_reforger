// Gestion d'unité - the date field of every form (owner, 3f test): a button showing the date, which opens a
// calendar over the screen. Nothing is typed; screens read GetValue() when they send their form.
// Use it for every date input, later screens included (opérations, déploiements).

//! Binds a button of the calendar to a named action of its field.
class CGQC_GU_DateAction : Managed
{
	protected CGQC_GU_DateField m_Field;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_DateAction(CGQC_GU_DateField field, string kind, string value)
	{
		m_Field = field;
		m_sKind = kind;
		m_sValue = value;
	}

	//------------------------------------------------------------------------------------------------
	void Fire()
	{
		if (m_Field)
			m_Field.Queue(m_sKind, m_sValue);
	}
}

class CGQC_GU_DateField : Managed
{
	static const float DAY_WIDTH = 56;
	static const int YEARS_PER_PAGE = 16;
	static const int POPUP_Z = 100;

	protected ref CGQC_GU_Button m_Button;
	protected string m_sValue;		// AAAA-MM-JJ, "" = none; an older partial date is kept until another is picked
	protected bool m_bOptional;		// EFFACER is offered
	protected bool m_bDark;			// dark screens
	protected bool m_bYearsFirst;	// birth dates start with the year

	// Open calendar
	protected Widget m_wVeil;
	protected Widget m_wSheet;
	protected Widget m_wBody;
	protected ref array<ref CGQC_GU_Button> m_aButtons = new array<ref CGQC_GU_Button>();
	protected ref array<ref Managed> m_aKeep = new array<ref Managed>();
	protected int m_iYear;
	protected int m_iMonth;
	protected int m_iFirstYear;		// first year of the year page
	protected bool m_bYears;
	protected string m_sKind;		// pressed, applied on the next frame
	protected string m_sPicked;

	//------------------------------------------------------------------------------------------------
	//! Creates the field's button inside parent (a layout). The region that owns the parent keeps the field:
	//! without a strong reference the object is deleted at once, and its button does nothing.
	static CGQC_GU_DateField Create(CGQC_GU_Region region, Widget parent, string value, bool optional = false, bool dark = false, bool yearsFirst = false)
	{
		CGQC_GU_DateField f = new CGQC_GU_DateField();
		f.m_sValue = value;
		f.m_bOptional = optional;
		f.m_bDark = dark;
		f.m_bYearsFirst = yearsFirst;
		f.m_Button = f.NewButton(parent, f.Label());
		if (!f.m_Button)
			return null;

		f.m_Button.GetOnActivated().Insert(f.Open);
		region.m_aDates.Insert(f);
		return f;
	}

	//------------------------------------------------------------------------------------------------
	string GetValue()
	{
		return m_sValue;
	}

	//------------------------------------------------------------------------------------------------
	protected string Label()
	{
		if (m_sValue.IsEmpty())
			return "CHOISIR UNE DATE";

		return m_sValue;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button NewButton(Widget parent, string text)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text);
		if (!b)
			return null;

		if (m_bDark)
			b.SetDark();

		b.SetCompact();
		return b;
	}

	//------------------------------------------------------------------------------------------------
	//! The calendar opens over the screen, on the month of the date shown, else on today's.
	void Open()
	{
		Close();
		Widget root = CGQC_GU_Ui.CurrentRoot();
		if (!root)
			return;

		string today = CGQC_GU_Time.TodayEastern();
		string from = m_sValue;
		if (from.Length() < 4)
			from = today;

		m_iYear = from.Substring(0, 4).ToInt();
		m_iMonth = 1;
		if (from.Length() >= 7)
			m_iMonth = Math.ClampInt(from.Substring(5, 2).ToInt(), 1, 12);

		m_bYears = m_bYearsFirst && m_sValue.IsEmpty();
		if (m_bYears)
			m_iYear -= 30;

		m_iFirstYear = m_iYear - YEARS_PER_PAGE / 2;

		// A full-screen button under the sheet: nothing behind it takes a click.
		m_wVeil = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_BUTTON, root);
		if (!m_wVeil)
			return;

		FrameSlot.SetAnchorMin(m_wVeil, 0, 0);
		FrameSlot.SetAnchorMax(m_wVeil, 1, 1);
		FrameSlot.SetOffsets(m_wVeil, 0, 0, 0, 0);
		m_wVeil.SetZOrder(POPUP_Z);
		Widget veil = m_wVeil.FindAnyWidget("Bg");
		if (veil)
			veil.SetColor(CGQC_GU_Style.Translucent(0x0A, 0x0D, 0x12, 0.55));

		array<string> unused = { "EdgeT", "EdgeB", "EdgeL", "EdgeR", "Label" };
		foreach (string name : unused)
		{
			Widget w = m_wVeil.FindAnyWidget(name);
			if (w)
				w.SetVisible(false);
		}

		Widget inner = CGQC_GU_W.Panel(root);
		if (!inner)
			return;

		m_wSheet = inner.GetParent();
		FrameSlot.SetAnchorMin(m_wSheet, 0.5, 0.5);
		FrameSlot.SetAnchorMax(m_wSheet, 0.5, 0.5);
		FrameSlot.SetAlignment(m_wSheet, 0.5, 0.5);
		FrameSlot.SetPos(m_wSheet, 0, 0);
		FrameSlot.SetSizeToContent(m_wSheet, true);
		m_wSheet.SetZOrder(POPUP_Z + 1);

		Widget bg = m_wSheet.FindAnyWidget("Bg");
		if (bg)
		{
			if (m_bDark)
				bg.SetColor(Color.FromSRGBA(0x1A, 0x1D, 0x21, 255));
			else
				bg.SetColor(CGQC_GU_Style.Parchment());
		}

		AlignableSlot.SetPadding(inner, 24, 20, 24, 20);
		m_wBody = inner;
		Draw();
	}

	//------------------------------------------------------------------------------------------------
	//! The press is applied on the next frame, because it redraws or removes the pressed button.
	void Queue(string kind, string value)
	{
		m_sKind = kind;
		m_sPicked = value;
		GetGame().GetCallqueue().Remove(Apply);
		GetGame().GetCallqueue().CallLater(Apply, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void Apply()
	{
		switch (m_sKind)
		{
			case "month":
				m_iMonth += m_sPicked.ToInt();
				if (m_iMonth < 1)
				{
					m_iMonth = 12;
					m_iYear--;
				}
				else if (m_iMonth > 12)
				{
					m_iMonth = 1;
					m_iYear++;
				}

				Draw();
				break;

			case "years.open":
				m_bYears = true;
				m_iFirstYear = m_iYear - YEARS_PER_PAGE / 2;
				Draw();
				break;

			case "years.page":
				m_iFirstYear += m_sPicked.ToInt() * YEARS_PER_PAGE;
				Draw();
				break;

			case "year":
				m_iYear = m_sPicked.ToInt();
				m_bYears = false;
				Draw();
				break;

			case "day":
				SetValue(m_sPicked);
				break;

			case "clear":
				SetValue(string.Empty);
				break;

			case "close":
				Close();
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetValue(string value)
	{
		m_sValue = value;
		if (m_Button)
			m_Button.SetText(Label());

		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button Btn(Widget row, float width, string text, string kind, string value = "")
	{
		Widget box = row;
		if (width > 0)
			box = CGQC_GU_W.Sized(row, width);

		CGQC_GU_Button b = NewButton(box, text);
		if (!b)
			return null;

		m_aButtons.Insert(b);
		if (width > 0)
			LayoutSlot.SetHorizontalAlign(b.GetRootWidget(), LayoutHorizontalAlign.Stretch);

		CGQC_GU_DateAction a = new CGQC_GU_DateAction(this, kind, value);
		m_aKeep.Insert(a);
		b.GetOnActivated().Insert(a.Fire);
		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected Color TitleColor()
	{
		if (m_bDark)
			return CGQC_GU_Style.DarkText();

		return CGQC_GU_Style.Ink();
	}

	//------------------------------------------------------------------------------------------------
	protected Color MutedColor()
	{
		if (m_bDark)
			return CGQC_GU_Style.DarkMuted();

		return CGQC_GU_Style.InkMuted();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearBody()
	{
		foreach (CGQC_GU_Button b : m_aButtons)
		{
			b.Destroy();
		}

		m_aButtons.Clear();
		m_aKeep.Clear();
		CGQC_GU_W.Clear(m_wBody);
	}

	//------------------------------------------------------------------------------------------------
	protected void Draw()
	{
		if (!m_wBody)
			return;

		ClearBody();
		CGQC_GU_W.Text(m_wBody, "CHOISIR UNE DATE", CGQC_GU_Style.FONT_TITLE, 14, MutedColor());

		if (m_bYears)
			Years();
		else
			Month();

		Widget foot = CGQC_GU_W.HBox(m_wBody);
		if (!foot)
			return;

		LayoutSlot.SetPadding(foot, 0, 12, 0, 0);
		if (m_bOptional && !m_sValue.IsEmpty())
		{
			CGQC_GU_Button clear = Btn(foot, 0, "EFFACER LA DATE", "clear");
			if (clear)
				LayoutSlot.SetPadding(clear.GetRootWidget(), 0, 0, 8, 0);
		}

		Btn(foot, 0, "ANNULER", "close");
	}

	//------------------------------------------------------------------------------------------------
	//! « < title > » over a grid; the title is a button when it opens the year page.
	protected void Head(string title, string step, bool opensYears)
	{
		Widget head = CGQC_GU_W.HBox(m_wBody);
		if (!head)
			return;

		LayoutSlot.SetPadding(head, 0, 10, 0, 8);
		Btn(head, DAY_WIDTH, "<", step, "-1");
		if (opensYears)
		{
			Btn(head, 5 * DAY_WIDTH, title, "years.open");
		}
		else
		{
			Widget middle = CGQC_GU_W.Sized(head, 5 * DAY_WIDTH);
			if (middle)
			{
				LayoutSlot.SetVerticalAlign(middle.GetParent(), LayoutVerticalAlign.Center);
				TextWidget t = CGQC_GU_W.Text(middle, title, CGQC_GU_Style.FONT_TITLE, 18, TitleColor());
				if (t)
					LayoutSlot.SetHorizontalAlign(t, LayoutHorizontalAlign.Center);
			}
		}

		Btn(head, DAY_WIDTH, ">", step, "1");
	}

	//------------------------------------------------------------------------------------------------
	protected void Years()
	{
		Head(string.Format("%1 à %2", m_iFirstYear, m_iFirstYear + YEARS_PER_PAGE - 1), "years.page", false);

		Widget row;
		for (int i = 0; i < YEARS_PER_PAGE; i++)
		{
			if (i % 4 == 0)
			{
				row = CGQC_GU_W.HBox(m_wBody);
				if (row)
					LayoutSlot.SetPadding(row, 0, 0, 0, 6);
			}

			if (!row)
				break;

			int year = m_iFirstYear + i;
			CGQC_GU_Button b = Btn(row, 7 * DAY_WIDTH / 4, year.ToString(), "year", year.ToString());
			if (b)
				b.SetSelected(year == m_iYear);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! One month, Sunday first, as on the Calendrier.
	protected void Month()
	{
		Head(CGQC_GU_CalendarBuilder.MonthName(m_iMonth) + " " + m_iYear.ToString(), "month", true);

		Widget names = CGQC_GU_W.HBox(m_wBody);
		for (int dow = 0; dow < 7; dow++)
		{
			Widget box = CGQC_GU_W.Sized(names, DAY_WIDTH);
			TextWidget name = CGQC_GU_W.Text(box, CGQC_GU_CalendarBuilder.DayName(dow), CGQC_GU_Style.FONT_TITLE, 13, MutedColor());
			if (name)
				LayoutSlot.SetHorizontalAlign(name, LayoutHorizontalAlign.Center);
		}

		string today = CGQC_GU_Time.TodayEastern();
		string key = CGQC_GU_CalendarBuilder.MonthKey(m_iYear, m_iMonth);
		int first = CGQC_GU_Time.DayOfWeek(m_iYear, m_iMonth, 1);
		int days = CGQC_GU_Time.DaysInMonth(m_iYear, m_iMonth);

		// Always six weeks, so the sheet keeps its size from one month to the next.
		Widget week;
		for (int i = 0; i < 42; i++)
		{
			if (i % 7 == 0)
			{
				week = CGQC_GU_W.HBox(m_wBody);
				if (week)
					LayoutSlot.SetPadding(week, 0, 4, 0, 0);
			}

			if (!week)
				break;

			int day = i - first + 1;
			if (day < 1 || day > days)
			{
				CGQC_GU_W.Sized(week, DAY_WIDTH, 28);
				continue;
			}

			string iso = key + "-" + CGQC_GU_Time.Pad2(day);
			CGQC_GU_Button b = Btn(week, DAY_WIDTH, day.ToString(), "day", iso);
			if (!b)
				continue;

			if (iso == today)
			{
				if (m_bDark)
					b.SetTextColor(CGQC_GU_Style.BlueLighter());
				else
					b.SetTextColor(CGQC_GU_Style.Blue());
			}

			b.SetSelected(iso == m_sValue);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(Apply);
		if (m_wBody)
			ClearBody();

		m_wBody = null;
		if (m_wSheet)
			m_wSheet.RemoveFromHierarchy();

		m_wSheet = null;
		if (m_wVeil)
			m_wVeil.RemoveFromHierarchy();

		m_wVeil = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Removes the calendar and the field's button, while their widgets are alive.
	void Destroy()
	{
		Close();
		if (m_Button)
			m_Button.Destroy();

		m_Button = null;
	}
}
