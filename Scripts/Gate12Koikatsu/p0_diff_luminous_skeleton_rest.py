"""NO-COOK Gate09 skeleton compatibility gate, across three V3 GLBs.
Compared by joint path/name AND array order. Equal counts are insufficient.
"""
import json
import math
import pathlib
import collections
import sys
from p0_audit_gltf_skinned_fidelity import load, accessor


def inspect(path):
    _, doc, binary = load(path)
    nodes = doc["nodes"]
    sk = doc["skins"][0]
    ids = sk["joints"]
    parents = {}
    for i, node in enumerate(nodes):
        for child in node.get("children", []):
            parents[child] = i
    def ident(index):
        names = []
        seen = set()
        while index not in seen:
            seen.add(index)
            names.append(nodes[index].get("name", f"node_{index}"))
            if index not in parents:
                break
            index = parents[index]
        return "/".join(reversed(names))
    triples = []
    for index in ids:
        item = nodes[index]
        triples.append({
            "id": ident(index), "name": item.get("name", ""),
            "position": item.get("translation", [0,0,0]),
            "rotation": item.get("rotation", [0,0,0,1]),
            "scale": item.get("scale", [1,1,1]),
            "matrix": item.get("matrix"),
        })
    matrices = accessor(doc, binary, sk.get("inverseBindMatrices", -1)) if "inverseBindMatrices" in sk else None
    return {
        "path": str(path), "joints": triples, "inverse_bind": matrices,
        "joint_count": len(ids),
        "joint_name_unique": len({t["name"] for t in triples}),
        "joint_path_unique": len({t["id"] for t in triples}),
    }


def compare(a, b):
    a_names = {x["id"]: x for x in a["joints"]}
    b_names = {x["id"]: x for x in b["joints"]}
    common = sorted(set(a_names) & set(b_names))
    rest_mismatch = 0
    largest_delta_m = 0
    rot_max = 0
    scale_max = 0
    for name in common:
        x, y = a_names[name], b_names[name]
        rest_mismatch += int(any(
            any(abs(float(v)-float(w))>1e-6 for v,w in zip(x[k],y[k]))
            for k in ("position","rotation","scale")))
        largest_delta_m = max(largest_delta_m,
            math.sqrt(sum((float(v)-float(w))**2 for v,w in zip(x["position"],y["position"]))))
        rot_max = max(rot_max,max(abs(float(v)-float(w)) for v,w in zip(x["rotation"],y["rotation"])))
        scale_max = max(scale_max,max(abs(float(v)-float(w)) for v,w in zip(x["scale"],y["scale"])))
    return {
        "names_in_both": len(common),
        "only_a": len(set(a_names)-set(b_names)),
        "only_b": len(set(b_names)-set(a_names)),
        "identical_ordered_names": [x["name"] for x in a["joints"]] == [x["name"] for x in b["joints"]],
        "identical_ordered_paths": [x["id"] for x in a["joints"]] == [x["id"] for x in b["joints"]],
        "rest_mismatch_in_shared_paths": rest_mismatch,
        "max_local_translation_delta_cm": largest_delta_m * 100,
        "max_local_quaternion_component_delta": rot_max,
        "max_local_scale_component_delta": scale_max,
        "animation_shared_skeleton_accepted": False,
        "outstanding_gate09": "Confirm and map bind/rest pose and skinned deformation under identical authored clip. Equal joint count is insufficient.",
    }


def main():
    if len(sys.argv) != 5:
        print("Usage: python p0_diff_luminous_skeleton_rest.py clothed.glb underwear.glb nude.glb output.json")
        return 2
    files = [pathlib.Path(x) for x in sys.argv[1:4]]
    data = [inspect(x) for x in files]
    output = {
        "schema":"GATE12_P0_REST_SKELETON_COMPARISON_V1",
        "sources":[{"path":d["path"],"joint_count":d["joint_count"],
                    "joint_name_unique":d["joint_name_unique"],
                    "joint_path_unique":d["joint_path_unique"]}
                   for d in data],
        "pairwise": {
            "CLOTHED_UNDERWEAR": compare(data[0],data[1]),
            "CLOTHED_NUDE": compare(data[0],data[2]),
            "UNDERWEAR_NUDE": compare(data[1],data[2]),
        },
        "no_native_animation_acceptance": True,
    }
    pathlib.Path(sys.argv[4]).parent.mkdir(parents=True,exist_ok=True)
    pathlib.Path(sys.argv[4]).write_text(json.dumps(output,indent=2)+"\n", encoding="utf-8")
    print(json.dumps(output["pairwise"],indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
