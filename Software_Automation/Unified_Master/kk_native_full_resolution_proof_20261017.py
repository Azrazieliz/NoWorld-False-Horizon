"""Rebuild full-resolution side-by-side evidence from immutable real Maker frames."""
import json,pathlib
import kk_native_parameter_calibration as cal
ROOT=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA\native_parameter_live_20261017_v1")
for item,roi in (("face_0",[.33,.61,.69,.82]),("body_1",[.15,.40,.86,.88])):
    rec=json.loads((ROOT/item/"NATIVE_PARAMETER_CALIBRATION.json").read_text())
    q=rec["captures"]
    b=cal.load_images(q["baseline"])
    t=cal.load_images(q["trial"])
    r=cal.load_images(q["restore"])
    result,changed,ref,trial=cal.compute(b,t,r,roi,.006,.12)
    out=ROOT/item/"FULL_RES_NATIVE_COMPARISON_V2.png"
    if out.exists():raise FileExistsError(out)
    cal.contact(ref,trial,changed,result["target_roi_pixels"],out)
    print("FULL_RES_NATIVE_PROOF="+json.dumps({"control":item,"file":str(out),
        "dimensions":[ref.shape[1]*3,ref.shape[0]+36],"skin_or_head_roi_change":result["local_changed_fraction"],
        "localized":result["correctly_localized"],"visual_restore_inside_noise":result["restoration_visual_verified"]}))
