"""Aggregate six read-only Koikatsu shard imports without copying source assets."""
import json,os
p=os.path.join(os.path.dirname(__file__),"phase2")
files=[os.path.join(p,"koikatsu_shard_audit.json")]+[os.path.join(p,"koikatsu_shard_%02d_audit.json"%i) for i in range(1,6)]
rows=[json.load(open(f,encoding='utf-8')) for f in files]
unique=set()
for row in rows:
 for m in row.get("mesh_metrics",[]):
  unique.update(m["shape_keys"])
out={"shards":len(rows),"all_executed":all(r["status"]=="EXECUTED" for r in rows),
 "mesh_parts_total":sum(r["mesh_objects"] for r in rows),
 "individual_shard_armature_bones":[list(r["armature_bones"].values()) for r in rows],
 "every_shard_has_121_bones":all(list(r["armature_bones"].values())==[121] for r in rows),
 "meshes_lacking_uv":sum(len(r["missing_uv_on_meshes"]) for r in rows),
 "vertices_total":sum(r["mesh_vertices_total"] for r in rows),
 "polygons_total":sum(r["mesh_polygons_total"] for r in rows),
 "blend_shapes_per_shard":[r["blend_shapes_total"] for r in rows],
 "distinct_shape_key_names":len(unique),
 "source_redistribution":"NOT_INCLUDED",
 "original_color_fidelity":"NOT_VALIDATED",
 "character_reconstruction":"NOT_DONE",
 "note":"Names can repeat between GLB shards; shape keys are not independent canonical character manifestations. Shards have their own armature copies, not a combined production rig."}
with open(os.path.join(p,"koikatsu_shards_summary.json"),"w",encoding="utf-8") as f:json.dump(out,f,indent=2)
print(json.dumps(out))
