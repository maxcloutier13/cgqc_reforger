// Gestion d'unité - server-side request dispatch. Every action checks the caller's rights here,
// never on the client. Replies carry a short code and a JSON payload with only what the caller may see.

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
}

class CGQC_GU_Requests
{
	static const string PING = "ping";
	static const string ME = "me";
	static const string MEMBER_GET = "member.get";
	static const string MEMBER_LIST = "member.list";

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_RequestResult Handle(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string action, string payload)
	{
		switch (action)
		{
			case PING:
				return new CGQC_GU_RequestResult(true, "pong");

			case ME:
				return MemberGet(mgr, caller, caller.uid);

			case MEMBER_GET:
				return MemberGet(mgr, caller, payload);	// payload = target uid

			case MEMBER_LIST:
				return MemberList(mgr);
		}

		return new CGQC_GU_RequestResult(false, "unknown_action");
	}

	//------------------------------------------------------------------------------------------------
	//! Full dossier, read-only: every member may read every dossier (owner decision 2026-11-26).
	protected static CGQC_GU_RequestResult MemberGet(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string uid)
	{
		CGQC_GU_Member m = mgr.FindMember(uid);
		if (!m)
			return new CGQC_GU_RequestResult(false, "not_found");

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		m.WriteTo(ctx);
		return new CGQC_GU_RequestResult(true, "ok", ctx.Export());
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
			list.Insert(s);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("members", list);
		return new CGQC_GU_RequestResult(true, "ok", ctx.Export());
	}
}
