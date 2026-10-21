"""Gate 09 original A1 Scarlet Cadence -> existing 975-bone Luminous UE rig.

Animation-only FBX import into an isolated native animation namespace.
This does not rebuild code, import new characters, replace source geometry,
or label unreviewed Gate09 combat motion as finished visual animation.
"""
import unreal,pathlib,json,hashlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
G=pathlib.Path(r"D:\UnrealProjects\Gate09_Protected_Production_20261016")
S=G/"A1_ActualGrounded_UnrealExports_v9"
info=json.loads((S/"INTERCHANGE.json").read_text(encoding="utf8"))
fbx=pathlib.Path(info["fbx"])
root="/Game/NoWorld/Combat/Animations/Gate09Intake"
asset_name="G09_LuminousV3_Base_A1_ScarletCadence_Grounded_v9"
dest=root+"/"+asset_name
skeleton_path=("/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/"
"Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/"
"Mesh_00_Prim_00_Skeleton.Mesh_00_Prim_00_Skeleton")
out=R/"Saved/Gate12AnimationAcceptance/Gate09_BaseA1_RealFBXToNativeUnreal_20261021.json"
report={"status":"START","source":str(fbx),"destination":dest,
    "source_interchange":str(S/"INTERCHANGE.json"),"native_asset_created":False,
    "visual_approval":False,"reused_original_rig":True,
    "existing_content_overwritten":False,"errors":[]}
try:
    assert info["status"]=="GATE09_V9_PHYSICALLY_GROUNDED_BLENDER_EXPORTED_UNREAL_COLD_PENDING"
    assert fbx.is_file()
    digest=hashlib.sha256(fbx.read_bytes()).hexdigest()
    assert digest==info["sha256"],"Gate09 authored original FBX hash differs"
    report["source_sha256"]=digest
    assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
    sk=unreal.load_asset(skeleton_path)
    assert isinstance(sk,unreal.Skeleton),"Original source skeleton missing"
    pose=sk.get_reference_pose()
    assert len(pose.get_bone_names())==975,"Not the preserved original 975-joint rig"
    assert not unreal.EditorAssetLibrary.does_asset_exist(dest),"Source already imported: refuse modifying or repeating"
    ui=unreal.FbxImportUI()
    ui.set_editor_property("automated_import_should_detect_type",False)
    ui.set_editor_property("mesh_type_to_import",unreal.FBXImportType.FBXIT_ANIMATION)
    ui.set_editor_property("import_as_skeletal",True)
    ui.set_editor_property("import_mesh",False)
    ui.set_editor_property("import_animations",True)
    ui.set_editor_property("skeleton",sk)
    anim_settings=ui.get_editor_property("anim_sequence_import_data")
    if anim_settings:
        # No nominal FPS stretching or synthesized animation keys.
        for name,value in (("remove_redundant_keys",False),
                           ("import_custom_attribute",True),
                           ("preserve_local_transform",True)):
            try:anim_settings.set_editor_property(name,value)
            except Exception:pass
    task=unreal.AssetImportTask()
    task.set_editor_property("filename",str(fbx))
    task.set_editor_property("destination_path",root)
    task.set_editor_property("destination_name",asset_name)
    task.set_editor_property("options",ui)
    task.set_editor_property("automated",True)
    task.set_editor_property("save",True)
    task.set_editor_property("replace_existing",False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    report["created_object_paths"]=[str(p) for p in task.get_editor_property("imported_object_paths")]
    obj=unreal.load_asset(dest)
    if not isinstance(obj,unreal.AnimSequence):
        raise RuntimeError("FBX did not yield an exact native animation asset: "+str(report["created_object_paths"]))
    if obj.get_editor_property("skeleton")!=sk:
        raise RuntimeError("Foreign rig imported: retained in isolated intake, not gameplay linked")
    mdl=obj.get_editor_property("controller").get_model_interface()
    tracks=[str(t) for t in mdl.get_bone_track_names()]
    assert len(tracks)>=20,"Animated named rig lacks meaningful actual joint motion"
    duration=float(obj.get_play_length())
    assert duration>0.3,"Unreal clip has no meaningful source time"
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj),"Failed saving native A1 animation"
    report.update({"status":"PASS_NATIVE_A1_SOURCE_ANIMATION_IMPORTED_PENDING_VISUAL",
       "native_asset_created":True,"asset":obj.get_path_name(),
       "native_duration_seconds":duration,"native_bone_tracks":len(tracks),
       "timeline_authoritative_hits":info["native_hit_event_seconds"],
       "source_grounded_accepted":False})
except Exception:
    report["status"]="FAILED_OR_RIG_MISMATCH_DO_NOT_ATTACH_GAMEPLAY"
    report["errors"].append(traceback.format_exc()[-4400:])
finally:
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
    unreal.log("GATE09_REAL_A1_IMPORT "+report["status"])
