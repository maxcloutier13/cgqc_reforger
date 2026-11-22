//------------------------------------------------------------------------------------------------
//!
//! \Author: 		Njpatman - Modif by silent1
//! \Last Updated: 	2026-11-22
//! \Source: 		Coalition Squad Interface + CGQC
//!	\Description: 	Client world system feeding the HUD and nametags: the locally controlled character, yaw,
//! 				vehicle/mortar state and the sorted group member list.
//! 
//------------------------------------------------------------------------------------------------
modded class CSI_HUDSystem
{
	//------------------------------------------------------------------------------------------------
	//! Display order: SL, the SL's team, other teams, unassigned; each team's TL first.
	//! Modded team order: RED, GREEN, BLUE, YELLOW, NONE.
	//! Players without CSI data are left out.
	//! \param[in] playerIDs
	//! \return sorted player IDs
	override array<int> GetSortedGroupArray(array<int> playerIDs)
	{
		if (!m_PlayerDataManager)
			m_PlayerDataManager = CSI_PlayerDataManager.GetInstance();

		CSI_EColorTeam currentSLColorTeam = CSI_EColorTeam.NONE;

		foreach (PlayerId playerID : playerIDs)
		{
			CSI_PlayerData playerData = m_PlayerDataManager.GetPlayerData(playerID);

			if (!playerData)
				continue;

			if (playerData.GetIsSquadLeader())
				currentSLColorTeam = playerData.GetColorTeam();
		};

		// Team order, with the SL's own team moved to the front
		array<CSI_EColorTeam> playersCTArray = {CSI_EColorTeam.RED, CSI_EColorTeam.GREEN, CSI_EColorTeam.BLUE, CSI_EColorTeam.YELLOW, CSI_EColorTeam.NONE};
		playersCTArray.RemoveItemOrdered(currentSLColorTeam);
		playersCTArray.InsertAt(currentSLColorTeam, 0);

		// One number per member: the sort key, then their place in the group, so members with
		// the same key keep the group's order
		array<int> sortedKeys = {};
		foreach (int groupIndex, PlayerId playerID : playerIDs)
		{
			CSI_PlayerData playerData = m_PlayerDataManager.GetPlayerData(playerID);

			if (!playerData)
				continue;

			// Sort key: SL 1, then two per team in playersCTArray order, TL first
			int sortKey = 1;
			if (!playerData.GetIsSquadLeader())
			{
				int teamIndex = playersCTArray.Find(playerData.GetColorTeam());
				if (teamIndex < 0)
					teamIndex = playersCTArray.Count() - 1;

				sortKey = teamIndex * 2 + 3;
				if (playerData.GetIsTeamLeader())
					sortKey--;
			}

			sortedKeys.Insert(sortKey * SORT_KEY_STEP + groupIndex);
		};

		sortedKeys.Sort();

		array<int> playersGroupArray = {};
		foreach (int sortedKey : sortedKeys)
		{
			// The place in the group is what's left of the key
			playersGroupArray.Insert(playerIDs[sortedKey - (sortedKey / SORT_KEY_STEP) * SORT_KEY_STEP]);
		}

		return playersGroupArray;
	}
}