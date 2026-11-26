// Gestion d'unité - client <-> server RPC layer on the player controller (the only entity a client owns).
// Flow: client request -> server check (CGQC_GU_Requests) -> reply to the owner, or a broadcast of the light state.

modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	//! True when the owning client of this controller runs in this process (Workbench host, listen host).
	protected bool CGQC_GU_IsLocalOwner()
	{
		if (RplSession.Mode() == RplMode.None)
			return true;

		return GetRplIdentity() == RplIdentity.Local();
	}

	//------------------------------------------------------------------------------------------------
	// Client -> server
	//------------------------------------------------------------------------------------------------
	//! Called on the owning client. The payload is a small JSON object (may be empty).
	void CGQC_GU_SendRequest(int requestId, string action, string payload)
	{
		if (Replication.IsServer())
		{
			CGQC_GU_RpcAsk_Request(requestId, action, payload);
			return;
		}

		Rpc(CGQC_GU_RpcAsk_Request, requestId, action, payload);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void CGQC_GU_RpcAsk_Request(int requestId, string action, string payload)
	{
		CGQC_GU_Manager mgr = CGQC_GU_Manager.Get();
		if (!mgr)
		{
			CGQC_GU_SendReply(requestId, false, "not_ready", string.Empty);
			return;
		}

		mgr.HandleRequest(this, requestId, action, payload);
	}

	//------------------------------------------------------------------------------------------------
	// Server -> owner
	//------------------------------------------------------------------------------------------------
	void CGQC_GU_SendReply(int requestId, bool ok, string code, string payload)
	{
		if (CGQC_GU_IsLocalOwner())
		{
			CGQC_GU_RpcDo_Reply(requestId, ok, code, payload);
			return;
		}

		Rpc(CGQC_GU_RpcDo_Reply, requestId, ok, code, payload);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void CGQC_GU_RpcDo_Reply(int requestId, bool ok, string code, string payload)
	{
		CGQC_GU_Client.OnReply(requestId, ok, code, payload);
	}

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_SendLightState(int serial, string phase, bool qualification, string unitStatus, string notice, int online)
	{
		if (CGQC_GU_IsLocalOwner())
		{
			CGQC_GU_RpcDo_LightState(serial, phase, qualification, unitStatus, notice, online);
			return;
		}

		Rpc(CGQC_GU_RpcDo_LightState, serial, phase, qualification, unitStatus, notice, online);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void CGQC_GU_RpcDo_LightState(int serial, string phase, bool qualification, string unitStatus, string notice, int online)
	{
		CGQC_GU_Client.OnLightState(serial, phase, qualification, unitStatus, notice, online);
	}
}
