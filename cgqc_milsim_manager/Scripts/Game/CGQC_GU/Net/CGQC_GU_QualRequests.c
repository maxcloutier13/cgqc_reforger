// Gestion d'unité - Phase 3 requests: qualification and exercise lists, instructors, progress and
// evaluation. Every write checks the caller here, on the server. Replies are display-ready.

class CGQC_GU_ExerciseDto : Managed
{
	string code;
	string title;
	string qual;
	bool required;		// REQUIS: used as a test of the qualification (owner answer 1)
	string format;
	string place;
	string description;
	string text;
}

class CGQC_GU_InstructorChip : Managed
{
	string uid;
	string name;		// "Capt. Roy"
	bool online;
	int students;		// students evaluated, every qualification
}

class CGQC_GU_QualDto : Managed
{
	string code;
	string title;
	int pp;
	string ppLabel;		// "PP1 — Infanterie"
	string levelAbbr;	// "Cdt."
	string levelLabel;	// "Cadet"
	string group;		// "QSP1" or ""
	bool aVenir;		// no content in the documentation yet
	string description;
	ref array<ref CGQC_GU_ExerciseDto> exercises = new array<ref CGQC_GU_ExerciseDto>();
	ref array<ref CGQC_GU_LabelItem> criteria = new array<ref CGQC_GU_LabelItem>();
	ref array<string> onSuccess = new array<string>();
	string onFailure;
	string text;
	ref array<ref CGQC_GU_InstructorChip> instructors = new array<ref CGQC_GU_InstructorChip>();
}

class CGQC_GU_InstructorRow : Managed
{
	string uid;
	string name;
	string rankAbbr;
	int rankOrder;
	bool online;
	int students;
	ref array<string> quals = new array<string>();
}

class CGQC_GU_QualRequests
{
	// Read (every member)
	static const string QUALS_CATALOGUE = "quals.catalogue";
	static const string EXERCISES_CATALOGUE = "exercises.catalogue";
	static const string INSTRUCTORS = "instructors";
	// Write
	static const string INSTRUCTOR_SET = "instructor.set";			// uid, qual (instructors.manage)
	static const string INSTRUCTOR_REMOVE = "instructor.remove";	// uid, qual (instructors.manage)
	static const string PROGRESS_SAVE = "progress.save";			// uid, qual, exercises, criteria (comma lists), notes
	static const string QUAL_EVALUATE = "qual.evaluate";			// uid, qual, result (passed|failed) [+ exercises, criteria, notes]

	static const int MAX_NOTES = 500;

	//------------------------------------------------------------------------------------------------
	//! null when the action is not one of ours.
	static CGQC_GU_RequestResult Handle(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string action, CGQC_GU_Payload p)
	{
		switch (action)
		{
			case QUALS_CATALOGUE:		return Ok(QualsCatalogue(mgr));
			case EXERCISES_CATALOGUE:	return Ok(ExercisesCatalogue(mgr));
			case INSTRUCTORS:			return Ok(Instructors(mgr));
			case INSTRUCTOR_SET:		return InstructorSet(mgr, caller, p, true);
			case INSTRUCTOR_REMOVE:		return InstructorSet(mgr, caller, p, false);
			case PROGRESS_SAVE:			return ProgressSave(mgr, caller, p);
			case QUAL_EVALUATE:			return QualEvaluate(mgr, caller, p);
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Ok(string payload)
	{
		return new CGQC_GU_RequestResult(true, "ok", payload);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Fail(string code)
	{
		return new CGQC_GU_RequestResult(false, code);
	}

	//------------------------------------------------------------------------------------------------
	// Reads
	//------------------------------------------------------------------------------------------------
	static CGQC_GU_ExerciseDto ExerciseDto(CGQC_GU_Exercise e)
	{
		CGQC_GU_ExerciseDto d = new CGQC_GU_ExerciseDto();
		d.code = e.code;
		d.title = e.title;
		d.qual = e.qual;
		d.required = e.required;
		d.format = e.format;
		d.place = e.place;
		d.description = e.description;
		d.text = e.text;
		return d;
	}

	//------------------------------------------------------------------------------------------------
	protected static string QualsCatalogue(CGQC_GU_Manager mgr)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		array<ref CGQC_GU_QualDto> list = new array<ref CGQC_GU_QualDto>();
		foreach (CGQC_GU_Qualification q : cat.m_aQualifications)
		{
			CGQC_GU_QualDto d = new CGQC_GU_QualDto();
			d.code = q.code;
			d.title = q.title;
			d.pp = q.pp;
			d.ppLabel = CGQC_GU_Quals.PpLabel(q.pp);
			CGQC_GU_Rank r = cat.FindRank(q.levelRank);
			if (r)
			{
				d.levelAbbr = r.abbr;
				d.levelLabel = r.label;
			}

			d.group = q.group;
			d.aVenir = q.state != "actif";
			d.description = q.description;

			// Exercises: the qualification's own list first, then any exercise that names it.
			foreach (string code : q.exercises)
			{
				CGQC_GU_Exercise e = cat.FindExercise(code);
				if (e)
					d.exercises.Insert(ExerciseDto(e));
			}

			foreach (CGQC_GU_Exercise ex : cat.m_aExercises)
			{
				if (ex.qual == q.code && !q.exercises.Contains(ex.code))
					d.exercises.Insert(ExerciseDto(ex));
			}

			foreach (CGQC_GU_LabelItem c : q.criteria)
			{
				d.criteria.Insert(c);
			}

			d.onSuccess.Copy(q.onSuccess);
			d.onFailure = q.onFailure;
			d.text = q.text;

			foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
			{
				if (!CGQC_GU_Quals.IsInstructorFor(m, q.code))
					continue;

				CGQC_GU_InstructorChip chip = new CGQC_GU_InstructorChip();
				chip.uid = m.uid;
				chip.name = CGQC_GU_OverviewBuilder.NameOf(cat, m);
				chip.online = mgr.IsOnline(m.uid);
				chip.students = m.stats.studentsEvaluated;
				InsertChip(cat, mgr, d.instructors, chip);
			}

			list.Insert(d);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("quals", list);
		ctx.WriteValue("ppNotes", PpNotes(cat));
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	//! The note under each PP group: what the promotions of that level ask for besides the qualifications
	//! (promotions.json). Automatic promotions with nothing else are left out. id = the PP number.
	protected static array<ref CGQC_GU_LabelItem> PpNotes(CGQC_GU_Catalogues cat)
	{
		array<ref CGQC_GU_LabelItem> notes = new array<ref CGQC_GU_LabelItem>();
		for (int pp = 1; pp <= 3; pp++)
		{
			array<string> lines = new array<string>();
			int officersOnly = 0;
			foreach (CGQC_GU_Promotion p : cat.m_aPromotions)
			{
				if (p.requiredQuals.IsEmpty())
					continue;

				CGQC_GU_Qualification first = cat.FindQualification(p.requiredQuals[0]);
				if (!first || first.pp != pp)
					continue;

				array<string> extras = new array<string>();
				if (p.minMonthsInRank > 0)
					extras.Insert(string.Format("%1 mois", p.minMonthsInRank));

				foreach (CGQC_GU_LabelItem check : p.otherChecks)
				{
					extras.Insert(CGQC_GU_Str.Lower(check.label));
				}

				if (p.officerDecision)
					extras.Insert("décision des officiers");

				if (extras.IsEmpty())
					continue;

				CGQC_GU_Rank from = cat.FindRank(p.from);
				CGQC_GU_Rank to = cat.FindRank(p.to);
				if (!from || !to)
					continue;

				string what;
				if (extras.Count() == 1 && p.officerDecision)
				{
					what = "décision des officiers";
					officersOnly++;
				}
				else
				{
					string last = extras[extras.Count() - 1];
					extras.Remove(extras.Count() - 1);
					if (extras.IsEmpty())
						what = "aussi " + last;
					else
						what = "aussi " + CGQC_GU_Str.Join(extras, ", ") + " et " + last;
				}

				lines.Insert(string.Format("%1 › %2 : %3.", from.label, to.label, what));
			}

			if (lines.IsEmpty())
				continue;

			CGQC_GU_LabelItem note = new CGQC_GU_LabelItem();
			note.id = pp.ToString();
			note.label = CGQC_GU_Str.Join(lines, " ");
			if (officersOnly == lines.Count())
			{
				if (officersOnly == 1)
					note.label = "Décision des officiers.";
				else
					note.label = "Chaque promotion : décision des officiers.";
			}

			notes.Insert(note);
		}

		return notes;
	}

	//------------------------------------------------------------------------------------------------
	//! Most students first.
	protected static void InsertChip(CGQC_GU_Catalogues cat, CGQC_GU_Manager mgr, array<ref CGQC_GU_InstructorChip> chips, CGQC_GU_InstructorChip chip)
	{
		int at = chips.Count();
		for (int i = 0; i < chips.Count(); i++)
		{
			if (chip.students > chips[i].students)
			{
				at = i;
				break;
			}
		}

		chips.InsertAt(chip, at);
	}

	//------------------------------------------------------------------------------------------------
	protected static string ExercisesCatalogue(CGQC_GU_Manager mgr)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		array<ref CGQC_GU_ExerciseDto> list = new array<ref CGQC_GU_ExerciseDto>();
		foreach (CGQC_GU_Exercise e : cat.m_aExercises)
		{
			list.Insert(ExerciseDto(e));
		}

		// Qualification filter: code + title, in catalogue order.
		array<ref CGQC_GU_LabelItem> quals = new array<ref CGQC_GU_LabelItem>();
		foreach (CGQC_GU_Qualification q : cat.m_aQualifications)
		{
			CGQC_GU_LabelItem item = new CGQC_GU_LabelItem();
			item.id = q.code;
			item.label = q.title;
			quals.Insert(item);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("exercises", list);
		ctx.WriteValue("quals", quals);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	//! « Instructeurs de l'unité »: everyone authorized for at least one qualification, most senior first.
	protected static string Instructors(CGQC_GU_Manager mgr)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		array<ref CGQC_GU_InstructorRow> rows = new array<ref CGQC_GU_InstructorRow>();
		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			if (m.instructorFor.IsEmpty())
				continue;

			CGQC_GU_InstructorRow row = new CGQC_GU_InstructorRow();
			row.uid = m.uid;
			row.name = CGQC_GU_OverviewBuilder.NameOf(cat, m);
			CGQC_GU_Rank r = cat.FindRank(m.rank);
			if (r)
			{
				row.rankAbbr = r.abbr;
				row.rankOrder = r.order;
			}

			row.online = mgr.IsOnline(m.uid);
			row.students = m.stats.studentsEvaluated;
			// Catalogue order, so the tags read PP1 first.
			foreach (CGQC_GU_Qualification q : cat.m_aQualifications)
			{
				if (CGQC_GU_Quals.IsInstructorFor(m, q.code))
					row.quals.Insert(q.code);
			}

			int at = rows.Count();
			for (int i = 0; i < rows.Count(); i++)
			{
				if (row.rankOrder > rows[i].rankOrder)
				{
					at = i;
					break;
				}
			}

			rows.InsertAt(row, at);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("instructors", rows);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	//! Replies with the target's fresh dossier.
	protected static CGQC_GU_RequestResult Saved(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Member m)
	{
		if (!mgr.SaveMemberNow(m))
			return Fail("write_failed");

		return Ok(CGQC_GU_DossierBuilder.Build(mgr, caller, m));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult InstructorSet(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p, bool authorize)
	{
		if (!CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, caller, CGQC_GU_Rights.INSTRUCTORS_MANAGE))
			return Fail("forbidden");

		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		string qual = p.Get("qual").Trim();
		if (!mgr.m_Catalogues.FindQualification(qual))
			return Fail("bad_qual");

		bool changed;
		if (authorize)
			changed = CGQC_GU_Quals.Authorize(mgr, m, qual, caller.uid);
		else
			changed = CGQC_GU_Quals.Revoke(m, qual, caller.uid);

		if (!changed)
			return Ok(CGQC_GU_DossierBuilder.Build(mgr, caller, m));

		return Saved(mgr, caller, m);
	}

	//------------------------------------------------------------------------------------------------
	//! Common checks for progress and evaluation; returns "" when allowed.
	protected static string CheckEvaluation(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Member m, string qual)
	{
		if (!m)
			return "not_found";

		if (!mgr.m_Catalogues.FindQualification(qual))
			return "bad_qual";

		if (m == caller && !CGQC_GU_Quals.IsCommand(caller))
			return "self_evaluation";

		if (!CGQC_GU_Quals.CanEvaluate(mgr, caller, m, qual))
			return "forbidden";

		if (CGQC_GU_Quals.HasPassed(m, qual))
			return "already_passed";

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult ProgressSave(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		string qual = p.Get("qual").Trim();
		string err = CheckEvaluation(mgr, caller, m, qual);
		if (!err.IsEmpty())
			return Fail(err);

		string notes = p.Get("notes").Trim();
		if (notes.Length() > MAX_NOTES)
			return Fail("too_long");

		array<string> exercises = new array<string>();
		p.GetList("exercises", exercises);
		array<string> criteria = new array<string>();
		p.GetList("criteria", criteria);
		CGQC_GU_Quals.SaveProgress(mgr, m, qual, exercises, criteria, notes, caller.uid);
		return Saved(mgr, caller, m);
	}

	//------------------------------------------------------------------------------------------------
	//! RÉUSSIR / ÉCHOUER from Progression. The panel's current ticks and notes are saved first, so what
	//! the instructor sees is what is recorded.
	protected static CGQC_GU_RequestResult QualEvaluate(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		string qual = p.Get("qual").Trim();
		string err = CheckEvaluation(mgr, caller, m, qual);
		if (!err.IsEmpty())
			return Fail(err);

		string result = CGQC_GU_Str.Lower(p.Get("result").Trim());
		if (result != CGQC_GU_Quals.RESULT_PASSED && result != CGQC_GU_Quals.RESULT_FAILED)
			return Fail("bad_result");

		if (p.Has("exercises") || p.Has("criteria") || p.Has("notes"))
		{
			string notes = p.Get("notes").Trim();
			if (notes.Length() > MAX_NOTES)
				return Fail("too_long");

			array<string> exercises = new array<string>();
			p.GetList("exercises", exercises);
			array<string> criteria = new array<string>();
			p.GetList("criteria", criteria);
			CGQC_GU_Quals.SaveProgress(mgr, m, qual, exercises, criteria, notes, caller.uid);
		}

		CGQC_GU_Quals.RecordResult(mgr, m, qual, result == CGQC_GU_Quals.RESULT_PASSED, caller.uid, string.Empty);
		CGQC_GU_Log.Info(string.Format("Évaluation %1 : %2 -> %3 par %4", qual, m.uid, result, caller.uid));
		return Saved(mgr, caller, m);
	}
}
