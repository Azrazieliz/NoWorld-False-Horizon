"""Non-destructive global-space variation of an already editable adaptive shader scene.
Demonstrates coordinated gradients across objects (global Empty anchor), not unique flat-object colors.
"""
import bpy,os,json,time
OUT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
start=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_KK_ChikaHaruno_AdaptiveMaterials_PRIVATE.blend"))
variants=[]
for m in bpy.data.materials:
 if not m.get("nw_editable_gradient") or not m.use_nodes:continue
 group=next(n for n in m.node_tree.nodes if n.type=="GROUP" and n.node_tree and n.node_tree.name=="NW_AsymMultiObject_Gradient_v1")
 c=next(n for n in m.node_tree.nodes if n.type=="MIX_RGB" and n.label=="Local (0) / Global world anchor (1)")
 c.inputs["Fac"].default_value=1.0
 if "hair" in m.name.lower():
  group.inputs["Gradient Offset"].default_value=-.45
  group.inputs["Tint Strength"].default_value=.82
  group.inputs["Asymmetry"].default_value=.74
 else:
  group.inputs["Gradient Offset"].default_value=-.15
  group.inputs["Tint Strength"].default_value=.74
  group.inputs["Asymmetry"].default_value=.35
 variants.append({"material":m.name,"global_mix":1.0,
   "strength":group.inputs["Tint Strength"].default_value,
   "asymmetry":group.inputs["Asymmetry"].default_value})
assert len(variants)==3
scene=bpy.context.scene
outblend=os.path.join(OUT,"NW_KK_ChikaHaruno_GlobalGradients_PRIVATE.blend")
bpy.ops.wm.save_as_mainfile(filepath=outblend,compress=True)
scene.render.filepath=os.path.join(OUT,"NW_KK_ChikaHaruno_GlobalGradients_PRIVATE.png")
bpy.ops.render.render(write_still=True)
res={"version":bpy.app.version_string,"material_count":len(variants),
  "variant":variants,"seconds":round(time.perf_counter()-start,3),
  "base_meshes_recreated":0,"variant_blend":outblend,
  "render":scene.render.filepath,
  "actual_koikatsu_in_game_mod":False,"professional_art_accepted":False}
with open(os.path.join(OUT,"global_gradient_results.json"),"w") as f:json.dump(res,f,indent=2)
print("NW_GLOBAL_GRADIENT="+json.dumps(res))
