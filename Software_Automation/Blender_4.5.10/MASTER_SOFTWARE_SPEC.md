# Master Software Specification — verified slice v0.1

## Architecture contract
1. Use Blender 4.5.10 native data APIs, node trees, mesh/BMesh/armatures and background CLI; context-dependent bpy.ops only where required (mesh primitives, FBX, save, render).
2. Keep parameters in reusable node-group sockets and Geometry Nodes modifier inputs. Colors and asymmetric gradients are *not* baked into a new mesh. Geometry position distinguishes world-space gradients across objects from Generated local gradients within each object.
3. Save milestone .blend sources; keep authored source graphs intact. Bake mesh or texture only for a separate disposable downstream export derivative.
4. Preserve provenance, dimension units, UVs, normals, reference pose, skeleton names, morph names, coordinate systems, and animation data through exports, and require importer-side checks for each.
5. Avoid sharing temporary Blender or Unreal preferences with active production processes.

## Implemented primitives
- `AUTOMATION/prototype_batch.py`: isolated procedural architectural deck, backdrop, crown, window trims, parametric pillar array. Reusable shader group `NW_Adaptive_Asymmetric_Color_v1` has sockets Vector, Primary, Secondary, Skew, Noise Warp; its output feeds Principled BSDF Base Color. Source selection between Generated and Geometry Position changes gradient scope.
- GN `NW_ArchitecturalBayArray_v1`: exposed Bay Count and Bay Spacing. Mesh Line -> generated Cube -> Transform Geometry -> Set Material -> Instance on Points -> Realize Instances -> Join Geometry. Evaluated topology proven to change with modifier parameters.
- Animation proof: three-bone root/wing armature, two weighted mesh surfaces, Euler keyframes at frames 1,7,13,19,25; Blender 4.5 slotted action persists on re-open.
- Grease Pencil v3 proof: one layer, four drawing frames with 3 five-point strokes each. This does not provide interpolation or cinematic quality.
- `AUTOMATION/rna_catalog.py`: introspects installed RNA metadata, writes `PARAMETER_REFERENCE.json`. Property metadata is *not* substitute for actual useful parameter range testing.

## Operational pitfalls discovered and corrected
- Blender 4.5 Action Slot supports `slot.identifier`, not `slot.name`; roundtrip proof initially failed on the incorrect attribute, corrected and passed.
- Changing Geometry Nodes modifier custom socket values alone can leave a stale evaluated result. Call `object.update_tag()` and `bpy.context.view_layer.update()` before depsgraph/evaluated mesh QC. A batch test initially observed the old 40 vertices after setting Bay Count=9; correction produced 72.

## Shader/export non-equivalence
Blender shader node graphs and procedural coordinate-dependent gradients are **source designs**, not portable Unreal materials. Unreal requires reconstructed material functions/instances or texture-baked and packed channels. For runtime parameterized edits, implement matching controls in Unreal, test on device, and never infer parity from FBX existence. Specific KK/KKS material preservation remains unverified.

## Test quality gates
- Script execution: exit status + machine-readable report.
- Native source persistence: reopen blend, assert sockets, modifiers, action slots, GP data.
- Regeneration: vary count and spacing without reconstructing objects; assert distinct evaluated vertices.
- Export: test file existence and in-Blender reimport of FBX separately from Unreal native import.
- Artistic approval: actual rendered reference comparisons, silhouette/face/style/lighting checks, motion critiques and Gate signoff; not yet achieved.
- License: do not vendor third-party add-ons or source assets without checking their licences.

## Release classification
Implemented and executed subsystems do not imply holistic proficiency; every capability has separate evidence in the coverage matrix. No PRODUCTION-READY classification at this milestone.
