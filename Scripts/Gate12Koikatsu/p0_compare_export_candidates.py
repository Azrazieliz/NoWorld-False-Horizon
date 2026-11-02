"""No-cook comparative export-family assessment. Does not approve a pipeline.

Input manifest JSON:
{
 "schema":"G12_P0_EXPORT_CANDIDATES_V1",
 "candidates":[
   {"label":"reference_glb_clothed","family":"corrected_kkbp",
    "character":"LUMINOUS_V3","appearance":"CLOTHED",
    "file":"ABSOLUTE_PATH_TO_REAL_GLB",
    "source_card_sha256":"verified SHA256 from Gate01",
    "source_provenance_verified_by_gate01":false,
    "approved_by_gate00":false}
 ]
}
Do NOT invent Gate01-approved source-card hashes or false card mappings.
Each candidate's provenance is a separate gate from its measured geometry.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
from p0_audit_gltf_skinned_fidelity import audit

FAMILIES={"raw_kkbp_existing","corrected_kkbp","native_unity_skinned","hybrid_unity_blender"}
STATES={"CLOTHED","UNDERWEAR","NUDE","OTHER_VERIFIED"}
CHARACTERS={"LUMINOUS_V3","USER_ALTER"}
def delta(a,b):
    if a is None or b is None:return None
    return round(b-a,6)


def cmp(a,b):
    a_geo,b_geo=a["geometry"],b["geometry"]
    ga=a.get("rest_geometry_bounds")
    gb=b.get("rest_geometry_bounds")
    sa=a.get("skeletons") or []
    sb=b.get("skeletons") or []
    return {
        "vertex_delta":delta(a_geo.get("vertices_primitive_sum"),b_geo.get("vertices_primitive_sum")),
        "triangle_delta":delta(a_geo.get("triangles_primitive_sum"),b_geo.get("triangles_primitive_sum")),
        "material_count_delta":len(b.get("materials",[]))-len(a.get("materials",[])),
        "texture_count_delta":len(b.get("images",[]))-len(a.get("images",[])),
        "morph_slot_delta":delta(a_geo.get("morph_targets_total"),b_geo.get("morph_targets_total")),
        "source_rest_aabb_delta_cm":
            [delta(x,y) for x,y in zip(ga["extent_cm"],gb["extent_cm"])] if ga and gb else None,
        "joint_count_equal":bool(sa and sb and sa[0]["joint_count"]==sb[0]["joint_count"]),
        "joint_rest_equal":bool(sa and sb and sa[0]["joint_rest_sha256"]==sb[0]["joint_rest_sha256"]),
        "joint_order_names_equal":bool(sa and sb and sa[0]["joint_name_sha256"]==sb[0]["joint_name_sha256"]),
        "images_present":bool(b.get("images")),
        "uv_skin_normal_coverage":all(b_geo.get(n)==0 for n in (
            "missing_uv0_primitives","missing_normals","missing_skin_attrs")),
        "does_not_prove_visual_equivalence":True,
    }


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--manifest",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    a=parser.parse_args()
    manifest=json.loads(a.manifest.read_text("utf-8"))
    if manifest.get("schema")!="G12_P0_EXPORT_CANDIDATES_V1":
        raise ValueError("Noncanonical candidate schema")
    seen=set()
    rows=[]
    checks=[]
    for entry in manifest.get("candidates",[]):
        label=entry.get("label")
        family=entry.get("family")
        char=entry.get("character")
        state=entry.get("appearance")
        if not label or label in seen or family not in FAMILIES or char not in CHARACTERS or state not in STATES:
            raise ValueError("Duplicate/invalid family/character/appearance/label")
        seen.add(label)
        path=Path(entry["file"])
        if not path.is_file():
            rows.append({"label":label,"status":"SOURCE_FILE_NOT_PROVIDED","path":str(path)})
            continue
        result=audit(path)
        provenance=bool(entry.get("source_provenance_verified_by_gate01",False) and
                        isinstance(entry.get("source_card_sha256"),str) and
                        len(entry["source_card_sha256"])==64)
        rows.append({
            "label":label,"family":family,"character":char,"appearance":state,
            "file":str(path),"sha256":result["sha256"],
            "source_card_sha256":entry.get("source_card_sha256"),
            "gate01_provenance_asserted":provenance,
            "gate00_pipeline_approved":bool(entry.get("approved_by_gate00",False)),
            "status":"QUANTIFIED_UNAPPROVED" if provenance else "QUANTIFIED_PROVENANCE_PENDING",
            "metrics":result,
        })
    for x in rows:
        for y in rows:
            if x is y or x.get("character")!=y.get("character") or x.get("appearance")!=y.get("appearance"):
                continue
            if "metrics" not in x or "metrics" not in y or x["label"]>=y["label"]:
                continue
            checks.append({"a":x["label"],"b":y["label"],
                           "same_fidelity_source_revision":
                               bool(x.get("source_card_sha256") and
                                    x["source_card_sha256"]==y.get("source_card_sha256") and
                                    x.get("gate01_provenance_asserted") and
                                    y.get("gate01_provenance_asserted")),
                           "quantified_differences":cmp(x["metrics"],y["metrics"])})
    report={
        "schema":"G12_P0_EXPORT_FAMILY_COMPARATIVE_V1",
        "status":"SOURCE_QUANTIFICATION_NOT_PIPELINE_SELECTION",
        "candidates":rows,"pairwise":checks,
        "unverified_candidates":sum(not x.get("gate01_provenance_asserted") for x in rows),
        "production_pipeline_selected":False,
        "native_unreal_import_and_render_acceptance":False,
        "required_handoff":["Verified same-source revision Gate01 export provenance",
                             "Skinned mesh and bind/rest pose equality or Gate09 retarget",
                             "Actual native Unreal LOD and texture/material parity",
                             "Matched rendered camera/lighting captures, Android profiling",
                             "Gate00 explicit separate pipeline approval"],
    }
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print("CANDIDATES",len(rows),"VALID_PAIRWISE",len(checks),
          "WAIT_GATE01",report["unverified_candidates"],"OUTPUT",a.output)
    return 0 if all("metrics" in r for r in rows) else 2

if __name__=="__main__":
    raise SystemExit(main())
