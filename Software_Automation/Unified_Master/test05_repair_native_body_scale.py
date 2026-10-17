"""Test 5 critical rig scale fix: KKBP exported .blend armature+mesh were both scaled,
producing DOUBLE-scale deformed geometry. Preserve editable rig and original source.
"""
import bpy,pathlib,json,time
root=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
for o in bpy.data.objects:
 if o.type=='ARMATURE' and o.name.startswith('model_arm'):
  o.scale=(1,1,1)
orig=bpy.data.objects['TEST05 | Real KKBP Skinned Character Base']
dp=bpy.context.evaluated_depsgraph_get()
ev=orig.evaluated_get(dp)
coords=[ev.matrix_world@v.co for v in ev.to_mesh().vertices]
minz=min(v.z for v in coords);maxz=max(v.z for v in coords)
width=max(v.x for v in coords)-min(v.x for v in coords)
ev.to_mesh_clear()
print('FIXED_SOURCE_KK_CHAR_DIMENSIONS',width,maxz-minz,'z',minz,maxz,flush=True)
if not 1<maxz-minz<2.5:raise RuntimeError('Native body still incorrectly scaled')
scene=bpy.context.scene
scene.render.resolution_percentage=57
scene.render.filepath=str(root/'TEST05_BodyScaleCorrected_VisualQA.png')
dest=root/'TEST05_CrystallineDragonSeraph_Editable_v2.blend'
if dest.exists():raise FileExistsError(dest)
bpy.ops.wm.save_as_mainfile(filepath=str(dest))
result={'status':'PASS_REAL_NATIVE_KKBP_ARMATURE_SCALE_REPAIR',
  'original_blend_untouched':True,'native_body_eval_height':maxz-minz,
  'native_body_eval_width':width,'source_mesh_rig_weight_preserved':True,
  'output_blend':str(dest),'visual_fidelity_accepted':False}
(root/'TEST05_BODY_SCALE_REPAIR.json').write_text(json.dumps(result,indent=2))
print('TEST05_REPAIRED_EDITABLE_BLEND',dest,flush=True)
bpy.ops.render.render(write_still=True)
print('TEST05_CORRECTED_PREVIEW',scene.render.filepath,flush=True)
