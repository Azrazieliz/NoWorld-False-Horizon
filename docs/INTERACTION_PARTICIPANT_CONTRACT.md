# Interaction participant contract

Stable native API: `Interaction/OGInteractionParticipantComponent.h`.

## Integration sequence

1. The real actor factory installs `UOGInteractionParticipantComponent` and calls
   `BindActualNativeParticipant(EntityId, RigFamilyId, Error)` against live Core,
   or `BindFromCanonicalStore(EntityId, RigFamilyId, Store, Host, Error, LifecycleCore)`.
   Entity must already exist; no Owner argument is accepted. The canonical runtime
   derives Manifestation and Owner from the Store. Native factory binding is the
   explicit assertion of which real actor embodies that entity.
   `BindActualNativeParticipant` and `RefreshFromCanonical` are Blueprint-callable.
   `DisplayName` is an editable, Blueprint-readable authored `FText` label for Host
   presentation only; it never selects or changes canonical identity/ownership.
2. Call `RefreshFromCanonical(Error)` at canonical/wardrobe changes. This refreshes
   regular canonical/appearance presentation and rediscovers the actual parts.
   An owned `UOGCharacterAppearanceComponent` remains the sole appearance path;
   the inherited unbound canonical adapter is not configured on a transient
   modular part. Original multipart factories without that appearance adapter
   may replace registered parts; a canonical rig configured by this participant
   is rebound to the exact new registered part, preserving entity/owner/state.
   Unbind clears an inherited adapter's binding only when this component installed
   that binding; it never destroys the inherited component.
3. Call `BuildParticipant(Role, Out, Error)` for a fresh read-only participant.
   `GetCurrentProjection` reads the complete current canonical projection. Neither
   operation refreshes visuals, creates state, or returns a stale cached snapshot.
4. Host uses `GetMotionController` for the active multipart/original native
   controller and `GetNativeParts` for actual skeletal parts. Discovery does not
   register parts or restart animation. Missing motion is compatible with ordinary
   logical interaction; native geometry is required for this physical binding.
   Configured controllers supply their exact registered list through the public
   `UOGSharedMotionControllerComponent::GetNativeParts() const` getter. Original
   multipart sources therefore exclude obsolete inherited/tutorial bodies and
   unrelated skeletal components. Component counts and visibility never stand in
   for the actual registration list. Active appearance parts are also checked
   against their exact authored native wardrobe contract.
5. Route relevance through `SetInteractionSignificance`; call `RestoreSignificance`
   on interaction exit. Every part's original policy is restored. Binding remains
   present regardless of significance; participant counts have no cap here.
   `IsInInteractionPresentation()` stays true until restoration/unbind, including
   background relevance and the interval after a clip finishes while the runtime
   awaits another action. Ordinary traversal presentation should yield while true.
6. Core lifetime release unbinds. Explicit Store callers without LifecycleCore must
   call `Unbind` before Store/Host destruction. Actor EndPlay/destruction unbinds.

## Source metadata

`FOGCharacterIdentityDefinition.CanonicalMaturity` is the existing canonical enum.
Store has no Identity-definition lookup. Optional package host/provider APIs expose
container leases and verified native installation, not a typed Identity query.
Imported package flags/JSON are therefore not treated as source-lore authority.

Install a native `SetSourceProfileResolver` backed by actual original definitions
or native authored lore/verified receipt. `FOGInteractionParticipantSourceProfile`
requires exact Entity/Identity/Version plus source profile ID/revision; empty
Identity/Version explicitly select a non-Manifestation entity, never a wildcard.
`FromDefinitions` copies `CanonicalMaturity` from the actual Identity and verifies
its actual Version, retaining optional descriptive authored age without inference.
For a world-born NPC/Ruler with no Identity/Version, the native source receipt must
name that exact entity and leave both content selectors empty. No profiles are
invented by this component and Unknown remains Unknown.

`ResolveSourceProfile` validates provenance/selectors on a fresh projection.
`HasCanonicalAdultSource` is a convenience check for the separately classified
adult/sexual source category only; absence, Unknown, and NonAdult fail that check.
It never blocks `BuildParticipant` or ordinary interactions. General mature
classification/privacy includes injury/violence and must not call this check just
because `bMaturePresentation` is set. No affection, romance, permission, appearance
heuristic, or new maturity classification is introduced. Legacy `AdultRestricted`
is not source metadata. Host/catalog own actual category selection.

## Ruler body

`ResolveActualRulerParticipant(World, Store, RulerId, Out, Error)` reads canonical
presentation on real world actors, requires exactly one physical actor projected
to that actual Store Ruler, and returns role `player`. Possession is not evidence.
The existing diagnostic `UpdateProtagonistRepresentation` installs a canonical
presentation on its separate Ruler body; the resolver can discover that component
without private pawn access. The production path is the existing Ruler pawn,
preserved and bound before possession switches to the owned companion. Its real
participant component is discovered directly; returning control must reuse that
same pawn. Missing/duplicate/conflicting bodies fail closed.

No World getter is required for this scan. The production World integration must
install/update ordinary `UOGCanonicalCharacterPresentation` or this participant
binding on the real Ruler body outside diagnostic execution, including possession
switches. A getter alone cannot supply a missing physical/projection binding.
If World wants an explicit accessor, the precise optional API is
`AActor* GetActualCanonicalRulerBody(const FOGEntityId& RulerId) const`; it must return
only a live actor whose current canonical projection matches RulerId, otherwise
nullptr. It must never default to the possessed companion.

## Validation

Six focused native automation cases cover Store-derived ownership, fresh state
and Version refresh, exact original-source selectors/maturity, ordinary NPC
interaction without maturity metadata, exact multipart controller discovery with
an obsolete hidden inherited body, significance ownership/restoration and part
replacement (including destruction of the canonical rig part), and the preserved
real Ruler pawn while a companion is possessed.
Only the new component pair, focused new tests, and this contract are in scope.
No native build/test execution or Git operations are authorized for this task;
the native cases are authored but unrun. Source checks verify declarations and
definitions, balanced delimiters, project includes, the motion getter dependency,
and absence of Store writes, entity creation, playback/registration side effects
in discovery, or diagnostic/possession dependencies in this component.
