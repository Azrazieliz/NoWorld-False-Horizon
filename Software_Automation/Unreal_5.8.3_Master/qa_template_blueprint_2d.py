"""Inspect template content for Niagara and 2D animation, and compile a generated Blueprint.
Read-only plugin sample discovery except compile of isolated QA Blueprint.
"""
import unreal,json,pathlib,traceback
out=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\UE58_ComplementaryTemplatesAndBlueprint.json")
r={"engine":unreal.SystemLibrary.get_engine_version(),"cases":{}}
for package in ("/Niagara","/Engine","/Paper2D"):
 try:
  items=unreal.EditorAssetLibrary.list_assets(package,recursive=True,include_folder=False)
  templates=[str(n) for n in items if ("Niagara" in str(n) or "NS_" in str(n) or
    "FX_" in str(n) or "Sprite" in str(n) or "Flipbook" in str(n))]
  r["cases"][package]={"scanned":len(items),"candidate_names":templates[:75]}
 except Exception as e:r["cases"][package]={"error":str(e)}
bp=unreal.load_asset("/Game/NW_MasterHub_QA/NativeAuthoring_Test_20261015/NW_BP_RuntimeExtensionActor")
try:
 assert bp and isinstance(bp,unreal.Blueprint)
 r["blueprint_compilation"]={"before_generated":bool(bp.generated_class())}
 unreal.BlueprintEditorLibrary.compile_blueprint(bp)
 unreal.EditorAssetLibrary.save_loaded_asset(bp)
 after=unreal.load_asset(bp.get_path_name())
 r["blueprint_compilation"]["native_compiled"]=bool(after.generated_class())
 r["blueprint_compilation"]["asset"]=after.get_path_name()
except Exception:r["blueprint_compilation"]={"error":traceback.format_exc()[-1400:]}
for s in ("PaperSpriteFactory","PaperFlipbookFactory","PaperSprite","PaperFlipbook","PaperSpriteSheetImporter",
 "NiagaraEditorLibrary","NiagaraSystem","NiagaraSystemFactoryNew","NiagaraEmitter"):
 klass=getattr(unreal,s,None)
 r.setdefault("classes",{})[s]={"present":bool(klass),
 "methods":[m for m in dir(klass) if ("emitter" in m or "sprite" in m or "flipbook" in m or "create" in m)][:40] if klass else []}
out.write_text(json.dumps(r,indent=2))
print("NW_UE_COMPLEMENT="+json.dumps({"blueprint":r.get("blueprint_compilation"),
 "packages":{k:v.get("scanned",None) for k,v in r["cases"].items()},
 "paper2d":r["classes"]["PaperFlipbookFactory"]["present"]}))
