"""One-command verification of the validated four-software production toolchain.
Never invent success or change installed software. Reads only actual native receipts.
"""
import pathlib,json,datetime,sys
HERE=pathlib.Path(__file__).resolve().parent
P=HERE.parent
QA=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
G=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
KK=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\INTEGRATION_FIXTURES\SM01_SM02\wing_v12_20261015")
checks=[]
def inspect(name,path,predicate,detail):
 try:
  value=json.loads(path.read_text(encoding="utf-8-sig"))
  passed=bool(predicate(value))
  record={"name":name,"passed":passed,"evidence_path":str(path),"detail":detail(value)}
 except Exception as e:record={"name":name,"passed":False,"evidence_path":str(path),"error":repr(e)}
 checks.append(record)
inspect("supported_routes",HERE/"ROUTE_PREFLIGHT_CHECKS.json",
 lambda x:x["routes_total"]>=26 and x["routes_passed"]==x["routes_total"],
 lambda x:{"total":x["routes_total"],"passed":x["routes_passed"]})
inspect("four_software_live_bridge",HERE/"LIVE_FOUR_SOFTWARE_ROUTER_SMOKE.json",
 lambda x:x["status"]=="PASS_FOUR_NATIVE_CATALOG_ROUTE_LINKS" and len(x["checks"])==4,
 lambda x:{"links":[v["software"] for v in x["checks"]]})
inspect("resumable_sequential_pipeline",pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\Unified_PipelineSmoke_20261015_2006\checkpoint.json"),
 lambda x:x["status"]=="PASS_ALL_STAGES" and len(x["steps"])==4 and all(s["ok"] for s in x["steps"]),
 lambda x:{"completed":len(x["steps"]),"status":x["status"]})
inspect("KK_Maker_native_skinned_wing",KK/"NW_Wing_Maker_NativeAcceptance.json",
 lambda x:x["status"]=="KK_NATIVE_CARD_AND_SKIN_PASS" and x["max_vertex_deformation"]>0.1 and x["max_undo_deviation"]<1e-4,
 lambda x:{"status":x["status"],"deformation":x["max_vertex_deformation"],"undo":x["max_undo_deviation"]})
inspect("Gaea_official_native_erosion",P/"Gaea_2.3.1_Master"/"VALIDATION"/"GAEA_NATIVE_512_OFFICIAL_REPORT.json",
 lambda x:x["Resolution"]==512 and x["NodeCount"]>=7 and x["Result"] in ("Success","AlreadyBuilt"),
 lambda x:{"resolution":x["Resolution"],"nodes":x["NodeCount"],"native_result":x["Result"]})
inspect("Blender_Gaea_tiled_UVs_0_seams",P/"Gaea_2.3.1_Master"/"VALIDATION"/"BLENDER_NATIVE_EROSION_TILES.json",
 lambda x:x["status"]=="BLENDER_TILED_TERRAIN_PASS" and x["tile_count"]==4 and x["maximum_seam_error_m"]<1e-5,
 lambda x:{"tiles":x["tile_count"],"seam_m":x["maximum_seam_error_m"]})
inspect("Unreal_Gaea_four_native_meshes",P/"Gaea_2.3.1_Master"/"VALIDATION"/"UNREAL_NATIVE_EROSION_IMPORT.json",
 lambda x:x["status"]=="PASS_NATIVE_IMPORT" and x["native_reloaded_count"]>=4 and x["source_untouched"],
 lambda x:{"count":x["native_reloaded_count"],"status":x["status"]})
inspect("Unreal_native_editor_five_domains",QA/"UE58_DeclarativeAssetAuthor_5Systems_v2.json",
 lambda x:x["full_success"] and x["success_count"]==5,
 lambda x:{"native_assets":x["success_count"],"types":[v["type"] for v in x["results"]],
 "niagara_emitter_functional":False})
inspect("Unreal_Paper2D_flipbook",QA/"UE58_Paper2D_GP_NativeFlipbook_v2.json",
 lambda x:x["status"]=="PASS_NATIVE_PAPER2D_FLIPBOOK" and x["native_key_sprites"]==7,
 lambda x:{"sprites":x["native_key_sprites"],"flipbook":x["flipbook"]})
inspect("Unreal_real_anim_pose_motion",QA/"UE58_Gate09_MotionFidelity_v2.json",
 lambda x:x["status"]=="PASS_NATIVE_ANIMATION_MOTION" and len(x["assets"])==2 and all(v.get("moving_bone_count",0)>=3 for v in x["assets"]),
 lambda x:{"moving_bones":[v.get("moving_bone_count",0) for v in x["assets"]]})
report={"schema":"NOWORLD_MASTER_TECHNICAL_ACCEPTANCE_V1",
 "verified_at_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
 "status":"PASS_TECHNICAL_PROOFS" if all(x["passed"] for x in checks) else "BLOCKED",
 "checks_passed":sum(x["passed"] for x in checks),
 "checks_total":len(checks),"checks":checks,
 "artist_quality_accepted":False,"Rakabi_2D_to_3D_accepted":False,
 "production_Android_integration_accepted":False,
 "native_Unreal_Landscape_WorldPartition_accepted":False,
 "universal_Gaea_nodes_or_GUI_automation_accepted":False}
path=HERE/"CAPABILITY_ACCEPTANCE_VERIFIED.json"
path.write_text(json.dumps(report,indent=2),encoding="utf-8")
print("NW_MASTER_ACCEPTANCE="+json.dumps({"status":report["status"],"pass":report["checks_passed"],
 "total":report["checks_total"],
 "failures":[x for x in checks if not x["passed"]]}))
if report["status"]!="PASS_TECHNICAL_PROOFS":sys.exit(2)
