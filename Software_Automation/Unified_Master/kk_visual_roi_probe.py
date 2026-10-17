"""Read-only diagnostic: whole-face score vs area-of-change score."""
import sys,pathlib,json
import kk_visual_fit as v
root=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA\native_hair_visual_feedback_smoke_20261017')
report=json.loads((root/'native_sweep_results.json').read_text())
ref=report['baseline_image']
regions={
 'whole_face':(0,0,768,1024),
 'hair_contour':(130,400,660,790),
 'bangs':(165,430,600,685),
 'side_hair':(115,560,665,850),
 'bottom_hair':(165,650,640,840)}
for label,roi in regions.items():
 try:
  rows=[]
  for x in report['results']:
   score=v.evaluate(ref,x['native_image'],roi,roi,require_white=True)
   rows.append((x['label'],score['score_0_to_100']))
  print('ROI',label,'ranked',sorted(rows,key=lambda r:r[1],reverse=True),flush=True)
 except Exception as e:print('ROI_FAIL',label,str(e)[:160],flush=True)
