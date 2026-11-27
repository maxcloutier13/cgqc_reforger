// Gestion d'unité - client-side cache of what the server sent. The UI (Phase 1+) reads from here
// and listens to the invokers; it never decides permissions itself.

void CGQC_GU_ReplyMethod(int requestId, bool ok, string code, string payload);
typedef func CGQC_GU_ReplyMethod;
typedef ScriptInvokerBase<CGQC_GU_ReplyMethod> CGQC_GU_ReplyInvoker;

class CGQC_GU_LightState
{
	int m_iSerial = -1;
	string m_sPhase;
	bool m_bQualification;
	string m_sUnitStatus;
	string m_sNotice;
	int m_iOnline;
	int m_iFlags;	// what this player's UI may offer; the server checks every action again

	static const int CAN_MENU = 1;		// unit.menu: Gestion d'unité entry
	static const int CAN_NOTICE = 2;	// notice.edit
	static const int CAN_PHASE = 4;		// phase.change
	static const int CAN_CALENDAR = 8;	// calendar.edit

	//------------------------------------------------------------------------------------------------
	bool Can(int flag)
	{
		return (m_iFlags & flag) != 0;
	}
}

class CGQC_GU_Client
{
	protected static ref CGQC_GU_LightState s_LightState = new CGQC_GU_LightState();
	protected static ref ScriptInvokerVoid s_OnLightState = new ScriptInvokerVoid();
	protected static ref CGQC_GU_ReplyInvoker s_OnReply = new CGQC_GU_ReplyInvoker();
	protected static int s_iNextRequestId = 1;

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_LightState GetLightState()
	{
		return s_LightState;
	}

	//------------------------------------------------------------------------------------------------
	static ScriptInvokerVoid GetOnLightState()
	{
		return s_OnLightState;
	}

	//------------------------------------------------------------------------------------------------
	static CGQC_GU_ReplyInvoker GetOnReply()
	{
		return s_OnReply;
	}

	//------------------------------------------------------------------------------------------------
	//! Sends a request through the local player controller. Returns the request id (0 if not possible).
	static int Request(string action, string payload = "")
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return 0;

		int id = s_iNextRequestId;
		s_iNextRequestId++;
		pc.CGQC_GU_SendRequest(id, action, payload);
		return id;
	}

	//------------------------------------------------------------------------------------------------
	static int RequestWith(string action, CGQC_GU_Payload payload)
	{
		return Request(action, payload.ToJson());
	}

	//------------------------------------------------------------------------------------------------
	static void OnReply(int requestId, bool ok, string code, string payload)
	{
		CGQC_GU_Log.Info(string.Format("Réponse %1 : %2 (%3), %4 octets", requestId, ok, code, payload.Length()));
		s_OnReply.Invoke(requestId, ok, code, payload);
	}

	//------------------------------------------------------------------------------------------------
	static void OnLightState(int serial, string phase, bool qualification, string unitStatus, string notice, int online, int flags)
	{
		s_LightState.m_iSerial = serial;
		s_LightState.m_sPhase = phase;
		s_LightState.m_bQualification = qualification;
		s_LightState.m_sUnitStatus = unitStatus;
		s_LightState.m_sNotice = notice;
		s_LightState.m_iOnline = online;
		s_LightState.m_iFlags = flags;
		CGQC_GU_Log.Info(string.Format("État reçu #%1 : phase=%2 qualif=%3 statut=%4 en ligne=%5 droits=%6", serial, phase, qualification, unitStatus, online, flags));
		s_OnLightState.Invoke();
	}
}
