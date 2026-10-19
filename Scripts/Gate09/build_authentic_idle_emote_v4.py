"""Build three authentic standing-motion actions on the existing Luminous rig for visual QA."""
import bpy, pathlib, gzip, json, hashlib
from mathutils import Quaternion
base=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
source=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\BLENDER_FRESH\CLOTHED\LUMINOUS_V3_CLOTHED_skinned.glb')
dst=base/'G09_AuthenticIdleAndBow_Source_Review_v4.blend'
if dst.exists():raise FileExistsError(dst)
initial=hashlib.sha256(source.read_bytes()).hexdigest()
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source))
rig=next(x for x in bpy.data.objects if x.type=='ARMATURE' and len(x.pose.bones)==975)
lookup={p.name.lower():p.name for p in rig.pose.bones}
aliases={'cf_j_hips':'Center','cf_j_waist01':'Hips','cf_j_spine01':'Spine','cf_j_spine02':'Chest','cf_j_spine03':'Upper Chest','cf_j_neck':'Neck','cf_j_head':'Head'}
remap_source=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\addons\KKBP_v9.5.0\unpacked_importer\kkbp_importer\extras\animationlibrary\Rokoko retargeting list (koikatsu fbx to KKBP Rigify).json')
for pair in json.loads(remap_source.read_text(encoding='utf8'))['bones'].values():
 k,v=pair
 if k.lower() not in aliases:
  v=v.replace('_fk','').replace('_FK','')
  if v.lower() in lookup:aliases[k.lower()]=v
aliases.update({'cf_j_arm00_l':'Left arm','cf_j_arm00_r':'Right arm',
 'cf_j_forearm01_l':'Left elbow','cf_j_forearm01_r':'Right elbow',
 'cf_j_hand_l':'Left wrist','cf_j_hand_r':'Right wrist',
 'cf_j_thigh00_l':'Left leg','cf_j_thigh00_r':'Right leg',
 'cf_j_leg01_l':'Left knee','cf_j_leg01_r':'Right knee',
 'cf_j_foot_l':'Left ankle','cf_j_foot_r':'Right ankle',
 'cf_j_toes_l':'Left toe','cf_j_toes_r':'Right toe'})
import numpy as np
from mathutils import Matrix
# Source-specific reflected orientation solved using 19 native prefab â†” actual Blender rest joints.
restbasis=json.loads((base/'G09_Unity_Native_Prefab_RestBasis_20261019.json').read_text(encoding='utf8'))['joints']
C=Matrix(((-0.9999999999886833,-3.2147109572467595e-06,3.507019129683276e-06),
(-3.5021174604535354e-06,-0.0015234959985741276,-0.9999988394731655),
(-3.220050156096382e-06,0.9999988394741313,-0.001523495987298673)))
def native_rest_world_rotation(path):
 src=np.array(restbasis[path]['relative_prefab_world_rest'],float)[:3,:3]
 U,_,Vt=np.linalg.svd(src)
 R=U@Vt
 if np.linalg.det(R)<0:
  U[:,-1]*=-1;R=U@Vt
 return Matrix(R.tolist())
scene=bpy.context.scene;scene.render.fps=30
for path in sorted(base.glob('f_tachi_*_G09_Native_30FPS_Samples.json.gz')):
 with gzip.open(path,'rt') as f:d=json.load(f)
 name=d['source_clip_name']; frames=d['frames'];rest=d['native_prefab_rest_quaternions_xyzw']
 act=bpy.data.actions.new('G09_REAL_KK_'+name)
 slot=act.slots.new('OBJECT',rig.name)
 rig.animation_data_create();rig.animation_data.action=act;rig.animation_data.action_slot=slot
 used=0
 for sourcepath in frames[0]:
  bone=lookup.get(sourcepath.split('/')[-1].lower()) or lookup.get(aliases.get(sourcepath.split('/')[-1].lower(),'').lower())
  if not bone:continue
  raw=rest[sourcepath];qref=Quaternion((raw[3],raw[0],raw[1],raw[2])).normalized()
  pb=rig.pose.bones[bone];pb.rotation_mode='QUATERNION'
  Rsource=native_rest_world_rotation(sourcepath)
  Rtarget=pb.bone.matrix_local.to_3x3()
  coordinate=C @ Rsource
  for frame,item in enumerate(frames,1):
   raw=item[sourcepath]['local_rotation_xyzw']
   delta=(qref.inverted() @ Quaternion((raw[3],raw[0],raw[1],raw[2])).normalized()).to_matrix()
   pb.rotation_quaternion=(Rtarget.inverted_safe() @ coordinate @ delta @ coordinate.inverted_safe() @ Rtarget).to_quaternion()
   pb.keyframe_insert('rotation_quaternion',frame=frame,group='NativeStanding')
  used+=1
 act.asset_mark()
 print('EXTRACTED_ACTION',name,'BONES',used,'FRAMES',len(frames),flush=True)
rig.animation_data.action=bpy.data.actions[sorted(a.name for a in bpy.data.actions if a.name.startswith('G09_REAL_KK_'))[0]]
rig.animation_data.action_slot=rig.animation_data.action.slots[0]
scene.frame_set(15)
bpy.ops.wm.save_as_mainfile(filepath=str(dst),compress=True)
assert hashlib.sha256(source.read_bytes()).hexdigest()==initial
print('SOURCE_REVIEW_SAVED',dst,hashlib.sha256(dst.read_bytes()).hexdigest())
