"""Actual saved Maker frame, synthetic source reframing, reversible mock native A/B."""
import json,pathlib,tempfile,shutil,numpy as np
from PIL import Image
import kk_reference_features as features
import kk_registered_gate_optimizer as runner
import kk_benchmark_guard as guard
import kk_native_visual_search as native
from kk_native_feature_alignment import figure_bbox
SRC=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST09_PINK_HAIR_CAT\T09_RESTORED_FIT_front.png')
with tempfile.TemporaryDirectory(prefix='kk_reg_opt_') as td:
 root=pathlib.Path(td)
 with Image.open(SRC) as raw:good=raw.convert('RGB')
 bbox=figure_bbox(good)
 crop=good.crop(bbox)
 rw=540;rh=int(crop.height*rw/crop.width)
 if rh>1376:rh=1376;rw=int(crop.width*rh/crop.height)
 img=Image.new('RGB',(922,1536),'white')
 img.paste(crop.resize((rw,rh),Image.Resampling.LANCZOS),(30,52))
 ref=root/'reference.png';img.save(ref)
 boxes={
  'hair':(.05,.02,.94,.32),'face':(.20,.13,.80,.33),
  'eyes':(.28,.17,.72,.25),'body':(.12,.30,.9,.73),
  'main_outfit':(.10,.36,.91,.77),'sleeves':(.01,.32,.99,.76),
  'legs':(.18,.69,.83,.96),'footwear':(.24,.92,.80,.99)}
 rois={k:[(30+b[0]*rw)/922,(52+b[1]*rh)/1536,
          (30+b[2]*rw)/922,(52+b[3]*rh)/1536] for k,b in boxes.items()}
 m=features.build_manifest(ref,boxes=rois)
 altered=np.array(good).copy()
 x0,y0,x1,y1=bbox
 rx0=int(x0+.13*(x1-x0));rx1=int(x0+.87*(x1-x0))
 ry0=int(y0+.34*(y1-y0));ry1=int(y0+.70*(y1-y0))
 patch=altered[ry0:ry1,rx0:rx1]
 mask=patch.min(axis=2)<240
 patch[mask]=[30,153,214]
 wrong=root/'wrong.png';Image.fromarray(altered).save(wrong)
 goodpath=root/'good.png';good.save(goodpath)
 base={'op':'set_clothes_top','args':{'guid':'mock','category':105,'original_id':0}}
 trial={'op':'set_clothes_top','args':{'guid':'mock','category':105,'original_id':1}}
 plan={'schema':'NOWORLD_NATIVE_VISUAL_SWEEP_V1','reference':str(ref),
       'view':'front','restore_baseline':[base],
       'candidates':[{'label':'blue_mismatch','steps':[base]},
                     {'label':'restored_shirt','steps':[trial]}]}
 old_camera=native.CAMERA;native.CAMERA=root/'camera';native.CAMERA.mkdir()
 state={'top':0};history=[]
 def submit(payload,timeout):
  history.append(payload.copy())
  if payload['op']=='set_clothes_top':state['top']=payload['originalId']
  if payload['op']=='render_camera_view':
   shutil.copyfile({0:wrong,1:goodpath}[state['top']],
       native.CAMERA/f'SM01_CAM_{payload["nonce"]}_front.png')
  print(json.dumps({'status':'RECEIPT_WRITTEN','data':'mock-native'}))
  return 0
 try:
  with guard.scored_native_sweep(plan,m):
   result=runner.run(plan,m,root/'registered',submit=submit,game_guard=False)
 finally:native.CAMERA=old_camera
 sweep=json.loads((root/'registered'/'NATIVE_NONDESTRUCTIVE_SWEEP.json').read_text())
 scores=result['candidates_ranked']
 first=next(r for r in scores if r['label']=='restored_shirt')
 baseline=result['baseline']
 oldscore={x['id']:x['score_0_to_100'] for x in baseline['features']}
 newscore={x['id']:x['score_0_to_100'] for x in first['feature_scorecard']['features']}
 checks={
  'baseline_registers_to_real_native_subject':result['registration']['native_baseline_bbox']==bbox,
  'fixed_native_roi_for_each_candidate':len(result['registration']['candidate_rois'])==8,
  'correct_shirt_improves_main_outfit':newscore['main_outfit']>oldscore['main_outfit']+4,
  'real_Maker_image_underlies_test':SRC.is_file(),
  'full_original_source_geometry_kept':result['registration']['reference_size']==[922,1536],
  'camera_zero_noise_is_valid':sweep['native_noise_measurement_valid'] is True and
      sweep['native_pixel_noise_floor']==0,
  'native_baseline_restored':result['native_state_restored'] and state['top']==0,
  'no_game_card_saved':not any(x['op'].startswith('save') for x in history),
 }
 print('REGISTERED_GATE_NATIVE_MOCK='+json.dumps({
  'passed':sum(checks.values()),'total':len(checks),'checks':checks,
  'baseline_main_outfit':oldscore['main_outfit'],
  'improved_main_outfit':newscore['main_outfit'],
  'native_game_touched':False},sort_keys=True),flush=True)
 if not all(checks.values()):raise SystemExit(2)
