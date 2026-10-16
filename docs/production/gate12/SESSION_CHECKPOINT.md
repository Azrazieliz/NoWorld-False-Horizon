# Gate 12 — Crash-safe executable checkpoint

Updated: 2026-10-16 Europe/Paris
Work item: P0-APP / Issue #25, with independent P0-FIDELITY hold
Status: ACTIVE — new native integration under Win64 build, not yet accepted
Working branch: production/gate12-koikatsu-ue583-fixture-20261010
Last verified predecessor HEAD: cdcb22bf3f4541e7334b27768d9f43bcfc4a18df
New source commit: NOT YET VERIFIED OR PUSHED AT INITIAL CHECKPOINT (update after actual test)

## Recovery FIRST ACTION (mandatory after conversation failure)
1. Read https://github.com/Azrazieliz/Offline-Game/issues/25 and its four linked documents on coordination/gate00-live-ops-20261016; obey later explicit direct user instruction over older board text.
2. On PC `D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010`, inspect `git log -1 --oneline; git status --short`, then this file. DO NOT git reset/clean/stash/rebase or merge PR#16. Frozen Foundation is a distinct worktree at `D:\UnrealProjects\Offline-Game` / `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`.
3. Preserve the pre-existing tracked modifications to `Source/OfflineGame/Private/Koikatsu/OGKoikatsuPlayableCharacter.cpp` and `Source/OfflineGame/Public/Koikatsu/OGKoikatsuPlayableCharacter.h` (not this commission). Protect untracked original Koikatsu characters, GLBs, uassets, APKs and user saves.
4. Read `Saved/Logs/Gate12_20261016_Issue25_VerticalSliceQaViewer_Win64.log`, resolve any actual compiler failures, and continue *from this source* rather than redoing the accepted native Ruler/World architecture. Command: `D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat OfflineGameEditor Win64 Development -Project=D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\OfflineGame.uproject -WaitMutex -NoHotReload -MaxParallelActions=1`.
5. If Win64 source compiles, run `Scripts/Gate12Koikatsu/run_gate12_pre_cook_native_verification.ps1 -Scope Gate12`, then `-Scope Full`; inspect actual index.json and all failures rather than repeating historical 148-test counts.
6. Inspect real native UMG in PIE/no-cook with baseline Ruler → Characters QA viewer → World → Ruler; record screenshots/logs from actual run. Save substantive source and tests in this Gate12 branch, push; update checkpoint with exact SHA and next action.

## Objective and operating rules
- Current user instruction: **NO ANDROID COOK, PACKAGE, SIGN, OR INSTALL** until separately authorized. Issue #25 linked board mentions a standing authorization; the current explicit user request is narrower and controls this session.
- Primary target: functional pre-cook Ruler → honest real imported noncanonical test viewer → World movement/camera → return/save. No invented Identity/Version/Manifestation, new character chips, E03/E04 resurrection, Gate11 ARTIFACT_READY or unapproved source-export defaults.
- Adult Mode and 13-logical-participant *visible test* requirements are retained for next app; prior native 13-participant test is not a rendered 13-character scene.
- Luminous 2D/3D creative approved: never reopen design; Gate10 screens frozen, Gate09 owns authored animation.

## Verified state at session start
- GitHub Gate12 HEAD `cdcb22bf3f4541e7334b27768d9f43bcfc4a18df`, source-accepted native Ruler/World/UMG and P0 fidelity diagnostic package.
- Previously verified Win64 P0 build and 148 total Unreal native tests (130 clean, 18 warning, 0 failures). This proof predates CURRENT unverified modifications.
- Issue25 and all four linked operating, recovery, board, machine-state files read from coordination branch. Software Mastery `gate_start.py --gate 12 --compact` PASS; preflight 33 rules /12 sources PASS; Mastery regression **34/34 PASS** on this session. Neither is visual or Android acceptance.
- Current P0 camera/lighting evidence shows source body bounds match UE; generic glTF opaque shader fails upper-eyeline alpha; morph effective 15→9 and zero-threshold test failed; cross-state skeletons incompatible. Production exporter hold remains.
- Immutable original Alter cards and V3 baseline have receipt at `Saved/P0CharacterQuality/source_preservation/preservation_receipt.json`. Keep this receipt private and preserve original Party cards.

## Actual local advancement, not yet accepted
- `Source/OfflineGame/Private/World/OGStartingRegionPresentation.cpp`: legacy Canvas World-entry now delegates to exact same validated native UMG World-pawn binding/rollback handler. Prior Canvas path bypassed selected-owned-pawn binding.
- `Source/OfflineGame/Public|Private/UI/OGGate12CharacterPreviewStage.*`: added development-only allowlisted exact existing CLOTHED Luminous QA Blueprint render route, passive/no input/no fake owner, explicitly rejects shipping and every other class.
- `Source/OfflineGame/Public|Private/UI/OGGate12NativeRulerWidget.*`: empty honest character roster now offers explicitly labeled `DEVELOPMENT QA — NOT OWNED` genuine 45-part model viewer; clean close, one-shot render target, retains canonical ownership exactly.
- `Source/OfflineGame/Private/Tests/OGGate12DevelopmentQaViewerTests.cpp`: regression rejects null/arbitrary/native classes and verifies real allowlisted QA asset exists; not compiled/verified when initial checkpoint written.
- Protected two Koikatsu C++ files remain untouched and unstaged. Old large unrelated untracked P0 materials/artifacts must not be mass staged.

## Next SINGLE executable action
**Read the currently running Win64 build log and repair first real compile error if any.** Then run targeted/full no-cook native suite and commit/push this actual work. Do not launch parallel Unreal builds on 8 GB RAM.

## Blocked on
- Production character binding and genuine accepted Manifestations: Gate11, separate from engineering QA viewer.
- Final adult authored participant animation/visible controls and 13 rendered actor demonstration: Gate09/Gate01/Gate12 integration, source 13 logical test ≠ visual pass.
- Android device acceptance: explicit later authorization to cook/install, no approval in current user message.
- P0 texture/morph/skeleton QA remains separate, do not wait on it for isolated Ruler/World runtime engineering.

## Replacement-conversation prompt
"Continue Gate12 from production/gate12-koikatsu-ue583-fixture-20261010 and docs/production/gate12/SESSION_CHECKPOINT.md. Read Issue #25 and current coordination docs, inspect real HEAD/worktree, preserve protected files, run the stated next single action and commit/push tests. Do not restart visual design or cook Android."
