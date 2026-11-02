"""Post-freeze: author 3 REAL CMU mocap retargeted native UE clips on ONE KK skeleton.

Source motion: real CMU 2010 converted BVH; not sine-wave procedural clips.
Requires source-vs-Unreal calibrated rest-space quaternions, on-disk source
SHA receipts and exact imported per-part UE skeleton. NO V1/V2 overwrite.
No gameplay playback authorization; preview/native data only.
"""
import json, os, pathlib, sys, traceback, time
import unreal
ROOT=pathlib.Path(os.environ.get("G12_GATE12_WORKTREE",r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010"))
BASE=ROOT/"Saved/Gate12Koikatsu/MotionV3"
REF=ROOT/"Saved/Gate12Koikatsu/playable_blueprint_authoring_report.json"
OUT="/Game/Experimental/Gate12Koikatsu/Animation/NativeMocapV3"
RECEIPT=BASE/"native_mocap_v3_onepart_ue_authoring.json"
PART="KK_Full_M03P00"
LABELS=("Walk","Run","Jump")
status={"status":"START","source":"REAL_CMU_MOCAP_BVH","part":PART,
        "native_assets":[],"changed_existing_assets":False,
        "user_visual_approval":False,"s26_runtime_approval":False}
def save():
    RECEIPT.parent.mkdir(parents=True,exist_ok=True)
    RECEIPT.write_text(json.dumps(status,indent=2,default=str)+"\n",encoding="utf-8")
def require(cond,msg):
    if not cond:raise RuntimeError(msg)
def keys_as_quats(track):
    vals=[]
    for seq in track:
        require(len(seq)==4,"Malformed source quaternion")
        vals.append(unreal.Quat(float(seq[0]),float(seq[1]),float(seq[2]),float(seq[3])))
    return vals
save()
try:
    require("OfflineGame_Gate12_Koikatsu_20261010" in str(unreal.Paths.get_project_file_path()),
            "Wrong UE project")
    check=json.loads((BASE/"verified_native_ue_glb_axis_transform.json").read_text("utf-8-sig"))
    require(check["status"]=="PASS_EXACT_UE_NATIVE_ONEPART_GLB_TRANSFORM" and
            check["tested_bones"]>=20,"Source/Unreal basis not fully calibrated")
    require(check["max_translation_error_cm"]<.03 and
            check["max_quaternion_angular_error_degrees"]<.15,
            "Source native skeleton rest orientation does not match GLB")
    report=json.loads(REF.read_text(encoding="utf-8-sig"))
    part=next(x for x in report["parts"] if x["component"]==PART)
    mesh=unreal.load_asset(part["mesh"])
    require(isinstance(mesh,unreal.SkeletalMesh),"Native skeletal mesh unavailable")
    sk=mesh.get_editor_property("skeleton")
    require(sk.get_path_name()==part["skeleton"],"Wrong saved skeleton")
    pose=sk.get_reference_pose()
    bones=set(str(v) for v in pose.get_bone_names())
    status["native_skeleton"]=sk.get_path_name()
    status["native_bone_count"]=len(bones)
    require(len(bones)>=20,"Selected skeleton is incomplete")
    for label in LABELS:
        src=BASE/("AN_KK_V3CMU_%s_native_source_tracks.json"%label)
        data=json.loads(src.read_text("utf-8-sig"))
        require(data["status"]=="PASS_CALIBRATED_30FPS_REAL_MOCAP_QUATERNION_TRACKS_NOT_UE_ASSETS",
                "Real mocap track receipt failed")
        require(data["motion"]==label and data["root_translation_tracks"]==0,"Root or motion mismatch")
        require(data["source_GLTF_to_UE_quat_map"]=="(-x,-z,-y,w)","Conversion basis mismatch")
        n=int(data["frames"])
        require(n>40 and len(data["sampled_frames"])==n,"Corrupt source mocap frames")
        name="AN_KK_V3CMU_%s_M03_P00"%label
        dest=OUT+"/"+name
        require(not unreal.EditorAssetLibrary.does_asset_exist(dest),
                "Fail closed: never overwrite real source-mocap native asset "+dest)
        factory=unreal.AnimSequenceFactory()
        factory.set_editor_property("target_skeleton",sk)
        anim=unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,OUT,unreal.AnimSequence,factory)
        require(isinstance(anim,unreal.AnimSequence),"Failed genuine native sequence asset")
        require(anim.get_editor_property("skeleton")==sk,"Cannot bind foreign skeleton")
        ctrl=anim.get_editor_property("controller")
        ctrl.set_frame_rate(unreal.FrameRate(numerator=30,denominator=1),should_transact=False)
        ctrl.set_number_of_frames(unreal.FrameNumber(value=n-1),should_transact=False)
        tracks=[]
        ctrl.open_bracket("CMU 2010 genuine mocap "+label,False)
        try:
            for bone,frame_quats in data["local_native_quaternion_tracks"].items():
                if bone not in bones:continue
                require(len(frame_quats)==n,"Source quaternions do not match frame count")
                require(bone not in ("Center","pelvis","Pelvis"),"Root translation/orientation authority blocked")
                tf=pose.get_bone_pose(unreal.Name(bone),unreal.AnimPoseSpaces.LOCAL)
                p=unreal.Vector(tf.translation.x,tf.translation.y,tf.translation.z)
                scl=unreal.Vector(tf.scale3d.x,tf.scale3d.y,tf.scale3d.z)
                require(bool(ctrl.add_bone_track(unreal.Name(bone),False)),
                        "Failed exact native bone "+bone)
                quats=keys_as_quats(frame_quats)
                require(bool(ctrl.set_bone_track_keys(unreal.Name(bone),
                        [p]*n,quats,[scl]*n,False)), "Could not write real pose track "+bone)
                tracks.append(bone)
        finally:
            ctrl.close_bracket(False)
        require(len(tracks)>=12,"Not enough true mocap limb tracks "+str(tracks))
        actual=set(str(v) for v in ctrl.get_model_interface().get_bone_track_names())
        require(actual==set(tracks),"Native animation controller bone mismatch")
        require(abs(anim.get_play_length()-(n-1)/30)<0.07,"Native clip duration mismatch")
        require(bool(unreal.EditorAssetLibrary.save_loaded_asset(anim)),"UE native save failed")
        status["native_assets"].append({"name":name,"asset":anim.get_path_name(),
            "skeleton":sk.get_path_name(),"mocap_data_file":str(src),
            "native_bone_tracks":tracks,"native_track_count":len(tracks),
            "frames":n,"duration":anim.get_play_length(),"genuine_source_mocap":True,
            "runtime_playable":False,"looping_approved":False,
            "foot_contact_ik_authoring":"PENDING",
            "real_pose_visual_check":"PENDING"})
        save()
    status["status"]="PASS_REAL_CMU_SOURCE_TO_ONEPART_NATIVE_UE_SEQUENCE_3_CLIPS_PRE_VISUAL"
except Exception as exc:
    status["status"]="FAIL_ONEPART_MOCAP_NATIVE_AUTHORING"
    status["error"]=str(exc)
    status["traceback"]=traceback.format_exc()[-4500:]
finally:
    status["utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
    save()
    unreal.log("GATE12_MOCAP_V3_NATIVE_AUTHORING="+status["status"])
