import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
o={"status":"STARTED","attempts":{}}
try:
 cls=unreal.AnimationCurveIdentifier
 obj=cls()
 o["attempts"]["dir"]=[x for x in dir(obj) if not x.startswith("__")]
 o["attempts"]["repr"]=str(obj)
 o["attempts"]["class_doc"]=str(cls.__doc__)
 tests=[
  ("constructor_kwargs",lambda:cls(curve_name=unreal.Name("Mouth_Happy"),curve_type=unreal.RawCurveTrackTypes.RCT_FLOAT)),
  ("set_attr_curve_name",lambda:setattr(obj,"curve_name",unreal.Name("Mouth_Happy"))),
  ("set_attr_CurveName",lambda:setattr(obj,"CurveName",unreal.Name("Mouth_Happy"))),
  ("set_prop_CurveName",lambda:obj.set_editor_property("CurveName",unreal.Name("Mouth_Happy"))),
  ("set_prop_curve_name",lambda:obj.set_editor_property("curve_name",unreal.Name("Mouth_Happy"))),
  ("set_properties",lambda:obj.set_editor_properties({"curve_name":unreal.Name("Mouth_Happy"),"curve_type":unreal.RawCurveTrackTypes.RCT_FLOAT})),
  ("import_text",lambda:obj.import_text("(CurveName=\"Mouth_Happy\",CurveType=RCT_Float)")),
  ("import_text_variant",lambda:obj._import_text("(CurveName=\"Mouth_Happy\",CurveType=RCT_Float)"))
 ]
 for name,fn in tests:
  try:o["attempts"][name]="OK "+str(fn())
  except Exception as e:o["attempts"][name]="ERR "+str(e)[:240]
 o["attempts"]["final_repr"]=str(obj)
 o["status"]="PASS"
except Exception as e:o["status"]="FAIL";o["error"]=str(e);o["trace"]=traceback.format_exc()[-1000:]
finally:
 (R/"curve_constructor_probe.json").write_text(json.dumps(o,indent=2,default=str),encoding="utf8")
