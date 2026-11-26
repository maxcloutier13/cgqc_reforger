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
	ref CGQC_GU_OrbatLive m_OrbatLive;

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

		RecomputeAllStatuses();

		int autosave = m_Catalogues.m_Settings.autosaveSeconds;
		if (autosave < 30)
			autosave = 30;

		GetGame().GetCallqueue().CallLater(Autosave, autosave * 1000, true);
		CGQC_GU_Log.Info(string.Format("Démarré : %1 dossiers chargés depuis %2.", m_mMembers.Count(), CGQC_GU_Storage.ROOT));

		// Players registered before the game started (the Workbench / listen host) missed
		// OnPlayerRegistered while the manager did not exist yet: pick them up now.
		array<int> players = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int playerId : players)
		{
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
		CGQC_GU_Storage.SaveNews(m_News);
	}

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
	}

	//------------------------------------------------------------------------------------------------
	// Player connection and identity
	//------------------------------------------------------------------------------------------------
	void OnPlayerRegistered(int playerId)
	{
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
	}

	//------------------------------------------------------------------------------------------------
	// Light replicated state (phase, status, notice, players online)
	//------------------------------------------------------------------------------------------------
	protected void SendLightState(int playerId)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!pc)
			return;

		pc.CGQC_GU_SendLightState(m_iLightSerial, m_Situation.phase, m_Situation.qualification, m_Situation.unitStatus, m_Notice.text, GetOnlineCount());
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
	}
}
