# Ranked code-first backlog — 2026-10-17

**Provisional while exhaustive requirement review continues.** Engineering is owned by the current completion effort; Gate numbers identify historical specification ownership, not requests for more conversations.

Ranking uses dependency breadth, gameplay impact, data risk, dependency depth, effort (estimates, not measurements).

| Order | ID | Priority | Category | Finding / next action |
|---|---|---|---|---|
| 1 | CF-001 | P0 | CODE | **DONE (2026-10-17)** - Restore the existing read-only preflight behind the persistence boundary; validate commands and current core/session without changing approved UI. |
| 2 | CF-003 | P0 | TESTING | **DONE (2026-10-17)** - Give each run a unique receipt path/token and bind source/binary hashes; preserve earlier receipt; test stale-result rejection. |
| 3 | CF-017 | P1 | TESTING | Trace trusted receipt ingestion/mount/rollback and stable content provider APIs without modifying Registry. |
| 4 | CF-007 | P1 | TESTING | Map individual combat tests and trace production entry to both executors; do not rebuild existing turn core. |
| 5 | CF-015 | P1 | TESTING | Trace territory UI commands to services and bind each frozen invariant to evidence. |
| 6 | CF-006 | P1 | CODE | Trace event/condition APIs and persistence; identify reusable quest runner or implement missing data-driven transitions with rollback. |
| 7 | CF-008 | P1 | CODE | Trace installed-region descriptors into stable world records and streaming actors; implement missing binding independently of art. |
| 8 | CF-002 | P0 | CODE | **OPEN P0** - Provide production participant context independent of StartScenario; prove neutral enter/exit without spawning diagnostic characters. |
| 9 | CF-014 | P1 | TESTING | Map provenance/lock/rollback tests and actual command entry before classifying implementation gaps. |
| 10 | CF-016 | P1 | TESTING | Trace promotion events and actor rehydration, including death/missing actors and language filtering. |
| 11 | CF-009 | P1 | AUTHORITY | Reconcile latest accepted mappings and native provider; preserve QA separation and zero fabricated ownership. |
| 12 | CF-004 | P1 | CODE | Trace both UI clients, then inject a validated installed-banner provider with honest empty state; use isolated test data only. |
| 13 | CF-005 | P1 | TESTING | Bind existing item tests to exact invariants; inspect ownership/slot failure boundaries before any new implementation. |
| 14 | CF-012 | P2 | TESTING | Inspect adb availability and guarded dev build route after P0 fixes; preserve package separation and user saves. |
| 15 | CF-010 | P2 | ANIMATION | Implement and verify rig-profile/cue boundary; retain source-specific authored clip dependencies separately. |
| 16 | CF-011 | P2 | TESTING | Map cue resolution/fallback/volume lifecycle tests, then bind accepted audio data. |
| 17 | CF-013 | P3 | ART | Resume exact reference comparison after code readiness; do not reopen accepted design. |

## Execution dependency sequence
1. Protect current work and establish real current build/native/runtime evidence (captured; gameplay failure remains).
2. Repair save-readiness command boundary (CF-001). No unrelated visual rollback.
3. Repair current-run evidence output (CF-003); then production participant binding (CF-002).
4. Trace canonical content/provider/transaction boundaries (CF-017), ownership/progression/inventory and territory invariants. Reuse implemented services.
5. Connect banner, quest/dialogue, encounters and world hydration through validated data contracts. Separate missing authored data from missing code.
6. Finish input, animation/audio interfaces and Android lifecycle/performance evidence. Device validation can run at meaningful implementation checkpoints.
7. Resume approved content/art integration after code readiness.

Independent read-only source audits are parallel-safe; concurrent mutations to the shared world store, Unreal build outputs and production worktree are not.
External content: accepted character kits, native rig-specific motion, kingdom geometry, approved item/quest/banner data and final source intake remain separate dependencies. Do not fabricate them.
Audit-only NOT_TESTED clauses are a review queue, not confirmed missing code. Their preliminary family priorities must be replaced when exact defects and dependencies are known.


## 2026-10-17 implementation disposition: CF-001 and CF-003 DONE

- CF-001 DONE (scoped): restored standalone read-only SQLite preflight and live canonical WAL-store validation; reject missing/closed/unmatched/invalid databases, preserve corrupt existing world and unchanged saves. Fixed fresh or healthy-unowned Ruler entry, valid Continue, and false-positive live WAL failure. Verified latest UE5.8.3 Win64 rebuild 8/8 actions, native Gate12 12 tests with 0 failures (11 clean, 1 warning), exact opening test Success, and fresh real Win64 game native Opening -> Ruler creation -> five-page nav -> World, with no fake Ruler/roster/territory. All per-run receipts retained in evidence directory.
- CF-003 DONE (isolated Win64 runner scope): each run gets independent UUID, per-run smoke observation, source SHA and compiled-module SHA, prior actual journey JSON moved into archive before startup and never read as a new result. Two fresh independent runs are preserved, one failed at the former opening boundary and the next passed that boundary and progressed 19 checks.
- CF-002 PROMOTED OPEN P0 (next real production issue despite provisional row order): 2026-10-17 live game first World shows Party defeated for a newly created Ruler with no assigned party, disables world interaction and ends real_neutral_world_interaction_entry at step 20 (19 prior checks PASS). Separate production actor/party binding must be repaired without auto-starting combat, fabricating roster assets, forcing summons, or changing canonical Foundation.
- Do NOT claim full 23-step native game journey, Android device, UX/visual parity, or exhaustive full systems audit DONE. Gate12 continues IN PROGRESS for independent remaining requirements. Exact evidence plus failed screenshots and unmodified earlier receipts under docs/production/gate12/evidence/20261017_code_first_p0_save_entry/.
