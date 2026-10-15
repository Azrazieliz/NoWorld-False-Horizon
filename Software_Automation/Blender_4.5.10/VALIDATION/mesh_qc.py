"""Inspect actual Blender evaluated mesh UV and topology metrics."""
import bpy,json,os
root=os.path.join(os.path.dirname(__file__),"local")
bpy.ops.wm.open_mainfile(filepath=os.path.join(root,"NW_EngineeringTests.blend"))
dep=bpy.context.evaluated_depsgraph_get()
meshes=[]
for ob in bpy.data.objects:
    if ob.type!="MESH" or ob.get("nw_role")!="ARCH":continue
    ev=ob.evaluated_get(dep)
    mesh=ev.to_mesh()
    mesh.calc_loop_triangles()
    meshes.append({"name":ob.name,"verts":len(mesh.vertices),
                   "polygons":len(mesh.polygons),"triangles":len(mesh.loop_triangles),
                   "uv_sets":[x.name for x in mesh.uv_layers]})
    ev.to_mesh_clear()
report={"count":len(meshes),"meshes":meshes,
    "missing_uv":[x["name"] for x in meshes if x["polygons"] and not x["uv_sets"]]}
with open(os.path.join(root,"mesh_qc.json"),"w") as f:json.dump(report,f,indent=2)
print("NOWORLD_MESH_QC="+json.dumps(report))
