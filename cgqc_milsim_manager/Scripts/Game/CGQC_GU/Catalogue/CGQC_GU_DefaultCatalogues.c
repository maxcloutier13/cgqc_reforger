// Gestion d'unité - FIRST-BOOT SEED ONLY.
// These values are written once to $profile:CGQC_GestionUnite/config/*.json when a file is missing.
// From then on the JSON files are the only source of truth: edit them, not this file.
// Sources: Docs/01-Spec-GestionUnite.md and https://maxcorp.gitbook.io/cgqc. Anything the gitbook
// does not define yet is left empty and marked "a_venir"; nothing is invented.

class CGQC_GU_DefaultCatalogues
{
	//------------------------------------------------------------------------------------------------
	static CGQC_GU_Settings Settings()
	{
		return new CGQC_GU_Settings();
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_LabelItem Item(string id, string label)
	{
		CGQC_GU_LabelItem i = new CGQC_GU_LabelItem();
		i.id = id;
		i.label = label;
		return i;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Rank Rank(string id, string abbr, string label, string category, int order, bool honorary = false)
	{
		CGQC_GU_Rank r = new CGQC_GU_Rank();
		r.id = id;
		r.abbr = abbr;
		r.label = label;
		r.category = category;
		r.order = order;
		r.insignia = id;
		r.honorary = honorary;
		return r;
	}

	//------------------------------------------------------------------------------------------------
	static void Ranks(array<ref CGQC_GU_LabelItem> categories, array<ref CGQC_GU_Rank> ranks)
	{
		categories.Insert(Item("subalternes", "Subalternes"));
		categories.Insert(Item("sous_officiers", "Sous-officiers"));
		categories.Insert(Item("officiers", "Officiers"));
		categories.Insert(Item("etat_major", "État-major"));
		categories.Insert(Item("autre", "Autre"));

		ranks.Insert(Rank("cdt", "Cdt.", "Cadet", "subalternes", 1));
		ranks.Insert(Rank("sdt", "Sdt.", "Soldat", "subalternes", 2));
		ranks.Insert(Rank("cpl", "Cpl.", "Caporal", "subalternes", 3));
		ranks.Insert(Rank("cplc", "CplC.", "Caporal-chef", "subalternes", 4));
		ranks.Insert(Rank("sgt", "Sgt.", "Sergent", "sous_officiers", 5));
		ranks.Insert(Rank("adju", "Adju.", "Adjudant", "sous_officiers", 6));
		ranks.Insert(Rank("adjum", "AdjuM.", "Adjudant-maître", "sous_officiers", 7));
		ranks.Insert(Rank("adjuc", "AdjuC.", "Adjudant-chef", "sous_officiers", 8));
		ranks.Insert(Rank("slt", "SLt.", "Sous-lieutenant", "officiers", 9));
		ranks.Insert(Rank("lt", "Lt.", "Lieutenant", "officiers", 10));
		ranks.Insert(Rank("capt", "Capt.", "Capitaine", "officiers", 11));
		ranks.Insert(Rank("maj", "Maj.", "Major", "etat_major", 12));
		ranks.Insert(Rank("lcol", "LCol.", "Lieutenant-colonel", "etat_major", 13));
		ranks.Insert(Rank("col", "Col.", "Colonel", "etat_major", 14));
		ranks.Insert(Rank("vet", "Vét.", "Vétéran", "autre", 0, true));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Role Role(string id, string label, string minRank)
	{
		CGQC_GU_Role r = new CGQC_GU_Role();
		r.id = id;
		r.label = label;
		r.minRank = minRank;
		return r;
	}

	//------------------------------------------------------------------------------------------------
	//! Roles unlocked per rank (cumulative), spec §3.
	static void Roles(array<ref CGQC_GU_Role> roles)
	{
		roles.Insert(Role("rifleman_lat", "Rifleman avec AT jetable", "sdt"));

		roles.Insert(Role("grenadier", "Grenadier", "cpl"));
		roles.Insert(Role("at", "AT", "cpl"));
		roles.Insert(Role("combat_engineer", "Combat Engineer", "cpl"));
		roles.Insert(Role("combat_lifesaver", "Combat Lifesaver", "cpl"));
		roles.Insert(Role("gunner", "Gunner", "cpl"));
		roles.Insert(Role("driver", "Driver", "cpl"));

		roles.Insert(Role("machinegunner", "MachineGunner", "cplc"));
		roles.Insert(Role("marksman", "Marksman", "cplc"));
		roles.Insert(Role("engineer", "Engineer", "cplc"));
		roles.Insert(Role("medic", "Medic", "cplc"));

		roles.Insert(Role("2ic", "2iC", "sgt"));
		roles.Insert(Role("sniper", "Sniper", "sgt"));
		roles.Insert(Role("spotter", "Spotter", "sgt"));
		roles.Insert(Role("drone_operator", "Drone Operator", "sgt"));
		roles.Insert(Role("tankiste", "Tankiste", "sgt"));

		roles.Insert(Role("tl", "TL", "adju"));
		roles.Insert(Role("jtac", "JTAC", "adju"));
		roles.Insert(Role("pilote", "Pilote", "adju"));

		roles.Insert(Role("sl", "SL", "adjum"));

		roles.Insert(Role("instructeur", "Instructeurs", "adjuc"));

		roles.Insert(Role("moderateur", "Modérateur", "slt"));
		roles.Insert(Role("zeus_apprenti", "Zeus en apprentissage", "slt"));

		roles.Insert(Role("moderateur_instructeur", "Modérateur et Instructeur", "lt"));
		roles.Insert(Role("zeus_confirme", "Zeus confirmé", "lt"));

		roles.Insert(Role("moderateur_instructeur_chef", "Modérateur et Instructeur en chef", "capt"));
		roles.Insert(Role("zeus_chef", "Zeus en chef", "capt"));

		roles.Insert(Role("admin", "Admin", "maj"));

		roles.Insert(Role("pouet", "Pouet", "col"));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_AccessGroup Group(string id, string label, int level, array<string> rights)
	{
		CGQC_GU_AccessGroup g = new CGQC_GU_AccessGroup();
		g.id = id;
		g.label = label;
		g.level = level;
		g.rights.Copy(rights);
		return g;
	}

	//------------------------------------------------------------------------------------------------
	//! Spec §3. Right names are listed in CGQC_GU_Rights.
	static void AccessGroups(array<ref CGQC_GU_AccessGroup> groups)
	{
		groups.Insert(Group("commandement", "Commandement", 100, { "*" }));
		groups.Insert(Group("zeus", "Zeus", 80, { "operations.manage", "deployments.manage", "phase.change", "orbat.assign" }));
		groups.Insert(Group("moderateur", "Modérateur", 60, { "notice.edit", "orbat.assign" }));
		groups.Insert(Group("instructeur", "Instructeur", 40, { "sessions.create", "quals.evaluate_own" }));
		groups.Insert(Group("membre", "Membre", 0, new array<string>()));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Promotion Promotion(string from, string to, array<string> quals, int months, bool officers, bool automatic)
	{
		CGQC_GU_Promotion p = new CGQC_GU_Promotion();
		p.from = from;
		p.to = to;
		p.requiredQuals.Copy(quals);
		p.minMonthsInRank = months;
		p.officerDecision = officers;
		p.automatic = automatic;
		return p;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_AnyOf AnyOf(int count, array<string> quals)
	{
		CGQC_GU_AnyOf a = new CGQC_GU_AnyOf();
		a.count = count;
		a.quals.Copy(quals);
		return a;
	}

	//------------------------------------------------------------------------------------------------
	//! CGQC progression chart, spec §3.
	static void Promotions(array<ref CGQC_GU_Promotion> promotions)
	{
		promotions.Insert(Promotion("cdt", "sdt", { "Q-MB" }, 0, false, true));

		CGQC_GU_Promotion p = Promotion("sdt", "cpl", { "Q-SI", "Q-SB" }, 2, false, false);
		p.otherChecks.Insert(Item("vibe_check", "Vibe check"));
		promotions.Insert(p);

		promotions.Insert(Promotion("cpl", "cplc", { "Q-EF", "Q-NT" }, 0, true, false));

		p = Promotion("cplc", "sgt", { "QL-2iC" }, 0, true, false);
		p.anyOf.Insert(AnyOf(1, { "QSP1-MED", "QSP1-MG", "QSP1-DMR", "QSP1-ENG" }));
		promotions.Insert(p);

		p = Promotion("sgt", "adju", { "QL-TL" }, 0, true, false);
		p.anyOf.Insert(AnyOf(1, { "QSP2-SNI", "QSP2-UAV", "QSP2-CENT", "QSP2-GRIF", "QSP2-JTAC" }));
		promotions.Insert(p);

		promotions.Insert(Promotion("adju", "adjum", { "QL-SL" }, 0, true, false));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Qualification Qual(string code, string title, int pp, string levelRank, string group = "")
	{
		CGQC_GU_Qualification q = new CGQC_GU_Qualification();
		q.code = code;
		q.title = title;
		q.pp = pp;
		q.levelRank = levelRank;
		q.group = group;
		q.state = "a_venir";
		return q;
	}

	//------------------------------------------------------------------------------------------------
	//! Spec §8. Only Q-MB has content in the gitbook today; the others are "a_venir".
	static void Qualifications(array<ref CGQC_GU_Qualification> quals)
	{
		CGQC_GU_Qualification qmb = Qual("Q-MB", "Qualification militaire de base", 1, "cdt");
		qmb.state = "actif";
		qmb.description = "La qualification d'entrée du CGQC : setup fonctionnel, communication avec l'équipe et bases de survie sur le terrain.";
		qmb.exercises.Copy({ "Q-MB-E1", "Q-MB-E2", "Q-MB-E3" });
		qmb.criteria.Insert(Item("c1", "Avoir 16 ans et plus"));
		qmb.criteria.Insert(Item("c2", "Être capable de se connecter sur le serveur"));
		qmb.criteria.Insert(Item("c3", "Nom au format « Cdt. NomDeFamille »"));
		qmb.criteria.Insert(Item("c4", "Keybinds obligatoires configurés"));
		qmb.criteria.Insert(Item("c5", "Être capable de se soigner"));
		qmb.criteria.Insert(Item("c6", "Être capable de stabiliser un blessé"));
		qmb.criteria.Insert(Item("c7", "Parler en jeu, en direct ou sur la 343"));
		qmb.criteria.Insert(Item("c8", "Familier avec les spécificités du setup CGQC"));
		qmb.onSuccess.Copy({
			"Promotion automatique au grade de Soldat",
			"Nom en jeu changé en « Sdt. NomDeFamille »",
			"Qualification inscrite au dossier",
			"Nom changé sur Discord et ajout au groupe Soldats"
		});
		qmb.onFailure = "Reprise à une date ultérieure, une fois les critères remplis.";
		quals.Insert(qmb);

		quals.Insert(Qual("Q-SI", "Soldat individuel", 1, "sdt"));
		quals.Insert(Qual("Q-SB", "Soldat binôme", 1, "sdt"));
		quals.Insert(Qual("Q-EF", "Équipe de feu", 1, "cpl"));
		quals.Insert(Qual("Q-NT", "Navigation terrestre", 1, "cpl"));

		quals.Insert(Qual("QL-2iC", "Leader junior", 2, "cplc"));
		quals.Insert(Qual("QSP1-MED", "QSP1 MED", 2, "cplc", "QSP1"));
		quals.Insert(Qual("QSP1-MG", "QSP1 MG", 2, "cplc", "QSP1"));
		quals.Insert(Qual("QSP1-DMR", "QSP1 DMR", 2, "cplc", "QSP1"));
		quals.Insert(Qual("QSP1-ENG", "QSP1 ENG", 2, "cplc", "QSP1"));
		quals.Insert(Qual("QL-TL", "Team leader", 2, "sgt"));
		quals.Insert(Qual("QSP2-SNI", "QSP2 SNI", 2, "sgt", "QSP2"));
		quals.Insert(Qual("QSP2-UAV", "QSP2 UAV", 2, "sgt", "QSP2"));
		quals.Insert(Qual("QSP2-CENT", "QSP2 CENT", 2, "sgt", "QSP2"));
		quals.Insert(Qual("QSP2-GRIF", "QSP2 GRIF", 2, "sgt", "QSP2"));
		quals.Insert(Qual("QSP2-JTAC", "QSP2 JTAC", 2, "sgt", "QSP2"));

		quals.Insert(Qual("QL-SL", "Squad leader", 3, "adju"));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Exercise Exercise(string code, string title, string qual, string format, string place, string description)
	{
		CGQC_GU_Exercise e = new CGQC_GU_Exercise();
		e.code = code;
		e.title = title;
		e.qual = qual;
		e.required = true;
		e.format = format;
		e.place = place;
		e.description = description;
		return e;
	}

	//------------------------------------------------------------------------------------------------
	//! Q-MB exercises from the gitbook; all REQUIS for now (spec §8).
	static void Exercises(array<ref CGQC_GU_Exercise> exercises)
	{
		exercises.Insert(Exercise("Q-MB-E1", "Soins individuels", "Q-MB", "Solo / Binôme", "Map de training",
			"Gérer ses propres blessures selon le niveau de menace, en priorisant rapidement."));
		exercises.Insert(Exercise("Q-MB-E2", "Secours au blessé", "Q-MB", "Solo / Binôme", "Map de training, Medical Training",
			"Évaluer et stabiliser un camarade au sol : sécurité d'abord, MARCH de base, position du blessé."));
		exercises.Insert(Exercise("Q-MB-E3", "Communication", "Q-MB", "Binôme", "Map de training",
			"Utiliser le vive-voix et la 343 selon la situation, avec une information pertinente sur le bon canal."));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Distinction Distinction(string id, string label, string category, string kind, string repeat, string proposer = "", string approver = "")
	{
		CGQC_GU_Distinction d = new CGQC_GU_Distinction();
		d.id = id;
		d.label = label;
		d.category = category;
		d.kind = kind;
		d.repeat = repeat;
		d.proposer = proposer;
		d.approver = approver;
		return d;
	}

	//------------------------------------------------------------------------------------------------
	//! Spec §7 and the approved mockup. Ribbon colours, quotes and the proposer/approver of the
	//! entries left empty are filled in Phase 6 from the mockup data.
	static void Distinctions(array<ref CGQC_GU_LabelItem> categories, array<ref CGQC_GU_Distinction> list)
	{
		categories.Insert(Item("vaillance", "Vaillance"));
		categories.Insert(Item("bravoure", "Bravoure"));
		categories.Insert(Item("merite", "Mérite"));
		categories.Insert(Item("service_operationnel", "Service opérationnel"));
		categories.Insert(Item("service", "Service"));

		list.Insert(Distinction("croix_victoria", "Croix de Victoria", "vaillance", "medal", "leaf"));
		list.Insert(Distinction("etoile_vaillance_militaire", "Étoile de la vaillance militaire", "vaillance", "medal", "leaf"));
		list.Insert(Distinction("medaille_vaillance_militaire", "Médaille de la vaillance militaire", "vaillance", "medal", "leaf"));

		list.Insert(Distinction("croix_vaillance", "Croix de la vaillance", "bravoure", "medal", "leaf"));
		list.Insert(Distinction("etoile_courage", "Étoile du courage", "bravoure", "medal", "leaf"));
		list.Insert(Distinction("medaille_bravoure", "Médaille de la bravoure", "bravoure", "medal", "leaf"));

		CGQC_GU_Distinction d = Distinction("croix_service_meritoire", "Croix du service méritoire", "merite", "medal", "leaf", "officers", "commandement");
		d.quote = "Professionnalisme exceptionnel apportant un bénéfice considérable ou un grand honneur à l'unité.";
		d.details = "Contribution durable qui élève l'unité : programme de formation, SOP, organisation majeure.";
		list.Insert(d);

		d = Distinction("medaille_service_meritoire", "Médaille du service méritoire", "merite", "medal", "leaf", "sl", "commandement");
		d.quote = "Service exceptionnel rendu à l'unité.";
		d.details = "Service soutenu au-dessus des attentes du grade.";
		list.Insert(d);

		d = Distinction("mention_commandant", "Mention élogieuse du commandant", "merite", "bar", "listed", "tl", "commandement");
		d.medal = "silver_bar";
		d.quote = "Action ou contribution remarquable au sein de l'unité.";
		d.details = "Une action précise et remarquable pendant une opération ou une séance.";
		list.Insert(d);

		d = Distinction("mention_chef_etat_major", "Mention élogieuse du chef d'état-major", "merite", "bar", "listed", "officers", "etat_major");
		d.medal = "gold_bar_3_leaves";
		d.quote = "Action ou service d'une portée exceptionnelle pour l'ensemble de l'unité.";
		d.details = "Au-dessus de la mention du commandant : l'effet dépasse la section ou l'opération.";
		list.Insert(d);

		d = Distinction("mention_commandant_en_chef", "Mention élogieuse du commandant en chef", "merite", "pennant", "collective", "etat_major", "etat_major");
		d.ribbon.Copy({ "navy", "red", "sky_blue" });
		d.medal = "gold_bar_pennant_fleurdelys";
		d.quote = "Distinction collective décernée à un indicatif complet pour une action d'ensemble remarquable.";
		d.details = "Le fanion appartient à l'indicatif ; chaque membre présent ce jour-là la reçoit à son dossier.";
		list.Insert(d);

		list.Insert(Distinction("etoile_campagne_generale", "Étoile de campagne générale", "service_operationnel", "medal", "deployment", "zeus_deployment", ""));
		list.Insert(Distinction("medaille_service_general", "Médaille du service général", "service_operationnel", "medal", "leaf"));
		list.Insert(Distinction("medaille_maintien_paix", "Médaille canadienne du maintien de la paix", "service_operationnel", "medal", "leaf"));
		list.Insert(Distinction("medaille_service_operationnel", "Médaille du service opérationnel", "service_operationnel", "medal", "leaf"));

		list.Insert(Distinction("decoration_forces_canadiennes", "Décoration des Forces canadiennes", "service", "medal", "clasp"));
		list.Insert(Distinction("medaille_service_special", "Médaille du service spécial", "service", "medal", "plainbar"));
		list.Insert(Distinction("medaille_sacrifice", "Médaille du sacrifice", "service", "medal", "leaf"));
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Seat Seat(string callsign, string role, string binome = "")
	{
		CGQC_GU_Seat s = new CGQC_GU_Seat();
		s.callsign = callsign;
		s.role = role;
		s.binome = binome;
		return s;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Element Element(string id, string label)
	{
		CGQC_GU_Element e = new CGQC_GU_Element();
		e.id = id;
		e.label = label;
		return e;
	}

	//------------------------------------------------------------------------------------------------
	protected static CGQC_GU_Section Section(string id, string label, string letter, string kind, bool active = false)
	{
		CGQC_GU_Section s = new CGQC_GU_Section();
		s.id = id;
		s.label = label;
		s.letter = letter;
		s.kind = kind;
		s.activeByDefault = active;
		return s;
	}

	//------------------------------------------------------------------------------------------------
	//! Hoplite layout (Spartan, Typhon), spec §6, with binôme colours.
	protected static CGQC_GU_Section Hoplite(string id, string label, string letter, string kind, bool active)
	{
		CGQC_GU_Section s = Section(id, label, letter, kind, active);

		CGQC_GU_Element e = Element(letter + "0", letter + "0");
		e.seats.Insert(Seat(letter + "0-0", "SL", "white"));
		e.seats.Insert(Seat(letter + "0-1", "Médic", "white"));
		e.seats.Insert(Seat(letter + "0-2", "Ingénieur", "white"));
		s.elements.Insert(e);

		e = Element(letter + "1", letter + "1");
		e.seats.Insert(Seat(letter + "1-0", "TL", "red"));
		e.seats.Insert(Seat(letter + "1-1", "Fusilier", "red"));
		e.seats.Insert(Seat(letter + "1-2", "MG", "green"));
		e.seats.Insert(Seat(letter + "1-3", "Fusilier", "green"));
		s.elements.Insert(e);

		e = Element(letter + "2", letter + "2");
		e.seats.Insert(Seat(letter + "2-0", "TL", "blue"));
		e.seats.Insert(Seat(letter + "2-1", "Fusilier", "blue"));
		e.seats.Insert(Seat(letter + "2-2", "MG", "yellow"));
		e.seats.Insert(Seat(letter + "2-3", "Fusilier", "yellow"));
		s.elements.Insert(e);

		return s;
	}

	//------------------------------------------------------------------------------------------------
	//! Units, sections and seats, spec §6.
	static void Units(array<ref CGQC_GU_Unit> units)
	{
		CGQC_GU_Unit cgqc = new CGQC_GU_Unit();
		cgqc.id = "cgqc";
		cgqc.label = "CGQC";
		cgqc.fullName = "Corps de Garde du Québec";
		cgqc.patch = "cgqc";

		CGQC_GU_Section zeus = Section("zeus", "Zeus", "Z", "Haut commandement");
		zeus.command = true;
		cgqc.sections.Insert(zeus);

		cgqc.sections.Insert(Hoplite("spartan", "Spartan", "S", "Hoplite, 1re section", true));
		cgqc.sections.Insert(Hoplite("typhon", "Typhon", "T", "Hoplite, 2e section", false));

		CGQC_GU_Section s = Section("argos", "Argos", "R", "Recon");
		CGQC_GU_Element e = Element("R1", "Orion");
		e.seats.Insert(Seat("R1-1", "Sniper"));
		e.seats.Insert(Seat("R1-2", "Spotter"));
		s.elements.Insert(e);
		e = Element("R2", "Hades");
		e.seats.Insert(Seat("R2-1", "JTAC"));
		s.elements.Insert(e);
		e = Element("R3", "Cyclope");
		e.seats.Insert(Seat("R3-1", "Drone (senior)"));
		e.seats.Insert(Seat("R3-2", "Drone (junior)"));
		s.elements.Insert(e);
		cgqc.sections.Insert(s);

		s = Section("ares", "Arès", "A", "Armes lourdes");
		e = Element("A1", "Atlas");
		e.seats.Insert(Seat("A1-1", "Antichar"));
		e.seats.Insert(Seat("A1-2", "Assistant"));
		s.elements.Insert(e);
		e = Element("A2", "Achille");
		e.seats.Insert(Seat("A2-1", "Mitrailleur"));
		e.seats.Insert(Seat("A2-2", "Assistant"));
		s.elements.Insert(e);
		e = Element("A3", "Kronos");
		e.seats.Insert(Seat("A3-1", "Tireur"));
		e.seats.Insert(Seat("A3-2", "Commandant"));
		s.elements.Insert(e);
		cgqc.sections.Insert(s);

		s = Section("titan", "Titan", "B", "Blindés");
		e = Element("B1", "Cerbère");
		e.seats.Insert(Seat("B1-1", "Conducteur"));
		e.seats.Insert(Seat("B1-2", "Tireur"));
		e.seats.Insert(Seat("B1-3", "Commandant"));
		s.elements.Insert(e);
		e = Element("B2", "Centaure");
		e.seats.Insert(Seat("B2-1", "Conducteur"));
		e.seats.Insert(Seat("B2-2", "Tireur"));
		e.seats.Insert(Seat("B2-3", "Commandant"));
		s.elements.Insert(e);
		cgqc.sections.Insert(s);

		s = Section("phoenix", "Phoenix", "P", "Air");
		e = Element("P1", "Griffon");
		e.seats.Insert(Seat("P1-1", "Pilote"));
		e.seats.Insert(Seat("P1-2", "Équipage"));
		e.seats.Insert(Seat("P1-3", "Équipage"));
		s.elements.Insert(e);
		e = Element("P2", "Pégase");
		e.seats.Insert(Seat("P2-1", "Pilote"));
		e.seats.Insert(Seat("P2-2", "Équipage"));
		e.seats.Insert(Seat("P2-3", "Équipage"));
		s.elements.Insert(e);
		cgqc.sections.Insert(s);

		units.Insert(cgqc);

		// UPO: instructor seats I-1..I-4; student seats E-x have no fixed count and are created on assignment.
		CGQC_GU_Unit upo = new CGQC_GU_Unit();
		upo.id = "upo";
		upo.label = "UPO";
		upo.fullName = "Unité de Préparation Opérationnelle";
		upo.patch = "upo";
		s = Section("upo_instructeurs", "Instructeurs", "I", "Instruction", true);
		e = Element("I", "Instructeurs");
		e.seats.Insert(Seat("I-1", "Instructeur"));
		e.seats.Insert(Seat("I-2", "Instructeur"));
		e.seats.Insert(Seat("I-3", "Instructeur"));
		e.seats.Insert(Seat("I-4", "Instructeur"));
		s.elements.Insert(e);
		upo.sections.Insert(s);
		s = Section("upo_eleves", "Élèves", "E", "Instruction", true);
		upo.sections.Insert(s);
		units.Insert(upo);

		CGQC_GU_Unit grit = new CGQC_GU_Unit();
		grit.id = "grit";
		grit.label = "GRIT";
		grit.fullName = "Groupe de Réaction et d'Intervention Tactique";
		grit.patch = "grit";
		grit.state = "a_venir";
		units.Insert(grit);
	}

	//------------------------------------------------------------------------------------------------
	//! Seed members, written only when the members folder is empty (first boot).
	static void SeedMembers(array<ref CGQC_GU_Member> members)
	{
		CGQC_GU_Member maj = new CGQC_GU_Member();
		maj.uid = "d7e9113c-f075-41c5-a72a-9ee5187dc723";
		maj.uidLinked = true;
		maj.lastName = "Cloutier";
		maj.displayName = "Cloutier";
		maj.rank = "maj";
		maj.accessGroups.Insert("commandement");
		maj.enlistedOn = "2022";
		maj.createdAt = CGQC_GU_Time.NowUtc();
		CGQC_GU_RecordEntry e = maj.AddRecord("enlistment", "systeme");
		e.on = "2022";
		members.Insert(maj);
	}
}
