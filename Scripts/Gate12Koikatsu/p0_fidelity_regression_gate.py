"""Strict, repeatable P0 quality gate; no cook, no asset mutation.
Read measured glTF, UE, camera, source receipt and skeleton data.
A green source compile is NEVER interpreted as a rendering/pipeline approval.
Exit 0 only if all acceptance criteria and Gate00 sign-off are present;
exit 2 for demonstrated defects/incomplete proof.
"""
import json
import pathlib
import sys
from datetime import datetime, timezone

ROOT=pathlib.Path(__file__).resolve().parents[2]
R=ROOT/"Saved/P0CharacterQuality"


def load(name):
    p=R/name
    if not p.is_file():
        return None
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def main():
    failures=[]
    passes=[]
    pending=[]
    a=load("gltf_v3_all_states_quantified.json")
    u=load("unreal_native_quality.json")
    s=load("skeleton_rest_diff.json")
    c=load("camera_scale_vs_apparent_size.json")
    m=load("unreal_material_pipeline.json")
    o=load("source_preservation/preservation_receipt.json")
    p=load("native_camera_and_geometry_compared.json")
    review=load("gate00_signed_pipeline_approval.json")
    if not a or len(a.get("files",[]))!=3:
        failures.append("Missing 3-state source skinned GLB quantification.")
    else:
        for f in a["files"]:
            g=f.get("geometry",{})
            skin=f.get("skeletons",[])
            if g.get("vertices_primitive_sum",0)<40000 or g.get("triangles_primitive_sum",0)<60000:
                failures.append("Unexpected source geometry loss: "+f["path"])
            if len(skin)!=1 or skin[0]["joint_count"]!=975:
                failures.append("Source GLB skinned joint count changed: "+f["path"])
            if g.get("missing_uv0_primitives") or g.get("missing_normals") or g.get("missing_skin_attrs"):
                failures.append("Source GLB lost UV, normals or skin: "+f["path"])
        passes.append("All three source GLBs retain substantial geometry, UV, normals and 975 joints.")
    if not o or len(o.get("cards",[]))!=3 or o.get("status")!="HASH_LOCKED_APPEARANCE_UNVERIFIED":
        failures.append("Three Alter originals and approved baseline ZIP not hash-locked.")
    elif not o.get("baseline"):
        failures.append("Gate00 base source ZIP not hash-locked.")
    else:
        passes.append("Three original Alter cards and V3 baseline archive preserved by SHA256.")
        pending.append("Gate01: source card appearance/state mapping and overwritten-card coordinate inspection.")
    if not s:
        failures.append("Missing cross-state bone rest-pose comparison.")
    elif all(x.get("rest_mismatch_in_shared_paths",0)==0 and
             x.get("only_a",1)==0 and x.get("only_b",1)==0
             for x in s.get("pairwise",{}).values()):
        passes.append("Cross-state bone rest/path compatibility equality (additional Gate09 animation QA still required).")
    else:
        failures.append("Cross-state 975-joint skeletons have differing paths/rest poses; common animation not accepted.")
    if not u:
        failures.append("No real Unreal skeletal asset bound/material/LOD import measurements.")
    elif not a:
        pending.append("Reconcile native Unreal values with three source GLBs.")
    else:
        for index,state in enumerate(("CLOTHED","UNDERWEAR","NUDE")):
            body=u.get("states",{}).get(state,{}).get("body",{})
            text=u.get("states",{}).get(state,{}).get("body_texture",{})
            prim=next((q for q in a["files"][index].get("primitive_details",[])
                       if q["mesh_index"]==3 and q["primitive_index"]==11), None)
            if not prim or "render_bounds" not in body:
                failures.append(f"{state}: Missing source/UE body primitive.")
                continue
            src=prim["max_m"]
            lo=prim["min_m"]
            native=body["render_bounds"]["size_cm"]
            for src_len, ue_len,axis in (
                ((src[0]-lo[0])*100,native[0],"horizontal"),
                ((src[1]-lo[1])*100,native[2],"vertical"),
                ((src[2]-lo[2])*100,native[1],"depth")):
                if abs(src_len-ue_len)>0.4:
                    failures.append(f"{state}: scale mismatch {axis}: {src_len:.3f}cm -> {ue_len:.3f}cm")
            if text.get("size_x")!=4096 or text.get("size_y")!=4096:
                failures.append(f"{state}: body 4096x4096 source texture lost.")
            if not body.get("material_slots") or not body.get("skeleton"):
                failures.append(f"{state}: missing source body material/skeleton.")
            if body.get("lod_count")!=1:
                failures.append(f"{state}: unexpected imported LOD count.")
            # Source morph thresholds were audited against real floating displacement.
            effective=15
            actual=body.get("morph_target_count",0)
            if actual < effective:
                failures.append(f"{state}: {effective} nonzero POSITION morph targets vs {actual} UE imported. Historical QA still lost targets at ZERO morph threshold; root cause requires importer investigation.")
        passes.append("Source-to-UE body bounds and nominal 4096px source image checked per state.")
    if not c:
        failures.append("No quantified camera-vs-world-scale separation.")
    else:
        for entry in c.get("baseline",[]):
            if entry.get("unposed_height_fraction",0)<0.65:
                failures.append(f"{entry['name']}: character appears at only {entry['unposed_height_fraction']*100:.1f}% of image height before pose/depth.")
        passes.append("Original camera/FOV/distance and proposed views analytically measured.")
    # Explicitly retain previous failed repair experiments instead of
    # repeating an already disproven "set morph threshold to zero" theory.
    morph_qa_dir = ROOT / "Saved/Gate12Koikatsu/LuminousV3/CLOTHED"
    historical_zero = morph_qa_dir / "staging_zero_morph_threshold_7_16_ue583.json"
    if historical_zero.is_file():
        probe = json.loads(historical_zero.read_text("utf-8"))
        if probe.get("originals_modified"):
            failures.append("Historical zero-threshold probe changed original imported sources.")
        for part in probe.get("parts", []):
            native_counts=[x.get("morph_count") for x in part.get("skeletal_meshes", [])]
            if native_counts and max(native_counts) < part.get("expected_source_nonzero_morphs", 0):
                failures.append(
                    f"ZERO-threshold morph reimport already FAILED index {part.get('index')}: "
                    f"source {part['expected_source_nonzero_morphs']} nonzero vs imported {max(native_counts)}.")
    else:
        pending.append("Prior zero-threshold morph experiments not found; cannot claim fix.")

    if not m:
        pending.append("Native hair/face/eyeline/body material parents, alpha blend, and texture links not yet audited.")
    else:
        for channel in ("body", "face", "hair", "eyeline_upper"):
            item = m.get("materials", {}).get(channel)
            if item is None:
                pending.append("No native material audit for " + channel)
                continue
            if item.get("error"):
                failures.append("Missing native " + channel + " material instance")
                continue
            if any("/InterchangeAssets/gltf/M_Default" in p for p in item.get("parents", [])):
                failures.append("Native " + channel + " is still bound to generic Interchange M_Default, not accepted character PBR")
            if "BLEND_OPAQUE" in item.get("blend_mode", "") and channel == "eyeline_upper":
                failures.append("Native alpha-gradient eyeline is OPAQUE but source GLB has 72.3% transparent / 15.7% gradient alpha")
            if item.get("two_sided") in ("False", "false"):
                pending.append("Native " + channel + " is single-sided; authored source material was double-sided, needs visual QA")
        pending.append("Actual streaming mip, shader translucency ordering, eye depth and screenshot quality still unverified")
    if not p:
        pending.append("Controlled matched native Party, Blender, raw Unreal, roster and World screenshots not yet captured; no fabricated visual parity.")
    if not review or review.get("approved_export_pipeline")!=True:
        pending.append("Gate00 has not selected/approved a production export pipeline.")
    if not review or review.get("gate09_rest_animation_accepted")!=True:
        pending.append("Gate09 must approve compatibility via shared source animation/deformation test.")
    if not review or review.get("gate11_production_bindings_accepted")!=True:
        pending.append("Gate11 accepted production character bindings and genuine roster/World actor not installed.")
    result={
        "schema":"GATE12_P0_CHARACTER_FIDELITY_ACCEPTANCE_V1",
        "utc":datetime.now(timezone.utc).isoformat(),
        "status":"BLOCKED" if failures or pending else "PASS_PRECOOK_SOURCE_ONLY",
        "source_preserved":bool(o and o.get("status")=="HASH_LOCKED_APPEARANCE_UNVERIFIED"),
        "passes":passes,"failed_checks":failures,"pending_external_evidence":pending,
        "source_acceptance_distinct_from_pipeline_acceptance":True,
        "no_android_cook_or_install":True,
        "approval_source":str(R/"gate00_signed_pipeline_approval.json"),
    }
    dest=R/"p0_fidelity_acceptance_report.json"
    dest.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(result,indent=2))
    return 0 if result["status"].startswith("PASS_") else 2


if __name__=="__main__":
    raise SystemExit(main())
