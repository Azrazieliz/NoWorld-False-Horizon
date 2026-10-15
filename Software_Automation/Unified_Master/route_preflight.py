"""Fail-closed startup audit of every declared Gate route. Does not run Blender,
Gaea, Unreal, or Maker; verifies API routing, absolute target files, and config names.
"""
import pathlib,json,datetime,importlib.util,sys,traceback,subprocess,hashlib
HERE=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location("gatectl",HERE/"gatectl.py")
router=importlib.util.module_from_spec(spec);spec.loader.exec_module(router)
P=HERE.parent
B=P/"Blender_5.2.2";G=P/"Gaea_2.3.1_Master";U=P/"Unreal_5.8.3_Master"
S=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation")
work=S/"INTEGRATION_FIXTURES"/"SM01_SM02"/"wing_v12_20261015"
native=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Gaea_FreshNativeBuild_Seed72931")
specs={
 "blender":{
  "api_search":{"query":"armature animation","limit":3},
  "architecture":{"spec":str(B/"GATE02"/"architecture_example.json")},
  "morphology":{"spec":str(B/"GATE01"/"morphology_example.json")},
  "weapon":{"spec":str(B/"GATE01"/"weapon_example.json")},
  "semantic_motion":{"spec":str(B/"GATE09"/"flight_example.json")},
  "fight_mocap":{"spec":str(B/"GATE09"/"combat_kick.json")},
  "grease_2d":{"spec":str(B/"GATE09"/"2d_transparent.json")},
  "kk_obj":{"source":str(B/"GATE02"/"architecture_example.json"),"native_job_id":"preflightOnly"}},
 "koikatsu":{
  "recipe_dry_run":{"recipe":str(S/"MASTER_PACKAGES"/"KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME"/"TEMPLATES"/"HARD_NONHUMAN_STRESS_RECIPE.json")},
  "joint_wing_offline":{"workdir":str(work)},
  "mod_lookup":{"query":"wing","limit":5},
  "native_wing_test":{"workdir":str(work)}},
 "gaea":{
  "graph":{"spec":str(G/"gaea_qa_config.json")},
  "native_build":{"project_dir":str(native)},
  "doctor":{},
  "blender_tiles":{"spec":str(G/"gaea_blender_job_example.json")},
  "node_search":{"query":"erosion","limit":3}},
 "unreal":{
  "capabilities":{},
  "asset_import":{"source":"D:/UnrealProjects/NW_BlenderGateHandoff_QA_20261015/NW_ModularWeapon_V2.fbx"},
  "animation_validate":{},
  "pcg_fixture":{},
  "asset_author":{"spec":str(U/"native_author_job_runner_v2.json")},
  "paper2d":{"source":"D:/UnrealProjects/NW_BlenderGateHandoff_QA_20261015/NW_GP_Game_2D_SpriteAtlas.png"},
  "api_search":{"query":"PCG Landscape", "limit":3}}
}
out=[]
for software,ops in specs.items():
 for op,param in ops.items():
  row={"software":software,"operation":op}
  job={"schema":"NOWORLD_SOFTWARE_JOB_V1","job_id":"Map_"+software+"_"+op,
   "owner_gate":{"blender":"Gate09","gaea":"Gate02","unreal":"Gate12","koikatsu":"Gate01"}[software],
   "test_only":True,"software":software,"operation":op,
   "requirements":{"preserve_originals":True,"max_walltime_seconds":180,"dedicated_gui":True},
   "params":param,"source_paths":[],"outputs":{"receipt_directory":"D:/UnrealProjects/NW_GatePreflight_NeverExecute"}}
  try:
   router.validate(job)
   cmd=router.command(job)
   if not pathlib.Path(cmd[0]).is_file():raise FileNotFoundError(cmd[0])
   for part in cmd[1:]:
    if part.lower().endswith(".py") and not pathlib.Path(part.split("=",1)[-1]).is_file():raise FileNotFoundError(part)
   row.update(preflight_passed=True,executable=cmd[0],script=next((a for a in cmd if a.endswith(".py")),None))
  except Exception as e:row.update(preflight_passed=False,error=repr(e))
  out.append(row)
log={"schema":"NOWORLD_ALL_GATE_ROUTES_PREFLIGHT_V1",
 "time_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
 "routes_total":len(out),"routes_passed":sum(x["preflight_passed"] for x in out),
 "native_execution_claimed":False,"checks":out}
path=HERE/"ROUTE_PREFLIGHT_CHECKS.json"
path.write_text(json.dumps(log,indent=2),encoding="utf-8")
print("NW_ROUTE_PREFLIGHT="+json.dumps({"routes":log["routes_total"],
 "passed":log["routes_passed"],"failures":[x for x in out if not x["preflight_passed"]]}))
if log["routes_total"]!=log["routes_passed"]:sys.exit(2)
