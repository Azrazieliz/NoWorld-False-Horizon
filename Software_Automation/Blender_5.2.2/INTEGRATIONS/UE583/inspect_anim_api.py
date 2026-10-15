"""Isolated UE5.8 inspect available animation-authoring Python APIs; no production Gate."""
import unreal,json,os,traceback
root=r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015"
test={"engine":unreal.SystemLibrary.get_engine_version(),
 "script":"inspect_available_anim_api","status":"STARTED"}
try:
 for name in ("AnimationLibrary","AnimNotify","AnimNotify_PlaySound",
 "AnimNotify_PlayParticleEffect","AnimNotifyState_TimedParticleEffect",
 "ControlRigBlueprint","IKRigDefinition","IKRetargeter","AssetToolsHelpers",
 "AnimSequenceFactory","AnimSeqExportOption"):
  ob=getattr(unreal,name,None)
  test[name]={"exists":bool(ob),
    "methods":[x for x in dir(ob) if ("notify" in x.lower() or "anim_seq" in x.lower() or "retarget" in x.lower())][:30] if ob else []}
 test["assets"]=[a for a in unreal.EditorAssetLibrary.list_assets("/Game/NW_BlenderImportTests",recursive=True,include_folder=False) if "Anim" in a or "Skeletal" in a]
 test["status"]="SUCCESS"
except Exception:test["status"]="FAILED";test["error"]=traceback.format_exc()
with open(os.path.join(root,"ue_anim_api_inventory.json"),"w") as f:json.dump(test,f,indent=2)
print("NW_UE_ANIM_API="+json.dumps(test))
