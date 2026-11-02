import json,struct,re,pathlib,collections
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
p=R/"full_fidelity_primitive_shards/Mesh_03_Prim_02.glb"
with p.open("rb") as f:f.seek(12);sz,typ=struct.unpack("<II",f.read(8));j=json.loads(f.read(sz))
m=j["meshes"][0];prim=m["primitives"][0];names=m.get("extras",{}).get("targetNames",[])
audit=json.loads((R/"full_fidelity_native_audit.json").read_text())
entry=[x for x in audit["parts"] if x["mesh_index"]==3 and x["primitive_index"]==2][0]
unreal=[x for x in entry["assets"] if x["class"]=="SkeletalMesh"][0]["morph_names"]
norm=lambda s:re.sub(r"[^a-z0-9]","",s.lower())
got=set(norm(n) for n in unreal)
rows=[]
for i,target in enumerate(prim["targets"]):
    if "POSITION" not in target:continue
    a=j["accessors"][target["POSITION"]]
    mag=max(abs(v) for v in a.get("min",[0,0,0])+a.get("max",[0,0,0]))
    if mag>1e-8:
        name=names[i] if i<len(names) else "#"+str(i)
        rows.append({"i":i,"name":name,"magnitude_m":mag,"native_found":norm(name) in got})
missing=[x for x in rows if not x["native_found"]]
res={"expected_nonzero":len(rows),"native_morph_count":len(unreal),"source_morph_names":names,"native_morph_names":unreal,
     "missing_named":missing,"effective":rows,"unreal_names_sample":unreal[:60],"summary":dict(collections.Counter("found" if x["native_found"] else "missing" for x in rows))}
(R/"missing_morph_03_02_diagnosis.json").write_text(json.dumps(res,indent=2,ensure_ascii=False),encoding="utf8")
print("EFFECTIVE",len(rows),"UE",len(unreal),"NAMED_MISSING",len(missing))
for x in sorted(missing,key=lambda x:-x["magnitude_m"])[:30]:print("MISSING",x["i"],x["name"],"DELTA_m",x["magnitude_m"])
print("NATIVE_SAMPLE",unreal[:15])
