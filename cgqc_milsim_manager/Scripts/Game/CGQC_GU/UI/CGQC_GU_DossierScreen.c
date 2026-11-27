// Gestion d'unité - Dossier (paper). Phase 1c: the screen opens from OUVRIR MON DOSSIER with the
// member's name; Phase 1d adds the fiche, Registre, Grade & accès and « Modifier mes infos ».

class CGQC_GU_DossierScreen : CGQC_GU_PaperScreen
{
	protected string m_sUid;		// "" = own dossier
	protected int m_iRequestId;

	//------------------------------------------------------------------------------------------------
	void CGQC_GU_DossierScreen(string uid = "")
	{
		m_sUid = uid;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnBuildPaper()
	{
		SetHeader("DOSSIER DU PERSONNEL", "");
		CGQC_GU_Client.GetOnReply().Insert(OnReply);

		if (m_sUid.IsEmpty())
		{
			m_iRequestId = CGQC_GU_Client.Request(CGQC_GU_Requests.ME);
		}
		else
		{
			CGQC_GU_Payload p = new CGQC_GU_Payload();
			p.Set("uid", m_sUid);
			m_iRequestId = CGQC_GU_Client.RequestWith(CGQC_GU_Requests.MEMBER_GET, p);
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnTeardown()
	{
		CGQC_GU_Client.GetOnReply().Remove(OnReply);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnReply(int requestId, bool ok, string code, string payload)
	{
		if (requestId != m_iRequestId || !ok)
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromString(payload))
			return;

		CGQC_GU_Member m = new CGQC_GU_Member();
		if (!m.ReadFrom(ctx))
			return;

		SetHeader("DOSSIER DU PERSONNEL", m.displayName);
	}
}
