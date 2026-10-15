# NoWorld: False Horizon — Advanced Art + Animation Mastery Reset (2026-10-15)

## Governing requirement

**Quality floor for final work:** user wants visual artistry at least comparable to, and ideally surpassing, Wuthering Waves, while preserving completely independent creative identity. That is an **aspiration and acceptance benchmark**, NOT proof that existing generated assets meet it. Creative approval by Gate 00 / specialist owner is mandatory for canonical characters/worlds. No unapproved franchise models or proprietary render textures may be used as production assets.

**Present evidence:** 0 of 25 professional demonstrations approved; current previous Blender scenes are prototyping-only. A project created by a team of hundreds with dedicated lighting, shader, character and animation specialists cannot be surpassed credibly by a few generic procedural scripts. Improve throughput by reusing *legally reusable high-quality source assets and expert-grade systems* while investing artistic refinement where it matters. Never declare superior quality based on triangle count, render resolution, shader node count or camera movement.

### Evidence-based target from original Kuro Games technical interview
https://www.unrealengine.com/developer-interviews/exploring-the-post-apocalyptic-charm-of-asg-open-worlds-in-wuthering-waves

Kuro reports:
* Kuro's team used character base-color and mask gradients rather than one undirected material node, with specific artist authored palettes.
* Anime color looks depended on customized lighting, LUT/color-grade, face shadow maps and authored 'stable' scene lighting, rather than one default PBR solution.
* They moved toward an expression **blendshape library**, because bones alone lacked precision and speed.
* Effects and cinematics were separately authored, controlled and keyframed in the runtime engine.
* Cross-function coordination is necessary: material look-dev + mesh geometry + pose readability + in-game presentation.

**Our response:** build composable parametric material mask libraries + true facial expressions with pose tests; treat runtime UE lookdev as mandatory; author skeletal playable actions rather than camera footage. Preserve authored quality via derivative lower-cost Android LODs, do not merely decimate the same hero mesh.

## Highest-efficiency four-software responsibility split

| Owner | Highest-value tasks | Native accepted output | Must not substitute |
|---|---|---|---|
| **Blender 5.2.2 LTS** | High/low 3D hard surface + sculpt/retopo, rigging/shape keys, precise reference match, UV/texture masks, profile revolve, in-place/root/IK animation blocking and authored asset variants, 2D Grease Pencil source | Layered editable .blend; skeletal Action + Action Slot; FBX/glTF data; UV/normal/material masks and morphs | Cycles movie is not a playable attack/character or engine shader parity |
| **Unreal 5.8.3** | UE-native Control Rig refinement, IK Rig/Retargeter, root/pose/motion warping, Animation Blueprints/Montages, AnimNotifies and hit event windows, Niagara VFX, gameplay mechanics, Android LOD/perf, stage light/LUT | Real AnimSequence on real character Skeleton + animations on game actor, mesh and mask materials, Niagara runtime system, retarget proof and on-device acceptance | Importing AnimSequence alone is insufficient to prove motion fidelity or art |
| **Koikatsu / KK/KKS** | Native character identity, face/hair/eyes/outfit variants, modded material controls, deformations and expression fidelity, export intake; edit material/gradient/damage in KK only with a validated compatible plugin | KK editable character card/source, exported compatible textures/morph rig, signed identity correspondence and side-by-side KK/Blender/UE | BLENDER-only hair gradient is not in-KK authoring; a glTF loads does not mean a face matches |
| **Gaea 2** | Terrain graphs, erosion/strata, macro-to-meso geology, climate masks, heightfields/weightmaps/cavity/AO, tiling and deterministic terrain seeds | Gaea graph + heightmap/weightmaps/JSON metadata -> Gaea2Unreal bridge -> UE Landscape | Massive Blender cubes/heightfield approximations are not realistic landforms |

**Unreal/Gaea facts:** Gaea's Unreal Output node exports UE-friendly dimensions including 1009, 2017, 4033 and height/masks/weightmaps, and the Gaea2Unreal importer uses paired JSON to set correct scale. Installed Gaea is `D:\Tools\Gaea 2`. Source: https://docs.gaea.app/guides/use-in/bridges/gaea2unreal/index.html . As of this note, its native plugin has NOT yet been installed/tested in isolated Unreal. Any Gaea license/API limitations must be checked before promising fully unattended bridge.

**Blender upgrade:** install official 5.2.2 LTS Windows x64 portable under `D:\Tools\Blender-5.2.2-LTS`, after SHA256 from https://download.blender.org/release/Blender5.2/blender-5.2.2.sha256. Do NOT overwrite 4.5.10. Run factory-startup compatibility matrix before migrating any existing native scene; latest feature != superior art.

## Focused studio-grade standards by deliverable

### Hero character
- **Base:** face silhouette and asymmetrical features must match approved orthographic/3/4 close-ups, with independently correct eye cornea/iris/eyelash/hair cards, expression creases and nose/mouth construction. Eye shadows should not create hard black artifacts.
- **Art:** clean normals/outline style, hair strand silhouette hierarchy, sculpted multi-level outfit complexity, fabric weave and wear mapped via authored region masks, metal/stone realistic/specular response as needed, authored materials for both front and back angles.
- **Rig:** deformation weights, IK/FK and twist bones, corrective sculpted keys, mouth and eyelid library, hair/cape/wing accessory secondary animation. Rig must be free of unpleasant pose clipping in at least 12 review poses.
- **Cross-engine:** KK original/Blender/UE closeups in identical poses and controlled neutral vs dramatic light, preservation manifest for morphs/textures and skeleton. Compare visually, not merely data counts.
- **Rendering:** collect images from actual UE gameplay material system including Android target; offline Cycles is look-dev only.

### Game animation (the most important correction)
- **Deliverable:** `AnimSequence` (UE) on intended Skeleton, editable Action (Blender) with documented `Action Slot`, root-motion/in-place decision, contact and interaction timing manifest, attack/recovery/landing/loop bounds, and UE AnimNotify timed events.
- **Blocking:** shot- and gameplay-specific silhouettes, readable intention; reference footage and clear contact line of action. Define feet/hip/chest/head/hand poses at anticipation, launch, contact, follow-through and recovery.
- **Polish:** center-of-mass arcs, physically plausible support, planted foot/contact, rotational inertia, torso counterrotation, limb overlap, asymmetry and snappy spacing while maintaining game responsiveness. Not just motion curves or root translation.
- **Unreal:** retarget with IK Rig and target hierarchy; Pose Warping/Motion Warping if needed, Animation Blueprint transitions and action windows, AnimNotifies to trigger damage/hit audio and Niagara effects. Test 3 representative body morphs, 2 camera distances and both X/Y traversal directions.
- **Performance:** run on Android target, show CPU/GPU traces and source vs UE per-bone sampled deviation. Camera keyframes/MP4 preview do NOT count.
- **Input source:** use available preexisting Gate12 local mocap where authorized **read-only** (existing CMU-derived walk/run/jump source tracks), plus motion reference and permitted libraries. Avoid copyright/provenance uncertainty for paid/stock assets.

### Combat VFX
- Author effect *components* and parameters: visible silhouette geometry, windup, primary beam, multi-ring impact, layered curl/fragments, emissive mask, dissolve and recovery. Niagara runtime graphs are the production source; Blender creates editable source meshes/masks and reference shapes. Trigger with actual AnimNotify on contact (not clock time independent of actor animation). Android shader permutations, overdraw, pooling and reductions mandatory.

### Hero architecture and worlds
- Begin with silhouette/reference approved and architectural design constraints, not indiscriminate ornament repetition. For a palace: actual scale, structurally credible cornices/column/gate geometry, high/low mesh, UV trim/unique textures and baked normal/roughness, glass/material variety, staging and interior usability.
- **Geology first** in Gaea with terrain erosion/masks, then Blender hero modular architecture, finally Unreal scene light/LUT and foliage/physics. Poly Haven CC0 (https://polyhaven.com/license) supplies legitimate PBR base maps where usable, but authored localized masks/hero materials remain necessary.
- Judge close- and long-distance silhouette vs approved visual target. Artificially adding 800 cubes cannot certify quality.

### 2D anime
- Blender 5.2 Grease Pencil cutouts, frame morph/inbetweens where suitable, artist-authored important poses, clean cel gradients/shadows, line weight/face volume and authored smear frames. Runtime deliverables are Paper2D/compatible skeletal clips, sprite atlases, material masks or Niagara—not default MP4s.
- Test 1s/2s exposure at 24 fps and actual Android/UI presentation. Avoid generating 24 independent images per second except when uniquely required.

## Use of references (4 user videos, actually sampled)
- Bott Brothers, anime VFX blast, 10s, https://youtube.com/shorts/-fRgXjXUxv4 — independent beam/shockwave/debris components, stylized mask/material quality, animation runtime split.
- uc studio, kick objects, 40s, https://youtube.com/shorts/t3jNQlXaEz8 — object rigid-body physics and contact vs keyframed block movement, not evidence of professional fighting.
- WiseFrog, Spin Tool bottle, 36s, https://youtube.com/shorts/XplZvYsRNSU — revolve a carefully traced cross-section and subdivide, generalizable to balusters/columns/ornaments; the 4.5 native `bmesh.ops.spin` was reproduced on 3 profiles.
- Round Table Animation, full fight scene Blender 5.0, 2h 7m, https://youtu.be/-31tUs696uQ — 17 metadata chapters, live-action fight blocking, interaction, hit poses and polish. Sampled selected parts, not every minute. Full manual artist-finesse must be captured in motion process and tested.

## Hard gate suite for each of the 25 professional demonstrations
1. Provenance + use-rights; exact references inspected and target look codified.
2. Editable source with character/environment identity controls, independent mask color/gradient and deterministic variants.
3. Expert visual comparisons close (head/hands) / medium / silhouette / multiple poses, motion playback; no ugly artifacts.
4. Correct skeletal, morphology/IK deformation, contact or surface topology as appropriate.
5. Native UE objects of correct class with actual response: AnimSequence+Notifies, Niagara, UV/PBR masks, Gaea heightfields, and relevant gameplay playback.
6. Profile and quality on Android, if relevant. Source high-fidelity and reduced in-engine variants must look consistent, not destructive quality downgrade.
7. Responsible Gate art approval; score 0/25 until genuine art signoff. No automatic self-promotion.

## Fast path to actually valuable completion

- **First hero validation**: one existing Koikatsu-derived character, complete eye/hair shading repair and confident reference match; one precise combat action with impact/recovery that can replay on its intended UE Skeleton, an AnimNotify hook and associated Niagara event; edit and polish with UE Control Rig. Judge this against good combat games at native gameplay runtime.
- **Second hero**: a character-specific creature/wing/mount using deliberate anatomy and gait, retarget/physical IK; no primitive ellipsoid body parts at acceptance.
- **Third hero**: one densely built believable architectural locus blended into actual Gaea terrain, close-up textures/stonework/interior details, UE lighting and Android metrics. Then scale via reusable asset variants.
- Expand to other 22 demonstrations only after the pattern is genuinely approved. Quality > speed; faster iteration comes from reuse and batch visual regression, **not** abandoning aesthetic approval.

## Current verified status (not to embellish)
- Blender 4.5.10 has source/render scripting and Unreal 5.8 isolated native asset import; current more detailed architecture remains visibly below specified benchmark.
- Blender 5.2.2 official portable download/installation process started, validation pending.
- Native UE 5.8 animation library, retarget and Control Rig APIs discovered; isolated native timed AnimNotify authoring under test.
- Existing `ANIMATION_3D/game_action_audit.py` rejected a camera-only sanctuary and passed *structural* skeletal motion of noncanonical mount/KK sources; **artistic motion never accepted**.
- 25/25 demo professional art approvals remain unresolved, 0 approved.
