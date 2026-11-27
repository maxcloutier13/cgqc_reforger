// Gestion d'unité - Dossier (paper), spec §5 "Dossier (paper)", Phase 1d.
// Left: the fiche (insignia, name, grade, access, presence, collapsible personal section, footer).
// Right: the member selector, then the open tab: Registre or Grade & accès.
// Everything shown comes from the server's "dossier" reply; every save is a double press and is checked
// again on the server. After a save the dossier is read again, so the screen always shows the file.

//! Binds a button to a named action of the screen, with an optional value (rank id, member uid, page...).
class CGQC_GU_DossierAction : CGQC_GU_Action
{
	protected CGQC_GU_DossierScreen m_Screen;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_DossierAction(CGQC_GU_DossierScreen screen, string kind, string value)
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

//! A part of the screen that is rebuilt as a whole: its widgets and the script objects behind them.
class CGQC_GU_Region
{
	Widget m_wRoot;
	ref array<ref CGQC_GU_Button> m_aButtons = new array<ref CGQC_GU_Button>();
	ref array<ref CGQC_GU_Check> m_aChecks = new array<ref CGQC_GU_Check>();
	ref array<ref CGQC_GU_ClickRow> m_aRows = new array<ref CGQC_GU_ClickRow>();
	ref array<ref Managed> m_aKeep = new array<ref Managed>();
	ref array<ref CGQC_GU_DateField> m_aDates = new array<ref CGQC_GU_DateField>();

	//------------------------------------------------------------------------------------------------
	void Reset()
	{
		foreach (CGQC_GU_DateField date : m_aDates)
		{
			date.Destroy();
		}

		m_aDates.Clear();
		foreach (CGQC_GU_Button b : m_aButtons)
		{
			b.Destroy();
		}

		foreach (CGQC_GU_Check c : m_aChecks)
		{
			c.Destroy();
		}

		foreach (CGQC_GU_ClickRow row : m_aRows)
		{
			row.Destroy();
		}

		m_aButtons.Clear();
		m_aChecks.Clear();
		m_aRows.Clear();
		m_aKeep.Clear();
		CGQC_GU_W.Clear(m_wRoot);
	}
}

class CGQC_GU_DossierScreen : CGQC_GU_PaperScreen
{
	static const int RECORD_PAGE = 10;
	static const int MEMBER_PAGE = 10;
	static const float FICHE_WIDTH = 420;
	static const float LINE_LABEL_WIDTH = 195;
	static const int BADGE_TAGS_PER_ROW = 4;

	// Record types staff may add by hand (enlistment is written by the system only).
	protected ref array<string> m_aManualTypes = { "note", "task", "operation", "deployment", "session", "qualification", "distinction" };

	protected string m_sUid;		// "" = own dossier until the first reply
	protected ref CGQC_GU_DossierDto m_Dto;
	protected ref array<ref CGQC_GU_MemberSummary> m_aMembers;

	protected int m_iDossierReq;
	protected int m_iListReq;
	protected int m_iWriteReq;

	protected CGQC_GU_ERailTab m_eTab;
	protected int m_iRecordPage;
	protected int m_iMemberPage;
	protected int m_iManualType;
	protected bool m_bPersonalOpen;
	protected bool m_bEditingPersonal;
	protected bool m_bSelectorOpen;

	protected ref CGQC_GU_Region m_Fiche = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Selector = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Body = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_MemberList = new CGQC_GU_Region();
	protected ref CGQC_GU_DossierTabs m_Tabs;	// Qualifications, Séances / FTX
	protected ref CGQC_GU_ProgressTabs m_Progress;	// Progression, Instructeur
	protected TextWidget m_wMessage;
	protected ref CGQC_GU_FieldWatch m_SearchWatch;

	// Edit fields of the open forms
	protected CGQC_GU_DateField m_BirthDate;
	protected EditBoxWidget m_wBirthPlace;
	protected EditBoxWidget m_wBloodType;
	protected MultilineEditBoxWidget m_wBio;
	protected CGQC_GU_DateField m_RecordDate;
	protected EditBoxWidget m_wRecordNote;
	protected EditBoxWidget m_wDisplayName;
	protected EditBoxWidget m_wLastName;
	protected EditBoxWidget m_wBohemiaUid;
	protected EditBoxWidget m_wSearch;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_DossierScreen(string uid = "", CGQC_GU_ERailTab tab = CGQC_GU_ERailTab.REGISTRY)
	{
		m_sUid = uid;
		m_eTab = tab;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildPaper()
	{
		SetHeader("DOSSIER DU PERSONNEL", "");

		Widget row = CGQC_GU_W.HBox(m_wContent);
		if (!row)
			return;

		LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);

		// Left: the fiche
		Widget ficheBox = CGQC_GU_W.Sized(row, FICHE_WIDTH);
		if (ficheBox)
		{
			LayoutSlot.SetVerticalAlign(ficheBox.GetParent(), LayoutVerticalAlign.Top);
			m_Fiche.m_wRoot = CGQC_GU_W.Panel(ficheBox, true);
		}

		// Right: selector, message line, tab body
		Widget right = CGQC_GU_W.VBox(row);
		if (right)
		{
			LayoutSlot.SetSizeMode(right, LayoutSizeMode.Fill);
			LayoutSlot.SetPadding(right, 32, 0, 0, 0);
			LayoutSlot.SetVerticalAlign(right, LayoutVerticalAlign.Stretch);
			m_Selector.m_wRoot = CGQC_GU_W.VBox(right);
			if (m_Selector.m_wRoot)
				LayoutSlot.SetHorizontalAlign(m_Selector.m_wRoot, LayoutHorizontalAlign.Stretch);

			m_wMessage = CGQC_GU_W.Text(right, "", CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.Red());
			if (m_wMessage)
				LayoutSlot.SetPadding(m_wMessage, 0, 6, 0, 6);

			m_Body.m_wRoot = CGQC_GU_W.VBox(right);
			if (m_Body.m_wRoot)
			{
				LayoutSlot.SetSizeMode(m_Body.m_wRoot, LayoutSizeMode.Fill);
				LayoutSlot.SetHorizontalAlign(m_Body.m_wRoot, LayoutHorizontalAlign.Stretch);
			}
		}

		m_Tabs = new CGQC_GU_DossierTabs(this, m_Body);	// Qualifications, Séances / FTX
		m_Progress = new CGQC_GU_ProgressTabs(this, m_Body);
		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		RequestDossier();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		if (m_wSearch && m_SearchWatch)
			m_wSearch.RemoveHandler(m_SearchWatch);

		m_Fiche.Reset();
		m_MemberList.Reset();
		m_Selector.Reset();
		m_Body.Reset();
	}

	//------------------------------------------------------------------------------------------------
	override protected CGQC_GU_ERailTab GetRailTab()
	{
		return m_eTab;
	}

	//------------------------------------------------------------------------------------------------
	//! The Dossier tabs switch in place, on the member being shown.
	override void OnRailSelected(CGQC_GU_ERailTab tab)
	{
		if (tab != CGQC_GU_ERailTab.REGISTRY && tab != CGQC_GU_ERailTab.GRADE && tab != CGQC_GU_ERailTab.QUALS
			&& tab != CGQC_GU_ERailTab.SESSIONS
			&& tab != CGQC_GU_ERailTab.PROGRESSION && tab != CGQC_GU_ERailTab.INSTRUCTOR)
		{
			super.OnRailSelected(tab);
			return;
		}

		m_eTab = tab;
		m_bSelectorOpen = false;
		SelectRailTab();
		BuildSelector();
		BuildBody();
	}

	//------------------------------------------------------------------------------------------------
	// Requests
	//------------------------------------------------------------------------------------------------
	protected void RequestDossier()
	{
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		if (!m_sUid.IsEmpty())
			p.Set("uid", m_sUid);

		m_iDossierReq = CGQC_GU_Client.RequestWith(CGQC_GU_Requests.DOSSIER, p);
	}

	//------------------------------------------------------------------------------------------------
	void Write(string action, CGQC_GU_Payload p)
	{
		SetMessage("", false);
		m_iWriteReq = CGQC_GU_Client.RequestWith(action, p);
	}

	//------------------------------------------------------------------------------------------------
	CGQC_GU_Payload ForThisMember()
	{
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		p.Set("uid", m_Dto.uid);
		return p;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot)
			return;

		if (requestId == m_iDossierReq)
		{
			if (!ok)
			{
				SetHeader("DOSSIER DU PERSONNEL", "Dossier introuvable");
				return;
			}

			CGQC_GU_DossierDto dto = new CGQC_GU_DossierDto();
			JsonLoadContext ctx = new JsonLoadContext();
			if (!ctx.LoadFromString(payload) || !ctx.ReadValue("d", dto) || !dto)
				return;

			m_Dto = dto;
			m_sUid = dto.uid;
			SetHeader("DOSSIER DU PERSONNEL", dto.displayName);
			BuildFiche();
			BuildSelector();
			BuildBody();
			return;
		}

		if (requestId == m_iListReq)
		{
			if (!ok)
				return;

			JsonLoadContext listCtx = new JsonLoadContext();
			array<ref CGQC_GU_MemberSummary> members = new array<ref CGQC_GU_MemberSummary>();
			if (!listCtx.LoadFromString(payload) || !listCtx.ReadValue("members", members))
				return;

			m_aMembers = members;
			BuildSelector();
			return;
		}

		if (requestId == m_iWriteReq)
		{
			if (!ok)
			{
				SetMessage(ErrorText(code), false);
				return;
			}

			m_bEditingPersonal = false;
			SetMessage("Enregistré.", true);
			if (m_aMembers)
				m_iListReq = CGQC_GU_Client.Request(CGQC_GU_Requests.MEMBER_LIST);

			RequestDossier();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SetMessage(string text, bool good)
	{
		if (!m_wMessage)
			return;

		m_wMessage.SetText(text);
		if (good)
			m_wMessage.SetColor(CGQC_GU_Style.Online());
		else
			m_wMessage.SetColor(CGQC_GU_Style.Red());
	}

	//------------------------------------------------------------------------------------------------
	static string ErrorText(string code)
	{
		switch (code)
		{
			case "forbidden":		return "Refusé : droits insuffisants.";
			case "not_found":		return "Dossier introuvable.";
			case "bad_date":		return "Date invalide (AAAA-MM-JJ).";
			case "too_long":		return "Texte trop long.";
			case "bad_name":		return "Nom invalide.";
			case "bad_note":		return "Entrée vide ou trop longue.";
			case "bad_type":		return "Type d'entrée invalide.";
			case "bad_rank":		return "Grade inconnu.";
			case "bad_group":		return "Groupe d'accès inconnu.";
			case "self_lockout":	return "Vous ne pouvez pas retirer votre propre accès Commandement.";
			case "bad_uid":			return "UID Bohemia invalide.";
			case "uid_taken":		return "Ce profil Bohemia a déjà son propre dossier.";
			case "already_linked":	return "Ce dossier est déjà lié.";
			case "same_member":		return "Un dossier ne peut pas être fusionné avec lui-même.";
			case "source_not_linked":	return "Le dossier à fusionner n'a pas de profil Bohemia.";
			case "bad_status":		return "Statut inconnu.";
			case "bad_phase":		return "Phase inconnue.";
			case "bad_title":		return "Titre vide ou trop long.";
			case "bad_time":		return "Heure invalide (HH:MM).";
			case "bad_qual":		return "Qualification inconnue.";
			case "bad_result":		return "Résultat invalide.";
			case "self_evaluation":	return "Vous ne pouvez pas vous évaluer vous-même.";
			case "already_passed":	return "Qualification déjà obtenue.";
			case "bad_instructor":	return "Instructeur chef invalide.";
			case "instructor_not_authorized":	return "L'instructeur chef n'est pas autorisé à enseigner cette qualification.";
			case "bad_exercise":	return "Exercice invalide pour cette séance.";
			case "too_many":		return "Trop de membres dans la séance.";
			case "bad_member":		return "Membre inconnu.";
			case "already_done":	return "Séance déjà terminée.";
			case "no_attendance":	return "Aucun membre présent.";
			case "write_failed":	return "Échec de l'écriture sur le serveur.";
			case "not_ready":		return "Serveur pas encore prêt.";
		}

		return "Erreur : " + code;
	}

	//------------------------------------------------------------------------------------------------
	// Actions
	//------------------------------------------------------------------------------------------------
	CGQC_GU_Button Btn(CGQC_GU_Region region, Widget parent, string text, string kind, string value = "", bool confirm = false, CGQC_GU_EButtonKind look = CGQC_GU_EButtonKind.PLAIN, bool compact = true)
	{
		CGQC_GU_Button b = CGQC_GU_Button.Create(parent, text, look, confirm);
		if (!b)
			return null;

		region.m_aButtons.Insert(b);
		if (compact)
			b.SetCompact();

		if (!kind.IsEmpty())
		{
			CGQC_GU_DossierAction a = new CGQC_GU_DossierAction(this, kind, value);
			region.m_aKeep.Insert(a);
			b.GetOnActivated().Insert(a.Fire);
		}

		return b;
	}

	//------------------------------------------------------------------------------------------------
	void OnAction(string kind, string value)
	{
		if (!m_Dto)
			return;

		switch (kind)
		{
			case "personal.toggle":
				m_bPersonalOpen = !m_bPersonalOpen;
				if (!m_bPersonalOpen)
					m_bEditingPersonal = false;
				BuildFiche();
				break;

			case "personal.edit":
				m_bEditingPersonal = true;
				BuildFiche();
				break;

			case "personal.cancel":
				m_bEditingPersonal = false;
				BuildFiche();
				break;

			case "personal.save":
				SavePersonal();
				break;

			case "selector.toggle":
				m_bSelectorOpen = !m_bSelectorOpen;
				m_iMemberPage = 0;
				if (m_bSelectorOpen && !m_aMembers)
					m_iListReq = CGQC_GU_Client.Request(CGQC_GU_Requests.MEMBER_LIST);
				BuildSelector();
				BuildBody();
				break;

			case "selector.page":
				m_iMemberPage = value.ToInt();
				BuildSelector();
				break;

			case "selector.pick":
				m_bSelectorOpen = false;
				m_bPersonalOpen = false;
				m_bEditingPersonal = false;
				m_iRecordPage = 0;
				m_Tabs.Reset();
				m_Progress.Reset();
				m_sUid = value;
				SetMessage("", false);
				RequestDossier();
				break;

			case "record.page":
				m_iRecordPage = value.ToInt();
				BuildBody();
				break;

			case "record.type":
				m_iManualType = (m_iManualType + 1) % m_aManualTypes.Count();
				BuildBody();
				break;

			case "record.add":
				AddRecord();
				break;

			case "rank.set":
			{
				CGQC_GU_Payload rp = ForThisMember();
				rp.Set("rank", value);
				Write(CGQC_GU_Requests.RANK_SET, rp);
				break;
			}

			case "access.save":
				SaveAccess();
				break;

			case "name.save":
				SaveName(false);
				break;

			case "name.profile":
				SaveName(true);
				break;

			case "profile.link":
			{
				if (!m_wBohemiaUid)
					break;

				CGQC_GU_Payload lp = ForThisMember();
				lp.Set("bohemiaUid", m_wBohemiaUid.GetText().Trim());
				Write(CGQC_GU_Requests.MEMBER_LINK, lp);
				break;
			}

			case "badge.progress":
				m_Progress.Choose(value);
				OnRailSelected(CGQC_GU_ERailTab.PROGRESSION);
				break;

			default:
				if (m_Tabs.OnAction(kind, value) || m_Progress.OnAction(kind, value))
					BuildBody();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SavePersonal()
	{
		CGQC_GU_Payload p = ForThisMember();
		if (m_BirthDate) p.Set("birthDate", m_BirthDate.GetValue());
		if (m_wBirthPlace) p.Set("birthPlace", m_wBirthPlace.GetText());
		if (m_wBloodType) p.Set("bloodType", m_wBloodType.GetText());
		if (m_wBio) p.Set("bio", m_wBio.GetText());
		Write(CGQC_GU_Requests.PERSONAL_SET, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddRecord()
	{
		CGQC_GU_Payload p = ForThisMember();
		p.Set("type", m_aManualTypes[m_iManualType]);
		if (m_RecordDate) p.Set("on", m_RecordDate.GetValue());
		if (m_wRecordNote) p.Set("note", m_wRecordNote.GetText());
		m_iRecordPage = 0;
		Write(CGQC_GU_Requests.RECORD_ADD, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void SaveAccess()
	{
		array<string> groups = new array<string>();
		foreach (CGQC_GU_Check c : m_Body.m_aChecks)
		{
			if (c.IsChecked())
				groups.Insert(c.GetId());
		}

		CGQC_GU_Payload p = ForThisMember();
		p.Set("groups", CGQC_GU_Str.Join(groups, ","));
		Write(CGQC_GU_Requests.ACCESS_SET, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void SaveName(bool useProfile)
	{
		CGQC_GU_Payload p = ForThisMember();
		if (useProfile)
			p.Set("useProfileName", "1");
		else if (m_wDisplayName)
			p.Set("displayName", m_wDisplayName.GetText());

		if (m_wLastName && !m_wLastName.GetText().Trim().IsEmpty())
			p.Set("lastName", m_wLastName.GetText());

		Write(CGQC_GU_Requests.NAME_SET, p);
	}

	//------------------------------------------------------------------------------------------------
	// Fiche
	//------------------------------------------------------------------------------------------------
	protected void BuildFiche()
	{
		m_Fiche.Reset();
		m_BirthDate = null;
		m_wBirthPlace = null;
		m_wBloodType = null;
		m_wBio = null;

		Widget f = m_Fiche.m_wRoot;
		if (!f || !m_Dto)
			return;

		// Insignia beside name and grade
		Widget top = CGQC_GU_W.HBox(f);
		if (top)
		{
			CGQC_GU_Insignia.Draw(top, m_Dto.insignia);
			Widget who = CGQC_GU_W.VBox(top);
			if (who)
			{
				LayoutSlot.SetPadding(who, 18, 0, 0, 0);
				LayoutSlot.SetVerticalAlign(who, LayoutVerticalAlign.Center);
				CGQC_GU_W.Text(who, m_Dto.rankAbbr + " " + m_Dto.displayName, CGQC_GU_Style.FONT_TITLE, 30, CGQC_GU_Style.Ink());
				CGQC_GU_W.Text(who, m_Dto.rankLabel, CGQC_GU_Style.FONT_LABEL, 19, CGQC_GU_Style.InkSoft());
			}
		}

		CGQC_GU_W.Rule(f, 14, 4);

		Pair(f, "ACCÈS", CGQC_GU_Str.Upper(m_Dto.access));

		CGQC_GU_W.Caption(f, "PRÉSENCE");
		Widget presence = CGQC_GU_W.HBox(f);
		if (presence)
		{
			Color dot = CGQC_GU_Style.Offline();
			if (m_Dto.online)
				dot = CGQC_GU_Style.Online();

			CGQC_GU_W.Dot(presence, 10, dot);
			TextWidget pt = CGQC_GU_W.Text(presence, m_Dto.presence, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkSoft());
			if (pt)
				LayoutSlot.SetPadding(pt, 8, 0, 0, 0);
		}

		CGQC_GU_W.Caption(f, "STATUT");
		CGQC_GU_W.Text(f, m_Dto.status, CGQC_GU_Style.FONT_TITLE, 17, CGQC_GU_Style.StatusColor(CGQC_GU_Str.Lower(m_Dto.status)));

		CGQC_GU_W.Rule(f, 14, 8);

		// Collapsible "Renseignements et états de service" (closed by default)
		string head = "RENSEIGNEMENTS ET ÉTATS DE SERVICE  +";
		if (m_bPersonalOpen)
			head = "RENSEIGNEMENTS ET ÉTATS DE SERVICE  −";

		Btn(m_Fiche, f, head, "personal.toggle");

		if (m_bPersonalOpen)
		{
			if (m_bEditingPersonal)
				BuildPersonalForm(f);
			else
				BuildPersonalView(f);
		}

		BuildBadgeLine(f);

		CGQC_GU_W.Rule(f, 14, 6);
		TextWidget footer = CGQC_GU_W.Text(f, m_Dto.footer, CGQC_GU_Style.FONT_VALUE, 13, CGQC_GU_Style.InkMuted());
		if (footer)
			LayoutSlot.SetHorizontalAlign(footer, LayoutHorizontalAlign.Center);
	}

	//------------------------------------------------------------------------------------------------
	//! INSIGNES: one blue tag for each qualification obtained.
	protected void BuildBadgeLine(Widget f)
	{
		CGQC_GU_W.Rule(f, 14, 0);
		CGQC_GU_W.Caption(f, "INSIGNES");

		Widget row;
		int count;
		foreach (CGQC_GU_DossierLadderItem item : m_Dto.ladder)
		{
			if (item.state != CGQC_GU_Quals.STATE_OBTAINED)
				continue;

			if (count % BADGE_TAGS_PER_ROW == 0)
			{
				row = CGQC_GU_W.HBox(f);
				if (row)
					LayoutSlot.SetPadding(row, 0, 3, 0, 3);
			}

			Widget tag = CGQC_GU_W.Tag(row, item.code, CGQC_GU_Style.Blue(), 13);
			if (tag)
				LayoutSlot.SetPadding(tag, 0, 0, 8, 0);

			count++;
		}

		if (count == 0)
			CGQC_GU_W.Text(f, "Aucun", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
	}

	//------------------------------------------------------------------------------------------------
	protected int ObtainedCount()
	{
		int count;
		foreach (CGQC_GU_DossierLadderItem item : m_Dto.ladder)
		{
			if (item.state == CGQC_GU_Quals.STATE_OBTAINED)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected void Pair(Widget parent, string label, string value)
	{
		CGQC_GU_W.Caption(parent, label);
		if (value.IsEmpty())
			value = "—";

		TextWidget t = CGQC_GU_W.Text(parent, value, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkSoft(), true);
		if (t)
			LayoutSlot.SetHorizontalAlign(t, LayoutHorizontalAlign.Stretch);
	}

	//------------------------------------------------------------------------------------------------
	//! Label and value on one line, for the dense personal section.
	protected void Line(Widget parent, string label, string value)
	{
		Widget row = CGQC_GU_W.HBox(parent);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 3, 0, 3);
		Widget lbl = CGQC_GU_W.Sized(row, LINE_LABEL_WIDTH);
		CGQC_GU_W.Text(lbl, label, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());

		if (value.IsEmpty())
			value = "—";

		Widget val = CGQC_GU_W.Sized(row, FICHE_WIDTH - 36 - LINE_LABEL_WIDTH);
		TextWidget t = CGQC_GU_W.Text(val, value, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft(), true);
		if (t)
			LayoutSlot.SetHorizontalAlign(t, LayoutHorizontalAlign.Stretch);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildPersonalView(Widget f)
	{
		Widget box = CGQC_GU_W.VBox(f);
		if (!box)
			return;

		LayoutSlot.SetPadding(box, 0, 8, 0, 0);
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Stretch);

		string birth = CGQC_GU_DossierBuilder.DisplayDate(m_Dto.birthDate);
		if (!m_Dto.birthPlace.IsEmpty())
		{
			if (birth.IsEmpty())
				birth = m_Dto.birthPlace;
			else
				birth = birth + ", " + m_Dto.birthPlace;
		}

		Line(box, "NAISSANCE", birth);
		Line(box, "TYPE SANGUIN", m_Dto.bloodType);
		Line(box, "ENRÔLÉ LE", m_Dto.enlistedOn);
		Line(box, "ANCIENNETÉ", m_Dto.seniority);
		Line(box, "OPÉRATIONS", m_Dto.operations.ToString());
		Line(box, "DÉPLOIEMENTS", m_Dto.deployments.ToString());
		Line(box, "DISTINCTIONS", m_Dto.distinctions.ToString());
		Line(box, "QUALIFICATIONS OBTENUES", ObtainedCount().ToString());
		Line(box, "SÉANCES / FTX", m_Dto.sessions.ToString());
		Line(box, "FORMATIONS DISPONIBLES", m_Dto.formationsCount.ToString());
		Line(box, "ÉLÈVES ÉVALUÉS", m_Dto.studentsEvaluated.ToString());

		CGQC_GU_W.Caption(box, "BIO", 8);
		string bio = m_Dto.bio;
		if (bio.IsEmpty())
			bio = "—";

		TextWidget b = CGQC_GU_W.Text(box, bio, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft(), true);
		if (b)
			LayoutSlot.SetHorizontalAlign(b, LayoutHorizontalAlign.Stretch);

		if (m_Dto.isSelf || m_Dto.canEdit)
		{
			CGQC_GU_Button edit = Btn(m_Fiche, box, "MODIFIER MES INFOS", "personal.edit");
			if (edit && !m_Dto.isSelf)
				edit.SetText("MODIFIER LES INFOS");

			if (edit)
				LayoutSlot.SetPadding(edit.GetRootWidget(), 0, 12, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildPersonalForm(Widget f)
	{
		Widget box = CGQC_GU_W.VBox(f);
		if (!box)
			return;

		LayoutSlot.SetPadding(box, 0, 4, 0, 0);
		float w = FICHE_WIDTH - 36;

		CGQC_GU_W.Caption(box, "DATE DE NAISSANCE");
		m_BirthDate = CGQC_GU_DateField.Create(m_Fiche, box, m_Dto.birthDate, true, false, true);

		CGQC_GU_W.Caption(box, "LIEU DE NAISSANCE");
		m_wBirthPlace = CGQC_GU_W.Field(box, m_Dto.birthPlace, w);
		CGQC_GU_W.Caption(box, "TYPE SANGUIN");
		m_wBloodType = CGQC_GU_W.Field(box, m_Dto.bloodType, 120);
		CGQC_GU_W.Caption(box, "BIO");
		m_wBio = CGQC_GU_W.FieldMulti(box, m_Dto.bio, w, 140);

		Widget buttons = CGQC_GU_W.HBox(box);
		if (buttons)
		{
			LayoutSlot.SetPadding(buttons, 0, 12, 0, 0);
			Btn(m_Fiche, buttons, "ENREGISTRER", "personal.save", "", true, CGQC_GU_EButtonKind.PRIMARY);
			CGQC_GU_Button cancel = Btn(m_Fiche, buttons, "ANNULER", "personal.cancel");
			if (cancel)
				LayoutSlot.SetPadding(cancel.GetRootWidget(), 10, 0, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Member selector
	//------------------------------------------------------------------------------------------------
	protected void BuildSelector()
	{
		if (m_wSearch && m_SearchWatch)
			m_wSearch.RemoveHandler(m_SearchWatch);

		string search;
		if (m_wSearch)
			search = m_wSearch.GetText();

		m_wSearch = null;
		m_MemberList.Reset();
		m_MemberList.m_wRoot = null;
		m_Selector.Reset();

		Widget s = m_Selector.m_wRoot;
		if (!s || !m_Dto)
			return;

		Widget row = CGQC_GU_W.HBox(s);
		if (row)
		{
			TextWidget cap = CGQC_GU_W.Text(row, "MEMBRE", CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
			if (cap)
			{
				LayoutSlot.SetVerticalAlign(cap, LayoutVerticalAlign.Center);
				LayoutSlot.SetPadding(cap, 0, 0, 12, 0);
			}

			string label = m_Dto.rankAbbr + " " + m_Dto.displayName;
			if (m_bSelectorOpen)
				label = label + "  −";
			else
				label = label + "  +";

			Btn(m_Selector, row, label, "selector.toggle");
		}

		if (!m_bSelectorOpen)
			return;

		Widget panel = CGQC_GU_W.Panel(s);
		if (!panel)
			return;

		LayoutSlot.SetPadding(panel.GetParent(), 0, 10, 0, 0);
		LayoutSlot.SetHorizontalAlign(panel.GetParent(), LayoutHorizontalAlign.Stretch);

		CGQC_GU_W.Caption(panel, "RECHERCHE", 0);
		m_wSearch = CGQC_GU_W.Field(panel, search, 360);
		if (m_wSearch)
		{
			if (!m_SearchWatch)
			{
				m_SearchWatch = new CGQC_GU_FieldWatch();
				m_SearchWatch.GetOnChanged().Insert(OnSearchChanged);
			}

			m_wSearch.AddHandler(m_SearchWatch);
		}

		Widget list = CGQC_GU_W.VBox(panel);
		if (list)
			LayoutSlot.SetPadding(list, 0, 10, 0, 0);

		m_MemberList.m_wRoot = list;
		FillMemberList();
	}

	//------------------------------------------------------------------------------------------------
	//! Typing in the search box refreshes the list only, so the field keeps its focus.
	protected void OnSearchChanged()
	{
		m_iMemberPage = 0;
		FillMemberList();
	}

	//------------------------------------------------------------------------------------------------
	protected void FillMemberList()
	{
		m_MemberList.Reset();
		Widget list = m_MemberList.m_wRoot;
		if (!list || !m_Dto)
			return;

		string search;
		if (m_wSearch)
			search = m_wSearch.GetText();

		if (!m_aMembers)
		{
			CGQC_GU_W.Text(list, "Chargement…", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
			return;
		}

		string needle = CGQC_GU_Str.Lower(search.Trim());
		array<CGQC_GU_MemberSummary> shown = new array<CGQC_GU_MemberSummary>();
		foreach (CGQC_GU_MemberSummary m : m_aMembers)
		{
			if (!needle.IsEmpty())
			{
				string hay = CGQC_GU_Str.Lower(m.rankAbbr + " " + m.displayName + " " + m.lastName);
				if (!hay.Contains(needle))
					continue;
			}

			shown.Insert(m);
		}

		int pages = Math.Max(1, (shown.Count() + MEMBER_PAGE - 1) / MEMBER_PAGE);
		m_iMemberPage = Math.ClampInt(m_iMemberPage, 0, pages - 1);
		int from = m_iMemberPage * MEMBER_PAGE;
		int to = Math.Min(from + MEMBER_PAGE, shown.Count());

		if (shown.IsEmpty())
			CGQC_GU_W.Text(list, "Aucun membre.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());

		for (int i = from; i < to; i++)
		{
			CGQC_GU_MemberSummary m = shown[i];
			Widget row = CGQC_GU_W.HBox(list);
			if (!row)
				continue;

			LayoutSlot.SetPadding(row, 0, 2, 0, 2);
			Color dot = CGQC_GU_Style.Offline();
			if (m.online)
				dot = CGQC_GU_Style.Online();

			ImageWidget d = CGQC_GU_W.Dot(row, 10, dot);
			if (d)
				LayoutSlot.SetPadding(d, 0, 0, 10, 0);

			CGQC_GU_Button b = Btn(m_MemberList, row, m.rankAbbr + " " + m.displayName, "selector.pick", m.uid);
			if (b && m.uid == m_Dto.uid)
				b.SetSelected(true);
		}

		Pager(m_MemberList, list, m_iMemberPage, pages, "selector.page");
	}

	//------------------------------------------------------------------------------------------------
	void Pager(CGQC_GU_Region region, Widget parent, int page, int pages, string kind)
	{
		if (pages <= 1)
			return;

		Widget row = CGQC_GU_W.HBox(parent);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 10, 0, 0);
		CGQC_GU_Button prev = Btn(region, row, "PRÉCÉDENTE", kind, (page - 1).ToString());
		if (prev)
			prev.SetEnabled(page > 0);

		TextWidget t = CGQC_GU_W.Text(row, string.Format("Page %1 / %2", page + 1, pages), CGQC_GU_Style.FONT_LABEL, 16, CGQC_GU_Style.InkMuted());
		if (t)
		{
			LayoutSlot.SetVerticalAlign(t, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(t, 14, 0, 14, 0);
		}

		CGQC_GU_Button next = Btn(region, row, "SUIVANTE", kind, (page + 1).ToString());
		if (next)
			next.SetEnabled(page < pages - 1);
	}

	//------------------------------------------------------------------------------------------------
	// Tabs
	//------------------------------------------------------------------------------------------------
	protected void BuildBody()
	{
		m_Body.Reset();
		m_RecordDate = null;
		m_wRecordNote = null;
		m_wDisplayName = null;
		m_wLastName = null;
		m_wBohemiaUid = null;

		if (!m_Body.m_wRoot || !m_Dto || m_bSelectorOpen)
			return;

		switch (m_eTab)
		{
			case CGQC_GU_ERailTab.GRADE:
				BuildGrade(m_Body.m_wRoot);
				break;

			case CGQC_GU_ERailTab.QUALS:
				m_Tabs.BuildQuals(m_Body.m_wRoot, m_Dto);
				break;

			case CGQC_GU_ERailTab.SESSIONS:
				m_Tabs.BuildSessions(m_Body.m_wRoot, m_Dto);
				break;

			case CGQC_GU_ERailTab.PROGRESSION:
				m_Progress.BuildProgress(m_Body.m_wRoot, m_Dto);
				break;

			case CGQC_GU_ERailTab.INSTRUCTOR:
				m_Progress.BuildInstructor(m_Body.m_wRoot, m_Dto);
				break;

			default:
				BuildRegistre(m_Body.m_wRoot);
		}
	}

	//------------------------------------------------------------------------------------------------
	Widget Section(Widget parent, string title)
	{
		TextWidget t = CGQC_GU_W.Text(parent, title, CGQC_GU_Style.FONT_TITLE, 22, CGQC_GU_Style.Ink());
		if (t)
			LayoutSlot.SetPadding(t, 0, 6, 0, 2);

		CGQC_GU_W.Rule(parent, 2, 8);
		return t;
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildRegistre(Widget body)
	{
		Section(body, "REGISTRE");

		int count = m_Dto.record.Count();
		int pages = Math.Max(1, (count + RECORD_PAGE - 1) / RECORD_PAGE);
		m_iRecordPage = Math.ClampInt(m_iRecordPage, 0, pages - 1);
		int from = m_iRecordPage * RECORD_PAGE;
		int to = Math.Min(from + RECORD_PAGE, count);

		// Column headings
		RecordRow(body, "DATE", "TYPE", "INSCRIPTION", "SIGNÉ PAR", true);

		if (count == 0)
			CGQC_GU_W.Text(body, "Aucune inscription.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());

		for (int i = from; i < to; i++)
		{
			CGQC_GU_DossierRecordLine line = m_Dto.record[i];
			RecordRow(body, line.date, line.type, line.text, line.by, false);
		}

		Pager(m_Body, body, m_iRecordPage, pages, "record.page");

		if (!m_Dto.canEdit)
			return;

		// Manual entry (staff)
		Widget form = CGQC_GU_W.Panel(body);
		if (!form)
			return;

		LayoutSlot.SetPadding(form.GetParent(), 0, 18, 0, 0);
		LayoutSlot.SetHorizontalAlign(form.GetParent(), LayoutHorizontalAlign.Stretch);
		TextWidget title = CGQC_GU_W.Text(form, "NOUVELLE INSCRIPTION", CGQC_GU_Style.FONT_TITLE, 17, CGQC_GU_Style.Ink());

		Widget row = CGQC_GU_W.HBox(form);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 6, 0, 0);

		Widget typeCol = CGQC_GU_W.VBox(row);
		CGQC_GU_W.Caption(typeCol, "TYPE");
		Btn(m_Body, typeCol, CGQC_GU_DossierBuilder.RecordTypeLabel(m_aManualTypes[m_iManualType]), "record.type");

		Widget dateCol = CGQC_GU_W.VBox(row);
		if (dateCol)
			LayoutSlot.SetPadding(dateCol, 16, 0, 0, 0);

		CGQC_GU_W.Caption(dateCol, "DATE");
		m_RecordDate = CGQC_GU_DateField.Create(m_Body, dateCol, "", true);


		Widget noteCol = CGQC_GU_W.VBox(row);
		if (noteCol)
			LayoutSlot.SetPadding(noteCol, 16, 0, 0, 0);

		CGQC_GU_W.Caption(noteCol, "INSCRIPTION");
		m_wRecordNote = CGQC_GU_W.Field(noteCol, "", 520);

		CGQC_GU_Button add = Btn(m_Body, form, "AJOUTER AU REGISTRE", "record.add", "", true, CGQC_GU_EButtonKind.PRIMARY);
		if (add)
			LayoutSlot.SetPadding(add.GetRootWidget(), 0, 12, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void RecordRow(Widget parent, string date, string type, string text, string by, bool heading)
	{
		Widget row = CGQC_GU_W.HBox(parent);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 4, 0, 4);

		ResourceName font = CGQC_GU_Style.FONT_VALUE;
		int size = 16;
		Color color = CGQC_GU_Style.InkSoft();
		if (heading)
		{
			font = CGQC_GU_Style.FONT_TITLE;
			size = 14;
			color = CGQC_GU_Style.InkMuted();
		}

		Cell(row, date, 120, font, size, color);
		if (heading)
			Cell(row, type, 150, font, size, color);
		else
			Cell(row, type, 150, CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Ink());

		Cell(row, text, 560, font, size, color);
		Cell(row, by, 200, font, size, color);

		if (!heading)
		{
			Widget sep = CGQC_GU_W.Rule(parent, 0, 0);
			if (sep)
				sep.SetOpacity(0.25);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Cell(Widget row, string text, float width, ResourceName font, int size, Color color)
	{
		Widget box = CGQC_GU_W.Sized(row, width);
		if (!box)
			return;

		LayoutSlot.SetPadding(box.GetParent(), 0, 0, 12, 0);
		TextWidget t = CGQC_GU_W.Text(box, text, font, size, color, true);
		if (t)
			LayoutSlot.SetHorizontalAlign(t, LayoutHorizontalAlign.Stretch);
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildGrade(Widget body)
	{
		Widget cols = CGQC_GU_W.HBox(body);
		if (!cols)
			return;

		LayoutSlot.SetHorizontalAlign(cols, LayoutHorizontalAlign.Stretch);

		// Left column: grade, roles, next promotion
		Widget left = CGQC_GU_W.Sized(cols, 640);
		if (left)
		{
			LayoutSlot.SetVerticalAlign(left.GetParent(), LayoutVerticalAlign.Top);
			BuildRanks(left);
		}

		// Right column: access groups, display name, Bohemia profile
		Widget right = CGQC_GU_W.Sized(cols, 460);
		if (right)
		{
			LayoutSlot.SetPadding(right.GetParent(), 40, 0, 0, 0);
			LayoutSlot.SetVerticalAlign(right.GetParent(), LayoutVerticalAlign.Top);
			BuildAccess(right);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildRanks(Widget col)
	{
		Section(col, "GRADE");

		Widget row;
		int inRow;
		foreach (CGQC_GU_DossierRank r : m_Dto.ranks)
		{
			if (!row || inRow == 5)
			{
				row = CGQC_GU_W.HBox(col);
				inRow = 0;
				if (row)
					LayoutSlot.SetPadding(row, 0, 0, 0, 8);
			}

			if (!row)
				break;

			Widget cell = CGQC_GU_W.Sized(row, 120);
			if (!cell)
				continue;

			Widget ins = CGQC_GU_Insignia.Draw(cell, r.insignia, 0.42);
			if (ins)
			{
				LayoutSlot.SetHorizontalAlign(ins, LayoutHorizontalAlign.Center);
				LayoutSlot.SetPadding(ins, 0, 0, 0, 4);
			}

			// Click a rank: it turns red; click it again to apply. Only one button is armed at a time.
			string kind;
			if (m_Dto.canEdit && !r.current)
				kind = "rank.set";

			// The current rank, or any rank for a viewer without members.edit, is never armed (owner, 2b test).
			CGQC_GU_Button b = Btn(m_Body, cell, r.abbr, kind, r.id, !kind.IsEmpty());
			if (b)
			{
				LayoutSlot.SetHorizontalAlign(b.GetRootWidget(), LayoutHorizontalAlign.Center);
				b.SetConfirmText("CONFIRMER");
				b.SetSelected(r.current);
				if (!m_Dto.canEdit && !r.current)
					b.SetEnabled(false);
			}

			inRow++;
		}

		CGQC_GU_W.Caption(col, "RÔLES DÉBLOQUÉS", 14);
		string roles = CGQC_GU_Str.Join(m_Dto.roles, " · ");
		if (roles.IsEmpty())
			roles = "—";

		TextWidget rt = CGQC_GU_W.Text(col, roles, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft(), true);
		if (rt)
			LayoutSlot.SetHorizontalAlign(rt, LayoutHorizontalAlign.Stretch);

		if (m_Dto.nextRank.IsEmpty())
			return;

		string next = "PROCHAIN GRADE : " + CGQC_GU_Str.Upper(m_Dto.nextRank);
		if (m_Dto.nextAutomatic)
			next = next + " · AUTOMATIQUE";

		CGQC_GU_W.Caption(col, next, 16);
		foreach (CGQC_GU_CriterionLine c : m_Dto.criteria)
		{
			Widget line = CGQC_GU_W.HBox(col);
			if (!line)
				continue;

			LayoutSlot.SetPadding(line, 0, 2, 0, 2);
			Color dot = CGQC_GU_Style.Offline();
			if (c.met)
				dot = CGQC_GU_Style.Online();

			CGQC_GU_W.Dot(line, 9, dot);
			TextWidget t = CGQC_GU_W.Text(line, c.text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
			if (t)
				LayoutSlot.SetPadding(t, 10, 0, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BuildAccess(Widget col)
	{
		Section(col, "ACCÈS");
		foreach (CGQC_GU_DossierGroup g : m_Dto.groups)
		{
			CGQC_GU_Check c = CGQC_GU_Check.Create(col, g.id, g.label, g.held);
			if (!c)
				continue;

			c.SetEnabled(m_Dto.canAccess);
			m_Body.m_aChecks.Insert(c);
		}

		if (m_Dto.canAccess)
		{
			CGQC_GU_Button save = Btn(m_Body, col, "ENREGISTRER LES ACCÈS", "access.save", "", true, CGQC_GU_EButtonKind.PRIMARY);
			if (save)
				LayoutSlot.SetPadding(save.GetRootWidget(), 0, 8, 0, 0);
		}

		// Display name
		Section(col, "NOM");
		if (m_Dto.canEdit)
		{
			CGQC_GU_W.Caption(col, "NOM AFFICHÉ", 0);
			m_wDisplayName = CGQC_GU_W.Field(col, m_Dto.displayName, 320);
			CGQC_GU_W.Caption(col, "NOM DE FAMILLE");
			m_wLastName = CGQC_GU_W.Field(col, m_Dto.lastName, 320);

			Widget buttons = CGQC_GU_W.HBox(col);
			if (buttons)
			{
				LayoutSlot.SetPadding(buttons, 0, 10, 0, 0);
				Btn(m_Body, buttons, "ENREGISTRER", "name.save", "", true, CGQC_GU_EButtonKind.PRIMARY);
				if (!m_Dto.profileName.IsEmpty())
				{
					CGQC_GU_Button prof = Btn(m_Body, buttons, "UTILISER LE NOM DU PROFIL", "name.profile", "", true);
					if (prof)
						LayoutSlot.SetPadding(prof.GetRootWidget(), 10, 0, 0, 0);
				}
			}
		}
		else
		{
			Pair(col, "NOM AFFICHÉ", m_Dto.displayName);
			Pair(col, "NOM DE FAMILLE", m_Dto.lastName);
		}

		// Bohemia profile
		Section(col, "PROFIL BOHEMIA");
		Widget tagRow = CGQC_GU_W.HBox(col);
		if (tagRow)
		{
			string tag = "NON LIÉ";
			Color tagColor = CGQC_GU_Style.Red();
			if (m_Dto.uidLinked)
			{
				tag = "LIÉ";
				tagColor = CGQC_GU_Style.Online();
			}

			CGQC_GU_W.Dot(tagRow, 10, tagColor);
			TextWidget tt = CGQC_GU_W.Text(tagRow, tag, CGQC_GU_Style.FONT_TITLE, 16, tagColor);
			if (tt)
				LayoutSlot.SetPadding(tt, 8, 0, 16, 0);

			if (!m_Dto.profileName.IsEmpty())
				CGQC_GU_W.Text(tagRow, m_Dto.profileName, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		}

		if (m_Dto.uidLinked)
		{
			Pair(col, "UID", m_Dto.uid);
			return;
		}

		if (!m_Dto.canEdit)
			return;

		CGQC_GU_W.Caption(col, "UID");
		m_wBohemiaUid = CGQC_GU_W.Field(col, "", 440);
		CGQC_GU_Button link = Btn(m_Body, col, "LIER LE PROFIL", "profile.link", "", true, CGQC_GU_EButtonKind.PRIMARY);
		if (link)
			LayoutSlot.SetPadding(link.GetRootWidget(), 0, 10, 0, 0);
	}
}
