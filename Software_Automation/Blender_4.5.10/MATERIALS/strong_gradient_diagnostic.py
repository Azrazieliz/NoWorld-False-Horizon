"""Visible, intentionally exaggerated SAME-MESH asymmetrical gradient diagnostic.
Not a proposed canonical palette. Existing styled/untouched local assets remain.
"""
import bpy,os,json,time
OUT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
started=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_KK_ChikaHaruno_AdaptiveMaterials_PRIVATE.blend"))
materials=[]
for m in bpy.data.materials:
 if not m.get('nw_editable_gradient'):continue
 grp=next(n for n in m.node_tree.nodes if n.type=="GROUP" and n.node_tree and n.node_tree.name=="NW_AsymMultiObject_Gradient_v1")
 tc=next(n for n in m.node_tree.nodes if n.type=="MIX_RGB" and n.label=="Local (0) / Global world anchor (1)")
 tc.inputs["Fac"].default_value=1.0
 grp.inputs["Tint Strength"].default_value=1.0
 grp.inputs["Gradient Axis"].default_value=(.40,0,4.0)
 grp.inputs["Asymmetry"].default_value=.6
 grp.inputs["Noise Strength"].default_value=.12
 if "hair" in m.name.lower():
  grp.inputs["Gradient Offset"].default_value=-5.50
  grp.inputs["Bottom Tint"].default_value=(.34,.14,1,1)
  grp.inputs["Top Tint"].default_value=(1,1,.25,1)
 else:
  grp.inputs["Gradient Offset"].default_value=-3.25
  grp.inputs["Bottom Tint"].default_value=(.03,.95,1,1)
  grp.inputs["Top Tint"].default_value=(1,.12,.72,1)
 materials.append(m.name)
assert len(materials)==3
scene=bpy.context.scene;scene.render.resolution_x=500;scene.render.resolution_y=720
scene.cycles.samples=10
f=os.path.join(OUT,"NW_KK_ChikaHaruno_StrongGradient_Diagnostic.blend")
bpy.ops.wm.save_as_mainfile(filepath=f,compress=True)
png=os.path.join(OUT,"NW_KK_ChikaHaruno_StrongGradient_Diagnostic.png")
scene.render.filepath=png;bpy.ops.render.render(write_still=True)
r={"status":"VISIBLE_GRADIENT_TEST","materials":materials,"same_mesh_tint":True,
 "global_anchor_enabled":True,"noncanonical_palette":True,
 "original_untouched":True,"rebuild_meshes":0,
 "blend":f,"preview":png,"time_sec":round(time.perf_counter()-started,3)}
with open(os.path.join(OUT,"strong_gradient_test.json"),"w") as fh:json.dump(r,fh,indent=2)
print("NOWORLD_STRONG_GRADIENT="+json.dumps(r))
