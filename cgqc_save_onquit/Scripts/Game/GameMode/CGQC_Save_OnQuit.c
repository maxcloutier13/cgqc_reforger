modded class SCR_RespawnSystemComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected_S(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected_S(playerId, cause, timeout);

		if (!Replication.IsServer())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		if (playerManager.GetPlayerCount() > 0)
			return;

		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
			return;

		saveManager.RequestSavePoint(ESaveGameType.AUTO);
		Print("[CGQC_Save_OnQuit] Last player disconnected — world save triggered.", LogLevel.NORMAL);
	}
}