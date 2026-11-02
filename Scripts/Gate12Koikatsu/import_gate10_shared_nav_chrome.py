"""Import only the neutral, Figma-derived Gate10 chrome shared by native Ruler screens.
Run in UnrealEditor Python commandlet after any active build. No mockup characters/data.
Do not replace the already approved Gacha texture.
"""
import unreal
from pathlib import Path
src=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\SourceArt\Gate10\Figma\Gate10_SharedFiveNavChrome_Neutral.png")
asset="/Game/UI/Gate10/Chrome/T_Gate10_SharedFiveNavChrome_Neutral"
if not src.is_file() or src.stat().st_size<20000:
    raise RuntimeError("Approved Gacha-derived neutral chrome missing or suspiciously short")
if unreal.EditorAssetLibrary.does_asset_exist(asset):
    print("SHARED_NAV_CHROME_PRESERVED",asset)
else:
    task=unreal.AssetImportTask()
    for key,value in {
        "filename":str(src),"destination_path":"/Game/UI/Gate10/Chrome",
        "destination_name":"T_Gate10_SharedFiveNavChrome_Neutral",
        "automated":True,"save":True,"replace_existing":False
    }.items():task.set_editor_property(key,value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not isinstance(unreal.EditorAssetLibrary.load_asset(asset),unreal.Texture2D):
        raise RuntimeError("Shared nav native UTexture2D not imported")
    print("SHARED_NAV_CHROME_IMPORTED",asset)
