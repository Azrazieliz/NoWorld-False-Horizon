"""Gate09 import-back proof for animation-only FBX, no Unreal/Android claims."""
import bpy,sys,json,pathlib,hashlib
from mathutils import Quaternion
root=pathlib.Path(sys.argv[sys.argv.index('--')+1]);fbx=root/'G09_Luminous_CMU144_20_AnimOnly_PROOF.fbx'
out=root/'G09_Luminous_CMU144_20_FBX_Reload.json'
if out.exists():raise FileExistsError(out)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(fbx),use_anim=True)
rigs=[o for o in bpy.data.objects if o.type=='ARMATURE']
if not rigs:raise RuntimeError('No skeleton in FBX')
a=max(rigs,key=lambda o:len(o.pose.bones));action=a.animation_data.action if a.animation_data else None
if not action:raise RuntimeError('No animation action after FBX import')
s=bpy.context.scene
f0,f1=[int(n) for n in action.frame_range]
probe=['Left arm','Right arm','Left leg','Right leg','Head','Hips']
samples={}
for f in [f0,(f0+f1)//2,f1]:
 s.frame_set(f)
 samples[str(f)]={name:[round(x,6) for x in a.pose.bones[name].matrix.to_translation()] for name in probe if name in a.pose.bones}
diff=any(any(samples[str(f0)][k][i]!=samples[str(f1)][k][i] for i in range(3)) for k in samples[str(f0)])
if not diff:raise RuntimeError('FBX import has no actual pose change in tested bones')
r={'status':'ANIMATION_ONLY_FBX_REIMPORT_HAS_TIME_VARYING_SKELETAL_POSES','input_sha256':hashlib.sha256(fbx.read_bytes()).hexdigest(),
   'bone_count':len(a.pose.bones),'action':action.name,'frames':[f0,f1],'sampled_frame_translations':samples,
   'not_proven':['Foot lock','Camera facing','Character visual acceptance','UE runtime','Android','All wardrobe retarget']}
out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
print('G09_FBX_IMPORT_QA='+json.dumps({k:r[k] for k in ['status','bone_count','action','frames']}))
