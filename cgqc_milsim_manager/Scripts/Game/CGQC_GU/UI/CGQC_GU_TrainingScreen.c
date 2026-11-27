// Gestion d'unité - Événements › Entraînement (paper), Phase 3f, after the approved mockup (Entrainement).
// Left: the session's fiche. Right: type, qualification, exercises covered, attendance and the writes.
// The form is edited on the client (the server's reply is the draft) and nothing is written without a double
// press on a write button; the server checks every write again.

//! Binds a button to a named action of the screen, with a value (code, uid, row number...).
class CGQC_GU_TrainingAction : CGQC_GU_Action
{
	protected CGQC_GU_TrainingScreen m_Screen;
	protected string m_sKind;
	protected string m_sValue;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_TrainingAction(CGQC_GU_TrainingScreen screen, string kind, string value)
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

//! One attendance line's widgets: what is read back into the draft, and the result tags.
class CGQC_GU_TrainingLine : Managed
{
	CGQC_GU_TrainingRowDto m_Row;
	CGQC_GU_Check m_Present;
	ref array<CGQC_GU_ClickRow> m_aPositions = new array<CGQC_GU_ClickRow>();	// INSTRUCTEUR, ÉLÈVE
	ref array<Widget> m_aGradeCells = new array<Widget>();	// hidden when the member is not graded
	ref array<CGQC_GU_Check> m_aExercises = new array<CGQC_GU_Check>();	// id = exercise code
	ref array<CGQC_GU_ClickRow> m_aTags = new array<CGQC_GU_ClickRow>();	// RÉUSSI, ÉCHOUÉ, —
}

class CGQC_GU_TrainingScreen : CGQC_GU_PaperScreen
{
	static const float FICHE_WIDTH = 560;
	static const float FICHE_INNER = 512;		// fiche width less its padding
	static const float LABEL_WIDTH = 170;		// fiche labels (190 with the cell padding)
	static const float FIELD_LABEL_WIDTH = 200;	// « Qualification liée », « Exercices couverts »
	static const float TABLE_WIDTH = 1016;
	static const float CHECK_WIDTH = 36;
	static const float POSITION_WIDTH = 165;
	static const float RESULT_WIDTH = 200;
	static const float FIXED_WIDTH = 461;		// ✓, position and result columns with their padding
	static const float EXERCISE_WIDTH = 90;
	static const int MEMBER_PAGE = 12;
	static const int MAX_ROWS = 60;
	static const int QUALS_PER_ROW = 2;
	static const int EXERCISE_TAGS_PER_ROW = 2;

	protected ref CGQC_GU_TrainingDto m_Dto;	// the session on screen, with the edits not yet written
	protected ref array<ref CGQC_GU_ExerciseDto> m_aExercises;
	protected ref array<ref CGQC_GU_MemberSummary> m_aMembers;

	protected int m_iReadReq;
	protected int m_iExerciseReq;
	protected int m_iListReq;
	protected int m_iWriteReq;
	protected string m_sDone;		// message shown when the pending write succeeds

	protected bool m_bQualOpen;
	protected bool m_bExerciseOpen;
	protected bool m_bInstructorOpen;
	protected bool m_bMemberOpen;
	protected bool m_bAddOnline;	// waiting for the member list to add everyone online
	protected int m_iMemberPage;
	protected string m_sSearch;		// member picker, kept while the page is redrawn
	protected string m_sMessage;
	protected bool m_bMessageGood;

	protected ref CGQC_GU_Region m_Head = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Fiche = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_Right = new CGQC_GU_Region();
	protected ref CGQC_GU_Region m_PickList = new CGQC_GU_Region();
	protected ref array<ref CGQC_GU_TrainingLine> m_aLines = new array<ref CGQC_GU_TrainingLine>();

	protected EditBoxWidget m_wName;
	protected CGQC_GU_DateField m_Date;
	protected EditBoxWidget m_wTime;
	protected EditBoxWidget m_wPlace;
	protected EditBoxWidget m_wSearch;
	protected TextWidget m_wPresent;
	protected TextWidget m_wMessage;
	protected ref CGQC_GU_FieldWatch m_SearchWatch;

	//------------------------------------------------------------------------------------------------
	override protected CGQC_GU_ERailTab GetRailTab()
	{
		return CGQC_GU_ERailTab.TRAININGS;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildPaper()
	{
		SetHeader("INSTRUCTION", "ENTRAÎNEMENT");

		// The title keeps the mockup's width; the browsing controls take the rest of the header.
		Widget titles = Find("HeaderTitles");
		if (titles)
		{
			LayoutSlot.SetSizeMode(titles, LayoutSizeMode.Auto);
			LayoutSlot.SetPadding(titles, 0, 0, 140, 0);
		}

		if (m_wHeaderTools)
			LayoutSlot.SetSizeMode(m_wHeaderTools, LayoutSizeMode.Fill);

		m_Head.m_wRoot = m_wHeaderTools;

		Widget row = CGQC_GU_W.HBox(m_wContent);
		if (!row)
			return;

		LayoutSlot.SetSizeMode(row, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);

		Widget ficheBox = CGQC_GU_W.Sized(row, FICHE_WIDTH);
		if (ficheBox)
		{
			LayoutSlot.SetVerticalAlign(ficheBox.GetParent(), LayoutVerticalAlign.Stretch);
			m_Fiche.m_wRoot = ficheBox;
		}

		m_Right.m_wRoot = CGQC_GU_W.VBox(row);
		if (m_Right.m_wRoot)
		{
			LayoutSlot.SetSizeMode(m_Right.m_wRoot, LayoutSizeMode.Fill);
			LayoutSlot.SetPadding(m_Right.m_wRoot, 24, 0, 0, 0);
			LayoutSlot.SetVerticalAlign(m_Right.m_wRoot, LayoutVerticalAlign.Stretch);
		}

		CGQC_GU_Client.GetOnReply().Insert(OnReply);
		m_iExerciseReq = CGQC_GU_Client.Request(CGQC_GU_QualRequests.EXERCISES_CATALOGUE);
		m_iListReq = CGQC_GU_Client.Request(CGQC_GU_Requests.MEMBER_LIST);
		RequestSession(string.Empty);
		Rebuild();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
		ResetPickList();
		m_aLines.Clear();
		m_Head.Reset();
		m_Fiche.Reset();
		m_Right.Reset();
	}

	//------------------------------------------------------------------------------------------------
	// Requests
	//------------------------------------------------------------------------------------------------
	//! id "": the next planned session, else the newest.
	protected void RequestSession(string id)
	{
		CGQC_GU_Payload p = new CGQC_GU_Payload();
		if (!id.IsEmpty())
			p.Set("id", id);

		m_iReadReq = CGQC_GU_Client.RequestWith(CGQC_GU_TrainingRequests.GET, p);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (!m_wRoot)
			return;

		if (requestId == m_iExerciseReq)
		{
			array<ref CGQC_GU_ExerciseDto> exercises = new array<ref CGQC_GU_ExerciseDto>();
			JsonLoadContext exCtx = new JsonLoadContext();
			if (ok && exCtx.LoadFromString(payload) && exCtx.ReadValue("exercises", exercises))
			{
				Capture();
				m_aExercises = exercises;
				Rebuild();
			}

			return;
		}

		if (requestId == m_iListReq)
		{
			array<ref CGQC_GU_MemberSummary> members = new array<ref CGQC_GU_MemberSummary>();
			JsonLoadContext listCtx = new JsonLoadContext();
			if (ok && listCtx.LoadFromString(payload) && listCtx.ReadValue("members", members))
			{
				m_aMembers = members;
				if (m_bAddOnline)
				{
					Capture();
					AddOnline();
					Rebuild();
				}
				else if (m_bMemberOpen)
				{
					FillPickList();
				}
			}
			else if (m_bAddOnline)
			{
				SetMessage(CGQC_GU_DossierScreen.ErrorText(code), false);
			}

			m_bAddOnline = false;
			return;
		}

		if (requestId != m_iReadReq && requestId != m_iWriteReq)
			return;

		if (!ok)
		{
			SetMessage(CGQC_GU_DossierScreen.ErrorText(code), false);
			return;
		}

		CGQC_GU_TrainingDto dto = new CGQC_GU_TrainingDto();
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(payload) || !ctx.ReadValue("t", dto) || !dto)
		{
			SetMessage("Séance illisible.", false);
			return;
		}

		m_sMessage = string.Empty;
		if (requestId == m_iWriteReq)
		{
			m_sMessage = m_sDone;
			m_bMessageGood = true;
		}

		m_Dto = dto;
		m_bAddOnline = false;
		ClosePickers();
		Rebuild();
	}

	//------------------------------------------------------------------------------------------------
	protected void SetMessage(string text, bool good)
	{
		m_sMessage = text;
		m_bMessageGood = good;
		if (!m_wMessage)
			return;

		m_wMessage.SetText(text);
		if (good)
			m_wMessage.SetColor(CGQC_GU_Style.Online());
		else
			m_wMessage.SetColor(CGQC_GU_Style.Red());
	}

	//------------------------------------------------------------------------------------------------
	// Actions
	//------------------------------------------------------------------------------------------------
	void OnAction(string kind, string value)
	{
		if (!m_Dto)
			return;

		switch (kind)
		{
			// Browsing: edits not written are dropped
			case "open":
				SetMessage("", false);
				RequestSession(value);
				return;

			case "new":
				NewSession();
				Rebuild();
				return;

			case "result":
				PickResult(value);
				return;

			case "position":
				PickPosition(value);
				return;

			// A fresh member list, so « en ligne » is current.
			case "member.online":
				SetMessage("", false);
				m_bAddOnline = true;
				m_iListReq = CGQC_GU_Client.Request(CGQC_GU_Requests.MEMBER_LIST);
				return;

			case "save":
				Send(CGQC_GU_TrainingRequests.SAVE, string.Empty, "Séance enregistrée.");
				return;

			case "plan":
				Send(CGQC_GU_TrainingRequests.STATUS, CGQC_GU_Training.STATUS_PLANNED, "Séance planifiée.");
				return;

			case "unplan":
				Send(CGQC_GU_TrainingRequests.STATUS, CGQC_GU_Training.STATUS_DRAFT, "Séance remise en brouillon.");
				return;

			case "delete":
			{
				CGQC_GU_Payload p = new CGQC_GU_Payload();
				p.Set("id", m_Dto.id);
				m_sDone = "Séance supprimée.";
				m_iWriteReq = CGQC_GU_Client.RequestWith(CGQC_GU_TrainingRequests.DELETE, p);
				return;
			}

			case "finish":
				Send(CGQC_GU_TrainingRequests.FINISH, string.Empty, "Séance terminée : les dossiers sont à jour.");
				return;

			case "page":
				m_iMemberPage = value.ToInt();
				FillPickList();
				return;

			default:
				Capture();
				Edit(kind, value);
				Rebuild();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Changes to the draft that redraw the page.
	protected void Edit(string kind, string value)
	{
		m_sMessage = string.Empty;
		switch (kind)
		{
			case "type":
				m_Dto.type = value;
				m_Dto.typeLabel = CGQC_GU_TrainingRequests.TypeLabel(value);
				break;

			case "qual.toggle":
			{
				bool open = !m_bQualOpen;
				ClosePickers();
				m_bQualOpen = open;
				break;
			}

			case "qual.pick":
				PickQual(value);
				ClosePickers();
				break;

			case "ex.toggle":
			{
				bool openEx = !m_bExerciseOpen;
				ClosePickers();
				m_bExerciseOpen = openEx;
				break;
			}

			case "ex.add":
			{
				CGQC_GU_ExerciseDto e = FindExercise(value);
				if (e && FindCovered(value) < 0)
				{
					CGQC_GU_LabelItem item = new CGQC_GU_LabelItem();
					item.id = e.code;
					item.label = e.title;
					m_Dto.exercises.Insert(item);
				}

				break;
			}

			case "ex.remove":
			{
				int at = FindCovered(value);
				if (at >= 0)
					m_Dto.exercises.RemoveOrdered(at);

				foreach (CGQC_GU_TrainingRowDto r : m_Dto.attendance)
				{
					r.exercises.Remove(value);
				}

				break;
			}

			case "inst.toggle":
			{
				bool openInst = !m_bInstructorOpen;
				ClosePickers();
				m_bInstructorOpen = openInst;
				break;
			}

			case "inst.pick":
				PickInstructor(value);
				ClosePickers();
				break;

			case "member.toggle":
			{
				bool openMember = !m_bMemberOpen;
				ClosePickers();
				m_bMemberOpen = openMember;
				m_iMemberPage = 0;
				break;
			}

			case "member.add":
				AddMember(value);
				break;

			case "member.remove":
				RemoveMember(value);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClosePickers()
	{
		m_bQualOpen = false;
		m_bExerciseOpen = false;
		m_bInstructorOpen = false;
		m_bMemberOpen = false;
		m_sSearch = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! A blank session on the screen only; the first write creates it.
	protected void NewSession()
	{
		if (!m_Dto.canCreate)
			return;

		CGQC_GU_TrainingDto d = new CGQC_GU_TrainingDto();
		d.type = "training";
		d.typeLabel = CGQC_GU_TrainingRequests.TypeLabel(d.type);
		d.status = CGQC_GU_Training.STATUS_DRAFT;
		d.canEdit = true;
		d.canCreate = true;
		d.viewer = m_Dto.viewer;
		d.total = m_Dto.total;
		d.qualChoices = m_Dto.qualChoices;
		d.instructorChoices = m_Dto.instructorChoices;
		d.commandUids = m_Dto.commandUids;
		m_Dto = d;
		m_bAddOnline = false;
		PickInstructor(d.viewer);
		ClosePickers();
		m_sMessage = string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Another qualification: the exercises and grades chosen for the former one no longer apply.
	protected void PickQual(string code)
	{
		if (code == m_Dto.qual)
			return;

		m_Dto.qual = code;
		m_Dto.qualTitle = LabelOf(m_Dto.qualChoices, code);

		for (int i = m_Dto.exercises.Count() - 1; i >= 0; i--)
		{
			CGQC_GU_ExerciseDto e = FindExercise(m_Dto.exercises[i].id);
			if (!code.IsEmpty() && (!e || e.qual != code))
				m_Dto.exercises.RemoveOrdered(i);
		}

		foreach (CGQC_GU_TrainingRowDto r : m_Dto.attendance)
		{
			r.exercises.Clear();
			r.result = string.Empty;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void PickInstructor(string uid)
	{
		m_Dto.instructor = uid;
		m_Dto.instructorName = LabelOf(m_Dto.instructorChoices, uid);

		// The instructor chef is on the fiche, not listed twice (Commandement may stay, to be graded).
		if (NoGrade(uid))
			RemoveMember(uid);

		foreach (CGQC_GU_TrainingRowDto r : m_Dto.attendance)
		{
			r.noGrade = NoGrade(r.uid);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The instructor chef is not graded on their own session, unless Commandement.
	protected bool NoGrade(string uid)
	{
		return uid == m_Dto.instructor && !m_Dto.commandUids.Contains(uid);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddMember(string uid)
	{
		if (m_Dto.attendance.Count() >= MAX_ROWS || FindRow(uid) >= 0 || !m_aMembers)
			return;

		foreach (CGQC_GU_MemberSummary m : m_aMembers)
		{
			if (m.uid != uid)
				continue;

			CGQC_GU_TrainingRowDto r = new CGQC_GU_TrainingRowDto();
			r.uid = m.uid;
			r.name = m.rankAbbr + " " + m.displayName;
			r.online = m.online;
			r.present = true;
			r.position = CGQC_GU_TrainingRequests.POSITION_STUDENT;
			r.noGrade = NoGrade(uid);
			m_Dto.attendance.Insert(r);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Everyone online who is not listed yet, except the instructor chef.
	protected void AddOnline()
	{
		if (!m_Dto || !m_Dto.canEdit)
			return;

		array<CGQC_GU_MemberSummary> online = new array<CGQC_GU_MemberSummary>();
		foreach (CGQC_GU_MemberSummary m : m_aMembers)
		{
			if (!m.online || m.uid == m_Dto.instructor || FindRow(m.uid) >= 0)
				continue;

			InsertSorted(online, m);
		}

		int added;
		foreach (CGQC_GU_MemberSummary s : online)
		{
			if (m_Dto.attendance.Count() >= MAX_ROWS)
				break;

			AddMember(s.uid);
			added++;
		}

		m_bMessageGood = added == online.Count();
		if (!m_bMessageGood)
			m_sMessage = string.Format("Séance pleine : %1 membres au maximum.", MAX_ROWS);
		else if (added == 0)
			m_sMessage = "Aucun autre membre en ligne.";
		else
			m_sMessage = string.Format("%1 membre(s) ajouté(s).", added);
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveMember(string uid)
	{
		int at = FindRow(uid);
		if (at >= 0)
			m_Dto.attendance.RemoveOrdered(at);
	}

	//------------------------------------------------------------------------------------------------
	//! value = "line|result" (result "" = —). Only the three tags are repainted, so the list does not jump.
	protected void PickResult(string value)
	{
		int bar = value.IndexOf("|");
		if (bar <= 0)
			return;

		int at = value.Substring(0, bar).ToInt();
		if (at < 0 || at >= m_aLines.Count())
			return;

		CGQC_GU_TrainingLine line = m_aLines[at];
		string result;
		if (bar + 1 < value.Length())
			result = value.Substring(bar + 1, value.Length() - bar - 1);

		line.m_Row.result = result;
		PaintResult(line);
	}

	//------------------------------------------------------------------------------------------------
	//! value = "line|position". The line is repainted in place; an assistant loses the grade controls.
	protected void PickPosition(string value)
	{
		int bar = value.IndexOf("|");
		if (bar <= 0 || bar + 1 >= value.Length())
			return;

		int at = value.Substring(0, bar).ToInt();
		if (at < 0 || at >= m_aLines.Count())
			return;

		CGQC_GU_TrainingLine line = m_aLines[at];
		line.m_Row.position = value.Substring(bar + 1, value.Length() - bar - 1);
		PaintPosition(line);
		ShowGrades(line);
	}

	//------------------------------------------------------------------------------------------------
	//! Reads the open fields and boxes into the draft, before anything is redrawn or sent.
	protected void Capture()
	{
		if (!m_Dto || !m_Dto.canEdit)
			return;

		if (m_wName)
			m_Dto.title = m_wName.GetText();

		if (m_Date)
			m_Dto.date = m_Date.GetValue();

		if (m_wTime)
			m_Dto.time = m_wTime.GetText().Trim();

		if (m_wPlace)
			m_Dto.place = m_wPlace.GetText();

		foreach (CGQC_GU_TrainingLine line : m_aLines)
		{
			CGQC_GU_TrainingRowDto r = line.m_Row;
			if (line.m_Present)
				r.present = line.m_Present.IsChecked();

			foreach (CGQC_GU_Check c : line.m_aExercises)
			{
				string code = c.GetId();
				if (c.IsChecked())
					r.exercises.Set(code, CGQC_GU_Quals.RESULT_PASSED);
				else if (r.exercises.Get(code) == CGQC_GU_Quals.RESULT_PASSED)
					r.exercises.Remove(code);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Sends the whole form with a write.
	protected void Send(string action, string status, string done)
	{
		Capture();

		CGQC_GU_Payload p = new CGQC_GU_Payload();
		p.Set("id", m_Dto.id);
		p.Set("type", m_Dto.type);
		p.Set("title", m_Dto.title);
		p.Set("date", m_Dto.date);
		p.Set("time", m_Dto.time);
		p.Set("place", m_Dto.place);
		p.Set("qual", m_Dto.qual);
		p.Set("instructor", m_Dto.instructor);

		array<string> codes = new array<string>();
		foreach (CGQC_GU_LabelItem e : m_Dto.exercises)
		{
			codes.Insert(e.id);
		}

		p.Set("exercises", CGQC_GU_Str.Join(codes, ","));

		// "uid|present|position|code:passed,…|result;…"
		array<string> rows = new array<string>();
		foreach (CGQC_GU_TrainingRowDto r : m_Dto.attendance)
		{
			string present = "0";
			if (r.present)
				present = "1";

			array<string> grades = new array<string>();
			foreach (string code, string value : r.exercises)
			{
				grades.Insert(code + ":" + value);
			}

			rows.Insert(r.uid + "|" + present + "|" + Clean(r.position) + "|" + CGQC_GU_Str.Join(grades, ",") + "|" + r.result);
		}

		p.Set("att", CGQC_GU_Str.Join(rows, ";"));
		if (!status.IsEmpty())
			p.Set("status", status);

		SetMessage("", false);
		m_sDone = done;
		m_iWriteReq = CGQC_GU_Client.RequestWith(action, p);
	}

	//------------------------------------------------------------------------------------------------
	//! The attendance separators cannot appear in a position.
	protected string Clean(string text)
	{
		string s = text;
		array<string> separators = { ";", "|", ",", ":" };
		foreach (string sep : separators)
		{
			s.Replace(sep, " ");
		}

		return s.Trim();
	}

	//------------------------------------------------------------------------------------------------
	// Page
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
			CGQC_GU_TrainingAction a = new CGQC_GU_TrainingAction(this, kind, value);
			region.m_aKeep.Insert(a);
			b.GetOnActivated().Insert(a.Fire);
		}

		return b;
	}

	//------------------------------------------------------------------------------------------------
	//! A button filling a box of a fixed width, so its armed label does not move the row.
	protected CGQC_GU_Button BoxBtn(CGQC_GU_Region region, Widget row, float width, float padLeft, string text, string kind, string value = "", bool confirm = false, CGQC_GU_EButtonKind look = CGQC_GU_EButtonKind.PLAIN)
	{
		Widget box = CGQC_GU_W.Sized(row, width);
		if (!box)
			return null;

		LayoutSlot.SetPadding(box.GetParent(), padLeft, 0, 0, 0);
		CGQC_GU_Button b = Btn(region, box, text, kind, value, confirm, look);
		if (b)
			LayoutSlot.SetHorizontalAlign(b.GetRootWidget(), LayoutHorizontalAlign.Stretch);

		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected void Rebuild()
	{
		ResetPickList();
		m_aLines.Clear();
		m_wName = null;
		m_Date = null;
		m_wTime = null;
		m_wPlace = null;
		m_wPresent = null;
		m_wMessage = null;

		BuildHead();
		BuildFiche();
		BuildRight();
	}

	//------------------------------------------------------------------------------------------------
	//! A session exists on screen: a stored one, or a new one being written.
	protected bool HasSession()
	{
		return m_Dto && !m_Dto.status.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! < title >, NOUVELLE SÉANCE, « n / n au dossier ».
	protected void BuildHead()
	{
		m_Head.Reset();
		Widget head = m_Head.m_wRoot;
		if (!head || !m_Dto)
			return;

		CGQC_GU_Button prev = BoxBtn(m_Head, head, 52, 0, "<", "open", m_Dto.prevId);
		if (prev)
			prev.SetEnabled(!m_Dto.prevId.IsEmpty());

		string title = m_Dto.title;
		if (!HasSession())
			title = "Aucune séance";
		else if (m_Dto.id.IsEmpty())
			title = "Nouvelle séance";

		TextWidget t = CGQC_GU_W.Text(head, title, CGQC_GU_Style.FONT_TITLE, 24, CGQC_GU_Style.Ink());
		if (t)
		{
			LayoutSlot.SetSizeMode(t, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(t, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(t, 16, 0, 16, 0);
		}

		CGQC_GU_Button next = BoxBtn(m_Head, head, 52, 0, ">", "open", m_Dto.nextId);
		if (next)
			next.SetEnabled(!m_Dto.nextId.IsEmpty());

		if (m_Dto.canCreate)
			BoxBtn(m_Head, head, 170, 16, "NOUVELLE SÉANCE", "new");

		Widget count = CGQC_GU_W.Sized(head, 160);
		if (!count)
			return;

		LayoutSlot.SetVerticalAlign(count.GetParent(), LayoutVerticalAlign.Center);
		if (m_Dto.index <= 0)
			return;

		TextWidget n = CGQC_GU_W.Text(count, string.Format("%1 / %2 au dossier", m_Dto.index, m_Dto.total), CGQC_GU_Style.FONT_LABEL, 16, CGQC_GU_Style.InkMuted());
		if (n)
			LayoutSlot.SetHorizontalAlign(n, LayoutHorizontalAlign.Right);
	}

	//------------------------------------------------------------------------------------------------
	// Fiche de la séance
	//------------------------------------------------------------------------------------------------
	protected void BuildFiche()
	{
		m_Fiche.Reset();
		if (!m_Fiche.m_wRoot)
			return;

		Widget f = CGQC_GU_W.Panel(m_Fiche.m_wRoot);
		if (!f)
			return;

		Widget box = f.GetParent();
		LayoutSlot.SetSizeMode(box, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(box, LayoutHorizontalAlign.Stretch);
		Widget bg = box.FindAnyWidget("Bg");
		if (bg)
			bg.SetColor(CGQC_GU_Style.PaperLight());

		AlignableSlot.SetPadding(f, 24, 22, 24, 22);

		CGQC_GU_W.Text(f, "FICHE DE LA SÉANCE", CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.InkMuted());
		if (!HasSession())
		{
			string empty = "Chargement…";
			if (m_Dto)
				empty = "Aucune séance au dossier.";

			CGQC_GU_W.Para(f, empty, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkMuted(), 12);
			return;
		}

		bool edit = m_Dto.canEdit;
		if (edit)
		{
			CGQC_GU_W.Caption(f, "TITRE", 10);
			m_wName = CGQC_GU_W.Field(f, m_Dto.title, FICHE_INNER);
		}
		else
		{
			CGQC_GU_W.Para(f, m_Dto.title, CGQC_GU_Style.FONT_TITLE, 30, CGQC_GU_Style.Ink(), 10);
		}

		string linked = "présence seulement";
		if (!m_Dto.qual.IsEmpty())
			linked = "qualification liée : " + m_Dto.qual;

		CGQC_GU_W.Para(f, CGQC_GU_TrainingRequests.TypeLabel(m_Dto.type) + " · " + linked, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkMuted(), 10);

		StatusChain(f);
		FicheTable(f, edit);

		if (!m_Dto.doneLine.IsEmpty())
			CGQC_GU_W.Para(f, m_Dto.doneLine, CGQC_GU_Style.FONT_LABEL, 15, CGQC_GU_Style.InkMuted(), 10);

		if (edit && m_bInstructorOpen)
			InstructorPicker(f);
	}

	//------------------------------------------------------------------------------------------------
	//! BROUILLON › PLANIFIÉE › TERMINÉE, the current one filled.
	protected void StatusChain(Widget f)
	{
		Widget row = CGQC_GU_W.HBox(f);
		if (!row)
			return;

		LayoutSlot.SetPadding(row, 0, 14, 0, 14);
		array<string> states = { CGQC_GU_Training.STATUS_DRAFT, CGQC_GU_Training.STATUS_PLANNED, CGQC_GU_Training.STATUS_DONE };
		for (int i = 0; i < states.Count(); i++)
		{
			if (i > 0)
			{
				TextWidget sep = CGQC_GU_W.Text(row, "›", CGQC_GU_Style.FONT_LABEL, 18, CGQC_GU_Style.BadgeDim());
				if (sep)
				{
					LayoutSlot.SetVerticalAlign(sep, LayoutVerticalAlign.Center);
					LayoutSlot.SetPadding(sep, 8, 0, 8, 0);
				}
			}

			bool current = states[i] == m_Dto.status;
			Color color = CGQC_GU_Style.BadgeDim();
			if (current)
				color = CGQC_GU_Style.Blue();

			Widget tag = CGQC_GU_W.Tag(row, CGQC_GU_TrainingRequests.StatusLabel(states[i]), color, 14);
			if (!tag || !current)
				continue;

			Widget fill = tag.FindAnyWidget("Bg");
			if (fill)
				fill.SetColor(CGQC_GU_Style.Blue());

			Widget inner = tag.FindAnyWidget("Inner");
			if (!inner)
				continue;

			TextWidget label = TextWidget.Cast(inner.GetChildren());
			if (label)
				label.SetColor(CGQC_GU_Style.PaperText());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Date, Début, Instructeur, Lieu, Présents: fields when the viewer may edit.
	protected void FicheTable(Widget f, bool edit)
	{
		Widget cells = FicheRow(f, "DATE", true);
		if (edit)
		{
			m_Date = CGQC_GU_DateField.Create(m_Fiche, cells, m_Dto.date);
		}
		else
		{
			Value(cells, CGQC_GU_DossierBuilder.DisplayDate(m_Dto.date));
		}

		cells = FicheRow(f, "DÉBUT");
		if (edit)
		{
			m_wTime = CGQC_GU_W.Field(cells, m_Dto.time, 100);
			Hint(cells, "HE (HH:MM)");
		}
		else
		{
			string time = "—";
			if (m_Dto.time.Length() == 5)
				time = m_Dto.time.Substring(0, 2) + " h " + m_Dto.time.Substring(3, 2) + " HE";

			Value(cells, time);
		}

		cells = FicheRow(f, "INSTRUCTEUR CHEF");
		if (edit)
		{
			string who = m_Dto.instructorName + "  +";
			if (m_bInstructorOpen)
				who = m_Dto.instructorName + "  −";

			Btn(m_Fiche, cells, who, "inst.toggle");
		}
		else
		{
			Value(cells, m_Dto.instructorName);
		}

		cells = FicheRow(f, "LIEU");
		if (edit)
		{
			m_wPlace = CGQC_GU_W.Field(cells, m_Dto.place, 280);
		}
		else
		{
			string place = m_Dto.place;
			if (place.IsEmpty())
				place = "—";

			Value(cells, place);
		}

		cells = FicheRow(f, "PRÉSENTS");
		m_wPresent = Value(cells, PresentCount().ToString());
	}

	//------------------------------------------------------------------------------------------------
	//! One ruled line of the fiche. Returns the row the value goes into.
	protected Widget FicheRow(Widget f, string label, bool first = false)
	{
		array<string> edges = { "EdgeB", "EdgeL", "EdgeR" };
		if (first)
			edges.Insert("EdgeT");

		CGQC_GU_ClickRow row = CGQC_GU_ClickRow.Create(f, null, CGQC_GU_Style.Clear(), CGQC_GU_Style.RowEdge(), edges);
		if (!row)
			return null;

		m_Fiche.m_aRows.Insert(row);
		Widget cells = row.GetCells();
		CGQC_GU_W.Para(CGQC_GU_Table.Stack(cells, 0, LABEL_WIDTH), label, CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.Ink());
		return CGQC_GU_W.HBox(CGQC_GU_Table.Stack(cells, 1, 0));
	}

	//------------------------------------------------------------------------------------------------
	protected TextWidget Value(Widget cells, string text)
	{
		return CGQC_GU_W.Text(cells, text, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkSoft());
	}

	//------------------------------------------------------------------------------------------------
	protected void Hint(Widget cells, string text)
	{
		TextWidget t = CGQC_GU_W.Text(cells, text, CGQC_GU_Style.FONT_LABEL, 14, CGQC_GU_Style.InkMuted());
		if (!t)
			return;

		LayoutSlot.SetVerticalAlign(t, LayoutVerticalAlign.Center);
		LayoutSlot.SetPadding(t, 10, 0, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void InstructorPicker(Widget f)
	{
		CGQC_GU_W.Caption(f, "CHOISIR L'INSTRUCTEUR CHEF", 12);
		Widget list = CGQC_GU_W.Scroll(f);
		Widget row;
		for (int i = 0; i < m_Dto.instructorChoices.Count(); i++)
		{
			if (i % 3 == 0)
			{
				row = CGQC_GU_W.HBox(list);
				if (row)
					LayoutSlot.SetPadding(row, 0, 0, 0, 8);
			}

			if (!row)
				break;

			CGQC_GU_LabelItem item = m_Dto.instructorChoices[i];
			CGQC_GU_Button b = Btn(m_Fiche, row, item.label, "inst.pick", item.id);
			if (!b)
				continue;

			b.SetSelected(item.id == m_Dto.instructor);
			LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Right side
	//------------------------------------------------------------------------------------------------
	protected void BuildRight()
	{
		m_Right.Reset();
		Widget r = m_Right.m_wRoot;
		if (!r)
			return;

		if (!HasSession())
		{
			Message(r);
			return;
		}

		bool edit = m_Dto.canEdit;
		TypeRow(r, edit);
		QualRow(r, edit);
		ExerciseRow(r, edit);

		Widget rule = CGQC_GU_W.Rule(r, 10, 10);
		if (rule)
			rule.SetOpacity(1);

		Widget list = CGQC_GU_W.Scroll(r);
		if (edit && m_bMemberOpen)
			MemberPicker(list);
		else
			Attendance(list, edit);

		Message(r);
		if (edit)
			Buttons(r);
	}

	//------------------------------------------------------------------------------------------------
	protected void Message(Widget r)
	{
		m_wMessage = CGQC_GU_W.Para(r, "", CGQC_GU_Style.FONT_TITLE, 16, CGQC_GU_Style.Red(), 6);
		SetMessage(m_sMessage, m_bMessageGood);
	}

	//------------------------------------------------------------------------------------------------
	//! FTX / ENTRAÎNEMENT, half the width each.
	protected void TypeRow(Widget r, bool edit)
	{
		Widget row = CGQC_GU_W.HBox(r);
		if (!row)
			return;

		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(row, 0, 0, 0, 10);
		array<string> types = { "ftx", "training" };
		foreach (int i, string type : types)
		{
			string kind;
			if (edit)
				kind = "type";

			CGQC_GU_Button b = Btn(m_Right, row, CGQC_GU_TrainingRequests.TypeLabel(type), kind, type);
			if (!b)
				continue;

			b.SetSelected(type == m_Dto.type);
			Widget w = b.GetRootWidget();
			LayoutSlot.SetSizeMode(w, LayoutSizeMode.Fill);
			LayoutSlot.SetHorizontalAlign(w, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(w, i * 6, 0, 0, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A labelled line of the right side. Returns the stack its content goes into.
	protected Widget Labelled(Widget r, string label)
	{
		Widget row = CGQC_GU_W.HBox(r);
		if (!row)
			return null;

		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(row, 0, 0, 0, 8);
		Widget box = CGQC_GU_W.Sized(row, FIELD_LABEL_WIDTH);
		if (box)
		{
			LayoutSlot.SetVerticalAlign(box.GetParent(), LayoutVerticalAlign.Top);
			CGQC_GU_W.Para(box, label, CGQC_GU_Style.FONT_TITLE, 15, CGQC_GU_Style.Ink(), 5);
		}

		Widget content = CGQC_GU_W.VBox(row);
		if (content)
		{
			LayoutSlot.SetSizeMode(content, LayoutSizeMode.Fill);
			LayoutSlot.SetPadding(content, 12, 0, 0, 0);
		}

		return content;
	}

	//------------------------------------------------------------------------------------------------
	protected void QualRow(Widget r, bool edit)
	{
		Widget content = Labelled(r, "QUALIFICATION LIÉE");
		if (!content)
			return;

		string current = "Aucune — présence seulement";
		if (!m_Dto.qual.IsEmpty())
			current = m_Dto.qual + " — " + m_Dto.qualTitle;

		if (!edit)
		{
			CGQC_GU_W.Para(content, current, CGQC_GU_Style.FONT_VALUE, 17, CGQC_GU_Style.InkSoft(), 4);
			return;
		}

		Widget row = CGQC_GU_W.HBox(content);
		string mark = "  +";
		if (m_bQualOpen)
			mark = "  −";

		BoxBtn(m_Right, row, 420, 0, current + mark, "qual.toggle");
		Hint(row, "ou « Aucune — présence seulement »");

		if (!m_bQualOpen)
			return;

		Widget picks = CGQC_GU_W.HBox(content);
		if (picks)
			LayoutSlot.SetPadding(picks, 0, 8, 0, 0);

		CGQC_GU_Button none = Btn(m_Right, picks, "Aucune — présence seulement", "qual.pick", "");
		if (none)
			none.SetSelected(m_Dto.qual.IsEmpty());

		for (int i = 0; i < m_Dto.qualChoices.Count(); i++)
		{
			if (i % QUALS_PER_ROW == 0)
			{
				picks = CGQC_GU_W.HBox(content);
				if (picks)
					LayoutSlot.SetPadding(picks, 0, 8, 0, 0);
			}

			CGQC_GU_LabelItem item = m_Dto.qualChoices[i];
			CGQC_GU_Button b = Btn(m_Right, picks, item.id + " — " + item.label, "qual.pick", item.id);
			if (!b)
				continue;

			b.SetSelected(item.id == m_Dto.qual);
			LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Exercises covered as tags (pressing × removes one), then + EXERCICE.
	protected void ExerciseRow(Widget r, bool edit)
	{
		Widget content = Labelled(r, "EXERCICES COUVERTS");
		if (!content)
			return;

		Widget row = CGQC_GU_W.HBox(content);
		int count = m_Dto.exercises.Count();
		for (int i = 0; i < count; i++)
		{
			if (i > 0 && i % EXERCISE_TAGS_PER_ROW == 0)
			{
				row = CGQC_GU_W.HBox(content);
				if (row)
					LayoutSlot.SetPadding(row, 0, 8, 0, 0);
			}

			CGQC_GU_LabelItem e = m_Dto.exercises[i];
			string text = e.id + " · " + e.label;
			if (edit)
			{
				CGQC_GU_Button b = Btn(m_Right, row, text + "  ×", "ex.remove", e.id);
				if (!b)
					continue;

				b.SetTextColor(CGQC_GU_Style.Blue());
				LayoutSlot.SetPadding(b.GetRootWidget(), 0, 0, 8, 0);
			}
			else
			{
				Widget tag = CGQC_GU_W.Tag(row, text, CGQC_GU_Style.Blue(), 14);
				if (tag)
					LayoutSlot.SetPadding(tag, 0, 3, 8, 3);
			}
		}

		if (!edit)
		{
			if (count == 0)
				CGQC_GU_W.Text(row, "Aucun", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());

			return;
		}

		// + EXERCICE follows the last tag, or takes the next line when that one is full.
		if (count > 0 && count % EXERCISE_TAGS_PER_ROW == 0)
		{
			row = CGQC_GU_W.HBox(content);
			if (row)
				LayoutSlot.SetPadding(row, 0, 8, 0, 0);
		}

		string add = "+ EXERCICE";
		if (m_bExerciseOpen)
			add = "− EXERCICE";

		Btn(m_Right, row, add, "ex.toggle");

		if (!m_bExerciseOpen)
			return;

		if (!m_aExercises)
		{
			CGQC_GU_W.Para(content, "Chargement…", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 6);
			return;
		}

		// With a qualification, its own exercises only.
		int shown;
		Widget picks;
		foreach (CGQC_GU_ExerciseDto x : m_aExercises)
		{
			if (FindCovered(x.code) >= 0 || (!m_Dto.qual.IsEmpty() && x.qual != m_Dto.qual))
				continue;

			if (shown % EXERCISE_TAGS_PER_ROW == 0)
			{
				picks = CGQC_GU_W.HBox(content);
				if (picks)
					LayoutSlot.SetPadding(picks, 0, 8, 0, 0);
			}

			shown++;
			CGQC_GU_Button pick = Btn(m_Right, picks, x.code + " · " + x.title, "ex.add", x.code);
			if (pick)
				LayoutSlot.SetPadding(pick.GetRootWidget(), 0, 0, 8, 0);
		}

		if (shown == 0)
			CGQC_GU_W.Para(content, "Aucun autre exercice au catalogue.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 6);
	}

	//------------------------------------------------------------------------------------------------
	// Attendance
	//------------------------------------------------------------------------------------------------
	//! ✓, member, position, then for a qualification session one column per test exercise and the result.
	protected void Attendance(Widget list, bool edit)
	{
		bool graded = !m_Dto.qual.IsEmpty();
		array<float> widths = { CHECK_WIDTH, 0, POSITION_WIDTH };
		array<string> titles = { "✓", "MEMBRE", "POSITION" };
		if (graded)
		{
			// The exercise columns narrow when there are many, so the member column stays readable.
			float width = EXERCISE_WIDTH;
			int n = m_Dto.exercises.Count();
			if (n > 0)
				width = Math.Min(width, (TABLE_WIDTH - FIXED_WIDTH - 190) / n - 20);

			foreach (CGQC_GU_LabelItem e : m_Dto.exercises)
			{
				titles.Insert(e.id);
				widths.Insert(width);
			}

			titles.Insert("RÉSULTAT " + m_Dto.qual);
			widths.Insert(RESULT_WIDTH);
		}

		CGQC_GU_Table.Head(m_Right, list, titles, widths);

		foreach (CGQC_GU_TrainingRowDto row : m_Dto.attendance)
		{
			AttendanceLine(list, row, widths, graded, edit);
		}

		if (m_Dto.attendance.IsEmpty())
			CGQC_GU_W.Para(list, "Aucun membre.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted(), 8);

		if (!edit)
			return;

		Widget add = CGQC_GU_W.HBox(list);
		if (!add)
			return;

		LayoutSlot.SetPadding(add, 0, 10, 0, 0);
		Btn(m_Right, add, "+ AJOUTER UN MEMBRE", "member.toggle");
		CGQC_GU_Button all = Btn(m_Right, add, "+ AJOUTER TOUS LES JOUEURS EN LIGNE", "member.online");
		if (all)
			LayoutSlot.SetPadding(all.GetRootWidget(), 8, 0, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void AttendanceLine(Widget list, CGQC_GU_TrainingRowDto r, array<float> widths, bool graded, bool edit)
	{
		Widget cells = CGQC_GU_Table.Row(m_Right, list);
		if (!cells)
			return;

		CGQC_GU_TrainingLine line = new CGQC_GU_TrainingLine();
		line.m_Row = r;
		int at = m_aLines.Insert(line);

		line.m_Present = Box(CGQC_GU_Table.Stack(cells, 0, widths[0]), "", r.present, edit);
		if (line.m_Present)
			line.m_Present.GetOnChanged().Insert(OnPresentChanged);

		Widget who = CGQC_GU_W.HBox(CGQC_GU_Table.Stack(cells, 1, widths[1]));
		TextWidget name = CGQC_GU_W.Text(who, r.name, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft(), true);
		if (name)
		{
			LayoutSlot.SetSizeMode(name, LayoutSizeMode.Fill);
			LayoutSlot.SetVerticalAlign(name, LayoutVerticalAlign.Center);
		}

		if (edit)
			Btn(m_Right, who, "×", "member.remove", r.uid);

		Widget position = CGQC_GU_Table.Stack(cells, 2, widths[2]);
		if (edit)
		{
			Widget choices = CGQC_GU_W.HBox(position);
			PositionTag(choices, line, at, "INSTRUCTEUR", CGQC_GU_TrainingRequests.POSITION_ASSISTANT);
			PositionTag(choices, line, at, "ÉLÈVE", CGQC_GU_TrainingRequests.POSITION_STUDENT);
			PaintPosition(line);
		}
		else
		{
			string text = r.position;
			if (text.IsEmpty())
				text = "—";

			CGQC_GU_W.Para(position, text, CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		}

		if (!graded)
			return;

		int column = 3;
		foreach (CGQC_GU_LabelItem e : m_Dto.exercises)
		{
			Widget cell = CGQC_GU_Table.Stack(cells, column, widths[column]);
			column++;
			line.m_aGradeCells.Insert(cell);
			bool passed = r.exercises.Get(e.id) == CGQC_GU_Quals.RESULT_PASSED;
			CGQC_GU_Check c = Box(cell, e.id, passed, edit);
			if (c)
				line.m_aExercises.Insert(c);
		}

		Widget resultCell = CGQC_GU_Table.Stack(cells, column, widths[column]);
		line.m_aGradeCells.Insert(resultCell);
		Widget results = CGQC_GU_W.HBox(resultCell);
		ResultTag(results, line, at, "RÉUSSI", CGQC_GU_Quals.RESULT_PASSED, edit);
		ResultTag(results, line, at, "ÉCHOUÉ", CGQC_GU_Quals.RESULT_FAILED, edit);
		ResultTag(results, line, at, "—", "", edit);
		PaintResult(line);
		ShowGrades(line);
	}

	//------------------------------------------------------------------------------------------------
	//! No grade controls for an assistant, nor for the instructor chef unless Commandement.
	protected void ShowGrades(CGQC_GU_TrainingLine line)
	{
		bool shown = !line.m_Row.noGrade && !CGQC_GU_TrainingRequests.IsInstructorPosition(line.m_Row.position);
		foreach (Widget cell : line.m_aGradeCells)
		{
			if (cell)
				cell.SetVisible(shown);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A checkbox without a visible label; id is kept for reading it back.
	protected CGQC_GU_Check Box(Widget parent, string id, bool checked, bool edit)
	{
		CGQC_GU_Check c = CGQC_GU_Check.Create(parent, id, "", checked);
		if (!c)
			return null;

		if (!edit)
			c.SetReadOnly();

		m_Right.m_aChecks.Insert(c);
		return c;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResultTag(Widget parent, CGQC_GU_TrainingLine line, int at, string text, string result, bool edit)
	{
		CGQC_GU_TrainingAction action;
		if (edit)
			action = new CGQC_GU_TrainingAction(this, "result", at.ToString() + "|" + result);

		CGQC_GU_ClickRow tag = Tag(parent, text, action);
		if (tag)
			line.m_aTags.Insert(tag);
	}

	//------------------------------------------------------------------------------------------------
	protected void PositionTag(Widget parent, CGQC_GU_TrainingLine line, int at, string text, string position)
	{
		CGQC_GU_TrainingAction action = new CGQC_GU_TrainingAction(this, "position", at.ToString() + "|" + position);
		CGQC_GU_ClickRow tag = Tag(parent, text, action);
		if (tag)
			line.m_aPositions.Insert(tag);
	}

	//------------------------------------------------------------------------------------------------
	//! A small outline tag, pressable when an action is given; painted by PaintTag.
	protected CGQC_GU_ClickRow Tag(Widget parent, string text, CGQC_GU_Action action)
	{
		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		CGQC_GU_ClickRow tag = CGQC_GU_ClickRow.Create(parent, action, CGQC_GU_Style.Clear(), CGQC_GU_Style.BadgeDim(), edges);
		if (!tag)
			return null;

		m_Right.m_aRows.Insert(tag);
		LayoutSlot.SetHorizontalAlign(tag.GetRootWidget(), LayoutHorizontalAlign.Left);
		LayoutSlot.SetPadding(tag.GetRootWidget(), 0, 0, 8, 0);

		TextWidget t = CGQC_GU_W.Text(tag.GetCells(), text, CGQC_GU_Style.FONT_TITLE, 13, CGQC_GU_Style.BadgeDim());
		if (t)
		{
			t.SetName("TagText");
			LayoutSlot.SetPadding(t, 8, 3, 8, 3);
		}

		return tag;
	}

	//------------------------------------------------------------------------------------------------
	protected void PaintTag(CGQC_GU_ClickRow tag, Color color)
	{
		Widget root = tag.GetRootWidget();
		if (!root)
			return;

		array<string> edges = { "EdgeT", "EdgeB", "EdgeL", "EdgeR" };
		foreach (string edge : edges)
		{
			Widget e = root.FindAnyWidget(edge);
			if (e)
				e.SetColor(color);
		}

		TextWidget t = TextWidget.Cast(root.FindAnyWidget("TagText"));
		if (t)
			t.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	//! The chosen result in its colour (blue, red, ink), the others dimmed.
	protected void PaintResult(CGQC_GU_TrainingLine line)
	{
		array<string> results = { CGQC_GU_Quals.RESULT_PASSED, CGQC_GU_Quals.RESULT_FAILED, "" };
		for (int i = 0; i < line.m_aTags.Count() && i < results.Count(); i++)
		{
			Color color = CGQC_GU_Style.BadgeDim();
			if (line.m_Row.result == results[i])
			{
				if (i == 0)
					color = CGQC_GU_Style.Blue();
				else if (i == 1)
					color = CGQC_GU_Style.Red();
				else
					color = CGQC_GU_Style.InkSoft();
			}

			PaintTag(line.m_aTags[i], color);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The chosen position in blue.
	protected void PaintPosition(CGQC_GU_TrainingLine line)
	{
		array<string> positions = { CGQC_GU_TrainingRequests.POSITION_ASSISTANT, CGQC_GU_TrainingRequests.POSITION_STUDENT };
		for (int i = 0; i < line.m_aPositions.Count() && i < positions.Count(); i++)
		{
			Color color = CGQC_GU_Style.BadgeDim();
			if (line.m_Row.position == positions[i])
				color = CGQC_GU_Style.Blue();

			PaintTag(line.m_aPositions[i], color);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int PresentCount()
	{
		int n;
		foreach (CGQC_GU_TrainingRowDto r : m_Dto.attendance)
		{
			if (r.present)
				n++;
		}

		return n;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPresentChanged()
	{
		foreach (CGQC_GU_TrainingLine line : m_aLines)
		{
			if (line.m_Present)
				line.m_Row.present = line.m_Present.IsChecked();
		}

		if (m_wPresent)
			m_wPresent.SetText(PresentCount().ToString());
	}

	//------------------------------------------------------------------------------------------------
	// + Ajouter un membre
	//------------------------------------------------------------------------------------------------
	protected void MemberPicker(Widget list)
	{
		Widget panel = CGQC_GU_W.Panel(list);
		if (!panel)
			return;

		LayoutSlot.SetHorizontalAlign(panel.GetParent(), LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetPadding(panel, 14, 10, 14, 10);

		Widget head = CGQC_GU_W.HBox(panel);
		if (head)
		{
			LayoutSlot.SetHorizontalAlign(head, LayoutHorizontalAlign.Stretch);
			TextWidget cap = CGQC_GU_W.Text(head, "AJOUTER UN MEMBRE", CGQC_GU_Style.FONT_TITLE, 14, CGQC_GU_Style.InkMuted());
			if (cap)
			{
				LayoutSlot.SetSizeMode(cap, LayoutSizeMode.Fill);
				LayoutSlot.SetVerticalAlign(cap, LayoutVerticalAlign.Center);
			}

			Btn(m_Right, head, "FERMER LA LISTE", "member.toggle");
		}

		CGQC_GU_W.Caption(panel, "RECHERCHE", 6);
		m_wSearch = CGQC_GU_W.Field(panel, m_sSearch, 360);
		if (m_wSearch)
		{
			if (!m_SearchWatch)
			{
				m_SearchWatch = new CGQC_GU_FieldWatch();
				m_SearchWatch.GetOnChanged().Insert(OnSearchChanged);
			}

			m_wSearch.AddHandler(m_SearchWatch);
		}

		m_PickList.m_wRoot = CGQC_GU_W.VBox(panel);
		if (m_PickList.m_wRoot)
		{
			LayoutSlot.SetHorizontalAlign(m_PickList.m_wRoot, LayoutHorizontalAlign.Stretch);
			LayoutSlot.SetPadding(m_PickList.m_wRoot, 0, 10, 0, 0);
		}

		FillPickList();
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetPickList()
	{
		if (m_wSearch)
			m_sSearch = m_wSearch.GetText();

		if (m_wSearch && m_SearchWatch)
			m_wSearch.RemoveHandler(m_SearchWatch);

		m_wSearch = null;
		m_PickList.Reset();
		m_PickList.m_wRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Typing refreshes the list only, so the field keeps its focus.
	protected void OnSearchChanged()
	{
		m_iMemberPage = 0;
		FillPickList();
	}

	//------------------------------------------------------------------------------------------------
	//! Members not yet in the session: online first, then by name.
	protected void FillPickList()
	{
		m_PickList.Reset();
		Widget list = m_PickList.m_wRoot;
		if (!list || !m_Dto)
			return;

		if (!m_aMembers)
		{
			CGQC_GU_W.Text(list, "Chargement…", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());
			return;
		}

		string needle;
		if (m_wSearch)
			needle = CGQC_GU_Str.Lower(m_wSearch.GetText().Trim());

		array<CGQC_GU_MemberSummary> shown = new array<CGQC_GU_MemberSummary>();
		foreach (CGQC_GU_MemberSummary m : m_aMembers)
		{
			if (FindRow(m.uid) >= 0 || NoGrade(m.uid))
				continue;

			if (!needle.IsEmpty())
			{
				string hay = CGQC_GU_Str.Lower(m.rankAbbr + " " + m.displayName + " " + m.lastName);
				if (!hay.Contains(needle))
					continue;
			}

			InsertSorted(shown, m);
		}

		if (shown.IsEmpty())
			CGQC_GU_W.Text(list, "Aucun membre.", CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkMuted());

		int pages = Math.Max(1, (shown.Count() + MEMBER_PAGE - 1) / MEMBER_PAGE);
		m_iMemberPage = Math.ClampInt(m_iMemberPage, 0, pages - 1);
		int to = Math.Min((m_iMemberPage + 1) * MEMBER_PAGE, shown.Count());
		for (int k = m_iMemberPage * MEMBER_PAGE; k < to; k++)
		{
			CGQC_GU_MemberSummary s = shown[k];
			Widget row = CGQC_GU_W.HBox(list);
			if (!row)
				continue;

			LayoutSlot.SetPadding(row, 0, 0, 0, 6);
			Color dot = CGQC_GU_Style.Offline();
			if (s.online)
				dot = CGQC_GU_Style.Online();

			ImageWidget d = CGQC_GU_W.Dot(row, 10, dot);
			if (d)
				LayoutSlot.SetPadding(d, 0, 0, 12, 0);

			Btn(m_PickList, row, s.rankAbbr + " " + s.displayName, "member.add", s.uid);
		}

		if (pages <= 1)
			return;

		Widget pager = CGQC_GU_W.HBox(list);
		CGQC_GU_Button prev = Btn(m_PickList, pager, "PRÉCÉDENT", "page", (m_iMemberPage - 1).ToString());
		if (prev)
			prev.SetEnabled(m_iMemberPage > 0);

		TextWidget n = CGQC_GU_W.Text(pager, string.Format("%1 / %2", m_iMemberPage + 1, pages), CGQC_GU_Style.FONT_VALUE, 16, CGQC_GU_Style.InkSoft());
		if (n)
		{
			LayoutSlot.SetVerticalAlign(n, LayoutVerticalAlign.Center);
			LayoutSlot.SetPadding(n, 12, 0, 12, 0);
		}

		CGQC_GU_Button next = Btn(m_PickList, pager, "SUIVANT", "page", (m_iMemberPage + 1).ToString());
		if (next)
			next.SetEnabled(m_iMemberPage < pages - 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Online first, then by name.
	protected void InsertSorted(array<CGQC_GU_MemberSummary> list, CGQC_GU_MemberSummary m)
	{
		string name = CGQC_GU_Str.Lower(m.displayName);
		int at = list.Count();
		for (int i = 0; i < list.Count(); i++)
		{
			bool before = m.online && !list[i].online;
			if (m.online == list[i].online)
				before = name.Compare(CGQC_GU_Str.Lower(list[i].displayName)) < 0;

			if (before)
			{
				at = i;
				break;
			}
		}

		list.InsertAt(m, at);
	}

	//------------------------------------------------------------------------------------------------
	// Writes
	//------------------------------------------------------------------------------------------------
	protected void Buttons(Widget r)
	{
		Widget row = CGQC_GU_W.HBox(r);
		if (!row)
			return;

		LayoutSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(row, 0, 8, 0, 0);

		bool isNew = m_Dto.id.IsEmpty();
		bool planned = m_Dto.status == CGQC_GU_Training.STATUS_PLANNED;

		string save = "ENREGISTRER LE BROUILLON";
		if (planned)
			save = "ENREGISTRER";

		BoxBtn(m_Right, row, 205, 0, save, "save", "", true);

		if (planned)
			BoxBtn(m_Right, row, 190, 8, "REMETTRE EN BROUILLON", "unplan", "", true);
		else
			BoxBtn(m_Right, row, 190, 8, "PLANIFIER", "plan", "", true);

		// A session never written is only dropped from the screen.
		if (isNew)
		{
			BoxBtn(m_Right, row, 130, 8, "ANNULER", "open", "");
		}
		else
		{
			CGQC_GU_Button remove = BoxBtn(m_Right, row, 130, 8, "SUPPRIMER", "delete", "", true);
			if (remove)
				remove.SetConfirmText("CONFIRMER");
		}

		CGQC_GU_Button discord = BoxBtn(m_Right, row, 165, 8, "PUBLIER SUR DISCORD", "");
		if (discord)
			discord.SetEnabled(false);

		CGQC_GU_Button finish = Btn(m_Right, row, "TERMINER ET AJOUTER AUX DOSSIERS", "finish", "", true, CGQC_GU_EButtonKind.PRIMARY);
		if (!finish)
			return;

		finish.SetEnabled(!isNew);
		Widget w = finish.GetRootWidget();
		LayoutSlot.SetSizeMode(w, LayoutSizeMode.Fill);
		LayoutSlot.SetHorizontalAlign(w, LayoutHorizontalAlign.Stretch);
		LayoutSlot.SetPadding(w, 8, 0, 0, 0);
	}

	//------------------------------------------------------------------------------------------------
	// Helpers
	//------------------------------------------------------------------------------------------------
	protected CGQC_GU_ExerciseDto FindExercise(string code)
	{
		if (!m_aExercises)
			return null;

		foreach (CGQC_GU_ExerciseDto e : m_aExercises)
		{
			if (e.code == code)
				return e;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected int FindCovered(string code)
	{
		foreach (int i, CGQC_GU_LabelItem e : m_Dto.exercises)
		{
			if (e.id == code)
				return i;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected int FindRow(string uid)
	{
		foreach (int i, CGQC_GU_TrainingRowDto r : m_Dto.attendance)
		{
			if (r.uid == uid)
				return i;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected string LabelOf(array<ref CGQC_GU_LabelItem> items, string id)
	{
		foreach (CGQC_GU_LabelItem item : items)
		{
			if (item.id == id)
				return item.label;
		}

		return string.Empty;
	}
}
