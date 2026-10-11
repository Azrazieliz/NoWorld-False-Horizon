# Gate 12 — Luminous Valentine V3 actual clothing vs Privacy effective rendering
Date: 2026-10-12 · Post-Android Gate00 P0 continuation · Gate11 acceptance pending

## Authority and intended design

Latest Gate00 2026-10-12 adult-scope clarification controls character semantics.
Luminous Valentine is **one valid intended canonical character**, not three characters.
CLOTHED is her base/default appearance. UNDERWEAR and NUDE are additional
adult-context appearance assets belonging to that same Identity and base Version.
The source interchange name LuminousValentineV3 and proposed Registry strings are
**not** approved canonical runtime identifiers, installed package receipts, or
persistent owned Manifestation IDs.

Character truth and world outcomes remain in the frozen v0.41 systemic runtime:
real persistent participants, physical state, equipment, injury, clothing, staged
action graphs, consequences, world events and existing inventory/progression.
No gallery-only runtime, separate adult save, replay-engine dependency, invented
bond/unlock rule or duplicated character record is introduced by this patch.

## Implemented (source committed; not yet native compiled)

* New read-only FOGLuminousV3AppearanceProjection (Blueprint-facing):
  ActualWardrobe, EffectiveRenderedWardrobe, bPrivacySfwEnabled,
  bExplicitContextAuthorized, bMaskedForPrivacyOrUnavailableAdultContext,
  bRenderable, bDefaultBaseAppearance.
* Pure FOGLuminousV3AppearancePolicy::Resolve(actual, privacy, approved-context,
  safe-base-available) has **no storage reference**. Normal CLOTHED renders
  CLOTHED. Authorized explicit alternate renders itself when Privacy off.
  SFW masks alternate with CLOTHED without changing actual. Clearing SFW
  renders actual again. If no safe fallback exists, presentation fails closed.
  Unsupported enum and unapproved adult context never reveal explicit assets.
* AOGLuminousV3PlayableCharacter.CurrentWardrobe is the **actual** selection.
  AOGLuminousV3PlayableCharacter.EffectiveAppearance is a separate derived
  representation. ApplyVisualWardrobe installs geometry for the effective
  representation only; it does not set CurrentWardrobe.
* Runtime refresh detects changes to the existing FOGPlayerProfileSettings
  SFW flag, changes only the native mesh/animation display, and never calls
  FOGFoundationCharacterRuntime::SetPresentation or alters SQLite.
* Native motion clip binding uses the skeleton of EffectiveRenderedWardrobe:
  a masked explicit actual state uses the genuine CLOTHED rig/clip rather
  than trying to play an incompatible explicit-rig animation on CLOTHED parts.
* A failed safe render hides all native visual parts instead of retaining an
  unsafe previous explicit render. Original mesh sources are unchanged.
* Actual outfit changes use SetLuminousWardrobe as before, with canonical
  save preflight and rollback. Calls with bPersist=true still require the
  existing validated party/Identity/Version. Unapproved explicit operations
  outside isolated QA are denied. No guessed IdentityId/VersionId is inserted.
* Engineering wardrobe toolbar labels begin "QA", lives only where the
  isolated Blueprint explicitly enables it, and is disabled by shipping
  compile policy. It is NOT part of the final World HUD; final roster/detail
  controls belong to Gate10's approved UI.
* Existing Gate12 post-Android functional Ruler session commits are retained.
  Character-detail canonical adult choice bindings remain blocked until
  Gate11 provides real accepted Registry mappings and owned Manifestations.
  There is no implicit ownership migration from a source fixture.

## Authored automated regressions, currently unexecuted

Unreal automation sources:
Source/OfflineGame/Private/Tests/OGGate12LuminousAppearancePrivacyTests.cpp

- OfflineGame.Gate12.Luminous.AppearancePrivacyRenderMatrix:
  base; authorized UNDERWEAR and NUDE; SFW mask; SFW off restore;
  unauthorized/unowned adult context; safe-fallback missing; unsupported
  state. ActualWardrobe is asserted to remain unchanged under visual masks.
- OfflineGame.Gate12.Luminous.PrivacyDoesNotMutateCanonicalOutfitOrOwnership:
  creates an isolated transient test SQLite database, a test Ruler, two
  independently owned Manifestations of one test Identity/base Version,
  differing original physical/outfit/other state JSON. Evaluates SFW on/off
  without writing, re-reads both original presentation records byte-equal,
  validates identity/version/owner and manifestation count remain unchanged.
  Fixture IDs are test-only and **never** become production Luminous IDs.

These tests prove only what they actually execute after an Unreal-native test
runner reports a result. Their creation and static source inspection are NOT
engine-native PASS or S26 PASS.

## Remaining validation gate / continuity

1. Authorized Windows host LAPTOP-1LI4VRCJ was OFFLINE in Desktop Commander
   during this change. No local worktree operation, UBT/editor, Android cook,
   ADB, rollback or app installation was attempted.
2. When connected: inspect actual descendant branch vs local uncommitted
   Motion V3/native content; fetch+reconcile without reset/clean.
3. Compile Unreal Editor C++ and run both native automation tests against
   an isolated -userdir; capture test-by-test XML/log evidence.
4. Use real Ruler/World presentation QA fixtures to toggle SFW during actual
   UNDERWEAR/NUDE state. Compare visual effective vs actual underlying state.
   Check no world save changes from privacy preference and no unsafe frame on
   missing-source fallback.
5. Rebuild the single isolated dev APK with the same intended signing and
   source recovery precautions only after native tests pass. Verify a safe
   signed same-package update, rollback and independent Samsung rendering.
6. Test real first and second legitimately owned Manifestations ONLY after
   Gate11 accepted mapping/ownership is available. Otherwise record BLOCKED.
7. Maintain immediate Gate12 P0 functional portrait Ruler and P1 camera,
   locomotion, foot sliding, jump and swim/fly tasks. No first-map beautification.
8. Preserve frozen Foundation 3f54f07cacbf193e160d0be15a7be5a7bc730c3b,
   original cards/121-part UAssets/nine Motion V3 clips, Registry v2, local
   world SQLite, installed com.azrazieliz.OfflineGame and com.azrazieliz.gate12luminousv3,
   and unmerged draft PR16.

## Independent status at authoring time

| Item | Result |
|---|---|
| Actual vs rendered source contract | IMPLEMENTED IN GITHUB SOURCE |
| QA-only World wardrobe visibility | IMPLEMENTED IN SOURCE; build unverified |
| Privacy render matrix automation | AUTHORED / NOT RUN |
| Two-Manifestation SQLite preservation automation | AUTHORED / NOT RUN |
| Windows UBT / native test runner | BLOCKED: workstation offline |
| New Android APK / S26 privacy regression | NOT TESTED |
| Gate11 canonical mapping / installed acceptance / ARTIFACT_READY | PENDING GATE11 |
| Post-Android Ruler full vertical slice | PARTIAL SOURCE / NATIVE VALIDATION PENDING |
| World camera, authored swim/fly, traction fixes | STILL OPEN |
