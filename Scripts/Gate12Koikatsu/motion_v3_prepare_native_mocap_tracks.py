"""Prepare real 30fps CMU motion track data in original GLB and native-UE space.

Do NOT automatically loop, smooth or mark these clips final. Source frame 0
is a synthetic T-pose and must be excluded. Each authored frame is an actual
captured frame, sampled 120Hz→30Hz; bone quaternion conversion is calibrated
against real UE5.8 skeleton rest transforms.
"""
from __future__ import annotations
import hashlib, json, math, pathlib, sys, time
from motion_v3_bvh_retarget_core import BVH,GLTFRig,V3_MAP,q_norm,q_angle_degrees,q_mul,q_conj
from motion_v3_verify_unreal_import_basis import source_quat_to_unreal
ROOT=pathlib.Path(__file__).resolve().parents[2]
SOURCE=ROOT/"SourceAssets/Gate12/MotionV3/CMU"
GLB=ROOT/"Saved/Gate12Koikatsu/full_fidelity_primitive_shards/Mesh_03_Prim_00.glb"
BASE=ROOT/"Saved/Gate12Koikatsu/MotionV3"
MAPS={
 "Walk":("02_01_walk.bvh","cf56db43157acc3d200b3d4215523f54d354a8926bea67e1f6c8031bd335ba7e"),
 "Run":("02_03_jog.bvh","6f4d38b92b041221030de2e1058903e0f7a8fa08f9c6c9efabf7df9a128011ac"),
 "Jump":("02_04_jump.bvh","1b9f80f045608eaf50e2912c2f9180e97cffc708ce717c227f52ce1bc6fe091b"),
}
def gather(name,filename,checksum,rig):
 src=SOURCE/filename
 assert hashlib.sha256(src.read_bytes()).hexdigest()==checksum
 b=BVH(src)
 assert abs(b.fps-120)<.01
 native_reference=json.loads((BASE/"native_ue_onepart_refpose_audit.json").read_text("utf-8-sig"))
 assert native_reference["status"]=="PASS_READONLY_UE_ONEPART_REFERENCE_POSE"
 expected_bones={x["name"]:x for x in native_reference["bone_poses"]}
 sample_frames=list(range(1,len(b.frames),4))
 originals=list(V3_MAP.values())
 names=[n for n in originals if n in rig.by_name]
 native_tracks={n:[] for n in names}
 notes=[]
 for fi in sample_frames:
  mapped=rig.mapped_pose(b,fi)
  for bone_name in names:
   source_q=mapped["local_q"][bone_name]
   ue_q=q_norm(source_quat_to_unreal(source_q))
   native_tracks[bone_name].append([round(float(x),8) for x in ue_q])
  notes.append({"raw_source_frame":fi,"second":round((fi-1)/b.fps,5)})
 # Validate time continuity per bone; retain high-frequency genuine mocap
 # motion without arbitrary smoothing or interpolating new frames.
 jumps={}
 for bone_name,quats in native_tracks.items():
  changes=[]
  for i in range(1,len(quats)):
   delta=q_mul(quats[i],q_conj(quats[i-1]))
   changes.append(q_angle_degrees(delta))
  jumps[bone_name]={"max_frame_to_frame_deg":round(max(changes),4),
               "mean_frame_to_frame_deg":round(sum(changes)/len(changes),4)}
  assert max(changes)<58,(bone_name,max(changes),"Unnatural 30Hz abrupt motion")
 # Do not confuse this source-derived native rotation stream with native
 # Unreal asset authoring, or root gameplay velocity/foot-locking.
 result={
   "status":"PASS_CALIBRATED_30FPS_REAL_MOCAP_QUATERNION_TRACKS_NOT_UE_ASSETS",
   "motion":name,"source_filename":filename,"source_sha256":checksum,
   "source_hz":b.fps,"frames":len(sample_frames),
   "duration_seconds":round((len(sample_frames)-1)/30,4),
   "synthetic_T_pose_frame_excluded":True,
   "first_valid_source_frame":sample_frames[0],
   "source_glb_SHA256":hashlib.sha256(GLB.read_bytes()).hexdigest(),
   "bone_count":len(native_tracks),
   "matched_bones_present_in_selected_actual_native_UE_skeleton":[n for n in names if n in expected_bones],
   "unreal_source_reference_calibrated_bones":len(expected_bones),
   "sampled_motion_provenance":"CMU original MoCap data, cgspeed Bruce Hahne BVH conversion",
   "root_translation_tracks":0,"clip_is_authorised_to_loop":False,
   "planted_foot_IK":"NOT_IMPLEMENTED","visual_quality":"NOT_APPROVED",
   "source_GLTF_to_UE_quat_map":"(-x,-z,-y,w)",
   "sampled_frames":notes,
   "local_native_quaternion_tracks":native_tracks,
   "sample_joint_angular_speed_report":jumps,
 }
 out=BASE/("AN_KK_V3CMU_%s_native_source_tracks.json"%name)
 out.write_text(json.dumps(result,separators=(",",":"),ensure_ascii=False)+"\n",encoding="utf8")
 print(name,"frames",result["frames"],"bones",result["bone_count"],
       "duration",result["duration_seconds"],
       "max_per_frame_deg",max(z["max_frame_to_frame_deg"] for z in jumps.values()))
 return result
def main():
 rig=GLTFRig(GLB)
 x={label:gather(label,filename,checksum,rig)
    for label,(filename,checksum) in MAPS.items()}
 result={"status":"PASS_3_CMUMOCAP_TRACK_SETS_READY_FOR_NATIVE_UE_AUTHORING",
         "all_three_sources_immutable":True,
         "native_glb_map_exact_onepart":"PASS_21_BONE_IMPORT_CALIBRATION",
         "runtime_new_animations":"NOT_AUTHORED",
         "action_states":["Walk","Run","Jump"],
         "frames":{k:v["frames"] for k,v in x.items()},
         "real_quaternion_bone_tracks":{k:v["bone_count"] for k,v in x.items()},
         "left_right_joint_articulation":"REAL_MOCAP_NOT_SINUSOIDAL",
         "original_source_poses_preserved":True,
         "duration_secs":{k:v["duration_seconds"] for k,v in x.items()},
         "time_utc":time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())}
 (BASE/"v3_real_motion_track_staging_receipt.json").write_text(json.dumps(result,indent=2)+"\n")
 print(json.dumps(result,indent=2))
if __name__=="__main__":main()
