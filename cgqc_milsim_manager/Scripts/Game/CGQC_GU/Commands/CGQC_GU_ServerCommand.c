// Gestion d'unité - admin console: "#gu ..." in chat (logged-in admin) or over RCON.
// Every result is also printed to the server log.
//   #gu list
//   #gu show <uid|nom>
//   #gu set <uid|nom> rank <grade>
//   #gu set <uid|nom> access <groupe,groupe,...>
//   #gu set <uid|nom> name <nom affiché>
//   #gu set <uid|nom> lastname <nom de famille>
//   #gu set <uid|nom> status <actif|inactif|mia|kia|auto>
//   #gu set <uid|nom> enlisted <AAAA[-MM[-JJ]]>
//   #gu save
//   #gu as <uid|nom> <action> clé=valeur ...   (test: runs a client request as that member)

class CGQC_GU_ServerCommand : ScrServerCommand
{
	//------------------------------------------------------------------------------------------------
	override string GetKeyword()
	{
		return "gu";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsServerSide()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override int RequiredRCONPermission()
	{
		return ERCONPermissions.PERMISSIONS_ADMIN;
	}

	//------------------------------------------------------------------------------------------------
	override int RequiredChatPermission()
	{
		return EPlayerRole.ADMINISTRATOR;
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		return Run(argv, "console:" + playerId.ToString());
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
	{
		return Run(argv, "console:rcon");
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnUpdate()
	{
		return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);
	}

	//------------------------------------------------------------------------------------------------
	protected ScrServerCmdResult Reply(string text, bool ok = true)
	{
		CGQC_GU_Log.Info("#gu : " + text);
		if (ok)
			return ScrServerCmdResult(text, EServerCmdResultType.OK);

		return ScrServerCmdResult(text, EServerCmdResultType.ERR);
	}

	//------------------------------------------------------------------------------------------------
	//! argv[0] is the keyword itself.
	protected ScrServerCmdResult Run(array<string> argv, string by)
	{
		CGQC_GU_Manager mgr = CGQC_GU_Manager.Get();
		if (!mgr)
			return Reply("Gestion d'unité non démarrée.", false);

		if (argv.Count() < 2)
			return Reply("Usage : #gu list | show <uid|nom> | set <uid|nom> <champ> <valeur> | save", false);

		string sub = CGQC_GU_Str.Lower(argv[1]);
		switch (sub)
		{
			case "list":
				return List(mgr);

			case "show":
				if (argv.Count() < 3)
					return Reply("Usage : #gu show <uid|nom>", false);

				return Show(mgr, argv[2]);

			case "set":
				if (argv.Count() < 5)
					return Reply("Usage : #gu set <uid|nom> <rank|access|name|lastname|status|enlisted> <valeur>", false);

				return Set(mgr, argv, by);

			case "save":
				mgr.SaveAll();
				return Reply("Sauvegarde complète effectuée.");

			case "as":
				if (argv.Count() < 4)
					return Reply("Usage : #gu as <uid|nom> <action> clé=valeur ...", false);

				return As(mgr, argv);
		}

		return Reply("Sous-commande inconnue : " + sub, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Test harness: runs a client request as if <who> sent it, with the same server-side checks.
	//! Values may contain spaces: a word without '=' continues the previous value.
	//! uid / source / target accept a name too (resolved like #gu show).
	protected ScrServerCmdResult As(CGQC_GU_Manager mgr, array<string> argv)
	{
		CGQC_GU_Member caller = mgr.FindMemberLoose(argv[2]);
		if (!caller)
			return Reply("Dossier introuvable : " + argv[2], false);

		string action = argv[3];
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		string key;
		for (int i = 4; i < argv.Count(); i++)
		{
			string token = argv[i];
			int eq = token.IndexOf("=");
			if (eq > 0)
			{
				key = token.Substring(0, eq);
				p.Set(key, token.Substring(eq + 1, token.Length() - eq - 1));
			}
			else if (!key.IsEmpty())
			{
				p.Set(key, p.Get(key) + " " + token);
			}
		}

		array<string> memberKeys = { "uid", "source", "target" };
		foreach (string k : memberKeys)
		{
			if (!p.Has(k))
				continue;

			CGQC_GU_Member target = mgr.FindMemberLoose(p.Get(k));
			if (target)
				p.Set(k, target.uid);
		}

		CGQC_GU_RequestResult result = CGQC_GU_Requests.Handle(mgr, caller, action, p.ToJson());
		string ok = "REFUSÉ";
		if (result.m_bOk)
			ok = "OK";

		return Reply(string.Format("%1 %2 (%3) par %4, %5 octets", action, ok, result.m_sCode, caller.displayName, result.m_sPayload.Length()), result.m_bOk);
	}

	//------------------------------------------------------------------------------------------------
	protected string Line(CGQC_GU_Manager mgr, CGQC_GU_Member m)
	{
		string rankAbbr = m.rank;
		CGQC_GU_Rank r = mgr.m_Catalogues.FindRank(m.rank);
		if (r)
			rankAbbr = r.abbr;

		string access;
		CGQC_GU_AccessGroup g = CGQC_GU_Permissions.GetHighestGroup(mgr.m_Catalogues, m);
		if (g)
			access = g.label;

		string online = "hors ligne";
		if (mgr.IsOnline(m.uid))
			online = "en ligne";

		return string.Format("%1 %2 | %3 | %4 | %5 | %6", rankAbbr, m.displayName, access, m.GetEffectiveStatus(), online, m.uid);
	}

	//------------------------------------------------------------------------------------------------
	protected ScrServerCmdResult List(CGQC_GU_Manager mgr)
	{
		string text = string.Format("%1 dossiers :", mgr.m_mMembers.Count());
		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			text += "\n" + Line(mgr, m);
		}

		return Reply(text);
	}

	//------------------------------------------------------------------------------------------------
	protected ScrServerCmdResult Show(CGQC_GU_Manager mgr, string who)
	{
		CGQC_GU_Member m = mgr.FindMemberLoose(who);
		if (!m)
			return Reply("Dossier introuvable : " + who, false);

		string text = Line(mgr, m);
		text += string.Format("\nprofil=%1 nom=%2 lié=%3 enrôlé=%4", m.profileName, m.lastName, m.uidLinked, m.enlistedOn);
		text += "\naccès=" + CGQC_GU_Str.Join(m.accessGroups, ",");
		text += string.Format("\nregistre=%1 entrées, qualifications=%2, distinctions=%3", m.record.Count(), m.qualifications.Count(), m.distinctions.Count());
		return Reply(text);
	}

	//------------------------------------------------------------------------------------------------
	protected ScrServerCmdResult Set(CGQC_GU_Manager mgr, array<string> argv, string by)
	{
		CGQC_GU_Member m = mgr.FindMemberLoose(argv[2]);
		if (!m)
			return Reply("Dossier introuvable : " + argv[2], false);

		string field = CGQC_GU_Str.Lower(argv[3]);
		string value = argv[4];
		for (int i = 5; i < argv.Count(); i++)
		{
			value += " " + argv[i];
		}

		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		switch (field)
		{
			case "rank":
			{
				CGQC_GU_Rank r = cat.FindRankLoose(value);
				if (!r)
					return Reply("Grade inconnu : " + value, false);

				if (r.id == m.rank)
					return Reply("Grade inchangé.");

				mgr.ChangeRank(m, r.id, by);
				break;
			}

			case "access":
			{
				array<string> groups = new array<string>();
				value.Split(",", groups, true);
				foreach (string g : groups)
				{
					string gid = g.Trim();
					if (!cat.FindAccessGroup(gid))
						return Reply("Groupe d'accès inconnu : " + gid, false);
				}

				m.accessGroups.Clear();
				foreach (string g2 : groups)
				{
					string id = g2.Trim();
					if (!m.accessGroups.Contains(id))
						m.accessGroups.Insert(id);
				}

				// Rights moved: refresh everyone's menu entry and edit buttons.
				mgr.BroadcastLightState();
				break;
			}

			case "name":
				m.displayName = value;
				break;

			case "lastname":
				m.lastName = value;
				break;

			case "status":
			{
				string s = CGQC_GU_Str.Lower(value);
				if (s == "auto")
				{
					m.status.manual = string.Empty;
					m.status.manualBy = string.Empty;
					m.status.manualAt = string.Empty;
				}
				else if (s == "actif" || s == "inactif" || s == "mia" || s == "kia")
				{
					m.status.manual = s;
					m.status.manualBy = by;
					m.status.manualAt = CGQC_GU_Time.NowUtc();
				}
				else
				{
					return Reply("Statut inconnu : " + value, false);
				}
				break;
			}

			case "enlisted":
				m.enlistedOn = value;
				break;

			default:
				return Reply("Champ inconnu : " + field, false);
		}

		if (!mgr.SaveMemberNow(m))
			return Reply("Modifié en mémoire, mais l'écriture du fichier a échoué.", false);

		return Reply("Modifié : " + Line(mgr, m));
	}
}
