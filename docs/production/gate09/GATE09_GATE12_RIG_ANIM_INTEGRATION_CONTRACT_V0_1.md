# Gate09 → Gate12: Rig / Source Motion Integration Contract v0.1 (2026-10-16)

**Scope:** Diagnostic only; P0 export pipeline remains HOLD. No automatic production remap, no UE/Android PASS.

## Source identity and invariants
- Source states: real separate CLOTHED, UNDERWEAR and NUDE wardrobe meshes for one Luminous Base. No new roster identities; privacy rendering is independent.
- Original glTF v2 binary source paths and hashes are recorded in Gate09 P0 source checkpoint. Each skin declares 975 joints, but names/path order and 193 paths per pair differ. **Never link clips by joint index across states.**
- Preserve every original bone, local rest matrix, global rest matrix, inverseBindMatrix, skin weights, bind pose and morph target. Missing bones are not auto-synthesized; wardrobe extras are not stripped to make an import match.
- Imported source Blender armature has 975 named bones. CMU diagnostic source rig has 31, with 22 explicitly mapped to exact existing Luminous names.
- Original GLB source up is glTF Y; Blender importer produces Blender Z-up. The diagnostic CMU importer was explicitly `axis_forward='-Z', axis_up='Y'`. Unreal input/camera/bone travel alignment **must be measured**, not treated as a universal 90° offset.

## Exact offered test interchange
- Protected editable source: `D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_Luminous_CMU144_20_Retarget_PROOF.blend` (SHA-256 `c5801fbbf6c9ffe94cb3bdd2422f7302e06de07fd8f02953eae8c0f7d6ceca1a`).
- Protected motion-only interchange: `...\G09_Luminous_CMU144_20_AnimOnly_PROOF.fbx` (SHA-256 `edd0b78a9725ef8f6fbd252889b1a579e3557a91459a563d26cb6ecb809397bb`).
- Real source BVH SHA-256: `d8e4025891f8ffc94ff53839525b0e71cfacd2cbec30a181bd738ec8220cbfce`.
- Format: Blender 4.5.10 Action with OBJECT slot, 975-bone armature. FBX 7400, animation-only export with armature only, add-leaf-bones disabled, all-action export disabled and baked animation active.
- Timeline: 24 fps, key frames 1–240 = 9.9583 sec diagnostic segment. Separate FBX reload showed animated action frames 2–241. **Do not infer musical timing or usable dance length from duration alone.**
- Motion: mapped local quaternion rotation deltas aligned via source/target local rest axes; root translation from CMU intentionally **not** copied. No authored gameplay root motion or verified stride speed; output must be treated as in-place diagnostic.
- Mapping: see `G09_Diagnostic_Retarget_Profile.json` (22 exact source/target pairs).
- Morph, eye, face and secondary motion: none authored in the proof; no fake morph mapping. Keep actual native shapekey names/indices stable. The rest source native morph investigation is separate (Gate01/Gate12 fidelity HOLD).
- Loop boundaries: NONE approved. Test segment frames 1–240 is not automatically cyclic. Blend windows, IK/contact locks, interrupts/cancels and physics markers: **not yet authored or accepted**.
- Visual evidence: three protected Workbench diagnostic images, front/side/diagonal at frame120, genuine evaluated mesh; gray non-Painterly material preview only.

## Fail-closed import/retarget checks owned by Gate12
1. Import the animation-only FBX to an isolated QA location, without changing accepted production skeletal assets or adoption of a new exporter.
2. Compare imported 975-bone paths, local rest and inverse bind transforms with the exact CLOTHED source rig; do not assume imported UE skeleton identity merely because bone count matches.
3. At reference frames corresponding to 1, 120, 240, compare Hips/Spine/Chest/Upper Chest, both arms/hands, both legs/feet, head and source mesh skinning against the protected Blender diagnostic.
4. Reject invalid root travel axes and foot sliding based on measured Unreal actor world transform, mesh forward, camera/input X/Y vectors, stride and real speed. No blanket 90° patch.
5. Repeated tests across UNDERWEAR and NUDE **require separate state-specific rig-path and rest compensation** until actual matching posed deformation proves common-core compatibility.
6. Do not drop original morphs or overwrite native upper-eyeline translucent material while testing animation. Material parity and effective morph loss remain separate Gate12 blockers.
7. Record per-category SOURCE_IMPORT, SKELETAL_DEFORMATION, FOOT_CONTACT, VISUAL, RUNTIME and DEVICE status independently. No DEVICE_VALIDATED or ARTIFACT_READY without corresponding owners' receipts.

## Production requirement still open
- Gate09: actual first-mode source-authored or faithfully adapted idle/start/walk/run/stop/turn/backwards/lateral with foot-contact times and stride characteristics; sustained water/air source loops; transitions/jumps.
- Gate09 + Gate03 where needed: selected genuinely compatible 8–20-second dance with beat map, facial/gaze timing, steps/contact, authorized source hash.
- Gate04: canonical combat mechanical timings before Gate09 skills/VFX.
- Gate01-3D: Alter's authentic three-card source mapping; separate manifestation/form, no compulsory 2D reference.
- Gate00: creative/artistic and pipeline acceptance. Gate11: content registry acceptance. Gate12: Unreal runtime and Android tests.

This contract is a **usable diagnostic interface**, not a production-ready Motion V3 library.
