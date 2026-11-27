// Gestion d'unité - Calendrier (Phase 2d). Built on the server: manual events (state/calendar.json), plus
// anniversaries computed from the dossiers (enlistment and birthdays). Operations, trainings and deployments
// are added here once they exist (Phases 3 and 5). Dates and times are Eastern time (HE).

class CGQC_GU_CalItem : Managed
{
	string date;	// AAAA-MM-JJ
	string kind;	// event | enlist | birthday (later: operation | training | deployment)
	string title;
	string time;	// HH:MM HE, "" for all day
	string note;
	string id;		// manual event id; "" for computed items
	bool yearly;
}

class CGQC_GU_CalendarDto : Managed
{
	string month;		// AAAA-MM shown
	string monthLabel;	// "NOVEMBRE 2026"
	string today;		// AAAA-MM-JJ (HE)
	int firstDow;		// weekday of the 1st, 0 = dimanche
	int days;			// days in the month
	bool canEdit;		// display only; calendar.add / remove check calendar.edit again
	ref array<ref CGQC_GU_CalItem> items = new array<ref CGQC_GU_CalItem>();
	ref array<ref CGQC_GU_CalItem> upcoming = new array<ref CGQC_GU_CalItem>();
}

class CGQC_GU_CalendarBuilder
{
	static const int UPCOMING_COUNT = 6;
	static const int UPCOMING_MONTHS = 3;	// look-ahead for « À VENIR » and « PROCHAIN AU CALENDRIER »

	//------------------------------------------------------------------------------------------------
	static string MonthName(int m)
	{
		array<string> names = { "JANVIER", "FÉVRIER", "MARS", "AVRIL", "MAI", "JUIN", "JUILLET", "AOÛT", "SEPTEMBRE", "OCTOBRE", "NOVEMBRE", "DÉCEMBRE" };
		if (m < 1 || m > 12)
			return string.Empty;

		return names[m - 1];
	}

	//------------------------------------------------------------------------------------------------
	static string DayName(int dow)
	{
		array<string> names = { "DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM" };
		return names[dow % 7];
	}

	//------------------------------------------------------------------------------------------------
	//! "AAAA-MM" -> year and month; false if unreadable.
	static bool ParseMonth(string s, out int y, out int m)
	{
		if (s.Length() != 7 || s.Get(4) != "-")
			return false;

		y = s.Substring(0, 4).ToInt();
		m = s.Substring(5, 2).ToInt();
		return y >= 2000 && y <= 2100 && m >= 1 && m <= 12;
	}

	//------------------------------------------------------------------------------------------------
	static string MonthKey(int y, int m)
	{
		return string.Format("%1-%2", y, CGQC_GU_Time.Pad2(m));
	}

	//------------------------------------------------------------------------------------------------
	static string Build(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, string month)
	{
		string today = CGQC_GU_Time.TodayEastern();
		int y = today.Substring(0, 4).ToInt();
		int m = today.Substring(5, 2).ToInt();
		int py, pm;
		if (ParseMonth(month, py, pm))
		{
			y = py;
			m = pm;
		}

		CGQC_GU_CalendarDto dto = new CGQC_GU_CalendarDto();
		dto.month = MonthKey(y, m);
		dto.monthLabel = MonthName(m) + " " + y.ToString();
		dto.today = today;
		dto.firstDow = CGQC_GU_Time.DayOfWeek(y, m, 1);
		dto.days = CGQC_GU_Time.DaysInMonth(y, m);
		dto.canEdit = CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, viewer, CGQC_GU_Rights.CALENDAR_EDIT);

		CollectMonth(mgr, y, m, dto.items);
		Sort(dto.items);
		Upcoming(mgr, today, dto.upcoming, false);

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("c", dto);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	//! Everything that falls in year y, month m.
	static void CollectMonth(CGQC_GU_Manager mgr, int y, int m, notnull array<ref CGQC_GU_CalItem> items)
	{
		string prefix = MonthKey(y, m);
		int days = CGQC_GU_Time.DaysInMonth(y, m);

		foreach (CGQC_GU_CalendarEvent e : mgr.m_Calendar.events)
		{
			string date;
			if (e.yearly)
			{
				if (e.date.Length() < 10 || e.date.Substring(5, 2).ToInt() != m)
					continue;

				// A yearly event in the past still shows; one dated in a future year starts that year.
				if (e.date.Substring(0, 4).ToInt() > y)
					continue;

				date = DayIn(y, m, e.date.Substring(8, 2).ToInt(), days);
			}
			else
			{
				if (!e.date.StartsWith(prefix))
					continue;

				date = e.date;
			}

			CGQC_GU_CalItem it = new CGQC_GU_CalItem();
			it.date = date;
			it.kind = "event";
			it.title = e.title;
			it.time = e.time;
			it.note = e.note;
			it.id = e.id;
			it.yearly = e.yearly;
			items.Insert(it);
		}

		// Anniversaries, computed from the dossiers (owner decision 4). KIA members are left out.
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		foreach (string uid, CGQC_GU_Member mem : mgr.m_mMembers)
		{
			if (mem.GetEffectiveStatus() == "kia")
				continue;

			string name = CGQC_GU_OverviewBuilder.NameOf(cat, mem);
			string enlisted = mem.enlistedOn;
			if (enlisted.Length() >= 10 && enlisted.Substring(5, 2).ToInt() == m)
			{
				int years = y - enlisted.Substring(0, 4).ToInt();
				if (years > 0)
				{
					CGQC_GU_CalItem a = new CGQC_GU_CalItem();
					a.date = DayIn(y, m, enlisted.Substring(8, 2).ToInt(), days);
					a.kind = "enlist";
					if (years == 1)
						a.title = name + " · 1 an au CGQC";
					else
						a.title = string.Format("%1 · %2 ans au CGQC", name, years);

					items.Insert(a);
				}
			}

			string birth = mem.personal.birthDate;
			if (birth.Length() >= 10 && birth.Substring(5, 2).ToInt() == m)
			{
				// Day only, never the age.
				CGQC_GU_CalItem b = new CGQC_GU_CalItem();
				b.date = DayIn(y, m, birth.Substring(8, 2).ToInt(), days);
				b.kind = "birthday";
				b.title = "Anniversaire · " + name;
				items.Insert(b);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Day d of the month, clamped (29 février in a common year shows on the 28th).
	protected static string DayIn(int y, int m, int d, int days)
	{
		if (d < 1)
			d = 1;

		if (d > days)
			d = days;

		return string.Format("%1-%2-%3", y, CGQC_GU_Time.Pad2(m), CGQC_GU_Time.Pad2(d));
	}

	//------------------------------------------------------------------------------------------------
	//! By date, then all-day items first, then time.
	static void Sort(notnull array<ref CGQC_GU_CalItem> items)
	{
		for (int i = 1; i < items.Count(); i++)
		{
			int j = i;
			while (j > 0 && SortKey(items[j - 1]) > SortKey(items[j]))
			{
				items.SwapItems(j - 1, j);
				j--;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string SortKey(CGQC_GU_CalItem it)
	{
		return it.date + " " + it.time + " " + it.title;
	}

	//------------------------------------------------------------------------------------------------
	//! The next items from today on, over UPCOMING_MONTHS months. eventsOnly skips the anniversaries.
	static void Upcoming(CGQC_GU_Manager mgr, string today, notnull array<ref CGQC_GU_CalItem> outItems, bool eventsOnly, int count = UPCOMING_COUNT)
	{
		int y = today.Substring(0, 4).ToInt();
		int m = today.Substring(5, 2).ToInt();
		for (int i = 0; i < UPCOMING_MONTHS && outItems.Count() < count; i++)
		{
			array<ref CGQC_GU_CalItem> month = new array<ref CGQC_GU_CalItem>();
			CollectMonth(mgr, y, m, month);
			Sort(month);
			foreach (CGQC_GU_CalItem it : month)
			{
				if (it.date < today)
					continue;

				if (eventsOnly && (it.kind == "enlist" || it.kind == "birthday"))
					continue;

				outItems.Insert(it);
				if (outItems.Count() >= count)
					break;
			}

			m++;
			if (m > 12)
			{
				m = 1;
				y++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! « PROCHAIN AU CALENDRIER » line: "12-02 · 19 h 30 · Jour du Souvenir", or "" if nothing is coming.
	static string NextLine(CGQC_GU_Manager mgr)
	{
		array<ref CGQC_GU_CalItem> next = new array<ref CGQC_GU_CalItem>();
		Upcoming(mgr, CGQC_GU_Time.TodayEastern(), next, true, 1);
		if (next.IsEmpty())
			return string.Empty;

		CGQC_GU_CalItem it = next[0];
		string text = CGQC_GU_OverviewBuilder.ShortDate(it.date);
		if (!it.time.IsEmpty())
			text += " · " + CGQC_GU_SituationBuilder.FrenchTime(it.time);

		return text + " · " + it.title;
	}
}
