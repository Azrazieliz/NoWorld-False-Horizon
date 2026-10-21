"""Unreal Editor Python read-only investigation of real Ruler UMG blueprint root."""
import unreal, json, pathlib
out=pathlib.Path(unreal.Paths.project_saved_dir())/"Gate12Readiness"/"issue25_ruler_widget_tree.json"
res={"schema":"GATE12_RULER_WIDGET_TREE_INSPECTION_V1","no_asset_mutation":True,"errors":[]}
try:
    src="/Game/NoWorld/UI/WBP_NoWorld_RulerShell"
    bp=unreal.load_asset(src)
    res["bp_class"]=bp.get_class().get_name() if bp else None
    if bp:
        for field in ("widget_tree","WidgetTree","generated_class"):
            try:
                value=bp.get_editor_property(field)
                res[field]=value.get_path_name() if value else None
                if field.lower()=="widget_tree" and value:
                    try:
                        root=value.get_editor_property("root_widget")
                        res["root_name"]=root.get_name() if root else None
                        res["root_class"]=root.get_class().get_name() if root else None
                    except Exception as e:res["root_error"]=str(e)
            except Exception as e:
                res[field+"_error"]=str(e)
    klass=unreal.load_class(None,src+"."+pathlib.Path(src).name+"_C")
    res["class_name"]=klass.get_path_name() if klass else None
    if klass:
        cdo=unreal.get_default_object(klass)
        for field in ("widget_tree","WidgetTree"):
            try:
                tree=cdo.get_editor_property(field)
                res["cdo_"+field]=tree.get_path_name() if tree else None
                if tree:
                    root=tree.get_editor_property("root_widget")
                    res["cdo_root"]=root.get_name() if root else None
                    res["cdo_root_class"]=root.get_class().get_name() if root else None
            except Exception as e:
                res["cdo_"+field+"_error"]=str(e)
except Exception as e: res["errors"].append(str(e))
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(res,indent=2)+"\n",encoding="utf-8")
unreal.log("GATE12_RULER_WIDGET_INSPECTION "+json.dumps(res))
