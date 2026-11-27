// Gestion d'unité - shared helpers: logging, UTC timestamps, stable IDs, small string utils.

class CGQC_GU_Log
{
	static void Info(string msg)
	{
		Print("[CGQC_GU] " + msg, LogLevel.NORMAL);
	}

	static void Warn(string msg)
	{
		Print("[CGQC_GU] " + msg, LogLevel.WARNING);
	}

	static void Error(string msg)
	{
		Print("[CGQC_GU] " + msg, LogLevel.ERROR);
	}
}

class CGQC_GU_Time
{
	//------------------------------------------------------------------------------------------------
	static string Pad2(int v)
	{
		if (v < 10)
			return "0" + v.ToString();

		return v.ToString();
	}

	//------------------------------------------------------------------------------------------------
	//! ISO 8601 date, UTC: 2026-11-28
	static string TodayUtc()
	{
		int y, m, d;
		System.GetYearMonthDayUTC(y, m, d);
		return string.Format("%1-%2-%3", y, Pad2(m), Pad2(d));
	}

	//------------------------------------------------------------------------------------------------
	//! Whole months from an ISO date ("2022", "2022-04" or "2022-04-04[T…]") to today (UTC). 0 if unknown.
	static int MonthsSince(string iso)
	{
		if (iso.Length() < 4)
			return 0;

		int fromYear = iso.Substring(0, 4).ToInt();
		int fromMonth = 1;
		int fromDay = 1;
		if (iso.Length() >= 7)
			fromMonth = iso.Substring(5, 2).ToInt();
		if (iso.Length() >= 10)
			fromDay = iso.Substring(8, 2).ToInt();

		int y, m, d;
		System.GetYearMonthDayUTC(y, m, d);
		int months = (y - fromYear) * 12 + (m - fromMonth);
		if (d < fromDay)
			months--;

		if (months < 0)
			return 0;

		return months;
	}

	//------------------------------------------------------------------------------------------------
	//! ISO 8601 timestamp, UTC: 2026-11-28T01:00:00Z
	static string NowUtc()
	{
		int y, mo, d, h, mi, s;
		System.GetYearMonthDayUTC(y, mo, d);
		System.GetHourMinuteSecondUTC(h, mi, s);
		return string.Format("%1-%2-%3T%4:%5:%6Z", y, Pad2(mo), Pad2(d), Pad2(h), Pad2(mi), Pad2(s));
	}
}

class CGQC_GU_Ids
{
	protected static const string HEX = "0123456789abcdef";
	protected static ref RandomGenerator s_Rng;

	//------------------------------------------------------------------------------------------------
	//! Stable, never-reused ID: <prefix>_<yyyymmdd>_<8 random hex>, e.g. rec_20261126_3fa91c0d
	static string New(string prefix)
	{
		if (!s_Rng)
		{
			// Seeded from the wall clock so a restart never replays the same sequence.
			s_Rng = new RandomGenerator();
			s_Rng.SetSeed(System.GetUnixTime());
		}

		int y, m, d;
		System.GetYearMonthDayUTC(y, m, d);

		string rnd;
		for (int i = 0; i < 8; i++)
		{
			int n = s_Rng.RandInt(0, 16);
			rnd += HEX.Get(n);
		}

		return string.Format("%1_%2%3%4_%5", prefix, y, CGQC_GU_Time.Pad2(m), CGQC_GU_Time.Pad2(d), rnd);
	}
}

class CGQC_GU_Str
{
	//------------------------------------------------------------------------------------------------
	static string Lower(string s)
	{
		string c = s;
		c.ToLower();
		return c;
	}

	//------------------------------------------------------------------------------------------------
	static string Upper(string s)
	{
		string c = s;
		c.ToUpper();
		return c;
	}

	//------------------------------------------------------------------------------------------------
	static string Join(array<string> items, string sep)
	{
		string res;
		if (!items)
			return res;

		for (int i = 0; i < items.Count(); i++)
		{
			if (i > 0)
				res += sep;

			res += items[i];
		}

		return res;
	}

	//------------------------------------------------------------------------------------------------
	//! Keep letters, digits, '-' and '_' only, so the value is safe as a file name.
	static string FileSafe(string s)
	{
		const string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
		string res;
		for (int i = 0; i < s.Length(); i++)
		{
			string ch = s.Get(i);
			if (allowed.Contains(ch))
				res += ch;
			else
				res += "_";
		}

		return res;
	}
}
