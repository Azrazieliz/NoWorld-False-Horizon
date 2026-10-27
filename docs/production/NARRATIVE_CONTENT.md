# Narrative content runtime

Implementation follows [Content Production and Narrative Continuity](../CONTENT_PRODUCTION_NARRATIVE.md),
especially section 6: authored alternate outcomes preserve canonical history.
The runtime adds no plot, characters, dialogue, prices or rewards. Approved
installed content starts empty; this is runtime infrastructure, not completed
narrative production.

## Implementation plan and integration API

1. Define typed catalog/commands in `Public/Narrative/OGNarrativeService.h` and
   focused synthetic Automation fixtures before implementing the runtime.
2. Parse and validate a bounded JSON contract; reject unknown/duplicate fields,
   invalid references, reward loops and lossy or malformed resource amounts.
3. Use existing per-ruler project records for quests, subject-bound dialogue and
   shared immutable branch locks. Use canonical resource balances and append-only
   world-event request receipts in one transaction. Add no database or migration.
4. Recheck previews under the transaction; verify subject identity, current
   canonical state, location, branch locks, balances and objective evidence.
5. Inspect source and fixtures without running Unreal or native compilation.

`FOGNarrativeService(IOGWorldStore&)` exposes `Preview`, `Apply`, `ReadQuest`,
`ReadDialogue` and `GetConversation`; signatures are defined in the public
header. Main owns UI/session/world-event integration. Commands are AcceptQuest,
AdvanceQuest, CompleteQuest, StartDialogue and ChooseDialogue.

`FOGNarrativeContext` supplies canonical RulerId, SubjectId and LocationId.
Subject selectors require an exact entity kind plus either a canonical
`entities.state_json.narrative_actor_id` or manifestation identity. Authored
canonical `state_equals` string predicates support NPC life/death conditions;
manifestations additionally support exact `lifecycle_state` requirements.
Actors and subjective knowledge are never sources of eligibility truth.

## Installed JSON contract

The installed file is `Content/NoWorld/Narrative/NarrativeCatalog.json`.
Main must stage `NoWorld/Narrative` as UFS. `LoadInstalled` reports a missing
file/staging error; it does not invent fallback story. The root has exactly
required `schema: "noworld.narrative.v1"`, `quests: []` and `dialogues: []`.
Unknown members, duplicate JSON keys, nulls, incorrect types and malformed
decimal values are rejected. Parsing clears output on failure and publishes
only a fully validated catalog. Native callers receive the same validation.

| Object | Required fields | Optional fields and defaults |
| --- | --- | --- |
| Quest | `quest_id`, `display_name`, nonempty `objectives` | `description: ""`, `subject`, `conditions`, `accept_effects`, `completion_effects`, `allow_remote_completion: false` |
| Objective | `objective_id`, `display_name`, `event_type`, `subject` | `required_count: 1`, `allow_historical_events: false` |
| Dialogue | `dialogue_id`, `display_name`, `subject`, `entry_node_id`, nonempty `nodes` | `conditions` |
| Node | `node_id`, `text`, `choices` | `terminal: false` |
| Choice | `choice_id`, `text`, `next_node_id` | `conditions`, `effects` |
| Subject rule | `entity_kind`, exactly one of `actor_id` / `identity_id` | `state_equals: {}`, `lifecycle_state`; life predicate requirements below |
| Conditions | None | `completed_quests: []`, `required_events: []`, `excluded_events: []`, `required_branches: []` |
| Event condition | `event_type`, `subject` | None |
| Effects | None | `costs: []`, `rewards: []`, `branch_locks: []` |
| Resource cost/reward | `resource_id`, `display_name`, `amount` | None |
| Branch requirement/lock | `branch_id`, `value_id` | None |

All IDs use `FOGContentId::IsValid()` and are immutable lowercase
`namespace:key` references. Labels and text must be nonblank. `event_type`,
`entity_kind`, `lifecycle_state` and state-predicate keys are stable ASCII name
tokens of letters/digits/`_-. :/` (without spaces), shorter than `NAME_SIZE`,
excluding `None`. JSON integer fields use plain integral tokens; `2.0`, `2e0`,
strings and fractions that round to integers are rejected. Amounts are strings
matching `[1-9][0-9]*`, at most signed int64; no double conversion occurs.

Currency and aggregated material costs/rewards use the same canonical
resource balances. Each list has unique resource IDs. The same resource may
appear once as a cost and once as a reward, with matching authored display
labels. Costs must be affordable **before** receiving the reward. Overflow
rejects the entire command; amounts never clamp or silently round.

Event-condition and objective `subject` is exactly `ruler` or `subject`.
The latter refers to the command's bound canonical subject, not any nearby
entity. A subject reference requires an authored subject rule. Duplicate or
directly contradictory conditions are rejected. Quest prerequisites must name
catalog quests and form a DAG. Dialogue node references must resolve; all
nodes must be reachable and acyclic. Terminal nodes have no choices;
nonterminal nodes have at least one. Choice IDs are unique within a dialogue.
No restart/repeatable loop can farm a choice or reward.

## Canonical subjects and local/remote policy

Main supplies the ruler from its validated session. The service independently
requires canonical entity kind `ruler` and rejects `diagnosticFixture` markers.
An optional `Context.InteractorEntityId` defaults to that ruler. A different
interactor must be an active, owned, non-diagnostic manifestation; local
presence checks use that exact entity, not the ruler's displaced body.

An actor subject matches both exact canonical entity kind and
`entities.state_json.narrative_actor_id == actor_id`. Its `state_equals` must
include `life_state` or `lifecycle_state`, plus any further authored exact
canonical string predicates. The canonical actor admission/spawn writer must
supply those fields. A manifestation subject matches its canonical identity
and requires an explicit `lifecycle_state` equal to the manifestation record.
Generic actor rules cannot substitute a manifestation lifecycle condition.
The service does not infer death from rumor or resurrect a dead participant.
Dead/adapted scenes need explicitly authored matching rules and branches.

| Command | Spatial policy |
| --- | --- |
| AcceptQuest | Current canonical location must match the interactor's presence. Subject-bound quests also require the subject there. |
| AdvanceQuest | Remote journal operation; no physical presence/location is needed. Keeps the accepted subject and rechecks its canonical identity/state. |
| CompleteQuest | Ruler-only quests can complete remotely. Subject-bound quests require local co-location unless `allow_remote_completion: true`. |
| StartDialogue / ChooseDialogue | Both interactor and exact authored subject must occupy the supplied accessible canonical location. |
| ReadQuest / ReadDialogue | Read saved history only; do not grant current eligibility or advance anything. |
| GetConversation | Local canonical subject checks; emits authored text and reviewed eligible choices without starting/advancing state. |

For local operations, `Context.LocationId` is the live interaction location.
Remote journal commands may leave it unset and default interactor to ruler.
Accepted project location remains unchanged; creating shared branch state on
remote turn-in reuses the accepted project's original location. Reads never
create knowledge/observation rows. Canonical position snapshots participate in
staleness checks; the native interaction layer owns range/line-of-sight rules.

## Objective evidence and same-tick ordering

`AdvanceQuest` carries up to 256 distinct `EvidenceEventIds` per command.
Each is read with `TryReadWorldEvent` and must already exist at or before the
command tick. Exact event type and primary/related entity membership must
match the objective. Each event counts once per objective, up to its authored
target of 1–1000 events. One event can satisfy multiple separately authored
objectives. Payload inference, report confidence and knowledge facts are not
used. Irrelevant evidence is ignored; a batch with no new matching credit is
ineligible. Credit does not automatically claim the completion reward.

Normally, events before acceptance do not count. At acceptance the service
records IDs already present for each objective **at the exact acceptance
tick** in `AcceptanceBaseline`. Evidence later appended at that same tick can
count when absent from this baseline; earlier events at that tick cannot.
The existing event API has a 1000-row query window. If that window cannot
prove the same-tick baseline complete, acceptance fails explicitly and asks
for a later canonical tick; no incomplete baseline is silently admitted.
`allow_historical_events: true` explicitly permits earlier canonical evidence.

Main may gather event IDs from existing exact-type/subject event lists and
submit a reviewed AdvanceQuest command. The service does not autonomously
scan or promise lossless catch-up of arbitrarily large histories through the
bounded list API. Callers should forward committed event IDs or arrange
bounded batches. Source events must include the ruler/subject as a primary or
related entity; merely putting their name in free text is insufficient.

## Transactions, branches and restart

Deterministic IDs namespace existing `projects` records for each ruler/quest,
ruler/dialogue/subject, and a ruler's shared branch locks. Existing canonical
location foreign keys remain intact. Sparse payloads use
`noworld.narrative_state.v1`; timestamps stay in the existing int64 columns.
Credited event IDs, same-tick acceptance baselines, accepted subjects, node,
completion and definition fingerprint survive restart. No migration or second
database is introduced.

`Preview` returns proposed state, labeled resource deltas, missing requirements
and a confirmation token; it writes nothing. `Apply` starts a WorldStore
transaction/savepoint, rechecks the full relevant canonical snapshot and then
updates balances, quest/conversation projects, immutable shared branch locks
and an append-only `noworld.narrative.command_applied` receipt atomically.
Entity revisions, current subject/lifecycle/state, actual presence/location,
definition, evidence, prerequisite projects, branch state and balances affect
the token. A changed condition rejects an old quote before mutation.

The caller generates a persistent request UUID and retains the exact command
(including evidence ordering) and token for retries. The request UUID is the
event ID. An exact retry returns `bAlreadyApplied` without another cost/reward;
using it for a different command/token/owner fails. Replays can acknowledge
prior success after later state changes; they perform no new mutation.
Failure at any stage rolls back the service's scope, preserving a caller's
outer transaction. A branch lock cannot be changed to a competing value.

Changing an accepted definition requires explicit content/save migration;
the kernel fails closed rather than silently rewriting an ongoing story.
This includes authored text/label changes because reviewed meaning is part
of the definition fingerprint.

## Native journal and World interaction

`UOGNarrativeSessionSubsystem` derives the current owner from the real Ruler
session. It keeps only transient subject selection and reviewed commands; the
service owns all persistent state. The existing native Records page opens the
quest journal. Available quests are shown only when their canonical acceptance
conditions hold; accepted/completed quests retain their actual saved progress.
New conversations expose a reviewed start action, then authored eligible choices.
The review shows labeled resource balances/costs/rewards before committing.

`AOGNarrativeWorldActor` is an authored visual interaction body for an existing
canonical subject/location, never an NPC factory. Content supplies the original
visuals, exact IDs, prompt and interaction range. The existing touch interaction
search reaches its `IOGWorldInteractable` interface. Opening checks the actual
locally controlled body's canonical binding, distance, clear line of sight and
shared saved location, then opens Records in Ruler Mode. Review/confirmation
revalidates the subject and owned interactor. Local completion requires returning
to the original subject unless the quest explicitly permits remote completion.

The session collects exact committed event IDs in bounded fair batches, skipping
complete objectives, credited IDs and preacceptance baselines. It does not mutate
progress while displaying the journal. Canonical evidence is applied through an
explicit reviewed progress command. Core release, owner changes and navigation
discard pending quotes; narration and prices cannot silently change a review.

## Runtime boundary and native verification

This kernel supports single-run quests, unordered counted event objectives,
explicit completion, finite dialogue graphs, exact canonical actor/life-state
conditions, quest prerequisites, immutable narrative branch consequences and
canonical resource costs/rewards. It does not implement arbitrary scripts,
payload-expression predicates, unique inventory items, XP/Rank changes,
automatic quest failure/abandonment, NPC death/resurrection mutations,
repeatable loops, timers or content migrations. Unsupported source rules need
an explicit extension; they must not be approximated or dropped.

Technical bounds are 1000 quests/dialogues each, 64 objectives per quest,
128 nodes per dialogue, 64 choices/condition/resource/branch entries per list,
4096 persisted shared branch locks, 64 JSON nesting levels and 8 Mi characters
per JSON document. These are format/runtime bounds, not invented story caps.

`OGNarrativeRuntimeTests.cpp` contains synthetic catalog, SQLite restart,
objective deduplication and same-tick ordering, rumor separation, owner/subject
validation, NPC death/stale preview, branch persistence, selected interactor,
remote completion policy, resource staleness and late receipt failure rollback
tests. They require separate Unreal execution. Source checks alone do not
prove these Automation tests pass, and no authored narrative is claimed complete.
