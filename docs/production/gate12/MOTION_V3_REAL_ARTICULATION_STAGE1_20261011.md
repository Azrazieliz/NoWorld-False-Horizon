# Gate 12 — Motion V3: authentic articulated mocap source and first UE native clips

**11 October 2026 — Stage 1 engineering evidence.** This is an isolated motion-development continuation, not an approved gameplay animation release or Gate11 intake.

## User acceptance requirement

The original motion is visually rejected: X/Y motion appears orthogonal; airborne movement curls/splits legs; limb motion is excessively fluid, uniform and lacks convincing articulated hip/knee/ankle/elbow pivots, planted-foot stance and controlled recovery.

Acceptance MUST look like an actual multi-joint articulated person:
- A hip rotates the thigh about a sagittal hinge; the knee bends the calf; the ankle follows foot contact and toe-off, and each joint has a distinct temporal curve.
- Left/right steps alternate support and swing. Foot-contact windows freeze or stabilize the planted foot relative to the world, not advance the same sine phase for the whole leg.
- Forward/back/lateral are determined by velocity relative to actor-facing, not raw world X/Y.
- Jump takeoff, airtime, landing, swimming and flying do not simply replay ground running clips.
- Over-smoothing is not acceptable; nor are jerky, robotically locked joints. Preserve the authentic capture timing and anatomical bend profiles.

## Native import coordinate calibration (validated)

Original source Koikatsu GLB rig uses **Y up, geometric face forward approximately +Z**.
Actual Unreal 5.8.3 imported skeletal reference uses **Z up**, with source face approximately **+UE Y**. The original fixture Blueprint sets all 19 components relative yaw 0, yet UE CharacterMovement uses actor +X forward. Thus a fixture-specific **-90 degrees UE yaw correction** is required to align visual forward with actor forward.

A fresh Unreal native read-only one-part skeleton inspector produced 21 actual bone reference transforms (M03P00). Exact import conversion tested:
- source glTF rest translation xyz metres -> Unreal native (x, z, y) × 100 centimetres;
- source glTF local quaternion xyzw -> UE (-x, -z, -y, w).
- Max calibrated error across 21 bones: 0.00005775 cm translation, 0.00003631 degree rotational difference.
- This is **ONE native skeleton component**; apply cross-part automatic visual rotation ONLY after checking all 19 components' relative BP rotations and target source rig.

The frozen Blueprint importer itself is NOT modified. The isolated native gameplay character has uncompiled source changes prepared for fixture-specific visual facing, actor-local forward/right velocity, directional class and distinct Flying/FlyingIdle state; these are **NOT yet Windows/Android native build accepted** because UE compilation encountered RAM/commit-pressure stops. Do not claim a new built/fixed phone APK.

## Actual human motion data, not generated sine-wave curves

CMU Graphics Lab real human capture, Bruce Hahne 2010 BVH conversion:
- 02_01 walk: 344 frames originally 120 Hz.
- 02_03 run/jog: 174 frames, 120 Hz.
- 02_04 jump/balance: 484 frames, 120 Hz.
- The first synthetic T-pose calibration frame is excluded from authored motion; sampled genuine capture frames at 30 Hz.
- Immutable originals, license statement, download URLs and SHA-256 hashes are preserved in SourceAssets/Gate12/MotionV3/CMU.
- CMU notes possible noisy foot/toe markers; original BVH data is not by itself polished game animation.

Real retarget computation:
1. Parse BVH original hierarchy and Z/Y/X Euler channel order exactly.
2. Compute true human capture global joint rotations per frame.
3. Compute human motion global delta from synthetic rest pose.
4. Apply to exact Koikatsu GLB original global rest pose for corresponding torso, leg, arm, head bones; reconstruct child-local quaternion curves without changing the original GLB or skin weights.
5. Apply the exact independently calibrated source-glTF-to-native-Unreal quaternion conversion, using captured 120 Hz frames downsampled to 30 Hz without artificial sine-wave interpolation.
6. Preserve the original capsule as movement/translation authority; no source root-motion tracks are written into experimental native animation assets at this stage.
7. Write genuine UE AnimSequence assets in separate NativeMocapV3 namespace; V1 and V2 remain unchanged.

## Measured source-space anatomical articulation

Actual retargeted Koikatsu left/right knee joint flexion:

| Motion | Left knee min–max | Right knee min–max | Distinct elbows |
|---|---|---|---|
| Walk | 10.1–74.4 degrees | 10.6–74.4 degrees | Left 24.4–51.9 / Right 21.7–43.9 degrees |
| Jog | 25.2–112.4 degrees | 26.8–109.2 degrees | Left 56.8–67.9 / Right 49.8–67.0 degrees |
| Jump | 17.5–99.9 degrees | 13.8–102.3 degrees | Left 26.6–54.3 / Right 21.9–55.9 degrees |

- All three 30fps offline retarget articulation checks **PASS** independently; maximal source-to-Koikatsu knee angle difference approximately 1.2 degrees (caused mainly by target rig rest knee bend).
- Walking/running show real alternating left/right knee phase and nonuniform, independent elbow changes.
- Source-foot low-height/low-horizontal-velocity contact candidates are detected in both walk and jog.
- **Foot lock and pelvis/root compensation in the target game are NOT yet implemented**, and the captured clip boundaries are NOT approved as loop seams. End-to-end motion is still unapproved.
- Three native per-bone 30fps quaternion track files are staged and reproducibly generated by scripts from the committed originals.

## Real Unreal engine authoring status

A real UE5.8.3 editor commandlet loaded the first exact imported native skeleton component (M03P00) and saved **three new UAnimSequence objects** for Walk, Run and Jump under /Game/Experimental/Gate12Koikatsu/Animation/NativeMocapV3. Each is tied to the real native skeleton, uses matching source bone names, preserves source rest translations and explicit no-root-motion constraint. Native one-part authoring reports 3/3 success.

A separate fresh Unreal Editor process independently cold-loaded all three saved native clips: **PASS_FRESH_UE5_NATIVE_3_MOCAP_CLIPS_ONEPART_PRE_VISUAL**. Exact source skeleton, 19 actual native UE bone tracks per clip, durations (2.833 s walk, 1.433 s run, 4.0 s jump), zero root-motion tracks. This establishes valid serialized native UE animation data for **one part only**. Nineteen-part authoring, anatomical visual side-view and real motion blending remain separate gates. Do NOT automatically enable these one-part clips in gameplay. No new Android APK has been cooked or installed.

## Next actions / hard quality gates

1. Independently cold-load the three native UAnimSequence assets, verify skeleton, track names/duration.
2. Render native skeleton front/side/rear pose samples and validate actual sagittal knee/ankle bend versus longitudinal twist. Compare skin deformation under cloth and hair.
3. Extract natural walk/run cycles and transitions with contact-aware loop seam/foot locking. Retarget full body to all 19 imported part skeletons, enforce one authoritative coherent pose and validate modular compatibility.
4. Finish native C++ Win64 build under safe memory constraints, then Animation Blueprint directional Blend Space with real crossfade and separate Air/Swim/Fly states.
5. Physically test **only a completed, approved experimental APK** on the S26; compare the user's bad V2 video against matching V3 gameplay movements, clothing/feet/water/camera and UE renderer GPU/CPU frame times.

**Preservation:** Gate12 isolated branch only, G12-KK-NONANIM-1.0.0 all ten locked files byte-identical, original Foundation 3f54f07cacbf193e160d0be15a7be5a7bc730c3b and original WorldState.db unchanged, original Android game com.azrazieliz.OfflineGame intact, PR16 draft/unmerged, Gate11 ARTIFACT_READY not granted.
