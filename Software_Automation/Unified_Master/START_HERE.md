# START HERE — NoWorld shared production-tool automation

> **MANDATORY NEW STARTUP (2026-10-16):** All future specialist Gates must load `MASTER_LEARNED_METHODS_V1.json` and `MASTER_EXECUTION_DOCTRINE_20261016.md`. Run `gate_start.py --gate NN` before action. Existing `gatectl.py` and `pipeline_runner.py` call `mastery_preflight.ensure_master()` automatically, preventing known methodology loss. Run `mastery_regression_tests.py` after learning or changing native-source paths. New knowledge is recorded with `record_learning.py`, not just mentioned in chat. All sample characters are disposable QA benchmarks, not accepted canonical assets. **Do not continue Test3 character refinement without a new user instruction.** The important return is robust Gates01–12 automation, speed and preservation of lessons. Older handoffs below may describe superseded capabilities and must not override the 2026-10-16 doctrine.

> **TEST 4 → TEST 5 MANDATORY (2026-10-17):** Read `TEST04_LESSONS_ONE_SESSION_GATE01_20261017.md` and K035–K037. Use `kk_one_session.py` to search the full catalog, stage relevant packages BEFORE one Maker launch, then send native edits as a persistent timed batch without repeated application restarts. The user's 6.7/10 Test4 rating and ~25-minute elapsed/~1-minute active estimate are recorded. Offline checks do not constitute a measured Test5 improvement.

**Branch:** `software/mastery-integrated` — do not merge into frozen production Foundation.
**Scope:** executable production *tools*, not creation of canonical characters/worlds or artistic approval.
**Windows host:** `LAPTOP-1LI4VRCJ`.

## Zero-click discovery, source-preserving jobs, automated checkpoints

```powershell
$root = 'D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation'
$python = 'D:\Tools\Python311\python.exe'

# Search actual installed APIs, procedural nodes and 27,849 indexed KK items
& $python "$root\Unified_Master\software_catalog.py" --search 'wing' --software koikatsu --limit 10
& $python "$root\Unified_Master\software_catalog.py" --search 'erosion' --software gaea
& $python "$root\Unified_Master\software_catalog.py" --search 'PCG' --software unreal
& $python "$root\Unified_Master\software_catalog.py" --search 'IK bone' --software blender

# Test installed command routing without starting heavyweight GUIs.
& $python "$root\Unified_Master\route_preflight.py"

# Run one approved, **new-ID, isolated** job with source hashes and native postconditions
& $python "$root\Unified_Master\gatectl.py" --job '<versioned-job.json>'

# Run a chain; preflight first, execute once, then resume after interrupted work.
& $python "$root\Unified_Master\pipeline_runner.py" --plan '<versioned-pipeline.json>' --preflight
& $python "$root\Unified_Master\pipeline_runner.py" --plan '<versioned-pipeline.json>'
& $python "$root\Unified_Master\pipeline_runner.py" --plan '<versioned-pipeline.json>' --resume
```

**Never run unchanged sample output names twice**: the system deliberately refuses overwrites.
`Unified_Master/JOB_SCHEMA_EXAMPLE.json` shows the single-job schema.
`Unified_Master/PIPELINE_EXAMPLE_FOUR_SOFTWARE.json` is a **previously executed example**. Copy it, change job IDs, destination folders and checkpoint location.
`Unified_Master/CAPABILITY_MATRIX_LIVE.json` is the machine-readable installed capability catalog.
`Unified_Master/ROUTE_PREFLIGHT_CHECKS.json`: 24/24 commands resolvable at current checkpoint.
`Unified_Master/LIVE_FOUR_SOFTWARE_ROUTER_SMOKE.json`: four actual lookup routes returned data.
The four-stage pipeline completed and a resume check independently succeeded.

## Software-specific entry points

| Software | Permanent operational owner | Master scripts and tested behaviors |
|---|---|---|
| **Koikatsu Party / SM01** | Gate 01 | The existing SM01 v0.8.0 master, native Maker/Maker cards, recipe executor, 27,849 existing item catalog; SM02's private native eight-bone wing mesh modification was packed into a KK zipmod, loaded, deformed and card-saved/reloaded successfully. Do **not** rebuild SM01's tools. |
| **Blender 5.2.2 / SM02** | Gates 01, 02, 09 | Headless modeling, UVs, morphology and skinned appendages, PBR adjustments, native Action/animation-only FBX, editable Grease Pencil source. 2,755 installed operator APIs mapped. |
| **Gaea 2.3.1** | Gate 02 | Native 16-bit graph, erosion/export, generated masks, 4-tile Blender conversion and UE mesh import. Native GUI build and export succeeded on isolated graph, but graph GUI automation is **specific to known layouts**, not robust to every arbitrary Gaea project. Gaea MCP 10 validated node schemas; others still need schema validation. |
| **Unreal 5.8.3** | Gate 12 | Actual native PCG graphs, parameterized material nodes, Sequencer with saved camera transform keys, native Blueprint compilation, Niagara base system, 2D PaperFlipbook, FBX static/skeletal assets, and real sampled AnimSequence pose validation. 11,174 exposed APIs mapped and searchable. Niagara emitter composition, Android builds and in-game behavior require further tests. |

Reference evidence is in the Gaea `VALIDATION` JSON reports and `D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015`. Native private KK geometry/cards must stay under the local SM01 integration fixture path; never commit or distribute proprietary source.

## Runtime/job safety

- All master operations remain `test_only:true` until each owning Gate authorizes production content; the API does **not** silently update NoWorld.
- Source paths are hashed before/after. All jobs need unique output folder, seconds timeout and isolated destinations.
- Gate tasks that take over the GUI (`gaea.native_build`, `koikatsu.native_wing_test`) require `requirements.dedicated_gui:true`; never interrupt a user's active software session.
- Any Unreal native asset destination must remain under `/Game/NW_MasterHub_QA/` within the isolated test project.
- The Studio OBJ bridge is an **optional static-prop route** and must not be confused with the tested native skinned KK Maker route. SM02 source now refuses to target the original Steam installation; only the isolated SM01 sandbox is allowed.
- A nonzero process exit or a missing postcondition is a failed job. A zero exit without a native receipt is only `PROCESS_EXIT_ZERO`, not proof of gameplay functionality.
- Asset import is not evidence of matching morph targets, physics, textures, deformation, responsiveness, framerate or aesthetic quality. Production/Android parity still requires Gate 12 testing.

## Gates should start producing their content, not another software survey

Gate 01: approved reference → SM01 existing mod asset discovery/native recipe → Blender edits where KK insufficient → native skin/mod roundtrip → review views → Unreal test. **Full automatically faithful 2D-to-3D has not been accepted**; let the user's forthcoming difficult test drive the missing reconstruction steps.

Gate 02: original world spec → Gaea graph+build → Blender architectural/nonheightfield details → UE Landscape or StaticMesh/PCG. Only StaticMesh terrain tiles are natively verified in the existing QA; **native UE Landscape/World Partition remains untested**. Gate 02 owns the world's actual geography.

Gate 09: authored/mocap-source animation → Blender Action+skeletal FBX → Gate12 IK retarget + actual movement fidelity checker. Synthetic basic flight/swim poses have moving bones; **professional movement/contact is not accepted**.

Gate 12: use native engine PCG, Blueprint, Sequencer, Niagara, GAS, AI, physics, rendering and runtime systems as applicable. The `UE58_ExposedAPIs_Installed.json` provides exact exposed Python calls; some capabilities require C++ or Blueprint rather than Python, and should use appropriate engine features—not replace Unreal with offline mockups.

## Readiness is not cosmetic acceptance

This system is ready to receive **new user-supplied functional/quality tests**. It is not a claim of exhaustive mastery or zero defects. No Rakabi character was reconstructed. Keep true creative output with Gates and validate each requested operation directly.
