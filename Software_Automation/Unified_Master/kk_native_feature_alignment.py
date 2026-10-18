"""Registered feature crops for original full-resolution -> white-background Maker render.

Fixes a concrete known issue: the original gate directly maps a reference ROI to
the SAME normalized coordinates on a different camera. On a native full-body
render those coordinates often contain only empty white background.

This module detects foreground *geometry*, not semantic anatomy. Source masks
are required for illustrated nonwhite backgrounds. Candidate ROIs stay anchored
to the baseline rendered figure so an accessory moving/floating cannot recenter
itself to conceal a regression. It never approves art or mutates Koikatsu.
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
from PIL import Image
import numpy as np
from scipy.ndimage import binary_closing, binary_dilation, label

def figure_bbox(image_or_path, mask=None):
    """Pixels (x0,y0,x1,y1). Avoid small isolated decorative/background spots."""
    if isinstance(image_or_path,(str,Path)):
        with Image.open(image_or_path) as image:
            a=np.asarray(image.convert("RGB"),dtype=np.uint8)
    else:
        a=np.asarray(image_or_path)
    if a.ndim!=3 or a.shape[2]!=3:
        raise ValueError("Expected RGB image")
    h,w=a.shape[:2]
    if mask is not None:
        if isinstance(mask,(str,Path)):
            with Image.open(mask) as m: fg=np.asarray(m.convert("L"))>=128
        else:fg=np.asarray(mask)>=128
        if fg.shape!=(h,w):raise ValueError("Foreground mask size mismatch")
    else:
        f=a.astype(np.int16)
        fg=np.max(255-f,axis=2)>27
    # Components can be disconnected at hair tips, foot tips or ears.
    # Preserve sufficiently large connected pieces and discard tiny noise.
    joined=binary_closing(binary_dilation(fg,iterations=2),iterations=1)
    components,count=label(joined)
    if count<1:raise ValueError("No visible subject in image")
    weights=np.bincount(components.ravel())
    weights[0]=0
    biggest=int(np.max(weights))
    useful=(weights>=max(100,int(biggest*.004)))
    useful[0]=False
    selected=useful[components]
    if selected.sum() < max(650,int(w*h*.003)):
        raise ValueError("Subject too small for credible feature scoring")
    y,x=np.nonzero(selected)
    bbox=(max(0,int(x.min())),max(0,int(y.min())),
          min(w,int(x.max()+1)),min(h,int(y.max()+1)))
    fraction=(bbox[2]-bbox[0])*(bbox[3]-bbox[1])/(h*w)
    if fraction<.012:raise ValueError("Subject has insufficient image footprint")
    return bbox

def register_boxes(manifest,baseline_native_image,reference_mask=None,min_padding=.10):
    """Map each REAL source ROI into the same baseline-native character frame.

    Does not guess semantics or change subject framing per variant. To support
    pose changes or occluded features, callers must provide per-view source/native
    feature annotations; this affine registration is conservative.
    """
    reference=manifest["reference_path"]
    with Image.open(reference) as im:rw,rh=im.size
    with Image.open(baseline_native_image) as im:cw,ch=im.size
    background=manifest.get("source_background")
    if reference_mask is None and background!="WHITE_LIKELY":
        raise ValueError("Illustration background needs explicit same-size subject mask")
    src=figure_bbox(reference,reference_mask)
    cand=figure_bbox(baseline_native_image)
    sw=src[2]-src[0];sh=src[3]-src[1]
    tw=cand[2]-cand[0];th=cand[3]-cand[1]
    if sw<=0 or sh<=0 or tw<=0 or th<=0:
        raise ValueError("Subject silhouette missing")
    mapped={}
    for f in manifest["features"]:
        if not f.get("feature_identity_verified"):
            raise ValueError("Unverified reference feature identity")
        a=f["reference_roi"]
        if not (0<=a[0]<a[2]<=rw and 0<=a[1]<a[3]<=rh):
            raise ValueError("Reference ROI outside source image")
        # Reference ROI -> normalized character-relative box -> native pixel box.
        coords=((a[0]-src[0])/sw,(a[1]-src[1])/sh,
                (a[2]-src[0])/sw,(a[3]-src[1])/sh)
        # Native fixed body bbox, slightly padded to avoid hiding floating
        # objects, while leaving huge scene padding outside the gate.
        x0=cand[0]+coords[0]*tw;y0=cand[1]+coords[1]*th
        x1=cand[0]+coords[2]*tw;y1=cand[1]+coords[3]*th
        pad_x=max(2,round((x1-x0)*min_padding))
        pad_y=max(2,round((y1-y0)*min_padding))
        roi=(max(0,int(x0-pad_x)),max(0,int(y0-pad_y)),
             min(cw,int(x1+pad_x)),min(ch,int(y1+pad_y)))
        if roi[2]-roi[0]<18 or roi[3]-roi[1]<18:
            raise ValueError("Camera cannot resolve feature "+f["id"])
        mapped[f["id"]]=roi
    return {"schema":"KK_NATIVE_SUBJECT_REGISTERED_ROIS_V1",
            "reference_bbox":src,"native_baseline_bbox":cand,
            "reference_size":[rw,rh],"native_size":[cw,ch],
            "candidate_rois":mapped,
            "source_background_masked":reference_mask is not None,
            "anchored_to_baseline":True,
            "pose_normalization_verified":False,
            "semantic_landmarks_detected":False}

class RegisteredFeatureScorer:
    """Adapter compatible with native.run's score(ref,image,..) callable."""
    def __init__(self,manifest,reference_mask=None):
        self.manifest=manifest
        self.reference_mask=reference_mask
        self.registration=None
    def __call__(self,reference,candidate,reference_roi=None,candidate_roi=None):
        import kk_feature_gates as gate
        if self.registration is None:
            self.registration=register_boxes(self.manifest,candidate,self.reference_mask)
        result=gate.score_features(self.manifest,candidate,
                                   rois=self.registration["candidate_rois"])
        return {"score_0_to_100":result["weighted_metric_score_0_to_100"],
                "components":{f["id"]:{"score":f["score_0_to_100"],
                     "blocked":f["gate_status"]!="METRIC_GATE_PASS"}
                    for f in result["features"]},
                "_registered_scorecard":result}
