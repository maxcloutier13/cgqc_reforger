// Gestion d'unité - member record (members/<bohemiaUid>.json). Layout: Docs/02-Schema-Donnees.md.
// Nested classes are Managed with public fields, serialized automatically by the JSON contexts.
// No display text is stored: codes and IDs only, labels come from the catalogues.

class CGQC_GU_MemberStatus : Managed
{
	string computed = "actif";	// actif | inactif | mia (from missed OPORD operations)
	string manual;				// "" | actif | inactif | mia | kia (Commandement override)
	string manualBy;
	string manualAt;
}

class CGQC_GU_Personal : Managed
{
	string birthDate;
	string birthPlace;
	string bloodType;
	string bio;
}

class CGQC_GU_InstructorAuth : Managed
{
	string qual;
	string authorizedBy;
	string on;
}

class CGQC_GU_QualResult : Managed
{
	string id;
	string qual;
	string result;		// passed | failed | in_progress
	string on;
	string evaluator;
	string sessionId;
}

class CGQC_GU_Progress : Managed
{
	string qual;
	ref map<string, string> exercises = new map<string, string>();	// exercise code -> date
	ref map<string, bool> criteria = new map<string, bool>();		// criterion id -> met
	string notes;
	string savedBy;
	string savedAt;
}

class CGQC_GU_ExerciseDone : Managed
{
	string code;
	string on;
	string instructor;
	string sessionId;
}

class CGQC_GU_Award : Managed
{
	string id;
	string distinction;
	string on;
	string deploymentId;
	string reason;
	string approvedBy;
}

class CGQC_GU_RecordEntry : Managed
{
	string id;
	string type;	// enlistment | promotion | operation | deployment | session | qualification | distinction | task | note
	string on;
	string by;		// uid of the signer, or "systeme" / "console"
	ref map<string, string> data = new map<string, string>();	// codes/IDs only, e.g. { "rank": "cpl" }
	string note;
}

class CGQC_GU_MemberStats : Managed
{
	int operations;
	int deployments;
	int sessions;
	int studentsEvaluated;
	int missedOps;
	string lastOpId;
}

//! One member file. Top-level keys are written one by one so the file root stays flat.
class CGQC_GU_Member : Managed
{
	static const int SCHEMA_VERSION = 1;

	int schemaVersion = SCHEMA_VERSION;
	string uid;
	bool uidLinked;
	string profileName;
	string lastName;
	string displayName;
	string rank = "cdt";
	ref array<string> accessGroups = new array<string>();
	ref CGQC_GU_MemberStatus status = new CGQC_GU_MemberStatus();
	string enlistedOn;
	string createdAt;
	string lastSeenAt;
	ref CGQC_GU_Personal personal = new CGQC_GU_Personal();
	ref array<ref CGQC_GU_InstructorAuth> instructorFor = new array<ref CGQC_GU_InstructorAuth>();
	ref array<ref CGQC_GU_QualResult> qualifications = new array<ref CGQC_GU_QualResult>();
	ref array<ref CGQC_GU_Progress> progress = new array<ref CGQC_GU_Progress>();
	ref array<ref CGQC_GU_ExerciseDone> exercises = new array<ref CGQC_GU_ExerciseDone>();
	ref array<ref CGQC_GU_Award> distinctions = new array<ref CGQC_GU_Award>();
	ref array<ref CGQC_GU_RecordEntry> record = new array<ref CGQC_GU_RecordEntry>();
	ref CGQC_GU_MemberStats stats = new CGQC_GU_MemberStats();

	//------------------------------------------------------------------------------------------------
	void WriteTo(JsonSaveContext ctx)
	{
		ctx.WriteValue("schemaVersion", schemaVersion);
		ctx.WriteValue("uid", uid);
		ctx.WriteValue("uidLinked", uidLinked);
		ctx.WriteValue("profileName", profileName);
		ctx.WriteValue("lastName", lastName);
		ctx.WriteValue("displayName", displayName);
		ctx.WriteValue("rank", rank);
		ctx.WriteValue("accessGroups", accessGroups);
		ctx.WriteValue("status", status);
		ctx.WriteValue("enlistedOn", enlistedOn);
		ctx.WriteValue("createdAt", createdAt);
		ctx.WriteValue("lastSeenAt", lastSeenAt);
		ctx.WriteValue("personal", personal);
		ctx.WriteValue("instructorFor", instructorFor);
		ctx.WriteValue("qualifications", qualifications);
		ctx.WriteValue("progress", progress);
		ctx.WriteValue("exercises", exercises);
		ctx.WriteValue("distinctions", distinctions);
		ctx.WriteValue("record", record);
		ctx.WriteValue("stats", stats);
	}

	//------------------------------------------------------------------------------------------------
	//! Missing keys keep their defaults; only "uid" is mandatory.
	bool ReadFrom(JsonLoadContext ctx)
	{
		ctx.ReadValue("schemaVersion", schemaVersion);
		if (!ctx.ReadValue("uid", uid) || uid.IsEmpty())
			return false;

		ctx.ReadValue("uidLinked", uidLinked);
		ctx.ReadValue("profileName", profileName);
		ctx.ReadValue("lastName", lastName);
		ctx.ReadValue("displayName", displayName);
		ctx.ReadValue("rank", rank);
		ctx.ReadValue("accessGroups", accessGroups);
		ctx.ReadValue("status", status);
		ctx.ReadValue("enlistedOn", enlistedOn);
		ctx.ReadValue("createdAt", createdAt);
		ctx.ReadValue("lastSeenAt", lastSeenAt);
		ctx.ReadValue("personal", personal);
		ctx.ReadValue("instructorFor", instructorFor);
		ctx.ReadValue("qualifications", qualifications);
		ctx.ReadValue("progress", progress);
		ctx.ReadValue("exercises", exercises);
		ctx.ReadValue("distinctions", distinctions);
		ctx.ReadValue("record", record);
		ctx.ReadValue("stats", stats);

		// A key that is present but null must not leave a null container behind.
		if (!accessGroups) accessGroups = new array<string>();
		if (!status) status = new CGQC_GU_MemberStatus();
		if (!personal) personal = new CGQC_GU_Personal();
		if (!instructorFor) instructorFor = new array<ref CGQC_GU_InstructorAuth>();
		if (!qualifications) qualifications = new array<ref CGQC_GU_QualResult>();
		if (!progress) progress = new array<ref CGQC_GU_Progress>();
		if (!exercises) exercises = new array<ref CGQC_GU_ExerciseDone>();
		if (!distinctions) distinctions = new array<ref CGQC_GU_Award>();
		if (!record) record = new array<ref CGQC_GU_RecordEntry>();
		if (!stats) stats = new CGQC_GU_MemberStats();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	CGQC_GU_RecordEntry AddRecord(string type, string by, string note = "")
	{
		CGQC_GU_RecordEntry e = new CGQC_GU_RecordEntry();
		e.id = CGQC_GU_Ids.New("rec");
		e.type = type;
		e.on = CGQC_GU_Time.NowUtc();
		e.by = by;
		e.note = note;
		record.Insert(e);
		return e;
	}

	//------------------------------------------------------------------------------------------------
	//! Effective status: the manual override wins over the computed one.
	string GetEffectiveStatus()
	{
		if (status && !status.manual.IsEmpty())
			return status.manual;

		if (status)
			return status.computed;

		return "actif";
	}
}
