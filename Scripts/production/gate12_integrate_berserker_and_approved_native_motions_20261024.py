"""One isolated UE editor session integrates the REAL native Berserker skeletal
Blueprint, user-approved shared motion source derivatives, and selective cook.
Existing original Base/Alter gameplay assets and user config never overwritten.
"""
import unreal,runpy,traceback,pathlib,json,datetime
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
tasks=[
 ("stage_native_berserker_actor","gate12_stage_real_berserker_version_actor_unreal_20261023.py"),
 ("derive_real_source_native_motions","gate12_import_shared_native_motions_on_real_berserker_20261023.py"),
 ("selective_original_native_cook","gate12_cook_selected_real_native_interaction_motions_20261023.py")]
report={"schema":"noworld.gate12.real.native.integration.v1","at":datetime.datetime.now(datetime.timezone.utc).isoformat(),"steps":[]}
out=R/"Saved/Gate12Readiness/GATE12_NATIVE_BERSERKER_INTERACTION_COOK_INTEGRATION_20261024.json"
for label,script in tasks:
 row={"name":label,"status":"STARTED"}
 try:
  runpy.run_path(str(R/"Scripts/production"/script),run_name="__main__")
  row["status"]="PYTHON_STEP_EXECUTED_VERIFY_OWN_RECEIPT"
 except Exception:
  row["status"]="ERROR"
  row["error"]=traceback.format_exc()[-3000:]
 report["steps"].append(row)
 out.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
 unreal.SystemLibrary.collect_garbage()
 unreal.log("G12_NATIVE_INTEGRATION_STEP "+label+"="+row["status"])
