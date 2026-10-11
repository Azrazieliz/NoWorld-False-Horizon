"""Real Luminous 975-bone UE5.8 native AnimSequences authored from CMU data.
Author three native clips for one wardrobe in a bounded isolated call.
NO procedural V1/V2. No UAsset or source overwrite. 30fps, 22 actual limb tracks.
Source glTF->Unreal bone basis was measured directly on 975 native skeleton.
"""
import unreal,pathlib,json,os,time,traceback,hashlib
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
STATE=os.environ.get("G12_LUMINOUS_NATIVE_ANIM_WARDROBE","CLOTHED").upper()
if STATE not in ("CLOTHED","UNDERWEAR","NUDE"):raise RuntimeError("Unexpected wardrobe state")
OUTPUT="/Game/Experimental/LuminousValentineV3/NativeMocapV3/"+STATE
J=Q/"MotionV3"/("luminous_%s_native_real_cmu_three_clip_ue583.json"%STATE)
r={"status":"START","wardrobe":STATE,"source":"CMU_2010_REAL_MOCAP","native_tracks":22,"native_bones":975,
   "native_sequences":[],"visual_approval":False,"android_approval":False,
   "imported_source_modified":False}
def save():J.write_text(json.dumps(r,indent=2,ensure_ascii=False)+"\n","utf8")
save()
try:
 manifest=json.loads((Q/"MotionV3/luminous_v3_all_9_real_cmu_source_tracks_manifest.json").read_text("utf8"))
 if manifest["status"]!="PASS_9_SOURCE_RETARGET_SETS_STAGED_NOT_UE_ANIM_ASSETS":
  raise RuntimeError("Missing source-authentic motion provenance")
 if "OfflineGame_Gate12_Koikatsu_20261010" not in str(unreal.Paths.get_project_file_path()):
  raise RuntimeError("Wrong Unreal project")
 cal=json.loads((Q/"luminous_975_ue_native_refpose_calibration_v3.json").read_text("utf8"))
 if cal["status"]!="PASS_MOTION_V3_NATIVE_975_QUAT_BASIS" or cal["max_error_deg"]>.15:raise RuntimeError("Native 975 basis not calibrated")
 d=Q/STATE
 imports=list(d.glob("native_shards_*_ue583.json"))
 primary=None
 for file in imports:
  entries=json.loads(file.read_text("utf-8-sig"))["native_parts"]
  primary=next((x for x in entries if x["index"]==3),primary)
 if not primary:raise RuntimeError("Missing body primary native import receipt")
 mesh=next(x["path"] for x in primary["assets"] if x["class"]=="SkeletalMesh")
 asset=unreal.load_asset(mesh)
 if not isinstance(asset,unreal.SkeletalMesh):raise RuntimeError("Imported 975 skeletal mesh absent")
 sk=asset.get_editor_property("skeleton")
 pose=sk.get_reference_pose()
 bones=set(str(x) for x in pose.get_bone_names())
 if len(bones)!=975:raise RuntimeError("Native 975-joint count wrong")
 r["mesh"]=mesh;r["skeleton"]=sk.get_path_name()
 for motion in ("Walk","Run","Jump"):
  inputfile=Q/"MotionV3"/("LV3_%s_RealCMU_%s_975_NativeTracks.json"%(STATE,motion))
  receipt=next(x for x in manifest["motion_sets"] if x["state"]==STATE and x["motion"]==motion)
  if hashlib.sha256(inputfile.read_bytes()).hexdigest()!=receipt["sha256"]:
   raise RuntimeError("Source retarget 975 track file hash drift")
  data=json.loads(inputfile.read_text("utf8"))
  if data["status"]!="PASS_SOURCE_AUTHENTIC_LUMINOUS_975_MOCAP_TRACKS_READY_FOR_NATIVE_UNREAL" or data["root_translation_tracks"]!=0:raise RuntimeError("Disallowed root translation")
  tracks=data["skeletal_tracks"]
  if len(tracks)!=22 or not set(tracks).issubset(bones):raise RuntimeError("Native 22-bone mapping mismatch "+str(set(tracks)-bones))
  n=int(data["frames"])
  if n<40 or any(len(x)!=n for x in tracks.values()):raise RuntimeError("Malformed genuine source frames")
  name="AN_LuminousV3_RealCMU_%s_%s_975"%(motion,STATE)
  destination=OUTPUT+"/"+name
  if unreal.EditorAssetLibrary.does_asset_exist(destination):
   raise RuntimeError("NEVER overwrite previous authentic native animation "+destination)
  factory=unreal.AnimSequenceFactory()
  factory.set_editor_property("target_skeleton",sk)
  anim=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,OUTPUT,unreal.AnimSequence,factory)
  if not isinstance(anim,unreal.AnimSequence) or anim.get_editor_property("skeleton")!=sk:raise RuntimeError("Animation skeleton factory mismatch")
  ctrl=anim.get_editor_property("controller")
  ctrl.set_frame_rate(unreal.FrameRate(numerator=30,denominator=1),should_transact=False)
  ctrl.set_number_of_frames(unreal.FrameNumber(value=n-1),should_transact=False)
  ctrl.open_bracket("Luminous genuine 30FPS CMU "+motion+" source retarget",False)
  authored=[]
  try:
   for name_bone,frames in tracks.items():
    tf=pose.get_bone_pose(unreal.Name(name_bone),unreal.AnimPoseSpaces.LOCAL)
    p=unreal.Vector(tf.translation.x,tf.translation.y,tf.translation.z)
    scale=unreal.Vector(tf.scale3d.x,tf.scale3d.y,tf.scale3d.z)
    if not ctrl.add_bone_track(unreal.Name(name_bone),False):raise RuntimeError("Failed to bind exact native 975 bone "+name_bone)
    rot=[unreal.Quat(*map(float,q)) for q in frames]
    if not ctrl.set_bone_track_keys(unreal.Name(name_bone),[p]*n,rot,[scale]*n,False):
     raise RuntimeError("Could not write authentic CMU bone keys "+name_bone)
    authored.append(name_bone)
  finally:ctrl.close_bracket(False)
  actual=set(str(x).casefold() for x in ctrl.get_model_interface().get_bone_track_names())
  expected_names={x.casefold() for x in authored}
  r["native_track_debug"]={"motion":motion,"expected":authored,"actual_normalized":sorted(actual),
   "missing":sorted(expected_names-actual),"unexpected":sorted(actual-expected_names)}
  save()
  if actual!=expected_names:
   raise RuntimeError("Unexpected track loss in native AnimSequence: "+repr(r["native_track_debug"]))
  duration=anim.get_play_length()
  if abs(duration-(n-1)/30)>0.08:raise RuntimeError("Incorrect native clip timing")
  if not unreal.EditorAssetLibrary.save_loaded_asset(anim):raise RuntimeError("Native animation save failed")
  r["native_sequences"].append({"motion":motion,"native_animation":anim.get_path_name(),
     "exact_skeleton":sk.get_path_name(),"frames":n,"native_tracks":len(authored),"duration_sec":duration,
     "source_file_sha256":receipt["sha256"],"root_translation":0,
     "genuine_source_mocap":True,"visual_motion_approval":False,
     "loop_policy":"AWAITING_VISUAL_QA","s26_thermal":"NOT_TESTED"})
  save()
 r["status"]="PASS_3_NATIVE_LUMINOUS_CMUMOCAP_CLIPS_PRE_VISUAL" if len(r["native_sequences"])==3 else "FAIL_INCOMPLETE"
except Exception as e:
 r["status"]="FAIL_OR_PARTIAL_NATIVE_ANIM_AUTHORING"
 r["exception"]=str(e);r["trace"]=traceback.format_exc()[-5000:]
finally:
 r["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime());save()
 unreal.log("G12_LUMINOUS_REAL_CMU_975_"+STATE+"="+r["status"])
