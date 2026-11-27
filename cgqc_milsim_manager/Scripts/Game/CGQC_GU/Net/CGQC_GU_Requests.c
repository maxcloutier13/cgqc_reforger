// Gestion d'unité - server-side request dispatch. Every action checks the caller's rights here,
// never on the client. Replies carry a short code and a JSON payload.
// Reading: every member may read every dossier (owner decision 2026-11-26). Writing: see each action.

class CGQC_GU_RequestResult
{
	bool m_bOk;
	string m_sCode;
	string m_sPayload;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_RequestResult(bool ok, string code, string payload = "")
	{
		m_bOk = ok;
		m_sCode = code;
		m_sPayload = payload;
	}
}

//! Light view of a member for lists (no personal data).
class CGQC_GU_MemberSummary : Managed
{
	string uid;
	bool uidLinked;
	string displayName;
	string lastName;
	string rank;
	string access;		// highest group only
	string status;
	bool online;
	string lastSeenAt;
}

class CGQC_GU_Requests
{
	// Read
	static const string PING = "ping";
	static const string ME = "me";
	static const string MEMBER_GET = "member.get";				// uid
	static const string MEMBER_LIST = "member.list";
	static const string OVERVIEW = "overview";					// pause menu panel for the caller
	// Write
	static const string PERSONAL_SET = "member.personal.set";	// uid, birthDate, birthPlace, bloodType, bio
	static const string RANK_SET = "member.rank.set";			// uid, rank
	static const string ACCESS_SET = "member.access.set";		// uid, groups (comma list)
	static const string NAME_SET = "member.name.set";			// uid, displayName | useProfileName=1, lastName (optional)
	static const string STATUS_SET = "member.status.set";		// uid, status (actif|inactif|mia|kia|auto)
	static const string RECORD_ADD = "member.record.add";		// uid, type, on (optional), note
	static const string MEMBER_CREATE = "member.create";		// lastName, rank, enlistedOn, discordName
	static const string MEMBER_LINK = "member.link";			// uid (unlinked dossier), bohemiaUid
	static const string MEMBER_MERGE = "member.merge";			// source (auto-created dossier), target (unlinked dossier)

	static const int MAX_SHORT = 48;
	static const int MAX_NOTE = 500;
	static const int MAX_BIO = 1500;

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_RequestResult Handle(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string action, string payloadJson)
	{
		CGQC_GU_Payload p = CGQC_GU_Payload.FromJson(payloadJson);

		switch (action)
		{
			case PING:
				return Ok("pong");

			case ME:
				return MemberJson(caller);

			case MEMBER_GET:
			{
				CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
				if (!m)
					return Fail("not_found");

				return MemberJson(m);
			}

			case MEMBER_LIST:
				return MemberList(mgr);

			case OVERVIEW:
				return Ok("ok", CGQC_GU_OverviewBuilder.Build(mgr, caller));

			case PERSONAL_SET:
				return PersonalSet(mgr, caller, p);

			case RANK_SET:
				return RankSet(mgr, caller, p);

			case ACCESS_SET:
				return AccessSet(mgr, caller, p);

			case NAME_SET:
				return NameSet(mgr, caller, p);

			case STATUS_SET:
				return StatusSet(mgr, caller, p);

			case RECORD_ADD:
				return RecordAdd(mgr, caller, p);

			case MEMBER_CREATE:
				return MemberCreate(mgr, caller, p);

			case MEMBER_LINK:
				return MemberLink(mgr, caller, p);

			case MEMBER_MERGE:
				return MemberMerge(mgr, caller, p);
		}

		return Fail("unknown_action");
	}

	//------------------------------------------------------------------------------------------------
	// Helpers
	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Ok(string code, string payload = "")
	{
		return new CGQC_GU_RequestResult(true, code, payload);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Fail(string code)
	{
		return new CGQC_GU_RequestResult(false, code);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Can(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string right)
	{
		return CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, caller, right);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult MemberJson(CGQC_GU_Member m)
	{
		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		m.WriteTo(ctx);
		return Ok("ok", ctx.Export());
	}

	//------------------------------------------------------------------------------------------------
	//! Saves the member and returns the fresh dossier, or a write error.
	protected static CGQC_GU_RequestResult Saved(CGQC_GU_Manager mgr, CGQC_GU_Member m)
	{
		if (!mgr.SaveMemberNow(m))
			return Fail("write_failed");

		return MemberJson(m);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult MemberList(CGQC_GU_Manager mgr)
	{
		array<ref CGQC_GU_MemberSummary> list = new array<ref CGQC_GU_MemberSummary>();
		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			CGQC_GU_MemberSummary s = new CGQC_GU_MemberSummary();
			s.uid = m.uid;
			s.uidLinked = m.uidLinked;
			s.displayName = m.displayName;
			s.lastName = m.lastName;
			s.rank = m.rank;
			CGQC_GU_AccessGroup g = CGQC_GU_Permissions.GetHighestGroup(mgr.m_Catalogues, m);
			if (g)
				s.access = g.id;

			s.status = m.GetEffectiveStatus();
			s.online = mgr.IsOnline(m.uid);
			s.lastSeenAt = m.lastSeenAt;
			list.Insert(s);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("members", list);
		return Ok("ok", ctx.Export());
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	//! Own dossier, or anyone's with members.edit.
	protected static CGQC_GU_RequestResult PersonalSet(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		if (m != caller && !Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		string birthDate = p.Get("birthDate").Trim();
		if (!CGQC_GU_Validate.IsIsoDateOrEmpty(birthDate))
			return Fail("bad_date");

		string birthPlace = p.Get("birthPlace").Trim();
		string bloodType = p.Get("bloodType").Trim();
		string bio = p.Get("bio").Trim();
		if (birthPlace.Length() > MAX_SHORT || bloodType.Length() > 8 || bio.Length() > MAX_BIO)
			return Fail("too_long");

		if (p.Has("birthDate")) m.personal.birthDate = birthDate;
		if (p.Has("birthPlace")) m.personal.birthPlace = birthPlace;
		if (p.Has("bloodType")) m.personal.bloodType = bloodType;
		if (p.Has("bio")) m.personal.bio = bio;
		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult RankSet(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		CGQC_GU_Rank r = mgr.m_Catalogues.FindRank(p.Get("rank"));
		if (!r)
			return Fail("bad_rank");

		if (r.id == m.rank)
			return MemberJson(m);

		mgr.ChangeRank(m, r.id, caller.uid);
		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult AccessSet(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.ACCESS_MANAGE))
			return Fail("forbidden");

		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		array<string> groups = new array<string>();
		p.GetList("groups", groups);
		foreach (string g : groups)
		{
			if (!mgr.m_Catalogues.FindAccessGroup(g))
				return Fail("bad_group");
		}

		// Never leave a member with no group at all: fall back to the default (Membre).
		if (groups.IsEmpty())
			groups.Insert(mgr.m_Catalogues.GetDefaultAccessGroup());

		// Commandement cannot remove its own Commandement access (avoids locking the unit out).
		if (m == caller && caller.accessGroups.Contains("commandement") && !groups.Contains("commandement"))
			return Fail("self_lockout");

		m.accessGroups.Clear();
		m.accessGroups.Copy(groups);
		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult NameSet(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		string display = p.Get("displayName").Trim();
		if (p.GetBool("useProfileName"))
			display = m.profileName;

		if (display.IsEmpty() || display.Length() > MAX_SHORT)
			return Fail("bad_name");

		m.displayName = display;

		if (p.Has("lastName"))
		{
			string lastName = p.Get("lastName").Trim();
			if (lastName.IsEmpty() || lastName.Length() > MAX_SHORT)
				return Fail("bad_name");

			m.lastName = lastName;
		}

		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult StatusSet(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		string s = CGQC_GU_Str.Lower(p.Get("status"));
		if (s == "auto")
		{
			m.status.manual = string.Empty;
			m.status.manualBy = string.Empty;
			m.status.manualAt = string.Empty;
		}
		else if (s == "actif" || s == "inactif" || s == "mia" || s == "kia")
		{
			m.status.manual = s;
			m.status.manualBy = caller.uid;
			m.status.manualAt = CGQC_GU_Time.NowUtc();
		}
		else
		{
			return Fail("bad_status");
		}

		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	//! Manual Registre entry by staff.
	protected static CGQC_GU_RequestResult RecordAdd(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		string type = CGQC_GU_Str.Lower(p.Get("type"));
		if (!CGQC_GU_Validate.IsRecordType(type))
			return Fail("bad_type");

		string note = p.Get("note").Trim();
		if (note.IsEmpty() || note.Length() > MAX_NOTE)
			return Fail("bad_note");

		string on = p.Get("on").Trim();
		if (!CGQC_GU_Validate.IsIsoDateOrEmpty(on))
			return Fail("bad_date");

		CGQC_GU_RecordEntry e = m.AddRecord(type, caller.uid, note);
		if (!on.IsEmpty())
			e.on = on;

		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	//! "+ Nouveau dossier": a roster member who has not connected yet (no UID, PROFIL NON LIÉ).
	protected static CGQC_GU_RequestResult MemberCreate(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		string lastName = p.Get("lastName").Trim();
		if (lastName.IsEmpty() || lastName.Length() > MAX_SHORT)
			return Fail("bad_name");

		string rank = p.Get("rank");
		if (rank.IsEmpty())
			rank = mgr.m_Catalogues.GetEntryRank();
		if (!mgr.m_Catalogues.FindRank(rank))
			return Fail("bad_rank");

		string enlisted = p.Get("enlistedOn").Trim();
		if (!CGQC_GU_Validate.IsIsoDateOrEmpty(enlisted))
			return Fail("bad_date");

		string discord = p.Get("discordName").Trim();
		if (discord.Length() > MAX_SHORT)
			return Fail("too_long");

		CGQC_GU_Member m = mgr.CreateUnlinkedMember(lastName, rank, enlisted, discord, caller.uid);
		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	//! "Lier un profil" by typing the Bohemia UID.
	protected static CGQC_GU_RequestResult MemberLink(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		CGQC_GU_Member m = mgr.FindMember(p.Get("uid"));
		if (!m)
			return Fail("not_found");

		if (m.uidLinked)
			return Fail("already_linked");

		string bohemiaUid = CGQC_GU_Str.Lower(p.Get("bohemiaUid").Trim());
		if (!CGQC_GU_Validate.IsBohemiaUid(bohemiaUid))
			return Fail("bad_uid");

		// That profile already has its own dossier (auto-created Cadet): use the merge instead.
		if (mgr.FindMember(bohemiaUid))
			return Fail("uid_taken");

		if (!mgr.RekeyMember(m, bohemiaUid))
			return Fail("write_failed");

		CGQC_GU_RecordEntry e = m.AddRecord("note", caller.uid, "Profil Bohemia lié.");
		e.data.Set("uid", bohemiaUid);
		return Saved(mgr, m);
	}

	//------------------------------------------------------------------------------------------------
	//! Merge an auto-created dossier (linked, e.g. a Cadet made on first connection) into an existing
	//! unlinked dossier: the target takes the UID and the source's Registre; the source is archived.
	protected static CGQC_GU_RequestResult MemberMerge(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!Can(mgr, caller, CGQC_GU_Rights.MEMBERS_EDIT))
			return Fail("forbidden");

		CGQC_GU_Member source = mgr.FindMember(p.Get("source"));
		CGQC_GU_Member target = mgr.FindMember(p.Get("target"));
		if (!source || !target)
			return Fail("not_found");

		if (source == target)
			return Fail("same_member");

		if (!source.uidLinked)
			return Fail("source_not_linked");

		if (target.uidLinked)
			return Fail("already_linked");

		if (!mgr.MergeMembers(source, target, caller.uid))
			return Fail("write_failed");

		return MemberJson(target);
	}
}

class CGQC_GU_Validate
{
	//------------------------------------------------------------------------------------------------
	protected static bool AllDigits(string s)
	{
		for (int i = 0; i < s.Length(); i++)
		{
			string c = s.Get(i);
			if (!"0123456789".Contains(c))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! "", "2022", "2022-04" or "2022-04-04".
	static bool IsIsoDateOrEmpty(string s)
	{
		int n = s.Length();
		if (n == 0)
			return true;

		if (n != 4 && n != 7 && n != 10)
			return false;

		if (!AllDigits(s.Substring(0, 4)))
			return false;

		if (n >= 7)
		{
			if (s.Get(4) != "-" || !AllDigits(s.Substring(5, 2)))
				return false;

			int month = s.Substring(5, 2).ToInt();
			if (month < 1 || month > 12)
				return false;
		}

		if (n == 10)
		{
			if (s.Get(7) != "-" || !AllDigits(s.Substring(8, 2)))
				return false;

			int day = s.Substring(8, 2).ToInt();
			if (day < 1 || day > 31)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Bohemia identity UUID: 8-4-4-4-12 lowercase hex.
	static bool IsBohemiaUid(string s)
	{
		if (s.Length() != 36)
			return false;

		for (int i = 0; i < 36; i++)
		{
			string c = s.Get(i);
			if (i == 8 || i == 13 || i == 18 || i == 23)
			{
				if (c != "-")
					return false;
			}
			else if (!"0123456789abcdef".Contains(c))
			{
				return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsRecordType(string type)
	{
		array<string> types = { "enlistment", "promotion", "operation", "deployment", "session", "qualification", "distinction", "task", "note" };
		return types.Contains(type);
	}
}
