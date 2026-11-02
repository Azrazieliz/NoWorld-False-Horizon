# Gate 12 — Genuine Koikatsu 3D QA viewer and World no-cook acceptance

**Date:** 2026-10-16. **Engine:** UE 5.8.3. **Source branch:** `production/gate12-koikatsu-ue583-fixture-20261010`. No Android cook, package, sign or install. Original `D:\UnrealProjects\Offline-Game` frozen Foundation is not modified, PR#16 remains unmerged.

## Verified in a REAL isolated Win64 game

**16/16 genuine World/Ruler runtime checks PASSED**, receipt [Runtime16of16.json](evidence/issue25/Real3DPreviewAndWorld_20261016/Runtime16of16.json). The executable game ran with a disposable copied project and separate SQLite save. The original database `Saved/OfflineGame/WorldState.db` remains 970,752 bytes with its original 2026-10-10 modification time.

Checks include real local player/Pawn/HUD, native Ruler UMG root/title/page/nav, canonical local Ruler restore, five native Ruler screens, exact **45 original imported CLOTHED Luminous skeletal meshes** with a live render target, viewer surviving 25 seconds of asynchronous shader preparation, native empty-roster World entry with original prototype Pawn, actual World mode over multiple ticks, return to Ruler with identical Pawn/Ruler ID and **zero** synthetic Manifestations or territory.

No 2D artwork is required for implementation; user plans to supply that separately. The Luminous viewport is **development QA only**, never a canonical item or an approved production visual catalog entry.

## Genuine, unaltered Unreal renderer evidence

| Capture | Evidence | Interpretation |
|---|---|---|
| Early 3D QA render | [Initial viewer PNG](evidence/issue25/Real3DPreviewAndWorld_20261016/ruler_characters_real_source_qa.png) | 45 real parts appear; incomplete materials while shaders/texture streaming warms |
| Warmed 3D QA render | [After warmup PNG](evidence/issue25/Real3DPreviewAndWorld_20261016/ruler_characters_real_source_qa_warm.png) | Genuine Luminous with long white hair and clothed silhouette clearly visible; dark body/material response still NOT approved fidelity |
| World gameplay | [World PNG](evidence/issue25/Real3DPreviewAndWorld_20261016/world.png) | Real render of StartingWorld and prototype Pawn, terrain, trees, movement/combat HUD; NOT final Gate02 map or character art |
| Return to Ruler | [Ruler PNG](evidence/issue25/Real3DPreviewAndWorld_20261016/ruler_after_world.png) | Actual composed Slate Ruler after real World-to-Ruler click handler |

The earlier all-black viewer resulted from freezing its first scene-capture frame before shader/texture compilation. The non-Shipping **QA viewer now captures live only while visible** and is destroyed on navigation. Canonical production preview thumbnail policy is unchanged; no shader or exporter changes were smuggled into this fix.

The older image named `world.png` in `NativeRulerFivePages_20261016` showed Ruler Home due an earlier same-tick screenshot-and-return race. The newly verified World snapshot separates screenshot and return across separate real game ticks. No world camera or lighting defaults were changed.

## Real, distinct blockers — no false acceptance

1. **P0 character fidelity:** body morph source 15 effective POSITION targets vs Unreal nine, facial morph effective losses, upper-eyeline 72.2651% fully transparent source pixels and gradient mishandled by opaque Unreal shader, and divergent 975-joint rest hashes across CLOTHED/UNDERWEAR/NUDE. Genuine source-export production-default change still requires Gate00 approval, gate-specific source/animation compatibility. A render appearing is not acceptance.
2. **World scope:** rendered StartingWorld is a prototype environment with placeholder mesh/presentation, not the approved complete kingdom, Gate02 terrain/caves/settlements/architecture content or final authored character visuals.
3. **Adult Mode:** native wardrobe/privacy separation and 13 logical systemic interaction participants remain source-tested, but user-requested visually embodied simultaneous 13 participants, interaction animation and Adult Mode gameplay controls are not demonstrated. Do not fake participant identities or authored clips.
4. **Gate10 UI:** existing native five Ruler screens function and are screenshot-verified, but frozen quality/detail for all nine art-directed screens, opening animations, full art and final responsive styling are not accepted from these small debug screenshots.
5. **Gate11:** production identity/version/manifestation art catalog is empty by design; cannot invent accepted content or ownership.
6. **Android:** No APK cooked, installed or tested, under user's explicit prohibition until further authorization.

## Verification

- Preceding full suite baseline was 150/150 native tests, **132 clean + 18 with warnings**.
- C++ incremental build after adding actual QA part inspection: `Saved/Logs/Gate12_Issue25_Real3DQAWorldFrame_Fixed_20261016.log`: **8/8 successful**.
- C++ after enabling live QA preview only during development and delayed 25-second evidence: `Saved/Logs/Gate12_Issue25_LiveQaShaderWarmup_20261016.log`: **6/6 successful**.
- **Latest Gate12 targeted native suite:** 10 clean + 1 warning + **0 failed**, Editor exit 0, no cook. [Actual verification summary](evidence/issue25/Real3DPreviewAndWorld_20261016/Native_targeted_verification_summary.json), [complete test index](evidence/issue25/Real3DPreviewAndWorld_20261016/Native_targeted_test_index.json), generated `Saved/Gate12Readiness/Native_Gate12_20261016_190628`.
- **Latest complete native suite:** 132 clean + 18 warning + **0 failed**, **150/150**, Editor exit 0. [Actual verification summary](evidence/issue25/Real3DPreviewAndWorld_20261016/Native_full_verification_summary.json), [complete test index](evidence/issue25/Real3DPreviewAndWorld_20261016/Native_full_test_index.json), generated `Saved/Gate12Readiness/Native_Full_20261016_190715`. Do not reinterpret the 18 warnings as visual approval.

## Gate 12 next scope

Preserve this verified implementation in Git, close native regression warnings only if genuine engineering defects, and continue honest feature integration with the existing Foundation. Do not churn already-accepted character design, invent missing art or merge frozen Foundation/PR#16.
