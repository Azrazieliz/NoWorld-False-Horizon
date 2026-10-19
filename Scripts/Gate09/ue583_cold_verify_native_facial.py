"""Gate09 read-only independent Unreal 5.8.3 cold-load of actual morph-target sequence."""
import unreal,pathlib,json,traceback,time
root=pathlib.Path(r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019")
dest=root/"Saved/Gate09QA/unreal_05_native_facial_cold.json"
path="/Game/Gate09_QA_20261019/AuthenticFaceSource/AN_G09_Luminous_NativeBlinkSmile_SourceProof"
meshpath="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh003Prim000/Mesh_03_Prim_00/SkeletalMeshes/Mesh_03_Prim_00"
res={"status":"STARTED","read_only":True,"asset":path,"mesh":meshpath}
def save():dest.write_text(json.dumps(res,indent=2,default=str)+'\n',encoding='utf8')
save()
try:
 assert "Gate09_Unreal583_Animation_QA_20261019" in str(unreal.Paths.get_project_file_path())
 seq=unreal.load_asset(path);mesh=unreal.load_asset(meshpath)
 assert isinstance(seq,unreal.AnimSequence) and isinstance(mesh,unreal.SkeletalMesh)
 target=mesh.get_editor_property("skeleton")
 assert target.get_path_name()==seq.get_editor_property("skeleton").get_path_name()
 available={str(t.get_name()).casefold() for t in mesh.get_editor_property("morph_targets")}
 model=seq.get_editor_property("controller").get_model_interface()
 res["duration"]=float(seq.get_play_length())
 res["model_frames"]=int(model.get_number_of_frames())
 res["mesh_native_morphs"]=len(available)
 names=set(str(x) for x in unreal.AnimationLibrary.get_animation_curve_names(seq,unreal.RawCurveTrackTypes.RCT_FLOAT))
 res["num_float_curves"]=model.get_number_of_float_curves()
 for m in ["get_curve_names","get_float_curves","get_curve_identifiers"]:
  if not hasattr(model,m):continue
  try:
   data=getattr(model,m)()
   res[m+"_repr"]=str(data)[:2500]
   for x in data:
    for prop in ["get_name","name","curve_name"]:
     try:
      v=getattr(x,prop)
      names.add(str(v() if callable(v) else v))
     except Exception:pass
    if isinstance(x,str):names.add(x)
  except Exception as e:res[m+"_error"]=str(e)[:240]
 res["native_identified_curve_names"]=sorted(names)
 res["all_identifiers_mappable"]=all(any(s.casefold() in str(n).casefold() for n in names)
 for s in ["Eye_Closed","Mouth_Distorted_Smile__Closed"])
 res["status"]="PASS_FACIAL_SOURCE_COLD_RELOAD" if res["all_identifiers_mappable"] and res["model_frames"]==60 else "PARTIAL_CURVE_API_REVIEW_REQUIRED"
except Exception:
 res["status"]="FAIL_FACIAL_COLD_RELOAD";res["error"]=traceback.format_exc()
finally:
 res["date_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 save();unreal.log("G09_FACIAL_COLD="+res["status"])
