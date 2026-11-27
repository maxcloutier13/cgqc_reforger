// Gestion d'unité - pause menu overview: one request, one reply with everything the panel shows.
// Built on the server; display text is composed here from codes + catalogue labels and is never stored.

class CGQC_GU_NewsLine : Managed
{
	string date;	// MM-DD
	string text;
}

class CGQC_GU_CriterionLine : Managed
{
	string text;	// "Q-SI obtenue", "2 mois au grade (1/2)"
	bool met;
}

class CGQC_GU_OverviewDto : Managed
{
	string name;			// "Maj. Cloutier"
	string sub;				// seat · role · section (Phase 4); access group until then
	string phase;			// "TEMPS LIBRE · GARNISON"
	int online;
	string notice;
	string nextRank;		// "Caporal-chef (CplC.)" or ""
	string nextCriteria;	// "Q-EF en cours · Q-NT à faire · décision des officiers"
	ref array<ref CGQC_GU_NewsLine> news = new array<ref CGQC_GU_NewsLine>();
}

class CGQC_GU_OverviewBuilder
{
	static const int NEWS_COUNT = 6;

	//------------------------------------------------------------------------------------------------
	static string Build(CGQC_GU_Manager mgr, CGQC_GU_Member me)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_OverviewDto dto = new CGQC_GU_OverviewDto();

		dto.name = NameOf(cat, me);
		CGQC_GU_AccessGroup g = CGQC_GU_Permissions.GetHighestGroup(cat, me);
		if (g)
			dto.sub = g.label;

		dto.phase = PhaseLabel(mgr.m_Situation);
		dto.online = mgr.GetOnlineCount();
		dto.notice = mgr.m_Notice.text;

		NextRank(cat, me, dto);
		News(mgr, dto);

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("o", dto);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	static string NameOf(CGQC_GU_Catalogues cat, CGQC_GU_Member m)
	{
		CGQC_GU_Rank r = cat.FindRank(m.rank);
		if (r)
			return r.abbr + " " + m.displayName;

		return m.displayName;
	}

	//------------------------------------------------------------------------------------------------
	static string PhaseLabel(CGQC_GU_Situation s)
	{
		string phase;
		switch (s.phase)
		{
			case "temps_libre":		phase = "TEMPS LIBRE"; break;
			case "entrainement":	phase = "ENTRAÎNEMENT"; break;
			case "briefing":		phase = "BRIEFING"; break;
			case "operation":		phase = "OPÉRATION"; break;
			case "aar":				phase = "AAR"; break;
			default:				phase = s.phase;
		}

		if (s.phase == "entrainement" && s.qualification)
			phase += " · QUALIFICATION";

		string status = "GARNISON";
		if (s.unitStatus == "deploye")
			status = "DÉPLOYÉ";

		return phase + " · " + status;
	}

	//------------------------------------------------------------------------------------------------
	//! Next rank and its criteria, from promotions.json (shared with the Dossier's Grade & accès tab).
	protected static void NextRank(CGQC_GU_Catalogues cat, CGQC_GU_Member me, CGQC_GU_OverviewDto dto)
	{
		array<ref CGQC_GU_CriterionLine> lines = new array<ref CGQC_GU_CriterionLine>();
		bool automatic;
		string next = BuildCriteria(cat, me, lines, automatic);
		if (next.IsEmpty())
			return;

		dto.nextRank = next;

		array<string> parts = new array<string>();
		foreach (CGQC_GU_CriterionLine line : lines)
		{
			parts.Insert(line.text);
		}

		string criteria = CGQC_GU_Str.Join(parts, " · ");
		if (automatic)
			criteria = "Automatique : " + criteria;

		dto.nextCriteria = criteria;
	}

	//------------------------------------------------------------------------------------------------
	//! Fills the criteria lines for the member's next rank and returns "Caporal-chef (CplC.)", or "" when
	//! there is none. Officers' decision and other checks are never marked met: people decide those.
	static string BuildCriteria(CGQC_GU_Catalogues cat, CGQC_GU_Member me, notnull array<ref CGQC_GU_CriterionLine> lines, out bool automatic)
	{
		automatic = false;
		CGQC_GU_Promotion p;
		foreach (CGQC_GU_Promotion candidate : cat.m_aPromotions)
		{
			if (candidate.from == me.rank)
			{
				p = candidate;
				break;
			}
		}

		if (!p)
			return string.Empty;

		CGQC_GU_Rank to = cat.FindRank(p.to);
		if (!to)
			return string.Empty;

		automatic = p.automatic;

		foreach (string code : p.requiredQuals)
		{
			string state = QualState(me, code);
			AddLine(lines, code + " " + state, state == "obtenue");
		}

		foreach (CGQC_GU_AnyOf any : p.anyOf)
		{
			int have;
			foreach (string q : any.quals)
			{
				if (HasPassed(me, q))
					have++;
			}

			string anyState = "à faire";
			if (have >= any.count)
				anyState = "obtenue";

			AddLine(lines, string.Format("%1 %2 (%3/%4)", AnyOfGroupName(any), anyState, have, any.count), have >= any.count);
		}

		if (p.minMonthsInRank > 0)
		{
			int months = CGQC_GU_Time.MonthsSince(MonthsInRankStart(me));
			AddLine(lines, string.Format("%1 mois au grade (%2/%1)", p.minMonthsInRank, Math.Min(months, p.minMonthsInRank)), months >= p.minMonthsInRank);
		}

		foreach (CGQC_GU_LabelItem check : p.otherChecks)
		{
			AddLine(lines, CGQC_GU_Str.Lower(check.label), false);
		}

		if (p.officerDecision)
			AddLine(lines, "décision des officiers", false);

		return string.Format("%1 (%2)", to.label, to.abbr);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddLine(array<ref CGQC_GU_CriterionLine> lines, string text, bool met)
	{
		CGQC_GU_CriterionLine line = new CGQC_GU_CriterionLine();
		line.text = text;
		line.met = met;
		lines.Insert(line);
	}

	//------------------------------------------------------------------------------------------------
	protected static string AnyOfGroupName(CGQC_GU_AnyOf any)
	{
		if (any.quals.IsEmpty())
			return "?";

		// "QSP1-MED" -> "QSP1"
		string first = any.quals[0];
		int dash = first.IndexOf("-");
		if (dash > 0)
			return first.Substring(0, dash);

		return first;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasPassed(CGQC_GU_Member m, string code)
	{
		foreach (CGQC_GU_QualResult r : m.qualifications)
		{
			if (r.qual == code && r.result == "passed")
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static string QualState(CGQC_GU_Member m, string code)
	{
		if (HasPassed(m, code))
			return "obtenue";

		foreach (CGQC_GU_QualResult r : m.qualifications)
		{
			if (r.qual == code && r.result == "failed")
				return "à reprendre";
		}

		foreach (CGQC_GU_Progress pr : m.progress)
		{
			if (pr.qual == code)
				return "en cours";
		}

		return "à faire";
	}

	//------------------------------------------------------------------------------------------------
	//! Date the current rank started: the last PROMOTION entry to that rank, else the enlistment date.
	static string MonthsInRankStart(CGQC_GU_Member m)
	{
		for (int i = m.record.Count() - 1; i >= 0; i--)
		{
			CGQC_GU_RecordEntry e = m.record[i];
			string rank;
			if (e.type == "promotion" && e.data.Find("rank", rank) && rank == m.rank)
				return e.on;
		}

		return m.enlistedOn;
	}

	//------------------------------------------------------------------------------------------------
	protected static void News(CGQC_GU_Manager mgr, CGQC_GU_OverviewDto dto)
	{
		array<ref CGQC_GU_NewsItem> items = mgr.m_News.items;
		int added;
		for (int i = items.Count() - 1; i >= 0 && added < NEWS_COUNT; i--)
		{
			string text = NewsText(mgr, items[i]);
			if (text.IsEmpty())
				continue;

			CGQC_GU_NewsLine line = new CGQC_GU_NewsLine();
			line.date = ShortDate(items[i].on);
			line.text = text;
			dto.news.Insert(line);
			added++;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string NewsText(CGQC_GU_Manager mgr, CGQC_GU_NewsItem item)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		string uid;
		item.data.Find("uid", uid);
		CGQC_GU_Member m = mgr.FindMember(uid);
		if (!m)
			return string.Empty;

		switch (item.type)
		{
			case "enlistment":
				return NameOf(cat, m) + " a rejoint l'unité";

			case "promotion":
			{
				string rankId, fromId;
				item.data.Find("rank", rankId);
				item.data.Find("from", fromId);
				CGQC_GU_Rank r = cat.FindRank(rankId);
				if (!r)
					return string.Empty;

				if (!fromId.IsEmpty() && cat.GetRankOrder(fromId) > r.order)
					return string.Format("%1 %2 rétrogradé %3", r.abbr, m.displayName, r.label);

				return string.Format("%1 %2 promu %3", r.abbr, m.displayName, r.label);
			}
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! "2026-11-25T..." -> "11-25" (MM-DD, owner's format)
	static string ShortDate(string iso)
	{
		if (iso.Length() < 10)
			return iso;

		return iso.Substring(5, 2) + "-" + iso.Substring(8, 2);
	}
}
