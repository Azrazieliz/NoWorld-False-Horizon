# Gate 12 — Motion V2 rejected: anatomical-axis quality gate

Date: 2026-10-11
Authority: Post-freeze Gate 12 animation acceptance only.
Regression: User reports revised Galaxy S26 Ultra gameplay locomotion is indistinguishably broken from prior V1.
Disposition: **FAIL — Motion V2 not accepted.** Do not convert technical .uasset loading, 19/19 synchronized clock or UE Android cook success into visual motion approval or Gate11 ARTIFACT_READY.

## Proven defect in actual source rig versus procedural generator

The shipped V2 authoring profile declares local rotation Y-axis (0,1,0) for upper/lower arms and thighs/calves. The immutable actual exported Koikatsu glTF 2.0 native-skeleton source has joint-to-child offset vectors very nearly aligned with that same Y axis:

| Parent-to-child reference bone | Absolute dot of V2 rotation axis and bone length |
|---|---:|
| thigh_l → calf_l | 0.998858 |
| thigh_r → calf_r | 0.998854 |
| calf_l → Left ankle | 0.997100 |
| calf_r → Right ankle | 0.997115 |
| upperarm_l → lowerarm_l | 0.970826 |
| upperarm_r → lowerarm_r | 0.970161 |
| lowerarm_l → hand_l | 0.990134 |
| lowerarm_r → hand_r | 0.989808 |

A bone-long-axis rotation mostly twists the bone. It is not the anatomical sagittal flexion/extension needed for walking, bending the knee or swinging the arm. All 8 audited limb joints **FAIL** the source-rig hinge-axis acceptance test. This finding is deterministic, based on native source-node hierarchy/translation and generator source code; it is not a visual guess. Audit script: Scripts/Gate12Koikatsu/audit_quality_v2_anatomical_hinge_axes.py. Receipt: evidence/koikatsu-motion-v2-rejection-20261011/anatomical_axis_fail_source_glb.json.

**Unreal-space caveat:** Unreal's importer may transform the glTF basis. The isolated Unreal 5.8.3 read-only native-pose audit was attempted but *terminated by measured physical RAM guard before Python execution*, and therefore does NOT yet confirm the final imported local axes. No UE asset was changed by the audit. Fixes must use independently verified imported UE anatomical axes, not simply substitute X for Y without testing.

## Why prior PASS receipts were insufficient

- 76/76 UAnimSequence assets and 19/19 exact skeletons confirm serialization/compatibility, not recognizable locomotion.
- 19/19 native state-switch logging confirms the player reaches synchronized clips, but not leg swing direction, heel/toe contact, upper-body timing or high-quality blend.
- Procedural sine-wave in-place clips are neither imported real Koikatsu action animations nor professionally authored natural biped motion; merely decreasing angles from 35 to 12 degrees does not solve inaccurate axes, stance or root/body coherence.
- Jump, turn, dodge, swimming, limb attachments/clothing collision and camera remain quality-rejected or untested.
- Physical Motion V2 launch is valid. The user's direct observation of animations is a legitimate FAIL superseding any earlier provisional quality suggestions.
- Android UI gfxinfo with 5 frames is *not* Unreal renderer FPS, and no UE renderer benchmark can be claimed.

## Repair contract for the next accepted candidate (Motion V3 or later)

1. Extract and validate the target character's **native UE-imported reference pose, source space mapping, and bone parent/child directions**. Require sagittal hinge axes perpendicular to the corresponding upper/lower bone direction; verify each knee and elbow bends towards correct anatomical side, rather than twisting.
2. Build a visual pose validation set in the actual Unreal engine showing neutral, left/right stride, planted stance, push-off, mid-air, swim. Inspect from front, side and rear and automatically reject implausible joint transforms or component misalignments. Preserve all original glTF meshes, skin weights, skeletons and original face/eye/clothing.
3. Replace temporary procedural motion profiles with rig-compatible **artist-authored/retargeted real locomotion**, covering complete idle/walk/run, start/stop, jump/land, turn, dodge and swim. Retarget across all 19 source components only with explicit skeleton compatibility and shared motion source.
4. Implement **real animation transitions/blending** and robust character physics interfaces (contact, foot planting, ground vs water) rather than only a synchronized normalized playback clock.
5. Conduct clothing/hair collision, camera occlusion and visible facial expression concurrent with movement QA, plus measured Unreal renderer CPU/GPU frame times on S26.
6. Repackage/physical-test ONLY after native pose-direction validation and side-by-side comparison with V2; Gate11 accepts nothing by implication.

Existing V2 test package may remain installed separately from original app, but MUST be labeled **visual FAIL**. No APK update was created during this audit.

## Preservation

Gate12 isolated branch only; G12-KK-NONANIM-1.0.0 ten-file contract PASS, Foundation 3f54f07cacbf193e160d0be15a7be5a7bc730c3b clean, PC WorldState.db preserved, original com.azrazieliz.OfflineGame unchanged. PR #16 remains draft/unmerged; Gate 11 registry ownership unchanged.
