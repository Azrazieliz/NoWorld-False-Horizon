"""Stage only the actual compiled exact-card exporter and an honest proof manifest.
Never install/launch Koikatsu while the memory-limited Unreal compiler is active."""
import pathlib,hashlib,shutil,json,datetime
R=pathlib.Path(r"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031")
SRC=R/"Scripts/production/Gate12GrandExactUnityMeshStream.cs"
DLL=pathlib.Path(r"D:\UnrealProjects\G12_LuminousNativeImport_20261031\Saved\CorrectKoikatsuExports\Gate12GrandExactUnityTextureStream_v4.dll")
CARD=R/"Assets/NoWorld/Luminous/KoikatsuOriginalCards/LUMINOUS_GRAND_CONVERGENCE_CLOTHED_KK.png"
EXPECTED="26c48e1c03c154f9f39a0b06afaade53423c11dbd079604e430162eb4cae5353"
def sha(p):
 with p.open("rb") as f:return hashlib.file_digest(f,"sha256").hexdigest()
assert sha(CARD)==EXPECTED
assert DLL.stat().st_size>=20000 and DLL.stat().st_mtime>=SRC.stat().st_mtime
s=SRC.read_text("utf8")
assert "NOWORLD_EXACT_KOIKATSU_UNITY_SKIN_V3_SPARSE_MORPHS_CURRENT_WEIGHTS" in s
assert "SaveOriginalMaterialTextures" in s and "SourceBone" not in s
assert "ShowWindow" not in s and "GetTexturePropertyNames" not in s
TARGET=R/"Tools/Koikatsu/GrandExactOriginalCardExporter"
TARGET.mkdir(parents=True,exist_ok=True)
binary=TARGET/"Gate12GrandExactUnityTextureStream.dll"
if binary.exists():assert sha(binary)==sha(DLL)
else:shutil.copy2(DLL,binary)
doc={"schema":"noworld.gate12.grand_exact_koikatsu_source_stream.v4",
     "card_file":CARD.name,"original_card_sha256":EXPECTED,
     "plugin_binary":binary.relative_to(R).as_posix(),
     "compiled_plugin_sha256":sha(binary),"compiled_plugin_bytes":binary.stat().st_size,
     "source_code":SRC.relative_to(R).as_posix(),"source_sha256":sha(SRC),
     "unity_version":"Koikatsu Party installed UnityEngine.dll .NET 3.5",
     "geometry":"source vertex/index/normals/uv/weight matrices and all original morph frame values",
     "textures":"full original GPU pixel readback for defined live named shader texture properties",
     "texture_slots_all_shaders_exhaustively_covered":False,
     "actual_original_card_export_complete":False,
     "native_blender_unreal_asset_import_complete":False,
     "native_android_gameplay_verified":False,
     "status":"PASS_COMPILED_NATIVE_KOIKATSU_EXACT_CARD_EXPORTER_ONLY_NOT_EXPORTED"}
(TARGET/"GRAND_EXACT_SOURCE_EXPORTER_PROVENANCE.json").write_text(json.dumps(doc,indent=2)+"\n","utf8")
print("GRAND_ORIGINAL_CARD_NATIVE_SOURCE_DLL_STAGED_NO_FAKE_MESH",doc["compiled_plugin_sha256"])
