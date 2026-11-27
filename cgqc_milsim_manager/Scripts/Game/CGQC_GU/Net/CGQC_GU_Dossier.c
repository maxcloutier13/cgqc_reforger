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

//! Dossier › Qualifications: the latest state of each qualification.
class CGQC_GU_DossierQualLine : Managed
{
	string code;
	string title;
	string level;		// "Caporal"
	string result;		// RÉUSSI / ÉCHEC / EN COURS
	string kind;		// ok | ko | cur (tag colour)
	string date;		// YYMMDD or "—"
	string evaluator;	// "Adju. Pelletier" or "—"
}

//! Dossier › Qualifications › Exercices réussis (test exercises of a passed qualification).
class CGQC_GU_DossierExerciseLine : Managed
{
	string code;
	string title;
	string qual;
	string date;
	string instructor;
}

//! A tick in the Progression panel: a test exercise (id = code) or a criterion (id = c1…).
class CGQC_GU_DossierCheck : Managed
{
	string id;
	string text;
	bool required;
	bool met;
}

//! Progression ladder / Insignes: one entry per qualification in the catalogue.
class CGQC_GU_DossierLadderItem : Managed
{
	string code;
	string title;
	int pp;
	string ppLabel;
	string levelAbbr;
	string levelLabel;
	string group;			// "QSP1" or ""
	bool aVenir;			// no content in the documentation yet
	string state;			// obtenu | en_cours | reprise | a_venir
	string stateDate;		// YYMMDD of the latest result, or ""
	ref array<ref CGQC_GU_DossierCheck> exercises = new array<ref CGQC_GU_DossierCheck>();
	ref array<ref CGQC_GU_DossierCheck> criteria = new array<ref CGQC_GU_DossierCheck>();
	string notes;
	string saved;			// "Progression enregistrée le 261124 par Lt. Gagnon" or ""
	string teachers;		// "Capt. Roy, Lt. Gagnon" or ""
}

//! Dossier › Séances / FTX: finished sessions the member attended.
class CGQC_GU_DossierSessionLine : Managed
{
	string id;
	string title;
	string sub;			// "Position : élève · réussi"
	string kind;		// ftx | training (filter)
	string type;		// FTX / ENTR.
	string date;		// YYMMDD
	string qual;		// "Q-MB" or "—"
	string exercises;	// "Q-MB-E2 · Q-MB-E3" or "—"
	string instructor;
}

//! Dossier › Instructeur: qualifications this member may teach.
class CGQC_GU_DossierTeachLine : Managed
{
	string qual;
	string title;
	string by;
	string on;
	int students;
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

	// Phase 3: Qualifications, Progression, Instructeur, Insignes
	ref array<ref CGQC_GU_DossierQualLine> quals = new array<ref CGQC_GU_DossierQualLine>();
	ref array<ref CGQC_GU_DossierExerciseLine> exercisesDone = new array<ref CGQC_GU_DossierExerciseLine>();
	ref array<ref CGQC_GU_DossierLadderItem> ladder = new array<ref CGQC_GU_DossierLadderItem>();
	ref array<ref CGQC_GU_DossierTeachLine> teach = new array<ref CGQC_GU_DossierTeachLine>();
	ref array<ref CGQC_GU_DossierSessionLine> sessionLines = new array<ref CGQC_GU_DossierSessionLine>();
	string insignes;		// codes obtained, for the fiche ("Q-MB · Q-SI")
	int formationsCount;	// qualifications this member may teach
	bool promotionPossible;	// only sent true to promotion.view holders

	// Viewer
	bool isSelf;
	bool canEdit;			// members.edit
	bool canAccess;			// access.manage
	bool canInstruct;		// instructors.manage
	bool canPromotion;		// promotion.view
	ref array<string> evaluable = new array<string>();	// qualification codes the viewer may evaluate here
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
		d.canInstruct = CGQC_GU_Permissions.HasRight(cat, viewer, CGQC_GU_Rights.INSTRUCTORS_MANAGE);
		d.canPromotion = CGQC_GU_Permissions.HasRight(cat, viewer, CGQC_GU_Rights.PROMOTION_POSSIBLE);
		if (d.canPromotion)
			d.promotionPossible = CGQC_GU_Quals.PromotionPossible(cat, m);

		CGQC_GU_Quals.EvaluableBy(mgr, viewer, m, d.evaluable);
		Training(mgr, m, d);

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("d", d);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	//! Phase 3 tabs: Qualifications, Exercices réussis, Progression ladder / Insignes, Instructeur.
	protected static void Training(CGQC_GU_Manager mgr, CGQC_GU_Member m, CGQC_GU_DossierDto d)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		array<string> obtained = new array<string>();

		// Qualifications table: in-progress first, then the latest result of each, newest first.
		array<ref CGQC_GU_DossierQualLine> done = new array<ref CGQC_GU_DossierQualLine>();
		array<string> doneDates = new array<string>();
		foreach (CGQC_GU_Qualification q : cat.m_aQualifications)
		{
			string level;
			CGQC_GU_Rank lr = cat.FindRank(q.levelRank);
			if (lr)
				level = lr.label;

			CGQC_GU_QualResult last = CGQC_GU_Quals.LatestResult(m, q.code);
			bool passed = CGQC_GU_Quals.HasPassed(m, q.code);
			if (passed)
				obtained.Insert(q.code);

			if (!last && !CGQC_GU_Quals.FindProgress(m, q.code))
				continue;

			CGQC_GU_DossierQualLine line = new CGQC_GU_DossierQualLine();
			line.code = q.code;
			line.title = q.title;
			line.level = level;
			if (passed || last)
			{
				// A pass stays the shown result even if a later line exists.
				CGQC_GU_QualResult shown = last;
				if (passed)
				{
					foreach (CGQC_GU_QualResult r : m.qualifications)
					{
						if (r.qual == q.code && r.result == CGQC_GU_Quals.RESULT_PASSED)
							shown = r;
					}
				}

				line.result = CGQC_GU_Quals.ResultLabel(shown.result);
				line.kind = "ko";
				if (shown.result == CGQC_GU_Quals.RESULT_PASSED)
					line.kind = "ok";

				line.date = DisplayDate(shown.on);
				line.evaluator = SignerOrDash(mgr, shown.evaluator);

				// A failed qualification with a new progress reads EN COURS again.
				if (!passed && CGQC_GU_Quals.FindProgress(m, q.code))
				{
					line.result = CGQC_GU_Quals.ResultLabel("");
					line.kind = "cur";
				}

				int at = done.Count();
				for (int i = 0; i < done.Count(); i++)
				{
					if (shown.on.Compare(doneDates[i]) > 0)
					{
						at = i;
						break;
					}
				}

				done.InsertAt(line, at);
				doneDates.InsertAt(shown.on, at);
			}
			else
			{
				line.result = CGQC_GU_Quals.ResultLabel("");
				line.kind = "cur";
				line.date = "—";
				line.evaluator = "—";
				d.quals.Insert(line);
			}
		}

		foreach (CGQC_GU_DossierQualLine dl : done)
		{
			d.quals.Insert(dl);
		}

		d.insignes = CGQC_GU_Str.Join(obtained, " · ");

		// Exercices réussis, newest first.
		for (int x = m.exercises.Count() - 1; x >= 0; x--)
		{
			CGQC_GU_ExerciseDone ex = m.exercises[x];
			CGQC_GU_DossierExerciseLine el = new CGQC_GU_DossierExerciseLine();
			el.code = ex.code;
			CGQC_GU_Exercise ce = cat.FindExercise(ex.code);
			if (ce)
			{
				el.title = ce.title;
				el.qual = ce.qual;
			}

			el.date = DisplayDate(ex.on);
			el.instructor = SignerOrDash(mgr, ex.instructor);
			d.exercisesDone.Insert(el);
		}

		// Ladder: every qualification with its state, test exercises, criteria and teachers.
		foreach (CGQC_GU_Qualification lq : cat.m_aQualifications)
		{
			CGQC_GU_DossierLadderItem item = new CGQC_GU_DossierLadderItem();
			item.code = lq.code;
			item.title = lq.title;
			item.pp = lq.pp;
			item.ppLabel = CGQC_GU_Quals.PpLabel(lq.pp);
			CGQC_GU_Rank rr = cat.FindRank(lq.levelRank);
			if (rr)
			{
				item.levelAbbr = rr.abbr;
				item.levelLabel = rr.label;
			}

			item.group = lq.group;
			item.aVenir = lq.state != "actif";
			item.state = CGQC_GU_Quals.InsigniaStateFull(mgr, m, lq.code);
			CGQC_GU_QualResult lr2 = CGQC_GU_Quals.LatestResult(m, lq.code);
			if (lr2)
				item.stateDate = DisplayDate(lr2.on);

			CGQC_GU_Progress pr = CGQC_GU_Quals.FindProgress(m, lq.code);
			array<CGQC_GU_Exercise> exs = new array<CGQC_GU_Exercise>();
			cat.ExercisesOf(lq.code, exs);
			foreach (CGQC_GU_Exercise e : exs)
			{
				CGQC_GU_DossierCheck c = new CGQC_GU_DossierCheck();
				c.id = e.code;
				c.text = e.title;
				c.required = e.required;
				if (pr)
					c.met = pr.exercises.Contains(e.code);
				else if (item.state == CGQC_GU_Quals.STATE_OBTAINED)
					c.met = HasExercise(m, e.code);

				item.exercises.Insert(c);
			}

			foreach (CGQC_GU_LabelItem crit : lq.criteria)
			{
				CGQC_GU_DossierCheck cc = new CGQC_GU_DossierCheck();
				cc.id = crit.id;
				cc.text = crit.label;
				cc.required = true;
				if (pr)
				{
					bool met;
					pr.criteria.Find(crit.id, met);
					cc.met = met;
				}
				else if (item.state == CGQC_GU_Quals.STATE_OBTAINED)
				{
					cc.met = true;
				}

				item.criteria.Insert(cc);
			}

			if (pr)
			{
				item.notes = pr.notes;
				item.saved = string.Format("Progression enregistrée le %1 par %2", DisplayDate(pr.savedAt), SignerName(mgr, pr.savedBy));
			}

			array<string> teachers = new array<string>();
			foreach (string tuid, CGQC_GU_Member t : mgr.m_mMembers)
			{
				if (CGQC_GU_Quals.IsInstructorFor(t, lq.code))
					teachers.Insert(CGQC_GU_OverviewBuilder.NameOf(cat, t));
			}

			item.teachers = CGQC_GU_Str.Join(teachers, ", ");
			d.ladder.Insert(item);
		}

		// Instructeur tab.
		foreach (CGQC_GU_InstructorAuth auth : m.instructorFor)
		{
			CGQC_GU_DossierTeachLine tl = new CGQC_GU_DossierTeachLine();
			tl.qual = auth.qual;
			CGQC_GU_Qualification tq = cat.FindQualification(auth.qual);
			if (tq)
				tl.title = tq.title;

			tl.by = SignerName(mgr, auth.authorizedBy);
			tl.on = DisplayDate(auth.on);
			tl.students = CGQC_GU_Quals.StudentsFor(mgr, m.uid, auth.qual);
			d.teach.Insert(tl);
		}

		d.formationsCount = m.instructorFor.Count();
		Sessions(mgr, m, d);
	}

	//------------------------------------------------------------------------------------------------
	//! TERMINÉE sessions where the member was present, newest first.
	protected static void Sessions(CGQC_GU_Manager mgr, CGQC_GU_Member m, CGQC_GU_DossierDto d)
	{
		array<CGQC_GU_Training> done = new array<CGQC_GU_Training>();
		foreach (string id, CGQC_GU_Training t : mgr.m_mTrainings)
		{
			if (t.status != CGQC_GU_Training.STATUS_DONE)
				continue;

			CGQC_GU_AttendanceRow row = t.FindRow(m.uid);
			if (!row || !row.present)
				continue;

			int at = done.Count();
			for (int i = 0; i < done.Count(); i++)
			{
				if (t.SortKey().Compare(done[i].SortKey()) > 0)
				{
					at = i;
					break;
				}
			}

			done.InsertAt(t, at);
		}

		foreach (CGQC_GU_Training s : done)
		{
			CGQC_GU_AttendanceRow r = s.FindRow(m.uid);
			CGQC_GU_DossierSessionLine line = new CGQC_GU_DossierSessionLine();
			line.id = s.id;
			line.title = s.title;
			line.kind = s.type;
			line.type = "ENTR.";
			if (s.type == "ftx")
				line.type = "FTX";

			string sub = "Position : —";
			if (!r.position.IsEmpty())
				sub = "Position : " + r.position;

			if (r.result == CGQC_GU_Quals.RESULT_PASSED)
				sub += " · réussi";
			else if (r.result == CGQC_GU_Quals.RESULT_FAILED)
				sub += " · échoué";

			line.sub = sub;
			line.date = DisplayDate(s.date);
			line.qual = "—";
			if (!s.qual.IsEmpty())
				line.qual = s.qual;

			line.exercises = "—";
			if (!s.exercises.IsEmpty())
				line.exercises = CGQC_GU_Str.Join(s.exercises, " · ");

			line.instructor = SignerOrDash(mgr, s.instructor);
			d.sessionLines.Insert(line);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasExercise(CGQC_GU_Member m, string code)
	{
		foreach (CGQC_GU_ExerciseDone e : m.exercises)
		{
			if (e.code == code)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static string SignerOrDash(CGQC_GU_Manager mgr, string by)
	{
		if (by.IsEmpty())
			return "—";

		return SignerName(mgr, by);
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

				case "session":
				{
					string sid, sposition;
					e.data.Find("sessionId", sid);
					e.data.Find("position", sposition);
					CGQC_GU_Training st = mgr.m_mTrainings.Get(sid);
					if (st)
						text = CGQC_GU_TrainingRequests.TypeLabel(st.type) + " : " + st.title;
					else
						text = "Séance";

					if (!sposition.IsEmpty())
						text += " · Position : " + sposition;

					break;
				}

				case "qualification":
				{
					string qual, result;
					e.data.Find("qual", qual);
					e.data.Find("result", result);
					if (result == CGQC_GU_Quals.RESULT_PASSED)
						text = qual + " réussie";
					else if (result == CGQC_GU_Quals.RESULT_FAILED)
						text = qual + " : échec";
					else
						text = qual;

					break;
				}

				case "note":
				{
					string teach, action;
					if (e.data.Find("instructorFor", teach))
					{
						e.data.Find("action", action);
						if (action == "revoke")
							text = "Autorisation d'enseigner retirée : " + teach;
						else
							text = "Autorisé à enseigner : " + teach;
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
