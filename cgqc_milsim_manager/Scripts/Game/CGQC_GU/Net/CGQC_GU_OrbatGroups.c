// Gestion d'unité - Phase 4b: ORBAT seats -> game groups (server only).
// One game group per section (one per unit for a unit with student seats, e.g. UPO), in the unit's
// faction (units.json `faction`), else the player's own faction. Found by id, else by name, else created.
// The ORBAT never changes anyone's faction; a seat whose move cannot happen yet stays `pending`
// and is applied on connection or faction choice. Freeing a seat moves nobody.

class CGQC_GU_GameGroupRef
{
	int m_iGroupId;
	string m_sUnit;
	string m_sKey;			// section id, or unit id for a unit-wide group
	string m_sFaction;		// faction key
	string m_sName;			// name the group must carry
}

class CGQC_GU_OrbatGroups
{
	protected static const int APPLY_DELAY_MS = 1500;	// after the game's own group placement
	protected static const int RENAME_DELAY_MS = 200;

	protected ref array<ref CGQC_GU_GameGroupRef> m_aGroups = new array<ref CGQC_GU_GameGroupRef>();
	protected bool m_bHooked;

	//------------------------------------------------------------------------------------------------
	void Init()
	{
		if (m_bHooked)
			return;

		SCR_AIGroup.GetOnCustomNameChanged().Insert(OnGroupNameChanged);
		m_bHooked = true;
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		if (m_bHooked)
			SCR_AIGroup.GetOnCustomNameChanged().Remove(OnGroupNameChanged);

		m_bHooked = false;
		GetGame().GetCallqueue().Remove(ApplyForPlayer);
		GetGame().GetCallqueue().Remove(Rename);
	}

	//------------------------------------------------------------------------------------------------
	// Player events (from the manager)
	//------------------------------------------------------------------------------------------------
	void OnPlayerRegistered(int playerId)
	{
		SCR_PlayerFactionAffiliationComponent fac = FactionComp(playerId);
		if (fac)
			fac.GetOnFactionChanged().Insert(OnPlayerFactionChanged);
	}

	//------------------------------------------------------------------------------------------------
	//! uid is still known here: the seat waits for the next connection.
	void OnPlayerDisconnected(CGQC_GU_Manager mgr, int playerId, string uid)
	{
		SCR_PlayerFactionAffiliationComponent fac = FactionComp(playerId);
		if (fac)
			fac.GetOnFactionChanged().Remove(OnPlayerFactionChanged);

		if (uid.IsEmpty())
			return;

		CGQC_GU_SeatAssignment a = CGQC_GU_OrbatRequests.FindByUid(mgr.m_OrbatLive, uid);
		if (a && !a.pending && SCR_GroupsManagerComponent.GetInstance())
		{
			a.pending = true;
			CGQC_GU_Storage.SaveOrbatLive(mgr.m_OrbatLive);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The player's dossier is bound (connection): apply a waiting seat once the game placed them.
	void OnPlayerBound(int playerId)
	{
		GetGame().GetCallqueue().CallLater(ApplyForPlayer, APPLY_DELAY_MS, false, playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerFactionChanged(FactionAffiliationComponent owner, Faction previousFaction, Faction newFaction)
	{
		if (!owner || !newFaction)
			return;

		PlayerController pc = PlayerController.Cast(owner.GetOwner());
		if (pc)
			GetGame().GetCallqueue().CallLater(ApplyForPlayer, APPLY_DELAY_MS, false, pc.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyForPlayer(int playerId)
	{
		CGQC_GU_Manager mgr = CGQC_GU_Manager.Get();
		if (!mgr)
			return;

		CGQC_GU_Member m = mgr.GetMemberForPlayer(playerId);
		if (!m)
			return;

		CGQC_GU_SeatAssignment a = CGQC_GU_OrbatRequests.FindByUid(mgr.m_OrbatLive, m.uid);
		if (!a || !a.pending)
			return;

		if (TryApply(mgr, a))
			CGQC_GU_Storage.SaveOrbatLive(mgr.m_OrbatLive);
	}

	//------------------------------------------------------------------------------------------------
	// Seats
	//------------------------------------------------------------------------------------------------
	//! A new seat: queue it, then try the move now. The caller saves orbat_live.json.
	void OnAssigned(CGQC_GU_Manager mgr, CGQC_GU_SeatAssignment a)
	{
		if (!SCR_GroupsManagerComponent.GetInstance())
		{
			a.pending = false;
			return;
		}

		a.pending = true;
		TryApply(mgr, a);
	}

	//------------------------------------------------------------------------------------------------
	//! true when `pending` changed. Logs why a move waits.
	protected bool TryApply(CGQC_GU_Manager mgr, CGQC_GU_SeatAssignment a)
	{
		bool wasPending = a.pending;
		string why = Move(mgr, a);
		if (why.IsEmpty())
		{
			a.pending = false;
			return wasPending;
		}

		a.pending = true;
		CGQC_GU_Log.Info(string.Format("ORBAT : %1 (%2) en attente de groupe : %3.", a.seat, a.uid, why));
		return !wasPending;
	}

	//------------------------------------------------------------------------------------------------
	//! "" when the player is in the seat's group, else the reason it waits.
	protected string Move(CGQC_GU_Manager mgr, CGQC_GU_SeatAssignment a)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return string.Empty;	// no groups in this scenario: the seat stays, nobody moves

		CGQC_GU_Unit unit = mgr.m_Catalogues.FindUnit(a.unit);
		CGQC_GU_Section section = FindSectionIn(unit, a.section);
		if (!section)
			return "section inconnue";

		int playerId = mgr.GetPlayerIdForUid(a.uid);
		if (playerId <= 0)
			return "hors ligne";

		SCR_PlayerControllerGroupComponent groupComp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (!groupComp)
			return "contrôleur absent";

		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerId);
		Faction faction = UnitFaction(unit);
		if (!faction)
			faction = playerFaction;

		if (!faction)
			return "aucune faction";

		if (playerFaction != faction)
			return "faction " + faction.GetFactionKey() + " attendue";

		SCR_AIGroup group = FindOrCreate(mgr, groups, unit, section, faction);
		if (!group)
			return "groupe non créé";

		if (!group.IsPlayerInGroup(playerId))
		{
			if (group.IsFull())
				return "groupe complet";

			// Server side: run the join request body directly (same checks as the group menu).
			groupComp.RPC_AskJoinGroup(group.GetGroupID());
			if (!group.IsPlayerInGroup(playerId))
				return "entrée refusée";

			CGQC_GU_Log.Info(string.Format("ORBAT : %1 (%2) placé dans le groupe « %3 ».", a.seat, a.uid, GroupName(mgr, unit, section)));
		}

		UpdateLeader(mgr, groups, group, unit, section);
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! The holder of the group's first seat leads it when they are in it.
	protected void UpdateLeader(CGQC_GU_Manager mgr, SCR_GroupsManagerComponent groups, SCR_AIGroup group, CGQC_GU_Unit unit, CGQC_GU_Section section)
	{
		string first = FirstSeat(unit, section);
		if (first.IsEmpty())
			return;

		CGQC_GU_SeatAssignment holder = CGQC_GU_OrbatRequests.FindBySeat(mgr.m_OrbatLive, unit.id, first);
		if (!holder)
			return;

		int leaderId = mgr.GetPlayerIdForUid(holder.uid);
		if (leaderId <= 0 || !group.IsPlayerInGroup(leaderId) || group.GetLeaderID() == leaderId)
			return;

		groups.SetGroupLeader(group.GetGroupID(), leaderId);
		CGQC_GU_Log.Info(string.Format("ORBAT : %1 (%2) chef du groupe « %3 ».", first, holder.uid, GroupName(mgr, unit, section)));
		// The game clears the custom name on a leader change: OnGroupNameChanged names it again.
	}

	//------------------------------------------------------------------------------------------------
	// Groups
	//------------------------------------------------------------------------------------------------
	protected SCR_AIGroup FindOrCreate(CGQC_GU_Manager mgr, SCR_GroupsManagerComponent groups, CGQC_GU_Unit unit, CGQC_GU_Section section, Faction faction)
	{
		string key = GroupKey(unit, section);
		string factionKey = faction.GetFactionKey();
		string name = GroupName(mgr, unit, section);

		CGQC_GU_GameGroupRef gr = FindRef(unit.id, key, factionKey);
		SCR_AIGroup group;
		if (gr)
		{
			group = groups.FindGroup(gr.m_iGroupId);
			if (group && group.GetFaction() != faction)
				group = null;
		}

		// Not known (or gone): a group of the faction already carrying the name, not one of ours.
		if (!group)
		{
			array<SCR_AIGroup> factionGroups = groups.GetPlayableGroupsByFaction(faction);
			if (factionGroups)
			{
				string wanted = CGQC_GU_Str.Lower(name);
				foreach (SCR_AIGroup g : factionGroups)
				{
					if (g && CGQC_GU_Str.Lower(g.GetCustomName()) == wanted && !FindRefById(g.GetGroupID()))
					{
						group = g;
						break;
					}
				}
			}
		}

		bool created;
		if (!group)
		{
			group = groups.CreateNewPlayableGroup(faction);
			if (!group)
				return null;

			created = true;
		}

		if (!gr)
		{
			gr = new CGQC_GU_GameGroupRef();
			gr.m_sUnit = unit.id;
			gr.m_sKey = key;
			gr.m_sFaction = factionKey;
			m_aGroups.Insert(gr);
		}

		gr.m_iGroupId = group.GetGroupID();
		gr.m_sName = name;

		// Kept when empty, open, sized to the seats, named with the section.
		group.SetCanDeleteIfNoPlayer(false);
		if (group.IsPrivate() && group.IsPrivacyChangeable())
			group.SetPrivate(false);

		group.SetMaxMembers(Capacity(unit, section));
		if (group.GetCustomName() != name)
			group.SetCustomName(name, 0);

		if (created)
			CGQC_GU_Log.Info(string.Format("ORBAT : groupe « %1 » créé (faction %2, %3 place(s)).", name, factionKey, Capacity(unit, section)));

		return group;
	}

	//------------------------------------------------------------------------------------------------
	//! After orbat.rename: the section's groups take the new name.
	void RefreshNames(CGQC_GU_Manager mgr)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;

		foreach (CGQC_GU_GameGroupRef gr : m_aGroups)
		{
			CGQC_GU_Unit unit = mgr.m_Catalogues.FindUnit(gr.m_sUnit);
			if (!unit)
				continue;

			CGQC_GU_Section section = FindSectionIn(unit, gr.m_sKey);
			if (!section && unit.sections.Count() > 0)
				section = unit.sections[0];

			if (!section)
				continue;

			gr.m_sName = GroupName(mgr, unit, section);
			SCR_AIGroup group = groups.FindGroup(gr.m_iGroupId);
			if (group && group.GetCustomName() != gr.m_sName)
				group.SetCustomName(gr.m_sName, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The game cleared one of our names (leader change, group emptied): name it again.
	protected void OnGroupNameChanged(SCR_AIGroup group)
	{
		if (!group || !Replication.IsServer() || !group.GetCustomName().IsEmpty())
			return;

		if (FindRefById(group.GetGroupID()))
			GetGame().GetCallqueue().CallLater(Rename, RENAME_DELAY_MS, false, group.GetGroupID());
	}

	//------------------------------------------------------------------------------------------------
	protected void Rename(int groupId)
	{
		CGQC_GU_GameGroupRef gr = FindRefById(groupId);
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!gr || !groups)
			return;

		SCR_AIGroup group = groups.FindGroup(groupId);
		if (group && group.GetCustomName().IsEmpty())
			group.SetCustomName(gr.m_sName, 0);
	}

	//------------------------------------------------------------------------------------------------
	// Helpers
	//------------------------------------------------------------------------------------------------
	//! A unit with student seats (no fixed seats) is one group; otherwise one group per section.
	static bool IsUnitWide(CGQC_GU_Unit unit)
	{
		foreach (CGQC_GU_Section s : unit.sections)
		{
			if (CGQC_GU_OrbatRequests.IsOpen(s))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static string GroupKey(CGQC_GU_Unit unit, CGQC_GU_Section section)
	{
		if (IsUnitWide(unit))
			return unit.id;

		return section.id;
	}

	//------------------------------------------------------------------------------------------------
	static string GroupName(CGQC_GU_Manager mgr, CGQC_GU_Unit unit, CGQC_GU_Section section)
	{
		if (IsUnitWide(unit))
			return unit.label;

		return CGQC_GU_OrbatRequests.SectionLabel(mgr, section);
	}

	//------------------------------------------------------------------------------------------------
	//! Seat count; 0 (no limit) for a unit-wide group with student seats.
	static int Capacity(CGQC_GU_Unit unit, CGQC_GU_Section section)
	{
		if (IsUnitWide(unit))
			return 0;

		int n;
		foreach (CGQC_GU_Element e : section.elements)
		{
			n += e.seats.Count();
		}

		return n;
	}

	//------------------------------------------------------------------------------------------------
	//! The group's first seat (S0-0 for Spartan, I-0 for UPO).
	static string FirstSeat(CGQC_GU_Unit unit, CGQC_GU_Section section)
	{
		array<CGQC_GU_Section> sections = {};
		if (IsUnitWide(unit))
		{
			foreach (CGQC_GU_Section s : unit.sections)
			{
				if (!s.command)
					sections.Insert(s);
			}
		}
		else
		{
			sections.Insert(section);
		}

		foreach (CGQC_GU_Section s : sections)
		{
			// Live seats (UPO, 4e): seat 0 leads, « I-0 », the teacher (owner).
			if (CGQC_GU_OrbatRequests.IsOpen(s))
				return s.letter + "-" + s.firstNumber.ToString();

			foreach (CGQC_GU_Element e : s.elements)
			{
				if (e.seats.Count() > 0)
					return e.seats[0].callsign;
			}
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! The unit's faction in this scenario, looked up by key; null when the scenario has none.
	static Faction UnitFaction(CGQC_GU_Unit unit)
	{
		if (!unit || unit.faction.IsEmpty())
			return null;

		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return null;

		return fm.GetFactionByKey(unit.faction);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Section FindSectionIn(CGQC_GU_Unit unit, string sectionId)
	{
		if (!unit)
			return null;

		foreach (CGQC_GU_Section s : unit.sections)
		{
			if (s.id == sectionId)
				return s;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_PlayerFactionAffiliationComponent FactionComp(int playerId)
	{
		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!pc)
			return null;

		return SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_GameGroupRef FindRef(string unitId, string key, string factionKey)
	{
		foreach (CGQC_GU_GameGroupRef gr : m_aGroups)
		{
			if (gr.m_sUnit == unitId && gr.m_sKey == key && gr.m_sFaction == factionKey)
				return gr;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_GameGroupRef FindRefById(int groupId)
	{
		foreach (CGQC_GU_GameGroupRef gr : m_aGroups)
		{
			if (gr.m_iGroupId == groupId)
				return gr;
		}

		return null;
	}
}
