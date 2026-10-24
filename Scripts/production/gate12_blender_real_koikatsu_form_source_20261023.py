"""Import authentic existing in-game KKBP PMX sources as editable Blender models.
Never fabricate geometry, mutate original Koikatsu files, or overwrite frozen art.
These are visually unpaired candidates until the card/export pairing is proved.
"""
import bpy,sys,pathlib,json,hashlib,traceback,datetime,time,addon_utils
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
K=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
OUT=R/"SourceAssets/Gate12/LuminousForms20261023"
opts=sys.argv[sys.argv.index("--")+1:]
assert len(opts)==2
name,stamp=opts
assert name in ("Awakened","Berserker","Grand")
pmx_dirs=[x for x in pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\Export_PMX").iterdir() if x.name.startswith(stamp)]
assert len(pmx_dirs)==1
src=pmx_dirs[0]
pmx=src/"model.pmx"
assert pmx.stat().st_size>1000000
sha=hashlib.sha256(pmx.read_bytes()).hexdigest()
dest=OUT/name/"CANDIDATE_NATIVE_FROM_EXISTING_KKBP_PMX"
dest.mkdir(parents=True,exist_ok=True)
blend=dest/(f"Luminous_{name}_SourceAuthentic_BodyRig.blend")
report_path=dest/"GATE12_NATIVE_KKBP_IMPORT_RECEIPT.json"
report={"schema":"noworld.gate12.native_koikatsu_candidate_source.v1",
 "character_version":name,"source_pmx":str(pmx),"source_sha256":sha,
 "source_export_directory":str(src),"status":"STARTED",
 "original_files_modified":False,"card_to_export_hash_proven":False,
 "canonical_geometry_accepted":False}
def save():
 report_path.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 if blend.exists():
  raise FileExistsError("Model already present; preserve instead of overwrite")
 sys.path[:0]=[str(K/"addons/KKBP_v9.5.0/unpacked_mmd_tools"),
              str(K/"addons/KKBP_v9.5.0/unpacked_mmd_tools/opencc_dependency"),
              str(K/"addons/KKBP_v9.5.0/unpacked_importer")]
 for addon in ("mmd_tools","kkbp_importer"):
  addon_utils.enable(addon,default_set=True,persistent=False)
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
 arms=[a for a in bpy.data.objects if a.type=="ARMATURE"]
 parts=[a for a in bpy.data.objects if a.type=="MESH"]
 report["duration_seconds"]=round(time.monotonic()-t,2)
 report["mesh_parts"]=len(parts)
 report["vertices"]=sum(len(m.data.vertices)for m in parts)
 report["armature_bones"]=sum(len(a.data.bones)for a in arms)
 report["morph_targets"]=sum(len(m.data.shape_keys.key_blocks)-1 for m in parts if m.data.shape_keys)
 report["materials"]=len(bpy.data.materials)
 assert len(arms)==1 and len(parts)>=3 and report["vertices"]>10000
 assert report["armature_bones"]>=700 and report["materials"]>=20
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),check_existing=False,compress=False)
 assert blend.stat().st_size>1000000
 report["editable_blend"]=str(blend)
 report["blend_sha256"]=hashlib.sha256(blend.read_bytes()).hexdigest()
 report["status"]="REAL_KKBP_MESH_RIG_MATERIALS_SAVED_UNPAIRED_TO_CARD"
except Exception as e:
 report["status"]="IMPORT_FAILED"
 report["exception"]=traceback.format_exc()[-2800:]
 print("SOURCE_IMPORT_FAILURE",repr(e),flush=True)
finally:
 save()
 print("SOURCE_IMPORT_RESULT",name,report["status"],
       "PARTS",report.get("mesh_parts"),"BONES",report.get("armature_bones"),flush=True)
