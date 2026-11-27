// Gestion d'unité - one training session or FTX: trainings/<id>.json (Phase 3b).
// Codes and IDs only; display text is built on the server when a screen asks for it.

class CGQC_GU_AttendanceRow : Managed
{
	string uid;
	bool present = true;
	string position;		// free text until the ORBAT (Phase 4): "Élève", "T1-1 · Fusilier"
	ref map<string, string> exercises = new map<string, string>();	// test exercise code -> passed | failed (qualification session only)
	string result;			// passed | failed | "" (qualification session only)
}

class CGQC_GU_Training : Managed
{
	static const int SCHEMA_VERSION = 1;

	static const string STATUS_DRAFT = "draft";
	static const string STATUS_PLANNED = "planned";
	static const string STATUS_DONE = "done";

	int schemaVersion = SCHEMA_VERSION;
	string id;
	string type = "training";	// training | ftx
	string title;
	string qual;				// linked qualification, "" = attendance only
	ref array<string> exercises = new array<string>();	// exercises covered (a test when a qualification is linked)
	string instructor;			// uid
	string place;
	string date;				// AAAA-MM-JJ, Eastern time as entered
	string time;				// HH:MM HE or ""
	string status = STATUS_DRAFT;
	ref array<ref CGQC_GU_AttendanceRow> attendance = new array<ref CGQC_GU_AttendanceRow>();
	string createdBy;
	string createdAt;
	string updatedBy;
	string updatedAt;
	string doneBy;
	string doneAt;

	//------------------------------------------------------------------------------------------------
	void WriteTo(JsonSaveContext ctx)
	{
		ctx.WriteValue("schemaVersion", schemaVersion);
		ctx.WriteValue("id", id);
		ctx.WriteValue("type", type);
		ctx.WriteValue("title", title);
		ctx.WriteValue("qual", qual);
		ctx.WriteValue("exercises", exercises);
		ctx.WriteValue("instructor", instructor);
		ctx.WriteValue("place", place);
		ctx.WriteValue("date", date);
		ctx.WriteValue("time", time);
		ctx.WriteValue("status", status);
		ctx.WriteValue("attendance", attendance);
		ctx.WriteValue("createdBy", createdBy);
		ctx.WriteValue("createdAt", createdAt);
		ctx.WriteValue("updatedBy", updatedBy);
		ctx.WriteValue("updatedAt", updatedAt);
		ctx.WriteValue("doneBy", doneBy);
		ctx.WriteValue("doneAt", doneAt);
	}

	//------------------------------------------------------------------------------------------------
	bool ReadFrom(JsonLoadContext ctx)
	{
		ctx.ReadValue("schemaVersion", schemaVersion);
		if (!ctx.ReadValue("id", id) || id.IsEmpty())
			return false;

		ctx.ReadValue("type", type);
		ctx.ReadValue("title", title);
		ctx.ReadValue("qual", qual);
		ctx.ReadValue("exercises", exercises);
		ctx.ReadValue("instructor", instructor);
		ctx.ReadValue("place", place);
		ctx.ReadValue("date", date);
		ctx.ReadValue("time", time);
		ctx.ReadValue("status", status);
		ctx.ReadValue("attendance", attendance);
		ctx.ReadValue("createdBy", createdBy);
		ctx.ReadValue("createdAt", createdAt);
		ctx.ReadValue("updatedBy", updatedBy);
		ctx.ReadValue("updatedAt", updatedAt);
		ctx.ReadValue("doneBy", doneBy);
		ctx.ReadValue("doneAt", doneAt);

		if (!exercises) exercises = new array<string>();
		if (!attendance) attendance = new array<ref CGQC_GU_AttendanceRow>();
		foreach (CGQC_GU_AttendanceRow row : attendance)
		{
			if (!row.exercises)
				row.exercises = new map<string, string>();
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	CGQC_GU_AttendanceRow FindRow(string memberUid)
	{
		foreach (CGQC_GU_AttendanceRow row : attendance)
		{
			if (row.uid == memberUid)
				return row;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Sort key for the « < > » order and lists: date then time.
	string SortKey()
	{
		return date + " " + time + " " + id;
	}
}
