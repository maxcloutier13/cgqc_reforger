// Gestion d'unité - pause menu hook (script-only modded class). The pause menu is the host of every
// Gestion d'unité screen, shows the CGQC logo in place of the game title, and carries the overview panel.

modded class PauseMenuUI
{
	protected ref CGQC_GU_OverviewPanel m_CGQC_GU_Overview;
	protected SCR_ButtonTextComponent m_CGQC_GU_MenuButton;

	protected static const ResourceName CGQC_GU_PAUSE_BUTTON = "{9ECCD201BCF07E95}UI/layouts/Menus/PauseMenu/PauseMenuButton.layout";
	protected static const float CGQC_GU_LOGO_SIZE = 150;	// source is 128 px; kept close to native to stay sharp

	//------------------------------------------------------------------------------------------------
	override protected void OnMenuOpen()
	{
		super.OnMenuOpen();

		CGQC_GU_Ui.OnHostOpened(m_wRoot);
		CGQC_GU_SetLogo();

		m_CGQC_GU_Overview = new CGQC_GU_OverviewPanel();
		m_CGQC_GU_Overview.Create(m_wRoot);

		// « Gestion d'unité » under « Continuer », for unit.menu holders only (Zeus, Commandement).
		// It is not the game's Game Master entry, which stays untouched.
		m_CGQC_GU_MenuButton = CGQC_GU_AddButtonAfter("Continue", "Gestion d'unité");
		if (m_CGQC_GU_MenuButton)
		{
			m_CGQC_GU_MenuButton.m_OnClicked.Insert(CGQC_GU_OnUnitMenu);
			CGQC_GU_ApplyMenuRight();
			CGQC_GU_Client.GetOnLightState().Insert(CGQC_GU_ApplyMenuRight);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CGQC_GU_ApplyMenuRight()
	{
		if (!m_CGQC_GU_MenuButton || !m_CGQC_GU_MenuButton.GetRootWidget())
			return;

		m_CGQC_GU_MenuButton.GetRootWidget().SetVisible(CGQC_GU_Client.GetLightState().Can(CGQC_GU_LightState.CAN_MENU));
	}

	//------------------------------------------------------------------------------------------------
	protected void CGQC_GU_OnUnitMenu()
	{
		CGQC_GU_Ui.OpenUnitMenu();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnMenuClose()
	{
		CGQC_GU_Client.GetOnLightState().Remove(CGQC_GU_ApplyMenuRight);
		m_CGQC_GU_MenuButton = null;

		if (m_CGQC_GU_Overview)
		{
			m_CGQC_GU_Overview.Destroy();
			m_CGQC_GU_Overview = null;
		}

		CGQC_GU_Ui.OnHostClosed();
		super.OnMenuClose();
	}

	//------------------------------------------------------------------------------------------------
	//! The CGQC logo replaces the "ARMA REFORGER" title image (spec §4).
	protected void CGQC_GU_SetLogo()
	{
		ImageWidget logo = ImageWidget.Cast(m_wRoot.FindAnyWidget("Logo"));
		if (!logo)
			return;

		if (!logo.LoadImageTexture(0, CGQC_GU_Style.LOGO))
			return;

		logo.SetImage(0);
		logo.SetSize(CGQC_GU_LOGO_SIZE, CGQC_GU_LOGO_SIZE);
	}

	//------------------------------------------------------------------------------------------------
	//! Inserts a vanilla-styled pause button right after the named one (same approach as the game's
	//! own rally restart button in SCR_PauseMenuUI_modded.c). Used by Phase 2 for « Gestion d'unité ».
	protected SCR_ButtonTextComponent CGQC_GU_AddButtonAfter(string afterName, string text)
	{
		SCR_ButtonTextComponent after = SCR_ButtonTextComponent.GetButtonText(afterName, m_wRoot);
		if (!after)
			return null;

		Widget afterWidget = after.GetRootWidget();
		Widget parent = afterWidget.GetParent();
		if (!parent)
			return null;

		array<ref Widget> existing = {};
		SCR_WidgetHelper.GetAllChildren(parent, existing);
		int insertPosition = existing.Find(afterWidget) + 1;
		if (insertPosition <= 0)
			return null;

		int order;
		foreach (Widget w : existing)
		{
			if (order == insertPosition)
				order++;

			w.SetZOrder(order);
			order++;
		}

		Widget button = GetGame().GetWorkspace().CreateWidgets(CGQC_GU_PAUSE_BUTTON, parent);
		if (!button)
			return null;

		button.SetZOrder(insertPosition);

		SizeLayoutWidget size = SizeLayoutWidget.Cast(button.GetChildren());
		if (size)
		{
			size.SetHeightOverride(54);
			LayoutSlot.SetPadding(size, 7, 1, 0, 1);
		}

		SCR_ButtonTextComponent comp = SCR_ButtonTextComponent.Cast(button.FindHandler(SCR_ButtonTextComponent));
		if (comp)
			comp.SetText(text);

		return comp;
	}
}
