"""UE 5.8.3 import of ONE original approved Alter UNDERWEAR or NUDE part.
Run one state+part per low-memory UnrealEditor-Cmd process. No source overwrite.
"""
import unreal,pathlib,json,hashlib,traceback,os
STATE=os.environ["G12_ALTER_STATE"]
PART=int(os.environ["G12_ALTER_PART"])
assert STATE in ("UNDERWEAR","NUDE") and 0<=PART<6
GAME=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
ART=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016")
FOLDER=ART/"BLENDER_FRESH"/STATE/"UNREAL_ORIGINAL_PARTS"
PROOF=json.loads((FOLDER/f"GATE12_ALTER_{STATE}_NATIVE_PARTS_20261022.json").read_text(encoding="utf8"))
assert PROOF["status"]=="EXACT_ORIGINAL_APPROVED_ALTER_WARDROBE_SKINNED_PARTS_EXPORTED"
assert len(PROOF["parts"])==6
ROW=PROOF["parts"][PART]
SOURCE=pathlib.Path(ROW["fbx"])
DEST="/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal"+STATE.title()
NAME=f"SK_Luminous_Alter_{STATE}_OriginalPart_{PART:02d}"
TARGET=DEST+"/"+NAME
RECEIPT=GAME/"Saved/Gate12Readiness"/f"Gate12_Alter_{STATE}_UE_OriginalPart_{PART:02d}_20261022.json"
report={
 "schema":"noworld.gate12.alter.original_wardrobe_native_import.v1",
 "state":STATE,"part":PART,"original_source_card_sha256":PROOF["source_card_sha256"],
 "source_blend_sha256":PROOF["source_blend_sha256"],
 "source_mesh_name":ROW["mesh_original_name"],
 "source_vertices":ROW["vertices"],"source_morph_targets":ROW["morph_targets"],
 "source_armature_bones":ROW["original_armature_bones"],
 "original_source_untouched":True,"base_mesh_substituted":False,
 "version":"character:luminous_valentine.version_alter_v1",
 "status":"STARTED"
}
try:
    assert GAME.name in unreal.Paths.get_project_file_path()
    assert SOURCE.is_file()
    with open(SOURCE,"rb") as file:
        h=hashlib.sha256()
        for block in iter(lambda:file.read(4*1024*1024),b""):h.update(block)
    assert h.hexdigest()==ROW["fbx_sha256"]
    if unreal.EditorAssetLibrary.does_asset_exist(TARGET):
        obj=unreal.load_asset(TARGET)
        assert isinstance(obj,unreal.SkeletalMesh)
        report["preexisting_asset_preserved"]=True
    else:
        unreal.EditorAssetLibrary.make_directory(DEST)
        ui=unreal.FbxImportUI()
        ui.set_editor_property("automated_import_should_detect_type",False)
        ui.set_editor_property("mesh_type_to_import",unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        ui.set_editor_property("import_as_skeletal",True)
        ui.set_editor_property("import_mesh",True)
        ui.set_editor_property("import_animations",False)
        ui.set_editor_property("import_materials",True)
        ui.set_editor_property("import_textures",True)
        ui.set_editor_property("create_physics_asset",False)
        shared=None
        if PART>0:
            shared=unreal.load_asset(DEST+f"/SK_Luminous_Alter_{STATE}_OriginalPart_00_Skeleton")
            assert isinstance(shared,unreal.Skeleton),"Original state part00 skeleton absent"
        ui.set_editor_property("skeleton",shared)
        ui.get_editor_property("skeletal_mesh_import_data").set_editor_property("import_morph_targets",True)
        task=unreal.AssetImportTask()
        for key,value in [
           ("filename",str(SOURCE)),("destination_path",DEST),
           ("destination_name",NAME),("automated",True),("save",True),
           ("replace_existing",False),("options",ui)]:
            task.set_editor_property(key,value)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        report["import_paths"]=[str(x) for x in task.get_editor_property("imported_object_paths")]
        obj=unreal.load_asset(TARGET)
        assert isinstance(obj,unreal.SkeletalMesh),"Original FBX not imported as native UE skeletal mesh"
    skeleton=obj.get_editor_property("skeleton")
    assert isinstance(skeleton,unreal.Skeleton)
    bones=len(skeleton.get_reference_pose().get_bone_names())
    report["native_skeleton_bones"]=bones
    assert bones==ROW["original_armature_bones"]+1,(bones,ROW["original_armature_bones"])
    report["native_mesh_path"]=obj.get_path_name()
    report["native_skeleton_path"]=skeleton.get_path_name()
    report["native_material_slots"]=len(obj.get_editor_property("materials"))
    assert report["native_material_slots"]>0
    try:
        report["native_morph_targets"]=len(obj.get_editor_property("morph_targets"))
    except Exception:
        report["native_morph_read_unavailable"]=True
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj)
    assert unreal.EditorAssetLibrary.save_loaded_asset(skeleton)
    report["status"]="ACTUAL_USER_CONFIRMED_ALTER_WARDROBE_PART_IMPORTED_NATIVE"
except Exception:
    report["status"]="SOURCE_OR_NATIVE_UE_IMPORT_BLOCKED_UNCHANGED"
    report["error"]=traceback.format_exc()[-4000:]
finally:
    RECEIPT.parent.mkdir(parents=True,exist_ok=True)
    RECEIPT.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
    unreal.log("G12_NATIVE_ALTER_"+STATE+"_"+str(PART)+"="+report["status"])
