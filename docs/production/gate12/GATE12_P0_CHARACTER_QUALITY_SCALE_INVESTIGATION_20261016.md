# Gate 12 — P0 Character Fidelity and Perceived Scale (2026-10-16)

**NoWorld: False Horizon** · Unreal Engine **5.8.3** · Android-first private game

**Authority:** Gate 00 investigation request; already SOURCE_ACCEPTED Gate 12 architecture is not being rebuilt. **Production export pipeline switching requires a separate Gate 00 decision.** NO Android cook/package/install.

## 1. Snapshot and preflight

- Foundation remains frozen at `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`, separate from Gate 12 worktree.
- Gate 12 branch started P0 from `f3fbe2fbbb0ca4fac2f661045757b5a380d56952`; pre-existing locally modified Koikatsu C++ source is protected. Preserve untracked character assets.
- Software Mastery `gate_start.py --gate 12` PASS; mastery preflight 28 methodology rules PASS; mastery regression **26/26 PASS**.
- Script `Scripts/Gate12Koikatsu/p0_preserve_native_alter_cards.py` snapshotted three user-authored Alter native Party PNG cards plus approved Luminous V3 baseline ZIP into `Saved/P0CharacterQuality/source_preservation`, hashed source and backup, never edited original.
- Newly modified Alter candidate saved October 16 had its current bytes captured; **do not substitute October 13 outdated hash**. Native Koikatsu morph comparison: face/body/hair unchanged between three candidate cards, only first coordinate clothing/accessory edits; six untouched inherited coordinate presets are **not approved** new appearances. Gate01 must verify state assignment visually, from native KK or reliable provenance.

## 2. Source skinned GLB, measured

| Source V3 state | Primitive vertices | Triangles | Images | Materials | Skinned joints | Rest-scene Y vertical |
|---|---:|---:|---:|---:|---:|---:|
| CLOTHED | 58,930 | 91,933 | 40 | 45 | 975 | 152.172 cm |
| UNDERWEAR | 49,309 | 77,832 | 34 | 39 | 975 | 150.936 cm |
| NUDE | 41,874 | 63,695 | 32 | 37 | 975 | 150.883 cm |

These are source GLB primitive sums; not total Unreal LOD0 or posed head-to-foot measurements. Every primitive has normals, UV0, JOINTS_0 and WEIGHTS_0; **none has imported glTF TANGENT**. The three `BLENDER_FRESH` and `BLENDER_ROUNDTRIPS` GLBs in existing evidence are SHA-256 *byte identical*, so no difference is introduced between those saved copies; this alone does not validate the original KKBP conversion, shader baking, texture mapping or Unreal import.

Material modes: CLOTHED 41 BLEND + 4 OPAQUE, UNDERWEAR 35 BLEND + 4 OPAQUE, NUDE 33 BLEND + 4 OPAQUE. Do not blindly convert gradient-alpha hair/eye layers to MASKED or switch all materials to OPAQUE.

## 3. Real native Unreal body LOD0 (no cook)

The actual `/Game/Experimental/LuminousValentineV3/{state}/Parts/Mesh003Prim011/Mesh_03_Prim_11` SkeletalMesh was loaded in Unreal Engine 5.8.3:

| Metric | Source glTF body primitive | Native Unreal body |
|---|---:|---:|
| Vertex count | 9,604 | **9,996** |
| Triangle count | Source value separately available in primitive accessor | **16,754** |
| Width | 123.82998 cm | **123.82998 cm** |
| Height | 128.14289 cm | **128.14289 cm** |
| Skeletal rig | Skinned | SkeletalMesh + skeleton |
| LODs | Source mesh data only | **1 LOD** |
| Body base texture | 4096×4096 | **4096×4096 editor source/built** |
| Body morphs (effectively moving POSITION) | **15** | **9** |

LOD0 values apply to this one body primitive for all three states, NOT the full 45-part character or final actor.

**Interpretation:** source→Unreal *body* width/height survives unit scaling; a blanket cm/meters conversion shrink is **not demonstrated here**. The 9,996 vs 9,604 count may reflect material split/vertex welding/normal splits, not necessarily more anatomical details; visual QA still needed.

Texture dimensions in `UTexture2D.Source`/Unreal editor are 4096×4096. Under headless `-NullRHI`, calling a runtime texture-resource size accessor returns zero; regression must inspect editor-source texture dimensions instead. This is a test measurement distinction, NOT evidence that native texture data is absent.

UE body import settings: recompute tangents ON, MikkTSpace ON, normals recomputation OFF, morph-position threshold **0.015 cm**, full-precision UV OFF, high-precision tangent OFF, high-precision skin weights OFF. Body source 15 effective nonzero POSITION morphs, 9 above `0.015 cm` threshold; six are missing in native UE import **even when the morph threshold is zero**; threshold-only repair failed. Source nominal 167/173 shape-key slots cannot be treated as 167/173 actual moving targets: previous byte-level QA proves most have zero POSITION deltas. **Crucially, historical isolated UE zero-threshold reimport for body index 016 still retained only 9/15 nonzero POSITION morphs.** The default 0.015cm threshold may contribute, but lowering it to zero did NOT repair these six losses. Earlier dense expansion of sparse accessor source also failed for the monitored morphs. Exact UE import morph-loss cause remains unknown.

## 4. Rest/bind pose Gate09 hard blocker

975 skinned joints in each of the three GLBs do **NOT** imply interoperable skeletons:

| States compared | Shared joint paths | Paths only in each | Rest mismatches on shared paths |
|---|---:|---:|---:|
| CLOTHED ↔ UNDERWEAR | 782 | 193 | 56 |
| CLOTHED ↔ NUDE | 782 | 193 | 89 |
| UNDERWEAR ↔ NUDE | 782 | 193 | 38 |

The exact ordered names, paths and rest-transform hashes differ. Gate09 must inspect bind/rest alignment, full physical rig, inverse bind matrices, morph/animation deformation and determine whether genuine animation retarget or canonical rig normalization is needed. No shared-975 skeleton acceptance or silent reskinning/renaming is authorized.

## 5. Geometry size versus apparent screen size

Source unposed character rest-scene Y extent is roughly **152.172 cm** CLOTHED. In native Unreal imported body primitive, source Y becomes native Z at 1 m → 100 cm with matching 128.143 cm body bounds.

Existing native Ruler preview capture: distance 360 cm, horizontal FOV 31°, 512×768 capture, image height approximate **390 px / 50.8%** for 152.172 cm source character. Controlled diagnostic alternative at 260 cm (same FOV) **540 px / 70.4%**.

Existing World camera: spring arm 395 cm, 80° FOV. At 1920×1080 16:9, approximate **441 px / 40.8%**. Controlled 300 cm, 70° alternative **695 px / 64.4%**. Close 270 cm, 65° alternative **849 px / 78.6%**.

These are **pinhole camera projections**, not screenshots or body/actor measurements. Actual forward orientation, pose, rendering, crop, occlusion, viewport dimensions, anti-aliasing, and UE show flags may change pixel height. Need matched rendered captures of native Party, Blender corrected, Unreal raw import, live roster and embodied World actor before visual acceptance. Do not modify production camera defaults solely based on calculation.

## 6. Existing demonstrated separate quality risks

- Archived Blender Test 3 had zero loaded textures despite exporter PNGs; corrective source importer restored 48 image datablocks and 51 connected image nodes. This points to a *Blender importer connection defect*, not automatically KKBP.
- Correcting overexposed studio lighting/tone mapping substantially improved Luminous Blender render without changing mesh or textures.
- Source glTF TANGENT attributes absent; Unreal recomputes MikkTSpace tangents. Check hair specular, normals, eyes and Painterly Anime PBR material equivalence. A source alpha-content audit found **three possible MASKED-depth-sort candidates** (eyeline-down, eyeline-shadow, and one garment material), while gradient-alpha upper eyeline must retain a suitable translucent representation; masked/opaque conversions require the art-direction comparison, not a global material sweep.
- **Native UE material mismatch directly confirmed:** Luminous body, face, sampled hair and upper eyeline MaterialInstanceConstants all inherit `/InterchangeAssets/gltf/M_Default` with **BLEND_OPAQUE, single-sided, default-lit**, whereas all their source glTF materials specify **BLEND and double-sided**. Their one BaseColorTexture parameter points to the imported source texture, so PNG presence alone does not restore source materials. The sampled body, face and hair PNGs are fully opaque and their alpha mismatch by itself does not prove visible loss; **upper eyeline** is decisively different: source 1024×1024 texture has **72.2651% fully transparent and 15.737% alpha-gradient pixels** but Unreal shader is OPAQUE. Native facial transparency is therefore not preserved. Restore approved eye/eyeline-specific material rather than blindly making every character surface translucent.
- Many BLEND materials can cause hair/eyeline sorting and Android overdraw.
- UE 4K textures can show lower effective mip residency under streaming, `TEXTUREGROUP_WORLD`, filtering and mobile compression although the asset remains 4096×4096. Need actual rendered texture mip, color space, compression (TC_Default vs TC_Normalmap), brightness and normals mapping.
- Real imported modular parts and material slot counts do not by themselves prove the complete preview actor has all owned state parts visible or camera framed correctly.

## 7. Proposed NON-PRODUCTION corrected profile for QA

1. Source: preserve native card, latest sha, KK coordinate and mod/zipmod provenance; export actual source geometry + skinned rigs + every required CM/DM/MT/NMP/normal/alpha texture; never replace with synthetic flat shaders.
2. Geometry: preserve full original LOD0 topology, UV0, normals, skin weights, active shape keys, joint hierarchy, rest/bind pose and unbaked scale; record source primitive/node bounds and UE component/world transforms individually. Set conversion at one documented basis step: source m to Unreal cm; do not apply an extra `0.01` factor in actor or Blueprint.
3. Morphs: existing UE zero-threshold QA still imports only 9/15 body morphs and 82/91 tested face morphs; sparse-accessor expansion also failed. Investigate importer target naming/deduplication, material splitting, sparse attributes, delta filtering and deformation before implementing a corrected QA-only import. Do not declare threshold-only fixes successful.
4. Tangents: test MikkTSpace recompute vs source-compatible baked tangents under identical lighting; avoid adjusting normals globally before material diagnostics.
5. Materials: separate skin/face, soft vs hard alpha hair, eye, eyebrow, eyeline and rim/outline layers. Maintain approved Painterly Anime PBR look, controlled roughness/specular and non-flattened eye detail, with double-sided/masked only where source alpha warrants. Do not automatically mask gradient alpha.
6. Textures: compare source PNG hashes and CM/DM/MT/NMP inputs with actual Unreal MaterialInstance parameter links, sRGB for albedo only, appropriate linear normals/masks, source mip generation, LOD bias, streaming/resident mip and Android ASTC compression. QA can force full-res mip residency briefly; shipping residency requires profiling.
7. Render: fixed camera focal/distance and character pose, identical 3-point/ambient studio and manual exposure, identical output resolution and no auto-exposure confound. Render raw-import → experimental actor → actual roster → World under same constraints.
8. Mobile: benchmark LOD/material draw calls, skeletal bone counts, alpha overdraw, 4K texture pool usage, morph vertex work, hair sorting and actual Galaxy S26 Ultra thermals/fps after separate authorized cook.

## 8. Candidate export family evaluation (Gate00 decision pending)

A. **Corrected KKBP**: reproduce actual installed exporter + addon versions, native source card and mod dependencies, Blender texture/node parity and FBX/GLB + Unreal import; benchmark all layers.

B. **Native Unity skinned extraction**: source-rig Unity assets via UnityPy/valid Unity tooling, preserve native mesh/bind pose/material source, test skinned format transfer and animation mapping. Source presence alone does not prove functional actor import.

C. **Hybrid**: native Unity rig/skinning and nonreplaceable KK appearance data, Blender for transparent material restoration and additional geometry, then controlled UE export.

Only compare variants from the **same verified latest card revision** with immutable hashes. Source card identity, exact Party textures, 3D pose, camera and Unreal render settings must match. Gate00 must separately approve the selected production path. Gate12 does NOT adopt a new export route without that authority.

`Scripts/Gate12Koikatsu/p0_compare_export_candidates.py` enforces source provenance labels and computes structural deltas; it never claims a pipeline winner. Missing Gate01-supplied export candidates are outstanding, not fake negative test results.

## 9. P0 automation and evidence

Implemented no-cook:
- `p0_preserve_native_alter_cards.py` — immutable snapshots and revision hashing.
- `p0_audit_gltf_skinned_fidelity.py` — source geometry/morph, image/material, skin and bounds.
- `p0_diff_luminous_skeleton_rest.py` — pairwise joint paths and rest pose comparison.
- `p0_unreal_native_quality_audit.py` — real imported Unreal selected parts, body texture and LOD properties.
- `p0_unreal_material_shader_readonly.py` — non-mutating material parent, alpha and used texture probes.
- `p0_quantify_camera_vs_mesh_size.py` — separate physical size from viewport framing.
- `p0_compare_export_candidates.py` — fail-closed cross-family source metrics.
- `p0_fidelity_regression_gate.py` — pre-cook acceptance fails closed; currently correctly **BLOCKED**.
- `OGGate12P0CharacterFidelityTests.cpp` — actual Unreal imported LOD0 body vertex/triangle, geometric bounds, skeletal/material reference and texture source dimensions. Previously needed a NullRHI correction to use `UTexture2D.Source` instead of runtime streaming resource.

**Latest verified no-cook Unreal source checkpoint after P0 test correction (2026-10-16):**
- Win64 Editor C++ build `Saved/Logs/Gate12_P0_TextureSourceFix_Build.log`: **4/4 actions PASS, DLL linked**, no cook.
- Gate12 targeted native suite `Saved/Gate12Readiness/Native_Gate12_20261016_150838/index.json`: **8 clean passes, 1 warning pass, 0 failures**, Unreal Editor exit 0.
- Complete project suite `Saved/Gate12Readiness/Native_Full_20261016_151839/index.json`: **130 clean passes, 18 warning passes, 0 failures** (148 tests), Unreal Editor exit 0.
- Real P0 LOD0 per-state body part: **9,996 vertices, 16,754 triangles, 1 LOD**. Imported native 4K body texture source dimension verified under `-NullRHI`. The warnings remain deliberate because 15 source-effective body morphs vs 9 native imported is *not* resolved.

`p0_fidelity_regression_gate.py` is intentionally **BLOCKED**, not production PASS. It preserves prior zero-threshold morph QA failures and now detects generic opaque imported materials including the alpha-gradient upper eyeline.

**This investigation does not modify or approve the production pipeline.** Source compilation and automation verification must be tied to the actual commit/date, not historical native pass counts.

## 10. Outstanding exact failures — stop before cook

- Gate09: skeleton paths/rest/bind compatibility and actual authored animation deformation.
- Gate01: explicit visual mapping of saved Alter coordinate 0 and source card identity, and actual three export-family source input packages with provenance.
- Gate12: investigate why **prior zero-threshold and sparse-expanded reimports both failed** to restore effective morphs; full Unreal LOD0 across all modular parts; experimental corrected eye/eyeline material rendering, texture mip and alpha inspection; controlled real rendered raw/roster/World screenshot parity.
- Gate11: canonical production actor/preview/Version bindings remain authority of Registry v2; existing experimental QA artifacts are not production catalog entries.
- Gate00: separate export pipeline selection/approval and acceptance criteria.
- Android: physical Samsung device character fidelity and actual full character/13-simultaneous logical interactions only after separately authorized cook.

**Gate 00 requested decision: CONTINUE P0 / HOLD PIPELINE APPROVAL.** Preserve all existing native source, user cards, local changes and frozen Foundation; do not cook, package or install.
