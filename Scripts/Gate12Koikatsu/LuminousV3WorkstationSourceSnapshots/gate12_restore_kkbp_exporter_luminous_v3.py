"""Restore ONLY the KK-specific exporter DLL from locally archived matching release.

Reversible, 1 target. No gameplay, mod or other plugin overwrites.
"""
from pathlib import Path
import json,datetime,hashlib,zipfile,os
GAME=Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
ARCH=Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\addons\KKBP_v9.5.0\KKBP_Exporter_5.06_KK_and_KKS.zip")
OUT=Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
TARGET=GAME/"BepInEx/plugins/KKBP_Exporter.dll"
MEMBER="Koikatsu or Koikatsu Party/BepInEx/plugins/KKBP_Exporter.dll"
def sha(b):return hashlib.sha256(b).hexdigest()
if not (OUT/"SEALED_INTAKE_RECEIPT.json").is_file():raise RuntimeError("Luminous sealed source first")
with zipfile.ZipFile(ARCH) as z:
    assert z.testzip() is None
    data=z.read(MEMBER)
assert len(data)==259584
preexisting=TARGET.is_file()
record={"schema":"GATE12_KKBP_KOIKATSU_DLL_RESTORE_V1",
        "date_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "source_release_zip":str(ARCH),
        "source_zip_sha256":sha(ARCH.read_bytes()),
        "source_member":MEMBER,
        "only_modified_target":str(TARGET),
        "source_Koikatsu_Party_net35":True,
        "source_dll_sha256":sha(data),
        "target_was_present":preexisting,"original_game_cards_unchanged":True}
if preexisting:
    orig=TARGET.read_bytes()
    if orig!=data:
        backup=OUT/"BACKUP_PRE_KKBP_EXPORTER_EXISTING_DLL.bin"
        if not backup.exists():backup.write_bytes(orig)
        assert backup.read_bytes()==orig
        record["preexisting_dll_backup_sha256"]=sha(orig)
else:
    orig=None
if not preexisting or orig!=data:
    temp=TARGET.with_name("KKBP_Exporter.gate12_staged")
    temp.write_bytes(data)
    if sha(temp.read_bytes())!=sha(data):raise RuntimeError("DLL staged bytes mismatch")
    temp.replace(TARGET)
assert TARGET.read_bytes()==data
record["status"]="PASS_SINGLE_KKBP_EXPORTER_DLL_VERIFIED"
record["installed_target_sha256"]=sha(TARGET.read_bytes())
record["modified_only"]="BepInEx/plugins/KKBP_Exporter.dll"
(OUT/"KKBP_EXPORTER_SINGLE_DLL_RESTORE_RECEIPT.json").write_text(json.dumps(record,indent=2)+"\n",encoding="utf8")
print(json.dumps(record,indent=2))
