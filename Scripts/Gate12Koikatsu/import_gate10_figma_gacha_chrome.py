# Reimport our own WIP chrome asset after exact Gate10 comparison.
# Never touches approved character art, game Foundation, saves or Registry.
import unreal
from pathlib import Path

source=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\SourceArt\Gate10\Figma\gacha_chrome_black_base_v3.png")
dest="/Game/UI/Gate10/Chrome"
asset_name="T_Gate10_Gacha_VectorChrome_V2"  # stable existing non-approved WIP asset
package=dest+"/"+asset_name
if not source.is_file() or source.stat().st_size<50000:
    raise RuntimeError("Missing corrected black-backed Figma chrome")
task=unreal.AssetImportTask()
task.set_editor_property("filename",str(source))
task.set_editor_property("destination_path",dest)
task.set_editor_property("destination_name",asset_name)
task.set_editor_property("automated",True)
task.set_editor_property("save",True)
# This replaces ONLY our own already-imported Gate12 work-in-progress asset.
task.set_editor_property("replace_existing",True)
task.set_editor_property("replace_existing_settings",True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
obj=unreal.EditorAssetLibrary.load_asset(package)
if not isinstance(obj,unreal.Texture2D):
    raise RuntimeError("Native UE Texture2D unavailable "+package)
print("GATE10_FIGMA_CORRECTED_NATIVE_CHROME_READY",obj.get_path_name())
