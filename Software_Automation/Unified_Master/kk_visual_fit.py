#!/usr/bin/env python3
"""NoWorld Gate01: CPU-only reference-guided Koikatsu image comparison.

NOT a generator or substitute for artistic identity judgment. A testable visual
objective with fail-closed reference checks and separate face/body evidence.
Uses PIL/NumPy/SciPy already installed on authorized Windows workstation.
Original assets and sources are never changed.
"""
from __future__ import annotations
import argparse, hashlib, json, math, pathlib
import numpy as np
from PIL import Image, ImageDraw, ImageOps
from scipy.ndimage import sobel, distance_transform_edt, binary_opening, binary_fill_holes, label, gaussian_filter

SIZE=(256,384)
def read_region(file,roi=None):
    path=pathlib.Path(file)
    if not path.is_file():raise FileNotFoundError(path)
    im=Image.open(path).convert("RGB")
    if roi is not None:
        if len(roi)!=4:raise ValueError("ROI must be x1,y1,x2,y2")
        x1,y1,x2,y2=map(int,roi)
        if not(0<=x1<x2<=im.width and 0<=y1<y2<=im.height):
            raise ValueError(f"Bad image region {roi} for {im.size}")
        im=im.crop((x1,y1,x2,y2))
    return np.asarray(im,dtype=np.float32)/255.

def white_background_confidence(a):
    h,w=a.shape[:2];k=max(3,min(h,w)//20)
    corners=np.concatenate([
      a[:k,:k].reshape(-1,3),a[:k,-k:].reshape(-1,3),
      a[-k:,:k].reshape(-1,3),a[-k:,-k:].reshape(-1,3)])
    # Native isolated Koikatsu renders have white corners; intentionally
    # fail closed on Maker scene backgrounds rather than hallucinate a silhouette.
    white=np.linalg.norm(corners-1,axis=1)
    confidence=float(np.mean(np.clip(1-white/.36,0,1)))
    return confidence

def foreground(a,white_only=True):
    confidence=white_background_confidence(a)
    if white_only and confidence<.68:
        raise ValueError(f"Uncontrolled background (white confidence {confidence:.3f}); use isolated native camera image")
    chroma=np.max(a,axis=2)-np.min(a,axis=2)
    darkness=1-np.min(a,axis=2)
    soft=np.clip((darkness-.085)*3.4+chroma*.4,0,1)
    binary=soft>.27
    # Ignore disconnected anti-aliasing and JPEG noise, but allow floating pieces.
    if binary.shape[0]>100 and binary.shape[1]>100:
        cleaned=binary_opening(binary,iterations=1)
        if cleaned.sum()>=binary.sum()*.6:binary=cleaned
    if binary.mean()<.008:
        raise ValueError("No visible character mass; likely blank, overexposed or misframed")
    if binary.mean()>.90:
        raise ValueError("Scene/background dominates; cannot validate character")
    return binary,soft,confidence

def bbox(mask,pad=.035):
    yy,xx=np.nonzero(mask)
    if len(xx)==0:raise ValueError("No target foreground")
    x1,x2=int(xx.min()),int(xx.max())+1
    y1,y2=int(yy.min()),int(yy.max())+1
    ex=int((x2-x1)*pad)+1;ey=int((y2-y1)*pad)+1
    return max(0,x1-ex),max(0,y1-ey),min(mask.shape[1],x2+ex),min(mask.shape[0],y2+ey)

def resize_frame(rgb,mask,shape=SIZE):
    """Align subject center/scale while preserving intrinsic aspect ratio."""
    h,w=rgb.shape[:2]
    b=bbox(mask)
    crop=rgb[b[1]:b[3],b[0]:b[2]]
    fg=mask[b[1]:b[3],b[0]:b[2]]
    H,W=shape[1],shape[0]
    scale=min((W*.90)/crop.shape[1],(H*.90)/crop.shape[0])
    nw=max(2,round(crop.shape[1]*scale));nh=max(2,round(crop.shape[0]*scale))
    image=Image.fromarray(np.uint8(np.clip(crop*255,0,255))).resize((nw,nh),Image.Resampling.BICUBIC)
    m=Image.fromarray(np.uint8(fg*255)).resize((nw,nh),Image.Resampling.NEAREST)
    cv=np.ones((H,W,3),dtype=np.float32)
    mk=np.zeros((H,W),dtype=np.bool_)
    x=(W-nw)//2;y=(H-nh)//2
    cv[y:y+nh,x:x+nw]=np.asarray(image,dtype=np.float32)/255
    mk[y:y+nh,x:x+nw]=np.asarray(m)>127
    return cv,mk,(crop.shape[1]/max(1,crop.shape[0]))

def rgb_lab(rgb):
    a=np.clip(rgb,0,1)
    linear=np.where(a>.04045,((a+.055)/1.055)**2.4,a/12.92)
    xyz=linear@np.array([[.4124564,.3575761,.1804375],
                         [.2126729,.7151522,.0721750],
                         [.0193339,.1191920,.9503041]],dtype=np.float32).T
    xyz=xyz/np.array([.95047,1.0,1.08883],dtype=np.float32)
    f=np.where(xyz>(6/29)**3,np.cbrt(xyz),xyz/(3*(6/29)**2)+4/29)
    L=116*f[...,1]-16
    aa=500*(f[...,0]-f[...,1]);bb=200*(f[...,1]-f[...,2])
    return np.stack([L,aa,bb],axis=-1)

def hist_similarity(a,b,mask_a,mask_b):
    """Compare source material palette; insensitive to unimportant pixel shifts."""
    def histogram(lab,mask):
        hist=[]
        for channel,low,high in ((0,0,100),(1,-110,110),(2,-110,110)):
            h,_=np.histogram(lab[:,:,channel][mask],bins=24,range=(low,high))
            h=h.astype(np.float64);h/=(h.sum()+1e-6);hist.append(h)
        return np.concatenate(hist)
    h1=histogram(a,mask_a);h2=histogram(b,mask_b)
    # histogram-intersection across 3 color channels, normalized to [0,1]
    return float(np.minimum(h1,h2).sum()/3)

def edge_map(img,mask):
    gray=np.mean(img,axis=2)
    smoothed=gaussian_filter(gray,1.1)
    ed=np.hypot(sobel(smoothed,axis=0),sobel(smoothed,axis=1))
    # Keep visible curves and wrinkles AND object boundaries.
    thresh=np.percentile(ed[mask],74) if mask.any() else np.inf
    return (ed>max(thresh,.045)) & (binary_dilate(mask,2))

def binary_dilate(mask,n):
    from scipy.ndimage import binary_dilation
    return binary_dilation(mask,iterations=n)

def edge_chamfer(a,b):
    if a.sum()<40 or b.sum()<40:return 0.
    da=distance_transform_edt(~a)
    db=distance_transform_edt(~b)
    distance=(np.clip(da[b],0,15).mean()+np.clip(db[a],0,15).mean())/30
    return float(np.clip(1-distance,0,1))

def evaluate(reference,candidate,reference_roi=None,candidate_roi=None,
             require_white=True,face_reference=None,face_candidate=None):
    a=read_region(reference,reference_roi)
    b=read_region(candidate,candidate_roi)
    ma,sa,conf_a=foreground(a,white_only=require_white)
    mb,sb,conf_b=foreground(b,white_only=require_white)
    a,m1,aspect_a=resize_frame(a,ma)
    b,m2,aspect_b=resize_frame(b,mb)
    p1=rgb_lab(a);p2=rgb_lab(b)
    intersection=(m1&m2).sum()
    union=(m1|m2).sum()
    silhouette=float(intersection/max(1,union))
    ratio_score=math.exp(-abs(math.log(max(.001,aspect_a)/max(.001,aspect_b)))*1.7)
    edge=edge_chamfer(edge_map(a,m1),edge_map(b,m2))
    palette=hist_similarity(p1,p2,m1,m2)
    joint=m1&m2
    color_dist=np.linalg.norm(p1-p2,axis=2)
    color_alignment=float(np.exp(-np.mean(np.clip(color_dist[joint],0,90))/60)) if joint.sum()>50 else 0
    # Explicit penalties for missing/extra color and wrong reference proportions.
    component={
      "silhouette_iou":round(silhouette,4),
      "aspect_ratio_similarity":round(ratio_score,4),
      "edge_structure":round(edge,4),
      "palette_histogram":round(palette,4),
      "aligned_color":round(color_alignment,4)}
    raw_score=(.29*silhouette+.19*ratio_score+.20*edge+.19*palette+.13*color_alignment)
    feature_score=None
    if face_reference and face_candidate:
        f=evaluate(face_reference,face_candidate,require_white=require_white)
        feature_score=float(f["score_0_to_100"]/100)
        raw_score=.58*raw_score+.42*feature_score
        component["face_view_score"]=round(feature_score,4)
    native_res=tuple(Image.open(candidate).size)
    original_res=tuple(Image.open(reference).size)
    source_pixels=a.shape[0]*a.shape[1]
    low_resolution=min(Image.open(reference).size)<384
    warnings=[]
    if low_resolution:warnings.append("REFERENCE_IS_LOW_RESOLUTION_NO_FIDELITY_ACCEPTANCE")
    if conf_a<.9 or conf_b<.9:warnings.append("BACKGROUND_NOT_PERFECTLY_WHITE")
    if not face_reference:warnings.append("FACE_NOT_INDEPENDENTLY_EVALUATED")
    warnings.append("PIXEL_METRIC_IS_NOT_AN_SEMANTIC_IDENTITY_MODEL")
    result={
     "schema":"NOWORLD_KK_VISUAL_SCORER_V1",
     "score_0_to_100":round(100*raw_score,2),
     "components":component,"reference_path":str(pathlib.Path(reference)),
     "candidate_path":str(pathlib.Path(candidate)),
     "reference_input_size":original_res,"candidate_input_size":native_res,
     "reference_background_confidence":round(conf_a,4),
     "candidate_background_confidence":round(conf_b,4),
     "reference_subject_bbox":bbox(ma),"candidate_subject_bbox":bbox(mb),
     "warnings":warnings,
     "visual_identity_accepted":False,
     "production_accepted":False}
    return result

def measure_visible_change(before,candidate,roi=None,noise_floor=0):
    """Measure localized appearance changes after subject centering/scale alignment.

    Calibrate `noise_floor` from repeated unchanged-condition native renders.
    Never claim a material improvement from non-calibrated pixel differences.
    """
    a=read_region(before,roi);b=read_region(candidate,roi)
    if a.shape!=b.shape:raise ValueError("Before/after camera framing differs")
    fg1,_,_=foreground(a,white_only=True)
    fg2,_,_=foreground(b,white_only=True)
    aa,m1,_=resize_frame(a,fg1)
    bb,m2,_=resize_frame(b,fg2)
    # Suppress subpixel camera jitter and dynamic anti-aliasing.
    aa=gaussian_filter(aa,sigma=(1.8,1.8,0))
    bb=gaussian_filter(bb,sigma=(1.8,1.8,0))
    diff=np.linalg.norm(aa-bb,axis=2)
    mass=m1|m2
    changed=(diff>.135)&mass
    fraction=float(changed.sum()/max(1,mass.sum()))
    rect=bbox(changed) if changed.sum() else None
    trusted=0<=noise_floor<.90
    return {"schema":"NOWORLD_NATIVE_VISIBLE_DELTA_V2",
       "changed_subject_fraction":round(fraction,5),
       "changed_pixels":int(changed.sum()),
       "subject_pixels":int(mass.sum()),
       "changed_bbox":rect,
       "calibrated_noise_floor":round(noise_floor,5),
       "measurably_changed":bool(trusted and fraction>noise_floor+.025),
       "quality_improved":None,
       "warning":"Native source movement persists; compare an identical-state camera repeat to establish noise floor."}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--reference",required=True)
    p.add_argument("--candidate",action="append",required=True)
    p.add_argument("--reference-roi",nargs=4,type=int)
    p.add_argument("--candidate-roi",nargs=4,type=int)
    p.add_argument("--reference-face")
    p.add_argument("--candidate-face")
    p.add_argument("--out",required=True)
    args=p.parse_args()
    if args.candidate_face and len(args.candidate)!=1:
        p.error("--candidate-face only with one --candidate")
    scored=[]
    for i,path in enumerate(args.candidate):
        try:
            result=evaluate(args.reference,path,args.reference_roi,args.candidate_roi,
                face_reference=args.reference_face,face_candidate=args.candidate_face)
        except (ValueError,FileNotFoundError) as exc:
            result={"candidate_path":path,"error":str(exc),"status":"REJECTED"}
        scored.append(result)
    scored.sort(key=lambda x:x.get("score_0_to_100",-1),reverse=True)
    data={"schema":"NOWORLD_KK_VISUAL_RANK_V1","results":scored,
       "candidate_count":len(scored),"ranked_count":sum("score_0_to_100" in x for x in scored),
       "note":"This is local reference comparison, NOT native artistic acceptance."}
    out=pathlib.Path(args.out)
    out.parent.mkdir(parents=True,exist_ok=True)
    if out.exists():raise FileExistsError("Refuse replacing old QA metrics")
    out.write_text(json.dumps(data,indent=2),encoding="utf-8")
    print("VISUAL_RANK="+json.dumps(
       [{"path":x["candidate_path"],"score":x.get("score_0_to_100"),"error":x.get("error")}
          for x in scored]))
    return 0 if data["ranked_count"] else 2
if __name__=="__main__":raise SystemExit(main())
