"""Offline guard validation. No game edits."""
import sys,json,tempfile,pathlib
from PIL import Image
import kk_native_visual_search as native
import kk_benchmark_guard as guard
import kk_reference_features as features
def refuses(fn):
 try:fn()
 except (RuntimeError,ValueError):return True
 return False
checks={}
with tempfile.TemporaryDirectory() as td:
 root=pathlib.Path(td)
 ref=root/'source.png'
 Image.new('RGB',(640,900),'white').save(ref)
 box={x[0]:list(x[2]) for x in features.TEMPLATE}
 manifest=features.build_manifest(ref,boxes=box,view='front')
 original={'op':'set_hair_palette','args':{'index':0,'value':.5,'v2':.4,'v3':.3}}
 trial={'op':'set_hair_palette','args':{'index':0,'value':.7,'v2':.4,'v3':.3}}
 plan={'reference':str(ref),'restore_baseline':[original],
       'candidates':[{'label':'change','steps':[trial]}]}
 checks['test09_direct_edit_blocked']=refuses(lambda:guard.require_permitted_step(trial,'test09_fit.py'))
 checks['test10_direct_edit_blocked']=refuses(lambda:guard.require_permitted_step(trial,'test10_fit.py'))
 checks['test06_backward_compatible']=not guard.script_requires_gate('test06_fit.py')
 checks['read_only_camera_allowed']=guard.require_permitted_step(
   {'op':'render_camera_view','args':{'group':'face'}},'test09_fit.py') is None
 checks['card_saving_prohibited']=refuses(lambda:guard.require_permitted_step(
   {'op':'save_derived_card','args':{}},'test09_fit.py'))
 with guard.scored_native_sweep(plan,manifest):
  guard.require_permitted_step(trial,'test09_fit.py')
  checks['authorized_step_allowed']=True
  checks['out_of_plan_step_blocked']=refuses(lambda:guard.require_permitted_step(
    {'op':'set_hair_palette','args':{'index':0,'value':.95,'v2':.1,'v3':.1}},'test09_fit.py'))
 checks['authorization_expires']=refuses(lambda:guard.require_permitted_step(trial,'test09_fit.py'))
 altered=dict(manifest,reference_sha256='wrong')
 checks['bad_reference_hash_blocked']=refuses(lambda:guard.authorize_plan(plan,altered))
 altered=dict(manifest,features=[dict(x,feature_identity_verified=False) for x in manifest['features']])
 checks['unverified_feature_boxes_blocked']=refuses(lambda:guard.authorize_plan(plan,altered))
 print('NATIVE_GUARD_CHECKS='+json.dumps(checks,sort_keys=True))
 if not all(checks.values()):raise SystemExit(2)
