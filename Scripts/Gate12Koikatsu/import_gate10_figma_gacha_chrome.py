# Gate10 approved interface-only Figma chrome transfer into Unreal.
# Preserves Foundation, gameplay widgets, registry, saves and existing assets.
import unreal
from pathlib import Path
import sys

source=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Readiness\Gate10_Figma_PixelMatched_V2\gacha_chrome_transparent.png")
dest="/Game/UI/Gate10/Chrome"
asset_name="T_Gate10_Gacha_VectorChrome_V2"
package=dest+"/"+asset_name
if not source.is_file() or source.stat().st_size < 50000:
    raise RuntimeError("Exact Figma export source missing")
if unreal.EditorAssetLibrary.does_asset_exist(package):
    print("Gate10 Figma chrome already present; no overwrite:", package)
else:
    task=unreal.AssetImportTask()
    task.set_editor_property("filename",str(source))
    task.set_editor_property("destination_path",dest)
    task.set_editor_property("destination_name",asset_name)
    task.set_editor_property("automated",True)
    task.set_editor_property("save",True)
    task.set_editor_property("replace_existing",False)
    task.set_editor_property("replace_existing_settings",False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    print("Imported packaged art-only chrome asset paths:",list(task.get_editor_property("imported_object_paths")))
obj=unreal.EditorAssetLibrary.load_asset(package)
if not isinstance(obj,unreal.Texture2D):
    raise RuntimeError("Failed native UE imported Texture2D: "+package)
print("GATE10_FIGMA_NATIVE_CHROME_READY",obj.get_path_name())
