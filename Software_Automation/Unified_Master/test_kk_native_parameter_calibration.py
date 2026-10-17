"""Offline instrumented fixture: verify actual pixel localization, noise and rollback gates."""
import tempfile,pathlib,json
from PIL import Image,ImageDraw
import kk_native_parameter_calibration as cal
import kk_native_visual_search as native

base={"op":"set_hair_palette","args":{"index":0,"value":.5,"v2":.5,"v3":.5}}
trial={"op":"set_hair_palette","args":{"index":0,"value":.9,"v2":.5,"v3":.5}}
spec={"schema":cal.SCHEMA,"view":"face","label":"hair color calibrated",
    "target_roi_norm":[.3,.04,.72,.4],"restore_baseline":[base],
    "trial_steps":[trial],"min_local_delta_fraction":.018}
tests={}
def check(name,fn):
    try:fn();tests[name]="PASS"
    except Exception as exc:tests[name]="FAIL:"+str(exc)
def require(ok,text):
    if not ok:raise AssertionError(text)
with tempfile.TemporaryDirectory() as td:
    root=pathlib.Path(td);old=native.CAMERA
    native.CAMERA=root/"camera";native.CAMERA.mkdir()
    # The calibration module references native.CAMERA dynamically, so no session reset.
    state={"color":.5,"renders":0,"mode":"hair","prevent_restore":False}
    def submit(data,timeout):
        if data["op"]=="set_hair_palette":
            if not (state["prevent_restore"] and data["value"]==.5 and state["renders"]>=5):
                state["color"]=data["value"]
        if data["op"]=="render_camera_view":
            n=state["renders"];state["renders"]+=1
            img=Image.new("RGB",(600,900),"white");d=ImageDraw.Draw(img)
            jitter=n%2
            d.ellipse((207,52+jitter,419,338+jitter),fill=(83,63,77))
            d.ellipse((250,149+jitter,372,315+jitter),fill=(230,196,174))
            # Modification occurs in either requested hair or nonrequested torso.
            if state["mode"]=="hair":
                c=(250,35,45) if state["color"]>.7 else (210,190,153)
                d.polygon([(224,145+jitter),(258,62+jitter),(365,62+jitter),(400,147+jitter),
                    (359,219+jitter),(278,188+jitter)],fill=c)
            else:
                c=(250,35,45) if state["color"]>.7 else (110,95,118)
                d.rectangle((220,425,420,775),fill=c)
            img.save(native.CAMERA/("SM01_CAM_"+data["nonce"]+"_face.png"))
        print(json.dumps({"status":"RECEIPT_WRITTEN"}));return 0
    check("reversible_hair_effect_in_target",lambda:require(
        cal.run(spec,root/"good",submit,game_guard=False)["calibrated_native_effect"],"Visible hair change rejected"))
    check("visual_evidence_generated",lambda:require(
        (root/"good"/"NATIVE_PARAMETER_VISUAL_DELTA.png").exists(),"No contact"))
    check("correct_baseline_after_trial",lambda:require(state["color"]==.5,"Baseline not restored"))
    def outside():
        state.update(color=.5,renders=0,mode="torso")
        x=cal.run(spec,root/"wrong_roi",submit,game_guard=False)
        require(not x["calibrated_native_effect"],"Outside ROI falsely calibrated")
        require(not x["observations"]["correctly_localized"],"Geometry location not checked")
    check("out_of_target_change_rejected",outside)
    def fake_face_morph_from_hair_only():
        state.update(color=.5,renders=0,mode="hair")
        x=dict(spec,label="face width must not be inferred from bangs",
            target_roi_norm=[.43,.27,.57,.38],min_local_delta_fraction=.005)
        result=cal.run(x,root/"fake_face",submit,game_guard=False)
        require(not result["calibrated_native_effect"],
                "Hair pixels were incorrectly credited as face geometry")
    check("hair_motion_not_face_calibration",fake_face_morph_from_hair_only)
    def unchanged():
        state.update(color=.5,renders=0,mode="hair")
        no_effect=dict(spec,trial_steps=[base])
        x=cal.run(no_effect,root/"no_effect",submit,game_guard=False)
        require(not x["calibrated_native_effect"],"No visual delta falsely positive")
    check("unchanged_native_parameter_rejected",unchanged)
    def badrestore():
        state.update(color=.5,renders=0,mode="hair",prevent_restore=True)
        x=cal.run(spec,root/"badrestore",submit,game_guard=False)
        require(not x["baseline_restoration_verified"],"Failed visual restore allowed")
        require(not x["calibrated_native_effect"],"Unsafe promotion with bad restore")
        state["prevent_restore"]=False
    check("visually_failed_restoration_rejected",badrestore)
    def invalid():
        x=dict(spec,target_roi_norm=[0.1,0.1,1.8,0.5])
        try:cal.validate(x)
        except ValueError:return
        raise AssertionError("Invalid ROI allowed")
    check("invalid_roi_blocked",invalid)
    def missingbaseline():
        x=dict(spec,restore_baseline=[{"op":"set_hair","args":{"index":0,"effective_id":19}}])
        try:cal.validate(x)
        except ValueError:return
        raise AssertionError("Unrestorable trial allowed")
    check("unmatched_baseline_setter_blocked",missingbaseline)
    def outfit_selection():
        b={"op":"set_clothes_top","args":{"guid":"baseline-mod","category":105,"original_id":0}}
        t={"op":"set_clothes_top","args":{"guid":"trial-mod","category":105,"original_id":2}}
        x=dict(spec,restore_baseline=[b],trial_steps=[t])
        require(cal.validate(x)==x,"Outfit selector incorrectly treated as slot identity")
    check("outfit_asset_switch_restorable",outfit_selection)
    native.CAMERA=old
print("PARAMETER_CALIBRATION_TESTS="+json.dumps({"passed":sum(v=="PASS" for v in tests.values()),
    "total":len(tests),"checks":tests,"real_game_edited":False},sort_keys=True))
if not all(v=="PASS" for v in tests.values()):raise SystemExit(2)
