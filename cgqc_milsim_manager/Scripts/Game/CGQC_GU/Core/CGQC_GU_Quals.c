// Gestion d'unité - qualifications, instructors and the promotion engine (Phase 3). Server only.
// Everything reads the catalogues (qualifications.json, exercises.json, promotions.json); no rule is
// hardcoded here. Nothing is written unless a request asks for it (owner answer 4).

class CGQC_GU_Quals
{
	static const string RESULT_PASSED = "passed";
	static const string RESULT_FAILED = "failed";

	// Insignes states (owner answer 6)
	static const string STATE_OBTAINED = "obtenu";
	static const string STATE_RUNNING = "en_cours";
	static const string STATE_RETAKE = "reprise";
	static const string STATE_TODO = "a_venir";

	static const string INSTRUCTOR_GROUP = "instructeur";
	static const string COMMAND_GROUP = "commandement";

	//------------------------------------------------------------------------------------------------
	// Reads on one member
	//------------------------------------------------------------------------------------------------
	static bool HasPassed(CGQC_GU_Member m, string qual)
	{
		foreach (CGQC_GU_QualResult r : m.qualifications)
		{
			if (r.qual == qual && r.result == RESULT_PASSED)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! The newest result for that qualification, or null.
	static CGQC_GU_QualResult LatestResult(CGQC_GU_Member m, string qual)
	{
		CGQC_GU_QualResult best;
		foreach (CGQC_GU_QualResult r : m.qualifications)
		{
			if (r.qual != qual)
				continue;

			if (!best || r.on.Compare(best.on) >= 0)
				best = r;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Progress FindProgress(CGQC_GU_Member m, string qual)
	{
		foreach (CGQC_GU_Progress p : m.progress)
		{
			if (p.qual == qual)
				return p;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsInstructorFor(CGQC_GU_Member m, string qual)
	{
		foreach (CGQC_GU_InstructorAuth a : m.instructorFor)
		{
			if (a.qual == qual)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! OBTENU (latest result passed), REPRISE (latest failed), EN COURS (a saved progress), else À VENIR.
	//! A planned session linking the qualification also counts as EN COURS (added with the sessions, 3b).
	static string InsigniaState(CGQC_GU_Member m, string qual)
	{
		if (HasPassed(m, qual))
			return STATE_OBTAINED;

		CGQC_GU_QualResult last = LatestResult(m, qual);
		if (last && last.result == RESULT_FAILED)
			return STATE_RETAKE;

		if (FindProgress(m, qual))
			return STATE_RUNNING;

		return STATE_TODO;
	}

	//------------------------------------------------------------------------------------------------
	// Rights
	//------------------------------------------------------------------------------------------------
	//! The Commandement access group: the only one that may evaluate itself (owner, 3e test).
	static bool IsCommand(CGQC_GU_Member m)
	{
		return m && m.accessGroups.Contains(COMMAND_GROUP);
	}

	//------------------------------------------------------------------------------------------------
	//! Commandement (instructors.manage) evaluates every qualification; an instructor only those they are
	//! authorized for. Nobody but Commandement evaluates themselves.
	static bool CanEvaluate(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Member target, string qual)
	{
		if (!caller || !target)
			return false;

		if (caller == target && !IsCommand(caller))
			return false;

		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		if (CGQC_GU_Permissions.HasRight(cat, caller, CGQC_GU_Rights.INSTRUCTORS_MANAGE))
			return true;

		return CGQC_GU_Permissions.HasRight(cat, caller, CGQC_GU_Rights.QUALS_EVALUATE_OWN) && IsInstructorFor(caller, qual);
	}

	//------------------------------------------------------------------------------------------------
	//! May run a session linked to that qualification (and grade it): Commandement, or an authorized instructor.
	static bool CanTeach(CGQC_GU_Manager mgr, CGQC_GU_Member m, string qual)
	{
		if (!m)
			return false;

		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		if (CGQC_GU_Permissions.HasRight(cat, m, CGQC_GU_Rights.INSTRUCTORS_MANAGE))
			return true;

		return CGQC_GU_Permissions.HasRight(cat, m, CGQC_GU_Rights.QUALS_EVALUATE_OWN) && IsInstructorFor(m, qual);
	}

	//------------------------------------------------------------------------------------------------
	//! Insignia state including PLANIFIÉE sessions: a member on the attendance of a planned session that
	//! links the qualification reads EN COURS (owner answer 6).
	static string InsigniaStateFull(CGQC_GU_Manager mgr, CGQC_GU_Member m, string qual)
	{
		string state = InsigniaState(m, qual);
		if (state != STATE_TODO)
			return state;

		foreach (string id, CGQC_GU_Training t : mgr.m_mTrainings)
		{
			if (t.status != CGQC_GU_Training.STATUS_PLANNED || t.qual != qual)
				continue;

			// Assistants are on the list to help, not to take the qualification.
			CGQC_GU_AttendanceRow row = t.FindRow(m.uid);
			if (row && !CGQC_GU_TrainingRequests.IsInstructorPosition(row.position))
				return STATE_RUNNING;
		}

		return state;
	}

	//------------------------------------------------------------------------------------------------
	//! Merges test exercises passed in a session into the open progress (kept with their first date).
	static void MergeSessionExercises(CGQC_GU_Member m, string qual, array<string> codes, string by)
	{
		if (codes.IsEmpty())
			return;

		CGQC_GU_Progress p = FindProgress(m, qual);
		if (!p)
		{
			p = new CGQC_GU_Progress();
			p.qual = qual;
			m.progress.Insert(p);
		}

		string today = CGQC_GU_Time.TodayEastern();
		foreach (string code : codes)
		{
			if (!p.exercises.Contains(code))
				p.exercises.Set(code, today);
		}

		p.savedBy = by;
		p.savedAt = CGQC_GU_Time.NowUtc();
	}

	//------------------------------------------------------------------------------------------------
	//! Qualification codes the viewer may evaluate on that dossier (display only).
	static void EvaluableBy(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Member target, notnull array<string> outCodes)
	{
		foreach (CGQC_GU_Qualification q : mgr.m_Catalogues.m_aQualifications)
		{
			if (CanEvaluate(mgr, viewer, target, q.code))
				outCodes.Insert(q.code);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Writes (the caller has already been checked)
	//------------------------------------------------------------------------------------------------
	//! Saves the progress panel: ticked test exercises (kept with the date they were first ticked),
	//! criteria and notes. Codes that do not belong to the qualification are ignored.
	static CGQC_GU_Progress SaveProgress(CGQC_GU_Manager mgr, CGQC_GU_Member m, string qual, array<string> exercises, array<string> criteria, string notes, string by)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_Qualification q = cat.FindQualification(qual);
		CGQC_GU_Progress p = FindProgress(m, qual);
		if (!p)
		{
			p = new CGQC_GU_Progress();
			p.qual = qual;
			m.progress.Insert(p);
		}

		map<string, string> kept = new map<string, string>();
		string today = CGQC_GU_Time.TodayEastern();
		foreach (string code : exercises)
		{
			CGQC_GU_Exercise e = cat.FindExercise(code);
			if (!e || e.qual != qual)
				continue;

			string since;
			if (!p.exercises.Find(code, since) || since.IsEmpty())
				since = today;

			kept.Set(code, since);
		}

		p.exercises = kept;

		map<string, bool> crit = new map<string, bool>();
		if (q)
		{
			foreach (CGQC_GU_LabelItem c : q.criteria)
			{
				if (criteria.Contains(c.id))
					crit.Set(c.id, true);
			}
		}

		p.criteria = crit;
		p.notes = notes;
		p.savedBy = by;
		p.savedAt = CGQC_GU_Time.NowUtc();
		return p;
	}

	//------------------------------------------------------------------------------------------------
	//! RÉUSSI / ÉCHEC on a qualification: result, Registre QUALIFICATION, the evaluator's student count,
	//! and on a pass the exercises passed, the news line and the automatic promotion. A pass clears the
	//! progress; a fail keeps it for the retake. sessionId is "" outside a session.
	static void RecordResult(CGQC_GU_Manager mgr, CGQC_GU_Member m, string qual, bool passed, string evaluator, string sessionId)
	{
		string rankBefore = m.rank;

		CGQC_GU_QualResult r = new CGQC_GU_QualResult();
		r.id = CGQC_GU_Ids.New("qr");
		r.qual = qual;
		r.result = RESULT_FAILED;
		if (passed)
			r.result = RESULT_PASSED;

		r.on = CGQC_GU_Time.NowUtc();
		r.evaluator = evaluator;
		r.sessionId = sessionId;
		m.qualifications.Insert(r);

		CGQC_GU_RecordEntry e = m.AddRecord("qualification", evaluator);
		e.data.Set("qual", qual);
		e.data.Set("result", r.result);
		if (!sessionId.IsEmpty())
			e.data.Set("sessionId", sessionId);

		CGQC_GU_Member ev = mgr.FindMember(evaluator);
		// One's own evaluation is not a student.
		if (ev && ev != m)
		{
			ev.stats.studentsEvaluated++;
			mgr.SaveMemberNow(ev);
		}

		if (!passed)
			return;

		// Test exercises ticked in the progress go into « Exercices réussis » (owner answer 1).
		CGQC_GU_Progress p = FindProgress(m, qual);
		if (p)
		{
			foreach (string code, string since : p.exercises)
			{
				AddExerciseDone(m, code, since, evaluator, sessionId);
			}

			m.progress.RemoveItem(p);
		}

		map<string, string> news = new map<string, string>();
		news.Set("qual", qual);
		news.Set("rank", rankBefore);
		mgr.AddNews("qualification", m.uid, news);

		CheckAutomaticPromotion(mgr, m, evaluator);
	}

	//------------------------------------------------------------------------------------------------
	static void AddExerciseDone(CGQC_GU_Member m, string code, string on, string instructor, string sessionId)
	{
		foreach (CGQC_GU_ExerciseDone done : m.exercises)
		{
			if (done.code == code && done.sessionId == sessionId && done.on == on)
				return;
		}

		CGQC_GU_ExerciseDone x = new CGQC_GU_ExerciseDone();
		x.code = code;
		x.on = on;
		x.instructor = instructor;
		x.sessionId = sessionId;
		m.exercises.Insert(x);
	}

	//------------------------------------------------------------------------------------------------
	//! Gives the Instructeur group on the first authorization (owner answer 3). Returns false if the
	//! member already teaches that qualification.
	static bool Authorize(CGQC_GU_Manager mgr, CGQC_GU_Member m, string qual, string by)
	{
		if (IsInstructorFor(m, qual))
			return false;

		CGQC_GU_InstructorAuth a = new CGQC_GU_InstructorAuth();
		a.qual = qual;
		a.authorizedBy = by;
		a.on = CGQC_GU_Time.TodayEastern();
		m.instructorFor.Insert(a);

		if (mgr.m_Catalogues.FindAccessGroup(INSTRUCTOR_GROUP) && !m.accessGroups.Contains(INSTRUCTOR_GROUP))
			m.accessGroups.Insert(INSTRUCTOR_GROUP);

		CGQC_GU_RecordEntry e = m.AddRecord("note", by);
		e.data.Set("instructorFor", qual);
		e.data.Set("action", "authorize");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The access group is left as it is (owner answer 3).
	static bool Revoke(CGQC_GU_Member m, string qual, string by)
	{
		for (int i = 0; i < m.instructorFor.Count(); i++)
		{
			if (m.instructorFor[i].qual != qual)
				continue;

			m.instructorFor.RemoveOrdered(i);
			CGQC_GU_RecordEntry e = m.AddRecord("note", by);
			e.data.Set("instructorFor", qual);
			e.data.Set("action", "revoke");
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Promotion engine (promotions.json)
	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Promotion NextPromotion(CGQC_GU_Catalogues cat, CGQC_GU_Member m)
	{
		foreach (CGQC_GU_Promotion p : cat.m_aPromotions)
		{
			if (p.from == m.rank)
				return p;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Qualifications, any-of groups and months in rank: everything the data can check.
	static bool MeetsMeasurable(CGQC_GU_Member m, CGQC_GU_Promotion p)
	{
		foreach (string code : p.requiredQuals)
		{
			if (!HasPassed(m, code))
				return false;
		}

		foreach (CGQC_GU_AnyOf any : p.anyOf)
		{
			int have;
			foreach (string q : any.quals)
			{
				if (HasPassed(m, q))
					have++;
			}

			if (have < any.count)
				return false;
		}

		if (p.minMonthsInRank > 0 && CGQC_GU_Time.MonthsSince(CGQC_GU_OverviewBuilder.MonthsInRankStart(m)) < p.minMonthsInRank)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! « Promotion possible »: everything measurable is met, and people still have to decide (officers'
	//! decision or another check). Automatic promotions never show it: they just happen.
	static bool PromotionPossible(CGQC_GU_Catalogues cat, CGQC_GU_Member m)
	{
		CGQC_GU_Promotion p = NextPromotion(cat, m);
		if (!p || p.automatic || !cat.FindRank(p.to))
			return false;

		return MeetsMeasurable(m, p);
	}

	//------------------------------------------------------------------------------------------------
	//! An automatic rule with nothing left for people to decide promotes at once, signed by the evaluator.
	static bool CheckAutomaticPromotion(CGQC_GU_Manager mgr, CGQC_GU_Member m, string by)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_Promotion p = NextPromotion(cat, m);
		if (!p || !p.automatic || p.officerDecision || !p.otherChecks.IsEmpty())
			return false;

		if (!cat.FindRank(p.to) || !MeetsMeasurable(m, p))
			return false;

		mgr.ChangeRank(m, p.to, by);
		CGQC_GU_Log.Info(string.Format("Promotion automatique : %1 -> %2", m.uid, p.to));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Students one instructor has evaluated for one qualification (every dossier is scanned).
	static int StudentsFor(CGQC_GU_Manager mgr, string instructorUid, string qual)
	{
		int n;
		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			foreach (CGQC_GU_QualResult r : m.qualifications)
			{
				if (r.evaluator == instructorUid && r.qual == qual && uid != instructorUid)
					n++;
			}
		}

		return n;
	}

	//------------------------------------------------------------------------------------------------
	static string PpLabel(int pp)
	{
		switch (pp)
		{
			case 1: return "PP1 — Infanterie";
			case 2: return "PP2 — Spécialistes";
			case 3: return "PP3 — Commandement";
		}

		return string.Format("PP%1", pp);
	}

	//------------------------------------------------------------------------------------------------
	static string ResultLabel(string result)
	{
		if (result == RESULT_PASSED)
			return "RÉUSSI";

		if (result == RESULT_FAILED)
			return "ÉCHEC";

		return "EN COURS";
	}
}
