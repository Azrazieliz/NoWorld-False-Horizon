"""NoWorld shared four-software capability catalog & safe read-only search.
Use installed authoritative inventories: Blender RNA, UE Python, SM01 SQLite,
Gaea 2 node specifications. Never copy installed mod assets or canonical data.
"""
import argparse,collections,importlib.util,json,pathlib,sqlite3,sys,hashlib,datetime,re
HERE=pathlib.Path(__file__).resolve().parent
P=HERE.parent
B=P/"Blender_5.2.2"
U=P/"Unreal_5.8.3_Master"
G=P/"Gaea_2.3.1_Master"
DB=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\CATALOG\FULL_INSTALLED_MOD_LIBRARY.sqlite")
NODES=pathlib.Path(r"D:\UnrealProjects\NW_Blender_ReferenceReview_20261015\external_sources\gaea-mcp\src\gaea_mcp")
BLEND=B/"MASTER_API"/"BLENDER_5_2_RNA_CATALOG.json"
UNREAL=U/"MASTER_API"/"UE58_ExposedAPIs_Installed.json"
MATRIX=HERE/"CAPABILITY_MATRIX_LIVE.json"
ROUTES={
 "blender":{
 "api_search":B/"MASTER_API"/"search_methods.py",
 "architecture":B/"GATE02"/"architecture_job.py",
 "morphology":B/"GATE01"/"morphology_job.py",
 "weapon":B/"GATE01"/"weapon_job.py",
 "semantic_motion":B/"GATE09"/"semantic_motion_job.py",
 "fight_mocap":B/"GATE09"/"combat_capture_job.py",
 "grease_2d":B/"GATE09"/"grease_game2d_job.py",
 "kk_obj":B/"INTEGRATIONS"/"KK_BRIDGE"/"submit_kk_job.py"},
 "koikatsu":{
 "recipe_dry_run":pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\AUTOMATION\produce_from_reference_recipe.py"),
 "joint_wing_offline":B/"INTEGRATIONS"/"SM01_SM02"/"sm01_pack_native_8bone.py",
 "native_wing_test":B/"INTEGRATIONS"/"SM01_SM02"/"sm01_native_wing_accept.py",
 "mod_lookup":HERE/"software_catalog.py"},
 "gaea":{
 "graph":G/"gaea_graph_job.py",
 "native_build":G/"gaea_native_unattended_build.py",
 "doctor":NODES/"doctor.py",
 "blender_tiles":G/"gaea_blender_tiled_mesh.py",
 "node_search":HERE/"software_catalog.py"},
 "unreal":{
 "api_search":U/"MASTER_API"/"search_unreal.py",
 "capabilities":U/"probe_unreal_master_subsystems.py",
 "asset_import":U/"qa_batch_import.py",
 "asset_author":U/"native_asset_author.py",
 "pcg_fixture":U/"qa_pcg_procedural.py",
 "paper2d":U/"qa_paper2d_flipbook.py",
 "animation_validate":U/"qa_animation_validate.py"
 }}
EVIDENCE={
 "koikatsu_joint_wing":"D:/Tools/GameAssetPipeline/ProductionAutomation/Software_Automation/INTEGRATION_FIXTURES/SM01_SM02/wing_v12_20261015/NW_Wing_Maker_NativeAcceptance.json",
 "gaea_native":"D:/UnrealProjects/OfflineGame_BlenderMastery_20261015/Software_Automation/Gaea_2.3.1_Master/VALIDATION/GAEA_NATIVE_512_OFFICIAL_REPORT.json",
 "gaea_blender":"D:/UnrealProjects/OfflineGame_BlenderMastery_20261015/Software_Automation/Gaea_2.3.1_Master/VALIDATION/BLENDER_NATIVE_EROSION_TILES.json",
 "gaea_unreal":"D:/UnrealProjects/OfflineGame_BlenderMastery_20261015/Software_Automation/Gaea_2.3.1_Master/VALIDATION/UNREAL_NATIVE_EROSION_IMPORT.json",
 "unreal_native_assets":"D:/UnrealProjects/NW_BlenderGateHandoff_QA_20261015/UE58_DeclarativeAssetAuthor_5Systems_v2.json",
 "unreal_paper2d":"D:/UnrealProjects/NW_BlenderGateHandoff_QA_20261015/UE58_Paper2D_GP_NativeFlipbook_v2.json",
 "unreal_sequencer":"D:/UnrealProjects/NW_BlenderGateHandoff_QA_20261015/UE58_Sequencer_CameraKeys_V3.json",
 "unreal_pcg":"D:/UnrealProjects/NW_BlenderGateHandoff_QA_20261015/UE58_PCG_WiredGraph_v3.json"
}
def open_db():
 if not DB.is_file():raise FileNotFoundError(DB)
 return sqlite3.connect("file:"+DB.as_posix()+"?mode=ro",uri=True)
def node_catalog():
 sys.path.insert(0,str(NODES.parent))
 from gaea_mcp.terrain import NODE_SPECS,UNSAFE_NODES
 return NODE_SPECS,UNSAFE_NODES
def count():
 b=json.loads(BLEND.read_text(encoding="utf-8"))
 u=json.loads(UNREAL.read_text(encoding="utf-8"))
 specs,unsafe=node_catalog()
 db=open_db()
 try:
  counts={t:db.execute("select count(*) from "+t).fetchone()[0] for t in ("packages","assets")}
 finally:db.close()
 return b,u,specs,unsafe,counts
def build():
 b,u,specs,unsafe,counts=count()
 routes={software:{name:{"path":str(path),"script_exists":path.is_file()}
   for name,path in row.items()} for software,row in ROUTES.items()}
 evidence={name:{"path":str(pathlib.Path(path)),"exists":pathlib.Path(path).is_file()}
   for name,path in EVIDENCE.items()}
 record={
  "schema":"NOWORLD_SOFTWARE_CAPABILITY_MAPPING_V1",
  "created_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
  "purpose":"Four-software discoverability and Gate-operable routes; not claims of unlimited software mastery",
  "software":{
   "blender":{"version":b.get("blender"),"installed_operator_count":len(b["operators"]),
    "RNA_type_count":len(b["types"]),"user_facing_index":str(BLEND),
    "status":"NATIVE_IMPLEMENTED_TECHNICAL"},
   "unreal":{"engine":u.get("engine"),"exposed_python_api_count":u.get("count"),
    "domains":u.get("domains",{}),"user_facing_index":str(UNREAL),
    "status":"NATIVE_EDITOR_SANDBOX_VALIDATION"},
   "koikatsu":{"packages_indexed":counts["packages"],"assets_indexed":counts["assets"],
    "indexed_readonly":True,"source_db":str(DB),
    "status":"SM01_NATIVE_MAKER_SKINNED_ACCESSORY_VERIFIED"},
   "gaea":{"native_node_types_mapped":len(specs),
    "node_ids":sorted(specs),"unsafe_known_node_ids":sorted(unsafe),
    "node_parameters":{name:spec.defaults for name,spec in specs.items()},
    "node_ports":{name:spec.ports for name,spec in specs.items()},
    "status":"NATIVE_GRAPH_EROSION_BUILD_AND_EXPORT_VERIFIED_FOR_FIXTURE"}},
  "routes":routes,"native_evidence":evidence,
  "gate_ownership":{"Gate01":["koikatsu","blender"],"Gate02":["gaea","blender","unreal"],
    "Gate09":["blender","unreal"],"Gate12":["unreal","blender","gaea","koikatsu"]},
  "restrictions":["Never overwrite original Steam installation, mod archive or character card",
    "Never alter frozen Foundation, Registry v2, active Gate assets or production Unreal map",
    "Sandbox QA remains separate from canonical production acceptance",
    "Native asset imported does not imply functional gameplay or visual fidelity",
    "Gaea native GUI automation is graph- and geometry-specific, not universal",
    "Do not execute a route based only on a catalog entry without its native postcondition"]}
 MATRIX.write_text(json.dumps(record,indent=2,ensure_ascii=False),encoding="utf-8")
 print("NW_SOFTWARE_MATRIX="+json.dumps({"mapping":str(MATRIX),
  "blender_operators":len(b["operators"]),"unreal_api":u.get("count"),
  "koikatsu_mods":counts["packages"],"koikatsu_items":counts["assets"],
  "gaea_safe_nodes":len(specs),"broken_routes":[s+":"+op for s,v in routes.items() for op,x in v.items() if not x["script_exists"]]}))
def search(term,software,limit):
 q=re.findall(r"[a-z0-9_]+",term.lower());result=[]
 if software in ("all","blender"):
  b=json.loads(BLEND.read_text(encoding="utf-8"))
  for op in b["operators"]:
   title=op.get("id","");h=title+" "+op.get("description","")
   score=sum((10 if x in title.lower() else 2) for x in q if x in h.lower())
   if score:result.append((score,{"software":"blender","api":"bpy.ops."+title,
    "props":[x.get("identifier") for x in op.get("props",[])][:25]}))
 if software in ("all","unreal"):
  u=json.loads(UNREAL.read_text(encoding="utf-8"))
  for k,v in u["classes"].items():
   body=(k+" "+" ".join(v.get("methods",[]))+" "+v.get("description","")).lower()
   score=sum(11 if x in k.lower() else 2 for x in q if x in body)
   if score:result.append((score,{"software":"unreal","api":"unreal."+k,
    "domain":v.get("domain"),"methods":v.get("methods",[])[:35]}))
 if software in ("all","gaea"):
  specs,_=node_catalog()
  for name,spec in specs.items():
   s=name.lower()+" "+spec.note.lower()+" "+" ".join(spec.defaults)
   score=sum(12 if x in name.lower() else 2 for x in q if x in s)
   if score:result.append((score,{"software":"gaea","node":name,
    "defaults":spec.defaults,"ports":spec.ports,"note":spec.note}))
 if software in ("all","koikatsu"):
  conn=open_db()
  try:
   for t in q[:4]:
    for asset_id,name,cat,file in conn.execute(
     "select asset_id,display_name,category_no,filename from asset_search where asset_search match ? limit ?",
     (t+"*",limit)).fetchall():
     result.append((7,{"software":"koikatsu","asset_id":asset_id,"name":name,
      "category":cat,"source":file,"license":"existing installed mod - check permissions"}))
  finally:conn.close()
 result.sort(key=lambda x:-x[0])
 print(json.dumps({"query":term,"software":software,
  "results":[dict({"score":score},**item) for score,item in result[:limit]]},indent=2,ensure_ascii=False))
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--build",action="store_true")
 p.add_argument("--search");p.add_argument("--software",choices=["all","blender","koikatsu","gaea","unreal"],default="all")
 p.add_argument("--limit",type=int,default=10)
 args=p.parse_args()
 if args.build:build()
 elif args.search:search(args.search,args.software,min(max(1,args.limit),40))
 else:p.error("Specify --build or --search")
if __name__=="__main__":main()
