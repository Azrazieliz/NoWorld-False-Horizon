"""Read-only original Blender shape-key delta audit for actual imported
Luminous Alter wardrobe. Diagnose FBX->UE morph-count gap without deleting,
rebaking, modifying textures or silently claiming parity.
"""
import bpy,sys,json,pathlib,hashlib,traceback,datetime,numpy as np
args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
assert len(args)==1 and args[0] in ("UNDERWEAR","NUDE")
state=args[0]
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016")/"BLENDER_FRESH"/state
source=ROOT/f"LUMINOUS_ALTER_{state}_original_import.blend"
assert pathlib.Path(bpy.data.filepath).resolve()==source.resolve()
original=json.loads((ROOT/"GATE12_FRESH_BLENDER_CONVERSION_RECEIPT.json").read_text())
assert original["status"]=="PASS_REAL_PRIMARY_PARTY_SOURCE_BLENDER_FBX_GLB_EXPORTED_PENDING_INDEPENDENT_VALIDATION"
expected=next(x["detail"]["sha256"] for x in original["steps"] if x["step"]=="EDITABLE_BLENDER_SOURCE_SAVED")
def sha256(p):
 h=hashlib.sha256()
 with open(p,"rb") as f:
  for b in iter(lambda:f.read(4*1024*1024),b""):h.update(b)
 return h.hexdigest()
assert sha256(source)==expected
out=ROOT/f"GATE12_ALTER_{state}_NATIVE_SHAPEDELTA_DETAILED_FIDELITY_AUDIT_20261022.json"
assert not out.exists(),"Preserve existing prior audit"
report={"schema":"noworld.gate12.alter.morph.original_blender_ue_gap.v1",
        "state":state,"status":"STARTED","source_blend_sha256":expected,
        "source_modified":False,"real_deformation_not_invented":True,
        "unit_note":"Absolute Blender source-space vertex delta",
        "zero_delta_epsilon":1e-7,
        "substantive_epsilon":1e-5,"parts":[]}
try:
 parts=sorted((o for o in bpy.data.objects if o.type=="MESH"),key=lambda obj:obj.name)
 for obj in parts:
  sk=obj.data.shape_keys
  keys=list(sk.key_blocks) if sk else []
  vertices=len(obj.data.vertices)
  row={"mesh":obj.name,"vertices":vertices,
       "source_shape_keys":max(0,len(keys)-1),"effective_morph_gt_1e7":0,
       "effective_morph_gt_1e5":0,"effective_morph_gt_1e4":0,
       "zero_delta_shape_keys":[],"nonzero_deformation_shape_keys":[]}
  if len(keys)>1:
   baseline=np.empty(vertices*3,dtype=np.float32)
   keys[0].data.foreach_get("co",baseline)
   actual=np.empty(vertices*3,dtype=np.float32)
   for key in keys[1:]:
    key.data.foreach_get("co",actual)
    delta=float(np.max(np.abs(actual-baseline)))
    if delta>1e-7:
     row["effective_morph_gt_1e7"]+=1
     row["nonzero_deformation_shape_keys"].append(key.name)
    else:row["zero_delta_shape_keys"].append(key.name)
    if delta>1e-5:row["effective_morph_gt_1e5"]+=1
    if delta>1e-4:row["effective_morph_gt_1e4"]+=1
  report["parts"].append(row)
  print("GATE12_TRUE_ALTER_MORPH_DELTA",state,obj.name,
    "source",row["source_shape_keys"],"nonzero",row["effective_morph_gt_1e7"],
    "significant",row["effective_morph_gt_1e5"],flush=True)
 report["source_total_shape_keys"]=sum(p["source_shape_keys"] for p in report["parts"])
 report["effective_gt_1e7"]=sum(p["effective_morph_gt_1e7"] for p in report["parts"])
 report["effective_gt_1e5"]=sum(p["effective_morph_gt_1e5"] for p in report["parts"])
 report["effective_gt_1e4"]=sum(p["effective_morph_gt_1e4"] for p in report["parts"])
 assert report["source_total_shape_keys"]==original["native_blender_geometry"]["shape_keys"]
 report["status"]="REAL_ORIGINAL_SHAPEKEY_DELTA_AUDIT_COMPLETED_NO_SOURCE_EDITS"
except Exception:
 report["status"]="ORIGINAL_SHAPEKEY_DELTA_AUDIT_INCOMPLETE"
 report["error"]=traceback.format_exc()[-4000:]
finally:
 report["time_utc"]=datetime.datetime.now(datetime.timezone.utc).isoformat()
 out.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
 print("GATE12_MORPH_REAL_AUDIT",state,report["status"],flush=True)
