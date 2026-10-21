"""Import only genuine transformed Gate02 assets, no placeholder meshes.
Editor asset authoring is not an Android game build. Preserve all old imports."""
import unreal,json,time,traceback,hashlib
from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
src=root/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
receipt=root/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/ue_native_module_import.json"
source_names={"shrub":"SM_R1_ShrubSource","fir_sapling":"SM_R1_FirSource",
              "oak_tree":"SM_R1_OakSource"}
result={"status":"STARTED","assets":[],"errors":[]}
def save():receipt.write_text(json.dumps(result,indent=2,default=str),encoding="utf-8")
save()
try:
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    for name,target in source_names.items():
        f=src/(name+"_NativeUE_Source.fbx")
        assert f.is_file(),f
        path="/Game/NoWorld/FirstWorld/R1/Modules"
        obj_path=path+"/"+target
        if unreal.EditorAssetLibrary.does_asset_exist(obj_path):
            result["assets"].append({"source":name,"status":"PROTECTED_EXISTING","package":obj_path})
            continue
        task=unreal.AssetImportTask()
        task.set_editor_property("filename",str(f))
        task.set_editor_property("destination_path",path)
        task.set_editor_property("destination_name",target)
        task.set_editor_property("automated",True)
        task.set_editor_property("save",True)
        task.set_editor_property("replace_existing",False)
        task.set_editor_property("async_",False)
        opts=unreal.FbxImportUI()
        opts.set_editor_property("import_mesh",True)
        opts.set_editor_property("import_as_skeletal",False)
        opts.set_editor_property("import_animations",False)
        opts.set_editor_property("import_materials",True)
        opts.set_editor_property("import_textures",True)
        opts.static_mesh_import_data.set_editor_property("combine_meshes",True)
        opts.static_mesh_import_data.set_editor_property("auto_generate_collision",True)
        task.set_editor_property("options",opts)
        tools.import_asset_tasks([task])
        mesh=unreal.EditorAssetLibrary.load_asset(obj_path)
        if not mesh or not isinstance(mesh,unreal.StaticMesh):
            raise RuntimeError("Actual Gate02 FBX did not create a static mesh at "+obj_path)
        entry={"source":name,"original_source_sha256":json.loads(
            (src/(name+"_conversion.json")).read_text())["source_sha256"],
            "fbx_sha256":hashlib.sha256(f.read_bytes()).hexdigest(),
            "status":"NATIVE_STATICMESH_IMPORTED_NOT_WORLD_VERIFIED",
            "package":mesh.get_path_name(),
            "bounds":str(mesh.get_bounds())}
        result["assets"].append(entry)
        save()
    result["status"]="SOURCE_BACKED_MODULES_NATIVE_IMPORTED"
except Exception as e:
    result["status"]="PARTIAL_OR_FAILED"
    result["errors"]=[str(e),traceback.format_exc()[-6500:]]
finally:
    save()
    unreal.log("GATE12_R1_MODULES="+result["status"])
