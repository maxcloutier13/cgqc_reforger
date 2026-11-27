// Gestion d'unité - file layout and load/save of members and unit state. Server only.
// $profile:CGQC_GestionUnite/{config,members,operations,deployments,trainings,proposals,state}/

class CGQC_GU_Storage
{
	static const string ROOT = "$profile:CGQC_GestionUnite";

	//------------------------------------------------------------------------------------------------
	static string Dir(string sub)
	{
		return ROOT + "/" + sub;
	}

	//------------------------------------------------------------------------------------------------
	static void EnsureLayout()
	{
		CGQC_GU_Json.EnsureDirectory(ROOT);
		CGQC_GU_Json.EnsureDirectory(Dir("config"));
		CGQC_GU_Json.EnsureDirectory(Dir("members"));
		CGQC_GU_Json.EnsureDirectory(Dir("operations"));
		CGQC_GU_Json.EnsureDirectory(Dir("deployments"));
		CGQC_GU_Json.EnsureDirectory(Dir("trainings"));
		CGQC_GU_Json.EnsureDirectory(Dir("proposals"));
		CGQC_GU_Json.EnsureDirectory(Dir("state"));
		CGQC_GU_Json.EnsureDirectory(Dir("archive"));
	}

	//------------------------------------------------------------------------------------------------
	//! Copy of a dossier in archive/<uid>_<timestamp>.json before it is merged away.
	static bool ArchiveMember(CGQC_GU_Member m)
	{
		string stamp = CGQC_GU_Time.NowUtc();
		stamp.Replace(":", "-");
		CGQC_GU_PrettyJsonSaveContext ctx = new CGQC_GU_PrettyJsonSaveContext();
		m.WriteTo(ctx);
		return CGQC_GU_Json.SafeSave(ctx, Dir("archive") + "/" + CGQC_GU_Str.FileSafe(m.uid) + "_" + stamp + ".json");
	}

	//------------------------------------------------------------------------------------------------
	static void RemoveMemberFiles(string uid)
	{
		string path = MemberPath(uid);
		array<string> files = { path, path + ".bak", path + ".tmp" };
		foreach (string f : files)
		{
			if (FileIO.FileExists(f))
				FileIO.DeleteFile(f);
		}
	}

	//------------------------------------------------------------------------------------------------
	static string MemberPath(string uid)
	{
		return Dir("members") + "/" + CGQC_GU_Str.FileSafe(uid) + ".json";
	}

	//------------------------------------------------------------------------------------------------
	// Members
	//------------------------------------------------------------------------------------------------
	static bool SaveMember(CGQC_GU_Member m)
	{
		if (!m || m.uid.IsEmpty())
			return false;

		m.schemaVersion = CGQC_GU_Member.SCHEMA_VERSION;
		CGQC_GU_PrettyJsonSaveContext ctx = new CGQC_GU_PrettyJsonSaveContext();
		m.WriteTo(ctx);
		return CGQC_GU_Json.SafeSave(ctx, MemberPath(m.uid));
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Member LoadMemberFile(string path)
	{
		JsonLoadContext ctx = CGQC_GU_Json.Open(path);
		if (!ctx)
			return null;

		CGQC_GU_Member m = new CGQC_GU_Member();
		if (!m.ReadFrom(ctx))
		{
			CGQC_GU_Log.Error("Dossier sans uid, ignoré : " + path);
			return null;
		}

		if (m.schemaVersion < CGQC_GU_Member.SCHEMA_VERSION)
			MigrateMember(m);
		else if (m.schemaVersion > CGQC_GU_Member.SCHEMA_VERSION)
			CGQC_GU_Log.Warn(string.Format("%1 : schemaVersion %2 plus récent que ce mod.", path, m.schemaVersion));

		return m;
	}

	//------------------------------------------------------------------------------------------------
	//! Migration hook: one step per version, each step raises schemaVersion by one.
	protected static void MigrateMember(CGQC_GU_Member m)
	{
		int from = m.schemaVersion;
		// if (m.schemaVersion == 1) { ...convert to 2...; m.schemaVersion = 2; }
		CGQC_GU_Log.Info(string.Format("Dossier %1 migré : v%2 -> v%3", m.uid, from, CGQC_GU_Member.SCHEMA_VERSION));
		m.schemaVersion = CGQC_GU_Member.SCHEMA_VERSION;
	}

	//------------------------------------------------------------------------------------------------
	//! Loads every members/*.json (the .tmp/.bak copies are only used as fallbacks).
	static void LoadAllMembers(map<string, ref CGQC_GU_Member> into)
	{
		string dir = Dir("members");
		array<string> files = SCR_FileIOHelper.FindFiles(dir, ".json");
		if (!files)
			return;

		foreach (string file : files)
		{
			string path = file;
			if (!path.Contains("/") && !path.Contains("\\"))
				path = dir + "/" + file;

			if (!path.EndsWith(".json"))
				continue;

			CGQC_GU_Member m = LoadMemberFile(path);
			if (!m)
				continue;

			if (into.Contains(m.uid))
			{
				CGQC_GU_Log.Warn("UID en double, fichier ignoré : " + path);
				continue;
			}

			into.Insert(m.uid, m);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Unit state
	//------------------------------------------------------------------------------------------------
	static bool SaveSituation(CGQC_GU_Situation s)
	{
		CGQC_GU_PrettyJsonSaveContext ctx = new CGQC_GU_PrettyJsonSaveContext();
		ctx.WriteValue("schemaVersion", CGQC_GU_Situation.SCHEMA_VERSION);
		ctx.WriteValue("phase", s.phase);
		ctx.WriteValue("qualification", s.qualification);
		ctx.WriteValue("unitStatus", s.unitStatus);
		ctx.WriteValue("log", s.log);
		return CGQC_GU_Json.SafeSave(ctx, Dir("state") + "/situation.json");
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Situation LoadSituation()
	{
		CGQC_GU_Situation s = new CGQC_GU_Situation();
		JsonLoadContext ctx = CGQC_GU_Json.Open(Dir("state") + "/situation.json");
		if (!ctx)
			return s;

		ctx.ReadValue("phase", s.phase);
		ctx.ReadValue("qualification", s.qualification);
		ctx.ReadValue("unitStatus", s.unitStatus);
		ctx.ReadValue("log", s.log);
		if (!s.log)
			s.log = new array<ref CGQC_GU_ChangeLogEntry>();

		return s;
	}

	//------------------------------------------------------------------------------------------------
	static bool SaveNotice(CGQC_GU_Notice n)
	{
		CGQC_GU_PrettyJsonSaveContext ctx = new CGQC_GU_PrettyJsonSaveContext();
		ctx.WriteValue("schemaVersion", CGQC_GU_Notice.SCHEMA_VERSION);
		ctx.WriteValue("text", n.text);
		ctx.WriteValue("by", n.by);
		ctx.WriteValue("at", n.at);
		return CGQC_GU_Json.SafeSave(ctx, Dir("state") + "/notice.json");
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Notice LoadNotice()
	{
		CGQC_GU_Notice n = new CGQC_GU_Notice();
		JsonLoadContext ctx = CGQC_GU_Json.Open(Dir("state") + "/notice.json");
		if (!ctx)
			return n;

		ctx.ReadValue("text", n.text);
		ctx.ReadValue("by", n.by);
		ctx.ReadValue("at", n.at);
		return n;
	}

	//------------------------------------------------------------------------------------------------
	static bool SaveNews(CGQC_GU_News n)
	{
		CGQC_GU_PrettyJsonSaveContext ctx = new CGQC_GU_PrettyJsonSaveContext();
		ctx.WriteValue("schemaVersion", CGQC_GU_News.SCHEMA_VERSION);
		ctx.WriteValue("items", n.items);
		return CGQC_GU_Json.SafeSave(ctx, Dir("state") + "/news.json");
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_News LoadNews()
	{
		CGQC_GU_News n = new CGQC_GU_News();
		JsonLoadContext ctx = CGQC_GU_Json.Open(Dir("state") + "/news.json");
		if (!ctx)
			return n;

		ctx.ReadValue("items", n.items);
		if (!n.items)
			n.items = new array<ref CGQC_GU_NewsItem>();

		return n;
	}

	//------------------------------------------------------------------------------------------------
	static bool SaveOrbatLive(CGQC_GU_OrbatLive o)
	{
		CGQC_GU_PrettyJsonSaveContext ctx = new CGQC_GU_PrettyJsonSaveContext();
		ctx.WriteValue("schemaVersion", CGQC_GU_OrbatLive.SCHEMA_VERSION);
		ctx.WriteValue("activeSections", o.activeSections);
		ctx.WriteValue("renames", o.renames);
		ctx.WriteValue("assignments", o.assignments);
		return CGQC_GU_Json.SafeSave(ctx, Dir("state") + "/orbat_live.json");
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_OrbatLive LoadOrbatLive()
	{
		CGQC_GU_OrbatLive o = new CGQC_GU_OrbatLive();
		JsonLoadContext ctx = CGQC_GU_Json.Open(Dir("state") + "/orbat_live.json");
		if (!ctx)
			return null;

		ctx.ReadValue("activeSections", o.activeSections);
		ctx.ReadValue("renames", o.renames);
		ctx.ReadValue("assignments", o.assignments);
		if (!o.activeSections) o.activeSections = new array<string>();
		if (!o.renames) o.renames = new map<string, string>();
		if (!o.assignments) o.assignments = new array<ref CGQC_GU_SeatAssignment>();
		return o;
	}
}
