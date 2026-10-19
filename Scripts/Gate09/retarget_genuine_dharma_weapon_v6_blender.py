"""Gate09 preserve source and derive 3 genuine Mixamo Koikatsu-compatible sword actions on Luminous V3.
Do not modify frozen blend or source GLB. Mechanical three-hit composition is NOT artistically accepted.
"""
import bpy,pathlib,gzip,json,hashlib
from mathutils import Quaternion,Matrix
import numpy as np
B=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
source=B/'G09_AuthenticIdleAndBow_Source_Review_v4.blend'
samples=B/'G09_DharmaMixamoSource_v6_20261019'
dst=B/'G09_Dharma_WeaponSourceRetarget_v6_20261019.blend'
fbxdir=B/'DharmaWeaponInterchange_v6_20261019'
if dst.exists() or fbxdir.exists():raise FileExistsError('Never replace authored source / exports')
original=hashlib.sha256(source.read_bytes()).hexdigest()
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
rig=next(x for x in bpy.data.objects if x.type=='ARMATURE' and len(x.pose.bones)==975)
lookup={p.name.lower():p.name for p in rig.pose.bones}
aliases={'cf_j_hips':'Center','cf_j_waist01':'Hips','cf_j_spine01':'Spine','cf_j_spine02':'Chest',
 'cf_j_spine03':'Upper Chest','cf_j_neck':'Neck','cf_j_head':'Head'}
remap=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\addons\KKBP_v9.5.0\unpacked_importer\kkbp_importer\extras\animationlibrary\Rokoko retargeting list (koikatsu fbx to KKBP Rigify).json')
for pair in json.loads(remap.read_text(encoding='utf8'))['bones'].values():
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
bases=json.loads((B/'G09_Unity_Native_Prefab_RestBasis_20261019.json').read_text(encoding='utf8'))['joints']
C=Matrix(((-0.9999999999886833,-3.2147109572467595e-06,3.507019129683276e-06),
 (-3.5021174604535354e-06,-0.0015234959985741276,-0.9999988394731655),
 (-3.220050156096382e-06,0.9999988394741313,-0.001523495987298673)))
def native_rest_rotation(path):
 src=np.array(bases[path]['relative_prefab_world_rest'],float)[:3,:3]
 U,_,Vt=np.linalg.svd(src);R=U@Vt
 if np.linalg.det(R)<0:U[:,-1]*=-1;R=U@Vt
 return Matrix(R.tolist())
scene=bpy.context.scene;scene.render.fps=30
manifest=[]
for name,role,phase in [('Slash Advance','Hit01_DiagonalDraw','first'),
 ('Great Sword Slash 2','Hit02_ReturnSlash','second'),('Thrust Slash','Hit03_FinishingThrust','third')]:
 inp=samples/('G09_WeaponSource_'+name.replace(' ','_')+'_30fps.json.gz')
 with gzip.open(inp,'rt',encoding='utf8') as f:d=json.load(f)
 frames=d['frames'];rest=d['native_prefab_rest_quaternions_xyzw']
 action=bpy.data.actions.new('G09_DHARMA_GENUINE_SOURCE_'+role)
 slot=action.slots.new('OBJECT',rig.name)
 rig.animation_data_create();rig.animation_data.action=action;rig.animation_data.action_slot=slot
 used=[];seen=set()
 for sourcepath in sorted(frames[0]):
  short=sourcepath.split('/')[-1].lower()
  bone=lookup.get(short) or lookup.get(aliases.get(short,'').lower())
  if bone is None or sourcepath not in rest or sourcepath not in bases or bone in seen:continue
  if 'local_rotation_xyzw' not in frames[0][sourcepath]:continue
  seen.add(bone);used.append(bone)
  raw=rest[sourcepath];qref=Quaternion((raw[3],raw[0],raw[1],raw[2])).normalized()
  pb=rig.pose.bones[bone];pb.rotation_mode='QUATERNION'
  Rsource=native_rest_rotation(sourcepath)
  Rtarget=pb.bone.matrix_local.to_3x3()
  coord=C@Rsource
  inv=Rtarget.inverted_safe()@coord
  for frame,item in enumerate(frames,1):
   qraw=item[sourcepath]['local_rotation_xyzw'];delta=qref.inverted()@Quaternion((qraw[3],qraw[0],qraw[1],qraw[2])).normalized()
   pb.rotation_quaternion=(inv@delta.to_matrix()@coord.inverted_safe()@Rtarget).to_quaternion()
   pb.keyframe_insert('rotation_quaternion',frame=frame,group='AuthenticMixamoWeapon')
 if len(used)<15:raise RuntimeError('insufficient genuine retargeted source bones '+str((role,used)))
 action.asset_mark()
 scene.frame_start=1;scene.frame_end=len(frames)
 manifest.append({'motion_role':role,'source_clip':name,'real_animated_bones':len(used),
  'bone_sample':used,'source_frame_count':len(frames),'source_original_seconds':d['seconds'],
  'source_original_fps':d['original_fps'],'export_fps':30,'visual_acceptance':False,
  'full_three_hit_A1_world_ms_compliance':False})
 print('AUTHENTIC_DHARMA_MOTION',role,'FRAMES',len(frames),'BONES',len(used),flush=True)
# preserve all actions on character source, choose first source as current
first=bpy.data.actions['G09_DHARMA_GENUINE_SOURCE_Hit01_DiagonalDraw']
rig.animation_data.action=first;rig.animation_data.action_slot=first.slots[0]
scene.frame_start=1;scene.frame_end=76;scene.frame_set(20)
bpy.ops.wm.save_as_mainfile(filepath=str(dst),compress=True)
fbxdir.mkdir()
for row in manifest:
 act=bpy.data.actions['G09_DHARMA_GENUINE_SOURCE_'+row['motion_role']]
 rig.animation_data.action=act;rig.animation_data.action_slot=act.slots[0]
 scene.frame_start=1;scene.frame_end=row['source_frame_count'];scene.frame_set(1)
 bpy.ops.object.select_all(action='DESELECT')
 rig.select_set(True);bpy.context.view_layer.objects.active=rig
 f=fbxdir/('G09_LuminousV3_Dharma_'+row['motion_role']+'.fbx')
 bpy.ops.export_scene.fbx(filepath=str(f),use_selection=True,object_types={'ARMATURE'},
  add_leaf_bones=False,axis_forward='-Z',axis_up='Y',bake_anim=True,
  bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
  bake_anim_use_all_actions=False,bake_anim_simplify_factor=0.0)
 assert f.stat().st_size>100000
 row.update({'fbx':f.name,'fbx_sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'fbx_bytes':f.stat().st_size})
 print('EXPORTED_GENUINE_DHARMA',f.name,f.stat().st_size,flush=True)
assert hashlib.sha256(source.read_bytes()).hexdigest()==original
m={'schema':'G09_DHARMA_SOURCE_V6_3_NATIVE_SWORD_ACTIONS_INTERCHANGE_V1',
'original_blend_sha256':original,'new_blend_sha256':hashlib.sha256(dst.read_bytes()).hexdigest(),
'original_character_unmodified':True,'dharma_sword_asset_produced':False,
'three_hit_world_cast_accepted':False,'combination_artically_accepted':False,
'v6_hit_ms':[180,390,650],'v6_scaling_atk_bps':[4000,6000,9000],'actions':manifest}
(fbxdir/'INTERCHANGE_MANIFEST.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf8')
print('G09_DHARMA_3_GENUINE_SWORD_ACTIONS_SOURCE_AND_FBX_READY',dst,flush=True)
