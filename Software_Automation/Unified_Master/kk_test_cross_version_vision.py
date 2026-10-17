"""Read-only cross-version reference ranking: does local CLIP recognize a character
from first-pass face against second-pass face with altered assets?
"""
import pathlib,json
from kk_local_vision_embeddings import rank
ROOT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_FIVE_CHARACTER_BATCH')
CAP=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\BepInEx\SM01_SANDBOX\captures')
cases=['POOL','SCHOOL','ANGEL','THRONE']
refs={}
candidates={}
for i,name in enumerate(cases):
 first=json.loads((ROOT/(f'{i+1:02d}_{name}_RECEIPT.json')).read_text())
 second=json.loads((ROOT/(name+'_V2_PASS_RECEIPT.json')).read_text())
 a=next(x for x in first['operations'] if x['op']=='render_views')
 b=next(x for x in second['operations'] if x['op']=='render_views' and x.get('ok'))
 refs[name]=CAP/('SM01_'+a['nonce']+'_face.png')
 candidates[name]=CAP/('SM01_'+b['nonce']+'_face.png')
 for p in (refs[name],candidates[name]):
  if not p.is_file():raise FileNotFoundError(p)
out=[]
for name in cases:
 report=rank(refs[name],list(candidates.values()))
 ranking=[next((n for n,f in candidates.items() if str(f)==row['candidate']),"?") for row in report['images']]
 out.append({'reference':name,'ranking':ranking,'correct_first':ranking[0]==name,'seconds':report['elapsed_seconds']})
 print('CROSS_VERSION_RETRIEVAL',name,'rank',ranking,'correct_top1',ranking[0]==name,flush=True)
path=ROOT/'LOCAL_CPU_CROSS_VERSION_RETRIEVAL_QA.json'
if path.exists():raise FileExistsError(path)
path.write_text(json.dumps({'trials':out,'matched_top1':sum(v['correct_first'] for v in out),
   'total':len(out),'visual_reconstruction_validated':False},indent=2))
