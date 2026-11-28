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
	// Grade & accès box (3g)
	string label;	// "Q-SI — title", "2 mois au grade"
	string state;	// OBTENUE / EN COURS / REPRISE / À VENIR / EN ATTENTE
	string kind;	// ok | ko | cur | dim (tag colour)
}

class CGQC_GU_OverviewDto : Managed
{
	string name;			// "Maj. Cloutier"
	string sub;				// "S1-1 · Fusilier · Spartan", "Zeus"; access group when no seat
	string leaderLabel;		// "CHEF D'ÉQUIPE" (4f); "" = no block
	string leader;			// "S1-0 Adju. Tremblay"
	string phase;			// "TEMPS LIBRE · GARNISON"
	int online;
	string notice;
	string nextRank;		// "Caporal-chef (CplC.)" or ""
	string nextCriteria;	// "Q-EF en cours · Q-NT à faire · décision des officiers"
	string nextEvent;		// « PROCHAIN AU CALENDRIER »: "12-02 · 19 h 30 · Jour du Souvenir" (2d)
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

		CGQC_GU_SeatInfo seat = CGQC_GU_OrbatRequests.SeatInfo(mgr, me.uid);
		if (!seat.callsign.IsEmpty())
			dto.sub = seat.Line();

		dto.leaderLabel = seat.leaderLabel;
		dto.leader = seat.leader;

		dto.phase = PhaseLabel(mgr.m_Situation);
		dto.online = mgr.GetOnlineCount();
		dto.notice = mgr.m_Notice.text;

		NextRank(mgr, me, dto);
		News(mgr, dto);
		dto.nextEvent = CGQC_GU_CalendarBuilder.NextLine(mgr);

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
		// A training with the Qualification box ticked reads QUALIFICATION alone (owner, 2026-11-26).
		return CGQC_GU_SituationBuilder.DisplayPhase(s.phase, s.qualification) + " · " + CGQC_GU_SituationBuilder.UnitStatusName(s.unitStatus);
	}

	//------------------------------------------------------------------------------------------------
	//! Next rank and its criteria, from promotions.json (shared with the Dossier's Grade & accès tab).
	protected static void NextRank(CGQC_GU_Manager mgr, CGQC_GU_Member me, CGQC_GU_OverviewDto dto)
	{
		array<ref CGQC_GU_CriterionLine> lines = new array<ref CGQC_GU_CriterionLine>();
		bool automatic;
		string next = BuildCriteria(mgr, me, lines, automatic);
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
	//! Each qualification reads its real state, as on the Insignes (a planned session counts as en cours).
	static string BuildCriteria(CGQC_GU_Manager mgr, CGQC_GU_Member me, notnull array<ref CGQC_GU_CriterionLine> lines, out bool automatic)
	{
		automatic = false;
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_Promotion p = CGQC_GU_Quals.NextPromotion(cat, me);
		if (!p)
			return string.Empty;

		CGQC_GU_Rank to = cat.FindRank(p.to);
		if (!to)
			return string.Empty;

		automatic = p.automatic;

		foreach (string code : p.requiredQuals)
		{
			string state = CGQC_GU_Quals.InsigniaStateFull(mgr, me, code);
			string label = code;
			CGQC_GU_Qualification qual = cat.FindQualification(code);
			if (qual)
				label = code + " — " + qual.title;

			CGQC_GU_CriterionLine line = AddLine(lines, code + " " + StateWords(state), state == CGQC_GU_Quals.STATE_OBTAINED, label);
			SetState(line, state);
		}

		foreach (CGQC_GU_AnyOf any : p.anyOf)
		{
			int have = 0;
			string best = CGQC_GU_Quals.STATE_TODO;
			foreach (string q : any.quals)
			{
				string s = CGQC_GU_Quals.InsigniaStateFull(mgr, me, q);
				if (s == CGQC_GU_Quals.STATE_OBTAINED)
					have++;
				else if (s == CGQC_GU_Quals.STATE_RUNNING)
					best = s;
				else if (s == CGQC_GU_Quals.STATE_RETAKE && best == CGQC_GU_Quals.STATE_TODO)
					best = s;
			}

			if (have >= any.count)
				best = CGQC_GU_Quals.STATE_OBTAINED;

			string group = AnyOfGroupName(any);
			string anyLabel = group + " — une spécialité";
			if (any.count > 1)
				anyLabel = string.Format("%1 — %2 spécialités (%3/%2)", group, any.count, Math.Min(have, any.count));

			string anyText = "à faire";
			if (have >= any.count)
				anyText = "obtenue";

			CGQC_GU_CriterionLine anyLine = AddLine(lines, string.Format("%1 %2 (%3/%4)", group, anyText, have, any.count), have >= any.count, anyLabel);
			SetState(anyLine, best);
		}

		if (p.minMonthsInRank > 0)
		{
			int months = CGQC_GU_Time.MonthsSince(MonthsInRankStart(me));
			bool enough = months >= p.minMonthsInRank;
			CGQC_GU_CriterionLine monthLine = AddLine(lines, string.Format("%1 mois au grade (%2/%1)", p.minMonthsInRank, Math.Min(months, p.minMonthsInRank)),
				enough, string.Format("%1 mois au grade", p.minMonthsInRank));
			if (enough)
			{
				monthLine.state = "COMPLÉTÉ";
				monthLine.kind = "ok";
			}
			else
			{
				monthLine.state = string.Format("%1 / %2 MOIS", Math.Max(months, 0), p.minMonthsInRank);
				monthLine.kind = "dim";
			}
		}

		foreach (CGQC_GU_LabelItem check : p.otherChecks)
		{
			CGQC_GU_CriterionLine checkLine = AddLine(lines, CGQC_GU_Str.Lower(check.label), false, check.label);
			SetWaiting(checkLine);
		}

		if (p.officerDecision)
		{
			CGQC_GU_CriterionLine officers = AddLine(lines, "décision des officiers", false, "Décision des officiers");
			SetWaiting(officers);
		}

		return string.Format("%1 (%2)", to.label, to.abbr);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_CriterionLine AddLine(array<ref CGQC_GU_CriterionLine> lines, string text, bool met, string label)
	{
		CGQC_GU_CriterionLine line = new CGQC_GU_CriterionLine();
		line.text = text;
		line.met = met;
		line.label = label;
		lines.Insert(line);
		return line;
	}

	//------------------------------------------------------------------------------------------------
	//! Tag of a qualification criterion, from its insignia state.
	protected static void SetState(CGQC_GU_CriterionLine line, string state)
	{
		if (state == CGQC_GU_Quals.STATE_OBTAINED)
		{
			line.state = "OBTENUE";
			line.kind = "ok";
		}
		else if (state == CGQC_GU_Quals.STATE_RUNNING)
		{
			line.state = "EN COURS";
			line.kind = "cur";
		}
		else if (state == CGQC_GU_Quals.STATE_RETAKE)
		{
			line.state = "REPRISE";
			line.kind = "ko";
		}
		else
		{
			line.state = "À VENIR";
			line.kind = "dim";
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetWaiting(CGQC_GU_CriterionLine line)
	{
		line.state = "EN ATTENTE";
		line.kind = "dim";
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
	//! A qualification state inside the pause panel's criteria sentence.
	protected static string StateWords(string state)
	{
		if (state == CGQC_GU_Quals.STATE_OBTAINED)
			return "obtenue";

		if (state == CGQC_GU_Quals.STATE_RUNNING)
			return "en cours";

		if (state == CGQC_GU_Quals.STATE_RETAKE)
			return "à reprendre";

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
	//! Every news line is composed here, by type (Phase 2f). Unit-level types first; member types need
	//! the member's dossier. qualification: Phase 3 (passes only). Reserved: operation and deployment (5),
	//! distinction (6). Until their text exists here, such items are simply not shown.
	static string NewsText(CGQC_GU_Manager mgr, CGQC_GU_NewsItem item)
	{
		switch (item.type)
		{
			case "unit_status":
			{
				string status;
				item.data.Find("status", status);
				if (status == "deploye")
					return "L'unité est déployée";

				return "L'unité est de retour en garnison";
			}
		}

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

			case "qualification":
			{
				// The rank held when the qualification was obtained ("Cdt. Nadeau a obtenu la Q-MB"),
				// even if it promoted them at once.
				string qual, rankAt;
				item.data.Find("qual", qual);
				item.data.Find("rank", rankAt);
				CGQC_GU_Rank held = cat.FindRank(rankAt);
				string who = NameOf(cat, m);
				if (held)
					who = held.abbr + " " + m.displayName;

				return string.Format("%1 a obtenu la %2", who, qual);
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
