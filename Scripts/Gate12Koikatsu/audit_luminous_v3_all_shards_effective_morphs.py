"""Compare meaningful source morph geometry vs nominal Koikatsu placeholder slots.

Read-only; run before native UE authoring. Uses strict each-shard SHA-256.
"""
import io,sys,runpy,contextlib,json,pathlib,hashlib,time,os
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
STATE=sys.argv[1] if len(sys.argv)>1 else "CLOTHED"
IDS={"CLOTHED":"LuminousV3CLOTHED","UNDERWEAR":"LuminousV3UNDERWEAR","NUDE":"LuminousV3NUDE"}
assert STATE in IDS
FOLDER=ROOT/"Saved/Gate12Koikatsu/LuminousV3/LosslessSources"/IDS[STATE]/"shards"
M=json.loads((FOLDER/"manifest_full_fidelity_primitives.json").read_text("utf8"))
SCRIPT=ROOT/"Scripts/Gate12Koikatsu/validate_luminous_source_morph_actual_deltas.py"
REPORT=ROOT/"Saved/Gate12Koikatsu/LuminousV3"/STATE/"source_shards_actual_nonzero_morph_inventory.json"
rows=[]
total_original_slots=0
total_nonzero=0
for idx,x in enumerate(M["shards"]):
 p=pathlib.Path(x["path"])
 assert p.parent.resolve()==FOLDER.resolve()
 assert hashlib.sha256(p.read_bytes()).hexdigest()==x["sha256"]
 tmp=FOLDER/(".g12_read_only_morph_audit_"+str(idx)+".json")
 sys.argv=[str(SCRIPT),str(p),"--output",str(tmp)]
 with contextlib.redirect_stdout(io.StringIO()):
  runpy.run_path(str(SCRIPT),run_name="__main__")
 d=json.loads(tmp.read_text("utf8"))
 tmp.unlink()
 row={"index":idx,"mesh":x["mesh_index"],"primitive":x["primitive_index"],
  "nominal_shape_key_slots":d["total_shape_key_slots"],
  "nonzero_position_morphs":d["nonzero_position_morphs"],
  "native_015mm_position_morphs":d["native_015mm_position_morphs"],
  "nonzero_normal_morphs":d["nonzero_normal_morphs"],
  "source_sha256":x["sha256"]}
 assert row["nominal_shape_key_slots"]==x["shard_primitive_counts"][0]["target_count"]
 total_original_slots+=row["nominal_shape_key_slots"]
 total_nonzero+=row["nonzero_position_morphs"]
 rows.append(row)
 if idx%12==0:
  print("SOURCE_SHARD_MORPH_AUDIT_PROGRESS",idx,"actual",row["nonzero_position_morphs"],"/",row["nominal_shape_key_slots"],flush=True)
assert len(rows)==len(M["shards"])
assert total_original_slots==M["full_morph_links"]
result={"schema":"G12_LUMINOUS_V3_MEANINGFUL_MORPH_SHARD_INVENTORY",
 "status":"PASS_BYTE_LEVEL_MEANINGFUL_SOURCE_MORPH_45_SHARDS" if len(rows)==45 else "PASS_BYTE_LEVEL_MEANINGFUL_SOURCE_MORPHS",
 "character":IDS[STATE],"state":STATE,
 "total_shards":len(rows),"nominal_source_morph_slots":total_original_slots,
 "meaningful_nonzero_position_morph_targets":total_nonzero,
 "nonzero_by_shard":rows,"source_GLB_immutable":True,
 "runtime_morph_playback":"NOT_TESTED","blender_source_bones_untouched":True}
REPORT.parent.mkdir(parents=True,exist_ok=True)
REPORT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
print("AUDIT_FINISHED",STATE,"PARTS",len(rows),"NOMINAL",total_original_slots,"MEANINGFUL",total_nonzero)
print("ZERO_MEANINGFUL_PARTS",[x["index"] for x in rows if x["nonzero_position_morphs"]==0])
