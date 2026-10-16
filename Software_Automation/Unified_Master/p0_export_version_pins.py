"""Emit reproducible P0 tool/source provenance; read-only source hashes."""
from pathlib import Path
import platform,sys,hashlib,json,datetime
R=Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
GAME=Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party')
SAN=Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014')
BL=Path(r'D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe')
files={
 'blender_executable':BL,
 'party_executable_original':GAME/'Koikatsu Party.exe',
 'kkbp_exporter_original':GAME/'BepInEx/plugins/KKBP_Exporter.dll',
 'kkbp_exporter_sandbox':SAN/'BepInEx/plugins/KKBP_Exporter.dll',
 'sandbox_runtime_dll_restored':SAN/'BepInEx/plugins/SM01RuntimeController.dll',
 'native_probe_QA_compiled_uninstalled':R/'B_CAPTURE_PLUGIN/P0_SM01RuntimeController_Unity35_QA.dll',
}
for x in ('CLOTHED','UNDERWEAR','NUDE'):
 folder=Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011')
 files['archived_glb_'+x]=folder/'BLENDER_FRESH'/x/('LUMINOUS_V3_'+x+'_skinned.glb')
 files['archived_blend_'+x]=folder/'BLENDER_FRESH'/x/('LUMINOUS_V3_'+x+'_original_import.blend')
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for buf in iter(lambda:f.read(1<<20),b''):h.update(buf)
 return h.hexdigest()
record={'schema':'NW_P0_TARGETED_PINNED_CONFIG_V1','date':'2026-10-16',
 'python':platform.python_version(),'blender': '5.2.2 LTS','kkbp_dotnet_version': '5.6.0.0',
 'original_party_is_user_controlled':True,'probe_QA_compiled_not_live_validated':True,'production_exporter_switch':False,
 'dependencies':{},'sources_sha_manifest':str(R/'SOURCE_IMMUTABLE_INPUT_MANIFEST.json'),
 'quality_regression_receipt':str(R/'P0_TARGETED_FOLLOWUP_REGRESSIONS.json'),
 'source_visual_evidence':str(R/'A_CONTROLLED_VIEWS')}
for name,p in files.items():
 if not p.is_file():raise FileNotFoundError(p)
 record['dependencies'][name]={'path':str(p),'size':p.stat().st_size,'sha256':digest(p)}
 if 'kkbp_exporter' in name and record['dependencies'][name]['size']<15000:raise RuntimeError('Unexpected KKBP DLL')
out=R/'P0_TARGETED_EXACT_TOOL_AND_SOURCE_PINS.json';out.write_text(json.dumps(record,indent=2))
print('P0_PINNED',len(record['dependencies']),'checks',out,'original_game_hash',record['dependencies']['kkbp_exporter_original']['sha256'][:16],
  'sandbox_exporter_hash',record['dependencies']['kkbp_exporter_sandbox']['sha256'][:16],flush=True)
