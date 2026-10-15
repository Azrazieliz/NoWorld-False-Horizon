"""Re-import skeletal FBX independently; evaluate bone, mesh, morph, animation preservation."""
import bpy, json, os, math, time
ROOT=os.path.join(os.path.dirname(__file__),"phase2")
start=time.perf_counter()
source=os.path.join(ROOT,"NW_ArticulatedMount.blend")
bpy.ops.wm.open_mainfile(filepath=source)
bones_expected=len(bpy.data.objects["NW_Skyrunner_Mount_Rig"].data.bones)
meshes_expected=sum(1 for o in bpy.data.objects if o.get("nw_source_part"))
head=bpy.data.objects["NW_Head"]
keys_expected=[x.name for x in head.data.shape_keys.key_blocks]
# Foot placement audit: evaluate bottom bone endpoint Z and stance horizontal displacement
rig=bpy.data.objects["NW_Skyrunner_Mount_Rig"]
tracks={}
for bone in ("FrontL_Lower","FrontR_Lower","HindL_Lower","HindR_Lower"):
 coords=[]
 for f in range(1,49):
  bpy.context.scene.frame_set(f)
  pb=rig.pose.bones[bone]
  p=rig.matrix_world @ (pb.matrix @ pb.bone.tail_local)
  coords.append([round(p.x,6),round(p.y,6),round(p.z,6)])
 deltas=[math.dist(a[:2],b[:2]) for a,b in zip(coords,coords[1:])]
 tracks[bone]={'min_z':min(q[2] for q in coords),
  'max_z':max(q[2] for q in coords),
  'max_frame_to_frame_xy':round(max(deltas),5),
  'near_ground_frames':sum(abs(q[2])<.08 for q in coords)}
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
path=os.path.join(ROOT,"NW_ArticulatedMount_Skeletal.fbx")
bpy.ops.import_scene.fbx(filepath=path)
rigs=[o for o in bpy.data.objects if o.type=='ARMATURE']
meshes=[o for o in bpy.data.objects if o.type=='MESH']
keys=[k.name for ob in meshes if ob.data.shape_keys for k in ob.data.shape_keys.key_blocks]
animated=[ob.name for ob in rigs if ob.animation_data and ob.animation_data.action]
reassigned={"attempted":False,"pass":False}
try:
 arm=rigs[0]
 action=next(a for a in bpy.data.actions if "Skyrunner_Mount_Rig" in a.name)
 reassigned["attempted"]=True
 arm.animation_data_create()
 arm.animation_data.action=action
 if action.slots:arm.animation_data.action_slot=action.slots[0]
 vals=[]
 for f in (1,7,13,19):
  bpy.context.scene.frame_set(f)
  vals.append(round(float(arm.pose.bones["FrontL_Upper"].rotation_euler.x),6))
 reassigned={"attempted":True,"pass":(max(vals)-min(vals)>.05),"angles":vals,
             "action":action.name,"slots":[s.identifier for s in action.slots]}
except Exception as exc:
 reassigned["error"]=repr(exc)
result={'blender':bpy.app.version_string,'source_rig_bones':bones_expected,
 'reimported_rigs':len(rigs),'reimported_bones':[len(x.data.bones) for x in rigs],
 'source_mesh_parts':meshes_expected,'reimported_mesh_objects':len(meshes),
 'source_shape_keys':keys_expected,'reimported_shape_keys':keys,
 'reimported_rigs_with_actions':animated,'all_actions':[a.name for a in bpy.data.actions],
 'import_armature_animation_data':[(o.name,str(o.animation_data),[(tr.name,len(tr.strips)) for tr in o.animation_data.nla_tracks] if o.animation_data else []) for o in rigs],
 'reassigned_action_motion_check':reassigned,
 'foot_tracks':tracks,
 'source_to_blender_reimport_bones_ok':any(len(r.data.bones)==bones_expected for r in rigs),
 'source_to_blender_reimport_mesh_count_ok':len(meshes)>=meshes_expected,
 'source_to_blender_reimport_morph_ok':any('NW_Facial_Expressive_Squeeze' in k for k in keys),
 'locomotion_quality_approved':False,'native_unreal_import_approved':False,
 'seconds':round(time.perf_counter()-start,3)}
with open(os.path.join(ROOT,'skeletal_reimport.json'),'w') as f:json.dump(result,f,indent=2)
print('NOWORLD_SKELETAL='+json.dumps({k:v for k,v in result.items() if k!='foot_tracks'}))
