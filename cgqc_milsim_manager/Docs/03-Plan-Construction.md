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
  - *Build status 2026-11-26:* written and passes `wb_validate_scripts`. Actions: `member.personal.set`, `.rank.set`, `.access.set`, `.name.set`, `.status.set`, `.record.add`, `member.create`, `member.link`, `member.merge`. Payload `{"p":{...}}`; merged dossiers are copied to `archive/` first; Commandement cannot remove its own Commandement. Test harness: `#gu as <membre> <action> clé=valeur`.
  - [x] Live test (16:01–16:02): create ×2, rank refused for a Sdt. and accepted for Commandement, own personal info OK, another member's personal info refused, KIA status, manual entry, link, and merge (Testeur into Cible: UID taken over, Registre merged, archive written, old file removed).
  - Fix after the test: a merge now also copies the auto dossier's personal info into the target's empty fields. Passes validation; not live-tested.
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
  - *Build status 2026-11-26:* [x] live test passed (CRC32 `99ffd7ea`). The shell fills the screen, the buttons click, the double press arms and resets, the server reply arrived (16:15), and Fermer and Échap close cleanly. First attempt was squashed in the corner: the layout root needs `FrameSlot` anchors 0,0–1,1, now set in `CGQC_GU_Screen.Build`.
    - **Screens live inside the pause menu** as widgets (`CGQC_GU_Ui` host + `CGQC_GU_Screen` / `CGQC_GU_PaperScreen`). The pause menu already provides the cursor and input blocking, so no input context and no menu registration are needed.
    - **Layouts:** `UI/layouts/CGQC_GU_Button`, `CGQC_GU_PaperShell` and `CGQC_GU_Text`. Their GUIDs were minted by Workbench through `wb_resources register`: the call reports "false" but writes the `.meta`.
    - **Fonts (owner decision: closest available):** Roboto Condensed Bold / Regular for titles and labels, Roboto Regular for values. The game ships no serif or monospace font that can be used.
    - **Temporary:** a pause button « ESSAI GESTION D'UNITÉ » opens `CGQC_GU_TestScreen`. Both are removed in 1c.
- **1c — Pause menu.**
  - `modded PauseMenuUI`, adding widgets the same way the game's own rally pause-menu code does.
  - The CGQC logo replaces the title.
  - The overview panel: header (rank and name, players online, phase and status), command notice, unit news, next rank with its criteria, and **OUVRIR MON DOSSIER**.
  - Seat, role and the operation section stay empty until Phases 4–5.
  - **Test:** the panel shows the live data; the vanilla buttons still work.
  - *Build status 2026-11-26:* **passed its live test** (see below).
    - New layout `CGQC_GU_Overview` and the logo `UI/Textures/CGQC_GU/CGQC_GU_Logo.edds`, imported by Workbench from the 512 px PNG of `cgqc_testing/CGQC_Logo_Shield2.png`.
    - New request `overview`, built on the server: name with rank, highest access, phase and status, players online, notice, next rank with criteria from `promotions.json`, and the last 6 news items. Display text is composed in the reply, never stored.
    - The notice bar is hidden when there is no notice. The operation panel reads « AUCUNE OPÉRATION EN COURS » until Phase 5.
    - OUVRIR MON DOSSIER opens `CGQC_GU_DossierScreen`, which shows the name only until 1d. The 1b test button and test screen are removed.
    - First Play test (17:16): the logo, the layout, the light state and the dossier button worked. The overview reply was ignored, and the dark panels rendered pale.
      - Cause of the ignored reply: in Workbench the reply arrived synchronously, inside `Request()`, before the caller stored its request id. Fix: local replies are now delivered on the next frame.
      - Panel colours are now set from script.
    - Second Play test (20:27, compile CRC bc5f4896): name and rank, access group, phase, players online and 4 news lines all show. The logo is now the round 128 px CGQC logo with transparency, sent by the owner, shown at 150 px.
      - The panels were still too faint: `Color.FromSRGBA` also linearises the alpha. Fix: `CGQC_GU_Style.Translucent` sets the opacity directly. Panels are now 0.72, raised from 0.55 for bright terrain.
      - Third test (20:30): the logo is fixed. The owner reimported it with Import Settings › Color Space = ToSRGB, which UI textures need. The panels were still pale: the UI blends in linear space, so a translucent dark veil darkens less than the mockup did. Fix: `Translucent` now converts the opacity, `1 - (1 - a)^2.2`. Panels are 0.65 in mockup terms; confirmed in the fourth test (20:34). The panel then moved left, starting 360 units from the left edge instead of 520, to close the gap next to the menu. **1c PASSED** (20:36): the owner approved the panel. The Dossier title showing the name was confirmed live (20:38).
- **1d — Dossier (paper).**
  - The fiche: insignia, name, grade, highest access tag, online status, the collapsible personal section, and the footer `QC-<uid>-CGQC`.
  - Registre tab: list, plus a manual entry for staff.
  - Grade & accès tab: rank buttons, roles unlocked, next-promotion criteria, access checkboxes, display name, and the Profil Bohemia field with the LIÉ tag.
  - « Modifier mes infos » for the member.
  - The member drop-down for staff.
  - Every save uses the double press.
  - **Test:** with a member account and a Commandement account.
  - *Build status 2026-11-26:* written, passes `wb_validate_scripts`, and loaded in Workbench (20:49, compile CRC32 `e4b081cd`). **Not yet live-tested.**
    - **New read action `dossier`** (`uid` optional, defaults to the caller), built on the server in `Net/CGQC_GU_Dossier.c`.
      - It returns display-ready fiche values, the Registre newest first, every rank with the current one flagged, the roles unlocked, the next-rank criteria with met / not met, and the access groups with held / not held.
      - It also returns viewer flags: `isSelf`, `canEdit` (members.edit) and `canAccess` (access.manage). These only decide which controls are shown; every write is still checked by its own action.
    - **Shared code:**
      - The next-rank criteria logic moved to `CGQC_GU_OverviewBuilder.BuildCriteria`, shared with the pause-menu panel. The panel text is unchanged.
      - `member.list` now also sends `rankAbbr` and `rankOrder`, and is sorted most senior rank first, then by name.
    - **Screen:**
      - Fiche on the left: drawn insignia, name, grade, access tag, presence dot, status, the collapsible section, and the footer.
      - Rail tabs REGISTRE and GRADE & ACCÈS.
      - Member selector (« MEMBRE ») at the top right, with search, online dots and pages of 10. Everyone can open it, since every member may read every dossier.
      - The Registre has pages of 10 and a « NOUVELLE INSCRIPTION » form for staff (type, optional date, text).
      - Grade & accès shows:
        - rank buttons with mini insignia;
        - roles unlocked;
        - next-rank criteria;
        - access checkboxes with ENREGISTRER LES ACCÈS;
        - displayed name and last name;
        - UTILISER LE NOM DU PROFIL;
        - Profil Bohemia with LIÉ / NON LIÉ, plus UID + LIER LE PROFIL when not linked.
      - Every save is a double press. After a save the dossier is read again, and refusals show as a red line.
    - **Owner's visual fixes:** PLAIN buttons are transparent with a thin ruled outline. Buttons inside the content use `SetCompact()`: a smaller label, sized to the text.
    - **Rank insignia** are drawn from rectangles (`CGQC_GU_Insignia`) using the `insignia` key in `ranks.json`. They are placeholders until real images exist.
    - **New layouts,** registered through `wb_resources`: `CGQC_GU_VBox`, `HBox`, `Sized`, `Canvas`, `Rect`, `Panel`, `Field`, `FieldMulti` and `Check`. `CGQC_GU_Button` gained 4 edge images for the outline.
    - **No scroll widget:** long lists use pages.
    - **First look by the owner (20:52), and the fixes made:**
      - The edit fields showed as small squares, and the insignia did not draw.
        - Cause: inside a SizeLayout, the child slot must be `AlignableSlot`, not `SizeLayoutSlot`.
        - Fixed in the `Sized`, `Canvas` and `FieldMulti` layouts. Fixed-width boxes are now left-aligned, so their width applies.
      - **The Modérateur access group is removed** (owner's request).
        - It is gone from the defaults and from the live `config/access_groups.json`, whose old copy is kept in the session scratchpad. The spec is updated too.
        - Dossiers that still list `moderateur` (Comeau) ignore it, and the next access save drops it.
        - The Modérateur *roles* unlocked by SLt./Lt./Capt. are separate and unchanged.
      - **Date display, owner's format:** short dates are MM-DD (pause-menu news), and full dates are YYMMDD (Registre, enlistment, last seen). Input fields still take AAAA-MM-JJ.
      - **Insignia simplified** to plain bars and squares, with no rotation. The owner will make real rank patches and distinctions with an external tool.
      - Compile CRC32 `7ab942a7` (20:56). The fixes have not been seen live yet.
    - **Second look by the owner (21:00), and the fixes made:**
      - Two rank buttons could be armed at once, and their red labels overlapped.
        - Rank buttons now only *pick* a rank, one at a time; picking another moves the selection.
        - A single double-press button, « ATTRIBUER LE GRADE : <abbr> », applies the pick. It is disabled while the pick is the current rank.
        - Throughout the UI, only one confirm button can be armed at a time: arming one disarms the previous.
      - **The Discord moderation roles are removed** (Modérateur; Modérateur et Instructeur; Modérateur et Instructeur en chef). They exist only on Discord, not in game.
        - They are gone from the defaults, the live `config/roles.json` and the spec table. Zeus, Admin and Pouet stay.
      - Compile CRC32 `2063fe67` (21:01). Not seen live yet.
    - **Third look by the owner (21:04), and the fixes made:**
      - **Rank buttons are back to a plain double press.** Click a rank and it turns red with « CONFIRMER », a short label so it fits the cell; click it again to apply. The separate « ATTRIBUER LE GRADE » button is removed.
        - The global "one armed button at a time" rule stays.
        - Buttons can now set their own armed label (`SetConfirmText`). The default is still « APPUYER DE NOUVEAU ».
      - **The pause menu's own buttons still reacted behind the paper screen;** a click reached « Redémarrer ».
        - Fix: while a screen is open, `CGQC_GU_Ui` hides every other child of the pause menu root (vanilla buttons and the overview panel) and clears the keyboard focus. Hidden widgets take no input. They are shown again when the screen closes.
      - Compile CRC32 `83d964e9` (21:05).
    - **1d PASSED** (2026-11-26, 21:08): the owner approved the Dossier after testing in Play. Next is 1e (Unité → Membres), waiting for the owner's go-ahead.
- **1e — Unité → Membres (paper).**
  - Columns available in Phase 1: online, name, rank, access, last connection, status. The instructor-for, seat and last-operation columns arrive with Phases 3–5.
  - Filters: Tous / En ligne / Actifs / Inactifs / MIA / KIA.
  - « + Nouveau dossier ».
  - The PROFIL NON LIÉ tag opens Lier un profil (UID or Cadet merge).
  - **Test:** the Phase 1 "Done when" on the Peer Tool with 2 clients. This also covers the remaining Phase 0 check, the new Cadet dossier.
  - *Build status 2026-11-26:* written, passes `wb_validate_scripts`, and loaded in Workbench (21:12, compile CRC32 `342f1834`). **Not yet live-tested.**
    - **Server, `member.list`:** each summary now also sends `accessLabel`, `lastSeen` (YYMMDD), `statusBy` (who set a manual status) and `autoCreated`. The reply also carries the rank list for pickers, the entry rank, and the viewer flag `canEdit` (members.edit). The flag only decides what the screen offers; every write is still checked by its own action.
      - `autoCreated` means a linked dossier whose ENRÔLEMENT is signed by the system with no note, i.e. made on first connection. Imported dossiers have the note « Import initial ».
    - **Screen `UI/CGQC_GU_MembersScreen.c`** (header UNITÉ / MEMBRES, rail MEMBRES and MON DOSSIER):
      - Filters with counts: Tous / En ligne / Actifs / Inactifs / MIA / KIA, plus a search field.
      - Table in pages of 12: online dot, name, grade, access, last connection (EN LIGNE or YYMMDD), status, profile. The name button opens that member's Dossier.
      - Staff only (`canEdit`):
        - « + NOUVEAU DOSSIER »: last name, Discord name, enlistment date (AAAA-MM-JJ), rank buttons (default Cadet), then CRÉER LE DOSSIER (double press).
        - The status cell is a button that opens STATUT: AUTOMATIQUE / ACTIF / INACTIF / MIA / KIA, each a double press (« CONFIRMER »). A manual status shows who set it.
        - The red PROFIL NON LIÉ tag opens LIER UN PROFIL: a Bohemia UID field with LIER LE PROFIL, or a list of auto-created Cadet dossiers, each with FUSIONNER (double press).
      - Plain members see the table read-only. PROFIL NON LIÉ is plain red text for them.
      - An open panel replaces the table, as the member selector does in the Dossier. ANNULER closes it.
    - **Entry point (temporary, until the Phase 2 menu):** a MEMBRES rail entry in the Dossier, shown only when the viewer has members.edit.
    - Screen switches go through `CGQC_GU_Ui.OpenNextFrame`, so the pressed button is not destroyed during its own click.
    - **First look by the owner (21:15), and the fixes made:**
      - **The rail stays the same on every paper screen:** REGISTRE, GRADE & ACCÈS, MEMBRES, FERMER. From Membres, REGISTRE and GRADE & ACCÈS open the viewer's own dossier on that tab.
      - **The status cell shows the status only** (« KIA »), never who set it.
      - **Each status change writes a Registre entry** of the new type `status` (STATUT). It records the signer and date, with `data` {status, from, effective}.
        - The text reads « Statut fixé : KIA » or « Statut automatique rétabli : ACTIF ».
        - Setting the status the member already has writes nothing.
        - STATUT is system-only: it is not in the manual entry types.
      - Compile CRC32 `517b8ab1` (21:18). Not seen live yet.
    - **Second look by the owner (21:21), and the fix made:**
      - **Status colour code:** ACTIF green, INACTIF yellow (dark, readable on parchment), MIA red, KIA black. It applies in the Membres status cell and the fiche's STATUT (`CGQC_GU_Style.StatusColor`, `CGQC_GU_Button.SetTextColor`).
      - The owner will reset all the data before the tool goes into use.
      - Compile CRC32 `01a42713` (21:22). The colours have not been seen live.
    - **1e PASSED** (2026-11-26, 21:22): the owner approved Membres and asked for the next step, given the colour code. Next is Phase 2.

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
