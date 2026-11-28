// Gestion d'unité - server-side manager. Exists on the server (or the Workbench host) only.
// Owns catalogues, members and unit state; every write and every permission check happens here.

class CGQC_GU_Manager
{
	protected static ref CGQC_GU_Manager s_Instance;

	ref CGQC_GU_Catalogues m_Catalogues = new CGQC_GU_Catalogues();
	ref map<string, ref CGQC_GU_Member> m_mMembers = new map<string, ref CGQC_GU_Member>();
	ref CGQC_GU_Situation m_Situation;
	ref CGQC_GU_Notice m_Notice;
	ref CGQC_GU_News m_News;
	ref CGQC_GU_Calendar m_Calendar;
	ref CGQC_GU_OrbatLive m_OrbatLive;
	ref CGQC_GU_OrbatGroups m_Groups = new CGQC_GU_OrbatGroups();
	ref map<string, ref CGQC_GU_Training> m_mTrainings = new map<string, ref CGQC_GU_Training>();
	ref map<string, ref CGQC_GU_Operation> m_mOperations = new map<string, ref CGQC_GU_Operation>();

	protected ref map<int, string> m_mPlayerToUid = new map<int, string>();
	protected ref set<string> m_sDirtyMembers = new set<string>();
	protected ref set<int> m_sAwaitingUid = new set<int>();
	protected int m_iLightSerial;

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Manager Get()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Called from the game mode's OnGameStart on the server.
	static void Start()
	{
		if (s_Instance)
			return;

		s_Instance = new CGQC_GU_Manager();
		s_Instance.Init();
	}

	//------------------------------------------------------------------------------------------------
	static void Stop()
	{
		if (!s_Instance)
			return;

		s_Instance.SaveAll();
		s_Instance.m_Groups.Stop();
		GetGame().GetCallqueue().Remove(s_Instance.Autosave);
		s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void Init()
	{
		CGQC_GU_Storage.EnsureLayout();
		m_Catalogues.LoadAll(CGQC_GU_Storage.Dir("config"));

		CGQC_GU_Storage.LoadAllMembers(m_mMembers);
		CGQC_GU_SeedRoster.Apply(this);

		m_Situation = CGQC_GU_Storage.LoadSituation();
		// The phase always starts at TEMPS LIBRE; the change log and unit status are kept.
		if (m_Situation.phase != "temps_libre")
		{
			LogSituationChange("systeme", "phase", m_Situation.phase, "temps_libre");
			m_Situation.phase = "temps_libre";
			m_Situation.qualification = false;
		}
		CGQC_GU_Storage.SaveSituation(m_Situation);

		m_Notice = CGQC_GU_Storage.LoadNotice();
		m_News = CGQC_GU_Storage.LoadNews();
		m_Calendar = CGQC_GU_Storage.LoadCalendar();
		CGQC_GU_Storage.LoadAllTrainings(m_mTrainings);
		CGQC_GU_Storage.LoadAllOperations(m_mOperations);

		m_OrbatLive = CGQC_GU_Storage.LoadOrbatLive();
		if (!m_OrbatLive)
		{
			m_OrbatLive = new CGQC_GU_OrbatLive();
			foreach (CGQC_GU_Unit unit : m_Catalogues.m_aUnits)
			{
				foreach (CGQC_GU_Section section : unit.sections)
				{
					if (section.activeByDefault)
						m_OrbatLive.activeSections.Insert(section.id);
				}
			}
			CGQC_GU_Storage.SaveOrbatLive(m_OrbatLive);
		}
		else
		{
			// Seats change from game to game: seats, role choices and locks go; sections and renames stay.
			int seats = m_OrbatLive.assignments.Count();
			int roles = m_OrbatLive.roles.Count();
			m_OrbatLive.assignments.Clear();
			m_OrbatLive.roles.Clear();
			CGQC_GU_Storage.SaveOrbatLive(m_OrbatLive);
			if (seats > 0 || roles > 0)
				CGQC_GU_Log.Info(string.Format("ORBAT : %1 place(s) et %2 rôle(s) choisi(s) effacés au démarrage ; %3 section(s) active(s) conservée(s).", seats, roles, m_OrbatLive.activeSections.Count()));
		}

		// UPO live seats (4e): back to the catalogue's default count at every start.
		CGQC_GU_OrbatRequests.ResetLiveSeats(this);
		CGQC_GU_Storage.SaveOrbatLive(m_OrbatLive);

		RecomputeAllStatuses();

		int autosave = m_Catalogues.m_Settings.autosaveSeconds;
		if (autosave < 30)
			autosave = 30;

		GetGame().GetCallqueue().CallLater(Autosave, autosave * 1000, true);
		CGQC_GU_Log.Info(string.Format("Démarré : %1 dossiers, %2 séances et %3 ordres chargés depuis %4.", m_mMembers.Count(), m_mTrainings.Count(), m_mOperations.Count(), CGQC_GU_Storage.ROOT));

		// Players registered before the game started (the Workbench / listen host) missed
		// OnPlayerRegistered while the manager did not exist yet: pick them up now.
		array<int> players = new array<int>();
		m_Groups.Init();
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			m_Groups.OnPlayerRegistered(playerId);
			TryIdentify(playerId, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Saving
	//------------------------------------------------------------------------------------------------
	void MarkDirty(CGQC_GU_Member m)
	{
		if (m)
			m_sDirtyMembers.Insert(m.uid);
	}

	//------------------------------------------------------------------------------------------------
	//! Writes one member now (used right after every confirmed change).
	bool SaveMemberNow(CGQC_GU_Member m)
	{
		if (!m)
			return false;

		m_sDirtyMembers.RemoveItem(m.uid);
		return CGQC_GU_Storage.SaveMember(m);
	}

	//------------------------------------------------------------------------------------------------
	protected void Autosave()
	{
		// lastSeenAt for everyone online, then every dirty file.
		foreach (int playerId, string uid : m_mPlayerToUid)
		{
			CGQC_GU_Member m = FindMember(uid);
			if (m)
			{
				m.lastSeenAt = CGQC_GU_Time.NowUtc();
				MarkDirty(m);
			}
		}

		FlushDirty();
	}

	//------------------------------------------------------------------------------------------------
	protected void FlushDirty()
	{
		array<string> uids = new array<string>();
		foreach (string uid : m_sDirtyMembers)
		{
			uids.Insert(uid);
		}

		m_sDirtyMembers.Clear();
		foreach (string uid : uids)
		{
			CGQC_GU_Member m = FindMember(uid);
			if (m)
				CGQC_GU_Storage.SaveMember(m);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SaveAll()
	{
		foreach (string uid, CGQC_GU_Member m : m_mMembers)
		{
			CGQC_GU_Storage.SaveMember(m);
		}

		m_sDirtyMembers.Clear();
		if (m_Situation) CGQC_GU_Storage.SaveSituation(m_Situation);
		if (m_Notice) CGQC_GU_Storage.SaveNotice(m_Notice);
		if (m_News) CGQC_GU_Storage.SaveNews(m_News);
		if (m_Calendar) CGQC_GU_Storage.SaveCalendar(m_Calendar);
		if (m_OrbatLive) CGQC_GU_Storage.SaveOrbatLive(m_OrbatLive);
		CGQC_GU_Log.Info(string.Format("Sauvegarde complète : %1 dossiers.", m_mMembers.Count()));
	}

	//------------------------------------------------------------------------------------------------
	// Members
	//------------------------------------------------------------------------------------------------
	CGQC_GU_Member FindMember(string uid)
	{
		return m_mMembers.Get(uid);
	}

	//------------------------------------------------------------------------------------------------
	//! Find by exact UID, then by display / last / profile name (case-insensitive).
	CGQC_GU_Member FindMemberLoose(string text)
	{
		CGQC_GU_Member m = FindMember(text);
		if (m)
			return m;

		string t = CGQC_GU_Str.Lower(text);
		foreach (string uid, CGQC_GU_Member c : m_mMembers)
		{
			if (CGQC_GU_Str.Lower(c.displayName) == t || CGQC_GU_Str.Lower(c.lastName) == t || CGQC_GU_Str.Lower(c.profileName) == t)
				return c;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	CGQC_GU_Member GetMemberForPlayer(int playerId)
	{
		string uid;
		if (!m_mPlayerToUid.Find(playerId, uid))
			return null;

		return FindMember(uid);
	}

	//------------------------------------------------------------------------------------------------
	//! 0 when the member is offline.
	int GetPlayerIdForUid(string uid)
	{
		foreach (int playerId, string boundUid : m_mPlayerToUid)
		{
			if (boundUid == uid)
				return playerId;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOnline(string uid)
	{
		foreach (int playerId, string boundUid : m_mPlayerToUid)
		{
			if (boundUid == uid)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	int GetOnlineCount()
	{
		return m_mPlayerToUid.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! New dossier at the entry rank (Cadet) with an ENROLLMENT entry in the Registre.
	CGQC_GU_Member CreateMember(string uid, string profileName, bool linked, string by)
	{
		CGQC_GU_Member m = new CGQC_GU_Member();
		m.uid = uid;
		m.uidLinked = linked;
		m.profileName = profileName;
		m.displayName = profileName;
		m.rank = m_Catalogues.GetEntryRank();
		string defaultGroup = m_Catalogues.GetDefaultAccessGroup();
		if (!defaultGroup.IsEmpty())
			m.accessGroups.Insert(defaultGroup);

		m.enlistedOn = CGQC_GU_Time.TodayUtc();
		m.createdAt = CGQC_GU_Time.NowUtc();
		m.AddRecord("enlistment", by);

		m_mMembers.Insert(uid, m);
		AddNews("enlistment", uid, null);
		SaveMemberNow(m);
		CGQC_GU_Log.Info(string.Format("Nouveau dossier Cadet : %1 (%2)", profileName, uid));
		return m;
	}

	//------------------------------------------------------------------------------------------------
	//! "+ Nouveau dossier": roster member without a UID yet (mbr_ key, PROFIL NON LIÉ).
	CGQC_GU_Member CreateUnlinkedMember(string lastName, string rank, string enlistedOn, string discordName, string by)
	{
		CGQC_GU_Member m = new CGQC_GU_Member();
		m.uid = CGQC_GU_Ids.New("mbr");
		m.uidLinked = false;
		m.lastName = lastName;
		m.displayName = lastName;
		m.discordName = discordName;
		m.rank = rank;
		string defaultGroup = m_Catalogues.GetDefaultAccessGroup();
		if (!defaultGroup.IsEmpty())
			m.accessGroups.Insert(defaultGroup);

		m.enlistedOn = enlistedOn;
		m.createdAt = CGQC_GU_Time.NowUtc();
		CGQC_GU_RecordEntry e = m.AddRecord("enlistment", by);
		if (!enlistedOn.IsEmpty())
			e.on = enlistedOn;

		m_mMembers.Insert(m.uid, m);
		CGQC_GU_Log.Info(string.Format("Nouveau dossier non lié : %1 (%2)", lastName, m.uid));
		return m;
	}

	//------------------------------------------------------------------------------------------------
	//! Rank change with its PROMOTION entry and a news item (codes only; labels come from ranks.json).
	void ChangeRank(CGQC_GU_Member m, string newRank, string by)
	{
		string oldRank = m.rank;
		m.rank = newRank;

		CGQC_GU_RecordEntry e = m.AddRecord("promotion", by);
		e.data.Set("rank", newRank);
		e.data.Set("from", oldRank);

		map<string, string> news = new map<string, string>();
		news.Set("rank", newRank);
		news.Set("from", oldRank);
		AddNews("promotion", m.uid, news);
	}

	//------------------------------------------------------------------------------------------------
	//! Moves a dossier to a new key (its Bohemia UID): new file written first, old file archived after.
	bool RekeyMember(CGQC_GU_Member m, string newUid)
	{
		if (m_mMembers.Contains(newUid))
			return false;

		string oldUid = m.uid;
		m.uid = newUid;
		m.uidLinked = true;
		if (!CGQC_GU_Storage.SaveMember(m))
		{
			m.uid = oldUid;
			m.uidLinked = false;
			return false;
		}

		m_mMembers.Remove(oldUid);
		m_mMembers.Insert(newUid, m);
		CGQC_GU_Storage.RemoveMemberFiles(oldUid);

		array<int> players = new array<int>();
		foreach (int playerId, string boundUid : m_mPlayerToUid)
		{
			if (boundUid == oldUid)
				players.Insert(playerId);
		}

		foreach (int playerId : players)
		{
			m_mPlayerToUid.Set(playerId, newUid);
		}

		// Keep unit news pointing at the dossier.
		bool newsChanged;
		foreach (CGQC_GU_NewsItem item : m_News.items)
		{
			string itemUid;
			if (item.data.Find("uid", itemUid) && itemUid == oldUid)
			{
				item.data.Set("uid", newUid);
				newsChanged = true;
			}
		}

		if (newsChanged)
			CGQC_GU_Storage.SaveNews(m_News);

		CGQC_GU_Log.Info(string.Format("Dossier %1 lié au profil %2", oldUid, newUid));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Merge an auto-created dossier into an unlinked one. The target keeps its own data, takes the
	//! source's UID, profile name and Registre entries; the source file is archived, never just deleted.
	bool MergeMembers(CGQC_GU_Member source, CGQC_GU_Member target, string by)
	{
		string sourceUid = source.uid;
		if (!CGQC_GU_Storage.ArchiveMember(source))
			return false;

		m_mMembers.Remove(sourceUid);
		CGQC_GU_Storage.RemoveMemberFiles(sourceUid);

		foreach (CGQC_GU_RecordEntry e : source.record)
		{
			target.record.Insert(e);
		}

		if (target.profileName.IsEmpty())
			target.profileName = source.profileName;

		// Personal info the player filled in on the auto-created dossier fills the target's empty fields.
		if (target.personal.birthDate.IsEmpty()) target.personal.birthDate = source.personal.birthDate;
		if (target.personal.birthPlace.IsEmpty()) target.personal.birthPlace = source.personal.birthPlace;
		if (target.personal.bloodType.IsEmpty()) target.personal.bloodType = source.personal.bloodType;
		if (target.personal.bio.IsEmpty()) target.personal.bio = source.personal.bio;

		// The auto-created dossier is the one the player has actually been using.
		if (!source.lastSeenAt.IsEmpty())
			target.lastSeenAt = source.lastSeenAt;

		string oldTargetUid = target.uid;
		// RekeyMember refuses a key that is in the map; the source was removed above.
		if (!RekeyMember(target, sourceUid))
		{
			CGQC_GU_Log.Error(string.Format("Fusion %1 -> %2 : échec de l'écriture ; le dossier source est dans archive/.", sourceUid, oldTargetUid));
			return false;
		}

		CGQC_GU_RecordEntry note = target.AddRecord("note", by, "Fusion du dossier créé automatiquement.");
		note.data.Set("mergedFrom", sourceUid);
		note.data.Set("previousKey", oldTargetUid);
		SaveMemberNow(target);
		CGQC_GU_Log.Info(string.Format("Fusion : dossier auto %1 fusionné dans %2 (%3)", sourceUid, oldTargetUid, target.lastName));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Automatic status from missed OPORD operations; thresholds come from settings.json.
	void RecomputeStatus(CGQC_GU_Member m)
	{
		CGQC_GU_Settings s = m_Catalogues.m_Settings;
		string computed = "actif";
		if (m.stats.missedOps >= s.miaAfterMissedOps)
			computed = "mia";
		else if (m.stats.missedOps >= s.inactiveAfterMissedOps)
			computed = "inactif";

		if (m.status.computed != computed)
		{
			m.status.computed = computed;
			MarkDirty(m);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RecomputeAllStatuses()
	{
		foreach (string uid, CGQC_GU_Member m : m_mMembers)
		{
			RecomputeStatus(m);
		}

		FlushDirty();
	}

	//------------------------------------------------------------------------------------------------
	void AddNews(string type, string uid, map<string, string> data)
	{
		CGQC_GU_NewsItem item = new CGQC_GU_NewsItem();
		item.id = CGQC_GU_Ids.New("news");
		item.type = type;
		item.on = CGQC_GU_Time.NowUtc();
		item.data.Set("uid", uid);
		if (data)
		{
			foreach (string k, string v : data)
			{
				item.data.Set(k, v);
			}
		}

		m_News.items.Insert(item);
		while (m_News.items.Count() > NEWS_MAX)
		{
			m_News.items.RemoveOrdered(0);
		}

		CGQC_GU_Storage.SaveNews(m_News);
	}

	static const int NEWS_MAX = 200;	// news.json keeps the newest items only

	//------------------------------------------------------------------------------------------------
	protected void LogSituationChange(string by, string field, string from, string to)
	{
		CGQC_GU_ChangeLogEntry e = new CGQC_GU_ChangeLogEntry();
		e.at = CGQC_GU_Time.NowUtc();
		e.by = by;
		e.field = field;
		e.from = from;
		e.to = to;
		m_Situation.log.Insert(e);

		while (m_Situation.log.Count() > SITUATION_LOG_MAX)
		{
			m_Situation.log.RemoveOrdered(0);
		}
	}

	static const int SITUATION_LOG_MAX = 500;

	//------------------------------------------------------------------------------------------------
	//! Situation change (Phase 2). Empty values are left as they are. Each changed field is logged; nothing
	//! is written when nothing changes. Qualification only exists with ENTRAÎNEMENT. Returns the number of changes.
	int ApplySituation(string by, string phase, string qualification, string unitStatus)
	{
		int changes;
		string newPhase = m_Situation.phase;
		if (!phase.IsEmpty())
			newPhase = phase;

		if (newPhase != m_Situation.phase)
		{
			LogSituationChange(by, "phase", m_Situation.phase, newPhase);
			m_Situation.phase = newPhase;
			changes++;
		}

		bool newQual = m_Situation.qualification;
		if (!qualification.IsEmpty())
			newQual = qualification == "1";

		if (m_Situation.phase != "entrainement")
			newQual = false;

		if (newQual != m_Situation.qualification)
		{
			LogSituationChange(by, "qualification", BoolCode(m_Situation.qualification), BoolCode(newQual));
			m_Situation.qualification = newQual;
			changes++;
		}

		if (!unitStatus.IsEmpty() && unitStatus != m_Situation.unitStatus)
		{
			LogSituationChange(by, "unitStatus", m_Situation.unitStatus, unitStatus);
			m_Situation.unitStatus = unitStatus;
			changes++;

			// DÉPLOYÉ / GARNISON makes unit news (owner decision 6); phase changes do not.
			map<string, string> news = new map<string, string>();
			news.Set("status", unitStatus);
			AddNews("unit_status", by, news);
		}

		if (changes == 0)
			return 0;

		CGQC_GU_Storage.SaveSituation(m_Situation);
		BroadcastLightState();
		return changes;
	}

	//------------------------------------------------------------------------------------------------
	protected static string BoolCode(bool v)
	{
		if (v)
			return "1";

		return "0";
	}

	//------------------------------------------------------------------------------------------------
	//! Command notice. Empty text clears it. Returns false if the file could not be written.
	bool SetNotice(string text, string by)
	{
		m_Notice.text = text;
		m_Notice.by = by;
		m_Notice.at = CGQC_GU_Time.NowUtc();
		bool saved = CGQC_GU_Storage.SaveNotice(m_Notice);
		BroadcastLightState();
		return saved;
	}

	//------------------------------------------------------------------------------------------------
	// Player connection and identity
	//------------------------------------------------------------------------------------------------
	void OnPlayerRegistered(int playerId)
	{
		m_Groups.OnPlayerRegistered(playerId);
		TryIdentify(playerId, false);
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerAudited(int playerId)
	{
		TryIdentify(playerId, false);
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerDisconnected(int playerId)
	{
		m_sAwaitingUid.RemoveItem(playerId);
		CGQC_GU_Member m = GetMemberForPlayer(playerId);
		string goneUid;
		if (m)
			goneUid = m.uid;

		m_Groups.OnPlayerDisconnected(this, playerId, goneUid);
		m_mPlayerToUid.Remove(playerId);
		if (m)
		{
			m.lastSeenAt = CGQC_GU_Time.NowUtc();
			SaveMemberNow(m);
		}

		BroadcastLightState();
	}

	//------------------------------------------------------------------------------------------------
	//! Dev fallback: allowed outside a dedicated server, or when settings.json says so.
	protected bool IsDevUidAllowed()
	{
		if (m_Catalogues.m_Settings.allowDevUid)
			return true;

		return RplSession.Mode() != RplMode.Dedicated;
	}

	//------------------------------------------------------------------------------------------------
	protected void TryIdentify(int playerId, bool allowFallback)
	{
		string uid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		string profileName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		CGQC_GU_Log.Info(string.Format("Identification joueur %1 (%2) : UID %3", playerId, profileName, uid));

		if (uid.IsEmpty())
		{
			if (!allowFallback)
			{
				// The backend UID usually arrives with the audit; wait before using the dev fallback.
				if (!m_sAwaitingUid.Contains(playerId))
				{
					m_sAwaitingUid.Insert(playerId);
					int delay = m_Catalogues.m_Settings.devUidDelaySeconds;
					GetGame().GetCallqueue().CallLater(DevFallback, delay * 1000, false, playerId);
				}
				return;
			}

			if (!IsDevUidAllowed())
			{
				CGQC_GU_Log.Warn(string.Format("Aucun UID Bohemia pour %1 (joueur %2) ; dossier non ouvert.", profileName, playerId));
				return;
			}

			uid = "dev-" + CGQC_GU_Str.FileSafe(profileName);
			CGQC_GU_Log.Warn(string.Format("UID de développement utilisé pour %1 : %2", profileName, uid));
		}

		m_sAwaitingUid.RemoveItem(playerId);
		BindPlayer(playerId, uid, profileName);
	}

	//------------------------------------------------------------------------------------------------
	protected void DevFallback(int playerId)
	{
		if (!m_sAwaitingUid.Contains(playerId))
			return;

		if (!GetGame().GetPlayerManager().GetPlayerController(playerId))
		{
			m_sAwaitingUid.RemoveItem(playerId);
			return;
		}

		TryIdentify(playerId, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void BindPlayer(int playerId, string uid, string profileName)
	{
		string previous;
		if (m_mPlayerToUid.Find(playerId, previous))
		{
			if (previous == uid)
				return;

			CGQC_GU_Log.Warn(string.Format("Joueur %1 : UID %2 remplacé par %3.", playerId, previous, uid));
		}

		m_mPlayerToUid.Set(playerId, uid);

		CGQC_GU_Member m = FindMember(uid);
		if (!m)
			m = CreateMember(uid, profileName, !uid.StartsWith("dev-"), "systeme");

		if (m.profileName != profileName)
			m.profileName = profileName;

		m.lastSeenAt = CGQC_GU_Time.NowUtc();
		SaveMemberNow(m);

		CGQC_GU_Log.Info(string.Format("Connecté : %1 -> dossier %2", profileName, uid));
		SendLightState(playerId);
		BroadcastLightState();
		m_Groups.OnPlayerBound(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Light replicated state (phase, status, notice, players online)
	//------------------------------------------------------------------------------------------------
	protected void SendLightState(int playerId)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!pc)
			return;

		pc.CGQC_GU_SendLightState(m_iLightSerial, m_Situation.phase, m_Situation.qualification, m_Situation.unitStatus, m_Notice.text, GetOnlineCount(), LightFlagsFor(playerId));
	}

	//------------------------------------------------------------------------------------------------
	//! What this player's UI may offer (display only; every action is checked again on the server).
	protected int LightFlagsFor(int playerId)
	{
		CGQC_GU_Member m = GetMemberForPlayer(playerId);
		if (!m)
			return 0;

		int flags;
		if (CGQC_GU_Permissions.HasRight(m_Catalogues, m, CGQC_GU_Rights.UNIT_MENU))
			flags |= CGQC_GU_LightState.CAN_MENU;
		if (CGQC_GU_Permissions.HasRight(m_Catalogues, m, CGQC_GU_Rights.NOTICE_EDIT))
			flags |= CGQC_GU_LightState.CAN_NOTICE;
		if (CGQC_GU_Permissions.HasRight(m_Catalogues, m, CGQC_GU_Rights.PHASE_CHANGE))
			flags |= CGQC_GU_LightState.CAN_PHASE;
		if (CGQC_GU_Permissions.HasRight(m_Catalogues, m, CGQC_GU_Rights.CALENDAR_EDIT))
			flags |= CGQC_GU_LightState.CAN_CALENDAR;

		return flags;
	}

	//------------------------------------------------------------------------------------------------
	void BroadcastLightState()
	{
		m_iLightSerial++;
		array<int> players = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
			SendLightState(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Requests from clients (through SCR_PlayerController)
	//------------------------------------------------------------------------------------------------
	void HandleRequest(SCR_PlayerController pc, int requestId, string action, string payload)
	{
		int playerId = pc.GetPlayerId();
		CGQC_GU_Member caller = GetMemberForPlayer(playerId);
		if (!caller)
		{
			pc.CGQC_GU_SendReply(requestId, false, "no_dossier", string.Empty);
			return;
		}

		CGQC_GU_RequestResult result = CGQC_GU_Requests.Handle(this, caller, action, payload);
		pc.CGQC_GU_SendReply(requestId, result.m_bOk, result.m_sCode, result.m_sPayload);

		// Rights may have moved: refresh everyone's light flags (menu entry, notice edit button).
		// An instructor authorization may add the Instructeur group; an evaluation adds news (and maybe a
		// promotion), and the pause panels re-read the overview on every light-state change.
		if (result.m_bOk && (action == CGQC_GU_Requests.ACCESS_SET || action == CGQC_GU_Requests.MEMBER_LINK || action == CGQC_GU_Requests.MEMBER_MERGE
			|| action == CGQC_GU_QualRequests.INSTRUCTOR_SET || action == CGQC_GU_QualRequests.QUAL_EVALUATE))
			BroadcastLightState();
	}
}
