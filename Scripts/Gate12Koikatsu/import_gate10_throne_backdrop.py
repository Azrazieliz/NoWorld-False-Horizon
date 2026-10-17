import unreal
from pathlib import Path
src=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\SourceArt\Gate10\Figma\vacant_throne_architecture_backdrop_v1.png")
dst="/Game/UI/Gate10/Opening"
name="T_Gate10_VacantThroneBackdrop_V1"
if not src.is_file() or src.stat().st_size<200000:
    raise RuntimeError("Original edited Figma throne source is absent")
target=dst+"/"+name
if unreal.EditorAssetLibrary.does_asset_exist(target):
    raise RuntimeError("Would overwrite existing Unreal throne backdrop")
task=unreal.AssetImportTask()
task.set_editor_property("filename",str(src))
task.set_editor_property("destination_path",dst)
task.set_editor_property("destination_name",name)
task.set_editor_property("automated",True)
task.set_editor_property("save",True)
task.set_editor_property("replace_existing",False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
obj=unreal.EditorAssetLibrary.load_asset(target)
if not isinstance(obj,unreal.Texture2D):
    raise RuntimeError("Native Unreal Texture2D failed: "+target)
print("GATE10_ORIGINAL_EDITABLE_THRONE_BACKGROUND_NATIVE",obj.get_path_name())
