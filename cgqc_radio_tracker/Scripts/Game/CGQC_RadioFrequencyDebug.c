// CGQC_RadioCheckAction.c

class CGQC_RadioCheckAction : SCR_ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BaseRadioComponent radio = BaseRadioComponent.Cast(pOwnerEntity.FindComponent(BaseRadioComponent));
		if (!radio || !radio.IsPowered())
			return;

		// --- Collect all unique frequencies from caller radio ---
		int countFreq = radio.TransceiversCount();
		if (countFreq == 0)
			return;

		array<int> freqs = new array<int>();

		for (int i = 0; i < countFreq; i++)
		{
			BaseTransceiver tsv = radio.GetTransceiver(i);
			if (tsv)
			{
				int f = tsv.GetFrequency();
				if (!freqs.Contains(f)) // prevent duplicates
					freqs.Insert(f);
			}
		}

		// --- Player manager setup ---
		PlayerManager playerMgr = GetGame().GetPlayerManager();
		if (!playerMgr)
			return;

		int callerPlayerId = playerMgr.GetPlayerIdFromControlledEntity(pUserEntity);
		PlayerController pc = playerMgr.GetPlayerController(callerPlayerId);
		if (!pc)
			return;

		SCR_PlayerController scrPC = SCR_PlayerController.Cast(pc);
		if (!scrPC)
			return;

		array<int> players = new array<int>();
		playerMgr.GetPlayers(players);

		// --- Build result ---
		string result = "";

		foreach (int freq : freqs)
		{
			float freqMHz = freq / 1000.0;
			string freqBlock = string.Format("\n\nFreq %1 MHz — listeners:", freqMHz);

			int matchCount = 0;

			foreach (int playerId : players)
			{
				IEntity playerEntity = playerMgr.GetPlayerControlledEntity(playerId);
				if (!playerEntity)
					continue;

				InventoryStorageManagerComponent invMgr = InventoryStorageManagerComponent.Cast(
					playerEntity.FindComponent(InventoryStorageManagerComponent)
				);
				if (!invMgr)
					continue;

				array<IEntity> items = new array<IEntity>();
				invMgr.GetItems(items);

				bool found = false;

				foreach (IEntity item : items)
				{
					if (!item || found)
						continue;

					BaseRadioComponent itemRadio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
					if (!itemRadio || !itemRadio.IsPowered())
						continue;

					int count = itemRadio.TransceiversCount();
					for (int i = 0; i < count; i++)
					{
						BaseTransceiver t = itemRadio.GetTransceiver(i);
						if (!t)
							continue;

						if (t.GetFrequency() == freq)
						{
							freqBlock = freqBlock + "\n" + playerMgr.GetPlayerName(playerId);
							matchCount++;
							found = true;
							break;
						}
					}
				}
			}

			if (matchCount == 0)
				freqBlock = freqBlock + "\n(none)";

			result = result + freqBlock;
		}

		// --- Send result to player ---
		scrPC.CGQC_RpcDo_ShowRadioCheckHint(result);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		PlayerManager playerMgr = GetGame().GetPlayerManager();
		if (!playerMgr)
			return false;

		int playerId = playerMgr.GetPlayerIdFromControlledEntity(user);
		PlayerController pc = playerMgr.GetPlayerController(playerId);
		if (!pc)
			return false;

		return pc.HasRole(EPlayerRole.GAME_MASTER);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void CGQC_RpcDo_ShowRadioCheckHint(string result)
	{
		SCR_HintManagerComponent.ShowCustomHint(result, "Radio Check", 10);
	}
}