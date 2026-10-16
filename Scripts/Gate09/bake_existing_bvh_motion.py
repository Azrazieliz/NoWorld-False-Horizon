"""Gate09 source-motion deformation proof: real CMU BVH mapped onto unchanged imported Luminous rig.
Diagnostic retarget, not author-approved locomotion/combat or canonical pipeline.
"""
import bpy,sys,pathlib,hashlib,json,math
from mathutils import Vector
args=sys.argv[sys.argv.index('--')+1:];glb,bvh,outdir=map(pathlib.Path,args)
assert glb.is_file() and bvh.is_file()
assert outdir.is_dir()
sources={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (glb,bvh)}
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(glb))
target=next(o for o in bpy.data.objects if o.type=='ARMATURE' and len(o.pose.bones)>500)
names=set(bpy.data.objects.keys())
bpy.ops.import_anim.bvh(filepath=str(bvh),axis_forward='-Z',axis_up='Y',update_scene_duration=True)
source=next(o for o in bpy.data.objects if o.type=='ARMATURE' and o.name not in names)
pairs={'Hips':'Hips','LowerBack':'Spine','Spine':'Chest','Spine1':'Upper Chest',
 'Neck':'Neck','Head':'Head',
 'LeftShoulder':'Left shoulder','LeftArm':'Left arm','LeftForeArm':'Left elbow','LeftHand':'Left wrist',
 'RightShoulder':'Right shoulder','RightArm':'Right arm','RightForeArm':'Right elbow','RightHand':'Right wrist',
 'LeftUpLeg':'Left leg','LeftLeg':'Left knee','LeftFoot':'Left ankle','LeftToeBase':'Left toe',
 'RightUpLeg':'Right leg','RightLeg':'Right knee','RightFoot':'Right ankle','RightToeBase':'Right toe'}
missing=[(a,b) for a,b in pairs.items() if a not in source.pose.bones or b not in target.pose.bones]
if missing:raise RuntimeError('unmapped bones '+repr(missing))
scene=bpy.context.scene;frames=range(max(1,scene.frame_start),min(scene.frame_end,240)+1)
if len(frames)<8:raise RuntimeError('missing source animation frames')
fps=int(scene.render.fps) or 30
action=bpy.data.actions.new('G09_CMU_144_20_Luminous_RigRetarget_DIAGNOSTIC')
slot=action.slots.new('OBJECT',target.name)
target.animation_data_create();target.animation_data.action=action;target.animation_data.action_slot=slot
# Preserve user-authored source bone names. Map relative *local rotation* through rest-axis alignment.
align={}
for from_name,to_name in pairs.items():
 src=source.pose.bones[from_name];dst=target.pose.bones[to_name]
 align[to_name]=(dst.bone.matrix_local.to_3x3().inverted_safe() @ src.bone.matrix_local.to_3x3()).to_quaternion()
 dst.rotation_mode='QUATERNION'
for f in frames:
 scene.frame_set(f)
 pose={name:source.pose.bones[name].matrix_basis.to_quaternion().copy() for name in pairs}
 for source_name,target_name in pairs.items():
  pb=target.pose.bones[target_name];q=align[target_name]
  pb.rotation_quaternion=q @ pose[source_name] @ q.inverted()
  pb.keyframe_insert(data_path='rotation_quaternion',frame=f,group='G09_RealMotionRetarget')
scene.frame_start=min(frames);scene.frame_end=max(frames)
# Skin-deformation proof: compare evaluated vertex locations at three distinct frames on same source mesh.
meshes=[o for o in bpy.data.objects if o.type=='MESH' and any(m.type=='ARMATURE' and m.object==target for m in o.modifiers)]
if not meshes:raise RuntimeError('no truly skinned mesh attached to target rig')
mesh=max(meshes,key=lambda x:len(x.data.vertices))
picked=list(range(0,len(mesh.data.vertices),max(1,len(mesh.data.vertices)//96)))[:96]
def sample(f):
 scene.frame_set(f);bpy.context.view_layer.update()
 dg=bpy.context.evaluated_depsgraph_get();o=mesh.evaluated_get(dg);m=o.to_mesh()
 arr=[tuple(o.matrix_world @ m.vertices[i].co) for i in picked if i<len(m.vertices)]
 o.to_mesh_clear();return arr
frame_start=scene.frame_start;frame_mid=(scene.frame_start+scene.frame_end)//2;frame_end=scene.frame_end
s0,s1,s2=[sample(f) for f in (frame_start,frame_mid,frame_end)]
def displacement(a,b):return max((Vector(x)-Vector(y)).length for x,y in zip(a,b))
changes={'start_to_middle_m':displacement(s0,s1),'start_to_end_m':displacement(s0,s2)}
if changes['start_to_middle_m']<1e-4 and changes['start_to_end_m']<1e-4:
 raise RuntimeError('no measured real vertex deformation in animated skin')
# Single edit-ready native animation source, plus animation-only interchange.
scene.frame_set(frame_mid)
source.hide_render=True;source.hide_set(True)
action.asset_mark();action.asset_data.description='Authentic CMU 144_20 retarget diagnostic. No motion-quality acceptance.'
blend=outdir/'G09_Luminous_CMU144_20_Retarget_PROOF.blend'
fbx=outdir/'G09_Luminous_CMU144_20_AnimOnly_PROOF.fbx'
report=outdir/'G09_Luminous_CMU144_20_Retarget_PROOF.json'
for p in (blend,fbx,report):
 if p.exists():raise FileExistsError('refuse overwrite '+str(p))
bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
bpy.ops.object.select_all(action='DESELECT')
target.select_set(True);bpy.context.view_layer.objects.active=target
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,
 bake_anim=True,bake_anim_use_all_bones=False,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0.0)
if {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (glb,bvh)}!=sources:raise RuntimeError('immutable source changed')
result={'schema':'GATE09_AUTHENTIC_MOTION_RETARGET_QA_V1','source_hashes':sources,'source_motion':'CMU 144_20.bvh (authentic BVH, purpose not canonical)',
 'source_armature':source.name,'source_bones':len(source.pose.bones),
 'target_armature':target.name,'target_bones':len(target.pose.bones),
 'mapped_semantic_bones':pairs,'mapped_count':len(pairs),
 'clip_frames':[frame_start,frame_end],'fps':fps,'duration_seconds':(frame_end-frame_start)/fps,
 'actual_skin_mesh':mesh.name,'sampled_skin_vertices':len(picked),'vertex_deformation':changes,
 'files':{p.name:{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size} for p in (blend,fbx)},
 'status':'REAL_SOURCE_MOTION_SKIN_DEFORMATION_PROVEN_VISUAL_QA_PENDING',
 'limitations':['Source CMU movement is not Luminous-authored choreography','Retarget axis/rest/profile require multi-camera artistic inspection','No Koikatsu native morph curves or hair physics baked','No Unreal import or Android evidence','No foot-contact acceptance']}
report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('GATE09_REAL_MOTION_RECEIPT='+json.dumps({k:result[k] for k in ('status','mapped_count','target_bones','actual_skin_mesh','vertex_deformation','clip_frames','fps')},ensure_ascii=False))
