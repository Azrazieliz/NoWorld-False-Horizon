# Blender 4.5.10 — Source Atlas (first investigated tranche)

**Scope:** These are sources actually found and read at summary/documentation level on 2026-10-15. They are not represented as exhaustive reviews, downloaded books, verified licences, or full source-code audits. Site pages can change; validate target commit before dependency adoption.

1. **Blender 4.5 LTS release** — Blender Foundation  
   https://www.blender.org/download/releases/4-5/  
   Use/assessment: Primary feature/version overview; GP V3, nodes, tool panels. Checked release summary; not all linked features individually studied.

2. **Blender 4.5 release notes** — Blender developers  
   https://developer.blender.org/docs/release_notes/4.5/geometry_nodes/  
   Use/assessment: Version-specific nodes; new fields and Grease Pencil functions; no claim all nodes reviewed.

3. **Blender 4.5 GP changes** — Blender developers  
   https://developer.blender.org/docs/release_notes/4.5/grease_pencil/  
   Use/assessment: Dedicated GP render pass, SSAA, node-tool caveats.

4. **Blender Actions 4.4 upgrade** — Blender developers  
   https://developer.blender.org/docs/release_notes/4.4/upgrading/slotted_actions/  
   Use/assessment: Critical Action Slot migration; older scripts must not assume legacy fcurves.

5. **Blender Python Operators** — Blender API  
   https://docs.blender.org/api/4.5/bpy.ops.html  
   Use/assessment: Operator poll and context semantics; direct RNA preferred.

6. **Blender 4.5 Shader/Modifier Property API** — Blender API  
   https://docs.blender.org/api/4.5/bpy.types.Property.html  
   Use/assessment: RNA-based introspection of ranges/defaults; source of rna_catalog.py design.

7. **Node tree interface API** — Blender API  
   https://docs.blender.org/api/4.5/bpy.types.NodeTreeInterface.html  
   Use/assessment: new_socket controls, used and executed in GN/shader groups.

8. **Grease Pencil drawing Python API** — Blender API  
   https://docs.blender.org/api/4.5/bpy.types.GreasePencilDrawing.html  
   Use/assessment: add_strokes, point edits; executed successfully on installed Blender.

9. **Grease Pencil frame API** — Blender API  
   https://docs.blender.org/api/4.5/bpy.types.GreasePencilFrames.html  
   Use/assessment: Frame creation and copy semantics; four frame key test.

10. **Geometry Nodes simulation zone** — Blender Manual  
   https://docs.blender.org/manual/id/4.5/modeling/geometry_nodes/simulation/simulation_zone.html  
   Use/assessment: Frame-dependent state; data provenance and cache still require runtime testing.

11. **Geometry Bake node** — Blender Manual  
   https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/geometry/operations/bake.html  
   Use/assessment: Cached intermediate data; Blender-version reading not guaranteed.

12. **Geometry Nodes attributes** — Blender Manual  
   https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/attributes_reference.html  
   Use/assessment: Domains and named attribute conversion issues relevant to UVs/weights.

13. **Geometry Nodes UV Unwrap** — Blender Manual  
   https://docs.blender.org/UATEST/manual/en/4.5/modeling/geometry_nodes/mesh/uv/uv_unwrap.html  
   Use/assessment: Store UV on face corners as 2D named attribute; no actual UV generator test yet.

14. **Blender Studio Tool Pipeline** — Blender Studio  
   https://studio.blender.org/tools/  
   Use/assessment: Real production pipeline boundary, provenance/ownership roles.

15. **Blender Studio Asset Pipeline** — Blender Studio  
   https://studio.blender.org/tools/addons/asset_pipeline  
   Use/assessment: Task-layer ownership and publish pattern; not yet adopted in project.

16. **Blender Studio Rigging** — Blender Studio  
   https://studio.blender.org/tools/pipeline-overview/rigging  
   Use/assessment: CloudRig and hand-authored corrective shapes; advanced rig maturity reference.

17. **Blender Studio Brushstroke Tools** — Blender Studio  
   https://studio.blender.org/tools/addons/brushstroke_tools  
   Use/assessment: 3D-surface-linked painterly stroke layers and Geometry Nodes.

18. **KK Blender Porter Pack** — Independent GitHub  
   https://github.com/MediaMoots/KK-Blender-Shader-Pack  
   Use/assessment: KK/KKS character export, material bake/atlas workflow. Check license/patch version before adopting.

19. **KK shader earlier branch** — Independent GitHub  
   https://github.com/gdtiti/KK-Blender-Shader-Pack  
   Use/assessment: Earlier shader and baking notes; compare versions before use.

20. **Send to Unreal** — EpicGames GitHub  
   https://github.com/EpicGames/BlenderTools  
   Use/assessment: Epic-sponsored Send to Unreal, UE to Rigify, no direct integration validated.

21. **Send to Unreal quickstart** — EpicGames GitHub  
   https://github.com/EpicGames/BlenderTools/blob/main/docs/send2ue/introduction/quickstart.md  
   Use/assessment: Requires Unreal Python and remote execution for Send2UE; avoid shared project settings changes.

22. **Unreal 5.8 FBX import options** — Epic documentation  
   https://dev.epicgames.com/documentation/unreal-engine/fbx-import-options-reference-in-unreal-engine  
   Use/assessment: Normals, tangents, collision, LOD, morph toggles; isolated native verification needed.

23. **Unreal 5.8 morph targets** — Epic documentation  
   https://dev.epicgames.com/documentation/unreal-engine/fbx-morph-target-pipeline-in-unreal-engine  
   Use/assessment: Morph import pipeline, version caution; no Koikatsu face fidelity acceptance.

24. **Unreal 5.8 skeletal meshes** — Epic documentation  
   https://dev.epicgames.com/documentation/unreal-engine/fbx-skeletal-mesh-pipeline-in-unreal-engine  
   Use/assessment: Skeleton, UV, textures, animation restrictions; require UE import evidence.

25. **Open-source modern building generator** — Independent GitHub  
   https://github.com/outerreaches/blender-building-generator  
   Use/assessment: Community generator candidate; not installed; GPL and stability audit needed.

26. **Open-source WFC level architect** — Independent GitHub  
   https://github.com/dottyboy/BlenderLevelArchitect  
   Use/assessment: Procedural room solver candidate; early test release and GPL; not validated on host.

## Open source hygiene

Do not vendor KKBP, Send2UE, CloudRig, community generators, textures, models, training videos or paid content without verifying their licences and asset-use permissions. Cite sources and their commit/version in any future adoption. Data extracted from Koikatsu is provenance-sensitive and must retain original usage rights.

## UV regression investigation (2026-10-15)

27. **Cube Node UV Map output** — https://docs.blender.org/UATEST/manual/en/4.5/modeling/geometry_nodes/mesh/primitives/cube.html — The procedural cube provides a UV field, but output geometry does not automatically retain it as a named UV layer. Confirmed by runtime before/after inspection.
28. **Store Named Attribute** — https://docs.blender.org/manual/id/4.5/modeling/geometry_nodes/attribute/store_named_attribute.html — Store FLOAT2 on face-corner domain with name UVMap to preserve generated mesh UVs. Validated in Blender 4.5.10; native Unreal still outstanding.

## Cross-disciplinary production and research tranche (2026-10-15; later session)

29. **Blender Studio — modern rigging pipeline**. https://studio.blender.org/tools/artist-guide/asset-creation/rigging. Blender Studio uses procedural control rigs with hand-painted weights and handcrafted corrective shape keys; a powerful reason not to expect universal automatic artistic quality. Adapt design for source/control vs export/deformation skeleton boundaries. Public studio workflow; read at overview depth only.
30. **CloudRig source/tooling**. https://studio.blender.org/tools/addons/cloudrig/introduction. Advanced Rigify extension used in studio work; investigate version/licence and nonhuman customization before integration. Not installed or tested here.
31. **Sprite Fright rigging hybrid practice**. https://studio.blender.org/blog/one-thing-ive-learned-sprite-fright-rigging-edition/. Procedural rigs must permit manually authored face behavior; unique facial rigs cannot be made fully generic without serious quality loss. Studio-authored qualitative production evidence.
32. **Blender Studio Sprite Fright pipeline**. https://studio.blender.org/blog/the-blender-studio-pipeline/. Asset tracking, remote teams and publish/pull processes matter as much as geometry scripts. Adapt ideas, do not claim their system installed.
33. **Physics-Based Motion Refinement, SIGGRAPH Asia 2025**. https://doi.org/10.1145/3757376.3771379. Contact drift, foot sliding and inter-character penetrations require physical/contact-aware constraints; interpolation is insufficient. Accessible paper abstract/summary researched; implementation not adopted.
34. **Epic Unreal 5.8 Skeletal Mesh FBX**. https://dev.epicgames.com/documentation/unreal-engine/importing-skeletal-meshes-using-fbx-in-unreal-engine. Import SkeletalMesh and inspect in Persona; Epic states FBX 2020.2 import pipeline. Our Blender export uses FBX 7.4; Unreal validation is mandatory, not assumed.
35. **Epic Unreal 5.8 Animation FBX**. https://dev.epicgames.com/documentation/unreal-engine/importing-animations-using-fbx-in-unreal-engine?lang=en-US. Skeletal FBX and animation handling, one clip per file recommendations. Need importer script and actual Action/Skeleton verified.
36. **Epic Unreal 5.8 Static Mesh FBX**. https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine. Multiple UV sets/LODs, material conversion, collision naming conventions, per-file collision restrictions. Our Blender export is not native success proof.
37. **Independent 4.5+ building generator**. https://github.com/outerreaches/blender-building-generator. GPL3 building shells, storefront/interior/damage; compare with procedural sanctuary before reinvention. Claims game ready originate from project README and have NOT been independently validated.
38. **BuildingNodes generator**. https://github.com/Durman/BuildingNodes. Panel-based live-edit architecture but no roof tool and quad-only base, not automatically better than native Geometry Nodes. Version support unverified.
39. **BlenderLevelArchitect**. https://github.com/dottyboy/BlenderLevelArchitect. Wave Function Collapse-inspired floor/room/corridor/stair and geometry; advertised rough-edge early test release, GPL3. Candidate procedural dungeon synthesis; not tested.
40. **Anime cel NPR independent**. https://github.com/Sy1via-sys/anime-cel-render. MIT shared node strategy, geometry shell outline, normal fake-light shading; project specifically targets MMD/PMX and must be adapted/tested for native anime characters.
41. **Anime Tools 4.5 community suite**. https://github.com/AtomaHuro/Anime-Tools/blob/main/README.md. Cel shading, animated outlines, halftone/hatching, painterly/hand-drawn overlays; evaluate source, licence and performance before use.
42. **Blender 4.5 Grease Pencil inherited animation**. https://docs.blender.org/UATEST/manual/en/4.5/grease_pencil/animation/introduction.html. Cut-out objects can inherit parent/armature animation; GP frames need not equal FPS. Used as theory for executed 48-frame hybrid with 14 distinct drawings.
43. **Blender 4.5 Line Art modifier behavior**. https://docs.blender.org/UATEST/manual/en/4.5/grease_pencil/modifiers/generate/line_art.html. Each Line Art modifier may compute a separate occlusion pass; cached variants important to low-spec throughput. Not yet benchmarked.
44. **Blender 4.5 NPR Freestyle**. https://docs.blender.org/manual/en/4.5/render/freestyle/introduction.html. Freestyle supports Python controlled lines, with notable memory and geometric intersection limitations. Not yet compared to GP/geometry outlines on host.

**Independent audit rule:** Mere source discovery is not source-code review. The above items are source triage with specific operational significance; they do not count as mastered systems until a compatible implementation and validation is recorded.

## Deeper source triangulation on executed import and animation failures

45. **Unreal 5.8 Import via Interchange** — https://dev.epicgames.com/documentation/unreal-engine/importing-assets-using-interchange-in-unreal-engine — Read operating details. Unreal interchange is asynchronous and offers pipeline stacks; its FBX Interchange feature is classified experimental. Our imported test environment used the native editor import task API; do not assume all old FBX Python property access works unchanged in 5.8.
46. **Unreal FBX Animation Pipeline** — https://dev.epicgames.com/documentation/unreal-engine/fbx-animation-pipeline-in-unreal-engine?lang=en-US — Unreal currently documents one animation per file and FBX 2020.2 conversion; this motivated a separate armature-only file. 5.8.3 actually produced one AnimSequence after the dedicated export, validated.
47. **Unreal Animation Sequences** — https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-sequences-in-unreal-engine — Key requirement is an existing matching skeleton by names/hierarchy and animation-import checkbox; our initial combined FBX test lacked a native AnimSequence, recovered with animation-only FBX.
48. **Physics-Based Motion Refinement (SIGGRAPH Asia 2025)** — https://doi.org/10.1145/3757376.3771379 — Actually reviewed abstract/method/failure categories: floating, interpenetration, phantom contacts and foot sliding. Contact-aware cleanup and animator-authored contact keyframes are needed; a naive sine-wave limb rig is insufficient. Our measured 0.11-units/frame contact-step displacement demonstrates need for correction. Research insight not implemented as their published simulator.
49. **Blender Studio Rigging Artist Guide** — https://studio.blender.org/tools/artist-guide/asset-creation/rigging — Studio production uses CloudRig procedural rig generation plus *manual* weights/corrective shape keys, which must be preserved as review gates in any automatic pipeline.
50. **KK Blender Porter Pack source and changelog** — https://github.com/MediaMoots/KK-Blender-Shader-Pack — Independent KKBP exporter plus Blender importer and custom shader, versioned KK shader 6.0 source. Current test directly imported existing Gate12-created GLB shards via stock Blender importer; no KKBP addon audit/install/mapping undertaken yet.
51. **Grease Pencil cutout animation inheritance** — https://docs.blender.org/manual/en/4.5/grease_pencil/animation/introduction.html — Parent/rig animation reduces hand drawings per second. In actual 48-frame hybrid test, two GP layers had only 14 authored drawings in total.
52. **Blender Line Art caching tradeoffs** — https://docs.blender.org/manual/en/4.5/grease_pencil/modifiers/generate/line_art.html — Multiple modifiers can duplicate occlusion computations; shared cache may improve performance but limits certain settings. Not benchmarked on host.
