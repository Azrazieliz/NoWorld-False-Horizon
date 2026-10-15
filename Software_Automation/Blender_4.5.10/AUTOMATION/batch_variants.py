"""Data-only batch variants for the engineering fixture; does not alter the source blend."""
import bpy, json, os, sys, time
BASE = os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
ROOT = os.path.join(BASE,"VALIDATION","local")
bpy.ops.wm.open_mainfile(filepath=os.path.join(ROOT,"NW_EngineeringTests.blend"))
start = time.perf_counter()
obj = bpy.data.objects["NW_Parametric_Pillars"]
modifier = obj.modifiers["NW_EditableArchitecturalBays"]
keys = {s.name:s.identifier for s in modifier.node_group.interface.items_tree if s.item_type=="SOCKET"}
modifier[keys["Bay Count"]] = 9
modifier[keys["Bay Spacing"]] = 1.12
obj.update_tag()
mat = bpy.data.materials["NW_Asymmetric_Copper"]
node = next(n for n in mat.node_tree.nodes if n.type=="GROUP")
node.inputs["Skew"].default_value = -1.35
node.inputs["Secondary"].default_value = (0.05,0.8,0.7,1)
bpy.context.view_layer.update()
evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
mesh = evaluated.to_mesh()
observed = len(mesh.vertices)
evaluated.to_mesh_clear()
assert observed == 72, "Expected 9 bays * 8 vertices; received "+str(observed)
dest = os.path.join(ROOT,"NW_EditableVariant.blend")
bpy.ops.wm.save_as_mainfile(filepath=dest,compress=True)
result={"status":"PASS","bay_count":9,"evaluated_vertices":observed,
  "gradient_skew":-1.35,"secondary_color":[0.05,0.8,0.7,1],
  "output":dest,"duration_seconds":round(time.perf_counter()-start,3)}
with open(os.path.join(ROOT,"variant_results.json"),"w") as f:json.dump(result,f,indent=2)
print("NOWORLD_VARIANT="+json.dumps(result))
