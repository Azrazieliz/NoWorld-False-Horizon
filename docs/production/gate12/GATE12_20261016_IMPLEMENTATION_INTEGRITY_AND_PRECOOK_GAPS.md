# Gate 12 — implementation integrity and pre-cook acceptance gaps (2026-10-16)

**Branch:** `production/gate12-koikatsu-ue583-fixture-20261010`
**Engine:** Unreal 5.8.3, source/native Android-first.
**Frozen Foundation:** `3f54f07cacbf193e160d0be15a7be5a7bc730c3b` (separate worktree unchanged).
**Stop rule:** NO Android cooking, package, sign, deploy, installation, device cleanup or user saves deletion without explicit later authorization.

## Authoritative user corrections
- D03 character chips / duplicate-conversion rewards were NEVER part of the approved architecture; the claim originated in a reporting error. Do not implement a new chip/conversion system or a fake removal task. Each repeat acquisition = another separately owned Manifestation.
- E03 canonical adulthood/content eligibility checks and E04 consent/interaction validation are removed as new Gate12 implementation requirements. Preserve independent existing source contracts; no destructive deletion.
- Gate09 authors animations, swim/fly skeletal clips, expressive motion and dance. Gate12 integrates clips, rig bindings, player controls, Android testing after Gate09 delivers.
- Character card and owned-character detail must offer true original 2D or live imported 3D, bound to exact canonical Identity+Version+owned Manifestation. Actual wardrobe and effective Privacy/SFW presentation are independent.
- 2D art may arrive later; never block unrelated engineering or substitute an unrelated image.

## Implemented native source and asset checkpoints
- Gate12 Ruler owner creation/restore and authoritative sqlite projections, 5 destinations, Home counts, named Records Chronicle/Intelligence with owner knowledge limits, Gacha event history, Territory claims.
- True Records list from canonical SQLite, sorted unread/priority/world tick, owner-and-time-verified persistent acknowledgement through `FOGReportService`.
- 3-column equal (width/height=0.445) owned Identity card grid, search by canonical name/ID/copy-label and sorting by name, selected level, number of independently owned Manifestations.
- Per-owned-Manifestation 2D/3D preference persisted into local `PlayerProfile.json` outside canonical save; never mutates physical clothing, equipment, world state, or other profile settings.
- Native `UOGGate12CharacterVisualCatalog` maps exact Identity+active Version to original 2D artwork, approved SFW artwork, authored wardrobe choices, real 3D Actor Blueprint implementing `IOGGate12CharacterPreviewConfiguration`.
- Native 3D scene capture only spawns approved owned read-only preview actor class after exact identity/owner validation; preview initialization requires successful `ConfigureOwnedPreview` including explicit Privacy argument; never substitute mannequin or guess content.
- Up to 6 still captured roster 3D images plus one separate live detail preview; missing/sensitive artwork is withheld truthfully, never displayed under unknown SFW state.
- Source-driven wardrobe controls accept only catalog-authorized StateJsonField/Choice for verified owned active Version; write only canonical outfit JSON through existing `FOGFoundationCharacterRuntime::SetPresentation` and preserve all unrelated fields; Privacy remains a noncanonical render preference.
- Native editable Widget Blueprint `/Game/NoWorld/UI/WBP_NoWorld_RulerShell`; native editable catalog Data Asset `/Game/NoWorld/UI/DA_NoWorld_CharacterVisuals`; the game HUD now chooses authored widget class when available with a native fallback, and enables the native Ruler interface by default for the Ruler surface.
- Native source tests: original Luminous privacy/facing and local Ruler tests + exact visual Version/no duplicate mapping + profile isolation/persistence + persistent Reports/owner isolation.
- Repeatable pre-cook integrity and native/full test runners and headless editor asset-authoring/validation scripts. A source-only audit NEVER claims device/content acceptance.

## Verification evidence to maintain and re-run after each source change
- Previous fully linked source checkpoint 15/15 UBT SUCCESS (no cook).
- Earlier expanded source checkpoint 16/16 UBT SUCCESS and 8/8 native Gate12 SUCCESS, full 147 tests: 130 success, 17 warning passes, 0 failures (no cook). Evidence is source-specific; do not attribute old receipts to subsequent changed source.
- Current wardrobe/privacy/WBP build and final suite require their own source-specific receipts before acceptance. See `Saved/Logs/Gate12_20261016_WardrobePrivacy_UMGIntegration_Build.log`, `Saved/Gate12Readiness` and exact run receipts.
- Pixel-perfect/genuine final UI, end-to-end navigation, real Koikatsu preview clothing, mobile touch/gameplay and Android device performance are NOT validated by a NullRHI native test.

## Missing required production assets / external Gate authority (NO fabrication)
1. **Gate11 Registry v2:** accepted real character IdentityId/VersionId and production package/owned Manifestation install mapping, including Luminous; per-Version 3D/2D catalogue entries remain empty until accepted. No fake UUID or ARTIFACT_READY.
2. **Gate01:** genuine author-approved presentation actor Blueprint per installed Version, complete modular skeletal rig/materials, original 2D illustrations when available. It must implement `ConfigureOwnedPreview(RulerId,IdentityId,VersionId,ManifestationId,bSfw)`; source/ownership validity and outfit must resolve from canonical store. No mutation from preview.
3. **Gate02:** genuine 100x100km world, map geometry and territory relief; current QA world is disposable; no fabricated production map.
4. **Gate07:** approved canonical banners/pools/currencies/tickets/economy state. Native Gacha remains truthful unavailable when no banner installed.
5. **Gate09:** accepted authored locomotion, jump/land, swimming/flying, movement blend, expressive/dance/scene clips, not available for native integration.
6. **Gate10:** nine final frozen visual UMG layouts/icons/rarity frame assets/opening animation source, beyond current functional native Ruler shell. The provided visual PNGs are references only; native editability exists but complete artistic implementation is not accepted. Android launcher icon resources remain external ZIP references; do not substitute.
7. **Gate04/05/08:** approved kits, narrative/dialogue, event/encounter content when supplied, must bind to existing executor/UI contracts, not synthetic production content.

## Acceptance still outstanding
- Verify no-cook build, full current-source native regression and runtime editor asset load; validate actual UI journey in PIE and genuine approved content wiring.
- Verify real S26 Android device only AFTER separate cook authorization. Do not call the current source ARTIFACT_READY or Android accepted prematurely.
- Keep two locally modified Koikatsu tracked files out of unrelated commits; never git reset/clean/stash or merge frozen Foundation.
