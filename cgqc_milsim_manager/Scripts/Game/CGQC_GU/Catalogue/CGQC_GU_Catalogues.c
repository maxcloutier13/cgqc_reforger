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
	ref array<ref CGQC_GU_SeatRole> m_aSeatRoles = new array<ref CGQC_GU_SeatRole>();

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
		FillQualContent(configDir);
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
	CGQC_GU_Exercise FindExercise(string code)
	{
		foreach (CGQC_GU_Exercise e : m_aExercises)
		{
			if (e.code == code)
				return e;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Exercises of one qualification, in file order.
	void ExercisesOf(string qual, notnull array<CGQC_GU_Exercise> outList)
	{
		foreach (CGQC_GU_Exercise e : m_aExercises)
		{
			if (e.qual == qual)
				outList.Insert(e);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Phase 3: gitbook content added to the defaults after the live files were written. Only EMPTY
	//! fields of entries that still exist are filled; anything the owner wrote is kept as it is.
	protected void FillQualContent(string configDir)
	{
		array<ref CGQC_GU_Qualification> defQuals = new array<ref CGQC_GU_Qualification>();
		CGQC_GU_DefaultCatalogues.Qualifications(defQuals);
		int qualFills;
		foreach (CGQC_GU_Qualification d : defQuals)
		{
			CGQC_GU_Qualification q = FindQualification(d.code);
			if (!q)
				continue;

			if (q.description.IsEmpty() && !d.description.IsEmpty()) { q.description = d.description; qualFills++; }
			if (q.text.IsEmpty() && !d.text.IsEmpty()) { q.text = d.text; qualFills++; }
			if (q.onFailure.IsEmpty() && !d.onFailure.IsEmpty()) { q.onFailure = d.onFailure; qualFills++; }
			if (q.onSuccess.IsEmpty() && !d.onSuccess.IsEmpty()) { q.onSuccess.Copy(d.onSuccess); qualFills++; }
			if (q.exercises.IsEmpty() && !d.exercises.IsEmpty()) { q.exercises.Copy(d.exercises); qualFills++; }
			if (q.criteria.IsEmpty() && !d.criteria.IsEmpty())
			{
				foreach (CGQC_GU_LabelItem c : d.criteria)
				{
					q.criteria.Insert(c);
				}

				qualFills++;
			}
		}

		array<ref CGQC_GU_Exercise> defEx = new array<ref CGQC_GU_Exercise>();
		CGQC_GU_DefaultCatalogues.Exercises(defEx);
		int exFills;
		foreach (CGQC_GU_Exercise de : defEx)
		{
			CGQC_GU_Exercise e = FindExercise(de.code);
			if (!e)
				continue;

			if (e.description.IsEmpty() && !de.description.IsEmpty()) { e.description = de.description; exFills++; }
			if (e.text.IsEmpty() && !de.text.IsEmpty()) { e.text = de.text; exFills++; }
			if (e.format.IsEmpty() && !de.format.IsEmpty()) { e.format = de.format; exFills++; }
			if (e.place.IsEmpty() && !de.place.IsEmpty()) { e.place = de.place; exFills++; }
		}

		if (qualFills > 0)
		{
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("qualifications", m_aQualifications);
			if (CGQC_GU_Json.SafeSave(save, configDir + "/qualifications.json"))
				CGQC_GU_Log.Info(string.Format("qualifications.json : %1 champ(s) vide(s) complété(s) depuis la documentation.", qualFills));
		}

		if (exFills > 0)
		{
			CGQC_GU_PrettyJsonSaveContext saveEx = NewFile();
			saveEx.WriteValue("exercises", m_aExercises);
			if (CGQC_GU_Json.SafeSave(saveEx, configDir + "/exercises.json"))
				CGQC_GU_Log.Info(string.Format("exercises.json : %1 champ(s) vide(s) complété(s) depuis la documentation.", exFills));
		}
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
			CGQC_GU_DefaultCatalogues.SeatRoles(m_aSeatRoles);
			CGQC_GU_DefaultCatalogues.Units(m_aUnits);
			CGQC_GU_PrettyJsonSaveContext save = NewFile();
			save.WriteValue("seatRoles", m_aSeatRoles);
			save.WriteValue("units", m_aUnits);
			WriteNew(save, path);
			return;
		}

		JsonLoadContext ctx = OpenChecked(path);
		if (ctx && ctx.ReadValue("units", m_aUnits) && m_aUnits)
		{
			ctx.ReadValue("seatRoles", m_aSeatRoles);
			if (!m_aSeatRoles)
				m_aSeatRoles = new array<ref CGQC_GU_SeatRole>();

			UpgradeUnits(path);
			return;
		}

		FallbackError(path);
		m_aSeatRoles = new array<ref CGQC_GU_SeatRole>();
		m_aUnits = new array<ref CGQC_GU_Unit>();
		CGQC_GU_DefaultCatalogues.SeatRoles(m_aSeatRoles);
		CGQC_GU_DefaultCatalogues.Units(m_aUnits);
	}

	//------------------------------------------------------------------------------------------------
	//! Phase 4a: a units.json written before 4a gets the mockup wording, the seat roles with their
	//! minimum rank and the faction keys. Only values that still hold the old defaults are changed;
	//! anything the owner wrote is kept. Each change is counted and logged.
	protected void UpgradeUnits(string path)
	{
		int changes;
		if (m_aSeatRoles.IsEmpty())
		{
			CGQC_GU_DefaultCatalogues.SeatRoles(m_aSeatRoles);
			changes += m_aSeatRoles.Count();
		}

		array<ref CGQC_GU_Unit> defs = new array<ref CGQC_GU_Unit>();
		CGQC_GU_DefaultCatalogues.Units(defs);
		foreach (CGQC_GU_Unit du : defs)
		{
			CGQC_GU_Unit u = FindUnit(du.id);
			if (!u)
				continue;

			// No faction key yet = a file from before 4a: the new unit fields come from the defaults.
			if (u.faction.IsEmpty() && !du.faction.IsEmpty())
			{
				u.faction = du.faction;
				u.noRankCheck = du.noRankCheck;
				changes++;
			}

			foreach (CGQC_GU_Section ds : du.sections)
			{
				CGQC_GU_Section s = FindSectionIn(u, ds.id);
				if (!s)
					continue;

				if (s.kind != ds.kind && s.kind == CGQC_GU_DefaultCatalogues.LegacyKind(ds.id))
				{
					s.kind = ds.kind;
					changes++;
				}

				// 4e: UPO seats become live seats (I-0 / I-1, E-1 to E-16), if the old default layout is
				// still there (no elements, or the single « I » element with I-1 to I-4).
				bool oldLayout = s.elements.IsEmpty() || (s.elements.Count() == 1 && s.elements[0].id == ds.letter);
				if (ds.liveSeats && !s.liveSeats && oldLayout)
				{
					s.liveSeats = true;
					s.seatRole = ds.seatRole;
					s.defaultSeats = ds.defaultSeats;
					s.firstNumber = ds.firstNumber;
					s.elements.Clear();
					changes++;
				}

				foreach (CGQC_GU_Element de : ds.elements)
				{
					CGQC_GU_Element e = FindElementIn(s, de.id);
					if (!e)
						continue;

					// The Hoplite elements were labelled with their own id ("S0").
					if (e.label == e.id && de.label != de.id)
					{
						e.label = de.label;
						changes++;
					}

					foreach (CGQC_GU_Seat dseat : de.seats)
					{
						CGQC_GU_Seat seat = FindSeatIn(e, dseat.callsign);
						if (!seat || seat.role == dseat.role)
							continue;

						CGQC_GU_SeatRole dr = FindSeatRole(dseat.role);
						bool oldDefault = CGQC_GU_DefaultCatalogues.LegacyRoleId(seat.role) == dseat.role;
						if (dr && seat.role == dr.label)
							oldDefault = true;

						if (oldDefault)
						{
							seat.role = dseat.role;
							changes++;
						}
					}
				}
			}
		}

		if (changes == 0)
			return;

		CGQC_GU_PrettyJsonSaveContext save = NewFile();
		save.WriteValue("seatRoles", m_aSeatRoles);
		save.WriteValue("units", m_aUnits);
		if (CGQC_GU_Json.SafeSave(save, path))
			CGQC_GU_Log.Info(string.Format("units.json : %1 valeur(s) par défaut mise(s) à jour (libellés de la maquette, rôles des places et grade minimum, factions, places UPO).", changes));
	}

	//------------------------------------------------------------------------------------------------
	// Units lookups (Phase 4)
	//------------------------------------------------------------------------------------------------
	CGQC_GU_Unit FindUnit(string id)
	{
		foreach (CGQC_GU_Unit u : m_aUnits)
		{
			if (u.id == id)
				return u;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Section FindSectionIn(CGQC_GU_Unit u, string id)
	{
		foreach (CGQC_GU_Section s : u.sections)
		{
			if (s.id == id)
				return s;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Element FindElementIn(CGQC_GU_Section s, string id)
	{
		foreach (CGQC_GU_Element e : s.elements)
		{
			if (e.id == id)
				return e;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Seat FindSeatIn(CGQC_GU_Element e, string callsign)
	{
		foreach (CGQC_GU_Seat seat : e.seats)
		{
			if (seat.callsign == callsign)
				return seat;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! By id, then by label (a role written by hand as « Fusilier »). null for free text.
	CGQC_GU_SeatRole FindSeatRole(string role)
	{
		foreach (CGQC_GU_SeatRole r : m_aSeatRoles)
		{
			if (r.id == role)
				return r;
		}

		foreach (CGQC_GU_SeatRole r2 : m_aSeatRoles)
		{
			if (r2.label == role)
				return r2;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	string SeatRoleLabel(string role)
	{
		CGQC_GU_SeatRole r = FindSeatRole(role);
		if (r)
			return r.label;

		return role;
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
