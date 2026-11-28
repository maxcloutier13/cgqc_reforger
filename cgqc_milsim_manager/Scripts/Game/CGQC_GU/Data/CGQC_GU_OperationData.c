// Gestion d'unité - one operation order (OPORD): operations/<id>.json (Phase 5a).
// Codes and IDs only, except the free text the author types and the ORBAT du jour snapshot, which keeps
// ranks and names as they were that night (owner answer 6).
// Sections follow the gitbook « Template OPORD » and the mockup tabs (Mission sits with Situation).

class CGQC_GU_OpHeader : Managed
{
	string name;			// « Faucon Noir » (shown « OPÉRATION FAUCON NOIR »)
	string hq;				// QG émetteur
	string date;			// AAAA-MM-JJ, Eastern time as entered
	string briefingTime;	// HH:MM HE or ""
	string launchTime;		// HH:MM HE or ""
	string timeZone = "est";	// fixed: Heure de l'Est (owner answer 7)
	string area;			// Zone d'opérations
	string mapName;			// Carte / terrain
	string weather;			// Moment et météo
}

class CGQC_GU_OpSituation : Managed
{
	string enemy;
	string friendly;
	string attachments;
}

class CGQC_GU_OpElementTask : Managed
{
	string element;		// element code: qg | jtac | <sectionId> | <sectionId>/<elementId>
	string task;
}

class CGQC_GU_OpExecution : Managed
{
	string task;		// Intention du commandant: tâche, but, état final
	string purpose;
	string endState;
	string concept;
	ref array<ref CGQC_GU_OpElementTask> elementTasks = new array<ref CGQC_GU_OpElementTask>();
	string coordination;
}

class CGQC_GU_OpSupport : Managed
{
	string type;		// medical | resupply | fires | air | engineer | other
	string details;
}

class CGQC_GU_OpCallsign : Managed
{
	string callsign;	// free text: « Zeus », « Spartan »
	string element;		// element code
	string chief;		// uid
	string mhz;			// « 43.0 »
}

class CGQC_GU_OpPoint : Managed
{
	string name;
	string grid;		// « 064 112 »
	string note;
}

class CGQC_GU_OpPasswords : Managed
{
	string challenge;
	string password;
	string running;
	string nearRecognition;
}

class CGQC_GU_OpSignal : Managed
{
	string text;		// « Fumée verte »
	string meaning;		// « LZ sécurisée »
}

class CGQC_GU_OpPace : Managed
{
	string level;		// P | A | C | E
	string means;
	string switchWhen;
	string mhz;
}

class CGQC_GU_OpCommand : Managed
{
	ref array<string> succession = new array<string>();	// uids, 1er first
	ref array<ref CGQC_GU_OpCallsign> callsigns = new array<ref CGQC_GU_OpCallsign>();
	ref array<ref CGQC_GU_OpPoint> points = new array<ref CGQC_GU_OpPoint>();
	ref CGQC_GU_OpPasswords passwords = new CGQC_GU_OpPasswords();
	ref array<ref CGQC_GU_OpSignal> signals = new array<ref CGQC_GU_OpSignal>();
	ref array<ref CGQC_GU_OpPace> pace = new array<ref CGQC_GU_OpPace>();	// always P, A, C, E
}

//! Filled in 5d (« Terminer et ajouter aux dossiers »).
class CGQC_GU_OpParticipant : Managed
{
	string uid;
	string seat;		// « S1-1 »
	string role;		// seat role id or free text
	string team;		// « Spartan-1 »
}

//! ORBAT du jour (5d): copy of the active sections when the order ends.
class CGQC_GU_OpSnapshotSeat : Managed
{
	string callsign;
	string role;		// seat role id or free text
	string roleLabel;
	string binome;
	string uid;			// "" = free seat
	string holder;		// rank + name as at that moment
	ref array<string> tags = new array<string>();
}

class CGQC_GU_OpSnapshotSection : Managed
{
	string unit;
	string section;
	string label;		// rename applied
	ref array<ref CGQC_GU_OpSnapshotSeat> seats = new array<ref CGQC_GU_OpSnapshotSeat>();
}

class CGQC_GU_OpOrbatSnapshot : Managed
{
	string takenAt;		// UTC, "" = not taken yet
	string takenBy;
	ref array<ref CGQC_GU_OpSnapshotSection> sections = new array<ref CGQC_GU_OpSnapshotSection>();
}

class CGQC_GU_Operation : Managed
{
	static const int SCHEMA_VERSION = 1;

	static const string STATUS_DRAFT = "draft";
	static const string STATUS_PUBLISHED = "published";
	static const string STATUS_DONE = "done";

	static const string PACE_LEVELS = "PACE";

	int schemaVersion = SCHEMA_VERSION;
	string id;
	string number;			// N° d'ordre « 2026-014 », editable
	string status = STATUS_DRAFT;
	ref CGQC_GU_OpHeader header = new CGQC_GU_OpHeader();
	ref CGQC_GU_OpSituation situation = new CGQC_GU_OpSituation();
	string mission;
	ref CGQC_GU_OpExecution execution = new CGQC_GU_OpExecution();
	ref array<ref CGQC_GU_OpSupport> support = new array<ref CGQC_GU_OpSupport>();
	ref CGQC_GU_OpCommand command = new CGQC_GU_OpCommand();
	string deploymentId;	// 5f
	ref array<ref CGQC_GU_OpParticipant> participants = new array<ref CGQC_GU_OpParticipant>();	// 5d
	ref CGQC_GU_OpOrbatSnapshot orbatSnapshot = new CGQC_GU_OpOrbatSnapshot();	// 5d
	string createdBy;
	string createdAt;
	string updatedBy;
	string updatedAt;
	string publishedBy;
	string publishedAt;
	string doneBy;
	string doneAt;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_Operation()
	{
		EnsurePace();
	}

	//------------------------------------------------------------------------------------------------
	//! The PACE plan always has its four rows, in order.
	void EnsurePace()
	{
		if (!command)
			command = new CGQC_GU_OpCommand();

		if (!command.pace)
			command.pace = new array<ref CGQC_GU_OpPace>();

		array<ref CGQC_GU_OpPace> rows = new array<ref CGQC_GU_OpPace>();
		for (int i = 0; i < PACE_LEVELS.Length(); i++)
		{
			string level = PACE_LEVELS.Get(i);
			CGQC_GU_OpPace found;
			foreach (CGQC_GU_OpPace row : command.pace)
			{
				if (row && row.level == level)
				{
					found = row;
					break;
				}
			}

			if (!found)
			{
				found = new CGQC_GU_OpPace();
				found.level = level;
			}

			rows.Insert(found);
		}

		command.pace = rows;
	}

	//------------------------------------------------------------------------------------------------
	void WriteTo(JsonSaveContext ctx)
	{
		ctx.WriteValue("schemaVersion", schemaVersion);
		ctx.WriteValue("id", id);
		ctx.WriteValue("number", number);
		ctx.WriteValue("status", status);
		ctx.WriteValue("header", header);
		ctx.WriteValue("situation", situation);
		ctx.WriteValue("mission", mission);
		ctx.WriteValue("execution", execution);
		ctx.WriteValue("support", support);
		ctx.WriteValue("command", command);
		ctx.WriteValue("deploymentId", deploymentId);
		ctx.WriteValue("participants", participants);
		ctx.WriteValue("orbatSnapshot", orbatSnapshot);
		ctx.WriteValue("createdBy", createdBy);
		ctx.WriteValue("createdAt", createdAt);
		ctx.WriteValue("updatedBy", updatedBy);
		ctx.WriteValue("updatedAt", updatedAt);
		ctx.WriteValue("publishedBy", publishedBy);
		ctx.WriteValue("publishedAt", publishedAt);
		ctx.WriteValue("doneBy", doneBy);
		ctx.WriteValue("doneAt", doneAt);
	}

	//------------------------------------------------------------------------------------------------
	bool ReadFrom(JsonLoadContext ctx)
	{
		ctx.ReadValue("schemaVersion", schemaVersion);
		if (!ctx.ReadValue("id", id) || id.IsEmpty())
			return false;

		ctx.ReadValue("number", number);
		ctx.ReadValue("status", status);
		ctx.ReadValue("header", header);
		ctx.ReadValue("situation", situation);
		ctx.ReadValue("mission", mission);
		ctx.ReadValue("execution", execution);
		ctx.ReadValue("support", support);
		ctx.ReadValue("command", command);
		ctx.ReadValue("deploymentId", deploymentId);
		ctx.ReadValue("participants", participants);
		ctx.ReadValue("orbatSnapshot", orbatSnapshot);
		ctx.ReadValue("createdBy", createdBy);
		ctx.ReadValue("createdAt", createdAt);
		ctx.ReadValue("updatedBy", updatedBy);
		ctx.ReadValue("updatedAt", updatedAt);
		ctx.ReadValue("publishedBy", publishedBy);
		ctx.ReadValue("publishedAt", publishedAt);
		ctx.ReadValue("doneBy", doneBy);
		ctx.ReadValue("doneAt", doneAt);

		Repair();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! A hand-edited file may miss whole blocks: put empty ones back.
	void Repair()
	{
		if (!header) header = new CGQC_GU_OpHeader();
		if (header.timeZone.IsEmpty()) header.timeZone = "est";
		if (!situation) situation = new CGQC_GU_OpSituation();
		if (!execution) execution = new CGQC_GU_OpExecution();
		if (!execution.elementTasks) execution.elementTasks = new array<ref CGQC_GU_OpElementTask>();
		if (!support) support = new array<ref CGQC_GU_OpSupport>();
		if (!command) command = new CGQC_GU_OpCommand();
		if (!command.succession) command.succession = new array<string>();
		if (!command.callsigns) command.callsigns = new array<ref CGQC_GU_OpCallsign>();
		if (!command.points) command.points = new array<ref CGQC_GU_OpPoint>();
		if (!command.passwords) command.passwords = new CGQC_GU_OpPasswords();
		if (!command.signals) command.signals = new array<ref CGQC_GU_OpSignal>();
		if (!participants) participants = new array<ref CGQC_GU_OpParticipant>();
		if (!orbatSnapshot) orbatSnapshot = new CGQC_GU_OpOrbatSnapshot();
		if (!orbatSnapshot.sections) orbatSnapshot.sections = new array<ref CGQC_GU_OpSnapshotSection>();
		foreach (CGQC_GU_OpSnapshotSection s : orbatSnapshot.sections)
		{
			if (!s.seats)
				s.seats = new array<ref CGQC_GU_OpSnapshotSeat>();

			foreach (CGQC_GU_OpSnapshotSeat seat : s.seats)
			{
				if (!seat.tags)
					seat.tags = new array<string>();
			}
		}

		EnsurePace();
	}

	//------------------------------------------------------------------------------------------------
	//! Sort key for the « < > » order and lists: date then launch time.
	string SortKey()
	{
		return header.date + " " + header.launchTime + " " + id;
	}
}
