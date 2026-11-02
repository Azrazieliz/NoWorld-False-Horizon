# Systemic interaction state contract

Authority: `docs/MATURE_CONTENT_V041_PRODUCTION_FREEZE.md`, then compatible provisions of `docs/MATURE_CONTENT.md`. This is a neutral engineering contract. The existing graph, original diagnostic graph parameters, variable participant arity and world store remain authoritative.

## Additive native API

```cpp
struct FOGInteractionPresentationPatch
{
    FName Role = NAME_None;
    TOptional<FOGContentId> SelectedSkinId;
    FString OutfitMergeJson = TEXT("{}");
    FString VariantMergeJson = TEXT("{}");
};
struct FOGInteractionAdultContextPatch
{
    FName Role = NAME_None;
    TOptional<FOGContentId> CurrentProfileVariantId;
    FString MutableContextMergeJson = TEXT("{}");
};
struct FOGInteractionRolePlacement
{
    FName Role = NAME_None;
    FTransform RelativeToAnchor = FTransform::Identity;
};
```

`FOGInteractionAuthoredConsequences` appends `TArray<FOGInteractionPresentationPatch> PresentationPatches` and `TArray<FOGInteractionAdultContextPatch> AdultContextPatches` after its existing arrays. `FOGInteractionActionNode` appends `FString DisplayName` and `TArray<FOGInteractionRolePlacement> RolePlacements`. `FOGInteractionActionGraph` appends `FString DisplayName`, `FString Description` and `bool bMaturePresentation = false`. Original fields and public method signatures retain their order and meaning.

`bMaturePresentation` classifies source presentation, including nonsexual categories such as injury. It is not mutable permission, canonical adulthood, an eligibility boolean or an access rule. Native definition/category lore filtering remains the caller's responsibility; the runtime does not implement a blanket mature gate or inspect a legacy shared-motion `authorizedAdult` parameter.

`TArray<FName> GetAvailableActions() const` returns the current node's next nodes, or the last completed node's next nodes while awaiting input; without either it returns the graph start. Results contain each authored next-node ID once. An inactive session returns an empty array. `const TArray<FName>& GetSequence() const` and `GetQueuedActions() const` expose the same pending queue, excluding the active node. These are read-only projections. Queue validation remains in the original `SetSequence`; `SetSequence({})` clears the pending queue.

`IOGSystemicInteractionPresentation::EndAction()` is an optional default no-op hook, called after `StopSharedScene()` when action presentation stops. A bridge should release action-scoped solo playback, constraints and reactions while retaining session character ownership. Session exit still calls `RestoreGameplay()` once. Shared-controller pacing belongs to the Host; the runtime updates already-playing legacy montages.

## Completion and canonical targets

Patches execute only at an actual completed action, inside the existing store transaction together with `interaction.action_completed`, knowledge, memory and other explicitly authored consequences. Entry, partial progress, zero pacing, failed completion and exit do not apply them. The store must provide synchronous transactional commit/rollback semantics; callers must not later roll back an enclosing transaction after treating a runtime transition as durable.

Each patch role must identify exactly one bound canonical participant. Repeated observer/group roles remain supported by old graphs but cannot select an arbitrary member for a patch or a single source placement. Use individual roles for targets. Rebinding the same graph to another NPC requires no target IDs in the patch. Multiple patches for the same target compose in array order. Full presentation replacement and sparse presentation patches for the same target in one action are rejected.

Target entities, current Manifestation/ownership binding and affected state records are read from the store inside the completion transaction. Cached `Participants[].Character` projections never supply replacement state. Missing entities, changed explicit bindings, malformed affected records or a state timestamp newer than completion fail without resetting or replacing that state. The next retry reads the canonical records again.

For a Manifestation, presentation uses the existing `ManifestationPresentationState` record. For an ordinary NPC, it uses the established entity `foundation_presentation` object with string `skin_id`, `outfit_state_json`, `variant_state_json` and decimal-string `updated_world_tick`. Unknown presentation extensions, entity siblings, canonical kind and untouched state survive. This is the existing ordinary character projection format, not a new save structure.

Adult expression/context uses the existing `CharacterAdultRuntimeState` record: `CurrentProfileVariantId`, `MutableContextStateJson` and completion timestamp. An absent record is initialized through that same record type. There are no new eligibility, consent, affection or reward fields or gameplay gates.

An unset optional ID preserves the fresh selection. A set valid content ID changes it; a set empty content ID explicitly clears it. `VariantMergeJson` uses the ordinary `hairstyle_id` field for hairstyle selection. Selecting a profile or presentation ID does not grant packages, ownership, development, forms or Versions.

## JSON and merge boundary

Patch roots and affected state roots must be JSON objects. Objects recursively merge. An object patch replaces an existing nonobject value at that key. Scalars and arrays replace the corresponding value. Null is an ordinary stored value, not deletion. Unmentioned fields retain their values, including nested siblings. An empty object patch preserves the existing JSON string verbatim. Rewritten objects may change formatting/order; numeric tokens are stored without converting untouched large integers to doubles.

Validation rejects empty/malformed JSON, duplicate decoded member names at any object depth, trailing documents, comments, trailing commas, invalid escapes, nonfinite/out-of-range numeric tokens, documents larger than 8 Mi characters and nesting beyond 64 levels. Canonical data is not silently repaired. Existing full presentation and memory JSON are validated as objects; knowledge values may be any strict JSON value. Full record arrays retain their explicit replacement semantics and absolute targets; sparse preservation guarantees describe the new patches.

Patch member names are checked at every object depth, including inside arrays, case-insensitively after removing underscores and hyphens. Reserved names are: `eligible`, `eligibility`, `iseligible`, `beligible`, `permission`, `permissions`, `consent`, `consentstate`, `affection`, `affectionpoints`, `affectionthreshold`, `authorizedadult`, `reward`, `rewards`, `currency`, `grants`, `manifestationid`, `identityid`, `owningrulerid`, `ownerentityid`, `versionid`, `activeversionid`, `grand`, `grandstate`, `isgrand`, `bgrand`, `convergence`, `progressionstate`. This is patch schema validation, not a participant access test. Existing untouched canonical keys are preserved. Arbitrarily named authored context values are inert data, never commands or grants.

There is no general entity-state patch API. The only entity mutation is the existing NPC presentation container. No patch calls Manifestation creation/ownership, Version, form, progression, Grand, resource or reward mutation APIs. No new database, archive, content definitions, animations, art or presentation text catalog is introduced.

## Authored source placement

`RolePlacements` supplies original source translation and orientation relative to a shared candidate anchor. It never generates choreography. Tagged environment anchors include their current rotation; the preferred position and nearby search candidates use identity orientation. Source transform scale is ignored; each embodied actor keeps its current body scale and logical-only participants keep unit scale.

All authored roles share one vertical correction derived from their current capsules and floor samples; their relative source translations, including height, remain intact. Other participants use the existing ring fallback outside the authored group's horizontal extent. Every logical participant remains represented, including more than ten. Environment floor/slope and world collision checks still run. Authored source roles can intentionally have nearby/overlapping roots, so the runtime does not impose additional source-role separation or synthesize offsets. Final pose/rig compatibility and active shared tracks remain the presentation bridge's responsibility.

Staging publishes transforms only after an entire candidate succeeds. Failed entry leaves ordinary actors untouched. Failed restaging preserves the previous successful placement. Bounded nearby search can fail without disabling the interaction category globally. A source scene with no supplied role placements continues using the original fallback staging.

## Lifecycle and world time

World time belongs to the canonical Host clock and advances independently of action pacing, including zero pacing. Active advances and exit persistence reject backwards ticks; equal ticks remain valid. A stale/unavailable exit tick reports failure while still restoring gameplay. Finite nonnegative pacing has no gameplay intensity ceiling; conversion for legacy montage playback is bounded to finite float range only.

At most one action completes per advance; surplus frame duration is discarded. A successful completion is marked consumed before refresh, presentation cleanup or starting a queued node. Failed post-commit refresh or queued staging can be retried on a later advance without repeating the completed action or its consequences. A recovery call starts the next action without charging that call's delta to it. A failed transaction leaves the original completed-duration action available for a fresh retry or interruption; interruption does not award unfinished consequences.

Completion and action replacement stop owned legacy montages and shared playback and invoke `EndAction`. Exit refreshes character projections and restores ordinary movement, visibility, input counters, camera and optional asset ownership even when event persistence fails. Existing same-location presence-delta restoration remains the runtime's coordinate boundary; cross-location world loading/coordinate resolution belongs to the ordinary Host/world integration.

## Validation and acceptance boundary

Automation cases in `OGSystemicInteractionRuntimeTests.cpp` cover fresh ordinary-NPC merges, canonical Manifestation preservation, optional clear/preserve behavior, ordered patches, rollback/retry, strict JSON, ambiguous roles, committed-completion queue recovery, source-role offsets with 13 participants, monotonic ticks and cleanup. Original neutral graph and automation scenarios remain.

Checkpoint validation covers source inspection and repository checks. Unreal compilation, native automation execution, source animation alignment, rendered behavior and Android gameplay remain unverified. The production Host implements `EndAction` cleanup; the user alone accepts gameplay on their phone.
