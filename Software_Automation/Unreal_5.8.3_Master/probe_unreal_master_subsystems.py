"""Actual Unreal Engine 5.8.3 subsystem availability in isolated QA project."""
import unreal,json,pathlib
OUT=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\Unreal_Master_CapabilityAudit.json")
names=["BlueprintFactory","KismetEditorUtilities","Blueprint","EditorAssetLibrary","AssetTools","EditorActorSubsystem","LevelSequenceFactoryNew","LevelSequence","MovieScene3DTransformTrack","PCGGraph","PCGGraphFactory","PCGComponent","NiagaraSystemFactoryNew","NiagaraSystem","NiagaraEmitter","MaterialFactoryNew","MaterialEditingLibrary","MaterialInstanceConstantFactoryNew","MaterialInstanceConstant","Landscape","LandscapeProxy","LandscapeImportHelper","LandscapeEditorUtils","WorldPartitionSubsystem","WorldPartitionBlueprintLibrary","InputAction","InputMappingContext","GameplayAbility","GameplayEffect","ControlRigBlueprint","ControlRigBlueprintFactory","IKRetargeterController","IKRigController","AnimSequence","AnimationLibrary","MoviePipelineQueue","EditorValidatorSubsystem","DataValidationSubsystem","SaveGame","PrimaryDataAsset","DataTable","DataTableFactory","AssetRegistryHelpers","SystemLibrary","PythonScriptLibrary"]
r={"version":unreal.SystemLibrary.get_engine_version(),"classes":{}}
for name in names:
 cls=getattr(unreal,name,None)
 r["classes"][name]={"available":bool(cls),"methods":[n for n in dir(cls) if any(x in n.lower() for x in ("create","new_","add_","get_","import","save","compile","validate","build","spawn"))][:50] if cls else []}
OUT.write_text(json.dumps(r,indent=2))
print("NW_UE_MASTER_AUDIT="+json.dumps({"engine":r["version"],"available":sum(x["available"] for x in r["classes"].values()),"total":len(names),"missing":[k for k,x in r["classes"].items() if not x["available"]]}))
