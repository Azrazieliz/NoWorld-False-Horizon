# Blender 5.2.2 LTS — NoWorld Mastery Migration & Integration

**Date:** 2026-10-15. **Engine:** Blender 5.2.2 LTS (official portable Windows x64), Unreal Engine 5.8.3, Gaea 2.3.1, Koikatsu KK/KKS source.
**Dedicated branch:** `software/mastery-blender`. **Professional demo approvals: 0/25.**

## Tested installation
- **Blender executable:** `D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe`.
- SHA256 of official 404,453,484-byte ZIP was verified as `3849d17a682cba006075aaa3f3597ecb5c9c30ec31035b2e092c53e40679b535`, matching `https://download.blender.org/release/Blender5.2/blender-5.2.2.sha256`. Legacy `D:\Tools\Blender\blender.exe` (4.5.10 LTS) remains intact.
- Binary ran natively and reported `Blender 5.2.2 LTS`. Latest Blender GUI opened `NW_StarfallSanctum_ArtDirected.blend` as a responsive window.
- Four **read-only** 4.5 source files opened cleanly: sanctuary (906 meshes, 19 materials), baked mount (1×16-bone rig with 4.5 Action Slot), 2-object Grease Pencil, and 3 lathed sample meshes. Tests in `VALIDATION/validate_compatibility.py`, outcomes `blender522_compatibility_results.json`: **4/4 structural pass**. No Cycles/EEVEE visual equality, export parity, gameplay art quality or Blender 5.2 file back-save test was claimed.
- **CloudRig official Blender Studio rigging extension** `bl_ext.blender_org.cloudrig` v2.2.31 (GPL-3.0-or-later) installed via Blender extension registry, *enabled*, imported and manifest-checked in installed 5.2.2. Validation `verify_cloudrig.py`. It supports Blender >=5.0 and 5.2. Under `--factory-startup`, preferences/extensions are deliberately not activated; omit that option for production extension tests.
- Blender online access preference was not changed permanently: official extension installation used the one-invocation `--online-mode` option.
- A successfully installed extension is **NOT** a production quality character rig. No existing KK rig re-generated with CloudRig yet.

## Correct multi-app workflow
Read `../Blender_4.5.10/NEXTGEN_WUWA_BENCHMARK_PIPELINE.md` as current quality/source/engine-authoring authority. For additional sources, see `../Blender_4.5.10/SOURCE_ATLAS.md`.

| Phase | Owner | Deliverable | Validation |
|---|---|---|---|
| Character artist source and facial design | KK/KKS + Blender 5.2 CloudRig/shape keys | Face/mesh/cloth/material masks, editable native source and expression library | Original KK vs Blender vs UE same pose and lights, 12 multiangle/pose tests |
| Hero motion source | Blender Action / 5.2 rig UI + permitted reference animations | Skeletal poses (not camera tracks), root vs in-place, anticipation, impact, recovery, secondary motion | Root/contact, target joint trajectories, expressive pose critique |
| In-engine actor and combat | UE 5.8 Control Rig, AnimSequence/Montage, IK Rig/Retargeter, Pose/Motion Warping | Actual game skeleton sequences with effect notify windows, gameplay responsiveness | Sandbox native import+bone parity then production Gate12 on-device |
| Runtime VFX | Blender art-authored shapes + UE Niagara masks/shaders | Shader and Niagara systems keyed by attack state, not video | Unreal playback, Android GPU/overdraw |
| Terrain and geology | Gaea 2 graph Build Swarm + Gaea2Unreal bridge | Heightmap, layer masks, erosion/deposition/rock and metadata for UE landscape | Native Gaea 2.3.1 build, scale metadata verified in isolated UE |
| Modular architecture and props | Blender art-level highpoly/lowpoly + CC0 scanned PBR and authored masks | Game-ready geometry UVs, detail, material masks and modular variants | Real game scene art review and LOD/material/mobile profiling |

## Actual game-skeletal action validation
- `../Blender_4.5.10/ANIMATION_3D/game_action_audit.py` ran in installed Blender 4.5.10 on sanctuary, 16-bone rig and 121-bone character; it rejects camera-only footage, requires an assigned Action+Action Slot, evaluated pose movement, bone mesh deformation, and baked constraint-free source for export.
- Isolated UE 5.8.3 `ue_anim_api_inventory.json` discovered `AnimationLibrary`, `AnimNotify_PlaySound`, `IKRigDefinition`, `IKRetargeter` and `ControlRigBlueprint` native APIs.
- `INTEGRATIONS/UE583/test_notify_authoring.py` cloned an existing noncanonical `AnimSequence` into the **isolated** `NW_BlenderInterop` project, authored two timed sound hook events on `NW_Action_Cues` at 0.22s and 0.72s, saved and reloaded the real Unreal asset. Result: **2 events and dedicated track verified native** (`VALIDATION/ue_notify_authoring_results.json`). This is event authoring only, **not** polished combat or actual registered damage/particles.
- Unreal 5.8 Control Rig supports animation authoring in Sequencer and runtime; documented bake to AnimSequence. References: https://dev.epicgames.com/documentation/unreal-engine/python-scripting-for-animating-with-control-rig-in-unreal-engine and https://dev.epicgames.com/documentation/unreal-engine/animating-characters-and-objects-in-unreal-engine and https://dev.epicgames.com/documentation/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine.

## Gaea 2 readiness and real blocker
- Installed **Gaea 2.3.1.0** includes `D:\Tools\Gaea 2\Gaea.Swarm.exe`, which prints a valid CLI contract for `--Filename`, `--buildpath`, `--resolution`, `--seed`, `--silent`, `--vars`.
- Tried building original bundled sample `Detailed Snow Peak.terrain` to an isolated output path at 512px. Swarm recognized CPU device and project; it failed early with **System.IO.IOException: The handle is invalid** in this remote console environment. No terrain assets emitted and no Unreal Gaea import demonstrated. Source sample unchanged.
- **Next environment work:** diagnose interactive console/terminal handle issue without altering user source, then run native Build Swarm on a *copy* of bundled samples, export heightfield+weights+JSON, install/test Gaea2Unreal **in sandbox only**, and verify scale/height in UE. Do not count until native output and matched terrain.
- Gaea official bridge: https://docs.gaea.app/guides/use-in/bridges/gaea2unreal/index.html. Unreal output node can target 1009/2017/4033 etc per https://docs.gaea.app/reference/nodes/output/unreal.html .

## External research and licensing cautions
- Wuthering Waves original Kuro Games interview: https://www.unrealengine.com/developer-interviews/exploring-the-post-apocalyptic-charm-of-asg-open-worlds-in-wuthering-waves ; gradients/masks, LUT/face shadow art, detailed keyframe control, curated blendshape library. **Benchmark is aspirational, NOT achieved**.
- Blender Studio Storm character rig CC-BY (requires >=5.0, paid Studio access may be necessary); can study methodology legally without bypassing gates: https://studio.blender.org/characters/storm/v1/ .
- Official CloudRig extension GPL and public docs: https://extensions.blender.org/add-ons/cloudrig/ .
- Adobe Mixamo royalty-free animations for video games, but raw asset redistribution restrictions and Adobe account action apply: https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html .
- Poly Haven CC0 textures/models, no automatic game look equivalence: https://polyhaven.com/license .
- Four user video references are documented via direct footage samples in `../Blender_4.5.10/NEXTGEN_WUWA_BENCHMARK_PIPELINE.md`. Videos are not vendored into Git and mere viewing does not constitute mastery.

## Active blockers
- No genuine Wuthering Waves-level or superior approved hero character/architecture/animation built. 25 professional demo acceptance targets still open.
- Need example-specific professional sculpt/retopo/reference fidelity, not generic procedural box composition.
- Need actual KK-to-UE eye/hair shading and facial likeness repair, source identity alignment.
- Need professional root/contact attack and native UE gameplay playthrough with root motion, retarget, Hit/Combo Window and Niagara effects, not cameras or preview videos.
- Need full highpoly detail, PBR/texture baking, world visual design and Gaea spatial integration on mobile.
- Gaea headless build failure must be corrected; no false Gaea completion.

Always preserve original production Gates and previous Blender 4.5 files.
