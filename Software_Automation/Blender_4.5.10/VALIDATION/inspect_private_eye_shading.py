"""Read-only material transparency/shader routing diagnostics for converted Koikatsu facial assets."""
import bpy,json,os
ROOT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
bpy.ops.wm.open_mainfile(filepath=os.path.join(ROOT,"NW_ChikaHaruno_Acting72Frames_NONCANONICAL.blend"))
records=[]
for m in bpy.data.materials:
 if not any(x in m.name.lower() for x in ("eyeline","eye","mayu","sirome","hitomi","nose","face")):continue
 nodes=m.node_tree.nodes if m.use_nodes and m.node_tree else []
 p=next((n for n in nodes if n.type=="BSDF_PRINCIPLED"),None)
 a=p.inputs.get("Alpha") if p else None
 r={"name":m.name,"alpha":a.default_value if a else None,
    "alpha_linked":[x.from_node.name+"."+x.from_socket.name for x in a.links] if a else [],
    "surface_render_method":getattr(m,"surface_render_method",None),
    "use_nodes":m.use_nodes,
    "images":[{"image":n.image.name if n.image else None,
      "alpha_mode":n.image.alpha_mode if n.image else None,
      "size":list(n.image.size) if n.image else None,
      "outputs":[l.to_socket.name for output in n.outputs for l in output.links]} for n in nodes if n.type=="TEX_IMAGE"],
    "inputs":[{"node":n.name,"type":n.type} for n in nodes]}
 records.append(r)
res={"material_count":len(records),"materials":records}
with open(os.path.join(ROOT,"eye_shader_inspection.json"),"w") as f:json.dump(res,f,indent=2)
print("EYE_MATERIALS="+json.dumps([{"name":r["name"],"alpha":r["alpha"],"linked":r["alpha_linked"],"method":r["surface_render_method"]} for r in records]))
