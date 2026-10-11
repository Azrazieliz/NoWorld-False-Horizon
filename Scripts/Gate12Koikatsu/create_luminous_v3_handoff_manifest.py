"""Reproducible, read-only status/provenance inventory for Gate 00/Gate 12.

Scans preserved working assets and immutable source receipts. Does NOT import,
delete, overwrite source, build the app, or change any frozen reference.
"""
from __future__ import annotations
import collections,datetime,hashlib,json,pathlib,subprocess,sys

W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
P=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
S=P/"characters/LUMINOUS_VALENTINE_V3/gate12_fresh_export_20261011"
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
E=W/"docs/production/gate12/evidence/luminous-v3-continuation-20261011"
E.mkdir(parents=True,exist_ok=True)
def hash_file(path):
    h=hashlib.sha256()
    with pathlib.Path(path).open("rb") as f:
        for block in iter(lambda:f.read(4*1024*1024),b""):
            h.update(block)
    return h.hexdigest()
def meta(path):
    path=pathlib.Path(path)
    return {"path":str(path),"exists":path.is_file(),"bytes":path.stat().st_size if path.is_file() else None,
            "sha256":hash_file(path) if path.is_file() else None}
def load(path):
    return json.loads(pathlib.Path(path).read_text(encoding="utf-8-sig"))
def git(*args):
    r=subprocess.run(["git","-C",str(W),*args],capture_output=True,text=True)
    return r.stdout.strip() if r.returncode==0 else "ERROR:"+r.stderr.strip()[:200]
states={}
native_details={}
for state,ident in (("CLOTHED","LuminousV3CLOTHED"),("UNDERWEAR","LuminousV3UNDERWEAR"),("NUDE","LuminousV3NUDE")):
    a=load(S/("FRESH_EXPORT_"+state+"_RECEIPT.json"))
    b=load(S/"BLENDER_FRESH"/state/"GATE12_FRESH_BLENDER_CONVERSION_RECEIPT.json")
    c=load(S/"BLENDER_FRESH"/state/"GATE12_GLTF_REAL_GEOMETRY_MORPH_AUDIT.json")
    shard=load(Q/"LosslessSources"/ident/"shards/manifest_full_fidelity_primitives.json")
    morph=load(Q/state/"source_shards_actual_nonzero_morph_inventory.json")
    raw=S/"BLENDER_FRESH"/state
    card=S/"SEALED_INPUTS"/(state+".png")
    verify=load(S/"SEALED_INTAKE_RECEIPT.json")["three_states"][state]
    assert hash_file(card)==verify["sha256"]==a["source_card_sha256"],state
    files={"primary_party_card":meta(card),
           "primary_party_pmx":meta(S/"FRESH_PARTY_EXPORTS"/state/"model.pmx"),
           "blender_editable":meta(raw/("LUMINOUS_V3_"+state+"_original_import.blend")),
           "fbx_with_bones":meta(raw/("LUMINOUS_V3_"+state+"_native.fbx")),
           "skinned_morphed_glb":meta(raw/("LUMINOUS_V3_"+state+"_skinned.glb")),
           "export_receipt":meta(S/("FRESH_EXPORT_"+state+"_RECEIPT.json")),
           "blend_receipt":meta(raw/"GATE12_FRESH_BLENDER_CONVERSION_RECEIPT.json"),
           "glb_audit":meta(raw/"GATE12_GLTF_REAL_GEOMETRY_MORPH_AUDIT.json"),
           "shard_manifest":meta(Q/"LosslessSources"/ident/"shards/manifest_full_fidelity_primitives.json"),
           "nonzero_morph_inventory":meta(Q/state/"source_shards_actual_nonzero_morph_inventory.json")}
    assert files["primary_party_pmx"]["sha256"]==a["pmx_sha256"]
    assert files["skinned_morphed_glb"]["sha256"]==c["glb_sha256"]
    assert len(shard["shards"])==morph["total_shards"]==c["primitive_count"]
    assert morph["nominal_source_morph_slots"]==c["morph_target_references"]
    states[state]={"card_source":verify,
                   "real_party_export":{"status":a["status"],"files":a["file_count"],"bytes":a["total_export_bytes"],
                   "source_dir":a["game_export_source_path"],"sealed_export_dir":a["state_output_path"],
                   "underlying_file_inventory_embedded_in_receipt":len(a["file_inventory"])},
                   "blender":{"status":b["status"],"geometry":{k:v for k,v in b["native_blender_geometry"].items() if k!="image_metrics"},
                              "steps":[i["step"] for i in b["steps"]]},
                   "glb":{"status":c["status"],"source_joint_count":c["joint_count_per_skin"],
                           "mesh_primitives":c["primitive_count"],"morph_slots":c["morph_target_references"],
                           "textures":c["embedded_image_count"],"material_count":c["materials_count"]},
                   "lossless_shards":{"status":shard["status"],"count":len(shard["shards"]),
                       "source_sha256":shard["original_glb_sha256"],
                       "all_shard_sha256_in_manifest":True},
                   "effective_morphs":{"nominal":morph["nominal_source_morph_slots"],
                        "nonzero_position":morph["meaningful_nonzero_position_morph_targets"]},
                   "critical_files":files}
    dir=Q/state
    rows=[]
    for p in sorted(dir.glob("native_shards_*_ue583.json")):
        try:r=load(p)
        except Exception as exc:rows.append({"journal":str(p),"json_parse_error":repr(exc)});continue
        rows.append({"journal":str(p),"journal_sha256":hash_file(p),
                     "batch_status":r.get("status"),"error":r.get("error"),
                     "parts":[{"index":q.get("index"),"mesh":q.get("mesh"),
                               "primitive":q.get("primitive"),"status":q.get("status"),
                               "source_nonzero_position_morphs":q.get("source_nonzero_position_morphs"),
                               "ue_total_native_morph_targets":q.get("ue_total_native_morph_targets"),
                               "ue_asset_count":q.get("ue_asset_count"),
                               "assets":q.get("assets",[])} for q in r.get("native_parts",[])]})
    native_details[state]={"batch_journals":rows}
    if state=="CLOTHED":
        content=W/"Content/Experimental/LuminousValentineV3/CLOTHED/Parts"
        uassets=[meta(p) for p in sorted(content.rglob("*.uasset"))] if content.is_dir() else []
        native_details[state]["uncommitted_native_uassets"]=uassets
        native_details[state]["uncommitted_native_uasset_total_bytes"]=sum(p["bytes"] for p in uassets)
        native_details[state]["uncommitted_native_uasset_count"]=len(uassets)

native_scripts=[
 "audit_luminous_v3_all_shards_effective_morphs.py",
 "import_luminous_v3_full_glb_real_ue583.py",
 "import_luminous_v3_lossless_shards_one_batch_ue583.py",
 "run_luminous_v3_native_full_glb_one_state_guarded.ps1",
 "run_luminous_v3_native_shard_batch_guarded.ps1",
 "validate_luminous_source_morph_actual_deltas.py",
 "build_kk_export_generic.py","verify_kk_nonanimation_baseline.py",
 "import_luminous_v3_first_native_part_guarded.ps1"]
external_scripts=[p for p in sorted((P/"pipeline").iterdir()) if p.is_file() and
                  (p.name.startswith("gate12_") or p.name.startswith("run_gate12_")) and
                  p.suffix in (".py",".ps1")]
assets=W/"Content/Experimental/LuminousValentineV3/CLOTHED/Parts"
relevant={
 "schema":"GATE12_LUMINOUS_V3_AUTHORITATIVE_CONTINUATION_MANIFEST_20261011",
 "generated_at_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
 "device":{"name":"LAPTOP-1LI4VRCJ","id":"3f193111-df57-4fc2-9e4f-1c4d8e8a5d87",
           "target_phone":"Samsung Galaxy S26 Ultra SM-S948B","adb_serial":"R3GYC0LSC7K"},
 "paths":{"isolated_ue_worktree":str(W),"frozen_foundation":r"D:\UnrealProjects\Offline-Game",
          "intake_and_conversions":str(S),"native_journals":str(Q),
          "lossless_source_shards":str(Q/"LosslessSources"),
          "original_party_install":r"D:\Tools\Steam\steamapps\common\Koikatsu Party",
          "pipeline_scripts":str(P/"pipeline"),
          "new_luminous_native_ue_uassets":str(assets),
          "source_cards":str(S/"SEALED_INPUTS")},
 "git":{"branch":git("branch","--show-current"),"head_at_checkpoint":git("rev-parse","HEAD"),
        "remote":git("remote","get-url","origin"),
        "frozen_foundation_commit":"3f54f07cacbf193e160d0be15a7be5a7bc730c3b",
        "known_open_draft_pr":"16"},
 "source_archive":meta(P/"handoffs/GATE00_LUMINOUS_V3_Koikatsu_3D_Source_and_Registry_Intake_20261011.zip"),
 "intake":meta(S/"SEALED_INTAKE_RECEIPT.json"),
 "kkbp_exporter_restoration":meta(S/"KKBP_EXPORTER_SINGLE_DLL_RESTORE_RECEIPT.json"),
 "animation_source_probe":meta(S/"KOIKATSU_INSTALLED_ANIMATION_CLIP_READONLY_PROBE.json"),
 "animation_native_curve_probe":meta(S/"KOIKATSU_ANIMATION_NATIVE_CURVE_STRUCTURE.json"),
 "states":states,"native_import":native_details,
 "code_files":{
  "gate12_native_scripts":{n:meta(W/"Scripts/Gate12Koikatsu"/n) for n in native_scripts},
  "external_workstation_pipeline_scripts":{p.name:meta(p) for p in external_scripts},
  "uncompiled_wip_cpp":{"header":meta(W/"Source/OfflineGame/Public/Koikatsu/OGKoikatsuPlayableCharacter.h"),
                        "implementation":meta(W/"Source/OfflineGame/Private/Koikatsu/OGKoikatsuPlayableCharacter.cpp")}},
 "not_yet_done":{"whole_luminous_runtime_playable":True,"runtime_state_switch_three_real_outfits":True,
                 "all_121_real_ue_skeletal_parts_complete":True,
                 "all_native_morphs_equal_meaningful_source_targets":True,
                 "luminous_android_cook_and_package":True,
                 "s26_luminous_apk_install":True,
                 "s26_luminous_gameplay_test":True,
                 "unreal_renderer_frame_time_measurement":True,
                 "gate11_artifact_ready":True},
 "instruction":"Do not treat PASS importer journals as production Gate11 acceptance; never reset or delete the source data, frozen Foundation, saves, original Android package, draft PR16, or existing partial uassets.",
}
name=E/"GATE12_LUMINOUS_V3_SOURCE_AND_WORKTREE_MANIFEST.json"
name.write_text(json.dumps(relevant,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
print("MANIFEST",name,"BYTES",name.stat().st_size,"SHA256",hash_file(name))
for state,datum in states.items():
    print("STATE",state,"export",datum["real_party_export"]["files"],"GLB_PRIMS",datum["glb"]["mesh_primitives"],
          "MORPH_NONZERO",datum["effective_morphs"]["nonzero_position"])
for state,native in native_details.items():
    print("STATE_NATIVE",state,"JOURNALS",len(native["batch_journals"]),
          "PARTS",sum(len(x.get("parts",[])) for x in native["batch_journals"]))
print("NATIVE_CLOTHED_UASSETS",native_details["CLOTHED"]["uncommitted_native_uasset_count"])
