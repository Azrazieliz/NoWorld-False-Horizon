#!/usr/bin/env python3
"""P0 reference feature manifest from LOCAL full-resolution image.

Anatomical region proposals are geometric priors, not a trained semantic detector.
Every proposed feature has a score gate; unsupported anatomy/pose is never silently
assumed. A user/artist can annotate/adjust boxes once in JSON. If the art shows a
seated/sidelong/fantasy creature, supply custom boxes/view and mark unobserved
features absent; never hallucinate a character's hidden feet or back.

The manifest is the versioned interface between reference interpretation,
native Koikatsu candidate selection, and per-feature scoring.
"""
import argparse,hashlib,json,pathlib,math
import numpy as np
from PIL import Image

TEMPLATE=[
 ("hair","Hair silhouette and color",(.25,.012,.78,.36),78,True,"head"),
 ("face","Face, chin and expression",(.37,.125,.66,.37),82,True,"face"),
 ("eyes","Eye color, shape and position",(.40,.185,.62,.28),85,True,"face"),
 ("body","Torso and body proportions",(.22,.30,.77,.75),80,True,"front"),
 ("main_outfit","Primary garment silhouette",(.20,.30,.80,.82),82,True,"front"),
 ("sleeves","Sleeves and arm outlines",(.10,.30,.89,.70),70,False,"front"),
 ("legs","Lower garment and leg proportions",(.26,.67,.75,.955),77,False,"front"),
 ("footwear","Shoes, boots and lower leg finish",(.28,.84,.73,.995),70,False,"front"),
]
# Relative standard template intentionally conservative. Positions are rough:
# these labels are NOT evidence that a detector has found an exact feature.
WEIGHTS=dict(hair=1.45,face=1.75,eyes=1.35,body=1.5,main_outfit=1.45,
             sleeves=.80,legs=1.,footwear=.7)

def digest(p):
 h=hashlib.sha256()
 with pathlib.Path(p).open("rb") as f:
  for buf in iter(lambda:f.read(1<<20),b""):h.update(buf)
 return h.hexdigest()

def load_boxes(path):
 text=pathlib.Path(path).read_text(encoding="utf-8-sig")
 obj=json.loads(text)
 if not isinstance(obj,dict) or any(not isinstance(k,str) for k in obj):
  raise ValueError("Custom features must map semantic IDs to boxes")
 for key,val in obj.items():
  if val is not None and (not isinstance(val,list) or len(val)!=4):
   raise ValueError("Custom feature rectangle must be [x1,y1,x2,y2] normalized")
 return obj

def build_manifest(reference,mode="humanoid_front",extra_features=None,
                   boxes=None,view="front",strict_background=False):
 image=pathlib.Path(reference)
 if not image.is_file():raise FileNotFoundError(reference)
 with Image.open(image) as f:
  w,h=f.size
  if min(w,h)<384:
   raise ValueError("Reference shorter side under 384px; use original upload, never preview thumbnail")
  a=np.array(f.convert("RGB"),dtype=np.float32)/255
 # Character foreground is only inferred for reliable almost-white background;
 # no claim of segmentation when unknown scene/camera.
 corners=np.concatenate([a[:24,:24].reshape(-1,3),a[:24,-24:].reshape(-1,3),
         a[-24:,:24].reshape(-1,3),a[-24:,-24:].reshape(-1,3)])
 white_conf=float(np.clip(1-(np.abs(corners-1).mean()/.13),0,1))
 if strict_background and white_conf<.60:raise ValueError("Uncontrolled artistic background: add subject region/mask")
 # Override boxes coordinates with user-guided estimates to avoid false confidence.
 boxes=boxes or {}
 features=[]
 for key,label,rect,limit,mandatory,camera in TEMPLATE:
  if key in boxes and boxes[key] is None:continue
  b=boxes.get(key,rect)
  if any(not isinstance(x,(int,float)) for x in b) or not 0<=b[0]<b[2]<=1 or not 0<=b[1]<b[3]<=1:
   raise ValueError("Invalid normalized reference region "+key)
  x1=max(0,int(b[0]*w));y1=max(0,int(b[1]*h))
  x2=min(w,max(x1+1,int(b[2]*w)));y2=min(h,max(y1+1,int(b[3]*h)))
  patch=a[y1:y2,x1:x2]
  if patch.size<48:raise ValueError("Feature patch too small")
  # Observable, measurable, non-semantic local description.
  light=np.mean(patch,axis=2)
  chroma=np.max(patch,axis=2)-np.min(patch,axis=2)
  rgb=np.median(patch.reshape(-1,3),axis=0)
  features.append({
   "id":key,"label":label,"reference_roi":[x1,y1,x2,y2],
   "candidate_roi_norm":list(b),"view":view if camera in ("front","head","face") else camera,
   "source_method":"USER_GROUNDED_ROI" if key in boxes else "GEOMETRIC_HUMANOID_PRIOR",
   "feature_identity_verified":key in boxes,
   "presence":"USER_SPECIFIED" if key in boxes else "UNVERIFIED_PROPOSAL",
   "critical":mandatory,
   "threshold_0_to_100":float(limit),
   "weight":WEIGHTS[key],
   "observed_color_rgb":[round(float(x),3) for x in rgb],
   "chroma_fraction":round(float((chroma>.1).mean()),4),
   "dark_fraction":round(float((light<.36).mean()),4),
   "low_detail_flag":bool(np.var(light)<.005),
   "note":"Score is local image evidence, never a semantic likeness score."})
 if extra_features:
  for name,rect in extra_features.items():
   if name in {f["id"] for f in features}:raise ValueError("Duplicate feature "+name)
   if not isinstance(name,str) or not 2<=len(name)<=70:raise ValueError("Bad feature identifier")
   if not isinstance(rect,(tuple,list)) or len(rect)!=4:raise ValueError("Bad extra feature box")
   if not 0<=rect[0]<rect[2]<=1 or not 0<=rect[1]<rect[3]<=1:
    raise ValueError("Custom feature requires normalized coordinates")
   features.append({"id":name,"label":name,"reference_roi":[int(rect[0]*w),int(rect[1]*h),int(rect[2]*w),int(rect[3]*h)],
       "candidate_roi_norm":list(rect),"view":view,"source_method":"USER_GROUNDED_ROI",
       "feature_identity_verified":True,"presence":"USER_SPECIFIED",
       "critical":True,"threshold_0_to_100":26.0,"weight":1.3,
       "note":"Custom feature; user-selected ROI needed for nonstandard geometry."})
 return {"schema":"NW_KK_REFERENCE_FEATURES_V1","reference_path":str(image),
     "reference_sha256":digest(image),"reference_resolution":[w,h],
     "reference_pose_mode":mode,"reference_view":view,
     "source_background":"WHITE_LIKELY" if white_conf>=.60 else "UNSEGMENTED_COMPLEX_BACKGROUND",
     "source_background_confidence":round(white_conf,3),
     "features":features,"feature_count":len(features),
     "feature_detection_status":"GEOMETRIC_PROPOSALS_NOT_SEMANTIC_DETECTION",
     "candidate_visual_acceptance":False,
     "warning":"No model has localized anime anatomical landmarks or recognized identity. Verify all ROIs before production."}

def main():
 p=argparse.ArgumentParser()
 p.add_argument("--reference",required=True);p.add_argument("--out",required=True)
 p.add_argument("--roi-overrides",help="JSON object of normalized boxes or null for absent features")
 p.add_argument("--pose",default="humanoid_front");p.add_argument("--view",default="front")
 a=p.parse_args()
 overrides=load_boxes(a.roi_overrides) if a.roi_overrides else {}
 known={k:overrides.pop(k) for k in list(overrides) if k in {f[0] for f in TEMPLATE}}
 m=build_manifest(a.reference,a.pose,extra_features=overrides,boxes=known,view=a.view)
 dest=pathlib.Path(a.out);dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists():raise FileExistsError(dest)
 dest.write_text(json.dumps(m,indent=2),encoding="utf-8")
 print("REFERENCE_FEATURES="+json.dumps({"count":m["feature_count"],"ids":[f["id"] for f in m["features"]],
   "verified_boxes":sum(f["feature_identity_verified"] for f in m["features"]),
   "background":m["source_background"]}))
if __name__=="__main__":main()
