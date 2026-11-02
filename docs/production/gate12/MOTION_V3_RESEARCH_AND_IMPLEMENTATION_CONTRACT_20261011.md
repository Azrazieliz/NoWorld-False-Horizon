# Gate 12 — Motion V3 coding research and pre-implementation contract

**2026-10-11 | RESEARCH / DIRECTION-MATH VALIDATION ONLY — NO V3 RUNTIME OR APK CREATED**

Authority: Gate 12 post-freeze animation engineering. G12-KK-NONANIM-1.0.0 ten-file lock, Foundation, original saves and Gate11 asset acceptance are unchanged.

## Observed user evidence

User-supplied Screen_Recording_20261011_093348_OfflineGame.mp4: 74.54 s, 2340×1080, nominal 60 fps. Koikatsu-derived character renders in actual Android gameplay, but when traveling along world Y the leg motion looks aligned to X (and vice versa); airborne motion curls/bends both legs. V2 user-visible motion is rejected. The user recording itself is NOT copied or published in GitHub.

## What existing C++ actually does

- AOGWorldPrototypeCharacter::MoveForward uses yaw-only controller rotation, FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) and AddMovementInput. MoveRight uses EAxis::Y. CharacterMovementComponent handles real capsule movement, collision, jumping, flying, swimming. Normal movement sets bOrientRotationToMovement=true.
- These game movement inputs follow native UE conventions: actor X forward, Y right, Z up. DO NOT globally swap X/Y or reverse existing user controls.
- AOGKoikatsuPlayableCharacter::ComputeState uses only horizontal speed, sprint, actor yaw rate, movement mode and state flags. It NEVER computes signed forward/right velocity relative to visual facing. Any world horizontal direction selects the SAME forward Walk/Run clips.
- Existing V2 native sequence writer uses bone-local Y rotation for thighs, calves, shoulders and elbows. Original immutable source glTF is Y-up; eight source bone vectors align with the chosen Y rotation axis at abs(dot)=0.970–0.999. Those are near-longitudinal bone twists, NOT anatomical sagittal flexion. The V2 generator was thus fundamentally invalid, not merely too exaggerated.
- V2 Jump/Fall/Land still use earlier procedural action sequences. The footage after approximately 56 seconds is explicitly in FLYING mode, not necessarily jumping. ComputeState checks IsSwimming, IsDodging and IsFalling, but DOES NOT check CharacterMovement->IsFlying or traversal mode Flying; airborne horizontal flight therefore incorrectly falls through to GROUND Walk/Run based on horizontal speed. The curled/spread legs during the later flying segment may be ground gait being played while flying. Jump must be tested separately from flight. Both need independent animation states; changing ground gait angles cannot correct this.
- Actual original source GLB geometrical eye/head positions suggest +Z source forward, +Y up and +X lateral. Unreal is +X forward, +Z up. Exact importer and SkeletalMesh component transform MUST be measured; never assume global source X equals local Unreal X.

## Correct world-to-actor direction algorithm (C++ reference, NOT yet integrated)

    const FVector V = GetVelocity();
    const FVector HorizontalWorld(V.X,V.Y,0.f);
    const FVector CharacterLocal = GetActorTransform()
        .InverseTransformVectorNoScale(HorizontalWorld);
    const float ForwardSpeed = CharacterLocal.X;
    const float RightSpeed = CharacterLocal.Y;
    const float GroundSpeed = HorizontalWorld.Size();
    const float DirectionDeg =
        FMath::RadiansToDegrees(FMath::Atan2(RightSpeed,ForwardSpeed));

Alternative: use AnimInstance CalculateDirection(Velocity, BaseRotation), but ensure BaseRotation is ACTUAL actor/visible mesh forward. Actor-local velocity is not automatically the same as asset-local velocity when a skeletal mesh's component yaw is offset. Debug-render actor X/right, velocity, mesh visible forward, and source-to-UE basis on every acceptance sample. For pure forward/back/left/right use 2D directional animation assets, not a world-axis switch.

The independent source-only direction math test validate_motion_v3_direction_contract.py passed 16 / 16 combinations: ±world X, ±world Y at yaw 0,90,180,270. It also correctly REJECTS all eight V2 limb-axis failures. Passing these preliminary tests is not approval of bone curves or imported Unreal component orientations.

## Required correct UE5.8 implementation

1. KEEP physics/input in existing CharacterMovementComponent. Animation reads real horizontal velocity, acceleration, actor yaw (and actor-vs-mesh yaw), is_falling, is_swimming, flying/traversal mode, sprint/dodge/action flags.
2. Independently verify actual imported native UE skeleton's local bone axes and 19 component reference transforms (the previous 8 GiB host commandlet was stopped by RAM guard before audit). For each hip/knee/elbow/shoulder pose, require hinge axis largely perpendicular to local segment vector and test anatomical flexion/extension SIGN using side-view geometry, mirror left/right. No guessing X or Y; old V2 Y fails source-rig checks.
3. Use real compatible artist-authored/retargeted animation source instead of another set of procedural sinusoids. Preferred approved UE content starting point: Third Person Template / Epic Game Animation Sample. Verify licensing and provenance. Characterize target source skeleton with IK Rig: pelvis retarget root, spine, head, left/right legs, left/right arms; calibrate retarget poses; verify foot/contact / reference alignment for each original part.
4. Produce ONE coherent authoritative full-body locomotion pose, then distribute to 19 separate visible skeletal parts using Leader Pose or Copy Pose ONLY after verifying hierarchy, bone indices, source-space/rest-pose compatibility and mobile performance. Epic notes Copy Pose copies only matching bones and is more game-thread expensive. Leader Pose has different render-thread/physics limitations. Skeletal Mesh Merge may remove morph capability and is NOT an approved substitute here without source-conversion version change. Preserve facial morphs and full fidelity.
5. Build Animation Blueprint + state machine instead of 19 independent PlayAnimation commands. Ground: 2D Blend Space with speed and signed facing-relative direction (idle, forward/backward walk/run and lateral strafes, starts/stops/pivots). Jump: separate takeoff, ascent, apex/fall, moving-in-air and landing; do not reuse folded-leg ground clips. Water: distinct treading, swim strokes/horizontal pose, ascent/descent; flight and dodge/attack must have their own states or layered montages.
6. Blend transitions (standard crossfade or measured inertialization) to avoid abrupt snaps. After real retarget, add foot IK / planted-foot contact, knee/elbow bend limits, appropriate clothing/hair checks. Facial mouth/eyebrow curve playback should remain independent of full body.
7. Visual admission test BEFORE ARM64 packaging: 360-degree turn, world ±X/±Y at yaw 0/90/180/270, diagonal, jump while moving, full landing, swim, dodge, front/side/rear poses, foot plant and cloth intersections. Reject unnatural limb tangents/pose discontinuities. Then independently test S26 gameplay and actual Unreal renderer game-thread/GPU time; Android UI gfxinfo is not UE FPS.

## Epic Unreal Engine 5.8 sources

- Movement Components: https://dev.epicgames.com/documentation/unreal-engine/movement-components-in-unreal-engine
- CharacterMovementComponent / movement-facing: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent
- Calculate Direction: https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Animation/CalculateDirection
- Blend Spaces speed/direction: https://dev.epicgames.com/documentation/unreal-engine/blend-spaces-in-unreal-engine
- Animation Blueprint: https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprints-in-unreal-engine
- State Machines: https://dev.epicgames.com/documentation/en-us/unreal-engine/state-machines-in-unreal-engine
- Transition blending: https://dev.epicgames.com/documentation/unreal-engine/transition-rules-in-unreal-engine
- IK Retargeting: https://dev.epicgames.com/documentation/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine
- Retargeting Bipeds: https://dev.epicgames.com/documentation/en-us/unreal-engine/retargeting-bipeds-with-ik-rig-in-unreal-engine
- Modular Characters/Leader Pose/Copy Pose/merge: https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-modular-characters-in-unreal-engine
- Transform Bone (Component/Bone Space): https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprint-transform-bone-in-unreal-engine
- Third Person Template: https://dev.epicgames.com/documentation/unreal-engine/third-person-template-in-unreal-engine
- Full Body IK: https://dev.epicgames.com/documentation/en-us/unreal-engine/control-rig-full-body-ik-in-unreal-engine

## Acceptance and preservation

Motion V2 remains visual FAIL despite prior native/cook/phone startup PASS. The V3 direction tests are PRE-IMPLEMENTATION checks only. Actual UE imported joint orientation, mesh forward yaw, full-body retarget, physical animation quality and V3 APK are NOT VALIDATED. No V3 APK was built or installed as part of this research. Source conversion freeze, original application, user save, frozen Foundation and PR16 remain unchanged. Gate11 ARTIFACT_READY not authorized.
