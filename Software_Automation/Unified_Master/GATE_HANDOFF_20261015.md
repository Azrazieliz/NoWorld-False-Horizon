# NoWorld — immediate software-master handoff (2026-10-15)

Branch: `software/mastery-integrated`. This is reusable **test-only** tooling, not approved game content.

## Verified right now

- Existing SM01 v0.8.0 master ZIP SHA256: `ff2275380a623386ebccf09a30e0651323ca82e700d9b2b538f34cd0848f04d0`.
- Existing master proof verifier: 10/10. Route preflight: 24/24. New runner fail-closed regressions: 9/9.
- Fresh four-software executable query pipeline passed, then resumed without rerunning: `D:/UnrealProjects/NW_BlenderGateHandoff_QA_20261015/FreshSmoke_20261015T191608394271/` (3.8 seconds total). **Lookup routing only**; not four simultaneous native GUI operations.
- User-made Koikatsu benchmark captured privately without touching source: `D:/UnrealProjects/NW_Blender_Private_Review_20261015/KK_UserBenchmark_20261015.png`.
- Existing Rakabi and Anti-Spiral candidate Blender sources, previews and reports retained under `D:/UnrealProjects/NW_Blender_Private_Review_20261015/`. Both are **visual likeness NOT ACCEPTED**; do not use their images as approved results.

## Gate 01 — character production: use user-authored native KK FIRST

1. Treat the current user-edited Anti-Spiral KK appearance as the benchmark, not the approximate Blender substitute. Save a **new copy** of the card before automated changes; preserve source hash and native Maker asset IDs.
2. Use the existing SM01 v0.8.0 tooling at `D:/Tools/GameAssetPipeline/ProductionAutomation/Software_Automation/MASTER_PACKAGES/KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME/`. Its `AUTOMATION/produce_from_reference_recipe.py`, `AUTOMATION/search_installed_mods.py`, `AUTOMATION/score_native_render_quality.py`, `AUTOMATION/export_card_materials_for_unreal.py`, `AUTOMATION/accept_universal_surface_v07.py` already exist. No reinstall.
3. Use SM01 native morphs, hair, gradients, cloth cutouts and mod catalog *before* requesting Blender residual geometry. The established direct style tests took ~5.652s and 8.57s, not full character design.
4. Only use Blender for geometry/rig/UV fixes that KK cannot achieve, then verified private native skinned zipmod round-trip. Compare **front, profile, back** with the 2D reference and KK user benchmark. Fail if silhouette, face identity, clothing or material is wrong. No acceptance from bone count alone.
5. Do not touch the original Steam installation; private KK assets stay out of public Git. User approval required before Gate11 canonical registration and Gate12 production import.

## Gate 02 — terrain

Use `Gaea_2.3.1_Master/gaea_graph_job.py` for isolated graphs and `gaea_blender_tiled_mesh.py` for tiled terrain meshes. Proven graph + native Gaea validation and 4 UE-reloaded StaticMeshes; **Unreal Landscape and World Partition not verified**. Do not steal focus from Gate02 active Gaea session. Maintain noncanonical QA outputs.

## Gate 09 — animations / 2D

Use `Blender_5.2.2/GATE09/` semantic skeletal Actions and animation-only FBX sources. Real moving bones required; camera-only motion is NOT accepted. Check foot contact, swim/fly sustain, facing axes and pose fidelity before handoff. Grease Pencil / sprite atlas are editable 2D source, not evidence of polished animation.

## Gate 12 — Unreal native authoring

Use the existing `Unreal_5.8.3_Master/native_asset_author.py` declarative editor authoring, indexed real 11,174 APIs, and QA project only. Native material params, 4-node/3-edge PCG graph, keyed Sequencer and generated Blueprint class have receipts. Niagara *system-only* is not a functional emitter; Blueprint generated class is not full compile/gameplay verification. Don't conflate editor assets with tested runtime or Android parity. First close true Blueprint compile, Niagara emitter, Landscape, Control Rig/IK, material parity and real Android QA gaps.

## Universal runner, quickstart

```powershell
$root='D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unified_Master'
$py='D:\Tools\Python311\python.exe'
& $py "$root\runner_regression_tests.py"
& $py "$root\route_preflight.py"
& $py "$root\fresh_four_route_smoke.py"
& $py "$root\gatectl.py" --job '<new unique versioned job>.json'
& $py "$root\pipeline_runner.py" --plan '<new unique versioned plan>.json' --preflight
& $py "$root\pipeline_runner.py" --plan '<new unique versioned plan>.json'
& $py "$root\pipeline_runner.py" --plan '<same plan>.json' --resume
```

Never rerun old output names; all default to test_only and refuse destination overwrite. Source hashes guard originals. A passing lookup/process exit is *not* a native gameplay result; require fresh explicit native postcondition for `NATIVE_VALIDATED`.

## Return-on-time rule

Do not spend further time indexing software already indexed. Choose a real reference and a saved native KK candidate; repair only evidenced deviations, retain its successful work, and judge value by end-to-end operations, editable assets, visual accuracy and time saved versus the user's approximately five-minute manual benchmark.
