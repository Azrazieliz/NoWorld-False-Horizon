# Generic native character appearance binding

`UOGCharacterAppearanceDefinition` is an authored presentation contract for one
exact Identity/Version. `UOGCharacterAppearanceComponent` assembles it on an
existing Actor or Character. It changes render components and native motion
registration only. It never creates ownership, selects a canonical Version, writes
outfit JSON, changes equipment, registers canon or imports source art. These
source files contain no production character definitions or art.

The component needs a registered owner with a scene root. It requires no actor
subclass, possession, fixed mesh name or universal mesh rotation. Existing native
world/preview adapters can delegate to it without Blueprint implementation logic.

## Authored Data Asset

Create `UOGCharacterAppearanceDefinition` Data Assets for existing source-backed
Identity/Version pairs and reference them in the component's `Definitions` array.
Missing, redirected, wrong-class and duplicate exact matching definitions fail.

| Field | Contract |
| --- | --- |
| `IdentityId`, `VersionId` | Exact existing content IDs, compared with canonical Store records. |
| `WardrobeStateJsonField` | Authored key used by the existing UI catalog in `OutfitStateJson`. |
| `SafeDefaultWardrobe` | Non-mature row in this same `Wardrobes` array. |
| `Wardrobes` | Independently authored rows; no form constants or universal counts. |
| `SharedMotionLibrary` | Exact typed soft reference to the shared native motion library. |

Every wardrobe has a unique `WardrobeId`, positive `ExpectedPartCount` equal to
`Parts.Num()`, authored `RigProfile`, `AnatomyProfile`, `OutfitProfile` and
`Capabilities`. `bUseIdenticalTopologyLeaderPose` is an explicit optional mode.

Every part has:

- Unique `PartId`, typed `Mesh` soft reference and exact typed `NativeSkeleton`
  reference. A same-count substitute or compatible-skeleton alias is insufficient.
- Complete ordered `BoneNames` and `BoneParentIndices`. The root parent is
  `INDEX_NONE`; subsequent parents must precede their children. The array length
  is the exact expected count, checked against both mesh and native Skeleton.
- Optional complete `ReferenceLocalTransforms`, one per bone. When present, both
  mesh and Skeleton local rest poses must match within `0.0001`. Empty explicitly
  omits this authored assertion; no rest pose is guessed.
- Complete `MorphTargets` inventory. Both exact count and every target's
  existence are checked. Empty explicitly means no morph targets.
- Authored `ComponentTransform` relative to the attachment root: finite,
  normalized rotation, nonzero scale axes. No yaw, height or form correction is
  added by the component.

Part counts, bone lists, native skeletons and rest poses can differ across
wardrobes and across parts. Native leader-pose mode separately requires identical
actual complete mesh topology and local rest transforms across its parts, even
when optional authored rest assertions are absent.

Native mesh, Skeleton and shared-library references must be valid top-level
`/Game/.../Asset.Asset` paths with matching package/object names. Reflected
`UPROPERTY` typed soft references in the Data Asset and component expose the
dependencies to cooking. The containing content Blueprint/Data Asset must still
be reachable from an authored cook root or Asset Manager/Primary Asset Label
policy. Runtime-only JSON strings do not cause an otherwise unreferenced asset
to cook. This change edits no cook configuration.

## Safe default and privacy fallback

Every wardrobe must provide `SafePrivacyFallback`, a direct non-mature row in the
same `Wardrobes` array. Safe rows may name themselves. Missing, mature, ambiguous
or cross-definition fallbacks fail contract validation. No fallback chains,
inferred clothing or generated substitute appear.

For `bSfw=true`, a requested mature row resolves only to its authored safe fallback;
a requested safe row renders itself. For `bSfw=false`, the requested row renders
itself. `RequestedWardrobe` remains the stored choice; `RenderedWardrobe` identifies
the geometry actually displayed. Neither is persisted by this component.
`bMatureAppearance` is descriptive presentation data, not canonical maturity,
ownership or adult-context authorization.

An invalid/missing fallback asset fails the transition and retains the old body,
binding and previously applied presentation mode. Callers must honor failure and
must not label the retained appearance as a successful privacy switch. A caller
handling a global privacy transition can conceal its viewport on failure.

## Canonical binding and refresh

```cpp
bool BindOwnedManifestation(const FOGEntityId& Ruler,
    const FOGContentId& Identity, const FOGContentId& Version,
    const FOGEntityId& Manifestation, bool bSfw, FString& Error);

bool MatchesOwnedManifestation(const FOGEntityId& Ruler,
    const FOGEntityId& Manifestation) const;

bool RefreshCanonicalAppearance(FString& Error);
bool ApplyWardrobe(FName Wardrobe, bool bSfw, FString& Error);
```

Binding resolves `UOGGameCoreSubsystem` from the owner's current Game Instance
and reads its live `IOGWorldStore`. The owner must be an existing canonical
`ruler` entity without the existing diagnostic-fixture marker. The exact
Manifestation record must contain the supplied owner, Identity, active Version
and `active` lifecycle. No identifiers or ownership are fabricated. Store pointers
are not cached, and no Store mutation method is called.

The binder reads `TryReadManifestationPresentationState` and parses its
`OutfitStateJson` using `WardrobeStateJsonField`. A missing record or absent field
uses `SafeDefaultWardrobe`. Malformed JSON, wrong-type/empty values, mismatched
record identity or unknown authored choices fail without changing the body.
Present saved strings must match authored wardrobe spelling. Unknown values are
never silently replaced by a default.

`MatchesOwnedManifestation` requires a complete native appearance and rechecks
live owner/Version/lifecycle, Core lifetime and saved wardrobe. It returns false
while a newly saved wardrobe differs from `RequestedWardrobe`, or while native
parts are missing/invalid, so stale geometry cannot pass the adapter's binding check.

`RefreshCanonicalAppearance` rereads the saved choice and renders it using the
current `bSfwPresentation`. An already-current complete appearance is a no-op.
Adapters may call it immediately after UI persistence. Owned bindings also keep
component Tick enabled and accumulate game delta time for a bounded 0.5-second
canonical refresh. At most one refresh is attempted per due tick; failed refreshes
retain the previous appearance and wait another interval, without catch-up loops.
Preview-only/unbound components do not poll, and stale Core bindings fail. Expression
overlays remain a separate per-frame operation. The accumulator resets after
successful installation and when the appearance is released.

Polling uses the currently applied `bSfwPresentation`; it does not infer or change
privacy mode. External privacy changes still enter through the existing bind/apply
methods with an explicit `bSfw` argument.

`ApplyWardrobe` is read-only rendering. With an owned binding, its requested row
must already equal the saved choice: persistence belongs to the existing UI/service
path. A privacy change can reapply that saved row with a new `bSfw`, or call
`BindOwnedManifestation` with the same real IDs and new presentation mode.

Store clear/import/recovery/shutdown invalidates the owned appearance through
`OnCanonicalRuntimeReleasing`. The component removes stale generated geometry
without revealing an unrelated old authored body. Normal component teardown
restores original visibility for explicitly replaced body components. Ownership,
Version and lifecycle are also rechecked before owned expression or motion calls.

## Hidden preparation and publication

`AttachmentComponentName=NAME_None` uses the owner's root. Otherwise that name
must resolve to exactly one owned registered scene component. Author each part's
transform relative to this root. `BodyComponentNamesToReplace` explicitly names
existing skeletal body components to conceal after successful preparation. No
body is guessed from `ACharacter::GetMesh`; the authored actor/native adapter
supplies names. The attachment root cannot itself be a replaced body. Keep this
configuration stable while installed.

1. Validate the entire definition and requested/privacy-resolved row. Load and
   pin every native mesh and exact Skeleton, check native classes/paths, counts,
   ordered hierarchies, optional rest transforms and morph inventories. Load the
   exact authored shared motion library.
2. Validate the attachment and replaced bodies. Create fresh transient parts
   hidden in game and editor visibility. Apply authored transforms, disable
   collision/overlaps/navigation effects, assign real native meshes and register.
3. Create a fresh `UOGSharedMotionControllerComponent` for those candidates,
   configure the authored library/Rig/Anatomy/Outfit/Capabilities, then register
   every actual part. The old controller retains its registration and playback.
4. Recheck candidate registration and canonical owner/Version/lifecycle/saved
   wardrobe immediately before publication, accounting for loading callbacks.
5. After fallible checks, hide the previous geometry, publish the complete new
   part set and controller, then destroy the previous generated components.

Scope cleanup destroys failed candidates and their candidate controller. Earlier
body visibility, owned binding and native playback remain intact on preparation
failure. Imported mesh/Skeleton assets are never modified. A missing initial
appearance stays absent; no placeholder is created. This is a synchronous
game-thread publication boundary, not an atomic GPU-frame or asset-transaction
guarantee across separate scene proxies.

Installation rejects reentry. The binder does not commandeer another system's
motion controller. `GetMotionController()` exposes the current shared instance;
it changes after successful wardrobe publication and should not be cached forever.

## Expressions and shared motion

Each wardrobe authors unique `Expressions` semantics with explicit
`(PartId, Morph, Scale)` mappings. Every target must exist on that exact part and
its declared native morph inventory. Scale is finite in `[-1,1]`.

`SetSemanticExpression(Semantic, Weight, Error)` accepts finite normalized weights
in `[0,1]` and validates all mapped live targets before writing. Contributions
from active semantics to the same exact target are summed and clamped to
`[-1,1]`. Zero explicitly zeroes that semantic's contribution. Only active mapped
targets are written; unmapped targets are preserved. No `ClearMorphTargets` or
same-name broadcast across unrelated parts is used. Semantic weights survive a
wardrobe switch only where the new rendered row also authors that semantic.

Expression Tick follows the shared controller and precedes managed mesh ticks.
This applies exact-part expression overrides after shared motion morph layers,
without changing the existing controller's cross-part layer behavior. The mapping
intentionally owns the exact target it addresses; other targets remain untouched.

`PlaySemanticAction(Action, Context, bAdultContextAuthorized, ParticipantCount,
ParticipantRole, Error)` delegates to the existing controller's `PlayAction` and
exact-rig resolver. Independent parts use same-source phase locking. Leader-pose
sharing requires the explicit mode and verified actual topology/rest compatibility.
There are no guessed clips, copied motion catalogs or source-identity aliases.

Authoritative root motion remains disabled by this component. Movement,
collision, combat, resources and adult-context permission stay with their existing
gameplay authorities. `bSfw` does not stand in for any of those permissions.

## Explicit asset preview

`InitializePreviewDefinition(Definition, Wardrobe, bSfw, Error)` uses the same
native validation and hidden publication without ownership reads or writes. It
cannot replace an already owned binding. A successful preview has no owned IDs,
`IsOwnedManifestationBound()` and `MatchesOwnedManifestation()` are false, and no
fake Ruler/Manifestation is created. Production previews of an owned copy should
use `BindOwnedManifestation` with real IDs. Moving from local preview to ownership
still requires full canonical validation and successful publication.

## Native World and roster adapters

Content Blueprint subclasses of `AOGContentPlayableCharacter` reuse the actual
World movement, camera and touch input from `AOGWorldPrototypeCharacter`. Assign
the authored appearance Definitions on its component. The existing exact-version
visual catalog can point its World class at this subclass. Successful binding
requires the same real owned Manifestation as the session; no per-character C++
subclass or new ownership record is needed. Traversal selects shared semantic
actions from actual movement state and never overwrites a playing combat action.
Run selection stays disabled until content authors a speed threshold.

`AOGContentCharacterPreviewActor` implements the existing owned preview interface
without being a Pawn. Point the same visual catalog's preview class at a content
subclass containing the same appearance Definitions. The roster's existing capture
stage binds the actual owned copy and privacy projection before rendering it.

An actual running battle binds the character's physical body to that exact owned
unit through `UOGCanonicalCombatActorBindingComponent`. Optional authored skill-to-
semantic mappings handle accepted action notifications. Local World action input
also retains exact skill/Ultimate-branch semantics, actual target facing, native
hit/defeat reactions and the existing HP-impact HUD. These presentation adapters
do not implement missing combat executors or activate unregistered v4 kits.

## Native verification still required

Source review checks the existing Store/shared-controller declarations and the
installed UE 5.8 engine headers for native Skeleton/mesh access, component mesh
assignment/registration/destruction, instance-component ownership, strong object
references and tick prerequisites. No C++ build, UHT run, native launch, cooking
or import is part of this change. Header inspection does not establish a passed
native compile or runtime acceptance.

Later native verification should exercise exact owned success and wrong-owner/
Version/lifecycle failures; saved-choice refresh; malformed/unknown choices;
different part/bone counts; wrong Skeleton/missing morphs; failed hidden candidate
registration preserving old body/playback; privacy fallback success/failure;
unmapped morph preservation with duplicate names on unrelated parts; independent/
leader motion compatibility; Store replacement; and cooked soft-reference
availability for actual authored content.
