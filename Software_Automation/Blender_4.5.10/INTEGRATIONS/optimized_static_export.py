"""Material-batched static FBX derivative. Preserves canonical editable source untouched.
Blender 4.5.10; applies source modifiers only to the derived export version.
"""
import bpy,os,json,time,collections,hashlib
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,'VALIDATION','phase2')
start=time.perf_counter()
original=os.path.join(OUT,'NW_ParametricSanctuary.blend')
bpy.ops.wm.open_mainfile(filepath=original)
objects=[o for o in bpy.data.objects if o.get('nw_role')=='ENV' and o.type=='MESH']
num_before=len(objects)
groups=collections.defaultdict(list)
for o in objects:
 m=o.data.materials[0] if len(o.data.materials)>0 else None
 groups[m.name if m else 'Unassigned'].append(o)
records=[]
for material_name,obs in sorted(groups.items()):
 bpy.ops.object.select_all(action="DESELECT")
 for ob in obs:ob.select_set(True)
 bpy.context.view_layer.objects.active=obs[0]
 # Non-destructive to source file; scene has been loaded into isolated process.
 bpy.ops.object.convert(target="MESH")
 bpy.ops.object.join()
 selected=bpy.context.view_layer.objects.active
 selected.name="NW_ExportBatch_"+material_name
 selected["nw_batch_material"]=material_name
 # Keep original world matrices and local UV data through object joins.
 records.append({"name":selected.name,"count_original":len(obs),
   "faces":len(selected.data.polygons),"uv_layers":[u.name for u in selected.data.uv_layers]})
all_mesh=[o for o in bpy.data.objects if o.type=='MESH' and o.get('nw_batch_material')]
assert len(all_mesh)==len(groups), ("batch count mismatch",len(all_mesh),len(groups))
assert all(len(o.data.uv_layers)>0 for o in all_mesh), "batch destroyed at least one UV map"
scene=bpy.context.scene;scene.frame_set(1)
outblend=os.path.join(OUT,'NW_ParametricSanctuary_BatchedExport.blend')
bpy.ops.wm.save_as_mainfile(filepath=outblend,compress=True)
scene.render.filepath=os.path.join(OUT,'NW_ParametricSanctuary_Batched.png')
bpy.ops.render.render(write_still=True)
bpy.ops.object.select_all(action='DESELECT')
for o in all_mesh:o.select_set(True)
bpy.context.view_layer.objects.active=all_mesh[0]
fbx=os.path.join(OUT,'NW_ParametricSanctuary_Batched.fbx')
bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'MESH'},bake_anim=False,
    apply_unit_scale=True)
report={"blender":bpy.app.version_string,"source_mesh_count":num_before,
  "batched_mesh_count":len(all_mesh),"reduction_ratio":round(num_before/len(all_mesh),2),
  "total_polygons":sum(len(o.data.polygons) for o in all_mesh),
  "all_batches_have_uv":True,"source_unchanged":True,
  "batches":records,"file":fbx,"seconds":round(time.perf_counter()-start,3),
  "unreal_materials_equivalent":False,"native_unreal_import_tested":False}
with open(os.path.join(OUT,"batched_export_results.json"),"w") as f:json.dump(report,f,indent=2)
print("NW_BATCH="+json.dumps({k:v for k,v in report.items() if k!="batches"}))
