// Gestion d'unité - server-side permission checks by access group.
// Which group holds which right is data (access_groups.json); the right names below are what code checks.

class CGQC_GU_Rights
{
	static const string ALL = "*";
	static const string MEMBERS_EDIT = "members.edit";				// edit any dossier, rank, status, link profiles
	static const string ACCESS_MANAGE = "access.manage";			// grant or remove access groups
	static const string PROMOTION_POSSIBLE = "promotion.view";		// see the "Promotion possible" flag
	static const string OPERATIONS_MANAGE = "operations.manage";
	static const string DEPLOYMENTS_MANAGE = "deployments.manage";
	static const string PHASE_CHANGE = "phase.change";
	static const string ORBAT_ASSIGN = "orbat.assign";
	static const string NOTICE_EDIT = "notice.edit";
	static const string SESSIONS_CREATE = "sessions.create";
	static const string QUALS_EVALUATE_OWN = "quals.evaluate_own";
	static const string UNIT_MENU = "unit.menu";						// Gestion d'unité entry in the Échap menu
	static const string CALENDAR_EDIT = "calendar.edit";				// manual calendar events (Phase 2)
	static const string INSTRUCTORS_MANAGE = "instructors.manage";	// authorize instructors per qualification (Phase 3)
}

class CGQC_GU_Permissions
{
	//------------------------------------------------------------------------------------------------
	static bool HasRight(CGQC_GU_Catalogues cat, CGQC_GU_Member m, string right)
	{
		if (!cat || !m)
			return false;

		foreach (string groupId : m.accessGroups)
		{
			CGQC_GU_AccessGroup g = cat.FindAccessGroup(groupId);
			if (!g)
				continue;

			if (g.rights.Contains(CGQC_GU_Rights.ALL) || g.rights.Contains(right))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Only the highest group is displayed as a tag.
	static CGQC_GU_AccessGroup GetHighestGroup(CGQC_GU_Catalogues cat, CGQC_GU_Member m)
	{
		CGQC_GU_AccessGroup best;
		if (!cat || !m)
			return null;

		foreach (string groupId : m.accessGroups)
		{
			CGQC_GU_AccessGroup g = cat.FindAccessGroup(groupId);
			if (g && (!best || g.level > best.level))
				best = g;
		}

		if (!best)
			best = cat.FindAccessGroup(cat.GetDefaultAccessGroup());

		return best;
	}
}
