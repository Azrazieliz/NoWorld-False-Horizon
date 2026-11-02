"""Gate12: accept only genuine KKBP PMX exports from exact user-approved Alter card hashes.
No geometry generation, no substitute Base appearance, no source edits, no overwrite.
"""
from __future__ import annotations
import pathlib,json,hashlib,shutil,zipfile,datetime
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016")
PROJECT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
GAME=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
FROZEN=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE01_LUMINOUS_BASE_AND_ALTER_NATIVE_3D_SOURCES_20261016.zip")
with zipfile.ZipFile(FROZEN) as zipped:
    manifest=json.loads(zipped.read("02_USER_CONFIRMED_FINAL_NATIVE_SOURCE_MANIFEST.json"))
source=manifest["alter"]["appearances"]
def digest(path):
    with open(path,"rb") as handle:
        h=hashlib.sha256()
        for block in iter(lambda:handle.read(1024*1024),b""):
            h.update(block)
    return h.hexdigest()
for state in ("UNDERWEAR","NUDE"):
    row=source[state]
    card=pathlib.Path(row["source"])
    assert card.is_file() and digest(card)==row["sha256"]
    api_json=PROJECT/"Saved/Gate12Readiness"/f"Gate12_Alter_{state}_Approved_API_Export_20261022.json"
    assert api_json.is_file(),f"Native API has not produced {state}: {api_json}"
    proof=json.loads(api_json.read_text(encoding="utf8"))
    assert proof["status"]=="EXPORTED_NATIVE_APPROVED_ALTER_CARD"
    assert proof["source_card_sha256"]==row["sha256"]
    pmx=pathlib.Path(proof["pmx_path"]).resolve(strict=True)
    assert pmx.name=="model.pmx" and pmx.parent.parent.resolve()==(GAME/"Export_PMX").resolve()
    assert digest(pmx)==proof["pmx_sha256"] and pmx.stat().st_size>10000
    files=sorted(f for f in pmx.parent.rglob("*") if f.is_file())
    assert 20<len(files)<3500
    assert any(f.suffix==".json" for f in files) and any(f.suffix==".png" for f in files)
    target=ROOT/"FRESH_PARTY_EXPORTS"/state
    assert not target.exists(),"Approved state output already exists; refusing overwrite"
    target.mkdir(parents=True)
    copied=[]
    for f in files:
        rel=f.relative_to(pmx.parent)
        assert ".." not in rel.parts
        dest=target/rel
        dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(f,dest)
        src_hash=digest(f)
        assert digest(dest)==src_hash
        copied.append({"path":rel.as_posix(),"sha256":src_hash,"bytes":dest.stat().st_size})
    report={
        "schema":"noworld.g12.alter.approved.native_kkbp_export.v1",
        "status":"PASS_GENUINE_KKBP_PMX_TEXTURE_METADATA_EXPORT_HASHED",
        "state":state,
        "native_card_sha256":row["sha256"],
        "pmx_sha256":proof["pmx_sha256"],
        "pmx_original_game_path":str(pmx),
        "original_card_file":str(card),
        "native_export_generated_via_api":True,
        "underlying_engine":"Koikatsu Party KKBP 5.06",
        "coordinate_index":0,
        "files_count":len(copied),
        "total_bytes":sum(item["bytes"] for item in copied),
        "file_inventory":copied,
        "original_sources_unchanged":True,
        "source_mesh_substituted":False,
        "accepted_at_utc":datetime.datetime.now(datetime.timezone.utc).isoformat()
    }
    receipt=ROOT/"FRESH_PARTY_EXPORTS"/("RECEIPT_"+state+".json")
    assert not receipt.exists()
    receipt.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
    print("GATE12_ALTER_REAL_KKBP_ACCEPTED",state,"FILES",len(files),"PMX_SHA256",proof["pmx_sha256"],flush=True)
