"""Offline regression for mandatory visual gates. Does not touch Koikatsu."""
import tempfile,pathlib,json
from PIL import Image,ImageDraw
import kk_reference_features as detect
import kk_feature_gates as gate

def make(path,top=(72,23,32),hair=(225,175,60),face=(230,190,170),
         floating=False,missing_hair=False):
 im=Image.new("RGB",(600,900),"white");d=ImageDraw.Draw(im)
 d.polygon([(240,280),(370,280),(450,720),(180,720)],fill=top)
 d.polygon([(270,630),(310,630),(299,861),(225,866)],fill=(44,40,51))
 d.polygon([(311,630),(360,630),(400,866),(307,866)],fill=(44,40,51))
 if not missing_hair:
  offset=-140 if floating else 0
  d.ellipse((250,55+offset,376,332+offset),fill=hair)
  d.ellipse((263,150+offset,354,258+offset),fill=face)
  for x in [284,330]:
   d.ellipse((x,187+offset,x+14,195+offset),fill=(140,49,70))
  d.polygon([(265,130+offset),(301,92+offset),(352,126+offset),(335,187+offset),(291,172+offset)],fill=hair)
 im.save(path)

with tempfile.TemporaryDirectory() as td:
 r=pathlib.Path(td);ref=r/'ref.png';same=r/'same.png';badrobe=r/'robe.png'
 flying=r/'floating.png';headless=r/'without_hair.png'
 make(ref);make(same);make(badrobe,top=(11,129,174));make(flying,floating=True)
 make(headless,missing_hair=True)
 boxes={k:list(t[2]) for k,*t in []}
 # Every standard feature region is explicitly grounded for the controlled synthetic data.
 override={row[0]:list(row[2]) for row in detect.TEMPLATE}
 baseline=detect.build_manifest(ref,boxes=override)
 raw=detect.build_manifest(ref)
 evidence=gate.score_features(baseline,same)
 proposal=gate.score_features(raw,same)
 color=gate.score_features(baseline,badrobe)
 hair=gate.score_features(baseline,flying)
 missing=gate.score_features(baseline,headless)
 a={x["id"]:x for x in evidence["features"]}
 b={x["id"]:x for x in color["features"]}
 c={x["id"]:x for x in hair["features"]}
 d={x["id"]:x for x in missing["features"]}
 checks={
  "eight_separate_features":len(a)==8,
  "verified_reference_self_passes":evidence["all_critical_metric_gates_pass"],
  "raw_prior_never_claims_identity":not proposal["all_critical_metric_gates_pass"],
  "wrong_robe_main_outfit_detected":b["main_outfit"]["score_0_to_100"]<a["main_outfit"]["score_0_to_100"]-5,
  "floating_hair_affects_hair_gate":c["hair"]["score_0_to_100"]<a["hair"]["score_0_to_100"]-4,
  "missing_hair_blocks_feature":d["hair"]["score_0_to_100"]<a["hair"]["score_0_to_100"]-4,
  "no_auto_artistic_approval":all(not q["artistic_visual_fidelity_accepted"] for q in (evidence,proposal,color,hair,missing)),
  "reference_provenance_verified":baseline["reference_sha256"]==detect.digest(ref),
  "custom_extra_horns_allowed":"horns" in [x["id"] for x in detect.build_manifest(ref,boxes=override,
       extra_features={"horns":[.17,.01,.84,.3]})["features"]],
 }
 print("FEATURE_GATE_REGRESSIONS="+json.dumps({"passed":sum(checks.values()),
       "total":len(checks),"checks":checks,
       "self":[(x["id"],x["score_0_to_100"]) for x in evidence["features"]],
       "wrong_robe":[(x["id"],x["score_0_to_100"]) for x in color["features"]],
       "floating_hair":[(x["id"],x["score_0_to_100"]) for x in hair["features"]],
       "native_game_touched":False},sort_keys=True),flush=True)
 if not all(checks.values()):raise SystemExit(2)
