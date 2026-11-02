"""Gate12 professional source-only Eastern Market partition.
Partition 508 genuine original Gate02 glTF objects into 12 balanced, independent
FBX sections. Never decimate, sculpt, reroll, change texture or bake fake boxes.
No source art overwrite; exact object/vertex/triangle count is the acceptance
condition before native Unreal integration of the same scene-local transforms.
"""
import bpy,json,hashlib,traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SOURCE=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019\anchors\eastern_market_town.glb")
OUT=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/eastern_market_original_object_shards"
REC=OUT/"eastern_market_original_object_shards_receipt.json"
STATUS={"schema":"gate12.gate02.eastern_market.lossless_object_partition.v1",
 "status":"STARTED","source_glb":str(SOURCE),"shards":[],"errors":[]}
def save():REC.write_text(json.dumps(STATUS,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
OUT.mkdir(parents=True,exist_ok=True)
save()
try:
 assert SOURCE.is_file()
 source_sha=hashlib.sha256(SOURCE.read_bytes()).hexdigest()
 assert source_sha=="a6373a82b629b667714f4e39b741ff66450d8ece338bf5eaf72431148d489e3c"
 STATUS["source_sha256"]=source_sha
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.gltf(filepath=str(SOURCE))
 objects=[o for o in bpy.data.objects if o.type=="MESH"]
 assert len(objects)==508,f"Original source object count changed: {len(objects)}"
 expected_vert=9455450
 expected_poly=10607212
 original_vert=sum(len(o.data.vertices) for o in objects)
 original_poly=sum(len(o.data.polygons) for o in objects)
 assert original_vert==expected_vert,(original_vert,expected_vert)
 assert original_poly==expected_poly,(original_poly,expected_poly)
 STATUS["original_mesh_count"]=len(objects)
 STATUS["original_vertices"]=original_vert
 STATUS["original_polygons"]=original_poly
 groups=[[] for _ in range(12)]
 totals=[0]*12
 for obj in sorted(objects,key=lambda o:(-len(o.data.polygons),o.name)):
  i=min(range(12),key=lambda idx:(totals[idx],idx))
  groups[i].append(obj)
  totals[i]+=len(obj.data.polygons)
 assert len({id(o) for g in groups for o in g})==len(objects)
 for i,group in enumerate(groups):
  path=OUT/f"eastern_market_source_section_{i:02d}.fbx"
  assert not path.exists(),f"Protected existing source partition: {path}"
  bpy.ops.object.select_all(action="DESELECT")
  for obj in group:obj.select_set(True)
  bpy.context.view_layer.objects.active=group[0]
  bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,
    object_types={"MESH"},apply_unit_scale=True,axis_forward="-Y",axis_up="Z",
    mesh_smooth_type="FACE",use_triangles=True,
    add_leaf_bones=False,bake_anim=False,path_mode="AUTO")
  record={"section":i,"FBX":str(path),"source_objects":[o.name for o in group],
   "mesh_count":len(group),"source_vertices":sum(len(o.data.vertices) for o in group),
   "source_polygons":sum(len(o.data.polygons) for o in group),
   "fbx_bytes":path.stat().st_size,
   "fbx_sha256":hashlib.sha256(path.read_bytes()).hexdigest()}
  STATUS["shards"].append(record)
  save()
  print("EASTERN_MARKET_SECTION",i,record["mesh_count"],
        record["source_vertices"],record["source_polygons"],flush=True)
 assert sum(s["mesh_count"] for s in STATUS["shards"])==len(objects)
 assert sum(s["source_vertices"] for s in STATUS["shards"])==original_vert
 assert sum(s["source_polygons"] for s in STATUS["shards"])==original_poly
 STATUS["status"]="ORIGINAL_EASTERN_MARKET_SOURCE_PRESERVED_12_FBXS_PENDING_NATIVE_UE"
except Exception:
 STATUS["status"]="FAILED_OR_PARTIAL"
 STATUS["errors"].append(traceback.format_exc()[-5000:])
finally:
 save()
 print("GATE12_EASTERN_MARKET_LOSSLESS_SOURCE_PARTITION="+STATUS["status"],flush=True)
