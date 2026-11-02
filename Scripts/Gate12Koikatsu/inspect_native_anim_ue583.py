import unreal,os,json,traceback,time
R=r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu"
out={"status":"STARTED","project":str(unreal.Paths.get_project_file_path()),"assets":[]}
def write():
 with open(os.path.join(R,"native_animation_inspection.json"),"w",encoding="utf8") as f:json.dump(out,f,indent=2,default=str)
write()
try:
 paths=[
 "/Game/Experimental/Gate12Koikatsu/Animation/AN_ChikaHaruno_SpineSway_UE583",
 "/Game/Experimental/Gate12Koikatsu/FullPrimitives/Mesh03Prim00",
 "/Game/Experimental/Gate12Koikatsu/FullFidelity/Mesh04",
 "/Game/Experimental/Gate12Koikatsu/FullFidelity/Mesh05"
 ]
 for path in paths:
  if unreal.EditorAssetLibrary.does_directory_exist(path):files=unreal.EditorAssetLibrary.list_assets(path,True,False)
  else:files=[path]
  for file in files:
   a=unreal.load_asset(file)
   if not a:continue
   cls=a.get_class().get_name()
   if cls not in ("AnimSequence","Skeleton","SkeletalMesh"):continue
   row={"path":str(file),"class":cls}
   if isinstance(a,unreal.SkeletalMesh):
    s=a.get_editor_property("skeleton")
    row["skeleton"]=s.get_path_name() if s else None
    row["morph_names"]=[x.get_name() for x in a.get_editor_property("morph_targets")][:12]
    row["bone_count"]=len(s.get_editor_property("bone_tree")) if s else -1
   if cls=="AnimSequence":
    for name in ["skeleton","sequence_length","number_of_sampled_keys","rate_scale","additive_anim_type","raw_curve_data","compressed_data_structure","data_model"]:
     try:
      val=a.get_editor_property(name)
      if name=="skeleton":row[name]=val.get_path_name() if val else None
      else:row[name]=str(val)[:250]
     except Exception as e:row[name+"_error"]=str(e)[:140]
    for name in ["get_play_length","get_number_of_sampled_keys","get_sampling_frame_rate","get_data_model"]:
     try:row[name]=str(getattr(a,name)())[:400]
     except Exception as e:row[name+"_error"]=str(e)[:140]
    try:row["track_names"]=[str(n) for n in unreal.AnimationBlueprintLibrary.get_bone_track_names(a)]
    except Exception as e:row["track_names_error"]=str(e)
    try:row["curve_names"]=[str(n) for n in unreal.AnimationBlueprintLibrary.get_animation_curve_names(a,unreal.RawCurveTrackTypes.RCT_FLOAT)]
    except Exception as e:row["curve_names_error"]=str(e)
   out["assets"].append(row);write()
 out["status"]="COMPLETE"
except Exception as e:out["status"]="ERROR";out["error"]=str(e);out["trace"]=traceback.format_exc()
finally:write();unreal.log("GATE12_NATIVE_ANIM_INSPECT_"+out["status"])
