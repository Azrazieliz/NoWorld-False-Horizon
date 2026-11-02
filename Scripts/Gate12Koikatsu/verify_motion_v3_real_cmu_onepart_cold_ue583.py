"""Independent fresh Unreal editor verification of three authentic CMU native clips.

Checks ACTUAL serialized .uasset types, exact source skeleton identity,
bone-track count and duration. Does not claim visual gait acceptance.
"""
import json,pathlib,os,time,traceback
import unreal
ROOT=pathlib.Path(os.environ.get("G12_GATE12_WORKTREE",r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010"))
BASE=ROOT/"Saved/Gate12Koikatsu/MotionV3"
OUT="/Game/Experimental/Gate12Koikatsu/Animation/NativeMocapV3"
PATH=BASE/"native_mocap_v3_onepart_cold_unreal.json"
RESULT={"status":"START","read_only":True,"fresh_native_UE_process":True,"clips":[],
        "physical_visual_approval":False,"runtime_on_S26":False}
def save():
    PATH.write_text(json.dumps(RESULT,indent=2)+"\n",encoding="utf-8")
try:
    assert "OfflineGame_Gate12_Koikatsu_20261010" in str(unreal.Paths.get_project_file_path())
    report=json.loads((BASE/"native_mocap_v3_onepart_ue_authoring.json").read_text("utf-8-sig"))
    assert report["status"]=="PASS_REAL_CMU_SOURCE_TO_ONEPART_NATIVE_UE_SEQUENCE_3_CLIPS_PRE_VISUAL"
    assert len(report["native_assets"])==3
    for part in report["native_assets"]:
        native=unreal.load_asset(part["asset"])
        assert isinstance(native,unreal.AnimSequence),part["asset"]
        assert native.get_editor_property("skeleton").get_path_name()==part["skeleton"]
        duration=native.get_play_length()
        assert abs(duration-part["duration"])<.07,part["name"]
        names=set(str(n) for n in native.get_editor_property("controller").get_model_interface().get_bone_track_names())
        required={"thigh_l","calf_l","thigh_r","calf_r","upperarm_l","upperarm_r","lowerarm_l","lowerarm_r"}
        assert required.issubset(names),(part["name"],sorted(required-names))
        assert names==set(part["native_bone_tracks"]),part["name"]
        assert not names.intersection(("Center","pelvis","Pelvis")),"Root channels forbidden"
        RESULT["clips"].append({"name":part["name"],"skeleton":part["skeleton"],
            "native_asset_type":"AnimSequence",
            "bones":len(names),"duration_seconds":duration,
            "frames":part["frames"],
            "genuine_real_motion_source":True,"native_source_compatible":True,
            "root_translation_tracks":0,"foot_contact_ik":"NOT_TESTED",
            "natural_visual_motion":"NOT_APPROVED"})
        save()
    assert len(RESULT["clips"])==3
    RESULT["status"]="PASS_FRESH_UE5_NATIVE_3_MOCAP_CLIPS_ONEPART_PRE_VISUAL"
except Exception as e:
    RESULT["status"]="FAIL_FRESH_UE5_NATIVE_3_MOCAP_CLIPS"
    RESULT["error"]=str(e)
    RESULT["traceback"]=traceback.format_exc()[-2500:]
finally:
    RESULT["utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
    save()
    unreal.log("GATE12_V3_MOCAP_COLD="+RESULT["status"])
