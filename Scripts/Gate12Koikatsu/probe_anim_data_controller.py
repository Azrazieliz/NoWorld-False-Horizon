import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
out={"status":"STARTED","classes":{},"methods":{}}
try:
 for n in ("AnimSequenceFactory","AnimSequence","AnimDataController","AnimationDataController","AnimDataModel","AnimationBlueprintLibrary","AnimPose","AnimPoseEvaluationOptions","AnimationLibrary","AssetToolsHelpers","AnimSequenceFactory"):
  try:
   cls=getattr(unreal,n)
   out["classes"][n]="PRESENT"
   out["methods"][n]={m:str(getattr(cls,m).__doc__)[:800] for m in dir(cls) if (("track" in m or "frame" in m or "controller" in m or "create" in m or "skeleton" in m or "rate" in m or "set_play" in m or "sequence_length" in m) and not m.startswith("_"))}
  except Exception as e:out["classes"][n]=str(e)
 seq=unreal.load_asset("/Game/Experimental/Gate12Koikatsu/Animation/AN_ChikaHaruno_SpineSway_UE583")
 for m in ["get_controller","get_data_model","get_number_of_sampled_keys","get_play_length","get_sampling_frame_rate"]:
  try:
   v=getattr(seq,m)()
   out["methods"]["EXISTING_SEQUENCE_"+m]=str(v)[:600]
   if m=="get_controller":
    out["methods"]["INSTANCE_CONTROLLER"]={a:str(getattr(v,a).__doc__)[:850] for a in dir(v) if ("track" in a or "frame" in a or "model" in a or "bracket" in a or "resize" in a or "rate" in a) and not a.startswith("_")}
  except Exception as e:out["methods"]["EXISTING_SEQUENCE_"+m+"_error"]=str(e)[:500]
 out["status"]="PASS"
except Exception as e:out["status"]="FAIL";out["error"]=str(e);out["trace"]=traceback.format_exc()[-1800:]
finally:
 (R/"anim_controller_api_probe.json").write_text(json.dumps(out,indent=2,default=str),encoding="utf8")
 unreal.log("G12_CONTROLLER_PROBE_"+out["status"])
