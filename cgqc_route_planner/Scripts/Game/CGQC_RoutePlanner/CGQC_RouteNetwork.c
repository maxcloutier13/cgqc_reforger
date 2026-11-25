// Route replication. Reuses the vanilla map marker components so no prefab or config override is needed:
// the manager (on the game mode) holds the routes, the sync component (on the player controller) sends requests.

//------------------------------------------------------------------------------------------------
modded class SCR_MapMarkerManagerComponent
{
	protected ref array<ref CGQC_Route> m_aCGQCRoutes = {};
	protected int m_iCGQCLastRouteId;		// server only
	protected int m_iCGQCRouteRevision;		// bumped on every change so the map knows to redraw

	//------------------------------------------------------------------------------------------------
	array<ref CGQC_Route> CGQC_GetRoutes()
	{
		return m_aCGQCRoutes;
	}

	//------------------------------------------------------------------------------------------------
	int CGQC_GetRouteRevision()
	{
		return m_iCGQCRouteRevision;
	}

	//------------------------------------------------------------------------------------------------
	int CGQC_CountRoutes(int factionIndex)
	{
		int count;
		foreach (CGQC_Route route : m_aCGQCRoutes)
		{
			if (route.m_iFactionIndex == factionIndex)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! Lowest number not used by a route this faction can see. Existing routes keep theirs when another is deleted.
	int CGQC_GetFreeRouteNumber(int factionIndex)
	{
		int number = 1;
		while (CGQC_IsRouteNumberUsed(number, factionIndex))
		{
			number++;
		}

		return number;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CGQC_IsRouteNumberUsed(int number, int factionIndex)
	{
		foreach (CGQC_Route route : m_aCGQCRoutes)
		{
			// A route without faction is visible to everyone, so it clashes with every faction
			if (route.m_iNumber == number && (factionIndex == -1 || route.IsVisibleTo(factionIndex)))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected CGQC_Route CGQC_FindRoute(int routeId)
	{
		foreach (CGQC_Route route : m_aCGQCRoutes)
		{
			if (route.m_iId == routeId)
				return route;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Server only
	void CGQC_AddRoute_S(int factionIndex, notnull array<int> coords)
	{
		if (!Replication.IsServer())
			return;

		int valueCount = coords.Count();
		if (valueCount < 2 || valueCount % 2 != 0 || valueCount > CGQC_Route.MAX_POINTS * 2)
			return;

		if (CGQC_CountRoutes(factionIndex) >= CGQC_Route.MAX_ROUTES_PER_FACTION)
			return;

		m_iCGQCLastRouteId++;
		int number = CGQC_GetFreeRouteNumber(factionIndex);

		CGQC_RpcDo_AddRoute(m_iCGQCLastRouteId, number, factionIndex, coords);	// server side
		Rpc(CGQC_RpcDo_AddRoute, m_iCGQCLastRouteId, number, factionIndex, coords);
	}

	//------------------------------------------------------------------------------------------------
	//! Server only. Anyone in the owning faction can delete a route.
	void CGQC_RemoveRoute_S(int routeId, int requesterFactionIndex)
	{
		if (!Replication.IsServer())
			return;

		CGQC_Route route = CGQC_FindRoute(routeId);
		if (!route || !route.IsVisibleTo(requesterFactionIndex))
			return;

		CGQC_RpcDo_RemoveRoute(routeId);	// server side
		Rpc(CGQC_RpcDo_RemoveRoute, routeId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void CGQC_RpcDo_AddRoute(int routeId, int number, int factionIndex, array<int> coords)
	{
		CGQC_Route route = new CGQC_Route();
		route.m_iId = routeId;
		route.m_iNumber = number;
		route.m_iFactionIndex = factionIndex;
		route.m_aCoords.Copy(coords);

		m_aCGQCRoutes.Insert(route);
		m_iCGQCRouteRevision++;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void CGQC_RpcDo_RemoveRoute(int routeId)
	{
		CGQC_Route route = CGQC_FindRoute(routeId);
		if (!route)
			return;

		m_aCGQCRoutes.RemoveItem(route);
		m_iCGQCRouteRevision++;
	}

	//------------------------------------------------------------------------------------------------
	// Join in progress
	override event protected bool RplSave(ScriptBitWriter writer)
	{
		if (!super.RplSave(writer))
			return false;

		writer.WriteInt(m_aCGQCRoutes.Count());
		foreach (CGQC_Route route : m_aCGQCRoutes)
		{
			writer.WriteInt(route.m_iId);
			writer.WriteInt(route.m_iNumber);
			writer.WriteInt(route.m_iFactionIndex);
			writer.WriteInt(route.m_aCoords.Count());
			foreach (int value : route.m_aCoords)
			{
				writer.WriteInt(value);
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override event protected bool RplLoad(ScriptBitReader reader)
	{
		if (!super.RplLoad(reader))
			return false;

		int routeCount;
		if (!reader.ReadInt(routeCount))
			return false;

		m_aCGQCRoutes.Clear();
		for (int i; i < routeCount; i++)
		{
			CGQC_Route route = new CGQC_Route();
			int valueCount;
			reader.ReadInt(route.m_iId);
			reader.ReadInt(route.m_iNumber);
			reader.ReadInt(route.m_iFactionIndex);
			if (!reader.ReadInt(valueCount))
				return false;

			for (int j; j < valueCount; j++)
			{
				int value;
				reader.ReadInt(value);
				route.m_aCoords.Insert(value);
			}

			m_aCGQCRoutes.Insert(route);
		}

		m_iCGQCRouteRevision++;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_MapMarkerSyncComponent
{
	//------------------------------------------------------------------------------------------------
	//! Called on the local player's controller
	void CGQC_AskAddRoute(notnull array<int> coords)
	{
		Rpc(CGQC_RpcAsk_AddRoute, coords);
	}

	//------------------------------------------------------------------------------------------------
	//! Called on the local player's controller
	void CGQC_AskRemoveRoute(int routeId)
	{
		Rpc(CGQC_RpcAsk_RemoveRoute, routeId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void CGQC_RpcAsk_AddRoute(array<int> coords)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (manager && coords)
			manager.CGQC_AddRoute_S(CGQC_GetOwnerFactionIndex(), coords);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void CGQC_RpcAsk_RemoveRoute(int routeId)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (manager)
			manager.CGQC_RemoveRoute_S(routeId, CGQC_GetOwnerFactionIndex());
	}

	//------------------------------------------------------------------------------------------------
	//! Server side: faction is resolved here, never trusted from the client
	protected int CGQC_GetOwnerFactionIndex()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!controller || !factionManager)
			return -1;

		Faction faction = SCR_FactionManager.SGetPlayerFaction(controller.GetPlayerId());
		if (!faction)
			return -1;

		return factionManager.GetFactionIndex(faction);
	}
}
