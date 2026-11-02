import json,struct,pathlib,collections
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
m=json.loads((R/"full_fidelity_primitive_shards/manifest_full_fidelity_primitives.json").read_text(encoding="utf8"))
native=json.loads((R/"full_fidelity_native_audit.json").read_text(encoding="utf8"))
actual={}
for a in native["parts"]:
 sk=[x for x in a["assets"] if x["class"]=="SkeletalMesh"]
 if sk:actual[(a["mesh_index"],a["primitive_index"])]=sk[0]["morphs"]
thresholds=[1e-8,1e-5,2e-5,2.5e-5,3e-5,5e-5,1e-4]
rows=[];tot=collections.Counter()
for sh in m["shards"]:
 p=pathlib.Path(sh["path"])
 with p.open("rb") as f:f.seek(12);sz,typ=struct.unpack("<II",f.read(8));d=json.loads(f.read(sz))
 magnitudes=[]
 for target in d["meshes"][0]["primitives"][0].get("targets",[]):
  if "POSITION" not in target: magnitudes.append(0);continue
  a=d["accessors"][target["POSITION"]]
  magnitudes.append(max((abs(x) for x in a.get("min",[0,0,0])+a.get("max",[0,0,0])),default=0))
 key=(sh["mesh_index"],sh["primitive_index"])
 obs=actual.get(key)
 rec={"mesh":key[0],"prim":key[1],"native_morphs":obs,
      "targets_source":len(magnitudes),"by_threshold":{str(t):sum(x>t for x in magnitudes) for t in thresholds}}
 rows.append(rec)
 for t in thresholds:
  if obs is not None and sum(x>t for x in magnitudes)!=obs:tot[t]+=1
(R/"native_morph_tolerance_report.json").write_text(json.dumps({"rows":rows,"mismatch_parts_by_threshold":{str(t):tot[t] for t in thresholds}},indent=2),encoding="utf8")
print("MISMATCHES_BY_THRESHOLD",dict(tot))
for x in rows:
 if x["mesh"]<=3:print("MORPH",x["mesh"],x["prim"],"source",x["targets_source"],"UE",x["native_morphs"],"threshold2.5e-5",x["by_threshold"]["2.5e-05"])
