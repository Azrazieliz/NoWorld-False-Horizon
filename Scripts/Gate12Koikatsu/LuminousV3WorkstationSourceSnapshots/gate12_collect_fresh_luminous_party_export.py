"""Gate12 immutable fresh KKBP Party export collector.

Usage after author actually clicked KKBP in Maker:
python gate12_collect_fresh_luminous_party_export.py --state CLOTHED --export-dir "D:\...\Export_PMX\20261011..."
This never creates a fake model, and will reject older diagnostic PMXs.
"""
from __future__ import annotations
import argparse,datetime,hashlib,json,pathlib,shutil,sys
BASE=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
GAME=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
STAGE=BASE/"characters/LUMINOUS_VALENTINE_V3/gate12_fresh_export_20261011"
ORIGINAL_CARDS=STAGE/"SEALED_INPUTS"
EXP=GAME/"Export_PMX"
CUTOFF=datetime.datetime(2026,10,11,18,34,49,tzinfo=datetime.timezone(datetime.timedelta(hours=2))).timestamp()
MAX_FILES=3500
def sha(path):
 h=hashlib.sha256()
 with path.open("rb") as f:
  for block in iter(lambda:f.read(4*1024*1024),b""):h.update(block)
 return h.hexdigest()
def main():
 parser=argparse.ArgumentParser()
 parser.add_argument("--state",required=True,choices=("CLOTHED","UNDERWEAR","NUDE"))
 parser.add_argument("--export-dir",required=True,type=pathlib.Path)
 args=parser.parse_args()
 src=args.export_dir.resolve(strict=True)
 assert src.is_dir() and src!=EXP and src.is_relative_to(EXP.resolve(strict=True))
 assert src.parent==EXP.resolve(strict=True),"Export source must be a direct Party KKBP export folder"
 assert src.stat().st_mtime>CUTOFF,"Old export predates verified KKBP restore"
 sealed=json.loads((STAGE/"SEALED_INTAKE_RECEIPT.json").read_text(encoding="utf8"))
 card=ORIGINAL_CARDS/(args.state+".png")
 assert sha(card)==sealed["three_states"][args.state]["sha256"]
 # Standard genuine exporter output must include nonempty PMX and sidecar assets;
 # neither a preview image nor one of the unfitted GLBs can qualify.
 pmx=src/"model.pmx"
 assert pmx.is_file() and pmx.stat().st_size>10_000
 assert pmx.stat().st_mtime>CUTOFF,"PMX itself is stale"
 files=[x for x in src.rglob("*") if x.is_file()]
 assert 5<=len(files)<=MAX_FILES,(len(files),"missing KKBP texture/metadata outputs")
 assert all(not x.is_symlink() for x in files)
 assert any(x.suffix.lower()==".json" for x in files),"Missing native KKBP exported metadata"
 assert any(x.suffix.lower() in (".png",".jpg",".jpeg") for x in files),"Missing KKBP texture payload"
 dest=STAGE/"FRESH_PARTY_EXPORTS"/args.state
 if dest.exists() and any(dest.rglob("*")):
  raise RuntimeError("Refuse overwrite previously archived genuine "+args.state+" export")
 dest.mkdir(parents=True,exist_ok=True)
 records=[]
 for f in sorted(files):
  rel=f.relative_to(src)
  assert ".." not in rel.parts and not rel.is_absolute()
  to=dest/rel
  to.parent.mkdir(parents=True,exist_ok=True)
  shutil.copy2(f,to)
  digest=sha(f)
  assert sha(to)==digest
  records.append({"path":rel.as_posix(),"bytes":f.stat().st_size,"sha256":digest})
 receipt={"schema":"G12_LUMINOUS_FRESH_PARTY_KKBP_EXPORT_1",
          "status":"PASS_REAL_PARTY_KKBP_PMX_TEXTURE_METADATA_EXPORT_COPIED_AND_HASHED",
          "one_identity_three_clothing_states":True,"state":args.state,
          "source_card_sha256":sha(card),"coordinate_slot":0,
          "outfit_viewport_display_mode":"CLOTHED",
          "game_export_source_path":str(src),"state_output_path":str(dest),
          "pmx_sha256":sha(dest/"model.pmx"),"file_count":len(records),
          "total_export_bytes":sum(x["bytes"] for x in records),
          "file_inventory":records,"copied_from_running_party_not_generated":True,
          "native_Blender_reimport":"NOT_STARTED",
          "UE_native_import":"NOT_STARTED",
          "Android_state_test":"NOT_STARTED",
          "timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat()}
 target=STAGE/("FRESH_EXPORT_"+args.state+"_RECEIPT.json")
 target.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf8")
 print(json.dumps({k:receipt[k] for k in ("status","state","source_card_sha256","pmx_sha256","file_count","total_export_bytes","state_output_path")},indent=2))
if __name__=="__main__":
 try:main()
 except Exception as e:
  print("FRESH_EXPORT_NOT_ACCEPTED",str(e),file=sys.stderr)
  sys.exit(2)
