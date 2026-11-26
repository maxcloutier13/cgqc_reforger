// Gestion d'unité - INITIAL ROSTER IMPORT (one time).
// Sources (2026-11-26): the CGQC AdminTools spreadsheet (join / Q-MB dates, Discord names) and the
// cgqc_core/Configs/CGQC/*.conf rank rosters (Bohemia UIDs, ranks). Conf ranks win over the sheet.
// Applied once per SEED_VERSION (state/seed.json): a member removed later is never re-added.
// Already-existing dossiers (same UID, or same last name for UID-less entries) are left untouched.

class CGQC_GU_SeedEntry
{
	string m_sUid;			// "" = no valid UID yet -> mbr_ key, PROFIL NON LIÉ
	string m_sLastName;
	string m_sRank;
	string m_sDiscord;
	string m_sJoined;		// ISO date of the Q-MB / joining, "" when unknown
	string m_sNote;

	void CGQC_GU_SeedEntry(string uid, string lastName, string rank, string discord, string joined, string note = "")
	{
		m_sUid = uid;
		m_sLastName = lastName;
		m_sRank = rank;
		m_sDiscord = discord;
		m_sJoined = joined;
		m_sNote = note;
	}
}

class CGQC_GU_SeedRoster
{
	static const int SEED_VERSION = 1;

	//------------------------------------------------------------------------------------------------
	protected static void Entries(array<ref CGQC_GU_SeedEntry> list)
	{
		list.Insert(new CGQC_GU_SeedEntry("d7e9113c-f075-41c5-a72a-9ee5187dc723", "Cloutier", "maj", "Silent1", "2022-04-04"));
		list.Insert(new CGQC_GU_SeedEntry("c3fcb72f-2f87-4937-8847-1fb62b39a40c", "Tremblay", "maj", "darkangel898", "2020-07-12"));
		list.Insert(new CGQC_GU_SeedEntry("e1c8cdf2-953a-4db0-be45-2cd8e79556f2", "Valiquette", "maj", "HELLRIK", "2022-11-12"));
		list.Insert(new CGQC_GU_SeedEntry("6538de9f-32ce-41cc-bc6f-844a6a3b2ce8", "Laforest", "capt", "frogham", "", "Q-MB : « Vieux » (date inconnue). Grade Lt. dans le tableau, Capt. dans cgqc_core."));
		list.Insert(new CGQC_GU_SeedEntry("120786ec-60cd-4a96-9e10-c846578745f2", "Genest", "lt", "lebeurre._", "2022-03-13"));
		list.Insert(new CGQC_GU_SeedEntry("e98c24ac-31b8-4647-94f8-c4ffa5bec5fe", "Trépanier", "slt", "atchoumec", "2024-11-29"));
		list.Insert(new CGQC_GU_SeedEntry("60e84a60-c12f-4b4a-91ed-c294bab6046e", "Comeau", "adjum", "Pig'sPeels", "2022-04-04"));
		list.Insert(new CGQC_GU_SeedEntry("", "Deslauriers", "adju", "Sampique", "2022-11-29"));
		list.Insert(new CGQC_GU_SeedEntry("fdeca5b5-5cba-403a-8e3a-d598325dbccc", "Dubé", "sgt", "Dubaille", "2022-05-30"));
		list.Insert(new CGQC_GU_SeedEntry("c6643f04-50cb-461a-b591-b6e6deb63ed9", "Fournier", "sgt", "psyck0u", "2023-10-02"));
		list.Insert(new CGQC_GU_SeedEntry("789954f6-cdb6-4f69-8095-af635b9a78a1", "Lauzon", "sgt", "JoelCarl", "2022-09-14"));
		list.Insert(new CGQC_GU_SeedEntry("dd7969a6-722f-4ae6-a9e7-898ee9e6fea5", "Pike", "sgt", "epicdudejo", "2021-06-01"));
		list.Insert(new CGQC_GU_SeedEntry("dce42da0-351f-4d03-821b-e89e46b70002", "Turcotte", "sgt", "server_error", "2019-10-15"));
		list.Insert(new CGQC_GU_SeedEntry("", "Xang", "cplc", "kittyazian", "2019-06-01"));
		list.Insert(new CGQC_GU_SeedEntry("f86ee5bc-39bd-4a59-98b7-9caf0ee061b6", "Audet", "sdt", "pl6527", "2026-02-15"));
		list.Insert(new CGQC_GU_SeedEntry("daa9fb24-0729-45bb-b41f-324d22ca632b", "Forgues", "sdt", "", ""));
		list.Insert(new CGQC_GU_SeedEntry("72c38387-e5fd-46f4-806e-ce1e7c0ce16e", "Walker", "sdt", "", ""));
		list.Insert(new CGQC_GU_SeedEntry("", "Frechette", "vet", "frechette033", "2020-03-01"));
	}

	//------------------------------------------------------------------------------------------------
	//! Access groups given by the import: État-major ranks (Maj. and up) are Commandement.
	protected static void AccessFor(CGQC_GU_Catalogues cat, string rank, array<string> groups)
	{
		if (rank == "maj" || rank == "lcol" || rank == "col")
			groups.Insert("commandement");
		else
			groups.Insert(cat.GetDefaultAccessGroup());
	}

	//------------------------------------------------------------------------------------------------
	static int GetAppliedVersion()
	{
		int version;
		JsonLoadContext ctx = CGQC_GU_Json.Open(CGQC_GU_Storage.Dir("state") + "/seed.json");
		if (ctx)
			ctx.ReadValue("seedVersion", version);

		return version;
	}

	//------------------------------------------------------------------------------------------------
	static void Apply(CGQC_GU_Manager mgr)
	{
		if (GetAppliedVersion() >= SEED_VERSION)
			return;

		array<ref CGQC_GU_SeedEntry> list = new array<ref CGQC_GU_SeedEntry>();
		Entries(list);

		int added;
		foreach (CGQC_GU_SeedEntry e : list)
		{
			if (Exists(mgr, e))
				continue;

			CGQC_GU_Member m = new CGQC_GU_Member();
			if (e.m_sUid.IsEmpty())
			{
				m.uid = CGQC_GU_Ids.New("mbr");
				m.uidLinked = false;
			}
			else
			{
				m.uid = e.m_sUid;
				m.uidLinked = true;
			}

			m.lastName = e.m_sLastName;
			m.displayName = e.m_sLastName;
			m.discordName = e.m_sDiscord;
			m.rank = e.m_sRank;
			AccessFor(mgr.m_Catalogues, e.m_sRank, m.accessGroups);
			m.enlistedOn = e.m_sJoined;
			m.createdAt = CGQC_GU_Time.NowUtc();

			CGQC_GU_RecordEntry rec = m.AddRecord("enlistment", "systeme", "Import initial");
			rec.on = e.m_sJoined;

			// Everyone on the roster holds Q-MB (it is the entry qualification); the sheet's date is its date.
			CGQC_GU_QualResult q = new CGQC_GU_QualResult();
			q.id = CGQC_GU_Ids.New("qr");
			q.qual = "Q-MB";
			q.result = "passed";
			q.on = e.m_sJoined;
			m.qualifications.Insert(q);

			if (!e.m_sNote.IsEmpty())
				m.AddRecord("note", "systeme", e.m_sNote);

			mgr.m_mMembers.Insert(m.uid, m);
			CGQC_GU_Storage.SaveMember(m);
			added++;
		}

		CGQC_GU_PrettyJsonSaveContext save = new CGQC_GU_PrettyJsonSaveContext();
		save.WriteValue("schemaVersion", 1);
		save.WriteValue("seedVersion", SEED_VERSION);
		save.WriteValue("appliedAt", CGQC_GU_Time.NowUtc());
		CGQC_GU_Json.SafeSave(save, CGQC_GU_Storage.Dir("state") + "/seed.json");
		CGQC_GU_Log.Info(string.Format("Import initial v%1 : %2 dossiers ajoutés.", SEED_VERSION, added));
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Exists(CGQC_GU_Manager mgr, CGQC_GU_SeedEntry e)
	{
		if (!e.m_sUid.IsEmpty())
			return mgr.FindMember(e.m_sUid) != null;

		string name = CGQC_GU_Str.Lower(e.m_sLastName);
		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			if (CGQC_GU_Str.Lower(m.lastName) == name)
				return true;
		}

		return false;
	}
}
