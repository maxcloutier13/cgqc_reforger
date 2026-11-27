// Gestion d'unité - Dossier (paper screen): one read, one display-ready reply for the fiche, the Registre
// and Grade & accès. Built on the server from codes + catalogue labels; display text is never stored.
// The viewer flags (canEdit, canAccess) only decide what the screen offers: every write is checked again
// on the server by its own action.

class CGQC_GU_DossierRecordLine : Managed
{
	string date;	// YYMMDD "261125"
	string type;	// "PROMOTION"
	string text;
	string by;		// "Maj. Cloutier", "Système"
}

class CGQC_GU_DossierRank : Managed
{
	string id;
	string abbr;
	string label;
	string insignia;
	bool current;
}

class CGQC_GU_DossierGroup : Managed
{
	string id;
	string label;
	bool held;
}

class CGQC_GU_DossierDto : Managed
{
	// Identity
	string uid;
	bool uidLinked;
	string footer;			// QC-<uid>-CGQC
	string displayName;
	string lastName;
	string profileName;
	string rank;
	string rankAbbr;
	string rankLabel;
	string insignia;
	string access;			// highest group label
	bool online;
	string presence;		// "EN LIGNE" / "Vu le 261125"
	string status;			// "ACTIF"

	// Renseignements et états de service
	string birthDate;		// raw ISO, for editing
	string birthPlace;
	string bloodType;
	string bio;
	string enlistedOn;		// display
	string seniority;		// "4 ans 7 mois"
	int operations;
	int deployments;
	int distinctions;
	int sessions;
	int studentsEvaluated;
	string qualsObtained;	// "Q-MB · Q-SI" or ""
	string formations;		// qualifications this member may teach

	// Registre, newest first
	ref array<ref CGQC_GU_DossierRecordLine> record = new array<ref CGQC_GU_DossierRecordLine>();

	// Grade & accès
	ref array<ref CGQC_GU_DossierRank> ranks = new array<ref CGQC_GU_DossierRank>();
	ref array<string> roles = new array<string>();
	string nextRank;
	bool nextAutomatic;
	ref array<ref CGQC_GU_CriterionLine> criteria = new array<ref CGQC_GU_CriterionLine>();
	ref array<ref CGQC_GU_DossierGroup> groups = new array<ref CGQC_GU_DossierGroup>();

	// Viewer
	bool isSelf;
	bool canEdit;			// members.edit
	bool canAccess;			// access.manage
}

class CGQC_GU_DossierBuilder
{
	//------------------------------------------------------------------------------------------------
	static string Build(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Member m)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_DossierDto d = new CGQC_GU_DossierDto();

		d.uid = m.uid;
		d.uidLinked = m.uidLinked;
		d.footer = "QC-" + m.uid + "-CGQC";
		d.displayName = m.displayName;
		d.lastName = m.lastName;
		d.profileName = m.profileName;
		d.rank = m.rank;

		CGQC_GU_Rank r = cat.FindRank(m.rank);
		if (r)
		{
			d.rankAbbr = r.abbr;
			d.rankLabel = r.label;
			d.insignia = r.insignia;
		}

		CGQC_GU_AccessGroup g = CGQC_GU_Permissions.GetHighestGroup(cat, m);
		if (g)
			d.access = g.label;

		d.online = mgr.IsOnline(m.uid);
		if (d.online)
			d.presence = "EN LIGNE";
		else if (!m.lastSeenAt.IsEmpty())
			d.presence = "Vu le " + DisplayDate(m.lastSeenAt);
		else
			d.presence = "Jamais connecté";

		d.status = StatusLabel(m.GetEffectiveStatus());

		// Renseignements
		d.birthDate = m.personal.birthDate;
		d.birthPlace = m.personal.birthPlace;
		d.bloodType = m.personal.bloodType;
		d.bio = m.personal.bio;
		d.enlistedOn = DisplayDate(m.enlistedOn);
		if (!m.enlistedOn.IsEmpty())
			d.seniority = Seniority(CGQC_GU_Time.MonthsSince(m.enlistedOn));

		d.operations = m.stats.operations;
		d.deployments = m.stats.deployments;
		d.distinctions = m.distinctions.Count();
		d.sessions = m.stats.sessions;
		d.studentsEvaluated = m.stats.studentsEvaluated;

		array<string> quals = new array<string>();
		foreach (CGQC_GU_QualResult q : m.qualifications)
		{
			if (q.result == "passed" && !quals.Contains(q.qual))
				quals.Insert(q.qual);
		}

		d.qualsObtained = CGQC_GU_Str.Join(quals, " · ");

		array<string> teach = new array<string>();
		foreach (CGQC_GU_InstructorAuth auth : m.instructorFor)
		{
			teach.Insert(auth.qual);
		}

		d.formations = CGQC_GU_Str.Join(teach, " · ");

		Record(mgr, m, d);
		Ranks(cat, m, d);
		Roles(cat, m, d);

		bool automatic;
		d.nextRank = CGQC_GU_OverviewBuilder.BuildCriteria(cat, m, d.criteria, automatic);
		d.nextAutomatic = automatic;

		foreach (CGQC_GU_AccessGroup group : cat.m_aAccessGroups)
		{
			CGQC_GU_DossierGroup dg = new CGQC_GU_DossierGroup();
			dg.id = group.id;
			dg.label = group.label;
			dg.held = m.accessGroups.Contains(group.id);
			d.groups.Insert(dg);
		}

		d.isSelf = viewer == m;
		d.canEdit = CGQC_GU_Permissions.HasRight(cat, viewer, CGQC_GU_Rights.MEMBERS_EDIT);
		d.canAccess = CGQC_GU_Permissions.HasRight(cat, viewer, CGQC_GU_Rights.ACCESS_MANAGE);

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("d", d);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	protected static void Record(CGQC_GU_Manager mgr, CGQC_GU_Member m, CGQC_GU_DossierDto d)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		for (int i = m.record.Count() - 1; i >= 0; i--)
		{
			CGQC_GU_RecordEntry e = m.record[i];
			CGQC_GU_DossierRecordLine line = new CGQC_GU_DossierRecordLine();
			line.date = DisplayDate(e.on);
			line.type = RecordTypeLabel(e.type);
			line.by = SignerName(mgr, e.by);

			string text;
			switch (e.type)
			{
				case "enlistment":
					text = "Enrôlement dans l'unité";
					break;

				case "promotion":
				{
					string rankId, fromId;
					e.data.Find("rank", rankId);
					e.data.Find("from", fromId);
					CGQC_GU_Rank r = cat.FindRank(rankId);
					if (r)
					{
						if (!fromId.IsEmpty() && cat.GetRankOrder(fromId) > r.order)
							text = string.Format("Rétrogradé %1 (%2)", r.label, r.abbr);
						else
							text = string.Format("Promu %1 (%2)", r.label, r.abbr);
					}

					break;
				}

				case "status":
				{
					string statusId, effective;
					e.data.Find("status", statusId);
					e.data.Find("effective", effective);
					if (statusId == "auto")
						text = "Statut automatique rétabli : " + StatusLabel(effective);
					else
						text = "Statut fixé : " + StatusLabel(statusId);

					break;
				}
			}

			if (!e.note.IsEmpty())
			{
				if (text.IsEmpty())
					text = e.note;
				else
					text = text + " · " + e.note;
			}

			line.text = text;
			d.record.Insert(line);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Every rank, most senior first; honorary ranks (Vétéran) last.
	protected static void Ranks(CGQC_GU_Catalogues cat, CGQC_GU_Member m, CGQC_GU_DossierDto d)
	{
		array<CGQC_GU_Rank> sorted = new array<CGQC_GU_Rank>();
		foreach (CGQC_GU_Rank r : cat.m_aRanks)
		{
			int at = sorted.Count();
			for (int i = 0; i < sorted.Count(); i++)
			{
				if (Before(r, sorted[i]))
				{
					at = i;
					break;
				}
			}

			sorted.InsertAt(r, at);
		}

		foreach (CGQC_GU_Rank r : sorted)
		{
			CGQC_GU_DossierRank dr = new CGQC_GU_DossierRank();
			dr.id = r.id;
			dr.abbr = r.abbr;
			dr.label = r.label;
			dr.insignia = r.insignia;
			dr.current = r.id == m.rank;
			d.ranks.Insert(dr);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Before(CGQC_GU_Rank a, CGQC_GU_Rank b)
	{
		if (a.honorary != b.honorary)
			return !a.honorary;

		return a.order > b.order;
	}

	//------------------------------------------------------------------------------------------------
	//! Roles unlocked by the rank (cumulative, roles.json).
	protected static void Roles(CGQC_GU_Catalogues cat, CGQC_GU_Member m, CGQC_GU_DossierDto d)
	{
		CGQC_GU_Rank mine = cat.FindRank(m.rank);
		if (!mine || mine.honorary)
			return;

		foreach (CGQC_GU_Role role : cat.m_aRoles)
		{
			CGQC_GU_Rank need = cat.FindRank(role.minRank);
			if (need && need.order <= mine.order)
				d.roles.Insert(role.label);
		}
	}

	//------------------------------------------------------------------------------------------------
	static string SignerName(CGQC_GU_Manager mgr, string by)
	{
		if (by.IsEmpty() || by == "systeme")
			return "Système";

		if (by == "console")
			return "Console";

		CGQC_GU_Member signer = mgr.FindMember(by);
		if (!signer)
			return "Inconnu";

		return CGQC_GU_OverviewBuilder.NameOf(mgr.m_Catalogues, signer);
	}

	//------------------------------------------------------------------------------------------------
	static string RecordTypeLabel(string type)
	{
		switch (type)
		{
			case "enlistment":		return "ENRÔLEMENT";
			case "promotion":		return "PROMOTION";
			case "operation":		return "OPÉRATION";
			case "deployment":		return "DÉPLOIEMENT";
			case "session":			return "SÉANCE / FTX";
			case "qualification":	return "QUALIFICATION";
			case "distinction":		return "DISTINCTION";
			case "task":			return "TÂCHE";
			case "note":			return "NOTE";
			case "status":			return "STATUT";
		}

		return CGQC_GU_Str.Upper(type);
	}

	//------------------------------------------------------------------------------------------------
	static string StatusLabel(string status)
	{
		switch (status)
		{
			case "actif":	return "ACTIF";
			case "inactif":	return "INACTIF";
			case "mia":		return "MIA";
			case "kia":		return "KIA";
		}

		return CGQC_GU_Str.Upper(status);
	}

	//------------------------------------------------------------------------------------------------
	//! Full dates as YYMMDD (owner's format): "2026-11-25[T…]" -> "261125", "2022-04" -> "2204", "2022" -> "2022".
	static string DisplayDate(string iso)
	{
		if (iso.Length() >= 10)
			return iso.Substring(2, 2) + iso.Substring(5, 2) + iso.Substring(8, 2);

		if (iso.Length() >= 7)
			return iso.Substring(2, 2) + iso.Substring(5, 2);

		return iso;
	}

	//------------------------------------------------------------------------------------------------
	static string Seniority(int months)
	{
		int years = months / 12;
		int rest = months % 12;
		string y;
		if (years == 1)
			y = "1 an";
		else if (years > 1)
			y = string.Format("%1 ans", years);

		string mo = string.Format("%1 mois", rest);
		if (y.IsEmpty())
			return mo;

		if (rest == 0)
			return y;

		return y + " " + mo;
	}
}
