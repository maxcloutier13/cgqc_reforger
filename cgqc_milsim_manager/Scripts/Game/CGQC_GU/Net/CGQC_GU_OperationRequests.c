// Gestion d'unité - Opérations (Phase 5a): operation orders (OPORD), server side only.
// Rights (owner answer 1): operations.manage creates, edits, publishes, returns to draft and deletes.
// Everyone reads PUBLIÉ and TERMINÉE orders; BROUILLON is seen by managers only.
// TERMINÉE is final (answer 3): read-only, never back, never deleted. It only comes from 5d.
//
// ops.save writes one section at a time (one tab or sub-page of the screen). Scalar fields change only
// when their key is sent ("" clears). A row list is replaced when its count key (« tn=0 ») or its first
// row is sent; rows are « <prefix><n>.<field> » from 1, so the console needs no escaping:
//   header     name number hq date briefing launch area map weather
//   situation  enemy friendly attachments mission
//   execution  task purpose endState concept coordination · t<n>.el t<n>.task
//   support    s<n>.type s<n>.details
//   cmdt       succ<n> (uid or name) · c<n>.callsign c<n>.el c<n>.chief c<n>.mhz
//   points     p<n>.name p<n>.grid p<n>.note
//   signals    challenge password running near · g<n>.text g<n>.meaning · P.means P.when P.mhz (A, C, E)

class CGQC_GU_OpSummary : Managed
{
	string id;
	string number;
	string name;
	string status;
	string statusLabel;		// BROUILLON / PUBLIÉ / TERMINÉE
	string date;			// AAAA-MM-JJ
	string launchTime;		// HH:MM or ""
	int participants;
	string deploymentId;
}

//! A member offered in the Chef and succession drop-downs (5c).
class CGQC_GU_OpPersonDto : Managed
{
	string uid;
	string name;		// rank + name
	string seat;		// « S1-1 » or ""
	bool leader;		// holds a Chef de section / Chef d'équipe seat
	bool zeus;			// never in the succession
	bool online;
}

//! Display values beside the stored order (« o » in the reply); every write is checked again.
class CGQC_GU_OpView : Managed
{
	string title;			// OPÉRATION FAUCON NOIR
	string byLine;			// OPORD 2026-014 · rédigé par Capt. Roy (Zeus)
	string statusLabel;
	string dateLabel;		// 261128
	string briefingLabel;	// 19 h 45 HE
	string launchLabel;		// 20 h 00 HE
	string timeZoneLabel;	// Heure de l'Est (HNE / HAE)
	string doneLine;		// Terminée le 261128 par … (5d)
	ref map<string, string> names = new map<string, string>();			// uid -> rank + name, every uid in the order
	ref map<string, string> elementLabels = new map<string, string>();	// element code -> label
	ref map<string, string> supportLabels = new map<string, string>();	// support type -> label
	bool canEdit;
	bool canManage;
	int index;				// place of this order among the visible ones, from 1
	int total;
	string prevId;
	string nextId;
	string nextNumber;		// what « Nouvel ordre » will get
	// Choices, sent to managers only (the reply stays small for readers).
	ref array<ref CGQC_GU_LabelItem> elementChoices = new array<ref CGQC_GU_LabelItem>();
	ref array<ref CGQC_GU_LabelItem> supportChoices = new array<ref CGQC_GU_LabelItem>();
	ref array<ref CGQC_GU_OpPersonDto> people = new array<ref CGQC_GU_OpPersonDto>();
}

class CGQC_GU_OperationRequests
{
	static const string LIST = "ops.list";
	static const string GET = "ops.get";			// id or number ("" = the current order, answer 2)
	static const string SAVE = "ops.save";			// id or number ("" = new), section, fields (see the top)
	static const string STATUS = "ops.status";		// id, status (published | draft)
	static const string DELETE = "ops.delete";		// id

	static const string SECTION_HEADER = "header";
	static const string SECTION_SITUATION = "situation";
	static const string SECTION_EXECUTION = "execution";
	static const string SECTION_SUPPORT = "support";
	static const string SECTION_CMDT = "cmdt";
	static const string SECTION_POINTS = "points";
	static const string SECTION_SIGNALS = "signals";

	static const string ELEMENT_QG = "qg";
	static const string ELEMENT_JTAC = "jtac";

	static const int MAX_SHORT = 60;		// names, QG, zone, map, callsign, signal, point name
	static const int MAX_LINE = 200;		// weather, attachments, one-line rows (task, note, meaning, PACE)
	static const int MAX_TEXT = 1000;		// multiline fields (situation, mission, intention, concept, coordination)
	static const int MAX_SUPPORT = 500;		// one support row's details
	static const int MAX_ROWS = 12;			// each row list
	static const int MAX_SUPPORT_ROWS = 8;
	static const int MAX_SUCCESSION = 5;
	static const int MAX_MHZ = 8;
	static const int MAX_GRID = 11;

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_RequestResult Handle(CGQC_GU_Manager mgr, CGQC_GU_Member caller, string action, CGQC_GU_Payload p)
	{
		switch (action)
		{
			case LIST:		return Ok(List(mgr, caller));
			case GET:		return Get(mgr, caller, p.Get("id"));
			case SAVE:		return Save(mgr, caller, p);
			case STATUS:	return SetStatus(mgr, caller, p);
			case DELETE:	return Delete(mgr, caller, p);
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Ok(string payload)
	{
		return new CGQC_GU_RequestResult(true, "ok", payload);
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Fail(string code)
	{
		return new CGQC_GU_RequestResult(false, code);
	}

	//------------------------------------------------------------------------------------------------
	// Rights
	//------------------------------------------------------------------------------------------------
	static bool CanManage(CGQC_GU_Manager mgr, CGQC_GU_Member m)
	{
		return CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, m, CGQC_GU_Rights.OPERATIONS_MANAGE);
	}

	//------------------------------------------------------------------------------------------------
	static bool CanSee(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Operation o)
	{
		return o.status != CGQC_GU_Operation.STATUS_DRAFT || CanManage(mgr, viewer);
	}

	//------------------------------------------------------------------------------------------------
	static bool CanEdit(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Operation o)
	{
		return o.status != CGQC_GU_Operation.STATUS_DONE && CanManage(mgr, viewer);
	}

	//------------------------------------------------------------------------------------------------
	// Labels
	//------------------------------------------------------------------------------------------------
	static string StatusLabel(string status)
	{
		switch (status)
		{
			case CGQC_GU_Operation.STATUS_PUBLISHED:	return "PUBLIÉ";
			case CGQC_GU_Operation.STATUS_DONE:			return "TERMINÉE";
		}

		return "BROUILLON";
	}

	//------------------------------------------------------------------------------------------------
	//! Type de soutien (owner answer 8), in drop-down order.
	static void SupportTypes(notnull array<ref CGQC_GU_LabelItem> outList)
	{
		outList.Insert(Label("medical", "Médical"));
		outList.Insert(Label("resupply", "Ravitaillement"));
		outList.Insert(Label("fires", "Soutien feu"));
		outList.Insert(Label("air", "Transport aérien"));
		outList.Insert(Label("engineer", "Génie"));
		outList.Insert(Label("other", "Autre"));
	}

	//------------------------------------------------------------------------------------------------
	static string SupportLabel(string type)
	{
		array<ref CGQC_GU_LabelItem> types = new array<ref CGQC_GU_LabelItem>();
		SupportTypes(types);
		foreach (CGQC_GU_LabelItem t : types)
		{
			if (t.id == type)
				return t.label;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! « 20:00 » -> « 20 h 00 HE », "" stays "".
	static string TimeLabel(string hhmm)
	{
		if (hhmm.Length() != 5)
			return hhmm;

		return hhmm.Substring(0, 2) + " h " + hhmm.Substring(3, 2) + " HE";
	}

	//------------------------------------------------------------------------------------------------
	static string Title(CGQC_GU_Operation o)
	{
		if (o.header.name.IsEmpty())
			return "OPÉRATION SANS NOM";

		return "OPÉRATION " + CGQC_GU_Str.UpperFr(o.header.name);
	}

	//------------------------------------------------------------------------------------------------
	//! Élément choices (owner answer 8): QG / GM, then the field sections and their elements (active ones
	//! first, renames applied, « Spartan-1 » for element S1), then JTAC. Live-seat sections (UPO) are left out.
	static void ElementChoices(CGQC_GU_Manager mgr, notnull array<ref CGQC_GU_LabelItem> outList)
	{
		outList.Insert(Label(ELEMENT_QG, "QG / GM"));
		for (int pass = 0; pass < 2; pass++)
		{
			foreach (CGQC_GU_Unit unit : mgr.m_Catalogues.m_aUnits)
			{
				foreach (CGQC_GU_Section section : unit.sections)
				{
					if (section.command || section.liveSeats)
						continue;

					bool active = mgr.m_OrbatLive.activeSections.Contains(section.id);
					if (active != (pass == 0))
						continue;

					string label = CGQC_GU_OrbatRequests.SectionLabel(mgr, section);
					outList.Insert(Label(section.id, label));
					foreach (CGQC_GU_Element element : section.elements)
					{
						outList.Insert(Label(section.id + "/" + element.id, ElementLabelOf(label, section, element)));
					}
				}
			}
		}

		outList.Insert(Label(ELEMENT_JTAC, "JTAC"));
	}

	//------------------------------------------------------------------------------------------------
	protected static string ElementLabelOf(string sectionLabel, CGQC_GU_Section section, CGQC_GU_Element element)
	{
		string letter = section.letter;
		if (!letter.IsEmpty() && element.id.StartsWith(letter) && element.id.Length() > letter.Length())
			return sectionLabel + "-" + element.id.Substring(letter.Length(), element.id.Length() - letter.Length());

		return sectionLabel + " " + element.label;
	}

	//------------------------------------------------------------------------------------------------
	//! Label of an element code, "" when the code is unknown.
	static string ElementLabel(CGQC_GU_Manager mgr, string code)
	{
		array<ref CGQC_GU_LabelItem> choices = new array<ref CGQC_GU_LabelItem>();
		ElementChoices(mgr, choices);
		foreach (CGQC_GU_LabelItem c : choices)
		{
			if (c.id == code)
				return c.label;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! A code, or a label typed in the console (« Spartan-1 »), to its code; "" when unknown.
	protected static string ElementCode(CGQC_GU_Manager mgr, string text)
	{
		string wanted = CGQC_GU_Str.Lower(text.Trim());
		array<ref CGQC_GU_LabelItem> choices = new array<ref CGQC_GU_LabelItem>();
		ElementChoices(mgr, choices);
		foreach (CGQC_GU_LabelItem c : choices)
		{
			if (CGQC_GU_Str.Lower(c.id) == wanted || CGQC_GU_Str.Lower(c.label) == wanted)
				return c.id;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_LabelItem Label(string id, string label)
	{
		CGQC_GU_LabelItem item = new CGQC_GU_LabelItem();
		item.id = id;
		item.label = label;
		return item;
	}

	//------------------------------------------------------------------------------------------------
	// Lookups
	//------------------------------------------------------------------------------------------------
	//! By id, else by N° d'ordre (the console uses numbers).
	static CGQC_GU_Operation Find(CGQC_GU_Manager mgr, string key)
	{
		string k = key.Trim();
		if (k.IsEmpty())
			return null;

		CGQC_GU_Operation o = mgr.m_mOperations.Get(k);
		if (o)
			return o;

		foreach (string id, CGQC_GU_Operation c : mgr.m_mOperations)
		{
			if (c.number == k)
				return c;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Visible orders in date order (oldest first), for the « < > » browsing.
	static void Visible(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, notnull array<CGQC_GU_Operation> outList)
	{
		foreach (string id, CGQC_GU_Operation o : mgr.m_mOperations)
		{
			if (viewer && !CanSee(mgr, viewer, o))
				continue;

			int at = outList.Count();
			for (int i = 0; i < outList.Count(); i++)
			{
				if (o.SortKey().Compare(outList[i].SortKey()) < 0)
				{
					at = i;
					break;
				}
			}

			outList.InsertAt(o, at);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! « Opération en cours » (owner answer 2): the PUBLIÉ order with the nearest date from today (HE),
	//! else the latest PUBLIÉ one, else null.
	static CGQC_GU_Operation Current(CGQC_GU_Manager mgr)
	{
		array<CGQC_GU_Operation> all = new array<CGQC_GU_Operation>();
		Visible(mgr, null, all);
		string today = CGQC_GU_Time.TodayEastern();
		CGQC_GU_Operation latest;
		foreach (CGQC_GU_Operation o : all)
		{
			if (o.status != CGQC_GU_Operation.STATUS_PUBLISHED)
				continue;

			if (o.header.date >= today)
				return o;

			latest = o;
		}

		return latest;
	}

	//------------------------------------------------------------------------------------------------
	//! N° d'ordre (owner answer 7): year + sequence, « 2026-014 ». The highest number of the year + 1.
	static string NextNumber(CGQC_GU_Manager mgr)
	{
		string year = CGQC_GU_Time.TodayEastern().Substring(0, 4);
		int highest;
		foreach (string id, CGQC_GU_Operation o : mgr.m_mOperations)
		{
			if (!o.number.StartsWith(year + "-"))
				continue;

			int n = o.number.Substring(5, o.number.Length() - 5).ToInt();
			if (n > highest)
				highest = n;
		}

		string seq = (highest + 1).ToString();
		while (seq.Length() < 3)
		{
			seq = "0" + seq;
		}

		return year + "-" + seq;
	}

	//------------------------------------------------------------------------------------------------
	//! « 2026-014 »: four digits, a dash, one to four digits.
	protected static bool IsNumber(string s)
	{
		int n = s.Length();
		if (n < 6 || n > 9 || s.Get(4) != "-")
			return false;

		for (int i = 0; i < n; i++)
		{
			if (i == 4)
				continue;

			if (!"0123456789".Contains(s.Get(i)))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool OnlyChars(string s, string allowed)
	{
		for (int i = 0; i < s.Length(); i++)
		{
			if (!allowed.Contains(s.Get(i)))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Reads
	//------------------------------------------------------------------------------------------------
	protected static string List(CGQC_GU_Manager mgr, CGQC_GU_Member viewer)
	{
		array<CGQC_GU_Operation> visible = new array<CGQC_GU_Operation>();
		Visible(mgr, viewer, visible);
		array<ref CGQC_GU_OpSummary> list = new array<ref CGQC_GU_OpSummary>();
		foreach (CGQC_GU_Operation o : visible)
		{
			CGQC_GU_OpSummary s = new CGQC_GU_OpSummary();
			s.id = o.id;
			s.number = o.number;
			s.name = o.header.name;
			s.status = o.status;
			s.statusLabel = StatusLabel(o.status);
			s.date = o.header.date;
			s.launchTime = o.header.launchTime;
			s.participants = o.participants.Count();
			s.deploymentId = o.deploymentId;
			list.Insert(s);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("ops", list);
		ctx.WriteValue("canManage", CanManage(mgr, viewer));
		CGQC_GU_Operation current = Current(mgr);
		string currentId;
		if (current)
			currentId = current.id;

		ctx.WriteValue("currentId", currentId);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_RequestResult Get(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, string key)
	{
		CGQC_GU_Operation o;
		if (!key.Trim().IsEmpty())
		{
			o = Find(mgr, key);
			if (!o || !CanSee(mgr, viewer, o))
				return Fail("not_found");
		}
		else
		{
			o = Current(mgr);
			if (!o)
			{
				array<CGQC_GU_Operation> visible = new array<CGQC_GU_Operation>();
				Visible(mgr, viewer, visible);
				if (!visible.IsEmpty())
					o = visible[visible.Count() - 1];
			}
		}

		return Ok(Detail(mgr, viewer, o));
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddName(CGQC_GU_Manager mgr, CGQC_GU_OpView v, string uid)
	{
		if (uid.IsEmpty() || v.names.Contains(uid))
			return;

		v.names.Set(uid, CGQC_GU_DossierBuilder.SignerName(mgr, uid));
	}

	//------------------------------------------------------------------------------------------------
	//! Reply {o: the stored order, v: display values}. o may be null (no order yet): « v » then only
	//! carries the viewer's rights and choices.
	static string Detail(CGQC_GU_Manager mgr, CGQC_GU_Member viewer, CGQC_GU_Operation o)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		CGQC_GU_OpView v = new CGQC_GU_OpView();
		v.canManage = CanManage(mgr, viewer);
		v.timeZoneLabel = "Heure de l'Est (HNE / HAE)";
		if (v.canManage)
			v.nextNumber = NextNumber(mgr);

		array<CGQC_GU_Operation> order = new array<CGQC_GU_Operation>();
		Visible(mgr, viewer, order);
		v.total = order.Count();

		if (o)
		{
			v.title = Title(o);
			string author = CGQC_GU_DossierBuilder.SignerName(mgr, o.createdBy);
			CGQC_GU_Member am = mgr.FindMember(o.createdBy);
			if (am)
			{
				CGQC_GU_AccessGroup g = CGQC_GU_Permissions.GetHighestGroup(cat, am);
				if (g)
					author += " (" + g.label + ")";
			}

			v.byLine = string.Format("OPORD %1 · rédigé par %2", o.number, author);
			v.statusLabel = StatusLabel(o.status);
			v.dateLabel = CGQC_GU_DossierBuilder.DisplayDate(o.header.date);
			v.briefingLabel = TimeLabel(o.header.briefingTime);
			v.launchLabel = TimeLabel(o.header.launchTime);
			if (o.status == CGQC_GU_Operation.STATUS_DONE)
				v.doneLine = string.Format("Terminée le %1 par %2", CGQC_GU_DossierBuilder.DisplayDate(o.doneAt), CGQC_GU_DossierBuilder.SignerName(mgr, o.doneBy));

			AddName(mgr, v, o.createdBy);
			foreach (string su : o.command.succession)
			{
				AddName(mgr, v, su);
			}

			foreach (CGQC_GU_OpCallsign cs : o.command.callsigns)
			{
				AddName(mgr, v, cs.chief);
				if (!cs.element.IsEmpty())
					v.elementLabels.Set(cs.element, ElementLabel(mgr, cs.element));
			}

			foreach (CGQC_GU_OpElementTask et : o.execution.elementTasks)
			{
				if (!et.element.IsEmpty())
					v.elementLabels.Set(et.element, ElementLabel(mgr, et.element));
			}

			foreach (CGQC_GU_OpSupport sp : o.support)
			{
				v.supportLabels.Set(sp.type, SupportLabel(sp.type));
			}

			foreach (CGQC_GU_OpParticipant pa : o.participants)
			{
				AddName(mgr, v, pa.uid);
			}

			v.canEdit = CanEdit(mgr, viewer, o);
			int idx = order.Find(o);
			v.index = idx + 1;
			if (idx > 0)
				v.prevId = order[idx - 1].id;

			if (idx >= 0 && idx < order.Count() - 1)
				v.nextId = order[idx + 1].id;
		}

		if (v.canManage)
		{
			ElementChoices(mgr, v.elementChoices);
			SupportTypes(v.supportChoices);
			People(mgr, v.people);
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		if (o)
			ctx.WriteValue("o", o);

		ctx.WriteValue("v", v);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	//! Chef and succession choices (owner answer 8): online members, Chef de section / Chef d'équipe
	//! seats first. Zeus members are flagged (callsign Chef only, never the succession).
	protected static void People(CGQC_GU_Manager mgr, notnull array<ref CGQC_GU_OpPersonDto> outList)
	{
		CGQC_GU_Catalogues cat = mgr.m_Catalogues;
		array<ref CGQC_GU_OpPersonDto> others = new array<ref CGQC_GU_OpPersonDto>();
		foreach (string uid, CGQC_GU_Member m : mgr.m_mMembers)
		{
			if (!mgr.IsOnline(uid))
				continue;

			CGQC_GU_OpPersonDto d = new CGQC_GU_OpPersonDto();
			d.uid = uid;
			d.name = CGQC_GU_OverviewBuilder.NameOf(cat, m);
			d.online = true;
			d.zeus = m.accessGroups.Contains(CGQC_GU_OrbatRequests.ZEUS_GROUP);

			CGQC_GU_SeatAssignment a = CGQC_GU_OrbatRequests.FindByUid(mgr.m_OrbatLive, uid);
			if (a)
			{
				d.seat = a.seat;
				CGQC_GU_SeatRef sr = CGQC_GU_OrbatRequests.Resolve(mgr, a.unit, a.seat);
				string catalogueRole;
				if (sr && sr.m_Seat)
					catalogueRole = sr.m_Seat.role;

				string role = CGQC_GU_OrbatRequests.RoleOf(mgr.m_OrbatLive, a.unit, a.seat, catalogueRole);
				d.leader = role == "chef_section" || role == "chef_equipe";
			}

			if (d.leader)
				outList.Insert(d);
			else
				others.Insert(d);
		}

		foreach (CGQC_GU_OpPersonDto o : others)
		{
			outList.Insert(o);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	//! UIDs from the screen; names are accepted too, for the #gu console.
	protected static CGQC_GU_Member ResolveMember(CGQC_GU_Manager mgr, string who)
	{
		CGQC_GU_Member m = mgr.FindMember(who);
		if (m)
			return m;

		return mgr.FindMemberLoose(who);
	}

	//------------------------------------------------------------------------------------------------
	//! Scalar field: the sent value, else the current one. A value over maxLen keeps the current one and
	//! raises s_bTooLong (checked once per section).
	protected static bool s_bTooLong;

	protected static string Text(CGQC_GU_Payload p, string key, int maxLen, string current)
	{
		if (!p.Has(key))
			return current;

		string v = p.Get(key).Trim();
		if (v.Length() > maxLen)
		{
			s_bTooLong = true;
			return current;
		}

		return v;
	}

	//------------------------------------------------------------------------------------------------
	//! Rows sent for a list: the count key (« tn »), else the contiguous rows from 1 that carry one of the
	//! fields. -1 = list not sent (left as it is); MAX + 1 = too many.
	protected static int RowCount(CGQC_GU_Payload p, string prefix, array<string> fields, int max)
	{
		if (p.Has(prefix + "n"))
		{
			int n = p.Get(prefix + "n").ToInt();
			if (n < 0)
				return 0;

			if (n > max)
				return max + 1;

			return n;
		}

		int count;
		for (int i = 1; i <= max + 1; i++)
		{
			bool found;
			foreach (string f : fields)
			{
				string key = prefix + i.ToString();
				if (!f.IsEmpty())
					key += "." + f;

				if (p.Has(key))
					found = true;
			}

			if (!found)
				break;

			count = i;
		}

		if (count == 0)
			return -1;

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected static string RowValue(CGQC_GU_Payload p, string prefix, int i, string field)
	{
		return p.Get(prefix + i.ToString() + "." + field).Trim();
	}

	//------------------------------------------------------------------------------------------------
	//! Applies one section of the form to o. Returns "" or an error code; the caller works on a copy.
	protected static string ApplySection(CGQC_GU_Manager mgr, CGQC_GU_Operation o, string section, CGQC_GU_Payload p)
	{
		switch (section)
		{
			case SECTION_HEADER:	return ApplyHeader(mgr, o, p);
			case SECTION_SITUATION:	return ApplySituation(o, p);
			case SECTION_EXECUTION:	return ApplyExecution(mgr, o, p);
			case SECTION_SUPPORT:	return ApplySupport(o, p);
			case SECTION_CMDT:		return ApplyCmdt(mgr, o, p);
			case SECTION_POINTS:	return ApplyPoints(o, p);
			case SECTION_SIGNALS:	return ApplySignals(o, p);
		}

		return "bad_section";
	}

	//------------------------------------------------------------------------------------------------
	protected static string ApplyHeader(CGQC_GU_Manager mgr, CGQC_GU_Operation o, CGQC_GU_Payload p)
	{
		CGQC_GU_OpHeader h = o.header;
		s_bTooLong = false;
		string name = Text(p, "name", MAX_SHORT, h.name);
		string hq = Text(p, "hq", MAX_SHORT, h.hq);
		string area = Text(p, "area", MAX_SHORT, h.area);
		string mapName = Text(p, "map", MAX_SHORT, h.mapName);
		string weather = Text(p, "weather", MAX_LINE, h.weather);
		if (s_bTooLong)
			return "too_long";

		h.name = name;
		h.hq = hq;
		h.area = area;
		h.mapName = mapName;
		h.weather = weather;

		if (h.name.IsEmpty())
			return "bad_name";

		if (p.Has("number"))
		{
			string number = p.Get("number").Trim();
			if (!IsNumber(number))
				return "bad_number";

			foreach (string id, CGQC_GU_Operation other : mgr.m_mOperations)
			{
				if (other.id != o.id && other.number == number)
					return "number_taken";
			}

			o.number = number;
		}

		if (p.Has("date"))
		{
			string date = p.Get("date").Trim();
			if (!date.IsEmpty())
			{
				if (date.Length() != 10 || !CGQC_GU_Validate.IsIsoDateOrEmpty(date))
					return "bad_date";

				if (date.Substring(8, 2).ToInt() > CGQC_GU_Time.DaysInMonth(date.Substring(0, 4).ToInt(), date.Substring(5, 2).ToInt()))
					return "bad_date";
			}

			h.date = date;
		}

		if (p.Has("briefing"))
		{
			string briefing = p.Get("briefing").Trim();
			if (!CGQC_GU_Validate.IsTimeOrEmpty(briefing))
				return "bad_time";

			h.briefingTime = briefing;
		}

		if (p.Has("launch"))
		{
			string launch = p.Get("launch").Trim();
			if (!CGQC_GU_Validate.IsTimeOrEmpty(launch))
				return "bad_time";

			h.launchTime = launch;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ApplySituation(CGQC_GU_Operation o, CGQC_GU_Payload p)
	{
		CGQC_GU_OpSituation s = o.situation;
		s_bTooLong = false;
		string enemy = Text(p, "enemy", MAX_TEXT, s.enemy);
		string friendly = Text(p, "friendly", MAX_TEXT, s.friendly);
		string attachments = Text(p, "attachments", MAX_LINE, s.attachments);
		string mission = Text(p, "mission", MAX_TEXT, o.mission);
		if (s_bTooLong)
			return "too_long";

		s.enemy = enemy;
		s.friendly = friendly;
		s.attachments = attachments;
		o.mission = mission;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ApplyExecution(CGQC_GU_Manager mgr, CGQC_GU_Operation o, CGQC_GU_Payload p)
	{
		CGQC_GU_OpExecution e = o.execution;
		s_bTooLong = false;
		string task = Text(p, "task", MAX_TEXT, e.task);
		string purpose = Text(p, "purpose", MAX_TEXT, e.purpose);
		string endState = Text(p, "endState", MAX_TEXT, e.endState);
		string concept = Text(p, "concept", MAX_TEXT, e.concept);
		string coordination = Text(p, "coordination", MAX_TEXT, e.coordination);
		if (s_bTooLong)
			return "too_long";

		e.task = task;
		e.purpose = purpose;
		e.endState = endState;
		e.concept = concept;
		e.coordination = coordination;

		array<string> fields = { "el", "task" };
		int n = RowCount(p, "t", fields, MAX_ROWS);
		if (n > MAX_ROWS)
			return "too_many";

		if (n < 0)
			return string.Empty;

		array<ref CGQC_GU_OpElementTask> rows = new array<ref CGQC_GU_OpElementTask>();
		for (int i = 1; i <= n; i++)
		{
			string el = RowValue(p, "t", i, "el");
			string rowTask = RowValue(p, "t", i, "task");
			if (el.IsEmpty() && rowTask.IsEmpty())
				continue;

			if (rowTask.Length() > MAX_LINE)
				return "too_long";

			CGQC_GU_OpElementTask row = new CGQC_GU_OpElementTask();
			if (!el.IsEmpty())
			{
				row.element = ElementCode(mgr, el);
				if (row.element.IsEmpty())
					return "bad_element";
			}

			row.task = rowTask;
			rows.Insert(row);
		}

		e.elementTasks = rows;
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ApplySupport(CGQC_GU_Operation o, CGQC_GU_Payload p)
	{
		array<string> fields = { "type", "details" };
		int n = RowCount(p, "s", fields, MAX_SUPPORT_ROWS);
		if (n > MAX_SUPPORT_ROWS)
			return "too_many";

		if (n < 0)
			return string.Empty;

		array<ref CGQC_GU_OpSupport> rows = new array<ref CGQC_GU_OpSupport>();
		for (int i = 1; i <= n; i++)
		{
			string type = CGQC_GU_Str.Lower(RowValue(p, "s", i, "type"));
			string details = RowValue(p, "s", i, "details");
			if (type.IsEmpty() && details.IsEmpty())
				continue;

			if (details.Length() > MAX_SUPPORT)
				return "too_long";

			if (!type.IsEmpty() && SupportLabel(type).IsEmpty())
				return "bad_support_type";

			CGQC_GU_OpSupport row = new CGQC_GU_OpSupport();
			row.type = type;
			row.details = details;
			rows.Insert(row);
		}

		o.support = rows;
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ApplyCmdt(CGQC_GU_Manager mgr, CGQC_GU_Operation o, CGQC_GU_Payload p)
	{
		// Succession: never a Zeus member (spec), each member once.
		array<string> plain = { "" };
		int ns = RowCount(p, "succ", plain, MAX_SUCCESSION);
		if (ns > MAX_SUCCESSION)
			return "too_many";

		if (ns >= 0)
		{
			array<string> succession = new array<string>();
			for (int i = 1; i <= ns; i++)
			{
				string who = p.Get("succ" + i.ToString()).Trim();
				if (who.IsEmpty())
					continue;

				CGQC_GU_Member m = ResolveMember(mgr, who);
				if (!m)
					return "bad_member";

				if (m.accessGroups.Contains(CGQC_GU_OrbatRequests.ZEUS_GROUP))
					return "zeus_not_allowed";

				if (!succession.Contains(m.uid))
					succession.Insert(m.uid);
			}

			o.command.succession = succession;
		}

		array<string> fields = { "callsign", "el", "chief", "mhz" };
		int n = RowCount(p, "c", fields, MAX_ROWS);
		if (n > MAX_ROWS)
			return "too_many";

		if (n < 0)
			return string.Empty;

		array<ref CGQC_GU_OpCallsign> rows = new array<ref CGQC_GU_OpCallsign>();
		for (int j = 1; j <= n; j++)
		{
			string callsign = RowValue(p, "c", j, "callsign");
			string el = RowValue(p, "c", j, "el");
			string chief = RowValue(p, "c", j, "chief");
			string mhz = RowValue(p, "c", j, "mhz");
			if (callsign.IsEmpty() && el.IsEmpty() && chief.IsEmpty() && mhz.IsEmpty())
				continue;

			if (callsign.Length() > MAX_SHORT)
				return "too_long";

			if (mhz.Length() > MAX_MHZ || !OnlyChars(mhz, "0123456789.,"))
				return "bad_mhz";

			CGQC_GU_OpCallsign row = new CGQC_GU_OpCallsign();
			row.callsign = callsign;
			row.mhz = mhz;
			if (!el.IsEmpty())
			{
				row.element = ElementCode(mgr, el);
				if (row.element.IsEmpty())
					return "bad_element";
			}

			if (!chief.IsEmpty())
			{
				CGQC_GU_Member cm = ResolveMember(mgr, chief);
				if (!cm)
					return "bad_member";

				row.chief = cm.uid;
			}

			rows.Insert(row);
		}

		o.command.callsigns = rows;
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ApplyPoints(CGQC_GU_Operation o, CGQC_GU_Payload p)
	{
		array<string> fields = { "name", "grid", "note" };
		int n = RowCount(p, "p", fields, MAX_ROWS);
		if (n > MAX_ROWS)
			return "too_many";

		if (n < 0)
			return string.Empty;

		array<ref CGQC_GU_OpPoint> rows = new array<ref CGQC_GU_OpPoint>();
		for (int i = 1; i <= n; i++)
		{
			string name = RowValue(p, "p", i, "name");
			string grid = RowValue(p, "p", i, "grid");
			string note = RowValue(p, "p", i, "note");
			if (name.IsEmpty() && grid.IsEmpty() && note.IsEmpty())
				continue;

			if (name.Length() > MAX_SHORT || note.Length() > MAX_LINE)
				return "too_long";

			if (grid.Length() > MAX_GRID || !OnlyChars(grid, "0123456789 "))
				return "bad_grid";

			CGQC_GU_OpPoint row = new CGQC_GU_OpPoint();
			row.name = name;
			row.grid = grid;
			row.note = note;
			rows.Insert(row);
		}

		o.command.points = rows;
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected static string ApplySignals(CGQC_GU_Operation o, CGQC_GU_Payload p)
	{
		CGQC_GU_OpPasswords pw = o.command.passwords;
		s_bTooLong = false;
		string challenge = Text(p, "challenge", MAX_SHORT, pw.challenge);
		string password = Text(p, "password", MAX_SHORT, pw.password);
		string running = Text(p, "running", MAX_SHORT, pw.running);
		string nearRecognition = Text(p, "near", MAX_LINE, pw.nearRecognition);
		if (s_bTooLong)
			return "too_long";

		pw.challenge = challenge;
		pw.password = password;
		pw.running = running;
		pw.nearRecognition = nearRecognition;

		foreach (CGQC_GU_OpPace pace : o.command.pace)
		{
			string level = pace.level;
			string means = Text(p, level + ".means", MAX_LINE, pace.means);
			string switchWhen = Text(p, level + ".when", MAX_LINE, pace.switchWhen);
			string paceMhz = Text(p, level + ".mhz", MAX_MHZ, pace.mhz);
			if (s_bTooLong)
				return "too_long";

			if (!OnlyChars(paceMhz, "0123456789.,"))
				return "bad_mhz";

			pace.means = means;
			pace.switchWhen = switchWhen;
			pace.mhz = paceMhz;
		}

		array<string> fields = { "text", "meaning" };
		int n = RowCount(p, "g", fields, MAX_ROWS);
		if (n > MAX_ROWS)
			return "too_many";

		if (n < 0)
			return string.Empty;

		array<ref CGQC_GU_OpSignal> rows = new array<ref CGQC_GU_OpSignal>();
		for (int i = 1; i <= n; i++)
		{
			string text = RowValue(p, "g", i, "text");
			string meaning = RowValue(p, "g", i, "meaning");
			if (text.IsEmpty() && meaning.IsEmpty())
				continue;

			if (text.Length() > MAX_SHORT || meaning.Length() > MAX_LINE)
				return "too_long";

			CGQC_GU_OpSignal row = new CGQC_GU_OpSignal();
			row.text = text;
			row.meaning = meaning;
			rows.Insert(row);
		}

		o.command.signals = rows;
		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Operation Copy(CGQC_GU_Operation o)
	{
		CGQC_GU_JsonStringSaveContext save = new CGQC_GU_JsonStringSaveContext();
		o.WriteTo(save);
		JsonLoadContext load = new JsonLoadContext();
		CGQC_GU_Operation c = new CGQC_GU_Operation();
		if (!load.LoadFromString(save.Export()) || !c.ReadFrom(load))
			return null;

		return c;
	}

	//------------------------------------------------------------------------------------------------
	//! ENREGISTRER: one section. id "" creates a BROUILLON (header section, with a name) and fills the
	//! N° d'ordre (owner answer 7).
	protected static CGQC_GU_RequestResult Save(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		string section = CGQC_GU_Str.Lower(p.Get("section").Trim());
		CGQC_GU_Operation work;
		string key = p.Get("id").Trim();
		if (key.IsEmpty())
		{
			if (section != SECTION_HEADER)
				return Fail("need_header");

			work = new CGQC_GU_Operation();
			work.id = CGQC_GU_Ids.New("op");
			work.number = NextNumber(mgr);
			work.createdBy = caller.uid;
			work.createdAt = CGQC_GU_Time.NowUtc();
		}
		else
		{
			CGQC_GU_Operation o = Find(mgr, key);
			if (!o)
				return Fail("not_found");

			if (o.status == CGQC_GU_Operation.STATUS_DONE)
				return Fail("already_done");

			// Work on a copy so a refused form leaves the stored order as it was.
			work = Copy(o);
			if (!work)
				return Fail("write_failed");
		}

		string err = ApplySection(mgr, work, section, p);
		if (!err.IsEmpty())
			return Fail(err);

		work.updatedBy = caller.uid;
		work.updatedAt = CGQC_GU_Time.NowUtc();
		if (!CGQC_GU_Storage.SaveOperation(work))
			return Fail("write_failed");

		mgr.m_mOperations.Set(work.id, work);
		if (work.status == CGQC_GU_Operation.STATUS_PUBLISHED)
			mgr.BroadcastLightState();

		CGQC_GU_Log.Info(string.Format("Ordre %1 (%2) : section %3 enregistrée par %4", work.number, work.id, section, caller.uid));
		return Ok(Detail(mgr, caller, work));
	}

	//------------------------------------------------------------------------------------------------
	//! PUBLIER / RETOUR EN BROUILLON. TERMINÉE only comes from the end of an order (5d).
	protected static CGQC_GU_RequestResult SetStatus(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		string status = CGQC_GU_Str.Lower(p.Get("status").Trim());
		if (status != CGQC_GU_Operation.STATUS_DRAFT && status != CGQC_GU_Operation.STATUS_PUBLISHED)
			return Fail("bad_status");

		CGQC_GU_Operation o = Find(mgr, p.Get("id"));
		if (!o)
			return Fail("not_found");

		if (o.status == CGQC_GU_Operation.STATUS_DONE)
			return Fail("already_done");

		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		if (status == o.status)
			return Ok(Detail(mgr, caller, o));

		// A published order needs its name and date (« Opération en cours » picks by date).
		if (status == CGQC_GU_Operation.STATUS_PUBLISHED && (o.header.name.IsEmpty() || o.header.date.IsEmpty()))
			return Fail("incomplete");

		string previous = o.status;
		o.status = status;
		o.updatedBy = caller.uid;
		o.updatedAt = CGQC_GU_Time.NowUtc();
		if (status == CGQC_GU_Operation.STATUS_PUBLISHED)
		{
			o.publishedBy = caller.uid;
			o.publishedAt = o.updatedAt;
		}

		if (!CGQC_GU_Storage.SaveOperation(o))
		{
			o.status = previous;
			return Fail("write_failed");
		}

		mgr.BroadcastLightState();
		CGQC_GU_Log.Info(string.Format("Ordre %1 (%2) : %3 par %4", o.number, o.id, StatusLabel(status), caller.uid));
		return Ok(Detail(mgr, caller, o));
	}

	//------------------------------------------------------------------------------------------------
	//! SUPPRIMER, BROUILLON or PUBLIÉ only (answer 3). A copy is kept in archive/.
	protected static CGQC_GU_RequestResult Delete(CGQC_GU_Manager mgr, CGQC_GU_Member caller, CGQC_GU_Payload p)
	{
		CGQC_GU_Operation o = Find(mgr, p.Get("id"));
		if (!o)
			return Fail("not_found");

		if (o.status == CGQC_GU_Operation.STATUS_DONE)
			return Fail("already_done");

		if (!CanManage(mgr, caller))
			return Fail("forbidden");

		if (!CGQC_GU_Storage.DeleteOperation(o))
			return Fail("write_failed");

		string id = o.id;
		string number = o.number;
		mgr.m_mOperations.Remove(id);
		mgr.BroadcastLightState();
		CGQC_GU_Log.Info(string.Format("Ordre %1 (%2) supprimé par %3", number, id, caller.uid));
		return Get(mgr, caller, string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	// Console (#gu op …)
	//------------------------------------------------------------------------------------------------
	//! Every order, one line each.
	static string DescribeList(CGQC_GU_Manager mgr)
	{
		array<CGQC_GU_Operation> all = new array<CGQC_GU_Operation>();
		Visible(mgr, null, all);
		string text = string.Format("%1 ordre(s) :", all.Count());
		CGQC_GU_Operation current = Current(mgr);
		foreach (CGQC_GU_Operation o : all)
		{
			string mark;
			if (o == current)
				mark = " | EN COURS";

			text += string.Format("\n%1 | %2 | %3 %4 | %5 | %6%7", o.number, StatusLabel(o.status), o.header.date, o.header.launchTime, o.header.name, o.id, mark);
		}

		return text;
	}

	//------------------------------------------------------------------------------------------------
	protected static string Rows(int n, string word)
	{
		return string.Format("%1 %2", n, word);
	}

	//------------------------------------------------------------------------------------------------
	//! One order: the header and counts, or one section in full.
	static string Describe(CGQC_GU_Manager mgr, CGQC_GU_Operation o, string section)
	{
		CGQC_GU_OpHeader h = o.header;
		string text = string.Format("%1 | %2 | %3", Title(o), o.number, StatusLabel(o.status));
		switch (section)
		{
			case SECTION_SITUATION:
				text += "\nennemi : " + o.situation.enemy;
				text += "\nallié : " + o.situation.friendly;
				text += "\npièces jointes : " + o.situation.attachments;
				text += "\nmission : " + o.mission;
				return text;

			case SECTION_EXECUTION:
				text += string.Format("\ntâche : %1\nbut : %2\nétat final : %3", o.execution.task, o.execution.purpose, o.execution.endState);
				text += "\nconcept : " + o.execution.concept;
				foreach (CGQC_GU_OpElementTask et : o.execution.elementTasks)
				{
					text += string.Format("\n- %1 : %2", ElementLabel(mgr, et.element), et.task);
				}

				text += "\ncoordination : " + o.execution.coordination;
				return text;

			case SECTION_SUPPORT:
				foreach (CGQC_GU_OpSupport sp : o.support)
				{
					text += string.Format("\n- %1 : %2", SupportLabel(sp.type), sp.details);
				}

				return text;

			case SECTION_CMDT:
				for (int i = 0; i < o.command.succession.Count(); i++)
				{
					text += string.Format("\nsuccession %1 : %2", i + 1, CGQC_GU_DossierBuilder.SignerName(mgr, o.command.succession[i]));
				}

				foreach (CGQC_GU_OpCallsign cs : o.command.callsigns)
				{
					string chief;
					if (!cs.chief.IsEmpty())
						chief = CGQC_GU_DossierBuilder.SignerName(mgr, cs.chief);

					text += string.Format("\n- %1 | %2 | %3 | %4", cs.callsign, ElementLabel(mgr, cs.element), chief, cs.mhz);
				}

				return text;

			case SECTION_POINTS:
				foreach (CGQC_GU_OpPoint pt : o.command.points)
				{
					text += string.Format("\n- %1 | %2 | %3", pt.name, pt.grid, pt.note);
				}

				return text;

			case SECTION_SIGNALS:
			{
				CGQC_GU_OpPasswords pw = o.command.passwords;
				text += string.Format("\nchallenge %1 / %2 · running %3 · reco %4", pw.challenge, pw.password, pw.running, pw.nearRecognition);
				foreach (CGQC_GU_OpSignal sg : o.command.signals)
				{
					text += string.Format("\n- %1 : %2", sg.text, sg.meaning);
				}

				foreach (CGQC_GU_OpPace pc : o.command.pace)
				{
					text += string.Format("\n%1 | %2 | %3 | %4", pc.level, pc.means, pc.switchWhen, pc.mhz);
				}

				return text;
			}
		}

		text += string.Format("\nQG %1 · %2 · briefing %3 · lancement %4", h.hq, h.date, TimeLabel(h.briefingTime), TimeLabel(h.launchTime));
		text += string.Format("\nzone %1 · carte %2 · météo %3", h.area, h.mapName, h.weather);
		text += string.Format("\n%1, %2, %3, %4, %5, %6", Rows(o.execution.elementTasks.Count(), "tâches"), Rows(o.support.Count(), "soutiens"),
			Rows(o.command.callsigns.Count(), "indicatifs"), Rows(o.command.points.Count(), "points"), Rows(o.command.signals.Count(), "signaux"),
			Rows(o.command.succession.Count(), "succession"));
		text += string.Format("\nrédigé par %1 · %2", CGQC_GU_DossierBuilder.SignerName(mgr, o.createdBy), o.id);

		// The reply a manager gets for this order (the largest one).
		CGQC_GU_JsonStringSaveContext file = new CGQC_GU_JsonStringSaveContext();
		o.WriteTo(file);
		text += string.Format("\ntaille : fichier %1 octets", file.Export().Length());
		return text;
	}

	//------------------------------------------------------------------------------------------------
	protected static string Fill(string seed, int len)
	{
		string s;
		while (s.Length() < len)
		{
			s += seed;
		}

		return s.Substring(0, len);
	}

	//------------------------------------------------------------------------------------------------
	//! Size test (engine risk: one reply = one JSON string). An order with every field and row at its
	//! maximum, accents included. Kept in memory unless saved (« #gu op max save »).
	static CGQC_GU_Operation BuildMax(CGQC_GU_Manager mgr, CGQC_GU_Member author)
	{
		string seed = "Élément héliporté vers l'objectif « Kalay », sécurité à 360°. ";
		CGQC_GU_Operation o = new CGQC_GU_Operation();
		o.id = CGQC_GU_Ids.New("op");
		o.number = NextNumber(mgr);
		o.createdBy = author.uid;
		o.createdAt = CGQC_GU_Time.NowUtc();
		o.updatedBy = author.uid;
		o.updatedAt = o.createdAt;

		CGQC_GU_OpHeader h = o.header;
		h.name = "TEST TAILLE MAX";
		h.hq = Fill(seed, MAX_SHORT);
		h.date = CGQC_GU_Time.TodayEastern();
		h.briefingTime = "19:45";
		h.launchTime = "20:00";
		h.area = Fill(seed, MAX_SHORT);
		h.mapName = Fill(seed, MAX_SHORT);
		h.weather = Fill(seed, MAX_LINE);

		o.situation.enemy = Fill(seed, MAX_TEXT);
		o.situation.friendly = Fill(seed, MAX_TEXT);
		o.situation.attachments = Fill(seed, MAX_LINE);
		o.mission = Fill(seed, MAX_TEXT);

		CGQC_GU_OpExecution e = o.execution;
		e.task = Fill(seed, MAX_TEXT);
		e.purpose = Fill(seed, MAX_TEXT);
		e.endState = Fill(seed, MAX_TEXT);
		e.concept = Fill(seed, MAX_TEXT);
		e.coordination = Fill(seed, MAX_TEXT);

		array<ref CGQC_GU_LabelItem> elements = new array<ref CGQC_GU_LabelItem>();
		ElementChoices(mgr, elements);
		array<ref CGQC_GU_LabelItem> types = new array<ref CGQC_GU_LabelItem>();
		SupportTypes(types);

		for (int i = 0; i < MAX_ROWS; i++)
		{
			CGQC_GU_OpElementTask et = new CGQC_GU_OpElementTask();
			et.element = elements[i % elements.Count()].id;
			et.task = Fill(seed, MAX_LINE);
			e.elementTasks.Insert(et);

			CGQC_GU_OpCallsign cs = new CGQC_GU_OpCallsign();
			cs.callsign = Fill(seed, MAX_SHORT);
			cs.element = elements[i % elements.Count()].id;
			cs.chief = author.uid;
			cs.mhz = "123.456";
			o.command.callsigns.Insert(cs);

			CGQC_GU_OpPoint pt = new CGQC_GU_OpPoint();
			pt.name = Fill(seed, MAX_SHORT);
			pt.grid = "0640 1120";
			pt.note = Fill(seed, MAX_LINE);
			o.command.points.Insert(pt);

			CGQC_GU_OpSignal sg = new CGQC_GU_OpSignal();
			sg.text = Fill(seed, MAX_SHORT);
			sg.meaning = Fill(seed, MAX_LINE);
			o.command.signals.Insert(sg);
		}

		for (int j = 0; j < MAX_SUPPORT_ROWS; j++)
		{
			CGQC_GU_OpSupport sp = new CGQC_GU_OpSupport();
			sp.type = types[j % types.Count()].id;
			sp.details = Fill(seed, MAX_SUPPORT);
			o.support.Insert(sp);
		}

		for (int k = 0; k < MAX_SUCCESSION; k++)
		{
			o.command.succession.Insert(author.uid);
		}

		CGQC_GU_OpPasswords pw = o.command.passwords;
		pw.challenge = Fill(seed, MAX_SHORT);
		pw.password = Fill(seed, MAX_SHORT);
		pw.running = Fill(seed, MAX_SHORT);
		pw.nearRecognition = Fill(seed, MAX_LINE);
		foreach (CGQC_GU_OpPace pc : o.command.pace)
		{
			pc.means = Fill(seed, MAX_LINE);
			pc.switchWhen = Fill(seed, MAX_LINE);
			pc.mhz = "123.456";
		}

		return o;
	}
}
