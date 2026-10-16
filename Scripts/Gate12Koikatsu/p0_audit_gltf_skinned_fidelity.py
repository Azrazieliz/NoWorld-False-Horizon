"""Read-only, source-level glTF 2.0 binary fidelity audit.
Quantifies geometry, materials, texture payloads, morphs, joint rest data,
unposed spatial bounds and camera-size confounds. It does NOT prove KK native
viewport parity, UE imported LOD fidelity or compatible skeletal animations.
"""
import argparse
import hashlib
import json
import math
import pathlib
import struct
from collections import Counter
from itertools import product

COMP = {5120: ("b", 1), 5121: ("B", 1), 5122: ("h", 2),
        5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4)}
DIM = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def matrix(a):
    if "matrix" in a:
        return tuple(float(v) for v in a["matrix"])
    t = a.get("translation", [0., 0., 0.])
    s = a.get("scale", [1., 1., 1.])
    x, y, z, w = a.get("rotation", [0., 0., 0., 1.])
    return (
        (1-2*(y*y+z*z))*s[0], (2*(x*y+z*w))*s[0],
        (2*(x*z-y*w))*s[0], 0,
        (2*(x*y-z*w))*s[1], (1-2*(x*x+z*z))*s[1],
        (2*(y*z+x*w))*s[1], 0,
        (2*(x*z+y*w))*s[2], (2*(y*z-x*w))*s[2],
        (1-2*(x*x+y*y))*s[2], 0,
        t[0], t[1], t[2], 1,
    )


def multiply(a, b):
    return tuple(sum(a[k*4+r]*b[c*4+k] for k in range(4))
                 for c in range(4) for r in range(4))


def transform(m, v):
    x, y, z = v
    return tuple(sum(m[k*4+j]*[x, y, z][k] for k in range(3)) + m[12+j]
                 for j in range(3))


def load(path):
    raw = path.read_bytes()
    if raw[:4] != b"glTF":
        raise ValueError("Not a binary glTF source")
    version, declared = struct.unpack_from("<II", raw, 4)
    if version != 2 or declared != len(raw):
        raise ValueError("glTF header/version/file length mismatch")
    chunks = {}
    off = 12
    while off < len(raw):
        size, kind = struct.unpack_from("<II", raw, off)
        off += 8
        chunks[kind] = raw[off:off+size]
        off += size
    return raw, json.loads(chunks[0x4E4F534A]), chunks.get(0x004E4942, b"")


def accessor(doc, binary, index):
    a = doc["accessors"][index]
    if "bufferView" not in a:
        return None
    fmt, size = COMP[a["componentType"]]
    count = DIM[a["type"]]
    view = doc["bufferViews"][a["bufferView"]]
    offset = view.get("byteOffset", 0) + a.get("byteOffset", 0)
    stride = view.get("byteStride", count * size)
    if a.get("count", 0) > 2000:
        return None
    try:
        return [struct.unpack_from("<"+fmt*count, binary, offset+i*stride)
                for i in range(a["count"])]
    except (ValueError, struct.error):
        return None


def audit(path):
    raw, doc, blob = load(path)
    materials = doc.get("materials", [])
    meshes = doc.get("meshes", [])
    nodes = doc.get("nodes", [])
    accessors = doc.get("accessors", [])
    skin = doc.get("skins", [])
    material_indices = Counter()
    vertex_total = 0
    triangle_total = 0
    uv_missing = normal_missing = tangent_missing = skin_attr_missing = 0
    morph_primitive_count = morph_count = 0
    vertex_bounds = []
    primitive_details = []
    for mesh_index, mesh in enumerate(meshes):
        morph_count += len(mesh.get("weights", []))
        for primitive_index, prim in enumerate(mesh.get("primitives", [])):
            attrs = prim.get("attributes", {})
            p = accessors[attrs["POSITION"]] if "POSITION" in attrs else {}
            vertices = p.get("count", 0)
            vertex_total += vertices
            primitive_details.append({
                "mesh_index":mesh_index,"mesh_name":mesh.get("name"),
                "primitive_index":primitive_index,
                "vertices":vertices,
                "min_m":p.get("min"),"max_m":p.get("max"),
                "material":materials[prim["material"]].get("name")
                if prim.get("material",-1) in range(len(materials)) else None,
                "morph_targets":len(prim.get("targets",[])),
            })
            mode = prim.get("mode", 4)
            indices = accessors[prim["indices"]]["count"] if "indices" in prim else vertices
            if mode == 4:
                triangle_total += indices // 3
            uv_missing += int("TEXCOORD_0" not in attrs)
            normal_missing += int("NORMAL" not in attrs)
            tangent_missing += int("TANGENT" not in attrs)
            skin_attr_missing += int("JOINTS_0" not in attrs or "WEIGHTS_0" not in attrs)
            if prim.get("targets"):
                morph_primitive_count += 1
                morph_count += len(prim["targets"])
            material_indices[prim.get("material", -1)] += 1
    node_mesh_world = []
    used_nodes = set()
    identity = (1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.)
    def traverse(n, parent, depth=0):
        if n in used_nodes or depth > 100:
            return
        used_nodes.add(n)
        local = multiply(parent, matrix(nodes[n]))
        if "mesh" in nodes[n]:
            node_mesh_world.append((nodes[n]["mesh"], local, n))
        for child in nodes[n].get("children", []):
            traverse(child, local, depth+1)
    scenes = doc.get("scenes", [])
    chosen = doc.get("scene", 0)
    if scenes:
        for root in scenes[chosen].get("nodes", []):
            traverse(root, identity)
    projected = []
    for mi, m, ni in node_mesh_world:
        if mi >= len(meshes):
            continue
        for prim in meshes[mi].get("primitives", []):
            p = accessors[prim["attributes"]["POSITION"]] if "POSITION" in prim.get("attributes", {}) else {}
            if "min" in p and "max" in p:
                for c in product((0,1), repeat=3):
                    v = [p["max"][i] if c[i] else p["min"][i] for i in range(3)]
                    projected.append(transform(m, v))
    aabb = {
        "min_m": [min(x[j] for x in projected) for j in range(3)],
        "max_m": [max(x[j] for x in projected) for j in range(3)],
    } if projected else None
    if aabb:
        aabb["extent_m"] = [aabb["max_m"][j] - aabb["min_m"][j] for j in range(3)]
        aabb["extent_cm"] = [round(100*x, 3) for x in aabb["extent_m"]]
        aabb["note"] = "mesh-node rest AABB, not posed skinned vertex/world actor bounds"
    image_data = []
    for i, im in enumerate(doc.get("images", [])):
        payload = b""
        if "bufferView" in im:
            view = doc["bufferViews"][im["bufferView"]]
            offset = view.get("byteOffset", 0)
            payload = blob[offset:offset+view["byteLength"]]
        wh = None
        if payload[:8] == b"\x89PNG\r\n\x1a\n":
            wh = list(struct.unpack_from(">II", payload, 16))
        try:
            if wh is None and payload:
                from PIL import Image
                import io
                with Image.open(io.BytesIO(payload)) as image:
                    wh = list(image.size)
        except (ImportError, OSError):
            pass
        image_data.append({
            "index": i, "name": im.get("name"), "sha256": sha(payload) if payload else None,
            "bytes": len(payload), "pixels": wh, "mime": im.get("mimeType"),
        })
    material_data = []
    for i, mat in enumerate(materials):
        tex = mat.get("pbrMetallicRoughness", {}).get("baseColorTexture")
        material_data.append({
            "index": i, "name": mat.get("name"), "alpha_mode": mat.get("alphaMode", "OPAQUE"),
            "double_sided": mat.get("doubleSided", False),
            "base_texture_index": tex.get("index") if tex else None,
            "normal_texture": "normalTexture" in mat,
            "emissive_texture": "emissiveTexture" in mat,
            "extensions": sorted(mat.get("extensions", {})),
        })
    skeletons = []
    for i, sk in enumerate(skin):
        joint_info = [{
            "index": int(j), "name": nodes[j].get("name"),
            "translation": nodes[j].get("translation", [0,0,0]),
            "rotation": nodes[j].get("rotation", [0,0,0,1]),
            "scale": nodes[j].get("scale", [1,1,1]),
            "matrix": nodes[j].get("matrix"),
        } for j in sk.get("joints", [])]
        resthash = sha(json.dumps(joint_info, sort_keys=True,
                                  separators=(",",":")).encode("utf-8"))
        skeletons.append({
            "skin_index": i, "joint_count": len(joint_info), "joint_rest_sha256": resthash,
            "joint_name_sha256": sha(json.dumps(
                [x["name"] for x in joint_info], separators=(",",":")).encode("utf-8")),
            "inverse_bind_accessor": sk.get("inverseBindMatrices"),
            "inverse_bind_count": accessors[sk["inverseBindMatrices"]]["count"]
            if "inverseBindMatrices" in sk else 0,
        })
    return {
        "path": str(path), "sha256": sha(raw), "bytes": len(raw),
        "geometry": {
            "mesh_count": len(meshes), "primitive_count": sum(len(m["primitives"]) for m in meshes),
            "vertices_primitive_sum": vertex_total, "triangles_primitive_sum": triangle_total,
            "missing_uv0_primitives": uv_missing, "missing_normals": normal_missing,
            "missing_tangents": tangent_missing, "missing_skin_attrs": skin_attr_missing,
            "primitives_with_morph_targets": morph_primitive_count,
            "morph_targets_total": morph_count,
            "material_primitive_usage": dict(material_indices),
            "nontriangulated_primitives": sum(
                int(p.get("mode",4) != 4) for m in meshes for p in m["primitives"]),
        },
        "rest_geometry_bounds": aabb,
        "primitive_details": primitive_details,
        "skeletons": skeletons,
        "node_count": len(nodes), "scene_mesh_nodes": len(node_mesh_world),
        "animations": len(doc.get("animations", [])),
        "materials": material_data, "images": image_data,
        "extensions_used": doc.get("extensionsUsed", []),
        "limitations": [
            "Native Koikatsu geometry/parity cannot be inferred from GLB alone.",
            "UE LOD0 and UE compression/mips require an independent Unreal-native audit.",
            "Morphs may be baked into vertex rest coordinates rather than stored in glTF targets.",
            "Bounds are at glTF scene nodes before skin deformation.",
            "Matching joint count does not establish animation compatibility.",
        ],
    }


def run():
    ap = argparse.ArgumentParser()
    ap.add_argument("--glb", type=pathlib.Path, nargs="+", required=True)
    ap.add_argument("--out", type=pathlib.Path, required=True)
    args = ap.parse_args()
    checks = []
    for path in args.glb:
        if not path.is_file():
            checks.append({"path": str(path), "error": "NOT_PRESENT"})
            continue
        try:
            checks.append(audit(path))
        except Exception as exc:
            checks.append({"path": str(path), "error": str(exc)})
    report = {
        "schema": "GATE12_P0_GLTF_QUANT_V1",
        "status": "SOURCE_QUANTIFIED_NOT_NATIVE_KK_OR_UE_VISUAL_ACCEPTED",
        "files": checks,
        "rest_compatibility": {},
    }
    nonempty = [x for x in checks if x.get("skeletons")]
    if len(nonempty) >= 2:
        report["rest_compatibility"] = {
            "skin_joint_counts": [x["skeletons"][0]["joint_count"] for x in nonempty],
            "skin_joint_rest_sha256": [x["skeletons"][0]["joint_rest_sha256"] for x in nonempty],
            "joint_counts_equal": len(set(x["skeletons"][0]["joint_count"] for x in nonempty)) == 1,
            "rest_transforms_equal": len(set(x["skeletons"][0]["joint_rest_sha256"] for x in nonempty)) == 1,
            "gate09_common_animation_accepted": False,
        }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    for item in checks:
        g = item.get("geometry", {})
        print(pathlib.Path(item["path"]).name,
              "verts", g.get("vertices_primitive_sum"),
              "tris", g.get("triangles_primitive_sum"),
              "bones", (item.get("skeletons") or [{}])[0].get("joint_count"),
              "extent_cm", (item.get("rest_geometry_bounds") or {}).get("extent_cm"),
              "morphs", g.get("morph_targets_total"),
              "images", len(item.get("images", [])),
              "error", item.get("error", "-"))
    print("REPORT", args.out)
    return int(any("error" in x for x in checks))


if __name__ == "__main__":
    raise SystemExit(run())
