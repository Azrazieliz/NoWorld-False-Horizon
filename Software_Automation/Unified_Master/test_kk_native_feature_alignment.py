"""Offline actual Maker screenshot regression: catch empty-screen ROI comparisons."""
import json,tempfile,pathlib,numpy as np
from PIL import Image
import kk_reference_features as f
import kk_feature_gates as gate
from kk_native_feature_alignment import figure_bbox,register_boxes
SOURCE=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST09_PINK_HAIR_CAT\T09_RESTORED_FIT_front.png')
def check(condition,text):
 if not condition:raise AssertionError(text)
with tempfile.TemporaryDirectory(prefix='kk_native_roi_') as root:
 root=pathlib.Path(root)
 with Image.open(SOURCE) as im:original=im.convert('RGB')
 bbox=figure_bbox(original)
 crop=original.crop(bbox)
 # Same REAL source figure placed at a dramatically different framing/location.
 # This detects comparing normalized full-frame pixels rather than character
 # semantics at a fixed baseline native camera ROI.
 W,H=922,1536
 rw=540;rh=int(crop.height*rw/crop.width)
 if rh>H-160:rh=H-160;rw=int(crop.width*rh/crop.height)
 image=Image.new('RGB',(W,H),'white')
 srcbox=(40,55,40+rw,55+rh)
 image.paste(crop.resize((rw,rh),Image.Resampling.LANCZOS),srcbox[:2])
 ref=root/'registered_reference.png';image.save(ref)
 # Four actual anatomical subsections grounded to transformed figure geometry.
 # Prevent synthetic test from passing via background-only coincidence.
 fractions={
  'hair':(.06,.00,.94,.32),'face':(.20,.12,.79,.33),
  'eyes':(.26,.17,.75,.24),'body':(.13,.29,.86,.69),
  'main_outfit':(.10,.33,.9,.77),
  'sleeves':(.00,.33,1.,.69),'legs':(.2,.67,.8,.99),
  'footwear':(.24,.90,.76,1.)}
 coords={}
 for key,v in fractions.items():
  coords[key]=[(srcbox[0]+v[0]*rw)/W,(srcbox[1]+v[1]*rh)/H,
               (srcbox[0]+v[2]*rw)/W,(srcbox[1]+v[3]*rh)/H]
 manifest=f.build_manifest(ref,boxes=coords)
 same=gate.score_features(manifest,SOURCE)
 registration=register_boxes(manifest,SOURCE)
 fixed=gate.score_features(manifest,SOURCE,rois=registration['candidate_rois'])
 a={x['id']:x for x in same['features']}
 b={x['id']:x for x in fixed['features']}
 empty_baseline=sum(a[x]['components']['status']!='SCORED' for x in ['face','eyes','main_outfit'])
 # Flip an actual native character's top garment pixels, not background.
 raw=np.asarray(original).copy()
 x0,y0,x1,y1=registration['candidate_rois']['main_outfit']
 crop_rgb=raw[y0:y1,x0:x1]
 visible=(crop_rgb.max(axis=2)<240)
 crop_rgb[visible]=[42,163,212]
 mutated=root/'wrong_top.png'
 Image.fromarray(raw).save(mutated)
 bad=gate.score_features(manifest,mutated,rois=registration['candidate_rois'])
 c={x['id']:x for x in bad['features']}
 checks={
  'real_native_render_used':SOURCE.is_file(),
  'reference_reframing_detected':registration['reference_bbox']!=registration['native_baseline_bbox'],
  'candidate_head_in_actual_character':registration['candidate_rois']['face'][0]>=bbox[0]-50,
  'registered_selfscore_exceeds_unregistered_by_15':fixed['weighted_metric_score_0_to_100']>same['weighted_metric_score_0_to_100']+15,
  'registered_main_outfit_correct_before_change':b['main_outfit']['score_0_to_100']>a['main_outfit']['score_0_to_100']+10,
  'wrong_top_lowers_its_own_feature':c['main_outfit']['score_0_to_100']<b['main_outfit']['score_0_to_100']-4,
  'fixed_roi_anchored_not_variant_recentred':registration['anchored_to_baseline'],
  'does_not_claim_semantic_pose_detection':not registration['pose_normalization_verified'],
 }
 print('NATIVE_ROI_ALIGNMENT='+json.dumps({
  'passed':sum(bool(x) for x in checks.values()),'total':len(checks),
  'checks':checks,'native_figure_bbox':bbox,
  'raw_global_score':same['weighted_metric_score_0_to_100'],
  'registered_score':fixed['weighted_metric_score_0_to_100'],
  'main_outfit_before':b['main_outfit']['score_0_to_100'],
  'main_outfit_after_wrong_recolor':c['main_outfit']['score_0_to_100'],
  'native_game_touched':False},sort_keys=True),flush=True)
 if not all(checks.values()):raise SystemExit(2)
