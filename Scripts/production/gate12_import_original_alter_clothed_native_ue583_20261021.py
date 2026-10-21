"""Unreal 5.8.3 asset-only import of USER-CONFIRMED original Alter CLOTHED.

Locked source is the distinct 1130-joint Koikatsu Alter, never the 975-bone
Base or an invented outfit/portrait. No C++ compilation/cooking performed.
"""
import unreal,json,hashlib,traceback,pathlib
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SOURCE=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016\BLENDER_FRESH\CLOTHED\LUMINOUS_ALTER_CLOTHED_original_native_ue583.fbx")
SHA="ffc4878e5eedd2d378376d229c5a920778933914a4c27828d127785644a9de63"
DEST="/Game/NoWorld/Characters/Luminous/Alter/NativeOriginalClothed"
NAME="SK_Luminous_Alter_CLOTHED_Original"
RECEIPT=ROOT/"Saved/Gate12Readiness/Gate12_OriginalAlterCLOTHED_UnrealNativeImport_20261021.json"
result={"status":"STARTED","source_fbx":str(SOURCE),
        "source_sha256_expected":SHA,"source_identity":"USER_CONFIRMED_ALTER_NOT_BASE",
        "native_destination":DEST,"native_name":NAME,
        "base_asset_overwritten":False,"source_modified":False,
        "compiler_invoked":False,"runtime_character_accepted":False}
try:
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==SHA
    assert "OfflineGame_Gate12_Koikatsu_20261010" in str(unreal.Paths.get_project_file_path())
    asset_path=DEST+"/"+NAME
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        obj=unreal.load_asset(asset_path)
        assert isinstance(obj,unreal.SkeletalMesh),"Existing asset name owned by another asset class"
        result["native_source_preserved"]=True
    else:
        unreal.EditorAssetLibrary.make_directory(DEST)
        opts=unreal.FbxImportUI()
        opts.set_editor_property("automated_import_should_detect_type",False)
        opts.set_editor_property("mesh_type_to_import",unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        opts.set_editor_property("import_as_skeletal",True)
        opts.set_editor_property("import_mesh",True)
        opts.set_editor_property("import_animations",False)
        opts.set_editor_property("skeleton",None)
        opts.set_editor_property("import_materials",True)
        opts.set_editor_property("import_textures",True)
        opts.set_editor_property("create_physics_asset",False)
        mesh_settings=opts.get_editor_property("skeletal_mesh_import_data")
        assert mesh_settings is not None,"Skeletal asset data importer not present"
        mesh_settings.set_editor_property("import_morph_targets",True)
        task=unreal.AssetImportTask()
        task.set_editor_property("filename",str(SOURCE))
        task.set_editor_property("destination_path",DEST)
        task.set_editor_property("destination_name",NAME)
        task.set_editor_property("automated",True)
        task.set_editor_property("save",True)
        task.set_editor_property("replace_existing",False)
        task.set_editor_property("options",opts)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        result["imported_paths"]=[str(s) for s in task.get_editor_property("imported_object_paths")]
        obj=unreal.load_asset(asset_path)
        assert isinstance(obj,unreal.SkeletalMesh),"Original FBX did not produce a native UE skeletal mesh"
    sk=obj.get_editor_property("skeleton")
    assert isinstance(sk,unreal.Skeleton),"Native source has no skeleton"
    bone_names=list(sk.get_reference_pose().get_bone_names())
    result["unreal_skeleton_bones"]=len(bone_names)
    result["native_skeletal_mesh"]=obj.get_path_name()
    result["native_skeleton"]=sk.get_path_name()
    assert len(bone_names)>=900,"Original rig unexpectedly lost essential source bones"
    mats=obj.get_editor_property("materials")
    result["material_slots"]=len(mats)
    assert len(mats)>0,"No actual imported Alter materials"
    try:
        morphs=obj.get_editor_property("morph_targets")
        result["imported_morph_targets"]=len(morphs)
    except Exception as exc:
        result["morph_target_query"]=str(exc)[:350]
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sk)
    result["status"]="PASS_ORIGINAL_ALTER_CLOTHED_NATIVE_SKINNED_MESH_IMPORT"
except Exception:
    result["status"]="FAILED_ORIGINALLY_SKINNED_UNREAL_IMPORT"
    result["error"]=traceback.format_exc()[-4800:]
finally:
    RECEIPT.parent.mkdir(parents=True,exist_ok=True)
    RECEIPT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
    unreal.log("GATE12_ALTER_ORIGINAL_CLOTHED_UE="+result["status"])
