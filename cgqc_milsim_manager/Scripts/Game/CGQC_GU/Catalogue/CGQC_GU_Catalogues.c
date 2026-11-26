// Gestion d'unité - catalogues loaded from $profile:CGQC_GestionUnite/config/*.json.
// A missing file is written once from CGQC_GU_DefaultCatalogues, then the file is the source of truth.
// A broken file is never overwritten: the defaults are used in memory and an error is logged.

class CGQC_GU_Catalogues
{
	static const int SCHEMA_VERSION = 1;

	ref CGQC_GU_Settings m_Settings;
	ref array<ref CGQC_GU_LabelItem> m_aRankCategories = new array<ref CGQC_GU_LabelItem>();
	ref array<ref CGQC_GU_Rank> m_aRanks = new array<ref CGQC_GU_Rank>();
	ref array<ref CGQC_GU_Role> m_aRoles = new array<ref CGQC_GU_Role>();
	ref array<ref CGQC_GU_AccessGroup> m_aAccessGroups = new array<ref CGQC_GU_AccessGroup>();
	ref array<ref CGQC_GU_Promotion> m_aPromotions = new array<ref CGQC_GU_Promotion>();
	ref array<ref CGQC_GU_Qualification> m_aQualifications = new array<ref CGQC_GU_Qualification>();
	ref array<ref CGQC_GU_Exercise> m_aExercises = new array<ref CGQC_GU_Exercise>();
	ref array<ref CGQC_GU_LabelItem> m_aDistinctionCategories = new array<ref CGQC_GU_LabelItem>();
	ref array<ref CGQC_GU_Distinction> m_aDistinctions = new array<ref CGQC_GU_Distinction>();
	ref array<ref CGQC_GU_Unit> m_aUnits = new array<ref CGQC_GU_Unit>();

	//------------------------------------------------------------------------------------------------
	void LoadAll(string configDir)
	{
		CGQC_GU_Json.EnsureDirectory(configDir);

		LoadSettings(configDir + "/settings.json");
		LoadRanks(configDir + "/ranks.json");
		LoadRoles(configDir + "/roles.json");
		LoadAccessGroups(configDir + "/access_groups.json");
		LoadPromotions(configDir + "/promotions.json");
		LoadQualifications(configDir + "/qualifications.json");
		LoadExercises(configDir + "/exercises.json");
		LoadDistinctions(configDir + "/distinctions.json");
		LoadUnits(configDir + "/units.json");

		CGQC_GU_Log.Info(string.Format("Catalogues : %1 grades, %2 rôles, %3 groupes d'accès, %4 promotions, %5 qualifications, %6 exercices, %7 distinctions, %8 unités.",
			m_aRanks.Count(), m_aRoles.Count(), m_aAccessGroups.Count(), m_aPromotions.Count(),
			m_aQualifications.Count(), m_aExercises.Count(), m_aDistinctions.Count(), m_aUnits.Count()));
	}

	//------------------------------------------------------------------------------------------------
	// Lookups
	//------------------------------------------------------------------------------------------------
	CGQC_GU_Rank FindRank(string id)
	{
		foreach (CGQC_GU_Rank r : m_aRanks)
		{
			if (r.id == id)
				return r;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Accepts an id ("cpl") or an abbreviation ("Cpl."), case-insensitive.
	CGQC_GU_Rank FindRankLoose(string text)
	{
		string t = CGQC_GU_Str.Lower(text);
		foreach (CGQC_GU_Rank r : m_aRanks)
		{
			if (CGQC_GU_Str.Lower(r.id) == t || CGQC_GU_Str.Lower(r.abbr) == t)
				return r;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	int GetRankOrder(string id)
	{
		CGQC_GU_Rank r = FindRank(id);
		if (!r)
			return -1;

		return r.order;
	}

	//------------------------------------------------------------------------------------------------
	CGQC_GU_AccessGroup FindAccessGroup(string id)
	{
		foreach (CGQC_GU_AccessGroup g : m_aAccessGroups)
		{
			if (g.id == id)
				return g;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! The group with the lowest level: what a new member gets (Membre).
	string GetDefaultAccessGroup()
	{
		CGQC_GU_AccessGroup best;
		foreach (CGQC_GU_AccessGroup g : m_aAccessGroups)
		{
			if (!best || g.level < best.level)
				best = g;
		}

		if (best)
			return best.id;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! The entry rank: lowest order among non-honorary ranks (Cadet).
	string GetEntryRank()
	{
		CGQC_GU_Rank best;
		foreach (CGQC_GU_Rank r : m_aRanks)
		{
			if (r.honorary)
				continue;

			if (!best || r.order < best.order)
				best = r;
		}

		if (best)
			return best.id;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	CGQC_GU_Qualification FindQualification(string code)
	{
		foreach (CGQC_GU_Qualification q : m_aQualifications)
		{
			if (q.code == code)
				return q;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	// Loaders
	//------------------------------------------------------------------------------------------------
	protected void LoadSettings(string path)
	{
		m_Settings = CGQC_GU_DefaultCatalogues.Settings();
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_PrettyJsonSaveContext save = new CGQC_GU_PrettyJsonSaveContext();
			save.WriteValue("schemaVersion", m_Settings.schemaVersion);
			save.WriteValue("discordWebhook", m_Settings.discordWebhook);
			save.WriteValue("timeZone", m_Settings.timeZone);
			save.WriteValue("inactiveAfterMissedOps", m_Settings.inactiveAfterMissedOps);
			save.WriteValue("miaAfterMissedOps", m_Settings.miaAfterMissedOps);
			save.WriteValue("autosaveSeconds", m_Settings.autosaveSeconds);
			save.WriteValue("allowDevUid", m_Settings.allowDevUid);
			save.WriteValue("devUidDelaySeconds", m_Settings.devUidDelaySeconds);
			save.WriteValue("longService", m_Settings.longService);
			CGQC_GU_Json.SafeSave(save, path);
			CGQC_GU_Log.Info("Fichier créé : " + path);
			return;
		}

		JsonLoadContext ctx = CGQC_GU_Json.Open(path);
		if (!ctx)
		{
			CGQC_GU_Log.Error("settings.json illisible ; valeurs par défaut utilisées en mémoire.");
			return;
		}

		ctx.ReadValue("schemaVersion", m_Settings.schemaVersion);
		ctx.ReadValue("discordWebhook", m_Settings.discordWebhook);
		ctx.ReadValue("timeZone", m_Settings.timeZone);
		ctx.ReadValue("inactiveAfterMissedOps", m_Settings.inactiveAfterMissedOps);
		ctx.ReadValue("miaAfterMissedOps", m_Settings.miaAfterMissedOps);
		ctx.ReadValue("autosaveSeconds", m_Settings.autosaveSeconds);
		ctx.ReadValue("allowDevUid", m_Settings.allowDevUid);
		ctx.ReadValue("devUidDelaySeconds", m_Settings.devUidDelaySeconds);
		ctx.ReadValue("longService", m_Settings.longService);
		if (!m_Settings.longService)
			m_Settings.longService = new CGQC_GU_LongService();
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadRanks(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.Ranks(m_aRankCategories, m_aRanks);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("categories", m_aRankCategories);
			save.WriteValue("ranks", m_aRanks);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("ranks", m_aRanks) && m_aRanks)
		{
			ctx.ReadValue("categories", m_aRankCategories);
			if (!m_aRankCategories)
				m_aRankCategories = new array<ref CGQC_GU_LabelItem>();

			return;
		}

		FallbackError(path);
		m_aRankCategories = new array<ref CGQC_GU_LabelItem>();
		m_aRanks = new array<ref CGQC_GU_Rank>();
		CGQC_GU_DefaultCatalogues.Ranks(m_aRankCategories, m_aRanks);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadRoles(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.Roles(m_aRoles);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("roles", m_aRoles);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("roles", m_aRoles) && m_aRoles)
			return;

		FallbackError(path);
		m_aRoles = new array<ref CGQC_GU_Role>();
		CGQC_GU_DefaultCatalogues.Roles(m_aRoles);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadAccessGroups(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.AccessGroups(m_aAccessGroups);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("accessGroups", m_aAccessGroups);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("accessGroups", m_aAccessGroups) && m_aAccessGroups)
			return;

		FallbackError(path);
		m_aAccessGroups = new array<ref CGQC_GU_AccessGroup>();
		CGQC_GU_DefaultCatalogues.AccessGroups(m_aAccessGroups);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadPromotions(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.Promotions(m_aPromotions);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("promotions", m_aPromotions);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("promotions", m_aPromotions) && m_aPromotions)
			return;

		FallbackError(path);
		m_aPromotions = new array<ref CGQC_GU_Promotion>();
		CGQC_GU_DefaultCatalogues.Promotions(m_aPromotions);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadQualifications(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.Qualifications(m_aQualifications);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("qualifications", m_aQualifications);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("qualifications", m_aQualifications) && m_aQualifications)
			return;

		FallbackError(path);
		m_aQualifications = new array<ref CGQC_GU_Qualification>();
		CGQC_GU_DefaultCatalogues.Qualifications(m_aQualifications);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadExercises(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.Exercises(m_aExercises);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("exercises", m_aExercises);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("exercises", m_aExercises) && m_aExercises)
			return;

		FallbackError(path);
		m_aExercises = new array<ref CGQC_GU_Exercise>();
		CGQC_GU_DefaultCatalogues.Exercises(m_aExercises);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadDistinctions(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.Distinctions(m_aDistinctionCategories, m_aDistinctions);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("categories", m_aDistinctionCategories);
			save.WriteValue("distinctions", m_aDistinctions);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("distinctions", m_aDistinctions) && m_aDistinctions)
		{
			ctx.ReadValue("categories", m_aDistinctionCategories);
			if (!m_aDistinctionCategories)
				m_aDistinctionCategories = new array<ref CGQC_GU_LabelItem>();

			return;
		}

		FallbackError(path);
		m_aDistinctionCategories = new array<ref CGQC_GU_LabelItem>();
		m_aDistinctions = new array<ref CGQC_GU_Distinction>();
		CGQC_GU_DefaultCatalogues.Distinctions(m_aDistinctionCategories, m_aDistinctions);
	}

	//------------------------------------------------------------------------------------------------
	protected void LoadUnits(string path)
	{
		if (!FileIO.FileExists(path))
		{
			CGQC_GU_DefaultCatalogues.Units(m_aUnits);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("units", m_aUnits);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("units", m_aUnits) && m_aUnits)
			return;

		FallbackError(path);
		m_aUnits = new array<ref CGQC_GU_Unit>();
		CGQC_GU_DefaultCatalogues.Units(m_aUnits);
	}

	//------------------------------------------------------------------------------------------------
	// Internals
	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_PrettyJsonSaveContext NewFile()
	{
		CGQC_GU_PrettyJsonSaveContext save = new CGQC_GU_PrettyJsonSaveContext();
		save.WriteValue("schemaVersion", SCHEMA_VERSION);
		return save;
	}

	//------------------------------------------------------------------------------------------------
	protected void WriteNew(JsonSaveContext save, string path)
	{
		if (CGQC_GU_Json.SafeSave(save, path))
			CGQC_GU_Log.Info("Fichier créé : " + path);
	}

	//------------------------------------------------------------------------------------------------
	protected JsonLoadContext OpenChecked(string path)
	{
		JsonLoadContext ctx = CGQC_GU_Json.Open(path);
		if (!ctx)
			return null;

		int version;
		ctx.ReadValue("schemaVersion", version);
		if (version > SCHEMA_VERSION)
			CGQC_GU_Log.Warn(string.Format("%1 : schemaVersion %2 plus récent que ce mod (%3).", path, version, SCHEMA_VERSION));

		// Migration hook: when SCHEMA_VERSION goes up, convert older catalogue files here.
		return ctx;
	}

	//------------------------------------------------------------------------------------------------
	protected void FallbackError(string path)
	{
		CGQC_GU_Log.Error(path + " illisible ; fichier conservé tel quel, valeurs par défaut utilisées en mémoire.");
	}
}
