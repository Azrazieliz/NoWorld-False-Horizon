# Gate 00 — P0 Software Mastery targeted follow-up
**Date:** 2026-10-16
**Status:** Proven source-level corrections / unresolved Unreal and native-assembly parity.
**Production decision:** HOLD; DO NOT SWITCH DEFAULT EXPORTER.
**Scope:** Research and isolated automation, not permission to create/migrate canonical characters.
**Evidence:** `D:\UnrealProjects\NW_SoftwareMaster_P0_20261016` (QA only).

## Start here, don't redo four-family inventory
- Master `MASTER_EXECUTION_DOCTRINE_20261016.md`, `MASTER_LEARNED_METHODS_V1.json`; now 33 cumulative lessons K001–K033, no old procedure overwritten.
- Local QA `SOURCE_IMMUTABLE_INPUT_MANIFEST.json`: Base CLOTHED/UNDERWEAR/NUDE + three separately hashed Alter candidates; first Alter was modified 2026-10-16 and cannot be remapped by clothing counts alone.
- `P0_TARGETED_EXACT_TOOL_AND_SOURCE_PINS.json` hashes installed Blender 5.2.2, original KKBP 5.6.0.0, sandbox KKBP (different DLL hash even though same version), source Blend+GLBs and original runtime.
- Previous full P0 four-family survey and Gate12 physical findings stay in force.

## NEW measured source->GLB->clean-Blender morph evidence
For **matching original Blender / GLB / clean Blender GLTF reimport** revisions of Luminous Base:

| Appearance | Original Blender shape-key names | Actual effective shape keys | Effective GLB named position deltas | Effective on clean Blender GLB reimport | Lost at these boundaries |
|---|---:|---:|---:|---:|---:|
| CLOTHED | 2,160 | 247 | 247 | 247 | **0** |
| UNDERWEAR | 2,174 | 242 | 242 | 242 | **0** |
| NUDE | 2,174 | 242 | 242 | 242 | **0** |

- All effective names matched, target indices were consistent across material primitives, and the median maximum position delta ratio was exactly 1.0. Source effectiveness was measured by basis subtraction and a 1e-7 threshold; GLB effectiveness by decoding real binary accessors, INCLUDING sparse indices and values; reimport effectiveness by re-measuring real Blender keyblocks after a fresh GLTF import.
- GLB CLOTHED has **5,556** POSITION morph accessors (5,237 sparse), but only **595 nonzero accessors**, aggregating to **247 effective unique mesh+name targets**. 4,961 accessors have zero effective deltas; many come from placeholder or irrelevant morphs replicated on other material primitives.
- UNDERWEAR/NUDE each have **5,518** position accessors, 5,199 sparse, and **242** effective unique mesh+name targets. CLOTHED's extra five effective targets belong to outfit-dependent deformations; comparison must use each matching source state.
- **Interpretation:** The archived Blender-to-GLB and fresh Blender GLB importer **did not lose the measured effective position morphs**. Gate12's separately observed missing Unreal morph effect is REAL but cannot be attributed to missing GLB accessor data in this source. Next check exact UE imported morph target name, vertex delta amplitude, target index, mesh/primitive splitting, importer sparse handling, LOD0, skeletal binding and per-state rest transforms. A zero morph-threshold setting alone will not restore missing effective deltas.
- **Limit:** No native Koikatsu assembled-source morph count was measured. Gate12 must not extrapolate Blender/GLB parity backwards to the live Party renderer.

**Primary artifacts:** `P0_BLEND_SOURCE_EFFECTIVE_MORPHS_CLOTHED/UNDERWEAR/NUDE.json` (CLOTHED legacy companion: `P0_BLEND_SOURCE_EFFECTIVE_MORPHS.json`), `P0_GLB_EFFECTIVE_SPARSE_MORPH_DELTAS.json`, `P0_MORPH_SOURCE_TO_GLB_NAME_DELTA_PARITY.json`, `P0_BLENDER_GLB_REIMPORT_EFFECTIVE_MORPHS.json`.

## NEW source eyeline material semantics
- GLB material **`KK cf_m_eyeline_00_up`** explicitly has `alphaMode=BLEND`, not OPAQUE, for CLOTHED, UNDERWEAR, NUDE.
- Source PMX/KKBP upper-eyeline native exported `cf_m_eyeline_00_up_MT_CT.png` contains partially transparent pixels (~2.06%). In the downstream GLB embedded image, partial alpha comprises **15.737% CLOTHED / 17.878% UNDERWEAR / 17.878% NUDE**. These images are not shown as pixel-identical: packaging/baking changed the alpha distribution but **retained gradients**.
- Down and kage eyeline exported MT_CT atlases show alpha zero; do not override with opaque default or infer visibility from map presence alone.
- Archived GLB also marks down/kage eyeline material `BLEND`.
- **Interpretation:** Gate12's opaque Unreal upper-eyeline is incompatible with verified archived upstream `BLEND` and nonzero alpha gradient. An Unreal-native translucent material that samples the correct texture alpha is required, with documented sort/depth behavior and independent under-eye/hair blend QA. Normal/eye/toon shader parity beyond alpha remains unaccepted.
- **Source contract:** `P0_GATE12_MATERIAL_AND_EFFECTIVE_MORPH_CONTRACT.json`.
- **Evidence:** `P0_NATIVE_GLB_EYELINE_ALPHA_AUDIT.json`, `P0_NATIVE_GLB_EYELINE_EMBEDDED_COMPARISON.json`.

## Earlier P0 evidence still valid
- Blender Test3 importer with ZERO source image nodes/datablocks was repaired using actual original PNGs: **48 loaded image datablocks, 51 texture nodes, 46 matched materials**, with geometry/rig untouched; no native hair/toon material acceptance.
- Source Luminous V3 had 121 loaded images, 57,221 vertices, 92,505 triangles, 975-bone armature, 2160 stored shape keys. Exported GLB had 40 embedded images, skin 975 bones, and remained visually degraded in controlled material review.
- Source overexposure diagnosed by matched-camera full face clipping **67.35% → 0%** with AgX/lower illumination, unchanged geometry. Thus camera/exposure effects and actual material/morph losses must be treated separately.
- Gate12 independently confirmed body physical dimensions, FOV/zoom effects, eyeline opaque material, Unreal effective morph loss and cross-wardrobe skeletal rest/hierarchy differences.

## Protected native assembled-capture status — BLOCKED, NOT VALIDATED
- Previously compiled, QA-only Unity3.5 `P0_SM01RuntimeController_Unity35_QA.dll` and source `p0_native_assembled_probe.cs` exist. This is a compiled PROTOTYPE, not a successful live assembled renderer capture.
- `p0_protected_native_live_try.ps1` verifies cloned source SHA, DLL backup, and no active user Party process before staging. The **original Steam Party PID 17728 was running**; the attempt halted BEFORE QA plugin deployment or Maker manipulation. It did not interrupt the user's program. Original sandbox DLL hash confirmed restored.
- Do not silently run the protected capture against original Steam Party or mutate its character. Retry only when the user-controlled game is not active and protected clone source readiness is verified; require actual runtime skinned meshes, vertex weights, mesh morphs and shader material properties for both Base and Alter, with a real receipt. If native-card plugin resolves incomplete appearance mods, fail rather than asserting parity.

## Four families, narrowed decision
- **A corrected KKBP:** working source PMX, Blender, GLB provenance and texture-link repair, effective morphs retained to fresh Blender GLB reimport; material shader fidelity and Unreal morph behavior FAIL or unverified. Current provisional baseline only.
- **B native capture:** compiled and source-protected prototype; not live-validated because original Party active. Cannot nominate yet.
- **C alternative importer/exporters:** AssetStudio/AssetRipper present in archive, but no identical-revision skinned/morphed assembled-source parity proof. Static meshes don't qualify. No proven replacement.
- **D hybrid:** plausible integration of original native shader maps + KKBP rig and morphs, but no matched improved native+UE render. No approved hybrid default.
- **No matched Base and Alter native/UE full-body/face/hair rendered improvement. DO NOT rank a new export default.**

## Reusable scripts and validations
- `p0_luminous_source_inventory.py` — original source/card hash+state audit.
- `p0_pmx_geometry_audit.py`, `p0_glb_audit.py` — original PMX/GLB stage stats.
- `p0_blender_all_effective_morphs.py`, `p0_effective_glb_morph_deltas.py`, `p0_morph_source_to_glb_parity.py`, `p0_blender_glb_reimport_effectiveness.py` — effective morph parity with sparse binary decode, exact names, deltas and import.
- `p0_targeted_eyeline_semantics.py`, `p0_eyeline_embedded_alpha.py`, `p0_gate12_material_contract.py` — source image alpha, GLB embedded-alpha, source-driven UE material contract.
- `p0_restore_kkbp_textures.py`, `p0_controlled_blender_views.py` — corrective texture mapping and controlled review (archived P0).
- `p0_native_assembled_probe.cs`, `p0_compile_native_qa.ps1`, `p0_protected_native_live_try.ps1` — protected native capture prototype (guarded, NOT tested live).
- `p0_export_version_pins.py` — exact tool hashes and source stage hashes.
- `p0_targeted_regressions.py` — 37/37 source-level checks.
- Permanent registry lessons K030–K033 appended after existing K001–K029; automatic startup `mastery_preflight.py` retained.

**Never confuse these 37 source-level checks with Unreal-native, renderer, Android or canonical art acceptance.**

## Gate 12 concrete action contract
1. Take `P0_GATE12_MATERIAL_AND_EFFECTIVE_MORPH_CONTRACT.json` and clone test assets into isolated **NO-COOK** UE5.8.3 QA. Keep source material slots.
2. Replace **opaque** upper-eyeline with a real alpha-preserving blend (source `BLEND`, embedded gradient). Test masked v. translucent only with evidence; default opaque cannot reproduce source.
3. For named nonzero source morphs, audit actual UE skeletal mesh's LOD0 deltas rather than UI target presence; check sparse-accessor import, target indices after material splitting, bind/rest pose and animation application. Reject zero effect regardless of import threshold.
4. Verify per-state rig hierarchy, bone names, bind/rest transforms and current owning Manifestation before retarget/merge; Gate11 Registry authority unchanged.
5. Render matched full-body/face/hair frames under controlled camera/FOV/light/tone matching native Koikatsu and the source Blender. Record physical geometry height separately from screen size.
6. Return Unreal delta readback, material readback and screenshots for Gate00 selection. Do not silently edit frozen Foundation or cook Android.

## Gate 01 source handoff
- Exact Base and current Alter original hashes in `SOURCE_IMMUTABLE_INPUT_MANIFEST.json`; Alter candidates cannot be named states by active clothing count only.
- Preserve updated Alter source revision from 2026-10-16 and the prior accepted Base V3 ZIP/intake. No migration overwrites, no redraw or redesign, no Registry promotion without Gate11.
- Validate real assembled 3D appearance using a protected clone once original Party is not user-active. The QA native capture prototype has not yet passed.

**Gate00 decision: continue P0, retain corrected KKBP as the provisional baseline, do not approve a pipeline switch.**
