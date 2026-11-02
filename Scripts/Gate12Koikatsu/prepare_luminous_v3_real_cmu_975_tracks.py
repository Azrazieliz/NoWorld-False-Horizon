"""Source-authentic CMU 2010 motion-to-Luminous 975-joint BVH retarget.
Uses glTF source world-rest reference and exact UE quaternion basis verified
against the real native 975-joint UE5.8 skeleton. NEVER changes original sources.
Creates one verified 30FPS track JSON for each source motion and wardrobe state.
"""
from pathlib import Path
import sys,hashlib,json,math,time
W=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
sys.path.insert(0,str(W/"Scripts/Gate12Koikatsu"))
import motion_v3_bvh_retarget_core as rt
from motion_v3_verify_unreal_import_basis import source_quat_to_unreal
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
SRC=W/"SourceAssets/Gate12/MotionV3/CMU"
MAP={
"Hips":"Pelvis","LeftUpLeg":"Left leg","LeftLeg":"Left knee","LeftFoot":"Left ankle","LeftToeBase":"Left toe",
"RightUpLeg":"Right leg","RightLeg":"Right knee","RightFoot":"Right ankle","RightToeBase":"Right toe",
"LowerBack":"Spine","Spine":"Chest","Spine1":"Upper Chest","Neck":"Neck","Head":"Head",
"LeftShoulder":"Left shoulder","LeftArm":"Left arm","LeftForeArm":"Left elbow","LeftHand":"Left wrist",
"RightShoulder":"Right shoulder","RightArm":"Right arm","RightForeArm":"Right elbow","RightHand":"Right wrist",
}
CMU={"Walk":("02_01_walk.bvh","cf56db43157acc3d200b3d4215523f54d354a8926bea67e1f6c8031bd335ba7e"),
 "Run":("02_03_jog.bvh","6f4d38b92b041221030de2e1058903e0f7a8fa08f9c6c9efabf7df9a128011ac"),
 "Jump":("02_04_jump.bvh","1b9f80f045608eaf50e2912c2f9180e97cffc708ce717c227f52ce1bc6fe091b")}
ref=json.loads((Q/"luminous_975_ue_native_refpose_calibration_v3.json").read_text("utf-8-sig"))
if ref["status"]!="PASS_MOTION_V3_NATIVE_975_QUAT_BASIS" or ref["max_error_deg"]>.15:
 raise RuntimeError("Actual native skeleton rotation basis not approved")
rt.V3_MAP=MAP
out=Q/"MotionV3"
out.mkdir(exist_ok=True)
r={"status":"START","source":"CMU 2010 genuine mocap BVH","authoritative_native_975_refpose_probe":str(Q/"luminous_975_ue_native_refpose_calibration_v3.json"),
 "rig_map":MAP,"motion_sets":[],"visual_approval":False,"phone_approval":False}
for state in ("CLOTHED","UNDERWEAR","NUDE"):
 manifest=json.loads((Q/("LosslessSources/LuminousV3"+state+"/shards/manifest_full_fidelity_primitives.json")).read_text("utf-8-sig"))
 row=manifest["shards"][3]
 glb=Path(row["path"])
 assert hashlib.sha256(glb.read_bytes()).hexdigest()==row["sha256"]
 rig=rt.GLTFRig(glb)
 assert len(set(MAP.values())&rig.names())==len(MAP),"Missing mapped anatomical target"
 for motion,(filename,sha) in CMU.items():
  file=SRC/filename
  assert hashlib.sha256(file.read_bytes()).hexdigest()==sha,(motion,file)
  bvh=rt.BVH(file)
  assert abs(bvh.fps-120)<.01
  samples=range(1,len(bvh.frames),4)
  tracks={native.replace(" ","-"):[] for native in MAP.values()}
  previous={}
  biggest=0.
  # Retarget authentic BVH world-space orientation change relative to source
  # rest onto independently verified Luminous glTF actual 975-joint rest.
  for frame in samples:
   pose=rig.mapped_pose(bvh,frame)
   for native in MAP.values():
    name=native.replace(" ","-")
    q=rt.q_norm(source_quat_to_unreal(pose["local_q"][native]))
    assert abs(sum(x*x for x in q)-1)<1e-5
    if name in previous:
     delta=rt.q_mul(q,rt.q_conj(previous[name]))
     ang=rt.q_angle_degrees(delta)
     biggest=max(biggest,ang)
     if ang>=58:raise RuntimeError("Abrupt source motion frame "+motion+"/"+state+"/"+native+"/"+str(ang))
    previous[name]=q
    tracks[name].append([round(x,9) for x in q])
  if len(tracks)!=22 or len(next(iter(tracks.values())))<40:raise RuntimeError("Missing full articulated mocap")
  path=out/("LV3_%s_RealCMU_%s_975_NativeTracks.json"%(state,motion))
  if path.exists():raise RuntimeError("Existing generated motion source; no overwrite "+str(path))
  payload={"status":"PASS_SOURCE_AUTHENTIC_LUMINOUS_975_MOCAP_TRACKS_READY_FOR_NATIVE_UNREAL",
    "wardrobe":state,"motion":motion,"source_bvh":str(file),"source_bvh_sha256":sha,
    "source_glb_sha256":row["sha256"],"root_translation_tracks":0,"source_sample_rate_hz":120,
    "target_sample_rate_hz":30,"synthetic_rest_frame_excluded":True,
    "native_rig_bones":975,"frames":len(tracks[next(iter(tracks))]),
    "max_30fps_joint_angular_change_deg":biggest,"skeletal_tracks":tracks,
    "animation_looping_visual_approved":False,"animation_retarged_native_UE":False,
    "directional_blendspace":"PENDING","foot_ik":"PENDING","user_visual_approval":False}
  path.write_text(json.dumps(payload,separators=(",",":"),ensure_ascii=False)+"\n","utf8")
  r["motion_sets"].append({"state":state,"motion":motion,"tracks":22,"frames":payload["frames"],
    "max_joint_change_deg":round(biggest,4),"json_path":str(path),
    "sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
  print("SOURCE_RETARGETED",state,motion,"FRAMES",payload["frames"],"JOINTS",22,"FRAME_DELTA_MAX_DEG",round(biggest,3))
r["status"]="PASS_9_SOURCE_RETARGET_SETS_STAGED_NOT_UE_ANIM_ASSETS" if len(r["motion_sets"])==9 else "FAIL"
r["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
(out/"luminous_v3_all_9_real_cmu_source_tracks_manifest.json").write_text(json.dumps(r,indent=2)+"\n","utf8")
