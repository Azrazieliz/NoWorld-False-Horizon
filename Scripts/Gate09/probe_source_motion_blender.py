import bpy,sys,json,pathlib,hashlib
args=sys.argv[sys.argv.index('--')+1:]
gltf,bvh,out=map(pathlib.Path,args)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(gltf))
rigs=[x for x in bpy.data.objects if x.type=='ARMATURE']
initial={x.name for x in bpy.data.objects}
print('TARGET_RIGS',[(o.name,len(o.data.bones),list(o.data.bones.keys())[:100]) for o in rigs])
bpy.ops.import_anim.bvh(filepath=str(bvh),axis_forward='-Z',axis_up='Y',update_scene_duration=True)
sr=[x for x in bpy.data.objects if x.type=='ARMATURE' and x.name not in initial]
print('BVH_RIGS',[(o.name,len(o.data.bones),list(o.data.bones.keys()),o.animation_data.action.name if o.animation_data and o.animation_data.action else None) for o in sr])
result={'gltf_sha256':hashlib.sha256(gltf.read_bytes()).hexdigest(),'bvh_sha256':hashlib.sha256(bvh.read_bytes()).hexdigest(),
'rigs':[{'name':r.name,'bones':len(r.data.bones),'bone_names':list(r.data.bones.keys())} for r in rigs],
'source':[{'name':r.name,'bones':len(r.data.bones),'bone_names':list(r.data.bones.keys()),'action':r.animation_data.action.name if r.animation_data and r.animation_data.action else None} for r in sr]}
out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
