// Gestion d'unité - « #gu fetch » (Phase 5a test tool, client side). Sends one real request through the
// player controller, as the screens do, then shows in the chat what came back: code, size, and whether
// the JSON reads back. On a dedicated server this crosses the network, which Workbench Play does not.

class CGQC_GU_ReplyProbe
{
	protected static int s_iWaiting;
	protected static string s_sAction;
	protected static bool s_bListening;

	//------------------------------------------------------------------------------------------------
	static int Send(string action, CGQC_GU_Payload p)
	{
		if (!s_bListening)
		{
			CGQC_GU_Client.GetOnReply().Insert(OnReply);
			s_bListening = true;
		}

		s_sAction = action;
		s_iWaiting = CGQC_GU_Client.RequestWith(action, p);
		return s_iWaiting;
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (requestId != s_iWaiting)
			return;

		s_iWaiting = 0;
		string text = string.Format("Réponse %1 %2 (%3), %4 octets", s_sAction, ok, code, payload.Length());
		if (!payload.IsEmpty())
		{
			JsonLoadContext load = new JsonLoadContext();
			bool parsed = load.LoadFromString(payload);
			text += string.Format(", JSON lu : %1", parsed);

			if (parsed && s_sAction.StartsWith("ops.") && s_sAction != CGQC_GU_OperationRequests.LIST)
			{
				CGQC_GU_Operation o = new CGQC_GU_Operation();
				if (load.ReadValue("o", o) && !o.id.IsEmpty())
				{
					text += string.Format(" · %1 %2 : %3 tâches, %4 soutiens, %5 indicatifs, %6 points, %7 signaux, mission %8 car.",
						o.number, o.header.name, o.execution.elementTasks.Count(), o.support.Count(), o.command.callsigns.Count(),
						o.command.points.Count(), o.command.signals.Count(), o.mission.Length());
				}
				else
				{
					text += " · aucun ordre";
				}
			}
		}

		CGQC_GU_Log.Info("#gu fetch : " + text);
		SCR_ChatPanelManager chat = SCR_ChatPanelManager.GetInstance();
		if (chat)
			chat.ShowHelpMessage(text);
	}
}
