// Gestion d'unité - catalogue data (config/*.json). Every rule lives in these files, never in code.

class CGQC_GU_LabelItem : Managed
{
	string id;
	string label;
}

//------------------------------------------------------------------------------------------------
// settings.json
class CGQC_GU_Settings : Managed
{
	int schemaVersion = 1;
	string discordWebhook;					// server-side only, never sent to clients
	string timeZone = "America/Toronto";	// UI shows Eastern time (HE)
	int inactiveAfterMissedOps = 3;
	int miaAfterMissedOps = 5;
	int autosaveSeconds = 300;
	bool allowDevUid;						// dev-only fake UID when the backend gives none (forced on outside a dedicated server)
	int devUidDelaySeconds = 10;			// wait this long for the real UID before using the dev fallback
	ref CGQC_GU_LongService longService = new CGQC_GU_LongService();
}

class CGQC_GU_LongService : Managed
{
	int firstAtYears = 1;
	int bronzeEvery = 1;
	int silverAt = 5;
	int goldAt = 10;
}

//------------------------------------------------------------------------------------------------
// ranks.json
class CGQC_GU_Rank : Managed
{
	string id;
	string abbr;
	string label;
	string category;
	int order;			// higher = more senior; used for sorting
	string insignia;
	bool honorary;		// set by hand only (Vétéran)
}

//------------------------------------------------------------------------------------------------
// roles.json
class CGQC_GU_Role : Managed
{
	string id;
	string label;
	string minRank;
}

//------------------------------------------------------------------------------------------------
// access_groups.json
class CGQC_GU_AccessGroup : Managed
{
	string id;
	string label;
	int level;		// higher = more authority; only the highest held is displayed
	ref array<string> rights = new array<string>();	// "*" = every right
}

//------------------------------------------------------------------------------------------------
// promotions.json
class CGQC_GU_AnyOf : Managed
{
	int count = 1;
	ref array<string> quals = new array<string>();
}

class CGQC_GU_Promotion : Managed
{
	string from;
	string to;
	ref array<string> requiredQuals = new array<string>();
	ref array<ref CGQC_GU_AnyOf> anyOf = new array<ref CGQC_GU_AnyOf>();
	int minMonthsInRank;
	bool officerDecision;
	bool automatic;
	ref array<ref CGQC_GU_LabelItem> otherChecks = new array<ref CGQC_GU_LabelItem>();	// e.g. vibe check
}

//------------------------------------------------------------------------------------------------
// qualifications.json
class CGQC_GU_Qualification : Managed
{
	string code;
	string title;
	int pp;					// 1, 2 or 3
	string levelRank;
	string group;			// e.g. QSP1 for QSP1-MED
	string state = "actif";	// actif | a_venir
	string description;
	ref array<string> exercises = new array<string>();
	ref array<ref CGQC_GU_LabelItem> criteria = new array<ref CGQC_GU_LabelItem>();
	ref array<string> onSuccess = new array<string>();
	string onFailure;
	string text;
}

//------------------------------------------------------------------------------------------------
// exercises.json
class CGQC_GU_Exercise : Managed
{
	string code;
	string title;
	string qual;
	bool required = true;
	string format;
	string place;
	string description;
	string text;
}

//------------------------------------------------------------------------------------------------
// distinctions.json
class CGQC_GU_Distinction : Managed
{
	string id;
	string label;
	string category;
	string kind;		// medal | bar | pennant
	ref array<string> ribbon = new array<string>();	// stripe colours, left to right
	string medal;
	string quote;
	string details;
	string proposer;	// tl | sl | officers | zeus_deployment | etat_major
	string approver;	// commandement | etat_major
	string repeat;		// leaf | deployment | plainbar | clasp | once | listed | collective
}

//------------------------------------------------------------------------------------------------
// units.json
class CGQC_GU_Seat : Managed
{
	string callsign;
	string role;
	string binome;		// white | red | green | blue | yellow | "" (Spartan and Typhon only)
}

class CGQC_GU_Element : Managed
{
	string id;
	string label;
	ref array<ref CGQC_GU_Seat> seats = new array<ref CGQC_GU_Seat>();
}

class CGQC_GU_Section : Managed
{
	string id;
	string label;
	string letter;
	string kind;			// e.g. "Hoplite, 1re section"
	bool activeByDefault;
	bool command;			// Zeus / high command: shown apart, not in the field chain
	ref array<ref CGQC_GU_Element> elements = new array<ref CGQC_GU_Element>();
}

class CGQC_GU_Unit : Managed
{
	string id;
	string label;
	string fullName;
	string patch;
	string state = "actif";	// actif | a_venir
	ref array<ref CGQC_GU_Section> sections = new array<ref CGQC_GU_Section>();
}
