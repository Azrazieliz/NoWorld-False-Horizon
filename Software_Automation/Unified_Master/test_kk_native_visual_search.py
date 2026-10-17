"""No-game regression for safeguarded Koikatsu visual candidate sweep."""
import json,pathlib,tempfile
from PIL import Image
import kk_native_visual_search as sweep
tests={}
def check(ok,msg):
 if not ok:raise AssertionError(msg)
def test(name,fn):
 try:fn();tests[name]="PASS"
 except Exception as e:tests[name]="FAIL "+str(e)
def rejected(fn):
 try:fn()
 except (ValueError,RuntimeError,FileExistsError):return True
 return False

with tempfile.TemporaryDirectory() as td:
 root=pathlib.Path(td)
 ref=root/"ref.png";Image.new("RGB",(512,768),"white").save(ref)
 base={"op":"set_hair_palette","args":{"index":0,"value":.5,"v2":.5,"v3":.5}}
 plan={"schema":"NOWORLD_NATIVE_VISUAL_SWEEP_V1","reference":str(ref),"view":"face",
       "restore_baseline":[base],
       "candidates":[{"label":"bad","steps":[{"op":"set_hair_palette","args":{"index":0,"value":.1,"v2":.1,"v3":.1}}]},
          {"label":"good","steps":[{"op":"set_hair_palette","args":{"index":0,"value":.9,"v2":.8,"v3":.4}}]}]}
 f=root/"plan.json";f.write_text(json.dumps(plan))
 test("valid_source_plan",lambda:check(sweep.parse_spec(f)["view"]=="face","Bad plan"))
 invalid=dict(plan);invalid["restore_baseline"]=[]
 j=root/"invalid.json";j.write_text(json.dumps(invalid))
 test("baseline_restore_mandatory",lambda:check(rejected(lambda:sweep.parse_spec(j)),"No restore accepted"))
 unsafe=dict(plan);unsafe["candidates"]=[{"label":"unsafe","steps":[{"op":"save_derived_card","args":{}}]}]
 j=root/"unsafe.json";j.write_text(json.dumps(unsafe))
 test("never_save_character_card",lambda:check(rejected(lambda:sweep.parse_spec(j)),"Unsafe save accepted"))
 old_camera=sweep.CAMERA
 sweep.CAMERA=root/"cam";sweep.CAMERA.mkdir()
 history=[]
 def submit(payload,seconds):
  history.append(payload.copy())
  if payload["op"]=="render_camera_view":
   Image.new("RGB",(512,768),"white").save(sweep.CAMERA/("SM01_CAM_"+payload["nonce"]+"_face.png"))
  print(json.dumps({"status":"RECEIPT_WRITTEN","data":"mock"}))
  return 0
 def scorer(ref,candidate,reference_roi=None,candidate_roi=None):
  idx=sum(x["op"]=="render_camera_view" for x in history)
  return {"score_0_to_100":{2:50.,3:30.,4:73.}[idx],
          "components":{"reference_mismatch":idx}}
 def real_protocol():
  receipt=sweep.run(plan,root/"receipt.json",submit=submit,game_guard=False,score=scorer)
  check(receipt["baseline_restored"] and receipt["best"]["label"]=="good","Wrong best")
  check(receipt["baseline_score"]==50 and receipt["best"]["score"]==73,"Wrong numeric scores")
  check(sum(x["op"]=="render_camera_view" for x in history)==4,"Wrong camera count")
  check(history[-1]["op"]=="set_hair_palette" and history[-1]["value"]==.5,"Not restored")
  check(not any(x["op"]=="save_derived_card" for x in history),"Saved card")
  check(receipt["baseline_noise_calibrated"] is False,"Blank reference was considered calibrated")
  check(receipt["recommendation"]=="KEEP_BASELINE_NO_VERIFIED_VISIBLE_IMPROVEMENT",
    "Score-only winner was recommended without actual pixel change")
 test("mocked_3_native_comparisons_restore",real_protocol)
 test("immutable_receipt",lambda:check(rejected(lambda:
    sweep.run(plan,root/"receipt.json",submit=submit,game_guard=False,score=scorer)),
    "Overwrite accepted"))
 sweep.CAMERA=old_camera
 print("KK_VISUAL_SWEEP_REGRESSIONS="+json.dumps({
  "passed":sum(x=="PASS" for x in tests.values()),"total":len(tests),
  "tests":tests,"native_game_edited":False,"paid_api_calls":0},sort_keys=True))
 if any(x!="PASS" for x in tests.values()):raise SystemExit(2)
