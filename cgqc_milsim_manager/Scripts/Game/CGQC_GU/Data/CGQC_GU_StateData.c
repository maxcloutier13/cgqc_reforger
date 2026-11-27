// Gestion d'unité - live unit state (state/*.json).

class CGQC_GU_ChangeLogEntry : Managed
{
	string at;
	string by;
	string field;	// phase | qualification | unitStatus
	string from;
	string to;
}

//! state/situation.json. The phase goes back to temps_libre at every server start; the log is kept.
class CGQC_GU_Situation : Managed
{
	static const int SCHEMA_VERSION = 1;

	int schemaVersion = SCHEMA_VERSION;
	string phase = "temps_libre";	// temps_libre | entrainement | briefing | operation | aar
	bool qualification;				// only meaningful with entrainement
	string unitStatus = "garnison";	// garnison | deploye
	ref array<ref CGQC_GU_ChangeLogEntry> log = new array<ref CGQC_GU_ChangeLogEntry>();
}

//! state/notice.json
class CGQC_GU_Notice : Managed
{
	static const int SCHEMA_VERSION = 1;

	int schemaVersion = SCHEMA_VERSION;
	string text;
	string by;
	string at;
}

class CGQC_GU_NewsItem : Managed
{
	string id;
	string type;	// promotion | qualification | operation | deployment | distinction | enlistment
	string on;
	ref map<string, string> data = new map<string, string>();
}

//! state/news.json
class CGQC_GU_News : Managed
{
	static const int SCHEMA_VERSION = 1;

	int schemaVersion = SCHEMA_VERSION;
	ref array<ref CGQC_GU_NewsItem> items = new array<ref CGQC_GU_NewsItem>();
}

class CGQC_GU_SeatAssignment : Managed
{
	string uid;
	string unit;
	string seat;
	string by;
	string at;
	bool pending;	// true while the member is offline: applied on next connection
}

//! state/orbat_live.json (used from Phase 4)
class CGQC_GU_OrbatLive : Managed
{
	static const int SCHEMA_VERSION = 1;

	int schemaVersion = SCHEMA_VERSION;
	ref array<string> activeSections = new array<string>();
	ref map<string, string> renames = new map<string, string>();
	ref array<ref CGQC_GU_SeatAssignment> assignments = new array<ref CGQC_GU_SeatAssignment>();
}

//! A manual calendar event (Phase 2d): basic events only, such as the jour du Souvenir. Operations,
//! trainings and deployments come from their own files (Phases 3 and 5); anniversaries are computed.
class CGQC_GU_CalendarEvent : Managed
{
	string id;
	string title;
	string date;		// AAAA-MM-JJ, Eastern time
	string time;		// HH:MM HE, "" for all day
	string note;
	bool yearly;		// repeats every year on the same month and day
	string createdBy;
	string createdAt;
}

//! state/calendar.json
class CGQC_GU_Calendar : Managed
{
	static const int SCHEMA_VERSION = 1;

	int schemaVersion = SCHEMA_VERSION;
	ref array<ref CGQC_GU_CalendarEvent> events = new array<ref CGQC_GU_CalendarEvent>();
}
