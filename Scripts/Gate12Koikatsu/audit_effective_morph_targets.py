"""Audit morph targets that contain actual POSITION displacement, without deleting any.
Unreal may not create MorphTarget .uassets for zero-delta blend shapes.
"""
import pathlib,json,struct,math,collections
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
MAN=json.loads((R/"full_fidelity_primitive_shards/manifest_full_fidelity_primitives.json").read_text(encoding="utf8"))
records=[]
for sh in MAN["shards"]:
    p=pathlib.Path(sh["path"])
    with p.open("rb") as f:
        f.seek(12);sz,typ=struct.unpack("<II",f.read(8));doc=json.loads(f.read(sz))
    acc=doc["accessors"]
    prim=doc["meshes"][0]["primitives"][0]
    shapes=prim.get("targets",[])
    counts=collections.Counter()
    for idx,target in enumerate(shapes):
        a=acc[target["POSITION"]] if "POSITION" in target else None
        if a is None:counts["no_position"]+=1;continue
        if a.get("min") is not None and a.get("max") is not None:
            maxmag=max(abs(x) for x in a["min"]+a["max"])
            category="effective_position" if maxmag>1e-8 else "zero_position"
        else:category="unknown_no_bounds"
        counts[category]+=1
        if "NORMAL" in target:
            normal=acc[target["NORMAL"]]
            if normal.get("min") and normal.get("max"):
                counts["effective_normal" if max(abs(x) for x in normal["min"]+normal["max"])>1e-8 else "zero_normal"]+=1
    record={"mesh":sh["mesh_index"],"primitive":sh["primitive_index"],"total_targets":len(shapes),"summary":dict(counts)}
    records.append(record)
(R/"effective_morph_source_audit.json").write_text(json.dumps(records,indent=2),encoding="utf8")
for x in records:
    print("MESH=%s PRIM=%s TOTAL=%s"%(x["mesh"],x["primitive"],x["total_targets"]),x["summary"])
