# Gate 12 -> Gate 00: source integration review (2026-10-16)

Project: **NoWorld: False Horizon**, private Android-first, Unreal Engine 5.8.3.
Working branch: `production/gate12-koikatsu-ue583-fixture-20261010`.
Frozen separate Foundation: `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`.
Status requested: **Gate 00 review of native source integration and known blockers,
NOT an Android cook authorization and NOT ARTIFACT_READY**.

## 1. Immutable decisions and protected source

- Never reset/clean/rebase/merge Foundation or delete two existing locally modified
  `OGKoikatsuPlayableCharacter.cpp/.h` files or other untracked character resources.
- Acquisition of the same Identity again gives a separately owned Manifestation.
  Character chips/conversion rewards were a reporting error, not a subsystem.
- Assistant-proposed E03/E04 tasks removed. Do not resurrect them.
  Existing independent frozen adult-system architecture is not deleted.
- Gate 09 owns the authored animation/dance source. Gate 12 owns Unreal runtime
  movement, retarget interfaces, source import and later Android validation.
- 2D original art may be supplied later. That must not block source integration.

## 2. Verified independently owned runtime/source

- Canonical SQLite local Ruler creation/restoration, real five-destination
  Home/Characters/Gacha/Territory/Records projections and Ruler/World mode.
- Fully native editable `WBP_NoWorld_RulerShell.uasset`, real
  `DA_NoWorld_CharacterVisuals.uasset` and safe-area render components.
  Native HUD verifies local PlayerController before creating any viewport UMG.
- Three-column roster, 0.445 card ratio; canonical Identity/Version and
  independent Manifestations; search, sort, selected-owned-copy persistence.
- 2D versus genuine live 3D preview per owned Manifestation, locally persisted
  without changing canonical world/wardrobe/equipment/ownership. 2D is optional;
  missing art never replaced with a fake.
- Per-Version asset catalog maps exact Identity/Version to original 2D and SFW
  2D, approved 3D display actor, pre-authorized wardrobe fields/options, and
  approved AOGWorldPrototypeCharacter-derived playable class.
- Preview actors must implement approved read-only owned/presentation contract;
  no generic mannequin. Scene capture refuses an invalid binding.
- Character-select -> World spawn seam validates live owner, active
  Identity/Version/Manifestation and exact authored playable actor interface.
  A failed pawn binding rolls back to Ruler without destroying the old pawn.
  Without selected owned character, the Ruler's normal playable baseline remains
  valid; this does not synthesize ownership. **Real approved package validation
  of this seam remains a separate requirement.**
- Version-authorized wardrobe options persist via existing canonical
  presentation record without modifying Privacy settings or unrelated JSON.
- Reports list/priority and owner-verified persistent acknowledgements;
  Chronicle/Intelligence knowledge-gated; Gacha real event history only.
  Absent authorized banners or currencies are locked/empty, never invented.
- Source-only pre-cook audit, full/native regression runners, UI asset authoring
  and validation scripts; exact-hash Android icon installer which NEVER cooks
  and refuses a modified approved ZIP.

## 3. Required proof and acceptance distinction

First-party no-cook build:
`D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat OfflineGameEditor
Win64 Development -Project=D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\OfflineGame.uproject
-WaitMutex -NoHotReload -MaxParallelActions=1`

Automated Gate12 native:
`powershell -ExecutionPolicy Bypass -File Scripts\Gate12Koikatsu\run_gate12_pre_cook_native_verification.ps1 -Scope Gate12`

Automated full suite: same command with `-Scope Full`.
Read-only source audit: `python Scripts\Gate12Koikatsu\gate12_pre_cook_integrity_audit.py`.

**Verified after the owned-World-pawn integration, on current source:**
- Win64 Unreal Editor build **11/11 actions, successful DLL linkage, exit 0**.
  Log: `Saved/Logs/Gate12_20261016_OwnedWorldPawn_Build.log`.
- `OfflineGame.Gate12`: **8/8 clean, 0 warning passes, 0 failures**.
  Report: `Saved/Gate12Readiness/Native_Gate12_20261016_104602/index.json`.
- Complete `OfflineGame`: **147 tests = 130 clean successes +
  17 warning passes + 0 failures**, editor exit 0.
  Report: `Saved/Gate12Readiness/Native_Full_20261016_104720/index.json`.
- Full actual approved-owned-pawn possession, Gate10 visual equivalence,
  actor wardrobe fidelity and Android device tests remain unproven without
  the accepted authored packages; the test pass is source/runtime logic only.

No physical Android test or APK of this source branch exists.

## 4. Outstanding Gate handoff assets (cannot be fabricated)

1. Gate 11 must accept installed canonical identities/versions, Manifestation
   ownership, real Luminous production package and visual/player IDs; current
   native visual catalog has zero authorized entries.
2. Gate 01 supplies exact original 2D art when available and genuine imported
   Koikatsu 3D authored preview/World pawn Blueprints. Both presentation
   contracts require canonical owner/Version/wardrobe and privacy binding.
3. Gate 10 supplies FINAL native screen art/layout layers, approved title
   animated opening and Android icon resources. Current widget is functional,
   but is not a faithful complete reproduction of all frozen concept PNGs.
   PNG screenshots must not become fake clickable UI backgrounds.
4. Gate 02 must deliver approved first world/terrain/territory maps. Current
   Foundation/QA test map is NOT the accepted first world.
5. Gate 07 must deliver approved real banners, pools, economies, gear inputs,
   item IDs and legal draw contracts before normal Gacha operation.
6. Gate 09 must deliver authored locomotion correction, swim/fly, jump/IK,
   expressive movement, dance and scene animation source. Gate 12 can integrate
   upon delivery; it cannot represent frozen placeholder poses as completion.
7. Final user-visible UI flow, performance, character/morph/material fidelity,
   wardrobe state and all Android controls require a separately authorized
   actual device package/test phase after Gate 00 authorizes cooking.

## 5. Review request / Gate 00 decision

Please decide **SOURCE_ACCEPT / REVISE / HOLD** for the compiled, test-verified
Gate12 native integration and its interface contracts *only*. Preserve frozen
Foundation and existing manifest authority. Record the missing cross-Gate
artifacts separately; commission their owners rather than fabricating stand-ins.
If SOURCE_ACCEPT, acknowledge this as a non-cook checkpoint and authorize
the appropriate cross-Gate intakes. Do NOT call ARTIFACT_READY, do not merge
PR #16 into Foundation, and do not authorize an APK cook, Android deploy, save
rewrite or release until the user explicitly requests that later operation.

The existing approved 2D artwork may be added later without repeating the
native roster/presentation implementation.
