"""Test 3: actual rigged KKBP PMX to editable Blender scene, preserve material and morph meshes."""
import bpy,sys,pathlib,json,time,hashlib,traceback
root=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\TOOLS')
sys.path.insert(0,str(root/'blender_mmd_tools_noWorld'))
sys.path.insert(0,str(root/'mmd_blender_python_deps'))
import mmd_tools
mmd_tools.register()
src=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\Export_PMX\20261016011323_ChikaHaruno')
out=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261016\TEST03_ORANGE_BRAID')
out.mkdir(parents=True,exist_ok=True)
base=src/'model.pmx';clothes=src/'Outfit 00'/'model.pmx'
for p in [base,clothes]:
 if not p.is_file() or p.stat().st_size<100000:raise FileNotFoundError('Incomplete native PMX: '+str(p))
start=time.perf_counter()
bpy.ops.wm.read_factory_settings(use_empty=True)
for role,pmx in [('character',base),('outfit',clothes)]:
 result=bpy.ops.mmd_tools.import_model(filepath=str(pmx),scale=.08,types={'MESH','ARMATURE','MORPHS','DISPLAY'},clean_model=False,remove_doubles=False)
 print('NATIVE_IMPORT',role,str(result),'time',round(time.perf_counter()-start,1),flush=True)
meshes=[o for o in bpy.data.objects if o.type=='MESH']
arms=[o for o in bpy.data.objects if o.type=='ARMATURE']
summary=[]
for ob in meshes:
 box=[tuple(round(v,3) for v in corner) for corner in ob.bound_box]
 item={'name':ob.name,'verts':len(ob.data.vertices),'faces':len(ob.data.polygons),
       'shapekeys':len(ob.data.shape_keys.key_blocks) if ob.data.shape_keys else 0,
       'materials':[m.name if m else None for m in ob.data.materials[:8]],
       'bounds':[[min(c[i] for c in box),max(c[i] for c in box)] for i in range(3)]}
 summary.append(item)
 print('MESH',item['name'],item['verts'],str(item['materials'])[:180],flush=True)
# Preserve original rig, morphs, joint weights, cloth geometry and all imported assets.
blend=out/'Test03_OrangeBraid_NativeKKBP_Editable.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
receipt={'schema':'NW_TEST03_REAL_RIGGED_BLENDER_IMPORT_V1','status':'PASS_EDITABLE_PMX_TO_BLENDER',
  'source_pmx':str(base),'source_hash':hashlib.sha256(base.read_bytes()).hexdigest(),
  'outfit_pmx':str(clothes),'source_card':r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\UserData\chara\female\SM01_AUTOTEST_test03_orange_character_final_0113.png',
  'blender':str(blend),'size':blend.stat().st_size,
  'mesh_objects':len(meshes),'armatures':len(arms),'vertices':sum(len(x.data.vertices) for x in meshes),
  'shape_keys':sum(len(x.data.shape_keys.key_blocks) for x in meshes if x.data.shape_keys),
  'elapsed_seconds':round(time.perf_counter()-start,2),'objects':summary,
  'reference_fidelity_accepted':False,'unreal_runtime_accepted':False}
(out/'BlenderImportReceipt.json').write_text(json.dumps(receipt,indent=2))
print('TEST03_REAL_EDITABLE_3D='+json.dumps({k:v for k,v in receipt.items() if k!='objects'}),flush=True)
