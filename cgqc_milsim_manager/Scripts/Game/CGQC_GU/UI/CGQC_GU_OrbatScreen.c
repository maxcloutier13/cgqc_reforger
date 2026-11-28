// Gestion d'unité - Unité → ORBAT (paper), Phases 4c (reading) and 4d (assigning). Artboards `Orbat`,
// `Orbat-Actives`. Header « n en ligne » and search; CGQC / UPO / GRIT tabs with patches (GRIT greyed);
// Zeus strip; « Sections actives · n » with three element columns, binôme markers and « n / n »; inactive
// section cards; « Non assignés · n »; the « PLACE CHOISIE » panel.
// 4d: Commandement / Zeus tick sections (unticking one with seats is a double press), rename them, set a
// seat's role, give, free and lock seats. Members take a free seat and leave their own. Every write is a
// double press, is checked again on the server, and its reply is the fresh ORBAT.
// 4e: the UPO tab (artboard `Orbat-UPO`), live seats: INSTRUCTEURS cards (I-0 = the teacher) with « Enseigne : … »,
// ÉLÈVES rows with their progress line. Commandement / Zeus add an empty seat (+ AJOUTER …) and remove one
// (RETIRER); a seat is filled like any other, and a right click only empties it.

//! A pressed tab, seat or button of the ORBAT screen.
class CGQC_GU_OrbatAction : CGQC_GU_Action
{
	protected CGQC_GU_OrbatScreen m_Screen;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_OrbatAction(CGQC_GU_OrbatScreen screen, string kind, string value)
	{
		m_Screen = screen;
		m_sKind = kind;
		m_sValue = value;
	}

	//------------------------------------------------------------------------------------------------
	override void Fire()
	{
		if (m_Screen)
			m_Screen.OnAction(m_sKind, m_sValue);
	}
}

class CGQC_GU_OrbatScreen : CGQC_GU_PaperScreen
{
	static const ResourceName PATCH_CGQC = "{1772CF1260734674}UI/Textures/CGQC_GU/CGQC_GU_Patch_CGQC.edds";
	static const ResourceName PATCH_UPO = "{B12336133793AC94}UI/Textures/CGQC_GU/CGQC_GU_Patch_UPO.edds";
	static const ResourceName PATCH_GRIT = "{FD1FC0DB78D2A628}UI/Textures/CGQC_GU/CGQC_GU_Patch_GRIT.edds";

	static const float ASIDE_W = 360;
	static const float LEFT_W = 1190;		// left column inside the scroll area (content 1600 − aside − gaps − slider)
	static const float COL_W = 372;			// one element column (three per row)
	static const float COL_GAP = 14;
	static const float CARD_GAP = 8;
	static const int COLS = 3;
	static const int CARDS_PER_ROW = 5;
	static const int FREE_TAGS_PER_ROW = 8;
	static const int GIVE_ROWS = 5;			// « Donner la place à »: matches shown
	static const float UPO_GAP = 18;		// between the two instructor card columns

	protected ref CGQC_GU_OrbatDto m_Dto;
	protected int m_iReadReq;
	protected int m_iWriteReq;
	protected string m_sNotice;			// last refusal, shown in the side panel
	protected string m_sRolePick;		// « Rôle de la place » choice, "" = the seat's own
	protected string m_sGiveUid;		// « Donner la place à » choice
	protected string m_sGiveSearch;
	protected string m_sRenaming;		// section whose name is being edited
	protected string m_sArmedSection;	// section unticked once: the second press frees its seats
	protected ref CGQC_GU_Region m_Give = new CGQC_GU_Region();
	protected ref map<string, Widget> m_mGiveRows = new map<string, Widget>();
	protected ref map<string, TextWidget> m_mArmLabels = new map<string, TextWidget>();
	protected TextWidget m_wNotice;
	protected TextWidget m_wRole;
	protected CGQC_GU_Button m_RoleBtn;
	protected CGQC_GU_Button m_AssignBtn;
	protected EditBoxWidget m_wGiveSearch;
	protected ref CGQC_GU_FieldWatch m_GiveWatch;
	protected EditBoxWidget m_wRename;
	protected string m_sUnit;		// tab shown
	protected string m_sSelUnit;	// chosen seat
	protected string m_sSel;
	protected string m_sAddUnit;	// « + AJOUTER … » sent: unit, section and the seats before it
	protected string m_sAddSection;
	protected ref array<string> m_aBeforeAdd = {};
	protected ref array<string> m_aCardKeys = {};		// seat rows drawn as UPO instructor cards

	protected ref CGQC_GU_Region m_Tabs = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Zeus = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Body = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Free = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Aside = new CGQC_GU_Region();
	protected ref map<string, Widget> m_mSeatRows = new map<string, Widget>();

	protected TextWidget m_wOnline;
	protected TextWidget m_wMessage;
	protected EditBoxWidget m_wSearch;
	protected ref CGQC_GU_FieldWatch m_SearchWatch;

	//------------------------------------------------------------------------------------------------
	override protected CGQC_GU_ERailTab GetRailTab()
	{
		return CGQC_GU_ERailTab.ORBAT;
	}

	//------------------------------------------------------------------------------------------------
	override void OnRailSelected(CGQC_GU_ERailTab tab)
	{
		if (tab != CGQC_GU_ERailTab.ORBAT)
		{
			super.OnRailSelected(tab);
			return;
		}

		RequestOrbat();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildPaper()
	{
		SetHeader("UNITÉ · PARTIE EN COURS", "ORBAT");

		if (m_wHeaderTools)
		{
			m_wOnline = CGQC_GU_W.Text(m_wHeaderTools, string.Empty, CGQC_GU_Style.FONT_LABEL, 18, CGQC_GU_Style.InkMuted());
			if (m_wOnline)
			{
				LayoutSlot.SetVerticalAlign(m_wOnline, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(m_wOnline, 0, 0, 40, 0);
			}

			TextWidget cap = CGQC_GU_W.Text(m_wHeaderTools, "RECHERCHE", CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
			if (cap)
			{
				LayoutSlot.SetVerticalAlign(cap, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(cap, 0, 0, 10, 0);
			}

			m_wSearch = CGQC_GU_W.Field(m_wHeaderTools, string.Empty, 380);
			if (m_wSearch)
			{
				m_SearchWatch = new CGQC_GU_FieldWatch();
				m_SearchWatch.GetOnChanged().Insert(OnSearchChanged);
				m_wSearch.AddHandler(m_SearchWatch);
			}
		}

		Widget c = m_wContent;
		if (!c)
			return;

		m_wMessage = CGQC_GU_W.Text(c, "Chargement…", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());

		m_Tabs.m_wRoot = CGQC_GU_W.HBox(c);
		if (m_Tabs.m_wRoot)
			LayoutSlot.SetPadding(m_Tabs.m_wRoot, 0, 0, 0, 10);

		Widget row = CGQC_GU_W.HBox(c);
		if (!row)
			return;

		LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);

		Widget left = CGQC_GU_W.VBox(row);
		if (left)
		{
			LayoutSlot.SetSizeMode(left, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(left, LayoutVerticalAlign.Stretch);

			m_Zeus.m_wRoot = CGQC_GU_W.VBox(left);
			if (m_Zeus.m_wRoot)
				LayoutSlot.SetHorizontalAlign(m_Zeus.m_wRoot, LayoutHorizontalAlign.Stretch);

			m_Body.m_wRoot = CGQC_GU_W.Scroll(left);

			m_Free.m_wRoot = CGQC_GU_W.VBox(left);
			if (m_Free.m_wRoot)
			{
				LayoutSlot.SetHorizontalAlign(m_Free.m_wRoot, LayoutHorizontalAlign.Stretch);
				LayoutSlot.SetPadding(m_Free.m_wRoot, 0, 10, 0, 0);
			}
		}

		Widget asideBox = CGQC_GU_W.Sized(row, ASIDE_W);
		if (asideBox)
		{
			Widget slot = asideBox.GetParent();
			LayoutSlot.SetVerticalAlign(slot, LayoutVerticalAlign.Stretch);
			LayoutSlot.SetPadding(slot, 16, 0, 0, 0);

			Widget inner = CGQC_GU_W.Panel(asideBox);
			if (inner)
			{
				Widget panel = inner.GetParent();
				LayoutSlot.SetSizeMode(panel, LayoutSizeMode.Fill);
				LayoutSlot.SetHorizontalAlign(panel, LayoutHorizontalAlign.Stretch);
				Widget bg = panel.FindAnyWidget("Bg");
				if (bg)
					bg.SetColor(CGQC_GU_Style.PaperLight());

				AlignableSlot.SetPadding(inner, 18, 18, 18, 18);
				AlignableSlot.SetVerticalAlign(inner, LayoutVerticalAlign.Stretch);
				m_Aside.m_wRoot = inner;
			}
		}

		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		RequestOrbat();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		GetGame().GetCallqueue().Remove(DisarmSection);
		if (m_wSearch && m_SearchWatch)
			m_wSearch.RemoveHandler(m_SearchWatch);

		RemoveGiveWatch();
		m_Give.Reset();
		m_mArmLabels.Clear();
		m_mSeatRows.Clear();
		m_Tabs.Reset();
		m_Zeus.Reset();
		m_Body.Reset();
		m_Free.Reset();
		m_Aside.Reset();
	}

	//------------------------------------------------------------------------------------------------
	// Data
	//------------------------------------------------------------------------------------------------
	protected void RequestOrbat()
	{
		m_iReadReq = CGQC_GU_Client.Request(CGQC_GU_OrbatRequests.GET);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot || (requestId != m_iReadReq && requestId != m_iWriteReq))
			return;

		bool isWrite = requestId == m_iWriteReq;
		bool added = isWrite && !m_sAddSection.IsEmpty();
		if (!ok)
		{
			if (isWrite)
			{
				m_sAddSection = string.Empty;
				ShowNotice(OrbatError(code));
			}
			else
			{
				ShowMessage(OrbatError(code));
			}

			return;
		}

		CGQC_GU_OrbatDto dto = new CGQC_GU_OrbatDto();
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(payload) || !ctx.ReadValue("o", dto) || !dto)
		{
			ShowMessage("ORBAT illisible.");
			return;
		}

		m_Dto = dto;
		if (m_wMessage)
			m_wMessage.SetVisible(false);

		// A write went through: the choices made for it are spent.
		if (isWrite)
		{
			m_sNotice = string.Empty;
			m_sRolePick = string.Empty;
			m_sGiveUid = string.Empty;
			m_sGiveSearch = string.Empty;
			if (m_wGiveSearch)
				m_wGiveSearch.SetText(string.Empty);

			m_sRenaming = string.Empty;
			DisarmSection();
		}

		if (m_wOnline)
			m_wOnline.SetText(string.Format("%1 en ligne", dto.online));

		// The tab shown: kept, else the caller's unit, else the first one in use.
		CGQC_GU_OrbatUnitDto shown = FindUnit(m_sUnit);
		if (!shown || !shown.available)
		{
			shown = FindUnit(dto.myUnit);
			if (!shown || !shown.available)
				shown = FirstAvailable();
		}

		m_sUnit = string.Empty;
		if (shown)
			m_sUnit = shown.id;

		// A seat was added: select it, ready to be given.
		if (added)
		{
			string newSeat = AddedSeat();
			if (!newSeat.IsEmpty())
			{
				m_sSelUnit = m_sAddUnit;
				m_sSel = newSeat;
			}

			m_sAddSection = string.Empty;
		}

		// The chosen seat: kept while it exists, else one's own seat, else the first active seat.
		if (!FindSeat(m_sSelUnit, m_sSel))
			DefaultSelection();

		BuildAll();
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowMessage(string text)
	{
		if (!m_wMessage)
			return;

		m_wMessage.SetText(text);
		m_wMessage.SetColor(CGQC_GU_Style.Red());
		m_wMessage.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	//! A refused write: one short line in the side panel.
	protected void ShowNotice(string text)
	{
		m_sNotice = text;
		if (!m_wNotice)
			return;

		m_wNotice.SetText(text);
		m_wNotice.SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	//! ORBAT refusal codes (4a) in short French; the others as everywhere else.
	static string OrbatError(string code)
	{
		switch (code)
		{
			case "forbidden":			return "Refusé : droits insuffisants.";
			case "above_rank":			return "Refusé : grade insuffisant.";
			case "locked":				return "Refusé : place verrouillée.";
			case "occupied":			return "Refusé : place déjà prise.";
			case "inactive_section":	return "Refusé : section inactive.";
			case "unknown_seat":		return "Place inconnue.";
			case "unknown_section":		return "Section inconnue.";
			case "unknown_role":		return "Rôle inconnu.";
			case "unit_unavailable":	return "Unité indisponible.";
			case "empty_seat":			return "La place est libre.";
			case "too_long":			return "Nom trop long (24 max).";
			case "not_found":			return "Membre introuvable.";
		}

		return CGQC_GU_DossierScreen.ErrorText(code);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_OrbatUnitDto FindUnit(string id)
	{
		if (!m_Dto || id.IsEmpty())
			return null;

		foreach (CGQC_GU_OrbatUnitDto u : m_Dto.units)
		{
			if (u.id == id)
				return u;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_OrbatUnitDto FirstAvailable()
	{
		if (!m_Dto)
			return null;

		foreach (CGQC_GU_OrbatUnitDto u : m_Dto.units)
		{
			if (u.available)
				return u;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_OrbatSeatDto FindSeat(string unitId, string callsign)
	{
		CGQC_GU_OrbatSectionDto section;
		CGQC_GU_OrbatElementDto element;
		return FindSeatIn(unitId, callsign, section, element);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_OrbatSeatDto FindSeatIn(string unitId, string callsign, out CGQC_GU_OrbatSectionDto section, out CGQC_GU_OrbatElementDto element)
	{
		CGQC_GU_OrbatUnitDto u = FindUnit(unitId);
		if (!u || callsign.IsEmpty())
			return null;

		foreach (CGQC_GU_OrbatSectionDto s : u.sections)
		{
			if (!s.active)
				continue;

			foreach (CGQC_GU_OrbatElementDto e : s.elements)
			{
				foreach (CGQC_GU_OrbatSeatDto seat : e.seats)
				{
					if (seat.callsign != callsign)
						continue;

					section = s;
					element = e;
					return seat;
				}
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! A unit with an open section (UPO): drawn as the `Orbat-UPO` artboard.
	protected bool IsUpo(CGQC_GU_OrbatUnitDto u)
	{
		return OpenSection(u) != null;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_OrbatSectionDto OpenSection(CGQC_GU_OrbatUnitDto u)
	{
		if (!u)
			return null;

		foreach (CGQC_GU_OrbatSectionDto s : u.sections)
		{
			if (s.open)
				return s;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! After « + AJOUTER … »: the seat that was not there before, or "".
	protected string AddedSeat()
	{
		CGQC_GU_OrbatSectionDto s = FindSection(m_sAddSection);
		if (!s)
			return string.Empty;

		foreach (CGQC_GU_OrbatElementDto e : s.elements)
		{
			foreach (CGQC_GU_OrbatSeatDto seat : e.seats)
			{
				if (!m_aBeforeAdd.Contains(seat.callsign))
					return seat.callsign;
			}
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected void DefaultSelection()
	{
		m_sSelUnit = string.Empty;
		m_sSel = string.Empty;
		if (!m_Dto)
			return;

		if (!m_Dto.mySeat.IsEmpty() && m_Dto.myUnit == m_sUnit && FindSeat(m_Dto.myUnit, m_Dto.mySeat))
		{
			m_sSelUnit = m_Dto.myUnit;
			m_sSel = m_Dto.mySeat;
			return;
		}

		CGQC_GU_OrbatUnitDto u = FindUnit(m_sUnit);
		if (!u)
			return;

		foreach (CGQC_GU_OrbatSectionDto s : u.sections)
		{
			if (!s.active || s.command)
				continue;

			foreach (CGQC_GU_OrbatElementDto e : s.elements)
			{
				if (e.seats.IsEmpty())
					continue;

				m_sSelUnit = u.id;
				m_sSel = e.seats[0].callsign;
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Actions
	//------------------------------------------------------------------------------------------------
	void OnAction(string kind, string value)
	{
		switch (kind)
		{
			case "tab":
			{
				if (value == m_sUnit)
					return;

				m_sUnit = value;
				DefaultSelection();
				BuildAll();
				ScrollToTop();
				break;
			}

			case "seat":
			{
				array<string> parts = {};
				value.Split("|", parts, false);
				if (parts.Count() != 2)
					return;

				m_sSelUnit = parts[0];
				m_sSel = parts[1];
				m_sRolePick = string.Empty;
				m_sGiveUid = string.Empty;
				m_sNotice = string.Empty;
				// Restyle in place, so the list keeps its scroll position.
				foreach (string key, Widget w : m_mSeatRows)
				{
					if (m_aCardKeys.Contains(key))
						StyleCard(w, key == value);
					else
						StyleSeatRow(w, key == value);
				}

				BuildAside();
				break;
			}

			// UPO live seats (owner, 4e test): add an empty seat (then filled like any other), remove one.
			case "liveseat.add":
			{
				CGQC_GU_OrbatUnitDto su = FindUnit(m_sUnit);
				CGQC_GU_OrbatSectionDto addSec = FindSection(value);
				if (!su || !addSec)
					return;

				// Remember the seats there now, so the reply can select the new one.
				m_sAddUnit = su.id;
				m_sAddSection = value;
				m_aBeforeAdd.Clear();
				foreach (CGQC_GU_OrbatElementDto ae : addSec.elements)
				{
					foreach (CGQC_GU_OrbatSeatDto aseat : ae.seats)
					{
						m_aBeforeAdd.Insert(aseat.callsign);
					}
				}

				CGQC_GU_Payload ap = new CGQC_GU_Payload();
				ap.Set("op", "add").Set("section", value);
				Send(CGQC_GU_OrbatRequests.SEATS, ap);
				break;
			}

			case "liveseat.remove":
			{
				array<string> rm = {};
				value.Split("|", rm, false);
				if (rm.Count() != 2)
					return;

				CGQC_GU_Payload rp2 = new CGQC_GU_Payload();
				rp2.Set("op", "remove").Set("unit", rm[0]).Set("seat", rm[1]);
				Send(CGQC_GU_OrbatRequests.SEATS, rp2);
				break;
			}

			case "open":
				CGQC_GU_Ui.OpenNextFrame(new CGQC_GU_DossierScreen(value));
				break;

			case "section":
				OnSectionCheck(value);
				break;

			case "rename":
				m_sRenaming = value;
				BuildBody();
				break;

			case "rename.cancel":
				m_sRenaming = string.Empty;
				BuildBody();
				break;

			case "rename.save":
			{
				if (m_sRenaming.IsEmpty() || !m_wRename)
					return;

				CGQC_GU_Payload rp = new CGQC_GU_Payload();
				rp.Set("section", m_sRenaming).Set("name", m_wRename.GetText().Trim());
				Send(CGQC_GU_OrbatRequests.RENAME, rp);
				break;
			}

			case "role.prev":
				StepRole(-1);
				break;

			case "role.next":
				StepRole(1);
				break;

			case "role.set":
				SendSeat(CGQC_GU_OrbatRequests.ROLE, "role", RolePick());
				break;

			case "give":
			{
				m_sGiveUid = value;
				// Restyle in place: a rebuild would swallow the second click of a double click.
				foreach (string giveUid, Widget giveRow : m_mGiveRows)
				{
					StyleSeatRow(giveRow, giveUid == value);
				}

				UpdateAssign();
				break;
			}

			case "give.now":
				m_sGiveUid = value;
				SendSeat(CGQC_GU_OrbatRequests.ASSIGN, "uid", value);
				break;

			case "assign":
				if (!m_sGiveUid.IsEmpty())
					SendSeat(CGQC_GU_OrbatRequests.ASSIGN, "uid", m_sGiveUid);
				break;

			case "take":
				SendSeat(CGQC_GU_OrbatRequests.ASSIGN, string.Empty, string.Empty);
				break;

			case "free":
				SendSeat(CGQC_GU_OrbatRequests.FREE, string.Empty, string.Empty);
				break;

			case "lock":
				SendSeat(CGQC_GU_OrbatRequests.LOCK, "on", value);
				break;

			case "seat.lock":
			case "seat.free":
				SeatShortcut(kind, value);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Row shortcuts (owner, 4d test): double click locks / unlocks, right click frees. Held seats only.
	protected void SeatShortcut(string kind, string key)
	{
		array<string> parts = {};
		key.Split("|", parts, false);
		if (parts.Count() != 2)
			return;

		CGQC_GU_OrbatSeatDto seat = FindSeat(parts[0], parts[1]);
		if (!seat || seat.uid.IsEmpty())
			return;

		if (m_sSelUnit != parts[0] || m_sSel != parts[1])
			OnAction("seat", key);

		if (kind == "seat.free")
		{
			SendSeat(CGQC_GU_OrbatRequests.FREE, string.Empty, string.Empty);
			return;
		}

		string on = "1";
		if (seat.locked)
			on = "0";

		SendSeat(CGQC_GU_OrbatRequests.LOCK, "on", on);
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	protected void Send(string action, CGQC_GU_Payload p)
	{
		ShowNotice(string.Empty);
		m_iWriteReq = CGQC_GU_Client.RequestWith(action, p);
	}

	//------------------------------------------------------------------------------------------------
	//! A write about the chosen seat; key / value is the one extra field (role, uid, on).
	protected void SendSeat(string action, string key, string value)
	{
		if (m_sSel.IsEmpty())
			return;

		CGQC_GU_Payload p = new CGQC_GU_Payload();
		p.Set("unit", m_sSelUnit).Set("seat", m_sSel);
		if (!key.IsEmpty())
			p.Set(key, value);

		Send(action, p);
	}

	//------------------------------------------------------------------------------------------------
	//! Ticking activates at once; unticking a section that holds seats needs a second press.
	protected void OnSectionCheck(string id)
	{
		CGQC_GU_Check check;
		foreach (CGQC_GU_Check c : m_Body.m_aChecks)
		{
			if (c.GetId() == id)
			{
				check = c;
				break;
			}
		}

		CGQC_GU_OrbatSectionDto s = FindSection(id);
		if (!check || !s)
			return;

		bool on = check.IsChecked();
		if (!on && s.filled > 0 && m_sArmedSection != id)
		{
			check.SetChecked(true);
			DisarmSection();
			m_sArmedSection = id;
			TextWidget arm = m_mArmLabels.Get(id);
			if (arm)
				arm.SetVisible(true);

			GetGame().GetCallqueue().CallLater(DisarmSection, CGQC_GU_Style.CONFIRM_TIMEOUT * 1000, false);
			return;
		}

		DisarmSection();
		string onValue = "0";
		if (on)
			onValue = "1";

		CGQC_GU_Payload p = new CGQC_GU_Payload();
		p.Set("section", id).Set("on", onValue);
		Send(CGQC_GU_OrbatRequests.ACTIVATE, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void DisarmSection()
	{
		GetGame().GetCallqueue().Remove(DisarmSection);
		if (m_sArmedSection.IsEmpty())
			return;

		TextWidget arm = m_mArmLabels.Get(m_sArmedSection);
		if (arm)
			arm.SetVisible(false);

		m_sArmedSection = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_OrbatSectionDto FindSection(string id)
	{
		CGQC_GU_OrbatUnitDto u = FindUnit(m_sUnit);
		if (!u)
			return null;

		foreach (CGQC_GU_OrbatSectionDto s : u.sections)
		{
			if (s.id == id)
				return s;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	// « Rôle de la place »
	//------------------------------------------------------------------------------------------------
	//! Roles offered: UPO seats take the UPO roles, the others every role but Instructeur and Élève.
	protected void RoleChoices(notnull array<CGQC_GU_OrbatRoleDto> list)
	{
		if (!m_Dto)
			return;

		CGQC_GU_OrbatUnitDto u = FindUnit(m_sSelUnit);
		bool upo = u && u.noRankCheck;
		foreach (CGQC_GU_OrbatRoleDto r : m_Dto.roles)
		{
			if (r.id == CGQC_GU_OrbatRequests.STUDENT_ROLE)
				continue;

			if (!upo && r.id == "instructeur")
				continue;

			list.Insert(r);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_OrbatRoleDto FindRole(string id)
	{
		if (!m_Dto)
			return null;

		foreach (CGQC_GU_OrbatRoleDto r : m_Dto.roles)
		{
			if (r.id == id)
				return r;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected string RolePick()
	{
		if (!m_sRolePick.IsEmpty())
			return m_sRolePick;

		CGQC_GU_OrbatSeatDto seat = FindSeat(m_sSelUnit, m_sSel);
		if (seat)
			return seat.role;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! « Tireur · Cpl. » (Arès) and « Tireur · Sgt. » (Titan) stay apart by their minimum rank.
	protected string RoleText(string id)
	{
		CGQC_GU_OrbatRoleDto r = FindRole(id);
		if (!r)
			return "—";

		if (r.minRankAbbr.IsEmpty())
			return r.label;

		return r.label + " · " + r.minRankAbbr;
	}

	//------------------------------------------------------------------------------------------------
	protected void StepRole(int dir)
	{
		array<CGQC_GU_OrbatRoleDto> list = {};
		RoleChoices(list);
		if (list.IsEmpty())
			return;

		string current = RolePick();
		int at = -1;
		foreach (int i, CGQC_GU_OrbatRoleDto r : list)
		{
			if (r.id == current)
			{
				at = i;
				break;
			}
		}

		if (at < 0)
			at = 0;
		else
			at = (at + dir + list.Count()) % list.Count();

		m_sRolePick = list[at].id;
		if (m_wRole)
			m_wRole.SetText(RoleText(m_sRolePick));

		UpdateRoleBtn();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateRoleBtn()
	{
		if (!m_RoleBtn)
			return;

		CGQC_GU_OrbatSeatDto seat = FindSeat(m_sSelUnit, m_sSel);
		bool changed;
		if (seat)
			changed = RolePick() != seat.role;

		m_RoleBtn.SetEnabled(changed);
	}

	//------------------------------------------------------------------------------------------------
	// « Donner la place à »
	//------------------------------------------------------------------------------------------------
	protected void RemoveGiveWatch()
	{
		if (m_wGiveSearch)
		{
			m_sGiveSearch = m_wGiveSearch.GetText();
			if (m_GiveWatch)
				m_wGiveSearch.RemoveHandler(m_GiveWatch);
		}

		m_wGiveSearch = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGiveSearch()
	{
		FillGive();
	}

	//------------------------------------------------------------------------------------------------
	//! The first matches, online first (the server's order); the seat's holder is left out.
	protected void FillGive()
	{
		m_Give.Reset();
		m_mGiveRows.Clear();
		Widget list = m_Give.m_wRoot;
		CGQC_GU_OrbatSeatDto seat = FindSeat(m_sSelUnit, m_sSel);
		if (!list || !m_Dto || !seat)
			return;

		string needle;
		if (m_wGiveSearch)
			needle = CGQC_GU_Str.Lower(m_wGiveSearch.GetText().Trim());

		CGQC_GU_OrbatUnitDto u = FindUnit(m_sSelUnit);
		CGQC_GU_OrbatRoleDto role = FindRole(seat.role);
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		int shown;
		foreach (CGQC_GU_OrbatPersonDto p : m_Dto.members)
		{
			if (p.uid == seat.uid || !Found(needle, p.name + " " + p.seat))
				continue;

			if (shown == GIVE_ROWS)
				break;

			CGQC_GU_ClickRow row = Row(m_Give, list, "give", p.uid, CGQC_GU_Style.Clear(), CGQC_GU_Style.RowEdge(), edges);
			if (!row)
				continue;

			shown++;
			// Double click: straight into the chosen seat (owner, 4d test), no ASSIGNER press.
			row.SetExtraActions(new CGQC_GU_OrbatAction(this, "give.now", p.uid), null);
			m_mGiveRows.Set(p.uid, row.GetRootWidget());
			StyleSeatRow(row.GetRootWidget(), p.uid == m_sGiveUid);
			Widget cells = row.GetCells();
			AlignableSlot.SetPadding(cells, 4, 3, 4, 3);

			Color dotColor = CGQC_GU_Style.Offline();
			if (p.online)
				dotColor = CGQC_GU_Style.Online();

			ImageWidget dot = CGQC_GU_W.Dot(cells, 8, dotColor);
			if (dot)
			{
				LayoutSlot.SetVerticalAlign(dot, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(dot, 0, 0, 8, 0);
			}

			TextWidget name = CGQC_GU_W.Text(cells, p.name, CGQC_GU_Style.FONT_VALUE, 15, CGQC_GU_Style.InkSoft());
			if (name)
			{
				LayoutSlot.SetSizeMode(name, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);
				if (!p.online)
					name.SetOpacity(0.5);
			}

			if (!p.seat.IsEmpty())
			{
				TextWidget at = CGQC_GU_W.Text(cells, p.seat, CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.InkMuted());
				if (at)
				{
					LayoutSlot.SetVerticalAlign(at, LayoutVerticalAlign.Center);
					LayoutSlot.SetPadding(at, 6, 0, 0, 0);
				}
			}

			if (u && !u.noRankCheck && role && role.minRankOrder >= 0 && p.rankOrder < role.minRankOrder)
				FilledTag(cells, "HORS GRADE", HorsGrade(), 9);
		}

		if (shown == 0)
			CGQC_GU_W.Text(list, "Aucun membre.", CGQC_GU_Style.FONT_VALUE, 15, CGQC_GU_Style.InkMuted());
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateAssign()
	{
		if (m_AssignBtn)
			m_AssignBtn.SetEnabled(!m_sGiveUid.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_Button Btn(CGQC_GU_Region region, Widget parent, string text, string kind, string value = "", bool confirm = false, CGQC_GU_EButtonKind look = CGQC_GU_EButtonKind.PLAIN)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text, look, confirm);
		if (!b)
			return null;

		region.m_aButtons.Insert(b);
		b.SetCompact();
		if (!kind.IsEmpty())
		{
			CGQC_GU_OrbatAction a = new CGQC_GU_OrbatAction(this, kind, value);
			region.m_aKeep.Insert(a);
			b.GetOnActivated().Insert(a.Fire);
		}

		return b;
	}

	//------------------------------------------------------------------------------------------------
	//! A full-width button of the side panel.
	protected CGQC_GU_Button WideBtn(Widget parent, string text, string kind, string value, CGQC_GU_EButtonKind look, float padTop)
	{
		CGQC_GU_Button b = Btn(m_Aside, parent, text, kind, value, true, look);
		if (!b)
			return null;

		LayoutSlot.SetHorizontalAlign(b.GetRootWidget(), LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(b.GetRootWidget(), 0, padTop, 0, 0);
		return b;
	}

	//------------------------------------------------------------------------------------------------
	//! Typing in the search box rebuilds the lists only, so the field keeps its focus.
	protected void OnSearchChanged()
	{
		BuildBody();
		BuildFree();
	}

	//------------------------------------------------------------------------------------------------
	protected string Needle()
	{
		if (!m_wSearch)
			return string.Empty;

		return CGQC_GU_Str.Lower(m_wSearch.GetText().Trim());
	}

	//------------------------------------------------------------------------------------------------
	protected bool Found(string needle, string hay)
	{
		if (needle.IsEmpty())
			return true;

		return CGQC_GU_Str.Lower(hay).Contains(needle);
	}

	//------------------------------------------------------------------------------------------------
	protected void ScrollToTop()
	{
		if (!m_Body.m_wRoot)
			return;

		ScrollLayoutWidget scroll = ScrollLayoutWidget.Cast(m_Body.m_wRoot.GetParent());
		if (scroll)
			scroll.SetSliderPos(0, 0);
	}

	//------------------------------------------------------------------------------------------------
	// Building
	//------------------------------------------------------------------------------------------------
	protected void BuildAll()
	{
		BuildTabs();
		BuildZeus();
		BuildBody();
		BuildFree();
		BuildAside();
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_ClickRow Row(CGQC_GU_Region region, Widget parent, string kind, string value, Color fill, Color edge, array<string> edges)
	{
		CGQC_GU_OrbatAction action;
		if (!kind.IsEmpty())
			action = new CGQC_GU_OrbatAction(this, kind, value);

		CGQC_GU_ClickRow row = CGQC_GU_ClickRow.Create(parent, action, fill, edge, edges);
		if (row)
			region.m_aRows.Insert(row);

		return row;
	}

	//------------------------------------------------------------------------------------------------
	//! CGQC / UPO / GRIT, each with its patch. A unit still to come (GRIT) is greyed out.
	protected void BuildTabs()
	{
		m_Tabs.Reset();
		if (!m_Tabs.m_wRoot || !m_Dto)
			return;

		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (CGQC_GU_OrbatUnitDto u : m_Dto.units)
		{
			bool on = u.id == m_sUnit;
			Color fill = CGQC_GU_Style.Clear();
			Color edge = CGQC_GU_Style.Rule();
			Color text = CGQC_GU_Style.InkSoft();
			if (on)
			{
				fill = CGQC_GU_Style.Blue();
				edge = CGQC_GU_Style.Blue();
				text = CGQC_GU_Style.PaperText();
			}

			string kind;
			if (u.available)
				kind = "tab";

			CGQC_GU_ClickRow tab = Row(m_Tabs, m_Tabs.m_wRoot, kind, u.id, fill, edge, edges);
			if (!tab)
				continue;

			Widget root = tab.GetRootWidget();
			LayoutSlot.SetHorizontalAlign(root, LayoutHorizontalAlign.Left);
			LayoutSlot.SetPadding(root, 0, 0, 6, 0);
			if (!u.available)
				root.SetOpacity(0.45);

			Widget cells = tab.GetCells();
			AlignableSlot.SetPadding(cells, 6, 6, 14, 6);

			ImageWidget patch = Patch(cells, u.patch);
			if (patch)
				LayoutSlot.SetPadding(patch, 0, 0, 12, 0);

			Widget names = CGQC_GU_W.VBox(cells);
			if (!names)
				continue;

			LayoutSlot.SetVerticalAlign(names, LayoutVerticalAlign.Center);
			CGQC_GU_W.Text(names, CGQC_GU_Str.UpperFr(u.label), CGQC_GU_Style.FONT_TITLE, 17, text);

			string full = u.fullName;
			if (!u.available && !full.Contains("à définir"))
				full += " · à définir";

			CGQC_GU_W.Text(names, full, CGQC_GU_Style.FONT_LABEL, 13, text);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected ImageWidget Patch(Widget parent, string key)
	{
		ResourceName res;
		switch (key)
		{
			case "cgqc":	res = PATCH_CGQC; break;
			case "upo":		res = PATCH_UPO; break;
			case "grit":	res = PATCH_GRIT; break;
		}

		if (res.IsEmpty())
			return null;

		ImageWidget img = ImageWidget.Cast(CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, parent));
		if (!img)
			return null;

		if (img.LoadImageTexture(0, res))
			img.SetImage(0);

		img.SetColor(CGQC_GU_Style.White());
		img.SetSize(85, 52);
		LayoutSlot.SetVerticalAlign(img, LayoutVerticalAlign.Center);
		return img;
	}

	//------------------------------------------------------------------------------------------------
	//! ZEUS strip: the unit's command section and the online members of the Zeus group.
	protected void BuildZeus()
	{
		m_Zeus.Reset();
		CGQC_GU_OrbatUnitDto u = FindUnit(m_sUnit);
		if (!m_Zeus.m_wRoot || !u)
			return;

		CGQC_GU_OrbatSectionDto command;
		foreach (CGQC_GU_OrbatSectionDto s : u.sections)
		{
			if (s.command)
			{
				command = s;
				break;
			}
		}

		if (!command)
			return;

		Widget inner = CGQC_GU_W.Panel(m_Zeus.m_wRoot);
		if (!inner)
			return;

		LayoutSlot.SetHorizontalAlign(inner.GetParent(), LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(inner.GetParent(), 0, 0, 0, 10);
		AlignableSlot.SetPadding(inner, 14, 7, 14, 7);

		Widget line = CGQC_GU_W.HBox(inner);
		if (!line)
			return;

		LayoutSlot.SetHorizontalAlign(line, LayoutHorizontalAlign.Stretch);
		TextWidget name = CGQC_GU_W.Text(line, CGQC_GU_Str.UpperFr(command.label), CGQC_GU_Style.FONT_TITLE, 20, CGQC_GU_Style.Ink());
		if (name)
			LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);

		TextWidget kind = CGQC_GU_W.Text(line, command.kind, CGQC_GU_Style.FONT_LABEL, 16, CGQC_GU_Style.InkMuted());
		if (kind)
		{
			LayoutSlot.SetSizeMode(kind, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(kind, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(kind, 14, 0, 14, 0);
		}

		string who;
		foreach (CGQC_GU_OrbatPersonDto p : m_Dto.zeus)
		{
			if (!who.IsEmpty())
				who += ", ";

			who += p.name;
		}

		if (who.IsEmpty())
			who = "—";

		TextWidget names = CGQC_GU_W.Text(line, who, CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.InkSoft());
		if (names)
			LayoutSlot.SetVerticalAlign(names, LayoutVerticalAlign.Center);
	}

	//------------------------------------------------------------------------------------------------
	//! « Sections actives · n », then the inactive section cards, in the scroll area.
	protected void BuildBody()
	{
		m_Body.Reset();
		m_mSeatRows.Clear();
		m_aCardKeys.Clear();
		m_mArmLabels.Clear();
		m_wRename = null;
		Widget body = m_Body.m_wRoot;
		CGQC_GU_OrbatUnitDto u = FindUnit(m_sUnit);
		if (!body || !u)
			return;

		string needle = Needle();
		if (IsUpo(u))
		{
			BuildUpo(body, u, needle);
			return;
		}

		array<CGQC_GU_OrbatSectionDto> active = {};
		array<CGQC_GU_OrbatSectionDto> inactive = {};
		foreach (CGQC_GU_OrbatSectionDto s : u.sections)
		{
			if (s.command)
				continue;

			if (s.active)
				active.Insert(s);
			else
				inactive.Insert(s);
		}

		Label(body, string.Format("SECTIONS ACTIVES · %1", active.Count()), 0);
		foreach (CGQC_GU_OrbatSectionDto a : active)
		{
			SectionBox(body, u, a, needle);
		}

		if (inactive.IsEmpty())
			return;

		string inactiveCaption = "SECTIONS INACTIVES";
		if (m_Dto.canManage)
			inactiveCaption += " — cocher pour activer";

		Label(body, inactiveCaption, 4);
		int perRow = Math.Min(inactive.Count(), CARDS_PER_ROW);
		float cardW = (LEFT_W - (perRow - 1) * CARD_GAP) / perRow;
		Widget cards;
		int inRow;
		foreach (CGQC_GU_OrbatSectionDto i : inactive)
		{
			if (!cards || inRow == perRow)
			{
				cards = CGQC_GU_W.HBox(body);
				inRow = 0;
				if (cards)
					LayoutSlot.SetPadding(cards, 0, 0, 0, CARD_GAP);
			}

			if (!cards)
				break;

			InactiveCard(cards, i, cardW, inRow > 0);
			inRow++;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Label(Widget parent, string text, float padTop)
	{
		TextWidget t = CGQC_GU_W.Text(parent, text, CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Ink());
		if (t)
			LayoutSlot.SetPadding(t, 0, padTop, 0, 8);
	}

	//------------------------------------------------------------------------------------------------
	//! One active section: check, name, kind, « filled / seats », then its elements three by three.
	protected void SectionBox(Widget parent, CGQC_GU_OrbatUnitDto u, CGQC_GU_OrbatSectionDto s, string needle)
	{
		Widget inner = CGQC_GU_W.Panel(parent);
		if (!inner)
			return;

		Widget box = inner.GetParent();
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Left);
		LayoutSlot.SetPadding(box, 0, 0, 0, 10);
		AlignableSlot.SetPadding(inner, 12, 8, 12, 8);

		Widget sized = CGQC_GU_W.Sized(inner, LEFT_W - 24);
		if (!sized)
			return;

		Widget head = CGQC_GU_W.HBox(sized);
		if (head)
		{
			LayoutSlot.SetHorizontalAlign(head, LayoutHorizontalAlign.Stretch);
			bool padName = SectionCheck(head, s);
			bool renaming = m_Dto.canManage && m_sRenaming == s.id;
			if (renaming)
			{
				// Empty name = back to the units.json name.
				m_wRename = CGQC_GU_W.Field(head, s.label, 300);
				if (m_wRename)
				{
					LayoutSlot.SetVerticalAlign(m_wRename, LayoutVerticalAlign.Center);
					if (padName)
						LayoutSlot.SetPadding(m_wRename, 10, 0, 0, 0);
				}

				CGQC_GU_Button save = Btn(m_Body, head, "ENREGISTRER", "rename.save", string.Empty, true, CGQC_GU_EButtonKind.PRIMARY);
				if (save)
				{
					save.SetConfirmText("CONFIRMER");
					LayoutSlot.SetVerticalAlign(save.GetRootWidget(), LayoutVerticalAlign.Center);
					LayoutSlot.SetPadding(save.GetRootWidget(), 10, 0, 0, 0);
				}

				CGQC_GU_Button cancel = Btn(m_Body, head, "ANNULER", "rename.cancel");
				if (cancel)
				{
					LayoutSlot.SetVerticalAlign(cancel.GetRootWidget(), LayoutVerticalAlign.Center);
					LayoutSlot.SetPadding(cancel.GetRootWidget(), 6, 0, 0, 0);
				}
			}
			else
			{
				TextWidget name = CGQC_GU_W.Text(head, CGQC_GU_Str.UpperFr(s.label), CGQC_GU_Style.FONT_TITLE, 21, CGQC_GU_Style.Ink());
				if (name)
				{
					LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);
					if (padName)
						LayoutSlot.SetPadding(name, 10, 0, 0, 0);
				}
			}

			TextWidget kind = CGQC_GU_W.Text(head, s.kind, CGQC_GU_Style.FONT_LABEL, 15, CGQC_GU_Style.InkMuted());
			if (kind)
			{
				LayoutSlot.SetSizeMode(kind, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(kind, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(kind, 12, 0, 12, 0);
			}

			// Unticked once with seats held: the second press frees them.
			if (m_Dto.canManage)
			{
				TextWidget arm = CGQC_GU_W.Text(head, CGQC_GU_Style.CONFIRM_TEXT, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.ConfirmRed());
				if (arm)
				{
					LayoutSlot.SetVerticalAlign(arm, LayoutVerticalAlign.Center);
					LayoutSlot.SetPadding(arm, 0, 0, 12, 0);
					arm.SetVisible(m_sArmedSection == s.id);
					m_mArmLabels.Set(s.id, arm);
				}
			}

			string count = string.Format("%1 / %2", s.filled, s.seatCount);
			if (s.open)
				count = string.Format("%1", s.filled);

			TextWidget n = CGQC_GU_W.Text(head, count, CGQC_GU_Style.FONT_LABEL, 15, CGQC_GU_Style.InkMuted());
			if (n)
				LayoutSlot.SetVerticalAlign(n, LayoutVerticalAlign.Center);

			if (m_Dto.canManage && !renaming)
			{
				CGQC_GU_Button rename = Btn(m_Body, head, "RENOMMER", "rename", s.id);
				if (rename)
				{
					LayoutSlot.SetVerticalAlign(rename.GetRootWidget(), LayoutVerticalAlign.Center);
					LayoutSlot.SetPadding(rename.GetRootWidget(), 12, 0, 0, 0);
				}
			}
		}

		ImageWidget rule = ImageWidget.Cast(CGQC_GU_W.Rule(sized, 5, 2));
		if (rule)
			rule.SetSize(2, 2);

		Widget cols;
		int inRow;
		foreach (CGQC_GU_OrbatElementDto e : s.elements)
		{
			if (!cols || inRow == COLS)
			{
				cols = CGQC_GU_W.HBox(sized);
				inRow = 0;
				if (cols)
					LayoutSlot.SetPadding(cols, 0, 0, 0, 4);
			}

			if (!cols)
				break;

			Widget col = CGQC_GU_W.Sized(cols, COL_W);
			if (!col)
				continue;

			LayoutSlot.SetVerticalAlign(col.GetParent(), LayoutVerticalAlign.Top);
			if (inRow > 0)
				LayoutSlot.SetPadding(col.GetParent(), COL_GAP, 0, 0, 0);

			ElementColumn(col, u, s, e, needle);
			inRow++;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The section checkbox, shown to Commandement / Zeus only: ticking activates the section.
	protected bool SectionCheck(Widget parent, CGQC_GU_OrbatSectionDto s)
	{
		if (!m_Dto || !m_Dto.canManage)
			return false;

		CGQC_GU_Check check = CGQC_GU_Check.Create(parent, s.id, string.Empty, s.active);
		if (!check)
			return false;

		m_Body.m_aChecks.Insert(check);
		CGQC_GU_OrbatAction action = new CGQC_GU_OrbatAction(this, "section", s.id);
		m_Body.m_aKeep.Insert(action);
		check.GetOnChanged().Insert(action.Fire);
		LayoutSlot.SetVerticalAlign(check.GetRootWidget(), LayoutVerticalAlign.Center);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ElementColumn(Widget col, CGQC_GU_OrbatUnitDto u, CGQC_GU_OrbatSectionDto s, CGQC_GU_OrbatElementDto e, string needle)
	{
		string caption = e.id + " · " + e.label;
		if (s.open)
			caption = CGQC_GU_Str.UpperFr(s.label);

		TextWidget t = CGQC_GU_W.Text(col, CGQC_GU_Str.UpperFr(caption), CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.InkMuted());
		if (t)
			LayoutSlot.SetPadding(t, 0, 4, 0, 2);

		if (e.seats.IsEmpty())
		{
			CGQC_GU_W.Text(col, "—", CGQC_GU_Style.FONT_VALUE, 15, CGQC_GU_Style.BadgeDim());
			return;
		}

		foreach (CGQC_GU_OrbatSeatDto seat : e.seats)
		{
			SeatRow(col, u, seat, needle);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SeatRow(Widget col, CGQC_GU_OrbatUnitDto u, CGQC_GU_OrbatSeatDto seat, string needle)
	{
		string key = u.id + "|" + seat.callsign;
		bool selected = u.id == m_sSelUnit && seat.callsign == m_sSel;
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		CGQC_GU_ClickRow row = Row(m_Body, col, "seat", key, CGQC_GU_Style.Clear(), CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return;

		// Commandement / Zeus: double click locks (or unlocks) the seat, right click frees it.
		if (m_Dto && m_Dto.canManage)
			row.SetExtraActions(new CGQC_GU_OrbatAction(this, "seat.lock", key), new CGQC_GU_OrbatAction(this, "seat.free", key));

		Widget root = row.GetRootWidget();
		m_mSeatRows.Set(key, root);
		StyleSeatRow(root, selected);

		Widget cells = row.GetCells();
		AlignableSlot.SetPadding(cells, 4, 3, 4, 3);

		bool isFree = seat.uid.IsEmpty();
		Color ink = CGQC_GU_Style.InkSoft();
		if (isFree)
			ink = CGQC_GU_Style.BadgeDim();

		if (!seat.binome.IsEmpty())
		{
			Widget mark = Swatch(cells, BinomeColor(seat.binome), 10, 20);
			if (mark)
				LayoutSlot.SetPadding(mark, 0, 0, 8, 0);
		}

		Fixed(cells, seat.callsign, 50, CGQC_GU_Style.FONT_TITLE, 15, ink);
		Fixed(cells, seat.roleLabel, 118, CGQC_GU_Style.FONT_LABEL, 15, ink);

		string who = seat.name;
		if (isFree)
			who = "libre";

		TextWidget name = CGQC_GU_W.Text(cells, who, CGQC_GU_Style.FONT_VALUE, 15, ink);
		if (name)
		{
			LayoutSlot.SetSizeMode(name, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);
			// An offline holder stays listed, dimmed.
			if (!isFree && !seat.online)
				name.SetOpacity(0.5);
		}

		SeatTags(cells, seat, 10);

		if (!needle.IsEmpty() && !Found(needle, seat.callsign + " " + seat.name + " " + seat.roleLabel))
			root.SetOpacity(0.3);
	}

	//------------------------------------------------------------------------------------------------
	//! A seat row's look: a bottom rule, or the blue outline and tint of the chosen seat.
	protected void StyleSeatRow(Widget root, bool selected)
	{
		if (!root)
			return;

		Widget bg = root.FindAnyWidget("Bg");
		if (bg)
		{
			if (selected)
				bg.SetColor(CGQC_GU_Style.RowSelected());
			else
				bg.SetColor(CGQC_GU_Style.Clear());
		}

		array<string> sides = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string side : sides)
		{
			Widget e = root.FindAnyWidget(side);
			if (!e)
				continue;

			if (selected)
			{
				e.SetVisible(true);
				e.SetColor(CGQC_GU_Style.Blue());
			}
			else
			{
				e.SetVisible(side == "EdgeB");
				e.SetColor(CGQC_GU_Style.RowEdge());
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! HORS GRADE (filled, mockup), VERROUILLÉ and HORS GROUPE (outlined).
	protected void SeatTags(Widget parent, CGQC_GU_OrbatSeatDto seat, int size)
	{
		if (seat.uid.IsEmpty())
			return;

		if (seat.horsGrade)
			FilledTag(parent, "HORS GRADE", HorsGrade(), size);

		if (seat.locked)
			SmallTag(parent, "VERROUILLÉ", CGQC_GU_Style.Ink(), size);

		if (seat.online && seat.pending)
			SmallTag(parent, "HORS GROUPE", CGQC_GU_Style.Red(), size);
	}

	//------------------------------------------------------------------------------------------------
	protected Widget SmallTag(Widget parent, string text, Color color, int size)
	{
		Widget box = CGQC_GU_W.Tag(parent, text, color, size);
		if (box)
			LayoutSlot.SetPadding(box, 6, 0, 0, 0);

		return box;
	}

	//------------------------------------------------------------------------------------------------
	protected Widget FilledTag(Widget parent, string text, Color fill, int size)
	{
		Widget box = SmallTag(parent, text, fill, size);
		if (!box)
			return null;

		Widget bg = box.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(fill);

		Widget inner = box.FindAnyWidget("Inner");
		if (inner)
		{
			TextWidget t = TextWidget.Cast(inner.GetChildren());
			if (t)
				t.SetColor(CGQC_GU_Style.White());
		}

		return box;
	}

	//------------------------------------------------------------------------------------------------
	//! Inactive section card: name, « n places », kind, and its elements on one line.
	protected void InactiveCard(Widget row, CGQC_GU_OrbatSectionDto s, float width, bool gap)
	{
		Widget sized = CGQC_GU_W.Sized(row, width);
		if (!sized)
			return;

		LayoutSlot.SetVerticalAlign(sized.GetParent(), LayoutVerticalAlign.Stretch);
		if (gap)
			LayoutSlot.SetPadding(sized.GetParent(), CARD_GAP, 0, 0, 0);

		Widget inner = CGQC_GU_W.Panel(sized);
		if (!inner)
			return;

		Widget box = inner.GetParent();
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetSizeMode(box, LayoutSizeMode.Fill);
		AlignableSlot.SetPadding(inner, 10, 8, 10, 8);

		Widget top = CGQC_GU_W.HBox(inner);
		if (top)
		{
			LayoutSlot.SetHorizontalAlign(top, LayoutHorizontalAlign.Stretch);
			bool padName = SectionCheck(top, s);
			TextWidget name = CGQC_GU_W.Text(top, CGQC_GU_Str.UpperFr(s.label), CGQC_GU_Style.FONT_TITLE, 17, CGQC_GU_Style.Ink());
			if (name)
			{
				LayoutSlot.SetSizeMode(name, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);
				if (padName)
					LayoutSlot.SetPadding(name, 8, 0, 0, 0);
			}

			string places = string.Format("%1 places", s.seatCount);
			if (s.open)
				places = "ouverte";

			TextWidget n = CGQC_GU_W.Text(top, places, CGQC_GU_Style.FONT_LABEL, 14, CGQC_GU_Style.InkMuted());
			if (n)
				LayoutSlot.SetVerticalAlign(n, LayoutVerticalAlign.Center);
		}

		CGQC_GU_W.Para(inner, s.kind, CGQC_GU_Style.FONT_LABEL, 14, CGQC_GU_Style.InkMuted(), 4);

		string line;
		foreach (CGQC_GU_OrbatElementDto e : s.elements)
		{
			if (!line.IsEmpty())
				line += " · ";

			line += e.id + " " + ShortElement(e.label);
		}

		if (!line.IsEmpty())
			CGQC_GU_W.Para(inner, line, CGQC_GU_Style.FONT_VALUE, 14, CGQC_GU_Style.InkSoft(), 4);
	}

	//------------------------------------------------------------------------------------------------
	//! « Élément SL » → « SL », « Équipe 1 » → « Éq. 1 » (mockup cards).
	protected string ShortElement(string label)
	{
		string prefixSl = "Élément ";
		string prefixTeam = "Équipe ";
		if (label.IndexOf(prefixSl) == 0)
			return label.Substring(prefixSl.Length(), label.Length() - prefixSl.Length());

		if (label.IndexOf(prefixTeam) == 0)
			return "Éq. " + label.Substring(prefixTeam.Length(), label.Length() - prefixTeam.Length());

		return label;
	}

	//------------------------------------------------------------------------------------------------
	// UPO tab (4e, artboard `Orbat-UPO`)
	//------------------------------------------------------------------------------------------------
	//! INSTRUCTEURS cards two by two, then the ÉLÈVES list.
	protected void BuildUpo(Widget body, CGQC_GU_OrbatUnitDto u, string needle)
	{
		foreach (CGQC_GU_OrbatSectionDto s : u.sections)
		{
			if (s.command)
				continue;

			// Both UPO sections have live seats: the seat role tells the student list from the instructor cards.
			if (s.seatRole == CGQC_GU_OrbatRequests.STUDENT_ROLE)
				StudentsBox(body, u, s, needle);
			else
				InstructorsBox(body, u, s, needle);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A UPO box: name, kind, count, « + AJOUTER … » (Commandement / Zeus), a rule. No checkbox (owner).
	//! Returns the content stack, or null for an inactive section (its caption only).
	protected Widget UpoBox(Widget parent, CGQC_GU_OrbatSectionDto s, int titleSize, int kindSize, string count, string addText)
	{
		Widget inner = CGQC_GU_W.Panel(parent);
		if (!inner)
			return null;

		Widget box = inner.GetParent();
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Left);
		LayoutSlot.SetPadding(box, 0, 0, 0, 10);
		AlignableSlot.SetPadding(inner, 18, 12, 18, 12);

		Widget sized = CGQC_GU_W.Sized(inner, LEFT_W - 36);
		if (!sized)
			return null;

		Widget head = CGQC_GU_W.HBox(sized);
		if (head)
		{
			LayoutSlot.SetHorizontalAlign(head, LayoutHorizontalAlign.Stretch);
			TextWidget name = CGQC_GU_W.Text(head, CGQC_GU_Str.UpperFr(s.label), CGQC_GU_Style.FONT_TITLE, titleSize, CGQC_GU_Style.Ink());
			if (name)
				LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);

			string kindText = s.kind;
			if (!s.active)
				kindText = "inactive";

			TextWidget kind = CGQC_GU_W.Text(head, kindText, CGQC_GU_Style.FONT_LABEL, kindSize, CGQC_GU_Style.InkMuted());
			if (kind)
			{
				LayoutSlot.SetSizeMode(kind, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(kind, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(kind, 12, 0, 12, 0);
			}

			if (s.active)
			{
				TextWidget n = CGQC_GU_W.Text(head, count, CGQC_GU_Style.FONT_TITLE, 18, CGQC_GU_Style.Ink());
				if (n)
					LayoutSlot.SetVerticalAlign(n, LayoutVerticalAlign.Center);

				// Adds an empty seat (double press); it is then filled like any other.
				if (m_Dto && m_Dto.canManage)
				{
					CGQC_GU_Button add = Btn(m_Body, head, addText, "liveseat.add", s.id, true);
					if (add)
					{
						add.SetConfirmText("CONFIRMER");
						LayoutSlot.SetVerticalAlign(add.GetRootWidget(), LayoutVerticalAlign.Center);
						LayoutSlot.SetPadding(add.GetRootWidget(), 12, 0, 0, 0);
					}
				}
			}
		}

		if (!s.active)
			return null;

		ImageWidget rule = ImageWidget.Cast(CGQC_GU_W.Rule(sized, 8, 4));
		if (rule)
			rule.SetSize(2, 2);

		return sized;
	}

	//------------------------------------------------------------------------------------------------
	//! INSTRUCTEURS « n / seats »: one card per seat, two per line; I-0 is the teacher. Anyone takes one.
	protected void InstructorsBox(Widget body, CGQC_GU_OrbatUnitDto u, CGQC_GU_OrbatSectionDto s, string needle)
	{
		Widget sized = UpoBox(body, s, 28, 17, string.Format("%1 / %2", s.filled, s.seatCount), "+ AJOUTER UN INSTRUCTEUR");
		if (!sized)
			return;

		float cardW = (LEFT_W - 36 - UPO_GAP) / 2;
		Widget line;
		int inRow;
		foreach (CGQC_GU_OrbatElementDto e : s.elements)
		{
			foreach (CGQC_GU_OrbatSeatDto seat : e.seats)
			{
				if (!line || inRow == 2)
				{
					line = CGQC_GU_W.HBox(sized);
					inRow = 0;
					if (!line)
						return;

					LayoutSlot.SetPadding(line, 0, 4, 0, 4);
				}

				Widget col = CGQC_GU_W.Sized(line, cardW, 76);
				if (!col)
					continue;

				if (inRow > 0)
					LayoutSlot.SetPadding(col.GetParent(), UPO_GAP, 0, 0, 0);

				InstructorCard(col, u, seat, needle);
				inRow++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! « I-0  Cpl. Roussel / Enseigne : Q-MB », or « libre ».
	protected void InstructorCard(Widget col, CGQC_GU_OrbatUnitDto u, CGQC_GU_OrbatSeatDto seat, string needle)
	{
		string key = u.id + "|" + seat.callsign;
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		CGQC_GU_ClickRow row = Row(m_Body, col, "seat", key, CGQC_GU_Style.Clear(), CGQC_GU_Style.Rule(), edges);
		if (!row)
			return;

		// Commandement / Zeus: double click locks (or unlocks), right click empties the seat (4d).
		if (m_Dto && m_Dto.canManage)
			row.SetExtraActions(new CGQC_GU_OrbatAction(this, "seat.lock", key), new CGQC_GU_OrbatAction(this, "seat.free", key));

		Widget root = row.GetRootWidget();
		LayoutSlot.SetSizeMode(root, LayoutSizeMode.Fill);
		m_mSeatRows.Set(key, root);
		m_aCardKeys.Insert(key);
		StyleCard(root, u.id == m_sSelUnit && seat.callsign == m_sSel);

		Widget cells = row.GetCells();
		AlignableSlot.SetPadding(cells, 14, 8, 14, 8);

		bool isFree = seat.uid.IsEmpty();
		Fixed(cells, seat.callsign, 64, CGQC_GU_Style.FONT_TITLE, 24, CGQC_GU_Style.Ink());

		Widget names = CGQC_GU_W.VBox(cells);
		if (names)
		{
			LayoutSlot.SetSizeMode(names, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(names, LayoutVerticalAlign.Center);
			if (isFree)
			{
				CGQC_GU_W.Text(names, "libre", CGQC_GU_Style.FONT_LABEL, 22, CGQC_GU_Style.BadgeDim());
			}
			else
			{
				TextWidget name = CGQC_GU_W.Text(names, seat.name, CGQC_GU_Style.FONT_TITLE, 22, CGQC_GU_Style.Ink());
				if (name && !seat.online)
					name.SetOpacity(0.5);

				string teaches = seat.teaches;
				if (teaches.IsEmpty())
					teaches = "—";

				CGQC_GU_W.Text(names, "Enseigne : " + teaches, CGQC_GU_Style.FONT_LABEL, 15, CGQC_GU_Style.InkMuted());
			}
		}

		SeatTags(cells, seat, 10);

		if (!needle.IsEmpty() && !Found(needle, seat.callsign + " " + seat.name + " " + seat.teaches))
			root.SetOpacity(0.3);
	}

	//------------------------------------------------------------------------------------------------
	//! An instructor card: always outlined; the chosen one blue and tinted.
	protected void StyleCard(Widget root, bool selected)
	{
		if (!root)
			return;

		Widget bg = root.FindAnyWidget("Bg");
		if (bg)
		{
			if (selected)
				bg.SetColor(CGQC_GU_Style.RowSelected());
			else
				bg.SetColor(CGQC_GU_Style.Clear());
		}

		array<string> sides = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string side : sides)
		{
			Widget e = root.FindAnyWidget(side);
			if (!e)
				continue;

			e.SetVisible(true);
			if (selected)
				e.SetColor(CGQC_GU_Style.Blue());
			else
				e.SetColor(CGQC_GU_Style.Rule());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! ÉLÈVES « n / seats », + AJOUTER UN ÉLÈVE, then one row per seat (empty or held) with RETIRER.
	protected void StudentsBox(Widget body, CGQC_GU_OrbatUnitDto u, CGQC_GU_OrbatSectionDto s, string needle)
	{
		Widget sized = UpoBox(body, s, 24, 16, string.Format("%1 / %2", s.filled, s.seatCount), "+ AJOUTER UN ÉLÈVE");
		if (!sized)
			return;

		int shown;
		foreach (CGQC_GU_OrbatElementDto e : s.elements)
		{
			foreach (CGQC_GU_OrbatSeatDto seat : e.seats)
			{
				StudentRow(sized, u, seat, needle);
				shown++;
			}
		}

		if (shown == 0)
			CGQC_GU_W.Para(sized, "—", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.BadgeDim(), 4);
	}

	//------------------------------------------------------------------------------------------------
	//! « E-1  Cdt. Nadeau   Q-MB · en cours   [RETIRER] », or « E-5  libre ». RETIRER removes the seat
	//! (Commandement / Zeus, double press); right click only empties it.
	protected void StudentRow(Widget parent, CGQC_GU_OrbatUnitDto u, CGQC_GU_OrbatSeatDto seat, string needle)
	{
		Widget line = CGQC_GU_W.HBox(parent);
		if (!line)
			return;

		LayoutSlot.SetHorizontalAlign(line, LayoutHorizontalAlign.Stretch);
		string key = u.id + "|" + seat.callsign;
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		CGQC_GU_ClickRow row = Row(m_Body, line, "seat", key, CGQC_GU_Style.Clear(), CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return;

		bool manage = m_Dto && m_Dto.canManage;
		if (manage)
			row.SetExtraActions(new CGQC_GU_OrbatAction(this, "seat.lock", key), new CGQC_GU_OrbatAction(this, "seat.free", key));

		Widget root = row.GetRootWidget();
		LayoutSlot.SetSizeMode(root, LayoutSizeMode.Fill);
		LayoutSlot.SetVerticalAlign(root, LayoutVerticalAlign.Stretch);
		m_mSeatRows.Set(key, root);
		StyleSeatRow(root, u.id == m_sSelUnit && seat.callsign == m_sSel);

		Widget cells = row.GetCells();
		AlignableSlot.SetPadding(cells, 4, 6, 4, 6);

		bool isFree = seat.uid.IsEmpty();
		Color ink = CGQC_GU_Style.InkSoft();
		if (isFree)
			ink = CGQC_GU_Style.BadgeDim();

		Fixed(cells, seat.callsign, 56, CGQC_GU_Style.FONT_TITLE, 17, ink);

		string who = seat.name;
		ResourceName whoFont = CGQC_GU_Style.FONT_TITLE;
		if (isFree)
		{
			who = "libre";
			whoFont = CGQC_GU_Style.FONT_LABEL;
		}

		TextWidget name = CGQC_GU_W.Text(cells, who, whoFont, 17, ink);
		if (name)
		{
			LayoutSlot.SetSizeMode(name, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);
			if (!isFree && !seat.online)
				name.SetOpacity(0.5);
		}

		if (!seat.progress.IsEmpty())
		{
			TextWidget prog = CGQC_GU_W.Text(cells, seat.progress, CGQC_GU_Style.FONT_LABEL, 15, CGQC_GU_Style.InkMuted());
			if (prog)
			{
				LayoutSlot.SetVerticalAlign(prog, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(prog, 8, 0, 4, 0);
			}
		}

		SeatTags(cells, seat, 10);

		if (manage)
		{
			CGQC_GU_Button remove = Btn(m_Body, line, "RETIRER", "liveseat.remove", key, true);
			if (remove)
			{
				remove.SetConfirmText("CONFIRMER");
				LayoutSlot.SetVerticalAlign(remove.GetRootWidget(), LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(remove.GetRootWidget(), 8, 0, 0, 0);
			}
		}

		if (!needle.IsEmpty() && !Found(needle, seat.callsign + " " + seat.name + " " + seat.progress))
			line.SetOpacity(0.3);
	}

	//------------------------------------------------------------------------------------------------
	//! « Encadrement de la séance » → « encadrement ».
	protected string KindWord(string kind)
	{
		string word = kind.Trim();
		int space = word.IndexOf(" ");
		if (space > 0)
			word = word.Substring(0, space);

		return CGQC_GU_Str.Lower(word);
	}

	//------------------------------------------------------------------------------------------------
	//! « Non assignés · n »: online members with no seat (Zeus apart).
	protected void BuildFree()
	{
		m_Free.Reset();
		// UPO tab: no « Non assignés » strip (artboard `Orbat-UPO`).
		if (!m_Free.m_wRoot || !m_Dto || IsUpo(FindUnit(m_sUnit)))
			return;

		Widget inner = CGQC_GU_W.Panel(m_Free.m_wRoot);
		if (!inner)
			return;

		LayoutSlot.SetHorizontalAlign(inner.GetParent(), LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetPadding(inner, 14, 7, 14, 7);

		string needle = Needle();
		Widget line = CGQC_GU_W.HBox(inner);
		if (!line)
			return;

		TextWidget cap = CGQC_GU_W.Text(line, string.Format("NON ASSIGNÉS · %1", m_Dto.unassigned.Count()), CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Ink());
		if (cap)
		{
			LayoutSlot.SetVerticalAlign(cap, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(cap, 0, 0, 6, 0);
		}

		int inRow;
		foreach (CGQC_GU_OrbatPersonDto p : m_Dto.unassigned)
		{
			if (inRow == FREE_TAGS_PER_ROW)
			{
				line = CGQC_GU_W.HBox(inner);
				inRow = 0;
				if (!line)
					break;

				LayoutSlot.SetPadding(line, 0, 6, 0, 0);
			}

			Widget tag = PersonTag(line, p.name);
			if (tag && !needle.IsEmpty() && !Found(needle, p.name))
				tag.SetOpacity(0.3);

			inRow++;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Outlined tag « ● Sdt. Fortin »; the dot is drawn (no ● glyph in the game fonts).
	protected Widget PersonTag(Widget parent, string name)
	{
		Widget box = SmallTag(parent, string.Empty, CGQC_GU_Style.InkSoft(), 14);
		if (!box)
			return null;

		LayoutSlot.SetPadding(box, 10, 0, 0, 0);
		Widget inner = box.FindAnyWidget("Inner");
		CGQC_GU_W.Clear(inner);
		Widget row = CGQC_GU_W.HBox(inner);
		if (!row)
			return box;

		ImageWidget dot = CGQC_GU_W.Dot(row, 7, CGQC_GU_Style.Online());
		if (dot)
			LayoutSlot.SetPadding(dot, 0, 0, 6, 0);

		TextWidget t = CGQC_GU_W.Text(row, name, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkSoft());
		if (t)
			LayoutSlot.SetVerticalAlign(t, LayoutVerticalAlign.Center);

		return box;
	}

	//------------------------------------------------------------------------------------------------
	//! « PLACE CHOISIE »: seat and role, where it sits, who holds it, the controls, OUVRIR LE DOSSIER.
	protected void BuildAside()
	{
		RemoveGiveWatch();
		m_Give.Reset();
		m_Give.m_wRoot = null;
		m_Aside.Reset();
		m_wNotice = null;
		m_wRole = null;
		m_RoleBtn = null;
		m_AssignBtn = null;
		Widget a = m_Aside.m_wRoot;
		if (!a)
			return;

		CGQC_GU_W.Text(a, "PLACE CHOISIE", CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.InkMuted());

		CGQC_GU_OrbatSectionDto section;
		CGQC_GU_OrbatElementDto element;
		CGQC_GU_OrbatSeatDto seat = FindSeatIn(m_sSelUnit, m_sSel, section, element);
		if (!seat)
		{
			CGQC_GU_W.Text(a, "—", CGQC_GU_Style.FONT_TITLE, 28, CGQC_GU_Style.BadgeDim());
			return;
		}

		CGQC_GU_W.Para(a, seat.callsign + " · " + CGQC_GU_Str.UpperFr(seat.roleLabel), CGQC_GU_Style.FONT_TITLE, 28, CGQC_GU_Style.Ink(), 8);

		// « Spartan · équipe S1 · binôme rouge »
		// UPO: « UPO · encadrement », « UPO · instruction ».
		CGQC_GU_OrbatUnitDto selUnit = FindUnit(m_sSelUnit);
		bool upo = IsUpo(selUnit);
		string where;
		if (upo)
			where = selUnit.label + " · " + KindWord(section.kind);
		else if (section.open || !element)
			where = section.label;
		else
			where = section.label + " · " + ElementWords(element);

		if (!seat.binome.IsEmpty())
			where += " · binôme " + BinomeWord(seat.binome);

		Widget line = CGQC_GU_W.HBox(a);
		if (line)
		{
			LayoutSlot.SetHorizontalAlign(line, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(line, 0, 10, 0, 0);
			if (!seat.binome.IsEmpty())
			{
				Widget mark = Swatch(line, BinomeColor(seat.binome), 10, 18);
				if (mark)
					LayoutSlot.SetPadding(mark, 0, 0, 8, 0);
			}

			TextWidget w = CGQC_GU_W.Text(line, where, CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.InkMuted(), true);
			if (w)
			{
				LayoutSlot.SetSizeMode(w, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(w, LayoutVerticalAlign.Center);
			}
		}

		string held = "Libre";
		if (!seat.uid.IsEmpty())
			held = "Occupée par " + seat.name;

		// « Occupée par Cpl. Roussel (autorisé Q-MB) » (artboard `Orbat-UPO`).
		if (upo && !seat.uid.IsEmpty())
		{
			if (!seat.teaches.IsEmpty())
				held += " (autorisé " + seat.teaches + ")";
			else if (!seat.progress.IsEmpty())
				held += " (" + seat.progress + ")";
		}

		CGQC_GU_W.Para(a, held, CGQC_GU_Style.FONT_LABEL, 17, CGQC_GU_Style.InkMuted(), 10);

		Widget tags = CGQC_GU_W.HBox(a);
		if (tags)
		{
			LayoutSlot.SetPadding(tags, 0, 10, 0, 0);
			SeatTags(tags, seat, 12);
		}

		m_wNotice = CGQC_GU_W.Para(a, m_sNotice, CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Red(), 10);
		if (m_wNotice)
			m_wNotice.SetVisible(!m_sNotice.IsEmpty());

		if (m_Dto && m_Dto.canManage)
			ManageControls(a, seat);
		else
			MemberControls(a, seat);

		Widget spacer = CGQC_GU_W.VBox(a);
		if (spacer)
			LayoutSlot.SetSizeMode(spacer, LayoutSizeMode.Fill);

		CGQC_GU_Button open = CGQC_GU_Button.Create(a, "OUVRIR LE DOSSIER");
		if (!open)
			return;

		m_Aside.m_aButtons.Insert(open);
		LayoutSlot.SetHorizontalAlign(open.GetRootWidget(), LayoutHorizontalAlign.Stretch);
		open.SetEnabled(!seat.uid.IsEmpty());
		CGQC_GU_OrbatAction action = new CGQC_GU_OrbatAction(this, "open", seat.uid);
		m_Aside.m_aKeep.Insert(action);
		open.GetOnActivated().Insert(action.Fire);
	}

	//------------------------------------------------------------------------------------------------
	//! Commandement / Zeus: « Rôle de la place », « Donner la place à », ASSIGNER, LIBÉRER, VERROUILLER.
	protected void ManageControls(Widget a, CGQC_GU_OrbatSeatDto seat)
	{
		CGQC_GU_W.Caption(a, "RÔLE DE LA PLACE", 16);
		Widget line = CGQC_GU_W.HBox(a);
		if (line)
		{
			LayoutSlot.SetHorizontalAlign(line, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(line, 0, 4, 0, 0);
			Btn(m_Aside, line, "<", "role.prev");
			m_wRole = CGQC_GU_W.Text(line, RoleText(RolePick()), CGQC_GU_Style.FONT_TITLE, 17, CGQC_GU_Style.Ink());
			if (m_wRole)
			{
				LayoutSlot.SetSizeMode(m_wRole, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(m_wRole, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(m_wRole, 10, 0, 10, 0);
			}

			Btn(m_Aside, line, ">", "role.next");
		}

		m_RoleBtn = WideBtn(a, "CHANGER LE RÔLE", "role.set", string.Empty, CGQC_GU_EButtonKind.PLAIN, 6);
		UpdateRoleBtn();

		CGQC_GU_W.Caption(a, "DONNER LA PLACE À", 16);
		m_wGiveSearch = CGQC_GU_W.Field(a, m_sGiveSearch, ASIDE_W - 36);
		if (m_wGiveSearch)
		{
			LayoutSlot.SetPadding(m_wGiveSearch, 0, 4, 0, 0);
			if (!m_GiveWatch)
			{
				m_GiveWatch = new CGQC_GU_FieldWatch();
				m_GiveWatch.GetOnChanged().Insert(OnGiveSearch);
			}

			m_wGiveSearch.AddHandler(m_GiveWatch);
		}

		m_Give.m_wRoot = CGQC_GU_W.VBox(a);
		if (m_Give.m_wRoot)
		{
			LayoutSlot.SetHorizontalAlign(m_Give.m_wRoot, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(m_Give.m_wRoot, 0, 6, 0, 0);
		}

		FillGive();

		m_AssignBtn = WideBtn(a, "ASSIGNER LA PLACE", "assign", string.Empty, CGQC_GU_EButtonKind.PRIMARY, 8);
		UpdateAssign();

		bool held = !seat.uid.IsEmpty();
		Widget row = CGQC_GU_W.HBox(a);
		if (!row)
			return;

		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(row, 0, 8, 0, 0);

		CGQC_GU_Button freeBtn = Btn(m_Aside, row, "LIBÉRER", "free", string.Empty, true);
		if (freeBtn)
		{
			freeBtn.SetConfirmText("CONFIRMER");
			freeBtn.SetEnabled(held);
			LayoutSlot.SetSizeMode(freeBtn.GetRootWidget(), LayoutSizeMode.Fill);
		}

		string lockText = "VERROUILLER";
		string lockValue = "1";
		if (seat.locked)
		{
			lockText = "DÉVERROUILLER";
			lockValue = "0";
		}

		CGQC_GU_Button lockBtn = Btn(m_Aside, row, lockText, "lock", lockValue, true);
		if (lockBtn)
		{
			lockBtn.SetConfirmText("CONFIRMER");
			lockBtn.SetEnabled(held);
			LayoutSlot.SetSizeMode(lockBtn.GetRootWidget(), LayoutSizeMode.Fill);
			LayoutSlot.SetPadding(lockBtn.GetRootWidget(), 8, 0, 0, 0);
		}

		// UPO live seat (4e): remove the seat itself; its holder is freed.
		CGQC_GU_OrbatSectionDto section;
		CGQC_GU_OrbatElementDto element;
		FindSeatIn(m_sSelUnit, m_sSel, section, element);
		if (section && section.open)
		{
			CGQC_GU_Button removeBtn = WideBtn(a, "RETIRER LA PLACE", "liveseat.remove", m_sSelUnit + "|" + m_sSel, CGQC_GU_EButtonKind.PLAIN, 8);
			if (removeBtn)
				removeBtn.SetConfirmText("CONFIRMER");
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Members: take a free seat, or leave one's own (not while locked).
	protected void MemberControls(Widget a, CGQC_GU_OrbatSeatDto seat)
	{
		if (!m_Dto)
			return;

		if (seat.uid.IsEmpty())
		{
			CGQC_GU_Button take = WideBtn(a, "PRENDRE LA PLACE", "take", string.Empty, CGQC_GU_EButtonKind.PRIMARY, 16);
			if (take)
				take.SetEnabled(!m_Dto.myLocked);

			return;
		}

		if (!seat.mine)
			return;

		CGQC_GU_Button leave = WideBtn(a, "QUITTER LA PLACE", "free", string.Empty, CGQC_GU_EButtonKind.PLAIN, 16);
		if (leave)
			leave.SetEnabled(!seat.locked);
	}

	//------------------------------------------------------------------------------------------------
	//! « équipe S1 » for a team, else « R1 Orion ».
	protected string ElementWords(CGQC_GU_OrbatElementDto e)
	{
		if (e.label.IndexOf("Équipe") == 0)
			return "équipe " + e.id;

		if (e.label.IndexOf("Élément") == 0)
			return "élément " + e.id;

		return e.id + " " + e.label;
	}

	//------------------------------------------------------------------------------------------------
	// Small drawings
	//------------------------------------------------------------------------------------------------
	//! Fixed-width text cell of a row, vertically centred.
	protected void Fixed(Widget row, string text, float width, ResourceName font, int size, Color color)
	{
		Widget box = CGQC_GU_W.Sized(row, width);
		if (!box)
			return;

		LayoutSlot.SetVerticalAlign(box.GetParent(), LayoutVerticalAlign.Center);
		LayoutSlot.SetPadding(box.GetParent(), 0, 0, 8, 0);
		CGQC_GU_W.Text(box, text, font, size, color);
	}

	//------------------------------------------------------------------------------------------------
	//! Binôme marker: a coloured bar with a thin ink outline (white must show on paper).
	protected Widget Swatch(Widget parent, Color color, float w, float h)
	{
		SizeLayoutWidget s = SizeLayoutWidget.Cast(CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_CANVAS, parent));
		if (!s)
			return null;

		s.EnableWidthOverride(true);
		s.SetWidthOverride(w);
		s.EnableHeightOverride(true);
		s.SetHeightOverride(h);
		LayoutSlot.SetVerticalAlign(s, LayoutVerticalAlign.Center);

		Widget c = s.FindAnyWidget("Canvas");
		if (!c)
			return s;

		CanvasRect(c, 0, 0, w, h, CGQC_GU_Style.Rule());
		CanvasRect(c, 1, 1, w - 2, h - 2, color);
		return s;
	}

	//------------------------------------------------------------------------------------------------
	protected void CanvasRect(Widget canvas, float x, float y, float w, float h, Color color)
	{
		Widget r = CGQC_GU_W.Create(CGQC_GU_Style.LAYOUT_RECT, canvas);
		if (!r)
			return;

		FrameSlot.SetPos(r, x, y);
		FrameSlot.SetSize(r, w, h);
		r.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	static Color BinomeColor(string binome)
	{
		switch (binome)
		{
			case "white":	return Color.FromSRGBA(0xFF, 0xFF, 0xFF, 255);
			case "red":		return Color.FromSRGBA(0xC0, 0x39, 0x2B, 255);
			case "green":	return Color.FromSRGBA(0x2E, 0x8B, 0x57, 255);
			case "blue":	return Color.FromSRGBA(0x2F, 0x5F, 0xD0, 255);
			case "yellow":	return Color.FromSRGBA(0xE0, 0xB0, 0x20, 255);
		}

		return CGQC_GU_Style.Clear();
	}

	//------------------------------------------------------------------------------------------------
	static string BinomeWord(string binome)
	{
		switch (binome)
		{
			case "white":	return "blanc";
			case "red":		return "rouge";
			case "green":	return "vert";
			case "blue":	return "bleu";
			case "yellow":	return "jaune";
		}

		return binome;
	}

	//------------------------------------------------------------------------------------------------
	//! HORS GRADE tag fill (mockup #B5651D).
	static Color HorsGrade()
	{
		return Color.FromSRGBA(0xB5, 0x65, 0x1D, 255);
	}
}
