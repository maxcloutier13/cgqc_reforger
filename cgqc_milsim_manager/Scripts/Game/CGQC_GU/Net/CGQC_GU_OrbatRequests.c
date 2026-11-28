// Gestion d'unité - Phase 4a: ORBAT state on the server (no game groups yet, that is 4b).
// Owner answers 2026-11-27: members take a free, unlocked seat in an active section within their rank
// and free only their own; one seat per member in all. Commandement / Zeus (orbat.assign) assign others,
// above rank, activate, rename, set roles and lock. A locked holder cannot leave or switch, and nobody
// else takes the seat. Anyone may take a UPO instructor seat; UPO seats are never HORS GRADE.
// Every write replies with the fresh ORBAT.

class CGQC_GU_OrbatSeatDto : Managed
{
	string callsign;	// S1-1, I-2, E-3
	string role;		// seat role id
	string roleLabel;	// « Fusilier »
	string binome;		// white | red | green | blue | yellow | ""
	string uid;			// "" = libre
	string name;		// « Cpl. Roussel »
	bool online;
	bool horsGrade;
	bool locked;
	bool mine;
	bool pending;		// game group move still to apply (4b)
	string teaches;		// UPO instructor seat: qualifications the holder may teach, « Q-MB, Q-SI » (4e)
	string progress;	// UPO student seat: « Q-MB · en cours » (4e)
}

class CGQC_GU_OrbatElementDto : Managed
{
	string id;			// S1
	string label;		// « Équipe 1 »
	ref array<ref CGQC_GU_OrbatSeatDto> seats = new array<ref CGQC_GU_OrbatSeatDto>();
}

class CGQC_GU_OrbatSectionDto : Managed
{
	string id;
	string label;		// with the rename applied
	string baseLabel;	// units.json name
	string letter;
	string kind;
	bool active;
	bool command;		// Zeus: shown apart, no seats
	bool open;			// live seats (UPO, 4e): seats added and removed during the game
	string seatRole;	// live seats: the seats' role (instructeur | eleve)
	int seatCount;
	int filled;
	ref array<ref CGQC_GU_OrbatElementDto> elements = new array<ref CGQC_GU_OrbatElementDto>();
}

class CGQC_GU_OrbatUnitDto : Managed
{
	string id;
	string label;
	string fullName;
	string patch;
	string faction;
	bool available;		// false while a_venir (GRIT)
	bool noRankCheck;	// UPO: never HORS GRADE
	ref array<ref CGQC_GU_OrbatSectionDto> sections = new array<ref CGQC_GU_OrbatSectionDto>();
}

class CGQC_GU_OrbatPersonDto : Managed
{
	string uid;
	string name;
	bool online;		// members[] only (4d picker)
	string seat;		// members[] only: current callsign, "" = none
	int rankOrder;		// members[] only, -1 = no rank
}

class CGQC_GU_OrbatRoleDto : Managed
{
	string id;
	string label;
	string minRankAbbr;	// "" = no minimum
	int minRankOrder;	// -1 = no minimum
}

class CGQC_GU_OrbatDto : Managed
{
	bool canManage;		// orbat.assign (display only; every write is checked again)
	int online;
	string myUnit;
	string mySeat;		// "" when the caller has no seat
	bool myLocked;
	ref array<ref CGQC_GU_OrbatUnitDto> units = new array<ref CGQC_GU_OrbatUnitDto>();
	ref array<ref CGQC_GU_OrbatPersonDto> zeus = new array<ref CGQC_GU_OrbatPersonDto>();		// online Zeus group, no seat
	ref array<ref CGQC_GU_OrbatPersonDto> unassigned = new array<ref CGQC_GU_OrbatPersonDto>();	// online, no seat, not Zeus
	ref array<ref CGQC_GU_OrbatRoleDto> roles = new array<ref CGQC_GU_OrbatRoleDto>();
	ref array<ref CGQC_GU_OrbatPersonDto> members = new array<ref CGQC_GU_OrbatPersonDto>();	// canManage only: everyone, online first
}

//! Where a callsign points. seat is null for an open section (UPO students).
class CGQC_GU_SeatRef
{
	CGQC_GU_Unit m_Unit;
	CGQC_GU_Section m_Section;
	CGQC_GU_Seat m_Seat;
	string m_sCallsign;		// "E" alone = next free student number
}

//! Where a member sits this game (4f): the fiche, the pause panel header and Membres « Ce soir ».
class CGQC_GU_SeatInfo
{
	string callsign;	// « S1-1 »; « Zeus » for an online Zeus member without a seat; "" = none
	string roleLabel;	// « Fusilier »
	string section;		// « Spartan-1 » (rename applied, element number); the unit for live seats (« UPO »)
	string leaderLabel;	// « CHEF D'ÉQUIPE »; "" when the member leads, or has no seat
	string leader;		// « S1-0 Adju. Tremblay », « S1-0 libre »

	//------------------------------------------------------------------------------------------------
	bool HasSeat()
	{
		return !roleLabel.IsEmpty() || !section.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! « S1-1 · Fusilier · Spartan »; « Zeus »; "".
	string Line()
	{
		if (!HasSeat())
			return callsign;

		return callsign + " · " + roleLabel + " · " + section;
	}

	//------------------------------------------------------------------------------------------------
	//! Fiche: « Ce soir : S1-1 · Fusilier (Spartan) », « Ce soir : Zeus », « Ce soir : sans place ».
	string Tonight()
	{
		if (HasSeat())
			return "Ce soir : " + callsign + " · " + roleLabel + " (" + section + ")";

		if (!callsign.IsEmpty())
			return "Ce soir : " + callsign;

		return "Ce soir : sans place";
	}
}

class CGQC_GU_OrbatRequests
{
	static const string GET = "orbat.get";				// (none)
	static const string ACTIVATE = "orbat.activate";	// section, on (0|1)                     orbat.assign
	static const string RENAME = "orbat.rename";		// section, name ("" = units.json name)  orbat.assign
	static const string ROLE = "orbat.role";			// seat, role (id or label, "" = default) [unit] orbat.assign
	static const string ASSIGN = "orbat.assign";		// seat [unit] [uid] [lock=1]  self, or orbat.assign for others
	static const string FREE = "orbat.free";			// [seat [unit] | uid]  own seat, or orbat.assign
	static const string LOCK = "orbat.lock";			// seat [unit] | uid, on (0|1, default 1)  orbat.assign
	static const string SEATS = "orbat.seats";			// section (add) | seat [unit] (remove), op add|remove  orbat.assign

	static const string ZEUS_GROUP = "zeus";
	static const string STUDENT_ROLE = "eleve";
	static const int MAX_SECTION_NAME = 24;

	//------------------------------------------------------------------------------------------------
	//! null when the action is not one of ours.
	static CGQC_GU_RequestResult Handle(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string action, CGQC_GU_Payload p)
	{
		switch (action)
		{
			case GET:		return Reply(mgr, caller, "ok");
			case ACTIVATE:	return Activate(mgr, caller, p);
			case RENAME:	return Rename(mgr, caller, p);
			case ROLE:		return SetRole(mgr, caller, p);
			case ASSIGN:	return Assign(mgr, caller, p);
			case FREE:		return Free(mgr, caller, p);
			case LOCK:		return Lock(mgr, caller, p);
			case SEATS:		return Seats(mgr, caller, p);
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Short names for the test console: « #gu as Roussel assign S1-1 ».
	static string FullAction(string action)
	{
		switch (action)
		{
			case "get": case "activate": case "rename": case "role": case "assign": case "free": case "lock": case "seats":
				return "orbat." + action;
		}

		return action;
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Activate(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		CGQC_GU_Unit unit;
		CGQC_GU_Section section = FindSection(mgr, p.Get("section"), unit);
		if (!section || section.command)
			return Fail("unknown_section");

		if (unit.state != "actif")
			return Fail("unit_unavailable");

		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		bool on = p.GetBool("on");
		if (on == live.activeSections.Contains(section.id))
			return Reply(mgr, caller, "unchanged");

		int freed;
		if (on)
		{
			live.activeSections.Insert(section.id);
		}
		else
		{
			live.activeSections.RemoveItem(section.id);
			// Unticking a section frees its seats (question 6).
			for (int i = live.assignments.Count() - 1; i >= 0; i--)
			{
				if (live.assignments[i].section == section.id)
				{
					live.assignments.RemoveOrdered(i);
					freed++;
				}
			}
		}

		if (!CGQC_GU_Storage.SaveOrbatLive(live))
			return Fail("write_failed");

		CGQC_GU_Log.Info(string.Format("ORBAT : section %1 %2 par %3 (%4 place(s) libérée(s)).", section.id, OnOff(on), caller.displayName, freed));
		return Reply(mgr, caller, "ok");
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Rename(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		CGQC_GU_Unit unit;
		CGQC_GU_Section section = FindSection(mgr, p.Get("section"), unit);
		if (!section || section.command)
			return Fail("unknown_section");

		string name = p.Get("name").Trim();
		if (name.Length() > MAX_SECTION_NAME)
			return Fail("too_long");

		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		if (name.IsEmpty() || name == section.label)
			live.renames.Remove(section.id);
		else
			live.renames.Set(section.id, name);

		if (!CGQC_GU_Storage.SaveOrbatLive(live))
			return Fail("write_failed");

		mgr.m_Groups.RefreshNames(mgr);
		CGQC_GU_Log.Info(string.Format("ORBAT : section %1 renommée « %2 » par %3.", section.id, SectionLabel(mgr, section), caller.displayName));
		return Reply(mgr, caller, "ok");
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult SetRole(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		CGQC_GU_SeatRef sr = Resolve(mgr, p.Get("unit"), p.Get("seat"));
		if (!sr || sr.m_sCallsign == sr.m_Section.letter)
			return Fail("unknown_seat");

		string key = RoleKey(sr.m_Unit.id, sr.m_sCallsign);
		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		string roleText = p.Get("role").Trim();
		if (roleText.IsEmpty())
		{
			live.roles.Remove(key);
		}
		else
		{
			CGQC_GU_SeatRole role = mgr.m_Catalogues.FindSeatRole(roleText);
			if (!role)
				return Fail("unknown_role");

			if ((sr.m_Seat && sr.m_Seat.role == role.id) || (!sr.m_Seat && sr.m_Section.seatRole == role.id))
				live.roles.Remove(key);
			else
				live.roles.Set(key, role.id);
		}

		if (!CGQC_GU_Storage.SaveOrbatLive(live))
			return Fail("write_failed");

		CGQC_GU_Log.Info(string.Format("ORBAT : rôle de %1 = %2 par %3.", sr.m_sCallsign, mgr.m_Catalogues.SeatRoleLabel(SeatRole(mgr, sr)), caller.displayName));
		return Reply(mgr, caller, "ok");
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Assign(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		bool manage = CanManage(mgr, caller);
		CGQC_GU_Member target = caller;
		if (!p.Get("uid").IsEmpty())
			target = mgr.FindMember(p.Get("uid"));

		if (!target)
			return Fail("not_found");

		bool isSelf = target.uid == caller.uid;
		if ((!isSelf || p.Has("lock")) && !manage)
			return Fail("forbidden");

		CGQC_GU_SeatRef sr = Resolve(mgr, p.Get("unit"), p.Get("seat"));
		if (!sr)
			return Fail("unknown_seat");

		if (sr.m_Unit.state != "actif")
			return Fail("unit_unavailable");

		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		if (!live.activeSections.Contains(sr.m_Section.id))
			return Fail("inactive_section");

		// The letter alone (« E »): the lowest empty seat of that live section; numbers never shift.
		if (sr.m_sCallsign == sr.m_Section.letter)
		{
			sr.m_sCallsign = FirstFreeSeat(mgr, sr.m_Unit, sr.m_Section);
			if (sr.m_sCallsign.IsEmpty())
				return Fail("occupied");
		}

		bool doLock = manage && p.GetBool("lock");
		CGQC_GU_SeatAssignment current = FindByUid(live, target.uid);
		CGQC_GU_SeatAssignment holder = FindBySeat(live, sr.m_Unit.id, sr.m_sCallsign);
		if (holder && holder.uid == target.uid)
		{
			if (!p.Has("lock") || holder.locked == doLock)
				return Reply(mgr, caller, "unchanged");

			holder.locked = doLock;
			return Saved(mgr, caller, string.Format("ORBAT : %1 %2 par %3.", sr.m_sCallsign, LockWord(doLock), caller.displayName));
		}

		if (!manage)
		{
			if (current && current.locked)
				return Fail("locked");

			if (holder && holder.locked)
				return Fail("locked");

			if (holder)
				return Fail("occupied");

			if (!sr.m_Unit.noRankCheck && !MeetsRank(mgr.m_Catalogues, target, SeatRole(mgr, sr)))
				return Fail("above_rank");
		}

		// Commandement / Zeus reassign: whoever held the seat is freed, locked or not.
		if (holder)
			live.assignments.RemoveItem(holder);

		// One seat per member, CGQC or UPO.
		if (current)
			live.assignments.RemoveItem(current);

		CGQC_GU_SeatAssignment a = new CGQC_GU_SeatAssignment();
		a.uid = target.uid;
		a.unit = sr.m_Unit.id;
		a.section = sr.m_Section.id;
		a.seat = sr.m_sCallsign;
		a.by = caller.uid;
		a.at = CGQC_GU_Time.NowUtc();
		a.locked = doLock;
		live.assignments.Insert(a);
		mgr.m_Groups.OnAssigned(mgr, a);

		string fromSeat;
		if (current)
			fromSeat = " (quitte " + current.seat + ")";

		string displaced;
		if (holder)
			displaced = string.Format(", %1 libéré", holder.uid);

		return Saved(mgr, caller, string.Format("ORBAT : %1 -> %2%3%4 par %5%6.", target.displayName, a.seat, fromSeat, displaced, caller.displayName, LockSuffix(doLock)));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Free(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		bool manage = CanManage(mgr, caller);
		CGQC_GU_SeatAssignment a = FindTarget(mgr, caller, p);
		if (!a)
			return Fail("empty_seat");

		if (!manage)
		{
			if (a.uid != caller.uid)
				return Fail("forbidden");

			if (a.locked)
				return Fail("locked");
		}

		string seat = a.seat;
		string uid = a.uid;
		mgr.m_OrbatLive.assignments.RemoveItem(a);
		return Saved(mgr, caller, string.Format("ORBAT : %1 libérée (%2) par %3.", seat, uid, caller.displayName));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Lock(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		CGQC_GU_SeatAssignment a = FindTarget(mgr, caller, p);
		if (!a)
			return Fail("empty_seat");

		bool doLock = true;
		if (p.Has("on"))
			doLock = p.GetBool("on");

		if (a.locked == doLock)
			return Reply(mgr, caller, "unchanged");

		a.locked = doLock;
		return Saved(mgr, caller, string.Format("ORBAT : %1 %2 par %3.", a.seat, LockWord(doLock), caller.displayName));
	}

	//------------------------------------------------------------------------------------------------
	//! UPO live seats (4e): add the lowest missing number, or remove a seat (its holder is freed).
	protected static CGQC_GU_RequestResult Seats(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		if (p.Get("op") == "add")
		{
			CGQC_GU_Unit unit;
			CGQC_GU_Section section = FindSection(mgr, p.Get("section"), unit);
			if (!section || !IsOpen(section))
				return Fail("unknown_section");

			array<int> numbers = LiveNumbers(live, section);
			int n = section.firstNumber;
			while (numbers.Contains(n))
			{
				n++;
			}

			numbers.Insert(n);
			numbers.Sort();
			return Saved(mgr, caller, string.Format("ORBAT : place %1-%2 ajoutée par %3.", section.letter, n, caller.displayName));
		}

		CGQC_GU_SeatRef sr = Resolve(mgr, p.Get("unit"), p.Get("seat"));
		if (!sr || !IsOpen(sr.m_Section) || sr.m_sCallsign == sr.m_Section.letter)
			return Fail("unknown_seat");

		CGQC_GU_SeatAssignment holder = FindBySeat(live, sr.m_Unit.id, sr.m_sCallsign);
		if (holder)
			live.assignments.RemoveItem(holder);

		live.roles.Remove(RoleKey(sr.m_Unit.id, sr.m_sCallsign));
		LiveNumbers(live, sr.m_Section).RemoveItem(SeatNumber(sr.m_sCallsign, sr.m_Section.letter));
		return Saved(mgr, caller, string.Format("ORBAT : place %1 retirée par %2.", sr.m_sCallsign, caller.displayName));
	}

	//------------------------------------------------------------------------------------------------
	// Reading
	//------------------------------------------------------------------------------------------------
	static string Build(CGQC_GU_Manager mgr, CGQC_GU_Member caller)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		CGQC_GU_OrbatDto dto = new CGQC_GU_OrbatDto();
		dto.canManage = CanManage(mgr, caller);
		dto.online = mgr.GetOnlineCount();

		CGQC_GU_SeatAssignment mine = FindByUid(live, caller.uid);
		if (mine)
		{
			dto.myUnit = mine.unit;
			dto.mySeat = mine.seat;
			dto.myLocked = mine.locked;
		}

		foreach (CGQC_GU_Unit unit : cat.m_aUnits)
		{
			CGQC_GU_OrbatUnitDto u = new CGQC_GU_OrbatUnitDto();
			u.id = unit.id;
			u.label = unit.label;
			u.fullName = unit.fullName;
			u.patch = unit.patch;
			u.faction = unit.faction;
			u.available = unit.state == "actif";
			u.noRankCheck = unit.noRankCheck;
			foreach (CGQC_GU_Section section : unit.sections)
			{
				u.sections.Insert(SectionDto(mgr, caller, unit, section));
			}

			dto.units.Insert(u);
		}

		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			if (!mgr.IsOnline(uid) || FindByUid(live, uid))
				continue;

			CGQC_GU_OrbatPersonDto person = new CGQC_GU_OrbatPersonDto();
			person.uid = uid;
			person.name = NameOf(cat, m);
			if (m.accessGroups.Contains(ZEUS_GROUP))
				dto.zeus.Insert(person);
			else
				dto.unassigned.Insert(person);
		}

		foreach (CGQC_GU_SeatRole role : cat.m_aSeatRoles)
		{
			CGQC_GU_OrbatRoleDto r = new CGQC_GU_OrbatRoleDto();
			r.id = role.id;
			r.label = role.label;
			r.minRankOrder = -1;
			CGQC_GU_Rank minRank = cat.FindRank(role.minRank);
			if (minRank)
			{
				r.minRankAbbr = minRank.abbr;
				r.minRankOrder = minRank.order;
			}

			dto.roles.Insert(r);
		}

		if (dto.canManage)
			MemberPicks(mgr, dto.members);

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("o", dto);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	//! « Donner la place à » (4d): members without a seat; online first, then rank (senior first), then last name.
	protected static void MemberPicks(CGQC_GU_Manager mgr, array<ref CGQC_GU_OrbatPersonDto> list)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		array<string> keys = {};
		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			if (FindByUid(live, uid))
				continue;

			CGQC_GU_OrbatPersonDto d = new CGQC_GU_OrbatPersonDto();
			d.uid = uid;
			d.name = NameOf(cat, m);
			d.online = mgr.IsOnline(uid);
			d.rankOrder = cat.GetRankOrder(m.rank);

			string last = m.lastName;
			if (last.IsEmpty())
				last = m.displayName;

			string key = CGQC_GU_Str.Lower(last + " " + m.displayName);
			int at = list.Count();
			for (int i = 0; i < list.Count(); i++)
			{
				CGQC_GU_OrbatPersonDto o = list[i];
				bool before = d.online && !o.online;
				if (d.online == o.online)
				{
					before = d.rankOrder > o.rankOrder;
					if (d.rankOrder == o.rankOrder)
						before = key.Compare(keys[i]) < 0;
				}

				if (before)
				{
					at = i;
					break;
				}
			}

			list.InsertAt(d, at);
			keys.InsertAt(key, at);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_OrbatSectionDto SectionDto(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Unit unit, CGQC_GU_Section section)
	{
		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		CGQC_GU_OrbatSectionDto s = new CGQC_GU_OrbatSectionDto();
		s.id = section.id;
		s.label = SectionLabel(mgr, section);
		s.baseLabel = section.label;
		s.letter = section.letter;
		s.kind = section.kind;
		s.active = live.activeSections.Contains(section.id);
		s.command = section.command;
		s.open = IsOpen(section);
		s.seatRole = section.seatRole;

		foreach (CGQC_GU_Element element : section.elements)
		{
			CGQC_GU_OrbatElementDto e = new CGQC_GU_OrbatElementDto();
			e.id = element.id;
			e.label = element.label;
			foreach (CGQC_GU_Seat seat : element.seats)
			{
				CGQC_GU_OrbatSeatDto seatDto = SeatDto(mgr, caller, unit, seat.callsign, RoleOf(live, unit.id, seat.callsign, seat.role), seat.binome);
				s.seatCount++;
				if (!seatDto.uid.IsEmpty())
					s.filled++;

				e.seats.Insert(seatDto);
			}

			s.elements.Insert(e);
		}

		if (s.open)
		{
			// Live seats: one element, the seats of this game in number order, empty or held.
			CGQC_GU_OrbatElementDto seats = new CGQC_GU_OrbatElementDto();
			seats.id = section.letter;
			seats.label = section.label;
			foreach (int n : LiveNumbers(live, section))
			{
				string callsign = section.letter + "-" + n.ToString();
				CGQC_GU_OrbatSeatDto liveDto = SeatDto(mgr, caller, unit, callsign, RoleOf(live, unit.id, callsign, section.seatRole), string.Empty);
				s.seatCount++;
				if (!liveDto.uid.IsEmpty())
					s.filled++;

				seats.seats.Insert(liveDto);
			}

			s.elements.Insert(seats);
		}

		return s;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_OrbatSeatDto SeatDto(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Unit unit, string callsign, string role, string binome)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_OrbatSeatDto d = new CGQC_GU_OrbatSeatDto();
		d.callsign = callsign;
		d.role = role;
		d.roleLabel = cat.SeatRoleLabel(role);
		d.binome = binome;

		CGQC_GU_SeatAssignment a = FindBySeat(mgr.m_OrbatLive, unit.id, callsign);
		if (!a)
			return d;

		d.uid = a.uid;
		d.locked = a.locked;
		d.pending = a.pending;
		d.mine = caller && a.uid == caller.uid;
		d.online = mgr.IsOnline(a.uid);
		CGQC_GU_Member holder = mgr.FindMember(a.uid);
		if (holder)
		{
			d.name = NameOf(cat, holder);
			d.horsGrade = !unit.noRankCheck && !MeetsRank(cat, holder, role);
			if (unit.noRankCheck)
			{
				if (role == STUDENT_ROLE)
					d.progress = StudentProgress(mgr, holder);
				else
					d.teaches = Teaches(holder);
			}
		}
		else
		{
			d.name = a.uid;
		}

		return d;
	}

	//------------------------------------------------------------------------------------------------
	//! UPO « Enseigne : … » (4e): the holder's teaching authorizations, in the order given.
	protected static string Teaches(CGQC_GU_Member m)
	{
		array<string> codes = new array<string>();
		foreach (CGQC_GU_InstructorAuth auth : m.instructorFor)
		{
			if (!codes.Contains(auth.qual))
				codes.Insert(auth.qual);
		}

		return CGQC_GU_Str.Join(codes, ", ");
	}

	//------------------------------------------------------------------------------------------------
	//! UPO student line (4e): the first qualification under way (« Q-MB · en cours ») or to retake
	//! (« Q-MB · reprise E2 », E = attempt number), else the next one the rank asks for (« · à faire »).
	protected static string StudentProgress(CGQC_GU_Manager mgr, CGQC_GU_Member m)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		foreach (CGQC_GU_Qualification q : cat.m_aQualifications)
		{
			if (q.state != "actif")
				continue;

			string state = CGQC_GU_Quals.InsigniaStateFull(mgr, m, q.code);
			if (state == CGQC_GU_Quals.STATE_RUNNING)
				return q.code + " · en cours";

			if (state == CGQC_GU_Quals.STATE_RETAKE)
			{
				int tries = 1;
				foreach (CGQC_GU_QualResult r : m.qualifications)
				{
					if (r.qual == q.code && r.result == CGQC_GU_Quals.RESULT_FAILED)
						tries++;
				}

				return string.Format("%1 · reprise E%2", q.code, tries);
			}
		}

		CGQC_GU_Promotion p = CGQC_GU_Quals.NextPromotion(cat, m);
		if (p)
		{
			foreach (string code : p.requiredQuals)
			{
				if (!CGQC_GU_Quals.HasPassed(m, code))
					return code + " · à faire";
			}
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Console view (#gu orbat): active sections seat by seat, then the inactive ones on one line.
	static string Describe(CGQC_GU_Manager mgr, string unitFilter)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		string text;
		foreach (CGQC_GU_Unit unit : cat.m_aUnits)
		{
			if (unit.state != "actif" || (!unitFilter.IsEmpty() && CGQC_GU_Str.Lower(unitFilter) != unit.id))
				continue;

			text += string.Format("\n== %1 (%2)", unit.label, unit.faction);
			array<string> inactive = new array<string>();
			foreach (CGQC_GU_Section section : unit.sections)
			{
				if (section.command)
					continue;

				if (!live.activeSections.Contains(section.id))
				{
					inactive.Insert(SectionLabel(mgr, section));
					continue;
				}

				CGQC_GU_OrbatSectionDto s = SectionDto(mgr, null, unit, section);
				text += string.Format("\n%1 [%2] %3 / %4", CGQC_GU_Str.Upper(s.label), section.id, s.filled, s.seatCount);
				foreach (CGQC_GU_OrbatElementDto e : s.elements)
				{
					foreach (CGQC_GU_OrbatSeatDto seat : e.seats)
					{
						string who = "libre";
						if (!seat.uid.IsEmpty())
							who = seat.name;

						if (seat.locked)
							who += " VERROUILLÉ";

						if (seat.horsGrade)
							who += " HORS GRADE";

						CGQC_GU_SeatAssignment held = FindBySeat(live, unit.id, seat.callsign);
						if (held && held.pending)
							who += " EN ATTENTE";

						text += string.Format("\n  %1 %2 : %3", seat.callsign, seat.roleLabel, who);
					}
				}
			}

			if (!inactive.IsEmpty())
				text += "\nInactives : " + CGQC_GU_Str.Join(inactive, ", ");
		}

		if (text.IsEmpty())
			return "Unité inconnue : " + unitFilter;

		return "ORBAT" + text;
	}

	//------------------------------------------------------------------------------------------------
	// Helpers
	//------------------------------------------------------------------------------------------------
	protected static bool CanManage(CGQC_GU_Manager mgr, CGQC_GU_Member m)
	{
		return CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, m, CGQC_GU_Rights.ORBAT_ASSIGN);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Reply(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string code)
	{
		return new CGQC_GU_RequestResult(true, code, Build(mgr, caller));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Fail(string code)
	{
		return new CGQC_GU_RequestResult(false, code);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Saved(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string logLine)
	{
		if (!CGQC_GU_Storage.SaveOrbatLive(mgr.m_OrbatLive))
			return Fail("write_failed");

		CGQC_GU_Log.Info(logLine);
		return Reply(mgr, caller, "ok");
	}

	//------------------------------------------------------------------------------------------------
	static string NameOf(CGQC_GU_Catalogues cat, CGQC_GU_Member m)
	{
		CGQC_GU_Rank r = cat.FindRank(m.rank);
		if (r)
			return r.abbr + " " + m.displayName;

		return m.displayName;
	}

	//------------------------------------------------------------------------------------------------
	static string SectionLabel(CGQC_GU_Manager mgr, CGQC_GU_Section section)
	{
		string name;
		if (mgr.m_OrbatLive.renames.Find(section.id, name) && !name.IsEmpty())
			return name;

		return section.label;
	}

	//------------------------------------------------------------------------------------------------
	//! A live-seat section (UPO, 4e): seats « letter-n » added and removed during the game.
	static bool IsOpen(CGQC_GU_Section section)
	{
		return !section.command && section.liveSeats;
	}

	//------------------------------------------------------------------------------------------------
	//! The seat numbers of a live-seat section this game, made with the default count if missing.
	static array<int> LiveNumbers(CGQC_GU_OrbatLive live, CGQC_GU_Section section)
	{
		foreach (CGQC_GU_LiveSeats ls : live.liveSeats)
		{
			if (ls.section == section.id)
				return ls.numbers;
		}

		CGQC_GU_LiveSeats made = new CGQC_GU_LiveSeats();
		made.section = section.id;
		for (int i = 0; i < section.defaultSeats; i++)
		{
			made.numbers.Insert(section.firstNumber + i);
		}

		live.liveSeats.Insert(made);
		return made.numbers;
	}

	//------------------------------------------------------------------------------------------------
	//! Server start: every live-seat section back to its default count (I-0, I-1; E-1 to E-16).
	static void ResetLiveSeats(CGQC_GU_Manager mgr)
	{
		mgr.m_OrbatLive.liveSeats.Clear();
		foreach (CGQC_GU_Unit unit : mgr.m_Catalogues.m_aUnits)
		{
			foreach (CGQC_GU_Section section : unit.sections)
			{
				if (IsOpen(section))
					LiveNumbers(mgr.m_OrbatLive, section);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The lowest empty seat of a live-seat section, or "".
	protected static string FirstFreeSeat(CGQC_GU_Manager mgr, CGQC_GU_Unit unit, CGQC_GU_Section section)
	{
		foreach (int n : LiveNumbers(mgr.m_OrbatLive, section))
		{
			string callsign = section.letter + "-" + n.ToString();
			if (!FindBySeat(mgr.m_OrbatLive, unit.id, callsign))
				return callsign;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Section FindSection(CGQC_GU_Manager mgr, string id, out CGQC_GU_Unit unit)
	{
		string wanted = CGQC_GU_Str.Lower(id.Trim());
		foreach (CGQC_GU_Unit u : mgr.m_Catalogues.m_aUnits)
		{
			foreach (CGQC_GU_Section s : u.sections)
			{
				if (s.id == wanted)
				{
					unit = u;
					return s;
				}
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! A callsign (case-insensitive) in the given unit, or in any unit when unitId is empty.
	//! Open sections accept their letter alone (« E ») or « E-n ».
	static CGQC_GU_SeatRef Resolve(CGQC_GU_Manager mgr, string unitId, string callsign)
	{
		string wanted = CGQC_GU_Str.Upper(callsign.Trim());
		string unitWanted = CGQC_GU_Str.Lower(unitId.Trim());
		if (wanted.IsEmpty())
			return null;

		foreach (CGQC_GU_Unit unit : mgr.m_Catalogues.m_aUnits)
		{
			if (!unitWanted.IsEmpty() && unit.id != unitWanted)
				continue;

			foreach (CGQC_GU_Section section : unit.sections)
			{
				if (section.command)
					continue;

				CGQC_GU_SeatRef sr = null;
				if (IsOpen(section))
				{
					if (wanted == section.letter || LiveNumbers(mgr.m_OrbatLive, section).Contains(SeatNumber(wanted, section.letter)))
					{
						sr = new CGQC_GU_SeatRef();
						sr.m_sCallsign = wanted;
					}
				}
				else
				{
					foreach (CGQC_GU_Element element : section.elements)
					{
						foreach (CGQC_GU_Seat seat : element.seats)
						{
							if (CGQC_GU_Str.Upper(seat.callsign) == wanted)
							{
								sr = new CGQC_GU_SeatRef();
								sr.m_Seat = seat;
								sr.m_sCallsign = seat.callsign;
							}
						}
					}
				}

				if (sr)
				{
					sr.m_Unit = unit;
					sr.m_Section = section;
					return sr;
				}
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! « E-3 » -> 3 and « I-0 » -> 0 for their letter; -1 when it is not that section's callsign.
	static int SeatNumber(string callsign, string letter)
	{
		string prefix = letter + "-";
		if (!callsign.StartsWith(prefix) || callsign.Length() <= prefix.Length())
			return -1;

		string digits = callsign.Substring(prefix.Length(), callsign.Length() - prefix.Length());
		for (int i = 0; i < digits.Length(); i++)
		{
			if ("0123456789".IndexOf(digits.Get(i)) < 0)
				return -1;
		}

		return digits.ToInt();
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_SeatAssignment FindByUid(CGQC_GU_OrbatLive live, string uid)
	{
		foreach (CGQC_GU_SeatAssignment a : live.assignments)
		{
			if (a.uid == uid)
				return a;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_SeatAssignment FindBySeat(CGQC_GU_OrbatLive live, string unitId, string callsign)
	{
		foreach (CGQC_GU_SeatAssignment a : live.assignments)
		{
			if (a.unit == unitId && a.seat == callsign)
				return a;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! The member's seat this game and who leads them (4f). The leader is the holder of the element's
	//! first seat (S1-0, S0-0, R1-1), or I-0 for live seats (UPO).
	static CGQC_GU_SeatInfo SeatInfo(CGQC_GU_Manager mgr, string uid)
	{
		CGQC_GU_SeatInfo info = new CGQC_GU_SeatInfo();
		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		CGQC_GU_SeatAssignment a = FindByUid(live, uid);
		if (!a)
		{
			CGQC_GU_Member m = mgr.FindMember(uid);
			if (m && mgr.IsOnline(uid) && m.accessGroups.Contains(ZEUS_GROUP))
				info.callsign = "Zeus";

			return info;
		}

		info.callsign = a.seat;
		CGQC_GU_SeatRef sr = Resolve(mgr, a.unit, a.seat);
		if (!sr)
			return info;

		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		info.roleLabel = cat.SeatRoleLabel(SeatRole(mgr, sr));

		string first;
		string firstRole;
		if (IsOpen(sr.m_Section))
		{
			info.section = sr.m_Unit.label;
			first = CGQC_GU_OrbatGroups.FirstSeat(sr.m_Unit, sr.m_Section);
			firstRole = "instructeur";
		}
		else
		{
			info.section = SectionLabel(mgr, sr.m_Section);
			foreach (CGQC_GU_Element element : sr.m_Section.elements)
			{
				if (element.seats.Count() > 0 && element.seats.Contains(sr.m_Seat))
				{
					first = element.seats[0].callsign;
					firstRole = element.seats[0].role;
					// « Spartan-1 » for element S1 (owner, 4f test).
					string letter = sr.m_Section.letter;
					if (element.id.StartsWith(letter) && element.id.Length() > letter.Length())
						info.section += "-" + element.id.Substring(letter.Length(), element.id.Length() - letter.Length());
				}
			}
		}

		if (first.IsEmpty() || first == a.seat)
			return info;

		switch (firstRole)
		{
			case "chef_section":	info.leaderLabel = "CHEF DE SECTION"; break;
			case "instructeur":		info.leaderLabel = "INSTRUCTEUR"; break;
			default:				info.leaderLabel = "CHEF D'ÉQUIPE"; break;
		}

		info.leader = first + " libre";
		CGQC_GU_SeatAssignment lead = FindBySeat(live, a.unit, first);
		if (lead)
		{
			CGQC_GU_Member holder = mgr.FindMember(lead.uid);
			if (holder)
				info.leader = first + " " + NameOf(cat, holder);
		}

		return info;
	}

	//------------------------------------------------------------------------------------------------
	//! The seat named in the payload (seat [+ unit]), else the member's (uid), else the caller's own.
	protected static CGQC_GU_SeatAssignment FindTarget(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_OrbatLive live = mgr.m_OrbatLive;
		if (!p.Get("seat").IsEmpty())
		{
			CGQC_GU_SeatRef sr = Resolve(mgr, p.Get("unit"), p.Get("seat"));
			if (!sr)
				return null;

			return FindBySeat(live, sr.m_Unit.id, sr.m_sCallsign);
		}

		if (!p.Get("uid").IsEmpty())
			return FindByUid(live, p.Get("uid"));

		return FindByUid(live, caller.uid);
	}

	//------------------------------------------------------------------------------------------------
	static string RoleKey(string unitId, string callsign)
	{
		return unitId + "/" + callsign;
	}

	//------------------------------------------------------------------------------------------------
	//! The role chosen for this game (« Rôle de la place »), else the catalogue role.
	static string RoleOf(CGQC_GU_OrbatLive live, string unitId, string callsign, string catalogueRole)
	{
		string chosen;
		if (live.roles.Find(RoleKey(unitId, callsign), chosen) && !chosen.IsEmpty())
			return chosen;

		return catalogueRole;
	}

	//------------------------------------------------------------------------------------------------
	protected static string SeatRole(CGQC_GU_Manager mgr, CGQC_GU_SeatRef sr)
	{
		string catalogueRole = sr.m_Section.seatRole;
		if (sr.m_Seat)
			catalogueRole = sr.m_Seat.role;

		return RoleOf(mgr.m_OrbatLive, sr.m_Unit.id, sr.m_sCallsign, catalogueRole);
	}

	//------------------------------------------------------------------------------------------------
	//! Rank at or above the role's minimum. A role with no minimum, or free text, has no limit.
	static bool MeetsRank(CGQC_GU_Catalogues cat, CGQC_GU_Member m, string role)
	{
		CGQC_GU_SeatRole r = cat.FindSeatRole(role);
		if (!r || r.minRank.IsEmpty())
			return true;

		int needed = cat.GetRankOrder(r.minRank);
		if (needed < 0)
			return true;

		return cat.GetRankOrder(m.rank) >= needed;
	}

	//------------------------------------------------------------------------------------------------
	protected static string OnOff(bool on)
	{
		if (on)
			return "activée";

		return "désactivée";
	}

	//------------------------------------------------------------------------------------------------
	protected static string LockWord(bool doLock)
	{
		if (doLock)
			return "verrouillée";

		return "déverrouillée";
	}

	//------------------------------------------------------------------------------------------------
	protected static string LockSuffix(bool doLock)
	{
		if (doLock)
			return ", verrouillée";

		return string.Empty;
	}
}
