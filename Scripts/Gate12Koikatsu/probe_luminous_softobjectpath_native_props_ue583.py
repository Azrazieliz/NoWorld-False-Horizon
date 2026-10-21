import unreal,pathlib,json,traceback
w=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
root=w/"Saved/Gate12Koikatsu/LuminousV3"
r=json.loads((root/"native_121_wardrobe_runtime_switch_ue583.json").read_text("utf8"))
out={"status":"START"}
try:
 c=unreal.EditorAssetLibrary.load_blueprint_class(r["character"])
 d=unreal.get_default_object(c)
 a=d.get_editor_property("clothed_native_parts")
 x=a[0]
 out["type"]=str(type(x))
 out["repr"]=repr(x)
 out["dir"]=[n for n in dir(x) if not n.startswith("__")]
 out["export_text"]=x.export_text() if hasattr(x,"export_text") else None
 out["tuple"]=repr(x.to_tuple())
 out["dict"]=repr(x.to_dict())
 out["constructor_input_repr"]=repr(unreal.SoftObjectPath(r["parts"]["CLOTHED"]["source_paths"][0]))
 out["status"]="PASS"
except Exception as e:
 out["status"]="FAIL";out["exception"]=str(e);out["trace"]=traceback.format_exc()[-1500:]
(root/"softobjectpath_reflection_probe_ue583.json").write_text(json.dumps(out,indent=2)+"\n","utf8")
unreal.log("SOFTOBJECT_PROBE="+out["status"])
