# Gestion d'unité — functional spec

Status: **mockup approved 2026-11-26**. This file records every decision taken while building the mockup, so a new session can build from it without the original conversation.

- Mockup canvas (private to the owner): https://claude.ai/artifact/XyxtNWguLbpMM9DYEHAQ1x
- Official unit documentation, the source of truth for ranks, roles, qualifications, exercises, composition and the OPORD template: https://maxcorp.gitbook.io/cgqc (index: `/llms.txt`; add `.md` to any page URL to get markdown). Re-read it before changing catalogue data. It is still being written; mark missing content as "à venir" and never invent it.
- Companion files: `02-Schema-Donnees.md` (data layout) and `03-Plan-Construction.md` (phased build plan).

---

## 1. Hard rules

- **In-game only.** No website and no external service, except the Discord webhook (one webhook to start, maybe categories later).
- **Standalone addon** with no dependencies. **No prefab overrides**; avoid anything that can conflict with other mods.
- **Server-side persistence**, one record per player keyed by Bohemia identity UID.
  - Records survive restarts.
  - Records can be viewed and edited for offline members.
- **Data format:** clean JSON, one file per member plus one file per category. Stable IDs, ISO dates, a schema version, and separate fields (never packed into display text). Files must be easy for an AI to digest; no export button. See `02-Schema-Donnees.md`.
- **UI language:** French by default, with standard military terms kept (OPORD, AAR, FTX). More languages later.
- **Text must be in-world:** no helper text that explains how the tool works.
- **Configurable rules:** promotion requirements, the rank ladder, role unlocks, qualifications, exercises, distinctions and thresholds are **editable data, never hardcoded**.
- **Confirm by pressing twice:** every button that saves, deletes, publishes, awards or merges turns red with "APPUYER DE NOUVEAU" on the first press. The second press executes it.
- **Build for one dedicated server first.** Syncing two servers comes later.

## 2. Visual identity

- **Dark screens** (pause menu, Situation, Calendrier, Gestion d'unité menu):
  - Background: the CGQC loading-screen image, darkened.
  - Panels: translucent dark; the pause-menu panels are roughly 55 % opaque.
  - Font: Roboto Condensed.
  - Accent: CGQC blue `#23439C`, with `#5B82E0` / `#8EAAF0` used on dark backgrounds.
- **Paper screens** (Dossier, Unité, Événements):
  - An aged "military personnel file" look: parchment background `#E8DCBB`, ruled boxes `#3A3123`.
  - Fonts: Tinos serif for labels and titles, Courier Prime for values.
  - Titles are large and dark.
  - Accents: CGQC blue for active and selected states, red `#8E2A22` for refusals and retakes.
- **Assets** (uploaded in the mockup; the in-game versions need `.edds` conversion):
  - CGQC, UPO and GRIT patches.
  - Small round logos: CGQC, UPO, and a plain fleur-de-lys.
  - The large CGQC logo: top-left of the pause menu, watermark on the fiche, header of the Gestion d'unité menu.
  - The loading-screen image.
  - Existing files to reuse: `cgqc_testing/CGQC_Logo_Shield2.png` and `cgqc_core/UI/Textures/Factions/*`.
- **Rank insignia and medals** are schematic drawings until real images exist. Rank insignia per rank:
  - Cdt.: blank slip-on
  - Sdt.: 1 chevron
  - Cpl.: 2 chevrons
  - CplC.: 2 chevrons + maple leaf
  - Sgt.: 3 chevrons + maple leaf
  - Adju.: crown
  - AdjuM.: crown + laurel
  - AdjuC.: arms
  - SLt., Lt., Capt.: 1, 2 or 3 Vimy stars
  - Maj.: crown
  - LCol.: crown + 1 star
  - Col.: crown + 2 stars + scarlet gorget
  - Vét.: to be defined

## 3. Ranks, roles, access

**Ranks** (abbreviation shown on the name tag, e.g. "Cpl. Roussel"):

| Category | Rank (abbr.) | Roles unlocked (cumulative) |
|---|---|---|
| Subalternes | Cadet (Cdt.) | — |
| | Soldat (Sdt.) | Rifleman with disposable AT |
| | Caporal (Cpl.) | Grenadier, AT, Combat Engineer, Combat Lifesaver, Gunner, Driver |
| | Caporal-chef (CplC.) | MachineGunner, Marksman, Engineer, Medic |
| Sous-officiers | Sergent (Sgt.) | 2iC, Sniper, Spotter, Drone Operator, Tankiste |
| | Adjudant (Adju.) | TL, JTAC, Pilote |
| | Adjudant-maître (AdjuM.) | SL |
| | Adjudant-chef (AdjuC.) | Instructeurs |
| Officiers | Sous-lieutenant (SLt.) | Zeus en apprentissage |
| | Lieutenant (Lt.) | Zeus confirmé |
| | Capitaine (Capt.) | Zeus en chef |
| État-major | Major (Maj.) | Admin |
| | Lieutenant-colonel (LCol.) | Admin |
| | Colonel (Col.) | "Pouet" |
| Autre | Vétéran (Vét.) | Honorary; set by hand |

- **Unit leader:** Maj. Cloutier, Bohemia UID `d7e9113c-f075-41c5-a72a-9ee5187dc723`, member since 2022.
- **Above rank:** GM or Commandement may give a player a role above their rank. It is shown as a **HORS GRADE** tag.
- **New players:** everyone becomes **Cadet** on first connection (the UID is recorded automatically).
- **Promotion criteria** follow the CGQC progression chart, stored as data:

| Promotion | Criteria |
|---|---|
| Cdt. → Sdt. | Q-MB. **Automatic** promotion. |
| Sdt. → Cpl. | Q-SI + Q-SB + 2 months + vibe check |
| Cpl. → CplC. | Q-EF + Q-NT + officers' decision |
| CplC. → Sgt. | QL-2iC + one QSP1 + officers' decision |
| Sgt. → Adju. | QL-TL + one QSP2 + officers' decision |
| Adju. → AdjuM. | QL-SL + officers' decision |

- **Access groups.** Assignable in combination, but **only the highest is shown** as a tag. Order: Commandement > Zeus > Instructeur > Membre. (Modérateur removed by the owner, 2026-11-26.)

| Group | Rights |
|---|---|
| Commandement | Everything, for everyone, including managing access groups. "Admin" is merged in here. |
| Zeus | Create operations and deployments, change the phase, assign ORBAT seats |
| Instructeur | Create sessions; evaluate their own qualifications |
| Membre | Default |

- **"Promotion possible"** (all criteria met except the officers' decision) is visible to Commandement only.

## 4. Navigation and who sees what

- **Pause menu.** The CGQC logo replaces the "ARMA REFORGER" title. Entries: Continuer, **Gestion d'unité** (GM only), Réapparaître, Redémarrer, Game Master, Menu des groupes, Paramètres, Sauvegarder et quitter.
- **Overview panel** (shown to everyone):
  - Header: rank and name in large type, seat and role, team leader, phase and status, number of players online.
  - Command notice (editable by Commandement).
  - **Opération en cours.** The middle section scrolls; the header and the OPORD link stay fixed. In order:
    1. Mission (highlighted).
    2. Forces ennemies | Forces alliées.
    3. A separator.
    4. Votre élément | Tâches.
    5. A separator.
    6. Fréquences | PACE (one line per letter), Signaux | Mots de passe, Points | Médical.
    7. The link to the full OPORD.
  - Right column: next on the calendar, last operation, unit news, your next rank (with criteria), and the **OUVRIR MON DOSSIER** button in CGQC blue. That button is how every player reaches their Dossier, opening on Registre.
- **Gestion d'unité menu** (GM only):
  - Situation
  - ÉVÉNEMENTS: Opérations, Déploiements, Entraînements, Calendrier
  - DOSSIER: Mon dossier
  - UNITÉ: Membres, ORBAT, Qualifications, Exercices, Distinctions
- **Side tabs on paper screens:**
  - ÉVÉNEMENTS: Opérations, Déploiements, Entraînements
  - DOSSIER: Registre, Grade & accès, Qualifications, Séances / FTX, Progression, Instructeur, Insignes, Distinctions
  - UNITÉ: Membres, ORBAT, Liste des qualifications, Liste des exercices, Liste des distinctions
  - Fermer

## 5. Screens

### Situation (GM)
- Phases: TEMPS LIBRE (the default at server start), ENTRAÎNEMENT (with a "Qualification" checkbox, unchecked by default), BRIEFING, OPÉRATION, AAR. Button: **Activer**.
- Unit status: GARNISON / DÉPLOYÉ. Every phase can happen in either status. In-game consequences (beards, etc.) come later.
- Change log: who changed what, and when.

### Calendrier
- Display only. Operations, trainings and deployment start/end each have their own colour, and a selected day shows its details.
- Time zone: Eastern (HE).
- Discord RSVP by reactions: later, and it needs a bot (a webhook can only post).

### Dossier (paper)
- **Member selector:** a drop-down sorted by rank (highest first), then last name, with a search field and online dots.
- **Fiche** (left card, on every Dossier screen):
  - Rank insignia, name, current callsign.
  - Grade.
  - **Accès:** the highest tag only.
  - Online status and tonight's seat.
  - A **collapsible "Renseignements et états de service"** section (closed by default) containing: Naissance, Type sanguin, Enrôlé le, Ancienneté, Opérations, Déploiements, Distinctions, Qualifications obtenues, Séances / FTX, Formations disponibles, Élèves évalués, and the Bio, with a "Modifier mes infos" button.
  - Insignes (qualification codes).
  - Distinctions as a ribbon rack with devices (scrolls when long; never a count in text).
  - Footer, centred: `QC-<uid>-CGQC`.
- **Personal info:** date and place of birth, blood type and bio are **editable by the member themselves**.
- **Registre:** entries of type PROMOTION / OPÉRATION / DÉPLOIEMENT / SÉANCE-FTX / QUALIFICATION / DISTINCTION / TÂCHE / NOTE / ENRÔLEMENT, with date and signer. Staff can add entries by hand.
- **Grade & accès:**
  - Display name, with "use profile name".
  - **Profil Bohemia:** an editable UID field with a LIÉ tag (Commandement).
  - Rank buttons with insignia.
  - Roles unlocked.
  - Next-promotion criteria with status.
  - Access-group checkboxes.
- **Qualifications:** each one with code, title, level, result (RÉUSSI / ÉCHEC / EN COURS), date and evaluator, plus the exercises passed.
- **Séances / FTX:** every session attended, with type, linked qualification, exercises and instructor.
- **Progression:**
  - The ladder grouped PP1 / PP2 / PP3, showing code, title, rank and status.
  - Clicking a qualification opens: exercises with checkboxes (REQUIS or not), the criteria checklist, notes, a partial save ("Enregistrer la progression"), and Réussir / Échouer. Pass/fail stays possible because sometimes it is the instructors' judgement.
- **Instructeur:** qualifications this member may teach. Authorization is **per qualification only**, never per exercise. The "Liste des instructeurs" dialog shows everyone and what they teach.
- **Insignes:** qualification badges in four states: obtenu, en cours, reprise, à venir.
- **Distinctions:** only those the member holds. See §7.

### Unité (paper)
- **Membres:**
  - Columns: online, name, rank, access, instructor-for, tonight's seat, last operation, operations missed, status.
  - **Status is automatic:** ACTIF; **INACTIF** after 3 missed OPORD operations; **MIA** after 5. Only OPORD operations count.
  - **Manual override** by Commandement: Actif / Inactif / MIA / **KIA**, shown with who set it.
  - Filters: Tous, En ligne, Actifs, Inactifs, MIA, KIA, ▲ Promotion possible (Commandement only).
  - "+ Nouveau dossier" creates a record for a roster member who has not connected yet.
  - **PROFIL NON LIÉ** tag opens the link dialog.
- **Lier un profil** (Commandement), two ways:
  - Type the UID.
  - Or **merge** a Cadet profile that was created automatically on first connection into an existing dossier (double press to confirm).
- **ORBAT:** see §6.
- **Liste des qualifications:**
  - Grouped PP1 / PP2 / PP3.
  - The detail panel shows the description, exercises (REQUIS or not; practice-only exercises will exist later), criteria as bullets, what happens on success, the full text, and **qualified instructors** (with online status and students evaluated).
- **Liste des exercices:** code, title, qualification, description, Requis column, full text.
- **Liste des distinctions:**
  - A reference book with one tab per category and large entries: ribbon + medal, quoted description, details, Proposée par, Approuvée par, Répétitions, and the count at CGQC.
  - **Click an entry to select it**, then **Proposer** (member or callsign + reason).
  - The **Propositions** tab (Commandement): pending proposals, an editable reason, an internal note, and Accepter et décerner / Renvoyer pour précision / Refuser, plus a history of decisions.

### Événements
- **OPORD:**
  - Status: **BROUILLON › PUBLIÉ › TERMINÉE**. "Retour en brouillon" is possible from PUBLIÉ.
  - Tabs: En-tête, 1 Situation et mission, 2 Exécution, 3 Soutien, 4 Commandement et transmissions, ORBAT du jour.
  - **En-tête:** name, number, issuing HQ, date, briefing time, launch time, time zone (**Heure de l'Est by default**), area of operations, map/terrain, time of day/weather.
  - **Situation et mission:** forces ennemies, forces alliées, attached elements, **mission (large field)**.
  - **Exécution:** commander's intent (Tâche / But / État final), concept of operations, tasks per element (rows; element drop-down), coordinating instructions.
  - **Soutien:** a list of support items (type + details), with rows that can be added.
  - **Commandement et transmissions,** three sub-pages:
    - Succession and callsigns: succession picked from online players, **starting with Spartan's SL or a TL, never Zeus**; callsign / element / leader (online players) / MHz rows.
    - Points: name / grid / note, with "Utiliser ma position".
    - Signaux et PACE: challenge, password, running password, near recognition; smoke / flares / brevity rows; PACE P/A/C/E with means, switch condition and MHz. **No "TUNE" buttons.**
  - Empty fields show a pale grey hint of what goes in them.
  - **"Terminer et ajouter aux dossiers"** opens the participants dialog:
    - Everyone online is pre-ticked; untick absent players. Each person's position comes from the ORBAT and can be edited. Offline members can be added.
    - It writes a Registre entry for each participant, sets the OPORD to TERMINÉE, and **saves a read-only ORBAT snapshot of that day** with the operation.
  - "Publier sur Discord" posts through the webhook.
- **Déploiement:**
  - Status BROUILLON › ACTIF › TERMINÉ.
  - Fields: name, area, start, end, commander, notes; tabs Opérations / Membres. Start and end appear on the calendar.
  - Ending a deployment is where Zeus proposes the campaign medals.
- **Entraînement:**
  - Type FTX or ENTRAÎNEMENT; status BROUILLON › PLANIFIÉE › TERMINÉE.
  - Fields: instructor, place, **linked qualification (optional)**, **exercises covered** (a session can cover only Q-MB-E1, without the whole qualification).
  - Attendance with a checkbox per exercise and RÉUSSI / ÉCHOUÉ / — for the qualification.
  - Finishing it writes to Séances / FTX, Qualifications and the Registre.

## 6. ORBAT

- **Unit selector**, with patches:
  - **CGQC** (Corps de Garde du Québec).
  - **UPO** (Unité de Préparation Opérationnelle): instructor seats I-1 to I-4, plus students E-1, E-2 and so on with no fixed count. Only members authorized for a qualification can take an instructor seat.
  - **GRIT** (Groupe de Réaction et d'Intervention Tactique): to be defined later.
- **Sections:**
  - Each section has an **"activate" checkbox**; active sections move to the top. By default **only Spartan is active**.
  - Every section can be **renamed**.
  - Zeus (high command) is shown apart from the sections and is not part of the field chain.
- **Reading groups and seats:**
  - The ORBAT reads the game's own groups.
  - Assigning a seat moves the player into the matching group (later, into the slot too).
  - Offline players get the seat on their next connection.
  - Seat roles are open for now; fixed seats come later, with an override.

| Section | Seats (callsign — role) |
|---|---|
| **Spartan** (Hoplite, 1st section), 11 seats | S0-0 SL, S0-1 Médic, S0-2 Ingénieur · S1-0 TL, S1-1 Fusilier, S1-2 MG, S1-3 Fusilier · S2-0 TL, S2-1 Fusilier, S2-2 MG, S2-3 Fusilier |
| **Typhon** (Hoplite, 2nd section) | Same layout with T |
| **Argos** (Recon), letter R | R1 Orion: R1-1 Sniper, R1-2 Spotter · R2 Hades: R2-1 JTAC · R3 Cyclope: R3-1 Drone (senior), R3-2 Drone (junior) |
| **Arès** (Heavy weapons), letter A | A1 Atlas: A1-1 Antichar, A1-2 Assistant · A2 Achille: A2-1 Mitrailleur, A2-2 Assistant · A3 Kronos: A3-1 Tireur, A3-2 Commandant |
| **Titan** (Armour), letter B | B1 Cerbère: B1-1 Conducteur, B1-2 Tireur, B1-3 Commandant · B2 Centaure: same |
| **Phoenix** (Air), letter P | P1 Griffon: P1-1 Pilote, P1-2 Équipage, P1-3 Équipage · P2 Pégase: same |

- **Binôme colours** (Spartan and Typhon only): a marker beside the seat, text stays readable.
  - SL element: white.
  - Team 1: red (TL + x-1), green (MG + x-3).
  - Team 2: blue, yellow.
  - The colour belongs to the binôme, not the team. No colours for any other group.

## 7. Distinctions

- **Display:** one card per distinction, never duplicated. Repeats are shown with devices, and every award is listed under the card with its date and reason.

**Catalogue**

| Category | Distinctions |
|---|---|
| VAILLANCE | Croix de Victoria; Étoile de la vaillance militaire; Médaille de la vaillance militaire |
| BRAVOURE | Croix de la vaillance; Étoile du courage; Médaille de la bravoure |
| MÉRITE | Croix du service méritoire; Médaille du service méritoire; Mention élogieuse du commandant (silver bar); Mention élogieuse du chef d'état-major (gold bar, 3 leaves); **Mention élogieuse du commandant en chef** (collective, awarded to a callsign: a pennant on a gold bar with navy / red / sky-blue bands and a gold fleur-de-lys drawn from the CGQC logo) |
| SERVICE OPÉRATIONNEL | Étoile de campagne générale; Médaille du service général; Médaille canadienne du maintien de la paix; Médaille du service opérationnel |
| SERVICE | Décoration des Forces canadiennes; Médaille du service spécial; Médaille du sacrifice |

- **Ribbon colours, medal descriptions and who may propose each one** are in the mockup data. They should be stored in the distinctions config:
  - Proposée par: TL+, SL+, officers, Zeus at deployment close, or État-major only.
  - Approuvée par: Commandement, or État-major for the top distinctions.
- **Medals with ribbons:**
  - **Leaf device on the ribbon:** 1 award = bare; 2 = silver leaf; 3 = gold; 4 = red; beyond that, combinations of leaves.
  - **Full medal:** one bar on the suspension ribbon for each extra award, the first earned closest to the medal.
- **Étoile de campagne générale:**
  - **Each award is a deployment.** The medal counts as the first; the first bar is the second deployment.
  - No theatres.
- **Médaille du service spécial:** one medal; each extra service adds a **plain bar** (no text).
- **Décoration des Forces canadiennes (long service):**
  - Awarded at **1 year**; the ribbon counts as the first year.
  - Then a **bronze clasp for each year**.
  - **Silver clasp at 5 years** (replaces the bronzes).
  - **Gold clasp at 10 years** (replaces silver and bronzes).
  - The thresholds are data values.
- **Mentions élogieuses:** no medal and no ribbon. The bar insignia is shown once, with the dates listed under it.
- **Collective commendation:** the pennant is shown once per award, tied to the callsign and the operation, with the subtitle "MENTION COLLECTIVE".
- **One-time distinctions:** an award to someone who already holds it is refused. There is no "une seule fois" text.

## 8. Qualifications and exercises (from the gitbook)

**PP1 — Infanterie**

| Rank | Qualification |
|---|---|
| Cadet | Q-MB Militaire de base |
| Soldat | Q-SI Soldat individuel, Q-SB Soldat binôme |
| Caporal | Q-EF Équipe de feu, Q-NT Navigation terrestre |

**PP2 — Spécialistes**

| Rank | Qualification |
|---|---|
| CplC. | QL-2iC Leader junior; QSP1-x, at least 1 of 4 (MED / MG / DMR / ENG) |
| Sgt. | QL-TL Team leader; QSP2-x, at least 1 of 5 (SNI / UAV / CENT / GRIF / JTAC) |

**PP3 — Commandement**

| Rank | Qualification |
|---|---|
| Adju. | QL-SL Squad leader |

- **Qualifications after Cpl.** can be taken without holding the rank, but they are still listed by rank.
- **Q-MB exercises:** E1 Soins individuels, E2 Secours au blessé, E3 Communication. **All REQUIS for now.**
- **Q-MB criteria** (checklist):
  - 16 ans et plus
  - connexion au serveur
  - nom « Cdt. NomDeFamille »
  - keybinds obligatoires
  - se soigner
  - stabiliser un blessé
  - parler en jeu (direct / 343)
  - familier avec le setup CGQC
- **Practice-only exercises** (not required) will be added later for other qualifications.

## 9. Discord

- "Publier sur Discord" on OPORD, Déploiement and Entraînement posts to one webhook, configured server-side. Categories may come later.
- **Calendar RSVP by reactions is a later phase** and needs a Discord bot.

## 10. Later / out of scope for now

- GRIT content.
- Platoon level.
- Syncing two servers.
- Discord RSVP bot.
- Locked squads, with automatic role assignment on spawn.
- Fixed seat roles.
- Kit locking.
- Consequences of Garnison / Déployé (beards, etc.).
- Other languages.
- Field Manual, Slideshow, Tutorial.
- Real images for insignia and medals.
- Vétéran insignia.
