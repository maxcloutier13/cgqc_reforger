// Gestion d'unité - Situation (Phase 2): phase, Qualification, unit status and the change log.
// Built on the server; display text is composed here from codes and never stored.

class CGQC_GU_SituationLogLine : Managed
{
	string when;	// "11-26 19:32" (MM-DD HH:MM, Eastern time)
	string text;	// "Phase : BRIEFING · Capt. Roy"
}

class CGQC_GU_SituationDto : Managed
{
	string phase;			// code: temps_libre | entrainement | briefing | operation | aar
	bool qualification;
	string unitStatus;		// code: garnison | deploye
	string since;			// "Depuis 19 h 32 · changé par Lt. Gagnon"
	string nextEvent;		// « PROCHAIN AU CALENDRIER » in the navigation
	int online;
	bool canPhase;			// display only; situation.set checks phase.change again
	ref array<string> phases = new array<string>();
	ref array<string> phaseLabels = new array<string>();
	ref array<string> phaseTexts = new array<string>();
	ref array<ref CGQC_GU_SituationLogLine> log = new array<ref CGQC_GU_SituationLogLine>();
}

class CGQC_GU_SituationBuilder
{
	static const int LOG_SENT = 60;	// newest entries sent to the screen

	//------------------------------------------------------------------------------------------------
	static void PhaseCodes(notnull array<string> codes)
	{
		codes.Insert("temps_libre");
		codes.Insert("entrainement");
		codes.Insert("briefing");
		codes.Insert("operation");
		codes.Insert("aar");
	}

	//------------------------------------------------------------------------------------------------
	static bool IsPhase(string code)
	{
		array<string> codes = new array<string>();
		PhaseCodes(codes);
		return codes.Contains(code);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsUnitStatus(string code)
	{
		return code == "garnison" || code == "deploye";
	}

	//------------------------------------------------------------------------------------------------
	static string PhaseName(string code)
	{
		switch (code)
		{
			case "temps_libre":		return "TEMPS LIBRE";
			case "entrainement":	return "ENTRAÎNEMENT";
			case "briefing":		return "BRIEFING";
			case "operation":		return "OPÉRATION";
			case "aar":				return "AAR";
		}

		return CGQC_GU_Str.Upper(code);
	}

	//------------------------------------------------------------------------------------------------
	//! Phase shown to players: a training with the Qualification box ticked reads QUALIFICATION alone.
	static string DisplayPhase(string code, bool qualification)
	{
		if (code == "entrainement" && qualification)
			return "QUALIFICATION";

		return PhaseName(code);
	}

	//------------------------------------------------------------------------------------------------
	//! Card text of each phase (approved mockup).
	static string PhaseText(string code)
	{
		switch (code)
		{
			case "temps_libre":		return "Par défaut au démarrage du serveur. Jeu libre, aucune activité organisée.";
			case "entrainement":	return "Séance d'entraînement, formation ou FTX en cours.";
			case "briefing":		return "Présentation de l'OPORD avant l'opération.";
			case "operation":		return "L'opération est lancée.";
			case "aar":				return "Revue après action (After Action Review).";
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static string UnitStatusName(string code)
	{
		if (code == "deploye")
			return "DÉPLOYÉ";

		return "GARNISON";
	}

	//------------------------------------------------------------------------------------------------
	//! "19:32" -> "19 h 32"
	static string FrenchTime(string hhmm)
	{
		if (hhmm.Length() < 5)
			return hhmm;

		return hhmm.Substring(0, 2) + " h " + hhmm.Substring(3, 2);
	}

	//------------------------------------------------------------------------------------------------
	static string Build(CGQC_GU_Manager mgr, CGQC_GU_Member viewer)
	{
		CGQC_GU_Situation s = mgr.m_Situation;
		CGQC_GU_SituationDto dto = new CGQC_GU_SituationDto();
		dto.phase = s.phase;
		dto.qualification = s.qualification;
		dto.unitStatus = s.unitStatus;
		dto.online = mgr.GetOnlineCount();
		dto.canPhase = CGQC_GU_Permissions.HasRight(mgr.m_Catalogues, viewer, CGQC_GU_Rights.PHASE_CHANGE);

		PhaseCodes(dto.phases);
		foreach (string code : dto.phases)
		{
			dto.phaseLabels.Insert(PhaseName(code));
			dto.phaseTexts.Insert(PhaseText(code));
		}

		dto.since = Since(mgr, s);
		dto.nextEvent = CGQC_GU_CalendarBuilder.NextLine(mgr);

		int sent;
		for (int i = s.log.Count() - 1; i >= 0 && sent < LOG_SENT; i--)
		{
			CGQC_GU_ChangeLogEntry e = s.log[i];
			CGQC_GU_SituationLogLine line = new CGQC_GU_SituationLogLine();
			string date, hhmm;
			CGQC_GU_Time.ToEastern(e.at, date, hhmm);
			line.when = CGQC_GU_OverviewBuilder.ShortDate(date) + " " + hhmm;
			line.text = ChangeText(e) + " · " + CGQC_GU_DossierBuilder.SignerName(mgr, e.by);
			dto.log.Insert(line);
			sent++;
		}

		CGQC_GU_JsonStringSaveContext ctx = new CGQC_GU_JsonStringSaveContext();
		ctx.WriteValue("s", dto);
		return ctx.Export();
	}

	//------------------------------------------------------------------------------------------------
	//! "Depuis 19 h 32 · changé par Lt. Gagnon", from the newest phase or Qualification entry.
	protected static string Since(CGQC_GU_Manager mgr, CGQC_GU_Situation s)
	{
		for (int i = s.log.Count() - 1; i >= 0; i--)
		{
			CGQC_GU_ChangeLogEntry e = s.log[i];
			if (e.field != "phase" && e.field != "qualification")
				continue;

			string date, hhmm;
			CGQC_GU_Time.ToEastern(e.at, date, hhmm);
			string text = "Depuis ";
			if (date != CGQC_GU_Time.TodayEastern())
				text += "le " + CGQC_GU_OverviewBuilder.ShortDate(date) + " ";

			text += FrenchTime(hhmm);
			if (e.by.IsEmpty() || e.by == "systeme")
				return text;

			return text + " · changé par " + CGQC_GU_DossierBuilder.SignerName(mgr, e.by);
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static string ChangeText(CGQC_GU_ChangeLogEntry e)
	{
		switch (e.field)
		{
			case "phase":
				return "Phase : " + PhaseName(e.to);

			case "qualification":
				if (e.to == "1")
					return "Qualification activée";

				return "Qualification désactivée";

			case "unitStatus":
				return "Statut : " + UnitStatusName(e.to);
		}

		return e.field + " : " + e.to;
	}
}
