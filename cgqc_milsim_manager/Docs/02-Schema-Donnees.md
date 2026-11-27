# Gestion d'unité — data schema (proposal)

Status: proposal for the build plan's Phase 0. Field names may be adjusted, but the principles below are fixed.

## Principles

- **Storage:** JSON on the dedicated server, under the server profile folder, for example `$profile:CGQC_GestionUnite/`. Never on clients.
- **Schema version:** every file has `"schemaVersion": 1`. Loading an older file runs a migration step.
- **Stable IDs:** every object gets an `id` that is never reused.
  - Format: a prefix plus a short random value, e.g. `mbr_7f3a91`, `op_2026_014`, `dep_bouclier_nord`. Members use their Bohemia UID as the key.
- **Dates:** ISO 8601.
  - Dates alone: `2026-11-28`.
  - Timestamps: UTC, `2026-11-28T01:00:00Z`.
  - The UI shows Eastern time (HE).
- **No display text in data.**
  - Store codes and IDs: `"rank": "cpl"`, never "Cpl. Roussel".
  - Labels come from the catalogues.
- **Flat-table friendly:** each list maps to a future table (members, record_entries, qualifications, exercise results, distinctions, operations, deployments, trainings, orbat_snapshots).
- **Rules are data:** ranks, roles, access groups, promotion criteria, qualifications, exercises, distinctions and thresholds all live in catalogue files. They are editable without a code change.

## Files

```
$profile:CGQC_GestionUnite/
  config/
    settings.json          discord webhook, time zone, status thresholds (inactif=3, mia=5)
    ranks.json             ordered ladder (order: higher = more senior), categories, abbreviations, insignia id
    roles.json             role id -> label, min rank (roles unlocked per rank are derived from this file)
    access_groups.json     ordered by level, rights list
    promotions.json        per rank transition: required quals / anyOf groups / months / officer decision / automatic
    qualifications.json    code, title, pp, level rank, exercises[], criteria[], onSuccess, text
    exercises.json         code, title, qualification, required, description, text
    distinctions.json      id, category, kind (medal|bar|pennant), ribbon stripes, medal key, proposer rule,
                           approver, repeat rule (leaf|deployment|plainbar|clasp|once|collective), description
    units.json             CGQC/UPO/GRIT, sections, elements, seats (callsign, default role, binôme colour)
  members/
    <bohemiaUid>.json      one file per member (see below)
  operations/<id>.json     OPORD + participants + orbat snapshot
  deployments/<id>.json
  trainings/<id>.json
  proposals/<id>.json      distinction proposals (pending + decided)
  state/
    situation.json         current phase, unit status, change log
    notice.json            command notice
    news.json              unit news feed
    orbat_live.json        active sections, section renames, seat assignments (incl. offline)
    calendar.json          manual calendar events (Phase 2d)
```

## Member file

```json
{
  "schemaVersion": 1,
  "uid": "d7e9113c-f075-41c5-a72a-9ee5187dc723",
  "uidLinked": true,
  "profileName": "silent1",
  "lastName": "Cloutier",
  "displayName": "Cloutier",
  "rank": "maj",
  "accessGroups": ["commandement", "zeus"],
  "status": { "computed": "actif", "manual": "", "manualBy": "", "manualAt": "" },
  "enlistedOn": "2022-03-01",
  "createdAt": "2026-11-26T19:00:00Z",
  "lastSeenAt": "2026-11-26T21:30:00Z",
  "personal": { "birthDate": "1981-02-03", "birthPlace": "Québec", "bloodType": "A+", "bio": "Commandant du CGQC." },
  "instructorFor": [{ "qual": "Q-MB", "authorizedBy": "<uid>", "on": "2025-01-10" }],
  "qualifications": [{ "id": "qr_01", "qual": "Q-MB", "result": "passed", "on": "2022-03-20", "evaluator": "<uid>", "sessionId": "trn_..." }],
  "progress": [{ "qual": "Q-EF", "exercises": { "Q-EF-E1": "2026-11-15" }, "criteria": { "c3": true }, "notes": "…", "savedBy": "<uid>", "savedAt": "…" }],
  "exercises": [{ "code": "Q-MB-E1", "on": "2022-03-12", "instructor": "<uid>", "sessionId": "trn_..." }],
  "distinctions": [{ "id": "aw_01", "distinction": "ecg", "on": "2026-04-02", "deploymentId": "dep_glace_noire", "reason": "…", "approvedBy": "<uid>" }],
  "record": [{ "id": "rec_01", "type": "promotion", "on": "2026-11-25", "by": "<uid>", "data": { "rank": "cpl" }, "note": "" }],
  "stats": { "operations": 184, "deployments": 9, "sessions": 42, "studentsEvaluated": 118, "missedOps": 0, "lastOpId": "op_2026_013" }
}
```

- `record.type` is one of: `enlistment, promotion, operation, deployment, session, qualification, distinction, task, note, status`.
  - `status` (added in 1e) is written by the system on every manual status change. Its `data` is `{status, from, effective}`; staff can't add it by hand.
- `stats` values are derived and can be recomputed from the event files at any time.
- **Distinction devices are never stored.** Leaves, bars and clasps are computed from the award list plus the rules in `distinctions.json`. The long-service clasps are computed from `enlistedOn` and the thresholds in `settings.json` → `longService { firstAtYears: 1, bronzeEvery: 1, silverAt: 5, goldAt: 10 }`.

## Event files (summary)

- **Operation:** `id`, `number`, `status (draft|published|done)`, `header{…}`, `situation{enemy, friendly, attachments}`, `mission`, `execution{task, purpose, endState, concept, elementTasks[], coordination}`, `support[]`, `command{succession[], callsigns[], points[], passwords{}, signals[], pace[]}`, `deploymentId`, `participants[{uid, seat, role, team}]`, `orbatSnapshot{…}`, `createdBy`, `publishedAt`, `doneAt`.
- **Deployment:** `id`, `name`, `area`, `start`, `end`, `commander`, `status`, `notes`, `operationIds[]`, `members[]`.
- **Training:** `id`, `type (ftx|training)`, `qual?`, `exercises[]`, `instructor`, `place`, `date`, `status`, `attendance[{uid, present, exercises{code: passed}, result}]`.
- **Proposal:** `id`, `distinction`, `target{uid|callsign}`, `operationId?`, `proposedBy`, `on`, `reason`, `internalNote`, `status (pending|accepted|refused|returned)`, `decidedBy`, `decidedAt`.

## Adjustments made while building Phase 0 (2026-11-26, kept by the owner)

- **Renamed fields.** `auto` and `ref` are reserved words in Enforce Script, so `status.auto` became `status.computed` and `record[].ref` became `record[].data`.
- **No `null` values.** Empty values are `""`. The engine JSON writer can't write `null` for strings.
- **New member fields:** `createdAt` and `lastSeenAt` (UTC timestamps).
- **Partial dates allowed.** `enlistedOn` may be just a year (`"2022"`) when the exact date isn't known. Maj. Cloutier's seed uses this.
- **Ranks don't list their roles.** `ranks.json` no longer holds the roles each rank unlocks. They come from `roles.json` (`minRank`), so each fact lives in one place only.
- **Settings keys:** `inactiveAfterMissedOps`, `miaAfterMissedOps`, `autosaveSeconds`, `allowDevUid`, `devUidDelaySeconds`, `longService`.
- **Access groups** carry a `level`; the highest held is the one displayed. Each group's `rights` is a list of right names, and `"*"` means every right. The right names code checks for are `members.edit`, `access.manage`, `promotion.view`, `operations.manage`, `deployments.manage`, `phase.change`, `orbat.assign`, `notice.edit`, `sessions.create` and `quals.evaluate_own`.
- **Calendar (Phase 2d).** `state/calendar.json` = `{schemaVersion, events[]}`. Each event is `{id "evt_…", title, date "AAAA-MM-JJ", time "HH:MM" or "", note, yearly, createdBy, createdAt}`.
  - Dates and times are **Eastern time as entered**, display only, not UTC.
  - Manual entries are basic events only. Operations, trainings and deployments come from their own files.
  - Enlistment anniversaries and birthdays are computed from the dossiers and never stored.
- **Dev UIDs.** These are `dev-<profile name>` and are created only when the backend gives no UID: always outside a dedicated server, and on a dedicated server only if `settings.json` has `allowDevUid: true`.
