> **NEW Gate00 authority (2026-10-20):** Read [GATE00_AUTHORITY_HANDOVER_AND_P0_ORDERS_20261020.md](GATE00_AUTHORITY_HANDOVER_AND_P0_ORDERS_20261020.md) FIRST. Gate11 is retired. Gate00 handles canonical Registry v2 intake; Gate12 performs automated technical/native acceptance without Gate11 waiting. All references below to starting/consulting Gate11 are historical only. Preserve remote-vs-local branch divergence, protected dirty Gate12 files, accepted art and old Registry sources.

# Gate 00 — Crash-safe recovery, session checkpoint and single-message activation

**Last known coordination point:** 16 October 2026. Read [ACTIVE_OPERATING_RULES_20261016.md](ACTIVE_OPERATING_RULES_20261016.md) and [LIVE_EXECUTION_BOARD_20261016.md](LIVE_EXECUTION_BOARD_20261016.md). Structured values in [execution_state.json](execution_state.json). User is not required to say `migration required`.

## Mandatory session startup / restart (for every Gate, including Gate00)

1. Read latest user message and current Gate specialization; identify one current task or a valid new user commission.
2. Fetch latest `coordination/gate00-live-ops-20261016` docs and GitHub issue #25. Do **not** assume these chats auto-sync: latest real user direction overrides older board text.
3. Read the Gate's current isolated branch HEAD, latest `docs/production/<gate>/SESSION_CHECKPOINT.md` or equivalent and current local uncommitted source. Never discard uncommitted work. Verify actual code/assets rather than trusting prose status.
4. Read dependencies ONLY if directly required for the next executable step. Consult their cited commit/manifest; do not request that the user paste old reports.
5. Load Software Mastery preflight/learned methods for relevant native operations; do not re-research solved issues. Work in the correct Gate's scope.
6. State a **brief observable action plan** (1–3 operations) and begin performing the task. Provide short factual progress while working, then checkpoint and commit.
7. Before response ends (and after any meaningful test/file change), update checkpoint and push to authorized branch. If no working GitHub write capability, clearly report **NOT SAVED TO REPO** and keep local backup/manifest; do not falsely claim full crash safety.

## Template: docs/production/<gate>/SESSION_CHECKPOINT.md

```markdown
# Gate NN — Last executable checkpoint
Updated: <YYYY-MM-DDThh:mm:ss+timezone>
Work item: <ID from issue #25 or explicit newer user instruction>
Status: ACTIVE | BLOCKED | DONE
Current owned branch and HEAD: <branch + full SHA>
Last verified local worktree: <clean / list uncommitted paths>
Artifact evidence: <real source paths, file hashes, tests, screenshots/clip paths>
Already complete / do not redo: <exact accepted work and accepted authority>
Action just executed: <concrete operation and outcome>
Progress since prior checkpoint: <small factual delta>
Next SINGLE executable action: <command or action, exact input and output>
Blocked on: <owner, exact missing evidence and why> | NONE
Quality/canonical acceptance: <specific creative/native/device/Registry status>
Source preservation: <protected source and backup hash where needed>
Device/Unreal cook authorization: YES — ongoing isolated development cook/package/test-sign/install/device QA (user approved 2026-10-16); never claim production release permission
Replacement-conversation prompt:
"Continue Gate NN from this exact GitHub checkpoint and latest worktree; begin with the Next SINGLE executable action. Do not restart."
```

**Don't fill empty states with invented values.** If a source private ZIP is protected, record its immutable metadata, not the bytes. Commit the checkpoint on each actual substantial transition, not solely before final chat message.

## One-message activation, used only when bringing a Gate back into scope

> **Gate [ID], continue your existing project with current authority. First read https://github.com/Azrazieliz/Offline-Game/issues/25 and the `coordination/gate00-live-ops-20261016/docs/production/gate00/` operating rules/board/recovery documents. Inspect your latest repo branch and session checkpoint. Read only directly needed dependencies. Show short factual progress, perform the next actual executable step, commit/push real progress and update your crash-safe checkpoint. Stop and mark DONE when this item's objective evidence passes. No repeated initialization or multi-gate messages.**

For the brand-new Gate01-3D conversation, additionally state "You are Gate 01-3D, dedicated only to native 3D character creation and source handoff. Preserve Luminous Base approved 3D and all Alter source; Gate01-2D owns creative visuals; load Software Mastery as Gate01."

## Acceptance / workflow rules

- Keep actual artifacts on each Gate's own branch, not the coordination branch. The coordination branch's docs describe status, not proof of implementation.
- `DONE` has verifiable link+hash, test/evidence receipt, exact acceptance authority and no unaddressed material blocker for that *particular* task. It can be DONE at 2D while downstream 3D or Registry stays OPEN.
- Only truly blocking external dependencies surface to Gate00; gate can continue separate owned production work.
- User receives no long relay instructions when one shared repo reference suffices.
- Routine isolated **development APK and device testing is already authorized** by the user (2026-10-16). Previous test APKs are not a finished app. Gate12 should compile/cook/install/test new dev builds without permission ping-pong, preserve actual user data/unrelated packages, and report evidence. Public release or destructive action still requires specific approval. Prior Android feasibility from earlier days is not proof that today's source is tested.
