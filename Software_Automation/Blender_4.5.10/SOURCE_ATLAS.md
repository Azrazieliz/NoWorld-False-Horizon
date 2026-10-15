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
