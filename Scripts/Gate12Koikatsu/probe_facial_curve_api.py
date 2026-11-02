import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
o={"status":"STARTED","classes":{}}
try:
 for name in ("AnimationCurveIdentifier","RichCurveKey","RawCurveTrackTypes","CurveFlags","AnimDataController"):
  try:
   cls=getattr(unreal,name)
   o["classes"][name]={"repr":str(cls),"doc":str(cls.__doc__)[:2400],
                       "public":{n:str(getattr(cls,n).__doc__)[:450] for n in dir(cls)
                          if n in ("curve_name","curve_type","curve_flags","time","value","interp_mode","set_curve_keys","add_curve","set_curve_key","RCT_FLOAT","ACF_DEFAULT")
                          or n.startswith("get_editor")}}
  except Exception as e:o["classes"][name]={"ERROR":str(e)}
 o["status"]="PASS"
except Exception as e:o["status"]="FAIL";o["error"]=str(e);o["trace"]=traceback.format_exc()[-1000:]
finally:
 (R/"probe_facial_curve_api.json").write_text(json.dumps(o,indent=2,default=str),encoding="utf8")
