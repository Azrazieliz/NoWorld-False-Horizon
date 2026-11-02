"""Read-only actual Blueprint class inheritance/path investigation."""
import unreal, json
from pathlib import Path
s=("/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/"
   "BP_LuminousValentineV3_CLOTHED_RuntimeQA."
   "BP_LuminousValentineV3_CLOTHED_RuntimeQA_C")
k=unreal.load_class(None,s)
bases={
    "LuminousNative":unreal.load_class(None,"/Script/OfflineGame.OGLuminousV3PlayableCharacter"),
    "WorldPawn":unreal.load_class(None,"/Script/OfflineGame.OGWorldPrototypeCharacter"),
    "Character":unreal.Character.static_class(),
    "Actor":unreal.Actor.static_class(),
}
r={
    "path":k.get_path_name() if k else None,
    "class":str(k),
    "inheritance":{},
    "super_class_path":None,
    "default_qa_asset_paths":{},
}
if k:
    for label,b in bases.items():
        try:
            r["inheritance"][label] = bool(k.is_child_of(b))
        except Exception as exc:r["inheritance"][label] = "ERR:"+str(exc)
    try:r["super_class_path"]=k.get_super_class().get_path_name()
    except Exception as exc:r["super_class_path"]="ERR:"+str(exc)
    c=unreal.get_default_object(k)
    for n in ("clothed_native_parts","initial_wardrobe",
              "run_non_persistent_wardrobe_qa_cycle"):
        try:
            v=c.get_editor_property(n)
            r["default_qa_asset_paths"][n]=(
                str(len(v)) if hasattr(v,"__len__") else str(v))
        except Exception as exc:r["default_qa_asset_paths"][n]="ERR:"+str(exc)
out=Path(unreal.Paths.project_saved_dir())/"Gate12Readiness"/"qa_blueprint_class_provenance.json"
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(r,indent=2)+"\n",encoding="utf-8")
unreal.log("GATE12_QA_CLASS_PROVENANCE "+json.dumps(r))
