# Workflow Library — Blender 4.5.10 verified operations

| Task | Primary control / tested method | UI fallback | Evidence / limitation |
|---|---|---|---|
| Rebuild engineering fixture | `blender -b --factory-startup -t 2 --python AUTOMATION/prototype_batch.py` | None required | engineering_results.json 10/10 |
| Create shader group sockets | `bpy.data.node_groups.new('...', 'ShaderNodeTree'); ng.interface.new_socket(...)` | Shader Editor > Group > New; N panel > Group | Run and saved |
| Make local asymmetry | Generated UV-like coordinate, split X/Y, skew Y, add noise warp, clamp, MixRGB | Shader Editor, group inputs | Saved, editable; not baked |
| Cross-object gradient | Geometry Position to group's Vector input | Shader Editor > Geometry > Position | Test authored |
| Create GN modifier controls | `GeometryNodeTree` interface sockets and `obj.modifiers.new(...,'NODES')` | Geometry Nodes workspace | Bay Count 5->7->9 test |
| Force depsgraph refresh | `obj.update_tag(); bpy.context.view_layer.update()` | Re-enter scene or edit modifier | Needed to avoid stale evaluation |
| Store Geometry Nodes cube UV | Mesh Cube UV Map -> Store Named Attribute (FLOAT2 / CORNER) -> mesh | Geometry Nodes > Attribute > Store Named Attribute | Source missing UV was corrected; 14/14 UV present |
| Query evaluated mesh | `obj.evaluated_get(depsgraph).to_mesh()` followed by `to_mesh_clear()` | Spreadsheet editor | 40 / 56 / 72 vertices observed |
| Build armature | Data API edit bones after setting Edit Mode; add mesh ARMATURE modifier | Armature Edit Mode; Object > Modifiers | Root + two wings tested |
| Key poses | pose bone `keyframe_insert('rotation_euler', frame)` | Pose Mode > Insert Keyframe | five keyed poses, frame 7 0.48rad |
| Verify 4.5 Action Slot | `obj.animation_data.action_slot.identifier` | Dope Sheet > Action Editor > Slots | Exists after re-open |
| Create Grease Pencil v3 frames | `bpy.data.grease_pencils_v3.new`, layers.new, frames.new, `drawing.add_strokes([5,...])` | 2D Animation workspace > Draw | four frames / 3 strokes |
| Preview | `scene.render.engine='BLENDER_WORKBENCH'; bpy.ops.render.render(write_still=True)` | Render > Render Image | 320x240, ~2.56 sec render |
| Export static scene | Select role-tagged meshes then `bpy.ops.export_scene.fbx(use_selection=True)` | File > Export > FBX | 91,868 bytes, Blender reimport success |
| Roundtrip | Re-open .blend, mutate sockets, reopen FBX | File > Open / Import FBX | 8/8 roundtrip tests |
| Inventory exact parameters | `AUTOMATION/rna_catalog.py` uses RNA of installed build | Python Console (Shift+F4) | 1657 descriptors |
| Save milestone | `bpy.ops.wm.save_as_mainfile(..., compress=True)` | File > Save As | source .blend 108,199 bytes |

Coordinates not calibrated; do not claim stable pixel targeting. Primary route is direct API. For specific UI-only workflows, first identify editor/layout/resolution/DPI, then measure and persist anchors, screenshots, shortcuts and recovery paths before calling them validated.

No Blender shader parity, facial preservation, Koikatsu material reconstruction, Gaea conversion, or native Unreal compatibility is currently proven.
