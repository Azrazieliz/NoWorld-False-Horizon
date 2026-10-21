"""Gate 12 isolated UE 5.8.3 Blueprint construction preflight + morph material fix."""
import unreal,json,pathlib,traceback,runpy,time
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
O={"status":"STARTED","class_apis":{},"method_docs":{}}
try:
 names=("SubobjectDataSubsystem","SubobjectDataBlueprintFunctionLibrary","AddNewSubobjectParams",
 "BlueprintFactory","BlueprintEditorLibrary","Blueprint","Character","CharacterMovementComponent",
 "SkeletalMeshComponent","AnimInstance","K2Node_InputAxisEvent","K2Node_InputAction",
 "K2Node_CallFunction","K2Node_Event","InputAxisKeyMapping","MaterialEditingLibrary",
 "MaterialUsage","MaterialInstance","Material","AssetToolsHelpers","EditorAssetLibrary",
 "KismetSystemLibrary","EnhancedInputComponent","InputSettings","BlueprintFunctionLibrary")
 for n in names:
  try:
   cls=getattr(unreal,n)
   api={"exists":True,"doc":str(cls.__doc__)[:280]}
   sel=[m for m in dir(cls) if any(x in m.lower() for x in ("subobject","get_handle","blueprint","add_","attach","get_object","component","set_material","usage","input","node","graph","editor_property","class","set_parent","default_object")) and not m.startswith("__")]
   api["matching_methods"]={m:str(getattr(cls,m).__doc__)[:380] for m in sel[:36]}
   O["class_apis"][n]=api
  except Exception as e:O["class_apis"][n]={"exists":False,"error":str(e)[:300]}
 O["status"]="PASS"
except Exception as e:O["status"]="FAIL";O["error"]=str(e);O["trace"]=traceback.format_exc()[-1400:]
finally:
 (R/"preflight_character_blueprint_apis.json").write_text(json.dumps(O,indent=2,default=str),encoding="utf8")
 unreal.log("G12_CHAR_BLUEPRINT_PREFLIGHT_"+O["status"])
try:
 runpy.run_path(str(R/"fix_morph_target_material_usage.py"),run_name="__main__")
except Exception as e:unreal.log_error("G12_MORPH_MATERIAL_EXCEPTION "+str(e))
