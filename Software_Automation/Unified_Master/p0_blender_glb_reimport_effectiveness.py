"""Actual Blender GLTF importer nonzero morph diagnostic. Starts factory empty, only QA."""
import bpy,pathlib,json,numpy as np
ROOT=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011')
OUT=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
report={}
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 bpy.ops.wm.read_factory_settings(use_empty=True)
 p=ROOT/'BLENDER_FRESH'/state/f'LUMINOUS_V3_{state}_skinned.glb'
 bpy.ops.import_scene.gltf(filepath=str(p))
 nobj=0;targets=0;effective=0;meshes=[]
 for obj in bpy.data.objects:
  if obj.type!='MESH' or not obj.data.shape_keys:continue
  kb=obj.data.shape_keys.key_blocks
  verts=len(obj.data.vertices)
  base=np.empty(verts*3,dtype=np.float32);kb[0].data.foreach_get('co',base)
  nonzero=[]
  for index,x in enumerate(kb):
   if index==0:continue
   data=np.empty(verts*3,dtype=np.float32);x.data.foreach_get('co',data)
   nz=bool(np.any(np.abs(data-base)>1e-7))
   if nz:nonzero.append(x.name)
  nobj+=1;targets+=len(kb)-1;effective+=len(nonzero)
  meshes.append({'name':obj.name,'vertex_count':verts,'keys':len(kb)-1,'effective_keys':len(nonzero),'first_effective':nonzero[:12]})
 report[state]={'source_glb':str(p),'mesh_with_morphs':nobj,'imported_keyblocks':targets,'effective_imported_keyblocks':effective,
                'blender_version':bpy.app.version_string,'meshes':meshes,'native_unreal_status':'NOT_TESTED'}
 print('BLENDER_GLB_REIMPORT',state,'morph_meshes',nobj,'total',targets,'effective',effective,flush=True)
OUT.joinpath('P0_BLENDER_GLB_REIMPORT_EFFECTIVE_MORPHS.json').write_text(json.dumps(report,indent=2))
