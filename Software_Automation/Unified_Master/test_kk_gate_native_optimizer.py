"""End-to-end offline QA, creates synthetic reference and Koikatsu-style native renders,
tests full feature scoring+candidate enumeration+state restoration without opening game."""
from __future__ import annotations
import tempfile,json,pathlib,shutil,sys
from PIL import Image,ImageDraw
import kk_reference_features as first
import kk_gate_native_optimizer as gate
import kk_native_visual_search as native

def reference(path,color=(72,23,32),floating=False):
 im=Image.new('RGB',(600,900),'white');d=ImageDraw.Draw(im)
 d.polygon([(240,280),(370,280),(450,720),(180,720)],fill=color)
 d.polygon([(270,630),(310,630),(299,861),(225,866)],fill=(44,40,51))
 d.polygon([(311,630),(360,630),(400,866),(307,866)],fill=(44,40,51))
 shift=-145 if floating else 0
 d.ellipse((250,55+shift,376,332+shift),fill=(225,175,60))
 d.ellipse((263,150+shift,354,258+shift),fill=(230,190,170))
 for x in (284,330):d.ellipse((x,187+shift,x+14,195+shift),fill=(140,49,70))
 d.polygon([(265,130+shift),(301,92+shift),(352,126+shift),(335,187+shift),(291,172+shift)],fill=(225,175,60))
 im.save(path)

with tempfile.TemporaryDirectory() as td:
 root=pathlib.Path(td);ref=root/'ref.png';bad=root/'wrong_robe.png';good=root/'correct.png';floating=root/'flying_hair.png'
 reference(ref);reference(good);reference(bad,(11,129,174));reference(floating,floating=True)
 features=first.build_manifest(ref,boxes={x[0]:list(x[2]) for x in first.TEMPLATE},view='front')
 base={"op":"set_clothes_top","args":{"guid":"fixture","category":105,"original_id":0}}
 plan={"schema":"NOWORLD_NATIVE_VISUAL_SWEEP_V1","reference":str(ref),"view":"front",
  "restore_baseline":[base],
  "candidates":[
    {"label":"wrong_robe","steps":[{"op":"set_clothes_top","args":{"guid":"fixture","category":105,"original_id":0}}]},
    {"label":"corrected_robe","steps":[{"op":"set_clothes_top","args":{"guid":"fixture","category":105,"original_id":1}}]},
    {"label":"floating_hair","steps":[{"op":"set_clothes_top","args":{"guid":"fixture","category":105,"original_id":2}}]}]}
 old_cam=native.CAMERA;native.CAMERA=root/'camera';native.CAMERA.mkdir()
 state={'id':0};history=[]
 def submit(message,timeout):
  history.append(message.copy())
  if message['op']=='set_clothes_top':state['id']=message['originalId']
  if message['op']=='render_camera_view':
   path=native.CAMERA/('SM01_CAM_'+message['nonce']+'_front.png')
   shutil.copyfile({0:bad,1:good,2:floating}[state['id']],path)
  print(json.dumps({"status":"RECEIPT_WRITTEN","data":"mock"}))
  return 0
 results=gate.run(plan,features,root/'proof',submit=submit,game_guard=False)
 best=results["candidates_ranked"][0]
 checks={
  "all_first_program_features_scored":results["feature_count"]==8,
  "best_candidate_has_valid_robe":best["label"]=="corrected_robe",
  "baseline_restored_after_trials":state["id"]==0 and results["native_state_restored"],
  "all_user_regions_verified":all(x["feature_identity_verified"] for x in features["features"]),
  "no_original_card_save":not any(x["op"]=="save_derived_card" for x in history),
  "no_premature_art_approval":not results["production_character_accepted"] and not results["all_gates_proven"],
  "per_feature_qa_crops_created":len(list((root/'proof'/'BEST_FEATURE_CROPS').glob('*.png')))==8,
  "wrong_outfit_lower_gate_score":next(x["score_0_to_100"] for x in results["baseline"]["features"] if x["id"]=="main_outfit") <
   next(x["score_0_to_100"] for x in best["feature_scorecard"]["features"] if x["id"]=="main_outfit")-5,
  "best_candidate_requires_human_review":results["selected_for_human_inspection"]=="corrected_robe",
  "review_requires_real_visible_delta":best["visible_native_change_verified"] is True,
 }
 native.CAMERA=old_cam
 # A real localized incremental gain remains reviewable while its hard gate fails.
 b={"features":[{"id":"hair","critical":True,"feature_identity_verified":True,
      "score_0_to_100":40.},{"id":"face","critical":True,"feature_identity_verified":True,
      "score_0_to_100":80.}],"blocked_critical_gates":["hair"]}
 g={"features":[{"id":"hair","critical":True,"feature_identity_verified":True,
      "score_0_to_100":47.},{"id":"face","critical":True,"feature_identity_verified":True,
      "score_0_to_100":80.}],"blocked_critical_gates":["hair"],"weighted_metric_score_0_to_100":63.5}
 trial={"label":"incremental","image":"mock","gates":g,"verified_delta":True}
 partial=gate.select_compare(b,[trial])[0]
 checks["visible_incremental_gain_reviewable_before_threshold"]=partial["eligible_for_user_review"]
 trial["verified_delta"]=False
 checks["unseen_gain_never_promoted"]=not gate.select_compare(b,[trial])[0]["eligible_for_user_review"]
 print("FEATURE_NATIVE_OPTIMIZER_QA="+json.dumps({"passed":sum(checks.values()),"total":len(checks),"checks":checks,"best":best["label"],"baseline_score":results["baseline"]["weighted_metric_score_0_to_100"],"best_score":best["score"],"paid_calls":0,"native_game_edited":False},sort_keys=True))
 if not all(checks.values()):raise SystemExit(2)
