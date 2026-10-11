"""Gate12 Luminous V3 primary source KKBP PMX -> real Blender -> FBX + GLB.
No synthetic assets, no texture downscale, no morph removal. One state / process.
"""
import bpy,sys,pathlib,json,time,hashlib,traceback,datetime,addon_utils,os
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
BASE=ROOT/"addons"/"KKBP_v9.5.0"
STAGE=ROOT/"characters"/"LUMINOUS_VALENTINE_V3"/"gate12_fresh_export_20261011"
STATES=("CLOTHED","UNDERWEAR","NUDE")
args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
if len(args)!=1 or args[0] not in STATES:raise RuntimeError("Usage: blender -b --python script -- CLOTHED|UNDERWEAR|NUDE")
state=args[0]
src=STAGE/"FRESH_PARTY_EXPORTS"/state
receipt=json.loads((STAGE/("FRESH_EXPORT_"+state+"_RECEIPT.json")).read_text(encoding="utf-8-sig"))
card=json.loads((STAGE/"SEALED_INTAKE_RECEIPT.json").read_text(encoding="utf-8-sig"))["three_states"][state]
out=STAGE/"BLENDER_FRESH"/state
out.mkdir(parents=True,exist_ok=True)
report_path=out/"GATE12_FRESH_BLENDER_CONVERSION_RECEIPT.json"
report={"schema":"GATE12_LUMINOUS_V3_PRIMARY_3D_BLENDER_IMPORT_V1",
        "state":state,"character_identity":"LUMINOUS_VALENTINE_V3",
        "source_card_sha256":card["sha256"],"source_pmx_sha256":receipt["pmx_sha256"],
        "status":"STARTED","steps":[],"errors":[],"runtime_unreal_integrated":False,
        "textures_downsampled":False,"source_mesh_decimated":False,"blender_material_editing":False}
def save():
 report["updated_utc"]=datetime.datetime.now(datetime.timezone.utc).isoformat()
 report_path.write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
def log(step,data=None):
 report["steps"].append({"step":step,"detail":data,"time_utc":datetime.datetime.now(datetime.timezone.utc).isoformat()})
 print("GATE12_LUMINOUS",state,step,json.dumps(data,default=str)[:1300],flush=True)
 save()
def hashfile(f):
 h=hashlib.sha256()
 with f.open("rb") as handle:
  for block in iter(lambda:handle.read(4*1024*1024),b""):h.update(block)
 return h.hexdigest()
def metrics():
 meshes=[obj for obj in bpy.data.objects if obj.type=="MESH"]
 arms=[obj for obj in bpy.data.objects if obj.type=="ARMATURE"]
 return {
  "object_count":len(bpy.data.objects),"mesh_count":len(meshes),
  "vertices":sum(len(m.data.vertices) for m in meshes),
  "faces":sum(len(m.data.polygons) for m in meshes),
  "skin_armature_count":len(arms),
  "armature_bones":sum(len(a.data.bones) for a in arms),
  "shape_keys":sum(max(0,len(obj.data.shape_keys.key_blocks)-1) for obj in meshes if obj.data.shape_keys),
  "material_count":len(bpy.data.materials),"image_count":len(bpy.data.images),
  "image_metrics":[{"name":im.name,"size":list(im.size),"file":im.filepath,"source":im.source}
                   for im in list(bpy.data.images)[:120]],
 }
save()
try:
 pmx=src/"model.pmx"
 if hashfile(pmx)!=receipt["pmx_sha256"]:raise RuntimeError("Primary accepted PMX byte hash drift")
 if receipt["status"]!="PASS_REAL_PARTY_KKBP_PMX_TEXTURE_METADATA_EXPORT_COPIED_AND_HASHED":
  raise RuntimeError("Source export not accepted")
 log("VERIFIED_PRIMARY_KKBP_SOURCE",{"pmx":str(pmx),"size":pmx.stat().st_size})
 sys.path[:0]=[str(BASE/"unpacked_mmd_tools"),str(BASE/"unpacked_mmd_tools"/"opencc_dependency"),str(BASE/"unpacked_importer")]
 for module in ("mmd_tools","kkbp_importer"):
  addon_utils.enable(module,default_set=True,persistent=False)
  if module not in bpy.context.preferences.addons:raise RuntimeError("Importer addon failed "+module)
 log("BLENDER_ADDONS_REGISTERED",{"blender":bpy.app.version_string})
 cfg=bpy.context.scene.kkbp
 cfg.import_dir=str(src)
 cfg.shader_dropdown="E"
 cfg.use_rigify=False
 cfg.sfw_mode=False
 cfg.fix_seams=True
 cfg.use_outline=False
 cfg.separate_clothes=False
 import kkbp_importer.common as common
 common.toggle_console=lambda:None
 t=time.monotonic()
 bpy.ops.kkbp.kkbpimport("EXEC_DEFAULT")
 baseline=metrics()
 if baseline["mesh_count"]<2 or baseline["vertices"]<1000 or baseline["skin_armature_count"]<1:
  raise RuntimeError("Real imported source missing meshes or skeleton "+repr(baseline))
 log("KKBP_NATIVE_REAL_IMPORT",{"elapsed_s":round(time.monotonic()-t,2),
          **{k:v for k,v in baseline.items() if k!="image_metrics"}})
 report["native_blender_geometry"]=baseline
 # Non-destructive .blend for source audibility
 blend=out/("LUMINOUS_V3_"+state+"_original_import.blend")
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),check_existing=False,compress=False)
 if blend.stat().st_size<10000:raise RuntimeError("Empty Blender file")
 log("EDITABLE_BLENDER_SOURCE_SAVED",{"bytes":blend.stat().st_size,"sha256":hashfile(blend),"path":str(blend)})
 # Preserve source textures/skin weights, all morph targets.
 fbx=out/("LUMINOUS_V3_"+state+"_native.fbx")
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=False,
        add_leaf_bones=False,path_mode="COPY",embed_textures=True)
 if fbx.stat().st_size<100000:raise RuntimeError("Real FBX missing")
 log("FBX_NATIVE_SKELETON_EXPORTED",{"bytes":fbx.stat().st_size,"sha256":hashfile(fbx),"path":str(fbx)})
 glb=out/("LUMINOUS_V3_"+state+"_skinned.glb")
 bpy.ops.export_scene.gltf(filepath=str(glb),export_format="GLB",
     export_skins=True,export_morph=True,export_yup=True)
 if glb.stat().st_size<100000:raise RuntimeError("Real GLB missing")
 log("GLB_SKIN_MORPH_TEXTURE_EXPORTED",{"bytes":glb.stat().st_size,"sha256":hashfile(glb),"path":str(glb)})
 report["status"]="PASS_REAL_PRIMARY_PARTY_SOURCE_BLENDER_FBX_GLB_EXPORTED_PENDING_INDEPENDENT_VALIDATION"
except Exception as exc:
 report["status"]="FAIL_BLENDER_SOURCE_CONVERSION"
 report["errors"].append({"error":repr(exc),"traceback":traceback.format_exc()[-6000:]})
 print("GATE12_LUMINOUS_EXPORT_FAILURE",state,repr(exc),flush=True)
 traceback.print_exc()
 save()
 raise
finally:
 save()
