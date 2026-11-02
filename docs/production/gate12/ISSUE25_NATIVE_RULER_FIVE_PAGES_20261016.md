# Gate 12 — five native Ruler screens and World round-trip (verified)

Date: 2026-10-16. Worktree `production/gate12-koikatsu-ue583-fixture-20261010`. Source derives from frozen Foundation `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`. No Foundation mutation, PR #16 merge, Android cook/installation or original saved world access.

## Source implementation
- The native Ruler UMG **ENTER WORLD** handler now permits an authentic new Ruler with no selected owned Manifestation to enter the real World via the existing prototype Pawn. If an owned Manifestation IS selected, it still requires exact approved visual catalog actor/version resolution. No synthetic character or ownership record is generated.
- The opt-in isolated runtime journey now verifies native Ruler **Home, Characters, Gacha, Territory and Records** through actual widget actions before entering World through the native widget and returning to Ruler. Slate visibility considers `SelfHitTestInvisible` genuinely child-interactive, while `Hidden`, `Collapsed` and `HitTestInvisible` remain excluded.
- The `run_isolated_nocook_desktop_smoke.ps1` runner supports `-Journey -Onscreen` and records actual no-cook Slate-inclusive screenshots (real engine `FScreenshotRequest::RequestScreenshot(..., bInShowUI=true)`), independent of the earlier misleading HighResShot result.

## Actual verified result
- Build: `Saved/Logs/Gate12_Issue25_FiveRulerPages_EmptyRosterWorld_20261016.log`: **6/6, Succeeded**.
- Run: isolated Win64 Unreal 5.8.3 `-game` with genuine UI, its own `Saved/OfflineGame/WorldState.db`, no fatal error. Original source game's database remains at 970,752 bytes, dated 2026-10-10.
- Durable real runtime receipt: [14 of 14 pass](evidence/issue25/NativeRulerFivePages_20261016/Runtime14of14.json). `complete=true`, `passed=true`, 14 separate steps including all five UI pages and persisted local Ruler + Pawn consistency after World return.
- Genuine screenshots:
  - [Home](evidence/issue25/NativeRulerFivePages_20261016/ruler_nav_home.png)
  - [Characters](evidence/issue25/NativeRulerFivePages_20261016/ruler_nav_characters.png)
  - [Gacha](evidence/issue25/NativeRulerFivePages_20261016/ruler_nav_gacha.png)
  - [Territory](evidence/issue25/NativeRulerFivePages_20261016/ruler_nav_territory.png)
  - [Records](evidence/issue25/NativeRulerFivePages_20261016/ruler_nav_records.png)

The five Ruler page shots are genuinely different composed UI screenshots. However, the frame named `world.png` still resembles Ruler Home, despite the successful World-mode/Pawn transition and three-second World-state verification. Thus **real World environment visibility/camera remains visually unaccepted**. It may be screenshot timing or actual presentation; do not assume either. The existing Ruler UI is a functional native shell, not acceptance of all frozen Gate10 visual specifications.

## Exactly remaining
1. Open development-only **real imported 45-part Luminous** viewer in Characters and capture/test its actual viewport render target. This must NOT create canonical ownership or spawn the QA gameplay Pawn.
2. Demonstrate genuine World environment visuals, live camera, movement and Ruler return frames; investigate home-like `world.png` despite mode pass.
3. Run current-target targeted + complete native suite after this source increment. Previous 150/150 native baseline predates the new empty-roster fix and expanded journey test.
4. Adult Mode visible interaction and 13 simultaneous embodied/animated characters remain unproven; keep accepted logic and Privacy/wardrobe separation and coordinate approved source animation without inventing clips.
5. Production character/wardrobe catalog cannot be populated without real Gate11 accepted Identity/Version/Manifestation data; standalone QA preview does not substitute it.
6. Android cook/install remains explicitly withheld pending new authorization.

## Next action
Verify the real 45-part QA viewer within the current isolated Win64 game, then re-run no-cook native tests. Do not touch frozen Foundation, PR#16, original saves or character art approval.
