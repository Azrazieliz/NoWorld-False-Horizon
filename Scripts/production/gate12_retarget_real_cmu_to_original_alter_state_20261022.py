"""Native UE 5.8 original Luminous Alter real CMU mocap technical retarget.
ONE clip x ONE approved original outfit skeleton PER PROCESS.
No synthetic gait, animation substitution, source overwrite or visual approval.
"""
import unreal,os,pathlib,json,hashlib,math,traceback
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
STATE=os.environ["G12_ALTER_MOCAP_STATE"]
LABEL=os.environ["G12_ALTER_MOCAP_ACTION"]
assert STATE in ("CLOTHED","UNDERWEAR","NUDE")
assert LABEL in ("Walk","Run","Jump")
DEST="/Game/NoWorld/Characters/Luminous/Alter/NativeMocapRealCMU"
NAME=f"AN_G12_Alter_{STATE}_RealCMU_{LABEL}"
TARGET=DEST+"/"+NAME
RECEIPT=ROOT/"Saved/Gate12Readiness"/f"Gate12_Alter_{STATE}_RealCMU_{LABEL}_20261022.json"
SOURCEFILE=ROOT/"Saved/Gate12Koikatsu/MotionV3"/f"AN_KK_V3CMU_{LABEL}_native_source_tracks.json"
SOURCE_PROOF=ROOT/"Saved/Gate12Koikatsu/MotionV3/native_mocap_v3_onepart_ue_authoring.json"
MAP={
"thigh_l":"Left-leg","calf_l":"Left-knee","Left ankle":"Left-ankle","ball_l":"Left-toe",
"thigh_r":"Right-leg","calf_r":"Right-knee","Right ankle":"Right-ankle","ball_r":"Right-toe",
"spine_01":"Spine","spine_02":"Chest","spine_03":"Upper-Chest",
"neck":"Neck","head":"Head",
"clavicle_l":"Left-shoulder","upperarm_l":"Left-arm","lowerarm_l":"Left-elbow","hand_l":"Left-wrist",
"clavicle_r":"Right-shoulder","upperarm_r":"Right-arm","lowerarm_r":"Right-elbow","hand_r":"Right-wrist"}
report={
 "schema":"noworld.gate12.luminous_alter.native_cmu_retarget.v1",
 "state":STATE,"motion":LABEL,"approved_character_source_version":"character:luminous_valentine.version_alter_v1",
 "status":"STARTED","real_mocap":True,"synthesized_animation":False,
 "source_assets_modified":False,"native_preview_only_until_visual_acceptance":True,
 "automatically_approved_for_gameplay":False,
 "retarget_method":"per-bone native rest-space quaternion delta onto original target rig",
 "mocap_visual_quality":"NOT_VERIFIED"}
def normalize(q):
 norm=math.sqrt(sum(float(v)*float(v) for v in q))
 assert norm>1e-9
 return tuple(float(v)/norm for v in q)
def mul(a,b):
 x,y,z,w=a;X,Y,Z,W=b
 return (w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,
         w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def inv(a):
 x,y,z,w=normalize(a);return (-x,-y,-z,w)
def quat(q):
 return tuple(float(getattr(q,axis)) for axis in ("x","y","z","w"))
try:
 assert ROOT.name in unreal.Paths.get_project_file_path()
 data=json.loads(SOURCEFILE.read_text(encoding="utf8"))
 proof=json.loads(SOURCE_PROOF.read_text(encoding="utf8"))
 assert proof["status"]=="PASS_REAL_CMU_SOURCE_TO_ONEPART_NATIVE_UE_SEQUENCE_3_CLIPS_PRE_VISUAL"
 assert data["status"]=="PASS_CALIBRATED_30FPS_REAL_MOCAP_QUATERNION_TRACKS_NOT_UE_ASSETS"
 assert data["source_GLTF_to_UE_quat_map"]=="(-x,-z,-y,w)"
 assert data["motion"]==LABEL and data["root_translation_tracks"]==0
 n=int(data["frames"]); assert n>=40 and len(data["sampled_frames"])==n
 assert data["clip_is_authorised_to_loop"] is False
 src_sk_path=proof["native_assets"][0]["skeleton"]
 src_sk=unreal.load_asset(src_sk_path)
 assert isinstance(src_sk,unreal.Skeleton),"Prior real source mocap reference skeleton missing"
 src_pose=src_sk.get_reference_pose()
 src_names=set(str(b) for b in src_pose.get_bone_names())
 target_sk=unreal.load_asset("/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal"+STATE.title()+"/SK_Luminous_Alter_"+STATE+"_OriginalPart_00_Skeleton")
 assert isinstance(target_sk,unreal.Skeleton),"Exact Alter state skeleton absent"
 target_pose=target_sk.get_reference_pose()
 target_names=set(str(b) for b in target_pose.get_bone_names())
 expected_bones=1123 if STATE=="NUDE" else 1131
 assert len(target_names)==expected_bones
 report["target_relevant_bone_names"]=[name for name in sorted(target_names) if any(k in name.lower() for k in ("left","right","spine","neck","head","hip","chest","pelvis"))][:140]
 report["source_relevant_bone_names"]=[name for name in sorted(src_names) if name in MAP]
 def native_source_bone(source):
  for candidate in (source,source.replace(" ","-"),source.replace(" ","_")):
   if candidate in src_names:return candidate
  return None
 chosen={source:target for source,target in MAP.items() if native_source_bone(source) and target in target_names and source in data["local_native_quaternion_tracks"]}
 # Real principal anatomical chains must match original user-approved target bone names.
 assert len(chosen)>=19,{"available":chosen,"missing":sorted(set(MAP)-set(chosen))}
 assert not unreal.EditorAssetLibrary.does_asset_exist(TARGET),"Never overwrite an existing native clip"
 report["source_mocap_file_sha256"]=hashlib.sha256(SOURCEFILE.read_bytes()).hexdigest()
 report["source_reference_skeleton"]=src_sk.get_path_name()
 report["target_original_skeleton"]=target_sk.get_path_name()
 report["target_bone_count"]=len(target_names)
 report["mapped_bone_count"]=len(chosen)
 unreal.EditorAssetLibrary.make_directory(DEST)
 factory=unreal.AnimSequenceFactory()
 factory.set_editor_property("target_skeleton",target_sk)
 anim=unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME,DEST,unreal.AnimSequence,factory)
 assert isinstance(anim,unreal.AnimSequence)
 controller=anim.get_editor_property("controller")
 controller.set_frame_rate(unreal.FrameRate(numerator=30,denominator=1),should_transact=False)
 controller.set_number_of_frames(unreal.FrameNumber(value=n-1),should_transact=False)
 done=[]
 controller.open_bracket(f"CMU original source calibrated Alter {STATE} {LABEL}",False)
 try:
  for source,target in chosen.items():
   source_tf=src_pose.get_bone_pose(unreal.Name(native_source_bone(source)),unreal.AnimPoseSpaces.LOCAL)
   target_tf=target_pose.get_bone_pose(unreal.Name(target),unreal.AnimPoseSpaces.LOCAL)
   original=normalize(quat(source_tf.rotation))
   desired=normalize(quat(target_tf.rotation))
   pos=unreal.Vector(target_tf.translation.x,target_tf.translation.y,target_tf.translation.z)
   scl=unreal.Vector(target_tf.scale3d.x,target_tf.scale3d.y,target_tf.scale3d.z)
   src_quats=data["local_native_quaternion_tracks"][source]
   assert len(src_quats)==n
   orientations=[]
   for frame in src_quats:
    # Delta is preserved from the real source, rotated onto the ORIGINAL
    # approved Alter state rest orientation; no generated sine/keyframe gait.
    delta=mul(inv(original),normalize(frame))
    result=normalize(mul(desired,delta))
    orientations.append(unreal.Quat(*result))
   assert bool(controller.add_bone_track(unreal.Name(target),False))
   assert bool(controller.set_bone_track_keys(unreal.Name(target),
       [pos]*n,orientations,[scl]*n,False))
   done.append({"from":source,"to":target})
 finally:
  controller.close_bracket(False)
 assert len(done)>=19
 assert abs(anim.get_play_length()-(n-1)/30)<0.07
 assert unreal.EditorAssetLibrary.save_loaded_asset(anim)
 report["native_animsequence"]=anim.get_path_name()
 report["mapped_bones"]=done
 report["frames"]=n
 report["duration_s"]=anim.get_play_length()
 report["status"]="IMPORTED_GENUINE_CMU_MOCAP_ON_EXACT_ALTER_RIG_PENDING_VISUAL"
except Exception:
 report["status"]="RETARGET_BLOCKED_OR_INCOMPLETE_NO_FALSE_APPROVAL"
 report["error"]=traceback.format_exc()[-4700:]
finally:
 RECEIPT.parent.mkdir(parents=True,exist_ok=True)
 RECEIPT.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
 unreal.log("G12_ALTER_CMU_"+STATE+"_"+LABEL+"="+report["status"])
