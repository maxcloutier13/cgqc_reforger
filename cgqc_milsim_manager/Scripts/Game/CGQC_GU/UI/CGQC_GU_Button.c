// Gestion d'unité - button built from CGQC_GU_Button.layout.
// Confirm buttons (save, delete, publish, award, merge) need two presses (spec §1): the first turns the
// button red with « APPUYER DE NOUVEAU », the second executes; it resets after CONFIRM_TIMEOUT.

enum CGQC_GU_EButtonKind
{
	PRIMARY,	// CGQC blue, light text
	PLAIN,		// paper button, dark text
	DANGER		// red
}

class CGQC_GU_Button : ScriptedWidgetEventHandler
{
	protected Widget m_wRoot;
	protected ImageWidget m_wBg;
	protected TextWidget m_wLabel;
	protected string m_sText;
	protected CGQC_GU_EButtonKind m_eKind;
	protected bool m_bConfirm;
	protected bool m_bArmed;
	protected bool m_bEnabled = true;

	protected ref ScriptInvokerVoid m_OnActivated = new ScriptInvokerVoid();

	//------------------------------------------------------------------------------------------------
	//! Creates the button inside parent (usually a layout widget). Keep the returned ref alive.
	static CGQC_GU_Button Create(Widget parent, string text, CGQC_GU_EButtonKind kind = CGQC_GU_EButtonKind.PLAIN, bool confirm = false)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(CGQC_GU_Style.LAYOUT_BUTTON, parent);
		if (!root)
		{
			CGQC_GU_Log.Error("Layout bouton introuvable : " + CGQC_GU_Style.LAYOUT_BUTTON);
			return null;
		}

		CGQC_GU_Button b = new CGQC_GU_Button();
		b.Init(root, text, kind, confirm);
		return b;
	}

	//------------------------------------------------------------------------------------------------
	protected void Init(Widget root, string text, CGQC_GU_EButtonKind kind, bool confirm)
	{
		m_wRoot = root;
		m_wBg = ImageWidget.Cast(root.FindAnyWidget("Bg"));
		m_wLabel = TextWidget.Cast(root.FindAnyWidget("Label"));
		m_sText = text;
		m_eKind = kind;
		m_bConfirm = confirm;
		root.AddHandler(this);
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvokerVoid GetOnActivated()
	{
		return m_OnActivated;
	}

	//------------------------------------------------------------------------------------------------
	Widget GetRootWidget()
	{
		return m_wRoot;
	}

	//------------------------------------------------------------------------------------------------
	void SetText(string text)
	{
		m_sText = text;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void SetEnabled(bool enabled)
	{
		m_bEnabled = enabled;
		if (!enabled)
			Disarm();

		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_bEnabled)
			return true;

		if (!m_bConfirm)
		{
			m_OnActivated.Invoke();
			return true;
		}

		if (!m_bArmed)
		{
			m_bArmed = true;
			Refresh();
			GetGame().GetCallqueue().Remove(Disarm);
			GetGame().GetCallqueue().CallLater(Disarm, CGQC_GU_Style.CONFIRM_TIMEOUT * 1000, false);
			return true;
		}

		Disarm();
		m_OnActivated.Invoke();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Disarm()
	{
		GetGame().GetCallqueue().Remove(Disarm);
		if (!m_bArmed)
			return;

		m_bArmed = false;
		Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void Refresh()
	{
		if (!m_wRoot)
			return;

		Color bg;
		Color fg;
		if (m_bArmed)
		{
			bg = CGQC_GU_Style.ConfirmRed();
			fg = CGQC_GU_Style.PaperText();
		}
		else if (m_eKind == CGQC_GU_EButtonKind.PRIMARY)
		{
			bg = CGQC_GU_Style.Blue();
			fg = CGQC_GU_Style.PaperText();
		}
		else if (m_eKind == CGQC_GU_EButtonKind.DANGER)
		{
			bg = CGQC_GU_Style.Red();
			fg = CGQC_GU_Style.White();
		}
		else
		{
			bg = CGQC_GU_Style.PaperButton();
			fg = CGQC_GU_Style.InkSoft();
		}

		if (m_wBg)
			m_wBg.SetColor(bg);

		if (m_wLabel)
		{
			m_wLabel.SetColor(fg);
			if (m_bArmed)
				m_wLabel.SetText(CGQC_GU_Style.CONFIRM_TEXT);
			else
				m_wLabel.SetText(m_sText);
		}

		if (m_bEnabled)
			m_wRoot.SetOpacity(1);
		else
			m_wRoot.SetOpacity(0.45);
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		GetGame().GetCallqueue().Remove(Disarm);
		if (m_wRoot)
		{
			m_wRoot.RemoveHandler(this);
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}
	}
}
