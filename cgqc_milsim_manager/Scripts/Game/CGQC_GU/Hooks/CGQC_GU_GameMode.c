// Gestion d'unité - game mode hooks (script-only modded class, no prefab change).
// Starts the server-side manager and forwards player connection events to it.

modded class SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	protected override void OnGameStart()
	{
		super.OnGameStart();

		if (Replication.IsServer())
			CGQC_GU_Manager.Start();
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnPlayerRegistered(int playerId)
	{
		super.OnPlayerRegistered(playerId);

		CGQC_GU_Manager mgr = CGQC_GU_Manager.Get();
		if (mgr && Replication.IsServer())
			mgr.OnPlayerRegistered(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerAuditSuccess(int iPlayerID)
	{
		super.OnPlayerAuditSuccess(iPlayerID);

		CGQC_GU_Manager mgr = CGQC_GU_Manager.Get();
		if (mgr && Replication.IsServer())
			mgr.OnPlayerAudited(iPlayerID);
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		CGQC_GU_Manager mgr = CGQC_GU_Manager.Get();
		if (mgr && Replication.IsServer())
			mgr.OnPlayerDisconnected(playerId);

		super.OnPlayerDisconnected(playerId, cause, timeout);
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnGameModeEnd(SCR_GameModeEndData endData)
	{
		super.OnGameModeEnd(endData);

		CGQC_GU_Manager mgr = CGQC_GU_Manager.Get();
		if (mgr && Replication.IsServer())
			mgr.SaveAll();
	}

	//------------------------------------------------------------------------------------------------
	//! World unload / server shutdown: final save, then release the manager.
	void ~SCR_BaseGameMode()
	{
		if (Replication.IsServer())
			CGQC_GU_Manager.Stop();
	}
}
