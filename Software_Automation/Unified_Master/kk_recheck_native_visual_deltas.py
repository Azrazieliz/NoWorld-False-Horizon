"""Re-evaluate immutable live Koikatsu screenshot receipts for real visible changes."""
import pathlib,json
import kk_visual_fit as v
root=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA\native_hair_visual_feedback_smoke_20261017')
orig=json.loads((root/'native_sweep_results.json').read_text())
for row in orig['results']:
 row['visible_delta']=v.measure_visible_change(orig['baseline_image'],row['native_image'])
 print('VISIBLE_NATIVE_CHANGE',row['label'],
       row['visible_delta']['changed_subject_fraction'],flush=True)
out=root/'native_sweep_with_visible_delta.json'
if out.exists():raise FileExistsError(out)
out.write_text(json.dumps(orig,indent=2))
