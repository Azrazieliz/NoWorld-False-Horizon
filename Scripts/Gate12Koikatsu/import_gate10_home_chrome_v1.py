import unreal
from pathlib import Path
src=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\SourceArt\Gate10\Figma\home_gothic_chrome_v2_black.png")
name="T_Gate10_HomeChrome_V1"
dest="/Game/UI/Gate10/Chrome"
pkg=dest+"/"+name
if not src.exists() or src.stat().st_size<50000:
    raise RuntimeError("Figma Home source absent")
if unreal.EditorAssetLibrary.does_asset_exist(pkg):
    print("HOME_CHROME_ALREADY_EXISTS_PRESERVING",pkg)
else:
    task=unreal.AssetImportTask()
    for k,v in (("filename",str(src)),("destination_path",dest),("destination_name",name),("automated",True),("save",True),("replace_existing",False)):
        task.set_editor_property(k,v)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not isinstance(unreal.EditorAssetLibrary.load_asset(pkg),unreal.Texture2D):
        raise RuntimeError("Native Home texture missing")
    print("HOME_CHROME_NATIVE_ASSET_READY",pkg)
