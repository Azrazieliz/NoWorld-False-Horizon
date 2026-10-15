"""Generate material-grouped FBX from highly detailed noncanonical sanctuary source, preserving .blend.
Do not batch animated gems or banners into static meshes (they move in source).
Check converted evaluated meshes + UV and list PBR material graph parity NOT preserved by FBX.
"""
import bpy,os,json,time,collections
BASE=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(BASE,"VALIDATION","phase4")
t=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_StarfallSanctum_ArtDirected.blend"))
src=[o for o in bpy.data.objects if o.type in {"MESH","CURVE"} and (o.get("nw_role")=="ENV" or o.get("nw_tier")=="art_directed_noncanonical")]
dynamic=[o for o in src if o.animation_data and o.animation_data.action]
static=[o for o in src if o not in dynamic]
groups=collections.defaultdict(list)
for ob in static:
 m=ob.data.materials[0] if len(ob.data.materials) else None
 groups[m.name if m else "NOMATERIAL"].append(ob)
records=[];objects=[]
for n,group in sorted(groups.items()):
 bpy.ops.object.select_all(action="DESELECT")
 for ob in group:ob.select_set(True)
 bpy.context.view_layer.objects.active=group[0]
 bpy.ops.object.convert(target="MESH")
 bpy.ops.object.join()
 merged=bpy.context.view_layer.objects.active
 merged.name="NW_StaticBatch_"+n.replace(" ","_")
 merged["nw_derived_material_batch"]=n
 if not merged.data.uv_layers and len(merged.data.polygons)>0:
  bpy.ops.object.mode_set(mode="EDIT")
  bpy.ops.mesh.select_all(action="SELECT")
  bpy.ops.uv.smart_project(island_margin=.01)
  bpy.ops.object.mode_set(mode="OBJECT")
 assert merged.data.uv_layers,("UV lost",n)
 records.append({"mat":n,"source_count":len(group),"mesh_name":merged.name,
   "triangles":sum(len(poly.vertices)-2 for poly in merged.data.polygons),
   "uv_layers":[u.name for u in merged.data.uv_layers]})
 objects.append(merged)
# Native editable derivation.
outblend=os.path.join(OUT,"NW_StarfallSanctum_OptimizedStaticDerivative.blend")
bpy.ops.wm.save_as_mainfile(filepath=outblend,compress=True)
bpy.ops.object.select_all(action="DESELECT")
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
path=os.path.join(OUT,"NW_StarfallSanctum_OptimizedStatic.fbx")
bpy.ops.export_scene.fbx(filepath=path,use_selection=True,
 object_types={"MESH"},apply_unit_scale=True,bake_anim=False)
r={"version":bpy.app.version_string,"input_objects":len(src),
 "input_static":len(static),"retained_dynamic_objects":len(dynamic),
 "static_material_groups":len(objects),"object_reduction_ratio":round(len(static)/len(objects),2),
 "groups":records,"optimized_file":path,"blend_derivative":outblend,
 "seconds":round(time.perf_counter()-t,3),
 "source_unchanged":True,"all_grouped_meshes_have_uv":True,
 "animation_exported_in_static_fbx":False,
 "blender_procedural_PBR_equal_in_unreal":False,"artist_quality_approved":False}
with open(os.path.join(OUT,"optimized_static_results.json"),"w") as f:json.dump(r,f,indent=2)
print("NW_STATIC_OPT="+json.dumps({k:v for k,v in r.items() if k!="groups"}))
