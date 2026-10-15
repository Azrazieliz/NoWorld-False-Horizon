# Blender 4.5.10 Mastery — Phase 2 Native Execution Evidence

**Date:** 2026-10-15. **Acceptance:** engineering validation only; 0/25 independent professional demonstration families accepted. Do not mistake source existence or import for professional quality.

## Noncanonical Blender projects generated
- `ENVIRONMENTS/parametric_sanctuary.py` executes a parameterized fantasy architecture/landmark with 200 editable source objects (arch stones, towers, masonry, spire, path, 11 seeded tree assemblies, rock dressing). It produced `NW_ParametricSanctuary.blend`, `.png`, `.fbx`. Measured ~2.42 seconds for entire script run including 1000x620 Workbench image, on 2 Blender threads. Not sophisticated architectural construction; reference is independent test art.
- `CREATURES/rigged_beast.py`: 36 distinct mesh parts, 16 deformation bones, four legs, 2 wings, face shape key, tail, reins/saddle and rider socket, 48 keyed frame duration. `NW_ArticulatedMount.blend`, 900x600 Workbench render and 906 KB skeletal FBX saved. Art quality is an engineering block-out, not refined anime production.
- `ANIMATION_2D/hybrid_strike.py`: 48 frames @24 fps (2 seconds), 14 Grease Pencil drawings across two independently editable GP objects, keyframe-driven 3D sword pivot, actual `NW_Hybrid_Strike.mp4` and `NW_Hybrid_Strike.blend`. Test succeeded in ~2.98 seconds. Characterless effect test, not animated series quality.
- `INTEGRATIONS/optimized_static_export.py`: non-destructive material-group aggregation of 200 source objects to 9 derived export meshes. Original editable sanctuary unchanged. All 9 optimized materials retain UV layer. Workbench pixel difference from original measured mean RGB abs difference 0.047, 0.044, 0.044 (8-bit channels) over 1000x620; exact identical pixels were NOT obtained, visual diff requires final-quality material comparisons. Blender throughput ~1.59 sec.

## Blender to Unreal 5.8.3 — actual isolated engine verification
Sandbox `D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015\NW_BlenderInterop.uproject`. Never the production NoWorld UE project.
- First run imported **200 actual StaticMesh assets** from unbatched sanctuary. Then failed due missing `FbxSkeletalMeshImportData.import_morph_targets` direct attribute. Corrected by `set_editor_property('import_morph_targets',True)`; next run created actual SkeletalMesh and Skeleton.
- Native SkeletalMesh audit: 6,244 vertices at LOD0, 6 sections, exact morph `NW_Facial_Expressive_Squeeze` present. This is a meaningful skeletal *import* proof, not body likeness or quality proof.
- `INTEGRATIONS/optimized_static_export.py` produced batched FBX; Unreal imported **9 actual StaticMesh** assets independently.
- Initial animation import found no AnimSequence despite FBX action datablocks surviving Blender reimport. `INTEGRATIONS/export_mount_animation.py` exports 1–48 as a separate ARMATURE-only FBX; rerunning the isolated UE importer against that file generated **1 native AnimSequence**.
- Blender roundtrip source GLB/FBX: 16 rig bones, 36 skeletal mesh parts, facial morph, 48 keyed source frames and animation action; importing action requires explicit Action Slot assignment in Blender 4.5, after which sampled pose rotation changes.
- **Unverified:** pixel-equivalent materials, shading, normals/texture quality at final material, Unreal rig pose and motion playback/retarget quality, Android performance, world-persistent source contracts. No Unreal production content was altered.

## Koikatsu six-shard read-only import audit
Source root is the preexisting Gate 12 working data, never copied or modified: `D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu\full_fidelity_shards`. Each six GLB sources successfully imported with actual Blender glTF importer.
- 12 mesh objects across 6 individually imported GLB shard scenes; summed **34,007 vertices / 53,670 polygons** across shards; **all 6** embedded an armature of **121 bones**.
- 0 meshes without UV layers. Distinct shape-key *names* among separate scenes: **602**, so do not sum all scene shape counts to claim distinct facial expressions. 598,598,453,147,1,0 nonbasis key counts per individual shard.
- Shader/image import still differs from native KK rendering; no approved identity reconstruction, complete merged character, material color parity, animation retarget or mod asset rights transfer. Audit script `INTEGRATIONS/koikatsu_glb_audit.py` is read-only and accepts path CLI arguments.

## Measured failures and quality blockers
- Native Unreal animation through a combined skeletal FBX using animation-only settings initially produced no AnimSequence; separate armature-only FBX resolved the single recorded case.
- Blender skeletal FBX reimport made Action datablocks but initially had no assigned rig action; assign imported action plus Action Slot explicitly.
- Lower-leg world-space foot endpoints in quadruped in-place 48-frame motion have maximum observed grounded horizontal frame-to-frame displacement ~0.1104 Blender scene units; automatic contact heuristic is coarse, yet these displacements preclude any claim of foot-locked quality. A physics-based solution or contact-aware IK and visual QC remains outstanding. Relevant research: SIGGRAPH Asia 2025 physics-based motion refinement, doi 10.1145/3757376.3771379.
- Actual 2D trial, mount anatomy and sanctuary geometry are simple stylized prototypes, not professional-quality polished art. Workbench previews are not equivalent to an Eevee/Cycles quality evaluation.

## Direct reproduction
Read `START_HERE.md` for first batch, then Phase 2 scripts. Run scripts in Blender 4.5.10 with `--background --factory-startup --threads 2 --python SCRIPT`, using isolated worktree and no live UE project. UE scripts and minimal sandbox `.uproject` are preserved in `INTEGRATIONS/UNREAL_5_8_3/`; their current paths reference the isolated local machine and should be parameterized for deployment to other PCs. UE 5.8.3 headless command: `UnrealEditor-Cmd.exe <isolated.uproject> -run=pythonscript -script=<absolute_python_script> -nullrhi -unattended -nosplash -nop4 -NoSound`. Tests can take ~30–60 seconds for UE startup even on simple assets.

## Evidence
Machine-readable reports in `VALIDATION/phase2/` and `VALIDATION/phase2/Unreal/`. Acceptance matrix `VALIDATION/phase2/DEMONSTRATION_STATUS.json` enumerates 25 families and marks ALL as professionally unaccepted. This accurately separates individual technical validations from the full mastery commission.

## Further animation engineering correction (2026-10-15)

- `ANIMATION_3D/contact_ik_gait.py` reloads the prior 16-bone mount and adds four world-space foot target empties, lower-leg IK constraints (two-bone chain) and explicit root travel. Standstill initial posture left the original leg chains fully stretched, yielding target errors around 0.138 units; lowering rig root by 0.16 avoided reach singularity. Re-executed 48 frames, 4 legs: **maximum planted-foot horizontal frame-to-frame change 0.000018 units** and **maximum solver target error 0.000078 units**. This is numerical contact evidence, not biomechanical quality acceptance. Source: `VALIDATION/phase2/mount_contact_ik_results.json`, independent derivative `NW_ArticulatedMount_IK.blend`.
- Direct IK-constraint -> armature FBX export was reimported by Blender with **up to 0.1284 units swing-frame positional difference** in sampled foot positions; stance sample positions were preserved. This failed the general pose-fidelity gate. `VALIDATION/phase2/contact_ik_fbx_roundtrip.json` remains as failure evidence.
- `ANIMATION_3D/bake_contact_ik_to_action.py` evaluated 48 IK frames and `bpy.ops.nla.bake(... visual_keying=True, clear_constraints=True)` converted the evaluated deformation pose into an animation action. **Discovered bug:** `use_current_action=False` dropped object root-location animation, initially shifting the apparent leg samples by 2.05625 units. Fix was to explicitly reinsert the 48 world-root location keys into the new action. Re-executed and measured maximum source-to-baked world foot error **0.000001 scene units** at frames 1,12,24,36,48; IK constraints all removed from the deform-only derivative. See `VALIDATION/phase2/contact_ik_manual_bake.json`.
- The resulting `NW_ArticulatedMount_IK_Baked.blend` and `NW_ContactIK_Baked_AnimOnly.fbx` preserve editable animation source and separately exportable deform motion, subject to independent FBX reimport and Unreal playback QC. Do not declare professional locomotion or gait before full-body anatomy, weight, ground friction, contact continuity through blend transitions, retarget and native playback quality reviews.

## Further export validation failure and artistic standard
The baked 48-frame foot-contact IK action imported into Unreal 5.8.3 as one native AnimSequence. However, independent Blender FBX reimport of this baked animation differs from the editable source by up to **0.128432 scene units** in swing frames 24 and 48. The 16-bone skeleton and action exist, but animation fidelity is not accepted. Consult `VALIDATION/phase2/contact_ik_baked_roundtrip.json` and `VALIDATION/phase2/Unreal/ue_ik_baked_results.json`. This issue must be resolved before professional animation or Unreal parity may be accepted.

The non-negotiable quality requirements, including refined artistic work beyond simple automation, are defined in `ARTISTIC_PRODUCTION_ACCEPTANCE.md`. Engineering implementation does not qualify as professional aesthetic acceptance.
