//------------------------------------------------------------------------------------------------
//! One finished route. Points are stored as whole-metre world X/Z pairs: x0, z0, x1, z1, ...
class CGQC_Route
{
	static const int MAX_POINTS = 50;
	static const int MAX_ROUTES_PER_FACTION = 10;

	int m_iId;
	int m_iNumber;					//!< shown on the map (IRP-1), unique among the routes of a faction
	int m_iFactionIndex = -1;		//!< -1 = creator had no faction, visible to everyone
	ref array<int> m_aCoords = {};

	//------------------------------------------------------------------------------------------------
	bool IsVisibleTo(int factionIndex)
	{
		return m_iFactionIndex == -1 || m_iFactionIndex == factionIndex;
	}

	//------------------------------------------------------------------------------------------------
	//! First point is the numbered IRP, last one the ORP, everything between is WP1, WP2, ...
	static string GetPointName(int index, int pointCount, int routeNumber)
	{
		if (index == 0)
			return "IRP-" + routeNumber.ToString();

		if (index == pointCount - 1)
			return "ORP";

		return "WP" + index.ToString();
	}

	//------------------------------------------------------------------------------------------------
	//! Name and 8-digit grid, plus the route total (IRP) or azimuth and distance from the previous point
	static string GetPointLabel(notnull array<int> coords, int index, int routeNumber)
	{
		int x = coords[index * 2];
		int z = coords[index * 2 + 1];
		int pointCount = coords.Count() / 2;

		string label = GetPointName(index, pointCount, routeNumber) + "\n" + GetGrid(x, z);
		if (index == 0)
		{
			if (pointCount > 1)
				label += "\nTotal: " + GetTotalDistance(coords).ToString() + " m";

			return label;
		}

		float deltaX = x - coords[index * 2 - 2];
		float deltaZ = z - coords[index * 2 - 1];

		// Clockwise from map north (+Z)
		int azimuth = Math.Round(Math.Atan2(deltaX, deltaZ) * Math.RAD2DEG);
		azimuth = (azimuth + 360) % 360;

		return label + "\n" + PadNumber(azimuth, 3) + "°  " + GetLegDistance(coords, index).ToString() + " m";
	}

	//------------------------------------------------------------------------------------------------
	//! Flat distance in metres from the previous point
	static int GetLegDistance(notnull array<int> coords, int index)
	{
		float deltaX = coords[index * 2] - coords[index * 2 - 2];
		float deltaZ = coords[index * 2 + 1] - coords[index * 2 - 1];

		return Math.Round(Math.Sqrt(deltaX * deltaX + deltaZ * deltaZ));
	}

	//------------------------------------------------------------------------------------------------
	//! Sum of the displayed leg distances, so it matches what players add up themselves
	static int GetTotalDistance(notnull array<int> coords)
	{
		int total = 0;
		int pointCount = coords.Count() / 2;
		for (int i = 1; i < pointCount; i++)
		{
			total += GetLegDistance(coords, i);
		}

		return total;
	}

	//------------------------------------------------------------------------------------------------
	//! 8-digit grid (10 m resolution)
	static string GetGrid(int x, int z)
	{
		int gridX, gridZ;
		SCR_MapEntity.GetGridPos(Vector(x, 0, z), gridX, gridZ, 1, 4);

		return PadNumber(gridX, 4) + " " + PadNumber(gridZ, 4);
	}

	//------------------------------------------------------------------------------------------------
	protected static string PadNumber(int value, int digits)
	{
		string result = value.ToString();
		while (result.Length() < digits)
		{
			result = "0" + result;
		}

		return result;
	}
}
