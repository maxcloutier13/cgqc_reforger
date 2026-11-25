// Hooks of the route planner into the vanilla map. No fields are added here on purpose:
// these classes are built from the map config.

//------------------------------------------------------------------------------------------------
modded class SCR_MapMarkersUI
{
	//------------------------------------------------------------------------------------------------
	override void Init()
	{
		super.Init();

		SCR_MapRadialUI radialUI = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));
		if (radialUI)
			radialUI.GetOnMenuInitInvoker().Insert(CGQC_OnRadialMenuInit);
	}

	//------------------------------------------------------------------------------------------------
	protected void CGQC_OnRadialMenuInit()
	{
		SCR_MapRadialUI radialUI = SCR_MapRadialUI.Cast(m_MapEntity.GetMapUIComponent(SCR_MapRadialUI));
		if (!radialUI)
			return;

		SCR_SelectionMenuEntry entry = radialUI.AddRadialEntry("Route Planner");
		entry.SetIcon(UIConstants.ICONS_IMAGE_SET, "dependencies");	// points joined by lines, the game has no route icon
		entry.GetOnPerform().Insert(CGQC_OnRoutePlannerPerformed);
	}

	//------------------------------------------------------------------------------------------------
	protected void CGQC_OnRoutePlannerPerformed(SCR_SelectionMenuEntry entry)
	{
		CGQC_RoutePlanner.GetInstance().StartPlanning();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		CGQC_RoutePlanner.GetInstance().OnMapOpen(m_MapEntity);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMapClose(MapConfiguration config)
	{
		CGQC_RoutePlanner.GetInstance().OnMapClose();
		super.OnMapClose(config);
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);
		CGQC_RoutePlanner.GetInstance().Update();
	}

	//------------------------------------------------------------------------------------------------
	//! Left clicks place route points while planning, they must not edit markers
	override protected void OnInputMapSelect(float value, EActionTrigger reason)
	{
		if (CGQC_RoutePlanner.IsPlanning())
			return;

		super.OnInputMapSelect(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Two quick points are not a double click for the marker menu
	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		if (CGQC_RoutePlanner.IsPlanning())
			return;

		super.OnInputQuickMarkerMenu(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! A marker under the cursor wins over a route
	override protected void OnInputMarkerDelete(float value, EActionTrigger reason)
	{
		if (CGQC_RoutePlanner.IsPlanning())
			return;

		if (!m_MarkerEditRoot && !CGQC_IsMarkerUnderCursor())
			CGQC_RoutePlanner.GetInstance().DeleteRouteUnderCursor();

		super.OnInputMarkerDelete(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	protected bool CGQC_IsMarkerUnderCursor()
	{
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		foreach (Widget widget : widgets)
		{
			if (widget.FindHandler(SCR_MapMarkerWidgetComponent) && m_MarkerMgr.GetMarkerByWidget(widget))
				return true;
		}

		return false;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_GadgetManagerComponent
{
	//------------------------------------------------------------------------------------------------
	//! Esc on the hand-held map ("MapEscape"): while planning it goes to the planner and keeps the map open
	override protected void OnPauseMenu()
	{
		if (CGQC_RoutePlanner.IsPlanning())
		{
			CGQC_RoutePlanner.GetInstance().OnEscape();
			return;
		}

		super.OnPauseMenu();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_MapRadialUI
{
	//------------------------------------------------------------------------------------------------
	//! Right click removes a point or finishes the route instead of opening the menu
	override protected void OnInputMenuOpen(float value, EActionTrigger reason)
	{
		if (!CGQC_RoutePlanner.IsPlanning())
		{
			super.OnInputMenuOpen(value, reason);
			return;
		}

		if (m_CursorModule.GetCursorState() & EMapCursorState.CS_PAN)
			return;

		CGQC_RoutePlanner.GetInstance().OnContextClick();
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps every other path to the radial menu closed while planning.
	//! Do not mod SCR_MapCursorModule for this: the map config then fails to create it ("Unknown class").
	override protected void InputOpenMenu(SCR_RadialMenuController controller, bool hasControl)
	{
		if (CGQC_RoutePlanner.IsPlanning())
		{
			m_RadialController.SetEnableControl(false);	// vanilla re-enables it on the next normal open
			return;
		}

		super.InputOpenMenu(controller, hasControl);
	}
}
