//------------------------------------------------------------------------------------------------
//! Widgets of one drawn route point, pooled and reused between redraws
class CGQC_RoutePointWidgets
{
	ImageWidget m_wGlow;
	ImageWidget m_wIcon;
	TextWidget m_wLabel;
	string m_sQuad;		//!< icon currently loaded

	//------------------------------------------------------------------------------------------------
	void SetVisible(bool visible)
	{
		m_wGlow.SetVisible(visible);
		m_wIcon.SetVisible(visible);
		m_wLabel.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	void Remove()
	{
		if (m_wGlow)
			m_wGlow.RemoveFromHierarchy();

		if (m_wIcon)
			m_wIcon.RemoveFromHierarchy();

		if (m_wLabel)
			m_wLabel.RemoveFromHierarchy();
	}
}

//------------------------------------------------------------------------------------------------
//! Client side of the route planner on the 2D map: planning input, drawing and deletion.
//! The route being planned is local; it is shared with the faction when planning ends.
//! Kept as a standalone singleton so no fields are added to the config-built map UI classes.
class CGQC_RoutePlanner
{
	protected static const ResourceName LABEL_FONT = "{EABA4FE9D014CCEF}UI/Fonts/RobotoCondensed/RobotoCondensed_Bold.fnt";
	protected static const ResourceName ICON_SET = "{E23427CAC80DA8B7}UI/Textures/Icons/icons_mapMarkersUI.imageset";
	protected static const ResourceName ICON_GLOW_SET = "{67B3A6DC2D712B52}UI/Textures/Icons/icons_mapMarkersUI-glow.imageset";
	protected static const string ICON_IRP = "circle";
	protected static const string ICON_WP = "point-of-interest-2";
	protected static const string ICON_ORP = "flag-2";
	protected static const string HINT_TITLE = "Route Planner";
	protected static const int CLICK_THROTTLE_MS = 300;		// ignores the click that picked the radial entry
	protected static const int COLOR_ROUTE = 0xFF101010;
	protected static const int COLOR_DRAFT = 0xFFB03000;
	protected static const float DRAG_START_PX = 6;			// cursor travel that makes a held press a drag
	protected static const float POINT_HIT_PX = 16;
	protected static const float LINE_HIT_PX = 8;
	protected static const float ICON_SIZE_PX = 36;
	protected static const float LINE_WIDTH_PX = 2;
	protected static const float DASH_PX = 3;
	protected static const float DASH_STEP_PX = 8;
	protected static const float LABEL_OFFSET_PX = 18;		// half an icon, right of and level with its top
	protected static const int LABEL_FONT_SIZE = 14;

	protected static ref CGQC_RoutePlanner s_Instance;

	protected SCR_MapEntity m_MapEntity;
	protected SCR_MapCursorModule m_CursorModule;
	protected Widget m_wMapFrame;
	protected CanvasWidget m_wCanvas;
	protected ref array<ref CGQC_RoutePointWidgets> m_aPointWidgets = {};
	protected ref array<ref CanvasWidgetCommand> m_aCommands;
	protected ref array<int> m_aDraft = {};
	protected ref SCR_HintUIInfo m_Hint;
	protected string m_sDrawKey;
	protected int m_iDraftRevision;
	protected int m_iPointWidgetsUsed;
	protected int m_iPlanningStartTick;
	protected int m_iLastPlaceTick;
	protected int m_iMovedPoint = -1;		// draft point carried by the cursor, -1 = none
	protected int m_iMoveOriginX;
	protected int m_iMoveOriginZ;
	protected float m_fPressX;				// cursor position of the press that picked up a point
	protected float m_fPressY;
	protected bool m_bButtonHeld;			// left button held on a carried point
	protected bool m_bDropOnRelease;		// the held press is the one that drops the point
	protected bool m_bPressHandled;			// the press picked up or dropped a point, its click must not act again
	protected bool m_bPlanning;
	protected bool m_bMapOpen;

	//------------------------------------------------------------------------------------------------
	static CGQC_RoutePlanner GetInstance()
	{
		if (!s_Instance)
			s_Instance = new CGQC_RoutePlanner();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsPlanning()
	{
		return s_Instance && s_Instance.m_bPlanning;
	}

	//------------------------------------------------------------------------------------------------
	void OnMapOpen(notnull SCR_MapEntity mapEntity)
	{
		RemoveListeners();
		m_bPlanning = false;
		m_bMapOpen = false;
		m_iMovedPoint = -1;
		m_bButtonHeld = false;
		m_aDraft.Clear();
		m_aPointWidgets.Clear();	// widgets of a previous map layout are gone

		m_MapEntity = mapEntity;
		m_CursorModule = SCR_MapCursorModule.Cast(mapEntity.GetMapModule(SCR_MapCursorModule));

		Widget root = mapEntity.GetMapMenuRoot();
		if (!root || !m_CursorModule)
			return;

		m_wMapFrame = root.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!m_wMapFrame)
			return;

		m_wCanvas = CanvasWidget.Cast(GetGame().GetWorkspace().CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.BLEND, Color.FromRGBA(255, 255, 255, 255), 0, m_wMapFrame));
		if (!m_wCanvas)
			return;

		FrameSlot.SetAnchorMin(m_wCanvas, 0, 0);
		FrameSlot.SetAnchorMax(m_wCanvas, 1, 1);
		FrameSlot.SetOffsets(m_wCanvas, 0, 0, 0, 0);

		InputManager inputManager = GetGame().GetInputManager();
		inputManager.AddActionListener("MapSelect", EActionTrigger.UP, OnInputSelect);
		inputManager.AddActionListener("MapDrag", EActionTrigger.DOWN, OnInputPress);
		inputManager.AddActionListener("MapDrag", EActionTrigger.UP, OnInputRelease);

		m_sDrawKey = string.Empty;
		m_bMapOpen = true;
	}

	//------------------------------------------------------------------------------------------------
	void OnMapClose()
	{
		if (!m_bMapOpen)
			return;

		FinishPlanning();
		RemoveListeners();
		m_bMapOpen = false;

		foreach (CGQC_RoutePointWidgets widgets : m_aPointWidgets)
		{
			widgets.Remove();
		}
		m_aPointWidgets.Clear();

		if (m_wCanvas)
			m_wCanvas.RemoveFromHierarchy();

		m_aCommands = null;
	}

	//------------------------------------------------------------------------------------------------
	void Update()
	{
		// No input context is activated here: an exclusive one blocks clicks, pan and zoom of the map
		if (!m_bMapOpen)
			return;

		UpdateHold();
		UpdateMove();
		Redraw();
	}

	//------------------------------------------------------------------------------------------------
	void StartPlanning()
	{
		if (!m_bMapOpen)
			return;

		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager)
			return;

		if (manager.CGQC_CountRoutes(GetLocalFactionIndex()) >= CGQC_Route.MAX_ROUTES_PER_FACTION)
		{
			ShowHint(string.Format("Route limit reached (%1). Hover a route and press Delete to free a slot.", CGQC_Route.MAX_ROUTES_PER_FACTION), 10);
			return;
		}

		m_aDraft.Clear();
		m_iMovedPoint = -1;
		m_bPressHandled = false;
		m_iDraftRevision++;
		m_iPlanningStartTick = System.GetTickCount();
		m_bPlanning = true;

		ShowHint("Left-click: add a point. The first is the IRP, the last is the ORP.<br/>Move a point: drag it, or left-click it and left-click again to drop it. Right-click or Esc cancels the move.<br/>Right-click a point: remove it.<br/>Right-click empty map or Esc: finish.<br/>Delete a finished route by hovering it and pressing Delete.", 600);
	}

	//------------------------------------------------------------------------------------------------
	//! Ends planning and shares the route with the faction
	void FinishPlanning()
	{
		if (!m_bPlanning)
			return;

		CancelMove();
		m_bPlanning = false;

		if (m_Hint)
			SCR_HintManagerComponent.HideHint(m_Hint);

		if (!m_aDraft.IsEmpty())
		{
			SCR_MapMarkerSyncComponent sync = GetLocalSyncComponent();
			if (sync)
				sync.CGQC_AskAddRoute(m_aDraft);
		}

		m_aDraft.Clear();
		m_iDraftRevision++;
	}

	//------------------------------------------------------------------------------------------------
	//! Esc while planning: cancels a move in progress, otherwise finishes the route
	void OnEscape()
	{
		if (!CancelMove())
			FinishPlanning();
	}

	//------------------------------------------------------------------------------------------------
	//! Right-click while planning
	void OnContextClick()
	{
		if (CancelMove())
			return;

		int index = FindDraftPointUnderCursor();
		if (index < 0)
		{
			FinishPlanning();
			return;
		}

		m_aDraft.RemoveOrdered(index * 2 + 1);
		m_aDraft.RemoveOrdered(index * 2);
		m_iDraftRevision++;
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
	}

	//------------------------------------------------------------------------------------------------
	//! Delete key outside planning: removes the whole route under the cursor
	void DeleteRouteUnderCursor()
	{
		if (!m_bMapOpen || m_bPlanning)
			return;

		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		SCR_MapMarkerSyncComponent sync = GetLocalSyncComponent();
		if (!manager || !sync)
			return;

		float cursorX, cursorY;
		GetCursorScreenPosition(cursorX, cursorY);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float pointRange = workspace.DPIScale(POINT_HIT_PX);
		float lineRange = workspace.DPIScale(LINE_HIT_PX);

		int factionIndex = GetLocalFactionIndex();
		int closestId = -1;
		float closestDistance = float.MAX;

		foreach (CGQC_Route route : manager.CGQC_GetRoutes())
		{
			if (!route.IsVisibleTo(factionIndex))
				continue;

			float previousX, previousY;
			int pointCount = route.m_aCoords.Count() / 2;
			for (int i; i < pointCount; i++)
			{
				float x, y;
				GetPointScreenPosition(route.m_aCoords, i, x, y);

				float distance = GetSegmentDistance(cursorX, cursorY, x, y, x, y);
				if (distance <= pointRange && distance < closestDistance)
				{
					closestDistance = distance;
					closestId = route.m_iId;
				}

				if (i > 0)
				{
					distance = GetSegmentDistance(cursorX, cursorY, previousX, previousY, x, y);
					if (distance <= lineRange && distance < closestDistance)
					{
						closestDistance = distance;
						closestId = route.m_iId;
					}
				}

				previousX = x;
				previousY = y;
			}
		}

		if (closestId < 0)
			return;

		sync.CGQC_AskRemoveRoute(closestId);
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
	}

	//------------------------------------------------------------------------------------------------
	// Input
	//------------------------------------------------------------------------------------------------
	//! Whether a left click may act on the route right now
	protected bool CanUseLeftClick()
	{
		if (!m_bPlanning || System.GetTickCount() - m_iPlanningStartTick < CLICK_THROTTLE_MS)
			return false;

		if (m_CursorModule.GetCursorState() & (SCR_MapCursorModule.STATE_POPUP_RESTRICTED | EMapCursorState.CS_CONTEXTUAL_MENU | EMapCursorState.CS_DRAG))
			return false;

		return !GetGame().GetWorkspace().GetFocusedWidget();	// click on a map UI element
	}

	//------------------------------------------------------------------------------------------------
	//! Left button pressed (mouse only): starts a drag, or the press that drops a carried point.
	//! Not limited by the click duration of MapSelect.
	protected void OnInputPress(float value, EActionTrigger reason)
	{
		EndHold();	// in case the release was never received
		m_bPressHandled = false;

		if (!CanUseLeftClick())
			return;

		if (m_iMovedPoint >= 0)
			m_bDropOnRelease = true;
		else if (!TryPickUpPoint())
			return;

		GetCursorScreenPosition(m_fPressX, m_fPressY);
		m_bPressHandled = true;
		m_bButtonHeld = true;
		UpdateHold();
	}

	//------------------------------------------------------------------------------------------------
	//! Left button released: a press that travelled was a drag and drops the point,
	//! otherwise the point stays carried until the next click
	protected void OnInputRelease(float value, EActionTrigger reason)
	{
		if (!m_bButtonHeld)
			return;

		float cursorX, cursorY;
		GetCursorScreenPosition(cursorX, cursorY);
		float travel = GetSegmentDistance(cursorX, cursorY, m_fPressX, m_fPressY, m_fPressX, m_fPressY);
		bool drop = m_bDropOnRelease || travel > GetGame().GetWorkspace().DPIScale(DRAG_START_PX);

		EndHold();

		if (drop)
			DropPoint();
	}

	//------------------------------------------------------------------------------------------------
	//! While the button is held, the draw cursor state keeps the map from opening its selection box.
	//! Pan and zoom are not restricted by it.
	protected void UpdateHold()
	{
		// Only when not set yet: HandleDraw(true) during a pan or zoom would clear it
		if (m_bButtonHeld && (m_CursorModule.GetCursorState() & EMapCursorState.CS_DRAW) == 0)
			m_CursorModule.HandleDraw(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void EndHold()
	{
		m_bDropOnRelease = false;
		if (!m_bButtonHeld)
			return;

		m_bButtonHeld = false;
		if (m_CursorModule)
			m_CursorModule.HandleDraw(false);
	}

	//------------------------------------------------------------------------------------------------
	//! \return true when a point was picked up
	protected bool TryPickUpPoint()
	{
		int index = FindDraftPointUnderCursor();
		if (index < 0)
			return false;

		// The second half of a double click must not pick up the point just placed
		if (System.GetTickCount() - m_iLastPlaceTick < CLICK_THROTTLE_MS)
			return false;

		m_iMovedPoint = index;
		m_iMoveOriginX = m_aDraft[index * 2];
		m_iMoveOriginZ = m_aDraft[index * 2 + 1];
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_START);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void DropPoint()
	{
		UpdateMove();
		m_iMovedPoint = -1;
		m_iLastPlaceTick = System.GetTickCount();
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
	}

	//------------------------------------------------------------------------------------------------
	//! Short click: adds a point. Also picks up and drops points for a gamepad, which has no press event here.
	protected void OnInputSelect(float value, EActionTrigger reason)
	{
		if (m_bPressHandled)
		{
			m_bPressHandled = false;
			return;
		}

		if (!CanUseLeftClick())
			return;

		if (m_iMovedPoint >= 0)
		{
			DropPoint();
			return;
		}

		if (FindDraftPointUnderCursor() >= 0)
		{
			TryPickUpPoint();
			return;
		}

		if (m_aDraft.Count() >= CGQC_Route.MAX_POINTS * 2)
			return;

		int x, z;
		GetCursorMapPosition(x, z);
		m_aDraft.Insert(x);
		m_aDraft.Insert(z);
		m_iDraftRevision++;
		m_iLastPlaceTick = System.GetTickCount();
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps the carried point under the cursor, its labels and legs follow on redraw
	protected void UpdateMove()
	{
		if (m_iMovedPoint < 0)
			return;

		int x, z;
		GetCursorMapPosition(x, z);
		SetDraftPoint(m_iMovedPoint, x, z);
	}

	//------------------------------------------------------------------------------------------------
	//! Puts the carried point back where it was picked up
	//! \return false when no point is being moved
	protected bool CancelMove()
	{
		EndHold();

		if (m_iMovedPoint < 0)
			return false;

		SetDraftPoint(m_iMovedPoint, m_iMoveOriginX, m_iMoveOriginZ);
		m_iMovedPoint = -1;
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetDraftPoint(int index, int x, int z)
	{
		if (m_aDraft[index * 2] == x && m_aDraft[index * 2 + 1] == z)
			return;

		m_aDraft[index * 2] = x;
		m_aDraft[index * 2 + 1] = z;
		m_iDraftRevision++;
	}

	//------------------------------------------------------------------------------------------------
	//! Map cursor position in whole metres
	protected void GetCursorMapPosition(out int x, out int z)
	{
		float worldX, worldZ;
		m_MapEntity.GetMapCursorWorldPosition(worldX, worldZ);
		x = Math.Round(worldX);
		z = Math.Round(worldZ);
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveListeners()
	{
		InputManager inputManager = GetGame().GetInputManager();
		inputManager.RemoveActionListener("MapSelect", EActionTrigger.UP, OnInputSelect);
		inputManager.RemoveActionListener("MapDrag", EActionTrigger.DOWN, OnInputPress);
		inputManager.RemoveActionListener("MapDrag", EActionTrigger.UP, OnInputRelease);
	}

	//------------------------------------------------------------------------------------------------
	// Drawing
	//------------------------------------------------------------------------------------------------
	//! Rebuilds the draw commands only when the view, the faction or a route changed
	protected void Redraw()
	{
		if (!m_wCanvas)
			return;

		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager)
			return;

		int originX, originY;
		m_MapEntity.WorldToScreen(0, 0, originX, originY, true);

		float width, height;
		m_wCanvas.GetScreenSize(width, height);

		int factionIndex = GetLocalFactionIndex();
		string drawKey = string.Format("%1|%2|%3|%4|%5|%6|%7|%8", m_MapEntity.GetCurrentZoom(), originX, originY, width, height, manager.CGQC_GetRouteRevision(), m_iDraftRevision, factionIndex);
		if (drawKey == m_sDrawKey)
			return;

		m_sDrawKey = drawKey;
		m_aCommands = new array<ref CanvasWidgetCommand>();
		m_iPointWidgetsUsed = 0;

		foreach (CGQC_Route route : manager.CGQC_GetRoutes())
		{
			if (route.IsVisibleTo(factionIndex))
				DrawRoute(route.m_aCoords, route.m_iNumber, COLOR_ROUTE, width, height);
		}

		// Preview of the number, the server assigns the final one when the route is finished
		if (m_bPlanning)
			DrawRoute(m_aDraft, manager.CGQC_GetFreeRouteNumber(factionIndex), COLOR_DRAFT, width, height);

		for (int i = m_iPointWidgetsUsed; i < m_aPointWidgets.Count(); i++)
		{
			m_aPointWidgets[i].SetVisible(false);
		}

		m_wCanvas.SetDrawCommands(m_aCommands);
	}

	//------------------------------------------------------------------------------------------------
	protected void DrawRoute(notnull array<int> coords, int routeNumber, int color, float width, float height)
	{
		float previousX, previousY;
		int pointCount = coords.Count() / 2;
		for (int i; i < pointCount; i++)
		{
			float x, y;
			GetPointScreenPosition(coords, i, x, y);

			if (i > 0)
				AddDottedLine(previousX, previousY, x, y, color, width, height);

			if (x >= 0 && x <= width && y >= 0 && y <= height)
				AddPoint(coords, i, routeNumber, x, y, color);

			previousX = x;
			previousY = y;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Only the part of the leg inside the canvas gets dashes, so zooming in stays cheap
	protected void AddDottedLine(float startX, float startY, float endX, float endY, int color, float width, float height)
	{
		float deltaX = endX - startX;
		float deltaY = endY - startY;
		float length = Math.Sqrt(deltaX * deltaX + deltaY * deltaY);
		if (length < 1)
			return;

		float from = 0;
		float to = 1;
		if (!ClipAxis(startX, deltaX, width, from, to) || !ClipAxis(startY, deltaY, height, from, to))
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float step = workspace.DPIScale(DASH_STEP_PX);
		float dash = workspace.DPIScale(DASH_PX);
		float lineWidth = workspace.DPIScale(LINE_WIDTH_PX);
		float unitX = deltaX / length;
		float unitY = deltaY / length;
		float last = to * length;

		// Dashes are aligned on the leg start so they do not crawl while panning
		for (float distance = Math.Ceil(from * length / step) * step; distance < last; distance += step)
		{
			float dashEnd = Math.Min(distance + dash, length);

			LineDrawCommand line = new LineDrawCommand();
			line.m_iColor = color;
			line.m_fWidth = lineWidth;
			line.m_Vertices = new array<float>();
			line.m_Vertices.Insert(startX + unitX * distance);
			line.m_Vertices.Insert(startY + unitY * distance);
			line.m_Vertices.Insert(startX + unitX * dashEnd);
			line.m_Vertices.Insert(startY + unitY * dashEnd);
			m_aCommands.Insert(line);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool ClipAxis(float start, float delta, float limit, inout float from, inout float to)
	{
		if (Math.AbsFloat(delta) < 0.001)
			return start >= 0 && start <= limit;

		float enter = -start / delta;
		float leave = (limit - start) / delta;
		if (enter > leave)
		{
			float swapped = enter;
			enter = leave;
			leave = swapped;
		}

		from = Math.Max(from, enter);
		to = Math.Min(to, leave);
		return from < to;
	}

	//------------------------------------------------------------------------------------------------
	//! Icon centered on the point with its label on the right.
	//! Shape tells the role apart, the tint is a second cue (colors of the vanilla marker palette).
	protected void AddPoint(notnull array<int> coords, int index, int routeNumber, float x, float y, int labelColor)
	{
		if (m_iPointWidgetsUsed >= m_aPointWidgets.Count())
		{
			CGQC_RoutePointWidgets created = CreatePointWidgets();
			if (!created)
				return;

			m_aPointWidgets.Insert(created);
		}

		CGQC_RoutePointWidgets widgets = m_aPointWidgets[m_iPointWidgetsUsed];
		m_iPointWidgetsUsed++;

		string quad = ICON_WP;
		Color tint = new Color(1, 1, 1, 1);
		if (index == 0)
		{
			quad = ICON_IRP;
			tint = new Color(0.014, 0.445, 0.08, 1);	// green
		}
		else if (index == coords.Count() / 2 - 1)
		{
			quad = ICON_ORP;
			tint = new Color(0.761, 0.392, 0.078, 1);	// orange
		}

		if (widgets.m_sQuad != quad)
		{
			widgets.m_sQuad = quad;
			widgets.m_wGlow.LoadImageFromSet(0, ICON_GLOW_SET, quad);
			widgets.m_wIcon.LoadImageFromSet(0, ICON_SET, quad);
		}

		widgets.m_wIcon.SetColor(tint);
		widgets.m_wLabel.SetText(CGQC_Route.GetPointLabel(coords, index, routeNumber));
		widgets.m_wLabel.SetColor(Color.FromInt(labelColor));
		widgets.SetVisible(true);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float posX = workspace.DPIUnscale(x);
		float posY = workspace.DPIUnscale(y);
		FrameSlot.SetPos(widgets.m_wGlow, posX, posY);
		FrameSlot.SetPos(widgets.m_wIcon, posX, posY);
		FrameSlot.SetPos(widgets.m_wLabel, posX + LABEL_OFFSET_PX, posY - LABEL_OFFSET_PX);
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_RoutePointWidgets CreatePointWidgets()
	{
		CGQC_RoutePointWidgets widgets = new CGQC_RoutePointWidgets();
		widgets.m_wGlow = CreateIconWidget(1);
		widgets.m_wIcon = CreateIconWidget(2);
		widgets.m_wLabel = TextWidget.Cast(GetGame().GetWorkspace().CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.BLEND, Color.FromRGBA(255, 255, 255, 255), 3, m_wMapFrame));

		if (!widgets.m_wGlow || !widgets.m_wIcon || !widgets.m_wLabel)
		{
			widgets.Remove();
			return null;
		}

		widgets.m_wGlow.SetColor(Color.FromSRGBA(21, 29, 32, 155));	// dark halo of vanilla markers
		widgets.m_wLabel.SetFont(LABEL_FONT);
		widgets.m_wLabel.SetExactFontSize(LABEL_FONT_SIZE);
		widgets.m_wLabel.SetShadow(3, 0xFFFFFFFF);	// light halo, readable over dark terrain
		FrameSlot.SetSizeToContent(widgets.m_wLabel, true);

		return widgets;
	}

	//------------------------------------------------------------------------------------------------
	protected ImageWidget CreateIconWidget(int zOrder)
	{
		ImageWidget image = ImageWidget.Cast(GetGame().GetWorkspace().CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.BLEND | WidgetFlags.STRETCH, Color.FromRGBA(255, 255, 255, 255), zOrder, m_wMapFrame));
		if (!image)
			return null;

		FrameSlot.SetSize(image, ICON_SIZE_PX, ICON_SIZE_PX);
		FrameSlot.SetAlignment(image, 0.5, 0.5);
		return image;
	}

	//------------------------------------------------------------------------------------------------
	// Helpers
	//------------------------------------------------------------------------------------------------
	protected void GetPointScreenPosition(notnull array<int> coords, int index, out float x, out float y)
	{
		int screenX, screenY;
		m_MapEntity.WorldToScreen(coords[index * 2], coords[index * 2 + 1], screenX, screenY, true);
		x = screenX;
		y = screenY;
	}

	//------------------------------------------------------------------------------------------------
	protected void GetCursorScreenPosition(out float x, out float y)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		x = workspace.DPIScale(SCR_MapCursorInfo.x);
		y = workspace.DPIScale(SCR_MapCursorInfo.y);
	}

	//------------------------------------------------------------------------------------------------
	protected int FindDraftPointUnderCursor()
	{
		float cursorX, cursorY;
		GetCursorScreenPosition(cursorX, cursorY);

		int closest = -1;
		float closestDistance = GetGame().GetWorkspace().DPIScale(POINT_HIT_PX);

		int pointCount = m_aDraft.Count() / 2;
		for (int i; i < pointCount; i++)
		{
			float x, y;
			GetPointScreenPosition(m_aDraft, i, x, y);

			float distance = GetSegmentDistance(cursorX, cursorY, x, y, x, y);
			if (distance <= closestDistance)
			{
				closest = i;
				closestDistance = distance;
			}
		}

		return closest;
	}

	//------------------------------------------------------------------------------------------------
	//! Distance from a screen point to a segment (a point when both ends are equal)
	protected float GetSegmentDistance(float pointX, float pointY, float startX, float startY, float endX, float endY)
	{
		float deltaX = endX - startX;
		float deltaY = endY - startY;
		float lengthSq = deltaX * deltaX + deltaY * deltaY;

		float ratio = 0;
		if (lengthSq > 0)
			ratio = Math.Clamp(((pointX - startX) * deltaX + (pointY - startY) * deltaY) / lengthSq, 0, 1);

		float offsetX = pointX - (startX + deltaX * ratio);
		float offsetY = pointY - (startY + deltaY * ratio);
		return Math.Sqrt(offsetX * offsetX + offsetY * offsetY);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetLocalFactionIndex()
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		Faction faction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (!factionManager || !faction)
			return -1;

		return factionManager.GetFactionIndex(faction);
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_MapMarkerSyncComponent GetLocalSyncComponent()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return null;

		return SCR_MapMarkerSyncComponent.Cast(controller.FindComponent(SCR_MapMarkerSyncComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowHint(string description, float duration)
	{
		m_Hint = SCR_HintUIInfo.CreateInfo(description, HINT_TITLE, duration, EHint.UNDEFINED, EFieldManualEntryId.NONE, false);
		SCR_HintManagerComponent.ShowHint(m_Hint, false, true);
	}
}
