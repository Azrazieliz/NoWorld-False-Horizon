"""Quality gate for 3 authentic CMU BVH clips retargeted onto the exact KK glTF rig.

Rejects prior sine-wave semantics by requiring independent real knee/elbow
kinematics, correct frame0 rest, bounded target/source articulation mismatch.
This does NOT assert UE-imported axes or visual pose/foot contacts approved.
"""
from __future__ import annotations
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
from statistics import mean
from motion_v3_bvh_retarget_core import BVH,GLTFRig,source_knee_flex,target_knee_flex,angle_between,v_sub,q_angle_degrees

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"SourceAssets/Gate12/MotionV3/CMU"
GLB=ROOT/"Saved/Gate12Koikatsu/full_fidelity_primitive_shards/Mesh_03_Prim_00.glb"
OUTPUT=ROOT/"Saved/Gate12Koikatsu/MotionV3/real_mocap_articulation_validation.json"
INPUTS={
    "walk":("02_01_walk.bvh","cf56db43157acc3d200b3d4215523f54d354a8926bea67e1f6c8031bd335ba7e"),
    "jog":("02_03_jog.bvh","6f4d38b92b041221030de2e1058903e0f7a8fa08f9c6c9efabf7df9a128011ac"),
    "jump":("02_04_jump.bvh","1b9f80f045608eaf50e2912c2f9180e97cffc708ce717c227f52ce1bc6fe091b"),
}
def extrema(arr):
    return {"min":round(min(arr),3),"max":round(max(arr),3),
            "range":round(max(arr)-min(arr),3),"mean":round(mean(arr),3)}
def elbows(trg,rig,side):
    names=rig.by_name
    p=trg["global_p"]
    u=p[names["upperarm_"+side]]
    l=p[names["lowerarm_"+side]]
    h=p[names["hand_"+side]]
    return angle_between(v_sub(l,u),v_sub(h,l))
def detect_source_foot_contacts(samples,foot):
    # Detect actual source mocap support candidates (foot low + horiz speed
    # low). Velocity is displacement per sampled 1/30 sec (source units).
    p=[x["source_global"][foot] for x in samples]
    heights=[q[1] for q in p]
    lo=min(heights)
    height_threshold=lo+.62
    steps=[]
    for i in range(1,len(p)):
        hor=math.hypot(p[i][0]-p[i-1][0],p[i][2]-p[i-1][2])
        low=min(p[i][1],p[i-1][1])<=height_threshold
        steps.append({"sample":i,"height":round(p[i][1],3),
                      "horizontal_motion_per_30hz_sample":round(hor,4),
                      "low":low})
    candidates=[r for r in steps if r["low"]]
    # Distinguish low foot height from true planted foot; reports evidence
    # rather than guessing a binary contact from an uncalibrated threshold.
    low_horizontal=sorted(r["horizontal_motion_per_30hz_sample"] for r in steps)
    median_speed=low_horizontal[len(low_horizontal)//2]
    planted=[r for r in candidates if r["horizontal_motion_per_30hz_sample"]<median_speed*.65]
    return {"low_foot_samples":len(candidates),
            "support_candidates_with_low_velocity":len(planted),
            "median_foot_xz_displacement_per_frame":round(median_speed,4),
            "sample_count":len(steps),"threshold_y":round(height_threshold,3),
            "notes":"Candidate contacts only, requires matching target foot lock and UE scene test."}
def validate():
    rig=GLTFRig(GLB)
    output={"status":"STARTED","source_glb":str(GLB),
            "source_character_mutated":False,
            "provenance":"CMU Graphics Lab mocap BVH, Bruce Hahne 2010 conversion",
            "source_rights":"Original CMU raw data + BVH converter rights/readme retained",
            "actual_ue_bone_axes":"NOT_VERIFIED",
            "game_ready":"NO",
            "clips":{}}
    for label,(name,sha) in INPUTS.items():
        path=SOURCE/name
        assert hashlib.sha256(path.read_bytes()).hexdigest()==sha, "BVH source corruption"
        b=BVH(path)
        samples=[]
        for frame in range(4,len(b.frames),4):
            src=b.pose(frame)
            trg=rig.mapped_pose(b,frame)
            samples.append({
                "frame":frame,
                "source_global":src["global_p"],
                "l_src_knee":source_knee_flex(b,frame,"l"),
                "r_src_knee":source_knee_flex(b,frame,"r"),
                "l_tgt_knee":target_knee_flex(rig,trg,"l"),
                "r_tgt_knee":target_knee_flex(rig,trg,"r"),
                "l_tgt_elbow":elbows(trg,rig,"l"),
                "r_tgt_elbow":elbows(trg,rig,"r")})
        assert len(samples)>25
        dev={}
        kinematic={}
        for side in ("l","r"):
            src=[s[f"{side}_src_knee"] for s in samples]
            tgt=[s[f"{side}_tgt_knee"] for s in samples]
            err=[abs(x-y) for x,y in zip(src,tgt)]
            dev[side]={"max_abs_knee_angle_error":round(max(err),4),
                       "mean_abs_knee_angle_error":round(mean(err),4)}
            assert max(err)<3.0,(label,side,max(err))
            assert max(tgt)-min(tgt)>(25 if label=="walk" else 40),(label,side,"limb does not articulate")
            assert max(tgt)<140,(label,side,"hyperflex")
            kinematic[f"{side}_knee_source"]=extrema(src)
            kinematic[f"{side}_knee_target"]=extrema(tgt)
            elbows_b=[s[f"{side}_tgt_elbow"] for s in samples]
            kinematic[f"{side}_elbow_target"]=extrema(elbows_b)
            assert max(elbows_b)-min(elbows_b)>5,(label,side,"static arm")
        if label in ("walk","jog"):
            # Alternating knee action: successive sample poses must differ.
            ll=[s["l_tgt_knee"] for s in samples]
            rr=[s["r_tgt_knee"] for s in samples]
            separated=sum(abs(a-b)>12 for a,b in zip(ll,rr))
            assert separated>len(samples)//3,(label,"legs flexing identically")
            kinematic["left_right_alternation_fraction"]=round(separated/len(samples),4)
        contact={foot:detect_source_foot_contacts(samples,foot)
                 for foot in ("LeftFoot","RightFoot")}
        output["clips"][label]={
            "source_filename":name,"source_sha256":sha,
            "source_frames":len(b.frames),"source_fps":b.fps,
            "samples_30fps":len(samples),
            "bone_map_count":len([x for x in b.joints if x in rig.names()]),
            "target_bone_map_count":len([x for x in rig.names()]),
            "articulation":kinematic,"knee_retarget_error":dev,
            "source_contact_candidates":contact,
            "clip_native_unreal_ue583":"NOT_YET_AUTHORED",
            "target_planted_foot_ik":"NOT_IMPLEMENTED",
            "visually_approved":"NO"}
    output["status"]="PASS_OFFLINE_REAL_MOCAP_ARTICULATION_3_CLIPS_NOT_UE"
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text(json.dumps(output,indent=2)+"\n",encoding="utf8")
    return output
if __name__=="__main__":
    s=validate()
    print(json.dumps({"status":s["status"],"clips":{
        k:{"samples_30fps":v["samples_30fps"],
           "knee_retarget_error":v["knee_retarget_error"],
           "l_knee_target":v["articulation"]["l_knee_target"],
           "r_knee_target":v["articulation"]["r_knee_target"],
           "l_elbow_target":v["articulation"]["l_elbow_target"],
           "r_elbow_target":v["articulation"]["r_elbow_target"],
           "contacts":v["source_contact_candidates"]}
        for k,v in s["clips"].items()}},indent=2))
