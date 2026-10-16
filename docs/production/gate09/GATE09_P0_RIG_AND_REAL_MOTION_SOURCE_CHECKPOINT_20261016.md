# Gate 09 — P0 Luminous Rig & Real Motion Source Checkpoint (2026-10-16)

**Authority:** Issue #25 and Gate09 first production commission; Issues #20/#21; Gate12 P0 quality report.
**Stage:** SOURCE_QA_PARTIAL — not a production-animation or export-pipeline acceptance.
**Isolation:** `production/gate09-rig-motion-20261016`; binaries and detailed per-bone reports stay in `D:\UnrealProjects\Gate09_Protected_Production_20261016`.
**Immutable source:** three `gate12_fresh_export_20261011/BLENDER_FRESH` GLBs were read, never edited; native Party cards/Alter sources untouched. Base artistic approval is not reopened.

## 1. Audited actual skinning and compatibility

| Property | CLOTHED | UNDERWEAR | NUDE |
|---|---:|---:|---:|
| Declared skinned joints | 975 | 975 | 975 |
| Joints with nonzero vertex weights | 324 | 297 | 295 |
| Embedded glTF animation channels | 0 | 0 | 0 |
| SHA-256 | `c1efbc9d1e0a09d840201922de9634147b359828cdfc59e5a79c7f723891c441` | `bdf8a59f319ccf49eb186395d3ec9c7a2434156bd07be7a3652be3b3e4b05257` | `a5800ca3865d1971c381ff510103730a1bcebeacda21daaa9bc4d32e9cb90074` |

Read-only parser `Scripts/Gate09/audit_luminous_rig.py` traverses actual node parents, builds local/global rest transforms, decodes source joint/weight buffer attributes, reads inverse bind matrices, and reports exact mismatched paths. Different path sets and different bind transforms are not normalized to pass.

| Pair | Shared joint paths | Unique per side | Distinct matrix-local rest among shared | Rest differences on weighted joints |
|---|---:|---:|---:|---:|
| CLOTHED ↔ UNDERWEAR | 782 | 193 | 58 | 36 |
| CLOTHED ↔ NUDE | 782 | 193 | 89 | 45 |
| UNDERWEAR ↔ NUDE | 782 | 193 | 38 | 16 |

Gate12's earlier raw-TRS comparator counted 56 (not 58) CLOTHED↔UNDERWEAR differences: the above comparator uses reconstructed **local 4×4 matrices**, so this discrepancy is definition/precision sensitive and requires case-by-case review rather than being concealed. The remaining pair counts match Gate12's earlier summary.

Terminal-bone name-only *heuristics* classify 129 of the 193 unique paths as possible hair/wardrobe secondary nodes, 57 unresolved, and 7 core-like; this is **not** an accepted bone-role map. 86 joint paths unique to each paired state carry nonzero skin weight in the inspected skinned primitives. No bone removal or shared-skeleton declaration is safe from these numbers alone.

**P0 conclusion:** source GLB structures fail direct joint-index identity and shared-rest identity. A path-resolved, deforming semantic core might be possible, but needs explicit body/secondary classifications, local/world rest and inverse-bind alignment, skinning under the *same* source clip for each state, plus outfit/secondary visual QA before any animation-sharing decision. Existing imported 975-joint equality is inadequate.

## 2. Real source animation inventory

- Koikatsu Unity `abdata/action/animator/14.unity3d`: read-only previously validated bundle SHA-256 `42e61bda9efbaf443b8ba5577f44be0ab7bdf85c87b5a2af1adc1a19c35473d3`, 299 genuine Unity AnimationClip objects. **STAND_IDLE** `f_tachi_00_18_L`: 30 fps, approximately 4.7 seconds, 702 hashed native bindings, extracted immutable gzip curve receipt SHA `0f2d71d79f3695854e4441c59859328a80a046e53d2df1b3b3fddf64eb023cdd`. **SWIM** `f_suiei_00_02_03_S`: 30 fps, approximately 0.667 seconds, 702 hashed bindings, extracted gzip SHA `3a63d06d502caeeb987f4aed37a47ab252fe93c40668b3aa153b4aa7be1826b2`. Neither is already mapped/baked into the 975-bone GLB. Swimming clip is short and must not be represented as a proven sustained loop.
- Studio `anime/02.unity3d`: 10 genuine clips including named `cf_cm_W_dance_*` and `cf_Dance_*` entries. Studio `anime/07.unity3d` has 1 clip, `anime/12.unity3d` has 4. Available does not imply a cleared dance retarget.
- Existing CMU motion corpus `NW_Blender_Mocap_Interop_20261015` has multiple genuinely animated BVH sources; first isolated proof uses real `144_20.bvh` SHA-256 `d8e4025891f8ffc94ff53839525b0e71cfacd2cbec30a181bd738ec8220cbfce`. This is a technical diagnostic, **not** approved Luminous combat, walk, swim, flight, or dance choreography.

Previously compiled motion records that contain static posed joints or wildly oversized retarget coordinates are **not** accepted as native Luminous movement. Reuse is evidence-gated.

## 3. New genuine skeleton-motion proof

Scripts: `probe_source_motion_blender.py`, `bake_existing_bvh_motion.py`, `capture_bvh_proof_views.py`, `qa_reimport_anim_fbx.py`.

- Blender 4.5.10 independently imported unchanged CLOTHED skinned GLB: authentic 975-bone Luminous armature and skinned outfit meshes.
- Imported actual 31-bone CMU `144_20.bvh` action; mapped **22 explicit anatomical rotation channels**, without renaming/removing/normalizing the 975 source bone hierarchy.
- Authored a diagnostic 240-frame animation sequence at 24 fps (9.958 seconds) in the actual Blender armature, generated edit-ready `.blend` and animation-only `.fbx`, 36 MB and 20 MB respectively.
- Evaluated the real armature-deformed `Hair Outfit 00` mesh on 96 sampled skinned vertices: start→middle maximum displacement **0.11347 m**, start→end **0.12021 m**. This is actual deformation, not moving the camera.
- Fresh animation-only FBX import created a 975-bone armature with an action and genuinely time-varying sampled poses across frames 2–241.
- Saved three actual skinned Blender Workbench diagnostic PNG renders at frame 120 (front, side and diagonal); visually checked for character presence and gross limb movement. These gray untextured captures are **not** accepted native 3D materials, anime quality, facial expression or clothing secondary simulation.
- SHA-256 edit-ready blend: `c5801fbbf6c9ffe94cb3bdd2422f7302e06de07fd8f02953eae8c0f7d6ceca1a`. Anim-only FBX: `edd0b78a9725ef8f6fbd252889b1a579e3557a91459a563d26cb6ecb809397bb`.
- Both original source hashes were compared again after authoring. Protected technical receipts include the three PNG hashes and exact mapping.

This proves an **importable genuinely moving diagnostic source clip**, not acceptable natural locomotion. Contact timing, lateral/forward axis, source motion genre, morph curves, secondary motion, native source material response, transition windows and cross-wardrobe deformation are unverified. Neither Unreal import nor device tests are claimed.

## 4. Regressions and mastery

- `Scripts/Gate09/test_gate09_source_qa.py`: **10/10 PASS** on original immutable GLB and BVH source hashes, weighted joint counts, all three pairwise path incompatibilities, source-motion skin displacement, generated output hashes, three captures and FBX reload receipt. Tests explicitly reject inferred shared skeleton or Android completion.
- Integrated Software Mastery mandatory `mastery_preflight.py` and `gate_start.py --gate 09` PASS. New permanent learning rule **K034** registered with source procedure, parameter/method restrictions and regression route. Master learning tests now **35/35 PASS**; runner tests **10/10 PASS**.
- No changes to existing dirty unrelated Software Mastery files. No foundational changes, no Android packaging, no private binary commit.

## 5. Status / owner dependencies

| Deliverable | Gate09 status | Specific next authority/action |
|---|---|---|
| Three-state source path/weight/rest/bind audit | **PARTIAL** | Gate09: resolve true core vs secondary per-bone semantics and deform all three states under one matched clip. Gate12: independently evaluate UE import/retarget. |
| Genuine source animation inventory | **PARTIAL** | Gate09: recover Unity hashed-path bindings and stage approved relevant walking/swimming/dance clips. |
| Real 975-joint animated source clip | **PASS (technical diagnostic)** | Gate09: remove contact/pivot/pose problems by authored motion QA; no Gameplay PASS. |
| FBX reimport moving skeleton | **PASS (Blender only)** | Gate12: native Unreal validation on isolated fixture if useful. |
| Front/side/diagonal captures | **PASS (diagnostic capture exists)** | Gate00: artistic quality remains unapproved. |
| P1 locomotion (forward/lateral/foot lock/jump/swim/fly) | **NOT_TESTED** | Gate09 needs production source motions/contact-authoring; Gate12 resolves input/camera basis. |
| P2 premium 8–20 s dance | **NOT_TESTED** | Source studio dance candidates available but no verified Luminous body/facial/beat bake. |
| Alter native compatibility | **BLOCKED (mapping pending)** | Gate01-3D verifies actual authored Alter source/state. No 2D dependency. |
| Gate04 combat kits | **BLOCKED (Gate04 authority)** | Await approved kit mechanics before canonical combat choreography. |
| Gate11 package/ARTIFACT_READY | **NOT_TESTED** | Only Gate11 may accept production content. |
| UE/mobile animation | **NOT_TESTED** | Gate12 alone owns runtime/physical testing. |

**Next single executable Gate09 task:** resolve Koikatsu source clip hashed bindings against actual native transform paths; validate 975-joint deformation across wardrobe appearances under the same proven native clip; then author a contact-marked actual ground-motion family. Do not fill missing locomotion/dance files with procedural placeholders.
