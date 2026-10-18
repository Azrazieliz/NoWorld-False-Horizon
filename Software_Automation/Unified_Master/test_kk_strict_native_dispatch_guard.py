"""Regression: Test07+ cannot bypass the real native authorization via script renaming."""
import pathlib,tempfile,json
from PIL import Image
import kk_benchmark_guard as g
import kk_native_visual_search as native
import kk_reference_features as sources

def denied(fn):
    try: fn()
    except RuntimeError as exc:
        if any(x in str(exc) for x in ("BLOCKED","SCORING_GATE_REQUIRED","outside scored reversible candidate plan")):return
        raise
    raise AssertionError("Ungated native mutation was permitted")

assert g.script_requires_gate("kk_ref10_real_native_fit.py")
assert g.script_requires_gate("kk_ref10_black_uv_fix.py")
assert g.script_requires_gate("new_random_native_edit.py")
assert g.script_requires_gate("other_direct_editor.py")
assert g.script_requires_gate("test10_build_character.py")
assert g.script_requires_gate("kk_ref11_fit.py")
assert not g.script_requires_gate("new_ref_gold_dancer_trial.py")
assert not g.script_requires_gate("test06_legacy.py")
# Name-only entries do not grant edit rights after Test06.
denied(lambda:g.require_permitted_step({"op":"set_hair","args":{"index":0,"effective_id":15}}))
with tempfile.TemporaryDirectory(prefix="kk_native_guard_q_") as td:
    root=pathlib.Path(td)
    mock=root/"fake_native_client.py"
    mock.write_text("import json\n"
        "def submit(payload,timeout=8):\n"
        " print(json.dumps({'status':'RECEIPT_WRITTEN','data':'mock'}))\n"
        " return 0\n",encoding="utf-8")
    original=native.master.CLIENT
    native.master.CLIENT=mock
    try:
        submit=native.client_factory()
        denied(lambda:submit({"op":"set_hair","effectiveId":4},8))
        nonce,_,r=native.invoke(submit,{"op":"inspect_appearance","args":{}},timeout=4)
        assert r["status"]=="RECEIPT_WRITTEN"
        src=root/"source.png"
        Image.new("RGB",(512,768),"white").save(src)
        boxes={"hair":[.16,.03,.81,.37],"face":[.28,.15,.68,.42],
               "eyes":[.32,.23,.65,.29],"body":[.12,.36,.91,.84],
               "main_outfit":[.22,.43,.82,.77],
               "sleeves":[.13,.44,.89,.73],"legs":[.15,.77,.85,.98],
               "footwear":None}
        manifest=sources.build_manifest(src,boxes=boxes)
        restore={"op":"set_hair_palette","args":{"index":0,"value":.3,"v2":.3,"v3":.3}}
        candidate={"op":"set_hair_palette","args":{"index":0,"value":.55,"v2":.45,"v3":.4}}
        plan={"schema":"NOWORLD_NATIVE_VISUAL_SWEEP_V1","reference":str(src),
              "view":"front","restore_baseline":[restore],
              "candidates":[{"label":"color_trial","steps":[candidate]}]}
        with g.scored_native_sweep(plan,manifest):
            nonce,_,r=native.invoke(submit,candidate,timeout=4)
            assert r["status"]=="RECEIPT_WRITTEN"
            denied(lambda:native.invoke(submit,{"op":"set_hair",
                     "args":{"index":1,"effective_id":15}},timeout=4))
        denied(lambda:native.invoke(submit,candidate,timeout=4))
    finally:
        native.master.CLIENT=original
print(json.dumps({"status":"PASS","cases":11,"real_game_edited":False,
"ungated_test07plus_blocked":True,"direct_client_submit_blocked":True,
"genuine_authorized_scored_edit_permitted":True}))
