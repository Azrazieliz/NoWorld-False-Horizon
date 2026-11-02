# Character development content

The installed catalog is `Content/NoWorld/Progression/DevelopmentCatalog.json`,
staged as UFS under `NoWorld/Progression`. Its root has exactly `schema` (the
string `noworld.character_development.v1`) and required `offers` (an array).
The shipped array is intentionally empty: no upgrade, price, cap, stat curve,
character, feat or other canon is invented. Empty installed content is valid.
Missing/unreadable installed content returns false with its path and staging
instructions; invalid content returns false with an actionable field/offer error.
Loading/parsing clears the output first and publishes it only after validation.

## Offer contract

Names are case-sensitive JSON keys. Unknown fields, nulls, wrong types,
duplicate object members and JSON nesting beyond 64 levels are rejected.
JSON integer fields use plain decimal integer tokens, without a fractional
part or exponent (even `1.0` and `1e0` are rejected from their original tokens).
Omitted optional fields use the defaults below; an explicitly present field
must have its documented type. No separate catalog version field exists: `v1`
is the format version and `version_id` identifies the character version.

| Key | Type / default | Meaning |
| --- | --- | --- |
| `development_id` | required content ID string | Globally unique offer ID in this catalog. |
| `identity_id` | required content ID string | Immutable character identity. |
| `version_id` | required content ID string | Character version for this offer. |
| `display_name` | required nonblank string | Player-facing offer name. |
| `description` | string, empty | Player-facing explanation. |
| `minimum_level` | JSON int32, 1 | Minimum current level, 1–100 inclusive. |
| `required_rarity` | name string, omitted | Exact current rarity requirement. |
| `required_reinforcement_state` | name string, omitted | Exact current reinforcement state requirement. |
| `prerequisite_development_ids` | content ID string array, `[]` | Completed offers required first. |
| `excluded_development_ids` | content ID string array, `[]` | Completed offers that block this choice. |
| `required_routes` | route object array, `[]` | Required route/node states. |
| `required_facts` | event requirement object array, `[]` | Required canonical events; see below. |
| `costs` | cost object array, `[]` | Canonical resource debits. Empty means no resource debit. |
| `result_level` | JSON int32, 0 | Zero preserves level; otherwise `minimum_level`–100 inclusive. |
| `result_rarity` | name string, omitted | Omitted preserves rarity. Requires `required_rarity` when supplied. |
| `result_reinforcement_state` | name string, omitted | Omitted preserves reinforcement state. |
| `completes_reinforcement` | boolean, false | Marks this authored reinforcement endpoint complete. |
| `stat_grants` | object, `{}` | Additive stat increments, in the scientific format below. |
| `granted_routes` | route object array, `[]` | Route/node states granted by completion. |
| `granted_forms` | content ID string array, `[]` | Authored form IDs granted by completion. |

The canonical [Existence / Power Rank contract, section 2](../EXISTENCE_POWER_RANK.md#2-meaning-and-levels)
defines Levels 1–100 within every Rank. These offers perform same-rank level
development only. Level 100 does not overflow into a new Rank; breakthroughs
remain the existing rank engine's responsibility (section 4 of that contract).
Both parsing and native row validation enforce this canonical level range.

Content IDs use the existing `FOGContentId::IsValid()` lowercase
`namespace:key` grammar; they are immutable references, never display names.
Name tokens (rarity, state and event type) contain only ASCII letters, digits,
`_`, `-`, `.`, `:`, `/`, are shorter than `NAME_SIZE`, and cannot be `None`.
Optional names are omitted to preserve/no requirement; empty strings are not
sentinels. Runtime name comparisons follow the existing `FName` semantics.

Each cost has exactly required `resource_id` (content ID), `display_name`
(nonblank string) and `amount` (decimal string). `amount` matches
`[1-9][0-9]*`, fits signed int64, and is never converted through a JSON double.
Currency and aggregated materials both debit the existing canonical resource
balance for `resource_id`. There is no `currency`, `material`, `kind`, or second
wallet field. Duplicate resource IDs within an offer are rejected rather than
silently aggregating or double-spending a balance.

Each required/granted route has exactly required `route_id` and `node_id`
(content IDs), plus `state` (name token). A route/node pair occurs at most once
within each array, regardless of state. A required state may change in the
granted array. Repeating the required state alone is not an effective outcome.

Each required fact has exactly required `fact_key` (event-type name token),
`display_name` (nonblank string) and `subject` (exactly `ruler` or
`manifestation`). It requires canonical event truth: service execution queries
`Store.ListWorldEvents` with that subject entity and `EventType = FactKey`.
A rumor, subjective knowledge row or inferred achievement cannot satisfy it.
Source events must name the target ruler/manifestation as a primary or related
entity so the subject query finds them. `minimum_confidence_bps` is unsupported
and rejected. Duplicate subject/event-type requirements are rejected.

## Stat numbers and safe keys

Every stat value is an object with exactly required `significand` (decimal
string `0` or `[1-9][0-9]*`, at most 9 digits) and `exponent10` (integral JSON
number). Its value is `significand * 10^exponent10`, normalized as an
`FOGLargeNumber`. No significand passes through floating point or rounds away
precision. Negative grants, fractions as integer fields, signed/whitespace/
exponential decimal strings and out-of-range values are rejected.

Exponents are restricted to `MIN_int32 / 4 .. MAX_int32 / 4` inclusive
(-536870912 .. 536870911), leaving headroom for normalization and the existing
large-number arithmetic. This is a representation safety limit, not a
character level/stat cap. Both authored and normalized exponents must remain
inside that interval. `max_hp`, `attack` and `defense` may be fractional
scientific numbers. All other grants must represent exact integers in
`0 .. MAX_int32`, matching the destination combat fields. Zero-only grants
cannot make an otherwise empty offer valid.

Only these exact lowercase stat keys are accepted:

```
max_hp
attack
defense
speed
crit_rate_bps
crit_damage_bonus_bps
crit_rate_resistance_bps
crit_damage_resistance_bps
hit_bps
dodge_bps
block_rate_bps
block_reduction_bps
defense_penetration_bps
damage_dealt_multiplier_bps
damage_taken_multiplier_bps
true_damage_resistance_bps
```

## Validation and integration

`ValidateDefinition` checks a single native or parsed row, including IDs,
names, display labels, levels, rarity, unique positive costs, duplicate forms,
routes, facts and prerequisite/exclusion IDs, self references, direct
require/exclude conflicts, safe stat numbers and effective nonempty outcomes.
It does not require referenced offers to be installed. `Preview` should call
this entry point before accepting a native definition.

`Validate` first calls `ValidateDefinition` for every row, then rejects
duplicate offer IDs, prerequisite/exclusion references missing from this same
catalog or belonging to a different identity/version, and prerequisite cycles.
It also rejects exclusions intersecting the target's full prerequisite closure,
including conflicts between separate prerequisite branches. A cycle-free graph
can still be unreachable when an ancestor excludes another required ancestor.
Graph traversals are iterative, including long chains. Exclusions may be mutual
between alternative offers; they are not prerequisite edges. Duplicate forms
are checked within each offer, since alternatives may grant the same form.

Rarity uses the existing order `R < SR < SSR < UR < LR < MR`. Results cannot
decrease rarity. If either rarity is outside that taxonomy, result and required
rarity must be equal; a new rarity order is never inferred. An absent result
preserves rarity. A level equal to its minimum, unchanged rarity/state, zero
stats or unchanged required route alone do not constitute an effective outcome.
An offer needs a level increase, rarity/state change, reinforcement completion,
positive stat increment, changed/new route state or granted form.

Catalog validation does not resolve resource, character, route, event or form
definitions against other content packages and does not execute development.
The main service owns canonical eligibility, event queries, exact resulting
state checks, transactions and application. Before shipping authored rows,
run native catalog/service tests for valid and malformed content and verify
the UFS catalog in a packaged offline build. No native build is required to
inspect or author this contract.

Automation coverage lives in `OGCharacterDevelopmentCatalogTests.cpp` under
`OfflineGame.Progression.DevelopmentCatalog`: synthetic authored fixtures and
the empty installed catalog, malformed syntax/types/numbers/duplicate members,
native row validation, reference cycles and transitive exclusion conflicts.
These tests must be run in Unreal separately; source checks do not execute them.
