"""Real-source KKBP -> Blender -> FBX/GLB per one validated Luminous V3 state.

Run only under Blender 4.x background after Party export is fully sealed.
NO simulated models, outfit substitution, SFW removals, texture downsampling.
Original card/PMX/readme and the 10-file UE engineering lock are immutable.
"""
import bpy,sys,pathlib,json,time,hashlib,traceback,datetime,addon_utils,os,struct
BASE=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
IMPORTER=BASE/"addons/KKBP_v9.5.0"
ROOT=BASE/"characters/LUMINOUS_VALENTINE_V3/gate12_fresh_export_20261011"
tail=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
assert len(tail)==1 and tail[0] in ("CLOTHED","UNDERWEAR","NUDE"),"One explicit state required"
STATE=tail[0]
SOURCE=ROOT/"FRESH_PARTY_EXPORTS"/STATE
PMX=SOURCE/"model.pmx"
OUT=ROOT/"BLENDER_ROUNDTRIPS"/STATE
OUT.mkdir(parents=True,exist_ok=True)
RECEIPT=OUT/"FRESH_KKBP_BLENDER_FBX_GLB_REPORT.json"
record={"schema":"GATE12_LUMINOUS_V3_REAL_KKBP_BLENDER_STATE_1",
        "state":STATE,"status":"START","fresh_party_export_verified":False,
        "source_unfitted_GLBS_substituted":False,
        "textures_downscaled":False,"morphs_frozen":False,
        "sfw_erase_mode":False,"asset_ready_gate11":False,
        "UE_native_import":"NOT_TESTED","Android_cook":"NOT_TESTED",
        "timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"steps":[]}
def write():RECEIPT.write_text(json.dumps(record,indent=2,ensure_ascii=False,default=str)+"\n",encoding="utf8")
def step(code,info):
 record["steps"].append({"name":code,"details":info,"utc":datetime.datetime.now(datetime.timezone.utc).isoformat()})
 write();print("STATE_STEP",STATE,code,info,flush=True)
def sha(path):
 h=hashlib.sha256()
 with pathlib.Path(path).open("rb") as f:
  for part in iter(lambda:f.read(4*1024*1024),b""):h.update(part)
 return h.hexdigest()
def count():
 meshes=[x for x in bpy.data.objects if x.type=="MESH"]
 arms=[x for x in bpy.data.objects if x.type=="ARMATURE"]
 return {"meshes":len(meshes),"vertices":sum(len(x.data.vertices) for x in meshes),
 "triangulation_ready":sum(len(x.data.polygons) for x in meshes),
 "armatures":len(arms),"bones":sum(len(x.data.bones) for x in arms),
 "shape_keys":sum(max(0,len(x.data.shape_keys.key_blocks)-1) for x in meshes if x.data.shape_keys),
 "materials":len(bpy.data.materials),"images":len(bpy.data.images)}
write()
try:
 party=json.loads((ROOT/("FRESH_EXPORT_"+STATE+"_RECEIPT.json")).read_text(encoding="utf8"))
 assert party["status"]=="PASS_REAL_PARTY_KKBP_PMX_TEXTURE_METADATA_EXPORT_COPIED_AND_HASHED"
 assert party["pmx_sha256"]==sha(PMX) and party["state"]==STATE
 assert PMX.stat().st_size>3_000_000
 assert len(list(SOURCE.rglob("*.png")))>10
 record["source_pmx"]={"path":str(PMX),"sha256":sha(PMX),"bytes":PMX.stat().st_size}
 record["source_card_sha256"]=party["source_card_sha256"]
 record["fresh_party_export_verified"]=True
 step("SOURCE_FRESH_PMX_AND_MATERIALS_VERIFIED",record["source_pmx"])
 sys.path.insert(0,str(IMPORTER/"unpacked_mmd_tools"))
 sys.path.insert(0,str(IMPORTER/"unpacked_mmd_tools/opencc_dependency"))
 sys.path.insert(0,str(IMPORTER/"unpacked_importer"))
 for mod in ("mmd_tools","kkbp_importer"):
  addon_utils.enable(mod,default_set=True,persistent=False)
  assert mod in bpy.context.preferences.addons,mod
 step("BLENDER_KKBP_ADDONS_ACTIVE",{"blender":bpy.app.version_string})
 panel=bpy.context.scene.kkbp
 panel.import_dir=str(SOURCE)
 panel.shader_dropdown="E"
 panel.use_rigify=False
 panel.sfw_mode=False
 panel.fix_seams=True
 panel.use_outline=False
 panel.separate_clothes=False
 import kkbp_importer.common as common
 common.toggle_console=lambda:None
 started=time.monotonic()
 bpy.ops.kkbp.kkbpimport("EXEC_DEFAULT")
 data=count()
 assert data["meshes"]>1 and data["vertices"]>10000 and data["armatures"]>=1,data
 assert data["shape_keys"]>0 and data["bones"]>50 and data["images"]>3,data
 step("GENUINE_KKBP_BLENDER_SOURCE_IMPORTED",{"seconds":round(time.monotonic()-started,2),**data})
 blend=OUT/("Luminous_V3_"+STATE+"_source.blend")
 assert not blend.exists(),"Never overwrite original Blender state"
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),check_existing=False)
 step("BLENDER_SOURCE_SAVED",{"bytes":blend.stat().st_size,"sha256":sha(blend)})
 fbx=OUT/("Luminous_V3_"+STATE+".fbx")
 assert not fbx.exists(),"Never overwrite prior exported FBX"
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=False,add_leaf_bones=False,
                           path_mode="COPY",embed_textures=True)
 assert fbx.stat().st_size>1_000_000
 step("NATIVE_FBX_EXPORTED",{"bytes":fbx.stat().st_size,"sha256":sha(fbx)})
 glb=OUT/("Luminous_V3_"+STATE+".glb")
 assert not glb.exists(),"Never overwrite prior exported GLB"
 bpy.ops.export_scene.gltf(filepath=str(glb),export_format="GLB",export_skins=True,
     export_morph=True,export_yup=True)
 bb=glb.read_bytes()
 assert bb[:4]==b"glTF" and len(bb)>1_000_000
 size,tag=struct.unpack_from("<II",bb,12)
 assert tag==0x4e4f534a
 j=json.loads(bb[20:20+size])
 m=sum(len(v["primitives"]) for v in j.get("meshes",[]))
 morph=sum(len(p.get("targets",[])) for mesh in j.get("meshes",[]) for p in mesh["primitives"])
 info={"bytes":len(bb),"sha256":sha(glb),"glTF_meshes":len(j.get("meshes",[])),
       "primitives":m,"skin_count":len(j.get("skins",[])),"morph_target_refs":morph,
       "materials":len(j.get("materials",[])),"images":len(j.get("images",[]))}
 assert m>0 and len(j.get("skins",[]))>=1 and morph>0 and info["images"]>3,info
 step("GENUINE_FULL_QUALITY_GLB_EXPORTED",info)
 record["status"]="PASS_FRESH_KOIKATSU_TO_BLENDER_FBX_GLB_PRE_UNREAL"
 record["final_artifacts"]={"blend":str(blend),"fbx":str(fbx),"glb":str(glb),"glb_info":info}
 write()
 print("LUMINOUS_FRESH_ROUNDTRIP_OK",STATE,flush=True)
except Exception as exc:
 record["status"]="FAIL_OR_BLOCKED_BLENDER_IMPORT"
 record["error"]=repr(exc)
 record["traceback"]=traceback.format_exc()[-3800:]
 write()
 print("LUMINOUS_FRESH_ROUNDTRIP_ERROR",STATE,repr(exc),flush=True)
 traceback.print_exc()
 sys.exit(2)
