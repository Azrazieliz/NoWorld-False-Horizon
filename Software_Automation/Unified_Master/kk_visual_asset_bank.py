#!/usr/bin/env python3
"""Persistent CPU-only visual image index of ACTUAL tested Koikatsu assets.

Never invent a catalog asset image from a filename. Index an asset only if
a real native white-camera screenshot was captured AND its tested recipe,
image SHA and model fingerprint are available. Incremental and reusable.
Stores float16 embeddings ~3 KB / asset, not copied high-res textures.
"""
from __future__ import annotations
import argparse,hashlib,json,pathlib,sqlite3,time,numpy as np
import kk_local_vision_embeddings as emb

DEFAULT=pathlib.Path(r"D:\Tools\OfflineGameAI\models\KoikatsuVisualMobileCLIP2\koikatsu_asset_bank.sqlite")
SCHEMA="""
CREATE TABLE IF NOT EXISTS visual_asset(
 recipe_hash TEXT PRIMARY KEY,
 label TEXT NOT NULL,
 category TEXT NOT NULL,
 trial_steps TEXT NOT NULL,
 native_capture TEXT NOT NULL,
 native_capture_sha TEXT NOT NULL,
 model_sha TEXT NOT NULL,
 embedding_fp16 BLOB NOT NULL,
 native_visible_change REAL,
 created_utc TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_category ON visual_asset(category);
"""
def digest(value):
 return hashlib.sha256(value).hexdigest()
def connect(db):
 db.parent.mkdir(parents=True,exist_ok=True)
 c=sqlite3.connect(db);c.executescript(SCHEMA);return c
def register(plan_file,receipt_file,db=DEFAULT,encoder=None):
 plan=json.loads(pathlib.Path(plan_file).read_text(encoding="utf-8-sig"))
 receipt=json.loads(pathlib.Path(receipt_file).read_text(encoding="utf-8-sig"))
 if plan.get("schema")!="NOWORLD_NATIVE_VISUAL_SWEEP_V1" or receipt.get("schema")!="NOWORLD_NATIVE_VISUAL_SWEEP_RECEIPT_V1":
  raise ValueError("Unrecognized source visual sweep")
 if not receipt.get("baseline_restored"):raise ValueError("Refuse non-restored native benchmark")
 if encoder is None:encoder=emb.LocalEncoder()
 con=connect(pathlib.Path(db))
 made=[];skipped=[]
 try:
  for result in receipt["results"]:
   idx=result["index"];item=plan["candidates"][idx]
   if result["label"]!=item["label"]:raise ValueError("Trial mismatch")
   delt=result.get("visible_delta",{})
   if delt.get("measurably_changed") is not True:
    skipped.append((item["label"],"NO_MEASURABLE_NATIVE_DELTA"));continue
   path=pathlib.Path(result["native_image"])
   if not path.is_file():raise FileNotFoundError(path)
   if path.suffix.lower() not in {".png",".jpg",".jpeg"}:raise ValueError("Not a render image")
   payload=json.dumps(item["steps"],sort_keys=True,separators=(',',':'))
   recipehash=digest((receipt["view"]+payload).encode())
   imgsha=emb._digest(path)
   feat,_=encoder.embed(path)
   if feat.shape!=(3,512):raise RuntimeError("Bad model output")
   # Use canonical model signature, so future ONNX upgrades cannot reuse stale vectors.
   created=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
   con.execute("""INSERT INTO visual_asset(recipe_hash,label,category,trial_steps,
      native_capture,native_capture_sha,model_sha,embedding_fp16,
      native_visible_change,created_utc)
      VALUES (?,?,?,?,?,?,?,?,?,?)
      ON CONFLICT(recipe_hash) DO UPDATE SET
        label=excluded.label,category=excluded.category,trial_steps=excluded.trial_steps,
        native_capture=excluded.native_capture,native_capture_sha=excluded.native_capture_sha,
        model_sha=excluded.model_sha,embedding_fp16=excluded.embedding_fp16,
        native_visible_change=excluded.native_visible_change,created_utc=excluded.created_utc""",
       (recipehash,item["label"],receipt["view"],payload,str(path),imgsha,
        encoder.hash,feat.astype(np.float16).tobytes(),delt["changed_subject_fraction"],created))
   made.append({"label":item["label"],"recipe_hash":recipehash})
  con.commit()
 finally:con.close()
 return {"schema":"NW_ASSET_VISUAL_CACHE_RECEIPT_V1","indexed":made,"skipped":skipped,
   "index_database":str(db),"native_capture_only":True,"production_accepted":False}

def query(reference,kind="face",limit=20,db=DEFAULT,encoder=None):
 if not (1<=limit<=100):raise ValueError("Limit out of bounds")
 if encoder is None:encoder=emb.LocalEncoder()
 v,_=encoder.embed(reference)
 con=connect(pathlib.Path(db))
 try:rows=con.execute("SELECT label,trial_steps,native_capture,model_sha,embedding_fp16,native_visible_change FROM visual_asset WHERE category=?",(kind,)).fetchall()
 finally:con.close()
 result=[]
 for label,recipe,path,model,buffer,change in rows:
  if model!=encoder.hash:continue
  feat=np.frombuffer(buffer,dtype=np.float16)
  if feat.size!=1536:continue
  feat=feat.astype(np.float32).reshape((3,512))
  similarity=emb.sim(v,feat)
  result.append({"label":label,"native_capture":path,
      "recipe":json.loads(recipe),"similarity":similarity,
      "visible_change":change})
 result.sort(key=lambda r:r["similarity"]["weighted_cosine"],reverse=True)
 return {"schema":"NW_KOIKATSU_VISUAL_ASSET_DISCOVERY_V1","reference":str(reference),
   "image_candidates_indexed":len(result),"top":result[:limit],
   "requires_visual_review":True,"paid_calls":0,
   "quality_approved":False}
def main():
 p=argparse.ArgumentParser(description=__doc__);s=p.add_subparsers(dest="cmd",required=True)
 a=s.add_parser("register");a.add_argument("--plan",required=True);a.add_argument("--receipt",required=True)
 a.add_argument("--db",default=str(DEFAULT))
 b=s.add_parser("search");b.add_argument("--reference",required=True);b.add_argument("--view",default="face")
 b.add_argument("--limit",type=int,default=20);b.add_argument("--db",default=str(DEFAULT))
 b.add_argument("--out",required=True)
 args=p.parse_args()
 if args.cmd=="register":
  data=register(args.plan,args.receipt,pathlib.Path(args.db))
 else:
  data=query(args.reference,args.view,args.limit,pathlib.Path(args.db))
  out=pathlib.Path(args.out);out.parent.mkdir(parents=True,exist_ok=True)
  if out.exists():raise FileExistsError(out)
  out.write_text(json.dumps(data,indent=2))
 print(json.dumps(data,indent=2))
if __name__=="__main__":main()
