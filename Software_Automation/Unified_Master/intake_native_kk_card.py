"""Non-destructive private intake for an existing Maker card; no GUI or Steam writes."""
import argparse,datetime,hashlib,json,pathlib,shutil,sys
from PIL import Image

def hash_file(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def ingest(src,out):
 src=pathlib.Path(src).resolve();out=pathlib.Path(out).resolve()
 if not src.is_file() or src.suffix.lower()!='.png':raise ValueError('Existing KK PNG card required')
 if out.exists():raise FileExistsError('Never overwrite intake: '+str(out))
 with Image.open(src) as image:
  if image.format!='PNG' or not image.width>=100:raise ValueError('Invalid PNG card candidate')
  dims=[image.width,image.height]
 before=hash_file(src)
 out.mkdir(parents=True)
 target=out/'SOURCE_CARD_UNMODIFIED.png'
 shutil.copy2(src,target)
 if hash_file(src)!=before or hash_file(target)!=before:raise RuntimeError('Native source card changed')
 record={'schema':'NW_KK_CARD_PRIVATE_INTAKE_V1','source_path':str(src),
 'sha256_original':before,'copy_path':str(target),'card_preview_size':dims,
 'source_unchanged':True,'user_approved_likeness':False,'visual_comparison_pending':True,
 'manual_edit_preserved':True,'no_kk_runtime_mutation':True,
 'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
 'status':'PRIVATE_IMMUTABLE_BASELINE_CAPTURED'}
 (out/'intake_receipt.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
 return record
if __name__=='__main__':
 ap=argparse.ArgumentParser()
 ap.add_argument('--source',required=True);ap.add_argument('--output',required=True)
 args=ap.parse_args()
 print('NW_NATIVE_CARD_INTAKE='+json.dumps(ingest(args.source,args.output)))
