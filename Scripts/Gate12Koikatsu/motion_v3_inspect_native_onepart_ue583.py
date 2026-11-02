"""Read-only UE5.8 one-skeleton reference pose calibration for CMU→KK retarget."""
import json, os, pathlib, sys, time, traceback
import unreal
ROOT=pathlib.Path(os.environ.get("G12_GATE12_WORKTREE",r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010"))
PATH=ROOT/"Saved/Gate12Koikatsu/MotionV3/native_ue_onepart_refpose_audit.json"
REFERENCE=ROOT/"Saved/Gate12Koikatsu/playable_blueprint_authoring_report.json"
NAMES=("Center","pelvis","cf_j_waist02","spine_01","spine_02","spine_03",
       "thigh_l","calf_l","Left ankle","ball_l","thigh_r","calf_r",
       "Right ankle","ball_r","clavicle_l","upperarm_l","lowerarm_l",
       "hand_l","clavicle_r","upperarm_r","lowerarm_r","hand_r","neck","head")
result={"status":"START","date_utc":time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime()),
        "read_only":True,"saved_assets_modified":False,"bone_poses":[]}
try:
    assert "OfflineGame_Gate12_Koikatsu_20261010" in str(unreal.Paths.get_project_file_path())
    report=json.loads(REFERENCE.read_text(encoding="utf-8-sig"))
    assert report["status"]=="PASS_BLUEPRINT_ASSEMBLED_PENDING_COOK_AND_ANDROID"
    part=next(q for q in report["parts"] if q["component"]=="KK_Full_M03P00")
    mesh=unreal.load_asset(part["mesh"])
    assert isinstance(mesh,unreal.SkeletalMesh)
    skeleton=mesh.get_editor_property("skeleton")
    assert skeleton.get_path_name()==part["skeleton"]
    pose=skeleton.get_reference_pose()
    names=set(str(x) for x in pose.get_bone_names())
    result["part"]="KK_Full_M03P00"
    result["skeleton"]=skeleton.get_path_name()
    result["bone_count"]=len(names)
    for name in NAMES:
        if name not in names:continue
        tf=pose.get_bone_pose(unreal.Name(name),unreal.AnimPoseSpaces.LOCAL)
        result["bone_poses"].append({
            "name":name,"translation":[tf.translation.x,tf.translation.y,tf.translation.z],
            "rotation":[tf.rotation.x,tf.rotation.y,tf.rotation.z,tf.rotation.w],
            "scale":[tf.scale3d.x,tf.scale3d.y,tf.scale3d.z]})
    assert len(result["bone_poses"])>=20
    result["status"]="PASS_READONLY_UE_ONEPART_REFERENCE_POSE"
except Exception as exc:
    result["status"]="FAIL_READONLY_UE_ONEPART_REFERENCE_POSE"
    result["error"]=str(exc)
    result["traceback"]=traceback.format_exc()[-2700:]
finally:
    PATH.parent.mkdir(parents=True,exist_ok=True)
    PATH.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    unreal.log("GATE12_MOTION_V3_ONEPART_NATIVE_RIG="+result["status"])
