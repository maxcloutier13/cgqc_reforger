# Gestion d'unité — phased build plan (APPROVED 2026-11-26)

Status: **approved as written by the owner on 2026-11-26**, including the four technical decisions below and v1 = Phases 0 to 5. Work proceeds phase by phase. Each phase ends with a test on a local dedicated server and the owner's go-ahead before the next one starts.

Inputs: `01-Spec-GestionUnite.md` (what to build) and `02-Schema-Donnees.md` (data layout). The mockup link is in the spec.

---

## Technical decisions (approved 2026-11-26)

1. **How to hook into the game without prefab overrides.**
   - Proposal: use **script-only `modded class`** extensions instead of editing prefabs or base configs.
     - `PauseMenuUI`: adds the overview panel and the GM-only entry.
     - `SCR_PlayerController`: client→server RPCs. A client can only call the server through an entity it owns.
     - Game mode start: `modded SCR_BaseGameMode` starts a script-only server manager. **No manager entity prefab** (owner decision 2026-11-26); the light state goes to clients through owner RPCs on `SCR_PlayerController`.
   - Modded classes chain with other mods instead of replacing them, so conflict risk is low. This is the only kind of change made to base-game behaviour.
2. **How our screens open.**
   - Option A (**chosen**): create our layouts directly as widgets and manage input focus ourselves. This needs no change to `chimeraMenus.conf`.
   - Option B (rejected): register real menus. This needs a config override of the menu preset list, which is a conflict risk, so I'd avoid it.
3. **Player identity:** `BackendApi.GetPlayerIdentityId` on the server.
   - It returns nothing in Workbench / Peer Tool without the backend.
   - Fallback for testing: a dev-only fake UID per player name, disabled on the real server.
4. **Storage:** JSON through the engine's JSON save/load contexts and `FileIO` under `$profile:`, on the server only. Clients receive only the data they are allowed to see.
5. **Discord:** an HTTP POST to the webhook from the server through the engine REST API. The URL lives in `config/settings.json` and is never sent to clients.

## Phase 0 — Foundations (no UI)

- Folder layout under `Scripts/Game/CGQC_GU/…`, `UI/…` and `Configs/…`. All classes use the `CGQC_GU_` prefix to avoid name clashes.
- **Storage service:**
  - Load and save every file from the schema.
  - `schemaVersion` with migration hooks.
  - Atomic writes: write a temporary file, then rename.
  - A periodic save, plus a save on server shutdown.
- **Catalogues:** ranks, roles, access groups, promotions, qualifications (Q-MB data from the gitbook; others marked "à venir"), exercises, distinctions, units and seats. All are loaded from config; none are hardcoded.
- **Player connection:** record the UID and profile name. If the UID is unknown, create a **Cadet** dossier and add an ENROLLMENT entry to the Registre.
- **Permissions:** server-side checks for every action, by access group. Highest group only is used for display.
- **RPC layer:** request → server check → reply or broadcast, plus replicated "light" state (phase, status, notice).
- **Seed data:** the initial roster import (`CGQC_GU_SeedRoster.c`, 18 members; owner decision 2026-11-26). It runs once per seed version, recorded in `state/seed.json`, and skips dossiers that already exist.
  - Maj., LCol. and Col. get Commandement; everyone else gets Membre.
  - Each imported member gets Q-MB on their join date.
  - Turcotte uses the UID confirmed by the owner (`dce42da0-…`); the cgqc_core conf has a typo. Deslauriers, Xang and Frechette are UID-less `mbr_…` dossiers to link later.
- **Done when:**
  - A player connecting creates `members/<uid>.json`.
  - A restart keeps all data.
  - Server log commands can list and edit members.

### Phase 0 — build status (2026-11-26)

- **Code:** `Scripts/Game/CGQC_GU/`, split into `Core/`, `Data/`, `Storage/`, `Catalogue/`, `Net/`, `Hooks/` and `Commands/`.
  - Compiles clean with `wb_validate_scripts` (0 errors) and `wb_reload` (Game module CRC32 `7151ea0a`).
  - **Not yet live-tested.**
- **Changes from the plan text, to confirm with the owner:**
  - **No manager entity prefab** (approved by the owner 2026-11-26). The light state (phase, Qualification, status, notice, players online) is pushed to each client through owner RPCs on `SCR_PlayerController`: on connection, and on every change. Nothing has to be spawned or registered, and there is no prefab or `.meta` to mint. The server-side manager is a script singleton started from `modded SCR_BaseGameMode.OnGameStart`.
  - **No file rename.** The engine has no rename call, so a safe write is: write `x.json.tmp`, check that it parses, keep the old file as `x.json.bak`, then copy the `.tmp` into place. Loading falls back to `.tmp`, then to `.bak`.
  - **Defaults are seeded from script.** On first boot the default catalogues are written to `$profile:CGQC_GestionUnite/config/*.json` from `CGQC_GU_DefaultCatalogues.c`. After that the JSON files are the only source of truth. A broken file is never overwritten; defaults are used in memory and an error is logged.
  - **Request payloads.** RPC requests carry a plain string argument. Replies carry JSON.
- **Admin console:** `#gu` in chat (logged-in admin) or over RCON.
  - Commands: `list`, `show <uid|nom>`, `set <uid|nom> rank|access|name|lastname|status|enlisted <valeur>`, `save`.
  - Every result is also printed to the log.
- **Identity:**
  - The UID is taken from `BackendApi` at registration and again at audit success.
  - If no UID arrives after `devUidDelaySeconds`, the dev UID `dev-<nom>` is used. This only happens outside a dedicated server, or when `allowDevUid` is true.
- **Gates for "Done when"** (Workbench Play test, 2026-11-26):
  - [x] On first boot, the config, state and seed member files are created under `profile/CGQC_GestionUnite/`.
  - [x] A restart keeps the data. `#gu set Cloutier rank cpl` survived Stop/Play, with a promotion entry in the Registre.
  - [x] `#gu list` / `show` / `set` / `save` work in chat.
  - [x] Connecting binds the player to their dossier.
    - The first test failed: the Workbench host registers before `OnGameStart`, so that event was missed.
    - Fix: the manager now picks up players already connected when it starts (Game module CRC32 `5c4afbac`).
    - Re-test passed. Workbench returned the real UID `d7e9113c-…`, which bound to the seeded Maj. Cloutier dossier.
    - `profileName` (`silent1`) and `lastSeenAt` were written, and the client received the light state.
  - [x] Roster import v1 (15:40, CRC32 `f6a523ee`): 17 dossiers added next to the existing one, 18 in total. Ranks match the confs, Majors are Commandement, Turcotte is linked, and Deslauriers, Xang and Frechette are not linked. `state/seed.json` was written.
  - [ ] Still to test with a second player: the path that creates a **new Cadet** dossier and its ENROLLMENT entry. This runs in the 2-client Peer Tool / dedicated-server test.

## Phase 1 — Pause menu + Dossier (core)

- **Pause menu:**
  - The CGQC logo.
  - The overview panel: header, command notice, unit news, next rank, and **OUVRIR MON DOSSIER**.
  - The operation section is added in Phase 5.
- **Dossier with the paper style:**
  - The member drop-down (sorted by rank, then last name).
  - The fiche: insignia (placeholder images), the highest access tag, the collapsible personal section, and the footer ID.
  - **Registre**, with manual entries.
  - **Grade & accès:** rank, access groups, display name, Bohemia ID link.
  - Self-edit of personal info.
- **Unité → Membres:**
  - Filters, automatic and manual status (KIA included), and "+ Nouveau dossier".
  - **Lier un profil:** type a UID, or merge an auto-created Cadet.
- Double-press confirmation on every write.
- **Done when:** Commandement can promote, set access, link a profile and edit any member, online or offline, and a plain member can read their own dossier and edit their personal info.

### Phase 1 — owner decisions (2026-11-26)

1. **Screens:** try `.layout` files first.
2. **Fonts:** use the closest fonts the game already has for now.
3. **Images:** I prepare the PNGs; the owner imports them as `.edds`.
4. **Who sees what:**
   - **Everyone can read every dossier, read-only.** Editing follows rights.
   - **Commandement:** everything.
   - **Instructeur:** starts trainings and gives the qualifications they are authorized for.
   - **Zeus is a designated access group**, assigned like the others.
     - It gives the Gestion d'unité entry in the Échap menu (right `unit.menu`), changing the phase, and all operation and deployment functions.
     - It has **no link to the game's Game Master role.** Players can still vote themselves GM on an empty map, and that must keep working untouched.
     - So wherever the spec says "GM only", read it as the Zeus group (or Commandement).
5. **Initial roster:** seed it from the AdminTools spreadsheet (join dates, Discord names) and the `cgqc_core/Configs/CGQC/*.conf` roster files (UIDs).
   - The conf ranks win over the spreadsheet's.
   - Members without a UID get a `mbr_…` key and the PROFIL NON LIÉ state.
   - The cgqc_core rosters stay in use for now; the milsim manager takes over later.

### Phase 1 — detailed breakdown

Each step ends with `wb_validate_scripts` + `wb_reload` and a short Play test before the next step starts.

**Questions to settle before any Phase 1 code**

1. **How screens are built.**
   - Option A (proposed): `.layout` files, one per screen, created with the enfusion-mcp layout tool. A one-time test layout in step 1b checks that Workbench registers them without a manual step.
   - Option B, if registration needs a manual step from you each time: build the widgets in script. Nothing to register, but harder to adjust visually.
2. **Fonts.** The game ships Roboto Condensed (dark screens) but not Tinos or Courier Prime (paper screens).
   - Option A (proposed): import both into the addon. They are free Google Fonts under the OFL licence. This needs a one-time import in Workbench.
   - Option B: use the closest fonts the game already has for v1.
3. **Images.** These need a one-time import in Workbench as `.edds`. I prepare the PNGs at the right sizes; you import them.
   - The CGQC logo, the UPO and GRIT patches, the round logos and the loading-screen background.
   - Placeholder rank insignia: simple drawn shapes until real images exist.
4. **Who sees other dossiers.**
   - Proposed: the member drop-down and other people's fiches are for holders of `members.edit` (Commandement).
   - Everyone else sees only their own dossier. That matches the Phase 1 "Done when".

**Steps**

- **1a — Server actions (no UI).** Each action checks rights on the server and writes the member file immediately.
  - Actions:
    - Self-edit of personal info.
    - Set rank: writes a PROMOTION entry and a news item.
    - Set access groups (`access.manage`).
    - Display name / use profile name.
    - Manual status (Actif / Inactif / MIA / KIA / auto).
    - Manual Registre entry.
    - + Nouveau dossier: a roster member who hasn't connected yet.
    - Lier un profil: type a UID, or merge an auto-created Cadet into an existing dossier.
  - Payloads become small JSON objects. Large replies (the member list) are split into chunks if needed.
  - **Test:** `#gu` debug calls plus a client request from the log.
- **1b — UI base.**
  - A screen base class: open/close as widgets on top of the game, mouse cursor, input focus, and Échap to close.
  - Dark and paper style helpers.
  - The double-press button component: red with « APPUYER DE NOUVEAU » on the first press, executes on the second, and resets after a few seconds.
  - **Test:** an empty test screen opens and closes cleanly in Play, and the game doesn't keep the inputs.
- **1c — Pause menu.**
  - `modded PauseMenuUI`, adding widgets the same way the game's own rally pause-menu code does.
  - The CGQC logo replaces the title.
  - The overview panel: header (rank and name, players online, phase and status), command notice, unit news, next rank with its criteria, and **OUVRIR MON DOSSIER**.
  - Seat, role and the operation section stay empty until Phases 4–5.
  - **Test:** the panel shows the live data; the vanilla buttons still work.
- **1d — Dossier (paper).**
  - The fiche: insignia, name, grade, highest access tag, online status, the collapsible personal section, and the footer `QC-<uid>-CGQC`.
  - Registre tab: list, plus a manual entry for staff.
  - Grade & accès tab: rank buttons, roles unlocked, next-promotion criteria, access checkboxes, display name, and the Profil Bohemia field with the LIÉ tag.
  - « Modifier mes infos » for the member.
  - The member drop-down for staff.
  - Every save uses the double press.
  - **Test:** with a member account and a Commandement account.
- **1e — Unité → Membres (paper).**
  - Columns available in Phase 1: online, name, rank, access, last connection, status. The instructor-for, seat and last-operation columns arrive with Phases 3–5.
  - Filters: Tous / En ligne / Actifs / Inactifs / MIA / KIA.
  - « + Nouveau dossier ».
  - The PROFIL NON LIÉ tag opens Lier un profil (UID or Cadet merge).
  - **Test:** the Phase 1 "Done when" on the Peer Tool with 2 clients. This also covers the remaining Phase 0 check, the new Cadet dossier.

## Phase 2 — Situation, calendar, news (GM)

- The **Gestion d'unité** entry (GM only).
- **Situation:** the 5 phases, the "Qualification" checkbox on Entraînement, Garnison / Déployé, and the change log. Changes are broadcast live to everyone.
- **Calendrier:** display only. It is built from operations, trainings and deployments once those exist; until then it uses manual entries.
- The unit news feed is generated automatically from promotions, qualifications, operations and the like.

## Phase 3 — Training and progression

- **Unité:** Liste des qualifications (with qualified instructors) and Liste des exercices.
- **Dossier:** Qualifications, Séances / FTX, Progression (partial save, exercises, criteria, pass/fail), Instructeur (per-qualification authorization, instructor list), Insignes.
- **Entraînement editor:** FTX or training, optional qualification, exercises covered, attendance with per-exercise results, and finishing the session into the records.
- **Promotion engine:** automatic Cdt. → Sdt. on Q-MB, and the "Promotion possible" flag (Commandement only). Everything is driven by `promotions.json`.

## Phase 4 — ORBAT

- CGQC / UPO tabs (GRIT left as a placeholder) with patches.
- Section activation (Spartan on by default) and rename.
- Seats and callsigns from `units.json`, binôme colours, HORS GRADE.
- **Seat assignment:** moves the player into the matching game group; offline players are queued for their next connection.
- UPO I-x / E-x seats, with the instructor-authorization check.

## Phase 5 — Operations and deployments

- **OPORD editor** with every tab:
  - The full En-tête.
  - Situation et mission, Exécution (element-task rows), Soutien (rows can be added).
  - Commandement et transmissions: succession, callsigns, points with "Utiliser ma position", signals and PACE.
  - Grey placeholder hints in empty fields.
- **Statuses** Brouillon › Publié › Terminée, with "Retour en brouillon".
- **Terminer et ajouter aux dossiers:** the participants dialog, Registre entries, missed-operation counting (Inactif / MIA), and the **ORBAT du jour** snapshot.
- The pause-menu **Opération en cours** panel: scrollable, in the approved layout.
- **Déploiements:** CRUD, linked operations, members, and close-out.

## Phase 6 — Distinctions

- **Unité → Liste des distinctions:** reference book, click to select, **Proposer**.
- **Propositions tab:** Commandement only; accept / refuse / send back, with an editable reason.
- **Dossier → Distinctions:** devices computed from the rules: leaves, a bar per deployment for the campaign star, plain bars, long-service clasps, commendation bars and the collective pennant. The fiche shows a ribbon rack.
- One-time distinctions can't be given twice. Zeus proposes the campaign medals when a deployment closes.

## Phase 7 — Discord

- "Publier sur Discord" for OPORD, deployments and trainings, through a server-side webhook with formatted embeds.

---

## Proposed v1

**Phases 0 to 5 (approved).** These cover member records, training, ORBAT and operations, which is the full loop of a unit night. Phases 6 and 7 would follow as v1.1.

**Later, as listed in the spec:** GRIT, platoon level, syncing two servers, the Discord RSVP bot, locked squads with automatic roles, fixed seat roles, kit and beard consequences, other languages, and real insignia and medal images.

## Housekeeping

- **Before publishing the mod to the Workshop,** remove the Workbench bridge scripts with `wb_cleanup` (`Scripts/WorkbenchGame/EnfusionMCP/`).
- **Workbench checks:** every phase ends with `wb_validate_scripts` + `wb_reload`, then a local dedicated-server test with 2 clients via the Peer Tool.
