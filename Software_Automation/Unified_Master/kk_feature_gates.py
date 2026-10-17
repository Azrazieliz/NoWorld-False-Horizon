#!/usr/bin/env python3
"""Multi-feature scorecard and fail-closed gates for authentic Koikatsu renders.

Each FIRST-PROGRAM feature receives independent normalized 0..100 evidence score,
its own gate, status, and visual crop. Unlike whole-frame image similarity, a good
background, skin color or garment cannot cancel a failed face/hair/accessory gate.

This is a local geometric/texture proxy, NOT TRELLIS semantic reconstruction.
No card is promoted on this score without reference/ROI verification and user
acceptance, even when every numerical gate passes.
"""
from __future__ import annotations
import argparse,hashlib,json,pathlib,math
import numpy as np
from PIL import Image,ImageDraw
from scipy.ndimage import sobel,gaussian_filter,distance_transform_edt
import kk_visual_fit as v
import kk_reference_features as f

def norm_box(box,width,height):
 if len(box)!=4 or not 0<=box[0]<box[2]<=1 or not 0<=box[1]<box[3]<=1:
  raise ValueError("Malformed normalized ROI")
 return (int(box[0]*width),int(box[1]*height),int(box[2]*width),int(box[3]*height))

def normalized_image(image,w=192,h=192):
 obj=Image.fromarray(np.uint8(np.clip(image*255,0,255)))
 return np.asarray(obj.resize((w,h),Image.Resampling.BICUBIC),dtype=np.float32)/255

def local_foreground(a,mode="white"):
 if mode=="white":
  # Faster and reproducible white-bg fg map, preserving nearly-white hair texture.
  # .93 cutoff controls global pad but exact crystal/white hair still difficult;
  # report source reliability separately.
  return np.linalg.norm(a-1,axis=2)>.13
 return np.ones(a.shape[:2],dtype=np.bool_)

def flat_feature_score(ref_patch,cand_patch,white_background=True):
 if min(*ref_patch.shape[:2],*cand_patch.shape[:2])<16:
  raise ValueError("Feature ROI too narrow to compare")
 a=normalized_image(ref_patch);b=normalized_image(cand_patch)
 srcmask=local_foreground(a,"white" if white_background else "full")
 dstmask=local_foreground(b,"white")
 if srcmask.sum()<70:return {"status":"NO_VISIBLE_REFERENCE_FEATURE","score":0.,
   "silhouette":0.,"palette":0.,"structure":0.,"location":0.}
 if dstmask.sum()<70:return {"status":"NO_VISIBLE_CANDIDATE_FEATURE","score":0.,
   "silhouette":0.,"palette":0.,"structure":0.,"location":0.}
 # Shape in the fixed ROI (do NOT crop center or it hides floating hair/capes).
 overlap=float((srcmask&dstmask).sum()/max(1,(srcmask|dstmask).sum()))
 src_coverage=srcmask.sum()/(192*192)
 dst_coverage=dstmask.sum()/(192*192)
 coverage=float(math.exp(-2*abs(src_coverage-dst_coverage)))
 pal=float(v.hist_similarity(v.rgb_lab(a),v.rgb_lab(b),srcmask,dstmask))
 grad_a=np.mean(a,axis=2);grad_b=np.mean(b,axis=2)
 grad_a=gaussian_filter(grad_a,.9);grad_b=gaussian_filter(grad_b,.9)
 mag_a=np.hypot(sobel(grad_a,axis=0),sobel(grad_a,axis=1))
 mag_b=np.hypot(sobel(grad_b,axis=0),sobel(grad_b,axis=1))
 ea=(mag_a>max(.06,float(np.percentile(mag_a[srcmask],72))))&srcmask
 eb=(mag_b>max(.06,float(np.percentile(mag_b[dstmask],72))))&dstmask
 structural=float(v.edge_chamfer(ea,eb)) if ea.sum()>6 and eb.sum()>6 else 0.
 def centroid(mask):
  y,x=np.nonzero(mask)
  if not len(x):return None
  return (np.mean(x)/192,np.mean(y)/192)
 ca=centroid(srcmask);cb=centroid(dstmask)
 location=math.exp(-5*math.hypot(ca[0]-cb[0],ca[1]-cb[1])) if ca and cb else 0.
 color_dist=np.linalg.norm(v.rgb_lab(a)-v.rgb_lab(b),axis=2)
 overlap_mask=srcmask&dstmask
 pixel_color=math.exp(-np.mean(np.clip(color_dist[overlap_mask],0,105))/48) if overlap_mask.any() else 0.
 score=float(np.clip((.19*overlap+.12*coverage+.22*pal+.22*structural+.17*location+.08*pixel_color)*100,0,100))
 return {"status":"SCORED","score":round(score,2),
   "silhouette":round(overlap*100,2),"coverage":round(coverage*100,2),
   "palette":round(pal*100,2),"structure":round(structural*100,2),
   "location":round(location*100,2),"aligned_color":round(pixel_color*100,2)}

def score_features(manifest,candidate,report_dir=None,rois=None,verified_only=False):
 if manifest.get("schema")!="NW_KK_REFERENCE_FEATURES_V1":raise ValueError("Invalid feature source manifest")
 original=pathlib.Path(manifest["reference_path"])
 if f.digest(original)!=manifest.get("reference_sha256"):
  raise RuntimeError("Reference image changed since feature detection")
 for d in ("reference_resolution","features"):
  if d not in manifest:raise ValueError("Missing essential manifest field "+d)
 with Image.open(original) as img:
  ref=img.convert("RGB")
 with Image.open(candidate) as img:
  cand=img.convert("RGB")
 if list(ref.size)!=manifest["reference_resolution"]:
  raise RuntimeError("Reference dimensions do not match source manifest")
 props=[];weighted=0.;denominator=0.;all_gates=True
 if report_dir:
  report_dir=pathlib.Path(report_dir)
  report_dir.mkdir(exist_ok=True,parents=True)
 for i,feat in enumerate(manifest["features"]):
  fid=feat["id"]
  box=feat["reference_roi"]
  if not all(isinstance(x,int) for x in box) or not(0<=box[0]<box[2]<=ref.width and 0<=box[1]<box[3]<=ref.height):
   raise ValueError("Bad reference patch for "+fid)
  candidate_box=(rois or {}).get(fid)
  if candidate_box is None:candidate_box=norm_box(feat["candidate_roi_norm"],cand.width,cand.height)
  elif all(isinstance(x,(int,float)) for x in candidate_box):
   if all(0<=x<=1 for x in candidate_box):
    candidate_box=norm_box(candidate_box,cand.width,cand.height)
  if not all(isinstance(x,int) for x in candidate_box) or not(0<=candidate_box[0]<candidate_box[2]<=cand.width and 0<=candidate_box[1]<candidate_box[3]<=cand.height):
   raise ValueError("Bad candidate region for "+fid)
  src=np.asarray(ref.crop(box),dtype=np.float32)/255
  dst=np.asarray(cand.crop(candidate_box),dtype=np.float32)/255
  metrics=flat_feature_score(src,dst,white_background=manifest["source_background"]=="WHITE_LIKELY")
  threshold=float(feat["threshold_0_to_100"])
  mandatory=bool(feat["critical"])
  grounded=bool(feat.get("feature_identity_verified"))
  image_score=metrics["score"]
  below=image_score<threshold
  # Approximate source features cannot be silently called detected.
  status="BLOCK_UNVERIFIED_FEATURE_ROI" if not grounded else (
       "BLOCK_MISSING_FEATURE" if metrics["status"]!="SCORED" else (
       "BLOCK_FEATURE_THRESHOLD" if below else "METRIC_GATE_PASS"))
  if manifest["source_background"]!="WHITE_LIKELY":
   status="BLOCK_BACKGROUND_SEGMENTATION_REQUIRED"
  if mandatory and status!="METRIC_GATE_PASS":all_gates=False
  if not verified_only or grounded:
   weighted+=image_score*float(feat["weight"])
   denominator+=float(feat["weight"])
  record={"id":fid,"label":feat["label"],"score_0_to_100":image_score,
   "threshold_0_to_100":threshold,"critical":mandatory,"weight":feat["weight"],
   "gate_status":status,"feature_identity_verified":grounded,
   "reference_roi":box,"candidate_roi":candidate_box,
   "components":metrics,
   "requested_view":feat.get("view"),"camera_framing_matched":None}
  if report_dir:
   left=ref.crop(box);right=cand.crop(candidate_box)
   left.thumbnail((240,240));right.thumbnail((240,240))
   output=Image.new("RGB",(510,292),"white")
   draw=ImageDraw.Draw(output)
   output.paste(left,((245-left.width)//2,28))
   output.paste(right,(260+(245-right.width)//2,28))
   draw.text((7,3),"Reference: "+fid,fill=(20,20,20))
   draw.text((261,3),"Koikatsu: "+str(round(image_score,2)),fill=(20,20,20))
   draw.text((8,270),status,fill=(145,30,30) if status!="METRIC_GATE_PASS" else (40,105,55))
   loc=report_dir/(str(i).zfill(2)+"_"+fid+".png")
   output.save(loc)
   record["comparison_image"]=str(loc)
  props.append(record)
 aggregate=round(weighted/denominator,2) if denominator else 0.
 required=sum(int(x["critical"]) for x in props)
 blocked=[x["id"] for x in props if x["critical"] and x["gate_status"]!="METRIC_GATE_PASS"]
 return {"schema":"NW_KK_FEATURE_GATE_SCORECARD_V1","reference":str(original),
  "candidate":str(candidate),"features":props,
  "individual_feature_count":len(props),"critical_feature_count":required,
  "blocked_critical_gates":blocked,
  "weighted_metric_score_0_to_100":aggregate,
  "all_critical_metric_gates_pass":all_gates and required>0,
  "artistic_visual_fidelity_accepted":False,
  "status":"REQUIRES_CORRECTED_REFERENCE_REGIONS_OR_VISUAL_REVIEW" if blocked else "METRIC_GATE_CLEAR_REQUIRES_ARTISTIC_REVIEW",
  "note":"Per-feature image proxy only; cannot assert 3D topology, semantic identity, pose normalization or quality."}

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--manifest",required=True);p.add_argument("--candidate",required=True)
 p.add_argument("--out",required=True);p.add_argument("--crops")
 a=p.parse_args()
 out=pathlib.Path(a.out)
 if out.exists():raise FileExistsError(out)
 manifest=json.loads(pathlib.Path(a.manifest).read_text(encoding="utf-8-sig"))
 crop_dir=pathlib.Path(a.crops) if a.crops else None
 score=score_features(manifest,a.candidate,report_dir=crop_dir)
 out.parent.mkdir(exist_ok=True,parents=True)
 out.write_text(json.dumps(score,indent=2),encoding="utf-8")
 print("FEATURE_GATES="+json.dumps({"weighted_metric_score":score["weighted_metric_score_0_to_100"],
  "gate_status":score["status"],"blocked":score["blocked_critical_gates"],
  "features":[(f["id"],f["score_0_to_100"]) for f in score["features"]]}))
if __name__=="__main__":main()
