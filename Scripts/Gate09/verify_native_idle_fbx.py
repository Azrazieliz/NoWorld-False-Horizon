"""Native Blender clean reimport tests of Gate09 FBX Actions, no Unreal claims."""
import bpy,pathlib,json,hashlib,math
from mathutils import Vector
root=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\UnrealInterchange_20261019')
receipts=[]
for f in sorted(root.glob('*.fbx')):
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(f),use_anim=True)
 arms=[o for o in bpy.data.objects if o.type=='ARMATURE']
 if len(arms)!=1:raise RuntimeError('ambiguous FBX armature')
 rig=arms[0];bones=rig.pose.bones
 if len(bones)!=975:raise RuntimeError('missing source rig bones')
 if not rig.animation_data or not rig.animation_data.action:raise RuntimeError('missing FBX Action')
 action=rig.animation_data.action
 first,last=[int(x) for x in action.frame_range]
 measured={}
 for frame in (first,(first+last)//2,last):
  bpy.context.scene.frame_set(frame);bpy.context.view_layer.update()
  measured[str(frame)]={key:list((rig.matrix_world @ bones[key].matrix @ Vector((0,.15,0))).to_tuple())
    for key in ('Left arm','Right arm','Head','Left leg','Right leg') if key in bones}
 movements={}
 for key in measured[str(first)]:
  a=Vector(measured[str(first)][key]);b=Vector(measured[str((first+last)//2)][key])
  c=Vector(measured[str(last)][key])
  movements[key]=max((a-b).length,(a-c).length)
 maximum=max(movements.values())
 if maximum<0.005:raise RuntimeError('FBX Action does not move limbs '+f.name)
 receipts.append({'filename':f.name,'sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'bones':len(bones),
 'frame_range':[first,last],'action':action.name,'maximum_three_sample_movement_m':maximum,
 'tested_bone_movement_m':movements,'status':'PASS_ACTUAL_SKELETAL_ACTION_REIMPORT_NOT_ART_APPROVAL'})
 print('REIMPORT',f.name,len(bones),first,last,round(maximum,4),flush=True)
out=root/'BLENDER_FBX_REIMPORT_QA.json'
if out.exists():raise FileExistsError(out)
out.write_text(json.dumps({'schema':'GATE09_BLENDER_REIMPORT_INTERCHANGE_V1','results':receipts,
'unreal_engine':'NOT_TESTED','ground_contact':'NOT_TESTED','artistic_approval':False},indent=2)+'\n',encoding='utf8')
print('VERIFIED',len(receipts),'FBX WITH_REAL_MOTION')
