// Gestion d'unité - Événements › Entraînement (Phase 3b): training sessions and FTX.
// Every write checks the caller here. Nothing reaches the dossiers until « Terminer et ajouter aux
// dossiers » is pressed (owner answer 4).

class CGQC_GU_TrainingSummary : Managed
{
	string id;
	string type;
	string typeLabel;		// FTX / ENTRAÎNEMENT
	string title;
	string qual;
	string date;			// AAAA-MM-JJ
	string time;			// HH:MM HE or ""
	string status;
	string statusLabel;		// BROUILLON / PLANIFIÉE / TERMINÉE
	string instructorName;
	int present;
}

class CGQC_GU_TrainingRowDto : Managed
{
	string uid;
	string name;
	bool online;
	bool present;
	string position;
	bool isInstructor;
	ref map<string, string> exercises = new map<string, string>();	// code -> passed | failed
	string result;
}

class CGQC_GU_TrainingDto : Managed
{
	string id;
	string type;
	string typeLabel;
	string title;
	string qual;
	string qualTitle;
	ref array<ref CGQC_GU_LabelItem> exercises = new array<ref CGQC_GU_LabelItem>();	// id = code, label = title
	string instructor;
	string instructorName;
	string place;
	string date;
	string time;
	string status;
	string statusLabel;
	int present;
	int recorded;			// members written to their dossier (TERMINÉE)
	ref array<ref CGQC_GU_TrainingRowDto> attendance = new array<ref CGQC_GU_TrainingRowDto>();
	string doneLine;		// "Terminée le 261202 par Lt. Gagnon" or ""
	string prevId;
	string nextId;
	// Viewer (display only; every write is checked again)
	bool canEdit;
	bool canCreate;
	ref array<ref CGQC_GU_LabelItem> qualChoices = new array<ref CGQC_GU_LabelItem>();
	ref array<ref CGQC_GU_LabelItem> instructorChoices = new array<ref CGQC_GU_LabelItem>();
}

class CGQC_GU_TrainingRequests
{
	static const string LIST = "training.list";
	static const string GET = "training.get";			// id ("" = the next planned, else the newest)
	static const string SAVE = "training.save";			// id ("" = new), type, title, date, time, place, qual, exercises, instructor, att
	static const string STATUS = "training.status";		// id, status (draft | planned)
	static const string DELETE = "training.delete";		// id
	static const string FINISH = "training.finish";		// id [+ the same fields as save]

	static const int MAX_TITLE = 60;
	static const int MAX_PLACE = 60;
	static const int MAX_POSITION = 40;
	static const int MAX_ROWS = 60;

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_RequestResult Handle(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string action, CGQC_GU_Payload p)
	{
		switch (action)
		{
			case LIST:		return Ok(List(mgr, caller));
			case GET:		return Get(mgr, caller, p.Get("id"));
			case SAVE:		return Save(mgr, caller, p);
			case STATUS:	return SetStatus(mgr, caller, p);
			case DELETE:	return Delete(mgr, caller, p);
			case FINISH:	return Finish(mgr, caller, p);
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
	// Rights
	//------------------------------------------------------------------------------------------------
	//! BROUILLON is seen by its instructor, its author and Commandement; the rest by everyone.
	static bool CanSee(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Training t)
	{
		if (t.status != CGQC_GU_Training.STATUS_DRAFT)
			return true;

		if (t.instructor == viewer.uid || t.createdBy == viewer.uid)
			return true;

		return CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, viewer, CGQC_GU_Rights.INSTRUCTORS_MANAGE);
	}

	//------------------------------------------------------------------------------------------------
	//! Before TERMINÉE: its instructor or author (sessions.create), or Commandement.
	static bool CanEdit(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Training t)
	{
		if (t.status == CGQC_GU_Training.STATUS_DONE)
			return false;

		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		if (CGQC_GU_Permissions.HasRight(cat, viewer, CGQC_GU_Rights.INSTRUCTORS_MANAGE))
			return true;

		if (!CGQC_GU_Permissions.HasRight(cat, viewer, CGQC_GU_Rights.SESSIONS_CREATE))
			return false;

		return t.instructor == viewer.uid || t.createdBy == viewer.uid;
	}

	//------------------------------------------------------------------------------------------------
	// Labels
	//------------------------------------------------------------------------------------------------
	static string TypeLabel(string type)
	{
		if (type == "ftx")
			return "FTX";

		return "ENTRAÎNEMENT";
	}

	//------------------------------------------------------------------------------------------------
	static string StatusLabel(string status)
	{
		switch (status)
		{
			case CGQC_GU_Training.STATUS_PLANNED:	return "PLANIFIÉE";
			case CGQC_GU_Training.STATUS_DONE:		return "TERMINÉE";
		}

		return "BROUILLON";
	}

	//------------------------------------------------------------------------------------------------
	protected static int PresentCount(CGQC_GU_Training t)
	{
		int n;
		foreach (CGQC_GU_AttendanceRow row : t.attendance)
		{
			if (row.present)
				n++;
		}

		return n;
	}

	//------------------------------------------------------------------------------------------------
	// Reads
	//------------------------------------------------------------------------------------------------
	//! Visible sessions in date order (oldest first), for the « < > » browsing.
	protected static void Visible(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, notnull array<CGQC_GU_Training> outList)
	{
		foreach (string id, CGQC_GU_Training t : mgr.m_mTrainings)
		{
			if (!CanSee(mgr, viewer, t))
				continue;

			int at = outList.Count();
			for (int i = 0; i < outList.Count(); i++)
			{
				if (t.SortKey().Compare(outList[i].SortKey()) < 0)
				{
					at = i;
					break;
				}
			}

			outList.InsertAt(t, at);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string List(CGQC_GU_Manager mgr, CGQC_GU_Member viewer)
	{
		array<CGQC_GU_Training> visible = new array<CGQC_GU_Training>();
		Visible(mgr, viewer, visible);
		array<ref CGQC_GU_TrainingSummary> list = new array<ref CGQC_GU_TrainingSummary>();
		foreach (CGQC_GU_Training t : visible)
		{
			CGQC_GU_TrainingSummary s = new CGQC_GU_TrainingSummary();
			s.id = t.id;
			s.type = t.type;
			s.typeLabel = TypeLabel(t.type);
			s.title = t.title;
			s.qual = t.qual;
			s.date = t.date;
			s.time = t.time;
			s.status = t.status;
			s.statusLabel = StatusLabel(t.status);
			s.instructorName = CGQC_GU_DossierBuilder.SignerName(mgr, t.instructor);
			s.present = PresentCount(t);
			list.Insert(s);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("trainings", list);
		ctx.WriteValue("canCreate", CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, viewer, CGQC_GU_Rights.SESSIONS_CREATE));
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Get(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, string id)
	{
		array<CGQC_GU_Training> visible = new array<CGQC_GU_Training>();
		Visible(mgr, viewer, visible);

		CGQC_GU_Training t;
		if (!id.IsEmpty())
		{
			t = mgr.m_mTrainings.Get(id);
			if (!t || !CanSee(mgr, viewer, t))
				return Fail("not_found");
		}
		else
		{
			// The next planned session from today, else the newest one.
			string today = CGQC_GU_Time.TodayEastern();
			foreach (CGQC_GU_Training c : visible)
			{
				if (c.status == CGQC_GU_Training.STATUS_PLANNED && c.date >= today)
				{
					t = c;
					break;
				}
			}

			if (!t && !visible.IsEmpty())
				t = visible[visible.Count() - 1];
		}

		return Ok(Detail(mgr, viewer, t, visible));
	}

	//------------------------------------------------------------------------------------------------
	//! t may be null (no session yet): the reply then only carries the viewer's choices.
	static string Detail(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Training t, array<CGQC_GU_Training> visible = null)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_TrainingDto d = new CGQC_GU_TrainingDto();
		d.canCreate = CGQC_GU_Permissions.HasRight(cat, viewer, CGQC_GU_Rights.SESSIONS_CREATE);

		foreach (CGQC_GU_Qualification q : cat.m_aQualifications)
		{
			if (CGQC_GU_Quals.CanTeach(mgr, viewer, q.code))
				d.qualChoices.Insert(Label(q.code, q.title));
		}

		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			if (CGQC_GU_Permissions.HasRight(cat, m, CGQC_GU_Rights.SESSIONS_CREATE))
				d.instructorChoices.Insert(Label(m.uid, CGQC_GU_OverviewBuilder.NameOf(cat, m)));
		}

		if (t)
		{
			d.id = t.id;
			d.type = t.type;
			d.typeLabel = TypeLabel(t.type);
			d.title = t.title;
			d.qual = t.qual;
			CGQC_GU_Qualification lq = cat.FindQualification(t.qual);
			if (lq)
				d.qualTitle = lq.title;

			foreach (string code : t.exercises)
			{
				string title;
				CGQC_GU_Exercise e = cat.FindExercise(code);
				if (e)
					title = e.title;

				d.exercises.Insert(Label(code, title));
			}

			d.instructor = t.instructor;
			d.instructorName = CGQC_GU_DossierBuilder.SignerName(mgr, t.instructor);
			d.place = t.place;
			d.date = t.date;
			d.time = t.time;
			d.status = t.status;
			d.statusLabel = StatusLabel(t.status);
			d.present = PresentCount(t);
			if (t.status == CGQC_GU_Training.STATUS_DONE)
			{
				d.recorded = d.present;
				d.doneLine = string.Format("Terminée le %1 par %2", CGQC_GU_DossierBuilder.DisplayDate(t.doneAt), CGQC_GU_DossierBuilder.SignerName(mgr, t.doneBy));
			}

			foreach (CGQC_GU_AttendanceRow row : t.attendance)
			{
				CGQC_GU_TrainingRowDto r = new CGQC_GU_TrainingRowDto();
				r.uid = row.uid;
				CGQC_GU_Member rm = mgr.FindMember(row.uid);
				if (rm)
					r.name = CGQC_GU_OverviewBuilder.NameOf(cat, rm);
				else
					r.name = "Inconnu";

				r.online = mgr.IsOnline(row.uid);
				r.present = row.present;
				r.position = row.position;
				r.isInstructor = row.uid == t.instructor;
				foreach (string ek, string ev : row.exercises)
				{
					r.exercises.Set(ek, ev);
				}

				r.result = row.result;
				d.attendance.Insert(r);
			}

			d.canEdit = CanEdit(mgr, viewer, t);

			// A defaulted parameter must not be reassigned (Phase 2 gotcha): use a local list.
			array<CGQC_GU_Training> order = new array<CGQC_GU_Training>();
			if (visible)
				order.Copy(visible);
			else
				Visible(mgr, viewer, order);

			int idx = order.Find(t);
			if (idx > 0)
				d.prevId = order[idx - 1].id;

			if (idx >= 0 && idx < order.Count() - 1)
				d.nextId = order[idx + 1].id;
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("t", d);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_LabelItem Label(string id, string label)
	{
		CGQC_GU_LabelItem item = new CGQC_GU_LabelItem();
		item.id = id;
		item.label = label;
		return item;
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	//! UIDs from the screen; names are accepted too, for the #gu console.
	protected static CGQC_GU_Member Resolve(CGQC_GU_Manager mgr, string who)
	{
		CGQC_GU_Member m = mgr.FindMember(who);
		if (m)
			return m;

		return mgr.FindMemberLoose(who);
	}

	//------------------------------------------------------------------------------------------------
	//! passed / failed, with the short forms ok / ko (game chat cuts long console lines).
	protected static string NormResult(string v)
	{
		string s = CGQC_GU_Str.Lower(v.Trim());
		if (s == "ok" || s == CGQC_GU_Quals.RESULT_PASSED)
			return CGQC_GU_Quals.RESULT_PASSED;

		if (s == "ko" || s == CGQC_GU_Quals.RESULT_FAILED)
			return CGQC_GU_Quals.RESULT_FAILED;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Applies the form fields to t. Returns "" or an error code; t is left untouched on error.
	protected static string ApplyFields(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Training t, CGQC_GU_Payload p)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;

		string type = CGQC_GU_Str.Lower(p.Get("type").Trim());
		if (type.IsEmpty())
			type = t.type;

		if (type != "training" && type != "ftx")
			return "bad_type";

		string title = p.Get("title").Trim();
		if (title.IsEmpty() || title.Length() > MAX_TITLE)
			return "bad_title";

		string date = p.Get("date").Trim();
		if (date.Length() != 10 || !CGQC_GU_Validate.IsIsoDateOrEmpty(date))
			return "bad_date";

		if (date.Substring(8, 2).ToInt() > CGQC_GU_Time.DaysInMonth(date.Substring(0, 4).ToInt(), date.Substring(5, 2).ToInt()))
			return "bad_date";

		string time = p.Get("time").Trim();
		if (!CGQC_GU_Validate.IsTimeOrEmpty(time))
			return "bad_time";

		string place = p.Get("place").Trim();
		if (place.Length() > MAX_PLACE)
			return "too_long";

		string qual = p.Get("qual").Trim();
		if (!qual.IsEmpty() && !cat.FindQualification(qual))
			return "bad_qual";

		// Instructor: default the author; must hold sessions.create, and teach the linked qualification.
		string instructorUid = t.instructor;
		if (p.Has("instructor") && !p.Get("instructor").Trim().IsEmpty())
		{
			CGQC_GU_Member im = Resolve(mgr, p.Get("instructor").Trim());
			if (!im)
				return "bad_instructor";

			instructorUid = im.uid;
		}

		if (instructorUid.IsEmpty())
			instructorUid = caller.uid;

		CGQC_GU_Member instructor = mgr.FindMember(instructorUid);
		if (!instructor || !CGQC_GU_Permissions.HasRight(cat, instructor, CGQC_GU_Rights.SESSIONS_CREATE))
			return "bad_instructor";

		if (!qual.IsEmpty())
		{
			// Only someone who may teach it links a qualification, and the session's instructor too.
			if (qual != t.qual && !CGQC_GU_Quals.CanTeach(mgr, caller, qual))
				return "forbidden";

			if (!CGQC_GU_Quals.CanTeach(mgr, instructor, qual))
				return "instructor_not_authorized";
		}

		// Exercises covered: real codes; with a qualification, its own exercises only.
		array<string> exercises = new array<string>();
		p.GetList("exercises", exercises);
		foreach (string code : exercises)
		{
			CGQC_GU_Exercise e = cat.FindExercise(code);
			if (!e)
				return "bad_exercise";

			if (!qual.IsEmpty() && e.qual != qual)
				return "bad_exercise";
		}

		// Attendance: "uid|present|position|code:passed,code:failed|result;…"
		array<ref CGQC_GU_AttendanceRow> rows = new array<ref CGQC_GU_AttendanceRow>();
		array<string> lines = new array<string>();
		p.Get("att").Split(";", lines, true);
		if (lines.Count() > MAX_ROWS)
			return "too_many";

		foreach (string line : lines)
		{
			array<string> f = new array<string>();
			line.Split("|", f, false);
			if (f.IsEmpty() || f[0].Trim().IsEmpty())
				continue;

			CGQC_GU_Member rm = Resolve(mgr, f[0].Trim());
			if (!rm)
				return "bad_member";

			bool seen;
			foreach (CGQC_GU_AttendanceRow other : rows)
			{
				if (other.uid == rm.uid)
					seen = true;
			}

			if (seen)
				continue;

			CGQC_GU_AttendanceRow row = new CGQC_GU_AttendanceRow();
			row.uid = rm.uid;
			row.present = f.Count() < 2 || f[1].Trim() != "0";
			if (f.Count() >= 3)
				row.position = f[2].Trim();

			if (row.position.Length() > MAX_POSITION)
				return "too_long";

			// Grades exist only in a qualification session, and never for the instructor (no self-evaluation).
			bool graded = !qual.IsEmpty() && rm.uid != instructorUid;
			if (graded && f.Count() >= 4)
			{
				array<string> grades = new array<string>();
				f[3].Split(",", grades, true);
				foreach (string g : grades)
				{
					int colon = g.IndexOf(":");
					if (colon <= 0)
						continue;

					string code = g.Substring(0, colon).Trim();
					string value = NormResult(g.Substring(colon + 1, g.Length() - colon - 1));
					if (!exercises.Contains(code))
						continue;

					if (value == CGQC_GU_Quals.RESULT_PASSED || value == CGQC_GU_Quals.RESULT_FAILED)
						row.exercises.Set(code, value);
				}
			}

			if (graded && f.Count() >= 5)
			{
				string result = NormResult(f[4]);
				if (result == CGQC_GU_Quals.RESULT_PASSED || result == CGQC_GU_Quals.RESULT_FAILED)
					row.result = result;
			}

			rows.Insert(row);
		}

		t.type = type;
		t.title = title;
		t.date = date;
		t.time = time;
		t.place = place;
		t.qual = qual;
		t.instructor = instructorUid;
		t.exercises.Clear();
		t.exercises.Copy(exercises);
		t.attendance.Clear();
		foreach (CGQC_GU_AttendanceRow r : rows)
		{
			t.attendance.Insert(r);
		}

		t.updatedBy = caller.uid;
		t.updatedAt = CGQC_GU_Time.NowUtc();
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! ENREGISTRER LE BROUILLON: creates (id "") or updates a session before TERMINÉE.
	protected static CGQC_GU_RequestResult Save(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, caller, CGQC_GU_Rights.SESSIONS_CREATE))
			return Fail("forbidden");

		string id = p.Get("id").Trim();
		CGQC_GU_Training t;
		bool isNew = id.IsEmpty();
		if (isNew)
		{
			t = new CGQC_GU_Training();
			t.id = CGQC_GU_Ids.New("trn");
			t.createdBy = caller.uid;
			t.createdAt = CGQC_GU_Time.NowUtc();
		}
		else
		{
			t = mgr.m_mTrainings.Get(id);
			if (!t)
				return Fail("not_found");

			if (t.status == CGQC_GU_Training.STATUS_DONE)
				return Fail("already_done");

			if (!CanEdit(mgr, caller, t))
				return Fail("forbidden");
		}

		// Work on a copy so a refused form leaves the stored session as it was.
		CGQC_GU_Training work = Copy(t);
		string err = ApplyFields(mgr, caller, work, p);
		if (!err.IsEmpty())
			return Fail(err);

		if (!CGQC_GU_Storage.SaveTraining(work))
			return Fail("write_failed");

		mgr.m_mTrainings.Set(work.id, work);
		if (work.status == CGQC_GU_Training.STATUS_PLANNED)
			mgr.BroadcastLightState();

		CGQC_GU_Log.Info(string.Format("Séance %1 enregistrée par %2", work.id, caller.uid));
		return Ok(Detail(mgr, caller, work));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Training Copy(CGQC_GU_Training t)
	{
		CGQC_GU_JsonStringSaveContext save = new CGQC_GU_JsonStringSaveContext();
		t.WriteTo(save);
		JsonLoadContext load = new JsonLoadContext();
		CGQC_GU_Training c = new CGQC_GU_Training();
		if (!load.LoadFromString(save.Export()) || !c.ReadFrom(load))
			return t;

		return c;
	}

	//------------------------------------------------------------------------------------------------
	//! BROUILLON <-> PLANIFIÉE. TERMINÉE only comes from training.finish.
	protected static CGQC_GU_RequestResult SetStatus(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_Training t = mgr.m_mTrainings.Get(p.Get("id"));
		if (!t)
			return Fail("not_found");

		if (t.status == CGQC_GU_Training.STATUS_DONE)
			return Fail("already_done");

		if (!CanEdit(mgr, caller, t))
			return Fail("forbidden");

		string status = CGQC_GU_Str.Lower(p.Get("status").Trim());
		if (status != CGQC_GU_Training.STATUS_DRAFT && status != CGQC_GU_Training.STATUS_PLANNED)
			return Fail("bad_status");

		if (status != t.status)
		{
			t.status = status;
			t.updatedBy = caller.uid;
			t.updatedAt = CGQC_GU_Time.NowUtc();
			if (!CGQC_GU_Storage.SaveTraining(t))
				return Fail("write_failed");

			mgr.BroadcastLightState();
		}

		return Ok(Detail(mgr, caller, t));
	}

	//------------------------------------------------------------------------------------------------
	//! SUPPRIMER, only before TERMINÉE. A copy is kept in archive/.
	protected static CGQC_GU_RequestResult Delete(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_Training t = mgr.m_mTrainings.Get(p.Get("id"));
		if (!t)
			return Fail("not_found");

		if (t.status == CGQC_GU_Training.STATUS_DONE)
			return Fail("already_done");

		if (!CanEdit(mgr, caller, t))
			return Fail("forbidden");

		if (!CGQC_GU_Storage.DeleteTraining(t))
			return Fail("write_failed");

		string id = t.id;
		mgr.m_mTrainings.Remove(id);
		mgr.BroadcastLightState();
		CGQC_GU_Log.Info(string.Format("Séance %1 supprimée par %2", id, caller.uid));
		return Get(mgr, caller, string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	//! TERMINER ET AJOUTER AUX DOSSIERS. The form on screen is saved first (if sent), then each member
	//! ticked present gets a SÉANCE-FTX entry; in a qualification session the test exercises passed go
	//! into the progress and RÉUSSI / ÉCHOUÉ is recorded, signed by the session's instructor.
	protected static CGQC_GU_RequestResult Finish(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_Training t = mgr.m_mTrainings.Get(p.Get("id"));
		if (!t)
			return Fail("not_found");

		if (t.status == CGQC_GU_Training.STATUS_DONE)
			return Fail("already_done");

		if (!CanEdit(mgr, caller, t))
			return Fail("forbidden");

		CGQC_GU_Training work = Copy(t);
		if (p.Has("title"))
		{
			string err = ApplyFields(mgr, caller, work, p);
			if (!err.IsEmpty())
				return Fail(err);
		}

		CGQC_GU_Member instructor = mgr.FindMember(work.instructor);
		if (!instructor)
			return Fail("bad_instructor");

		if (!work.qual.IsEmpty() && !CGQC_GU_Quals.CanTeach(mgr, instructor, work.qual))
			return Fail("instructor_not_authorized");

		if (PresentCount(work) == 0)
			return Fail("no_attendance");

		// The session is closed first, so a second press can never write the dossiers twice.
		work.status = CGQC_GU_Training.STATUS_DONE;
		work.doneBy = caller.uid;
		work.doneAt = CGQC_GU_Time.NowUtc();
		if (!CGQC_GU_Storage.SaveTraining(work))
			return Fail("write_failed");

		mgr.m_mTrainings.Set(work.id, work);

		int written;
		foreach (CGQC_GU_AttendanceRow row : work.attendance)
		{
			if (!row.present)
				continue;

			CGQC_GU_Member m = mgr.FindMember(row.uid);
			if (!m)
				continue;

			CGQC_GU_RecordEntry e = m.AddRecord("session", work.instructor);
			e.data.Set("sessionId", work.id);
			e.data.Set("type", work.type);
			if (!work.qual.IsEmpty())
				e.data.Set("qual", work.qual);
			if (!row.position.IsEmpty())
				e.data.Set("position", row.position);

			m.stats.sessions++;

			bool graded = !work.qual.IsEmpty() && m.uid != work.instructor && !CGQC_GU_Quals.HasPassed(m, work.qual);
			if (graded)
			{
				array<string> passedEx = new array<string>();
				foreach (string code, string value : row.exercises)
				{
					if (value == CGQC_GU_Quals.RESULT_PASSED)
						passedEx.Insert(code);
				}

				CGQC_GU_Quals.MergeSessionExercises(m, work.qual, passedEx, work.instructor);
				if (row.result == CGQC_GU_Quals.RESULT_PASSED || row.result == CGQC_GU_Quals.RESULT_FAILED)
					CGQC_GU_Quals.RecordResult(mgr, m, work.qual, row.result == CGQC_GU_Quals.RESULT_PASSED, work.instructor, work.id);
			}

			mgr.SaveMemberNow(m);
			written++;
		}

		mgr.BroadcastLightState();
		CGQC_GU_Log.Info(string.Format("Séance %1 terminée par %2 : %3 dossier(s)", work.id, caller.uid, written));
		return Ok(Detail(mgr, caller, work));
	}
}
