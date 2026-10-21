"""Bind real existing 2s Unreal AnimSequence to all full-quality skinned meshes.
No new native code, no lossy mesh export, no changes to original world map.
This verifies serialized animation configuration; runtime motion requires device proof.
"""
import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
SRC="/Game/Experimental/Gate12Koikatsu/FullFidelityQA/Maps/L_KK_CompleteSource_UE583"
DST="/Game/Experimental/Gate12Koikatsu/FullFidelityQA/Maps/L_KK_SpineSwayAnimated_UE583"
ANIM="/Game/Experimental/Gate12Koikatsu/Animation/AN_ChikaHaruno_SpineSway_UE583"
out={"status":"STARTED","source":SRC,"destination":DST,"animation":ANIM,"binding":[]}
def save():
 (R/"sway_animation_level_bind_report.json").write_text(json.dumps(out,indent=2,default=str),encoding="utf8")
save()
try:
 if "OfflineGame_Gate12_Koikatsu_20261010" not in str(unreal.Paths.get_project_file_path()):
  raise RuntimeError("Not isolated branch")
 if unreal.EditorAssetLibrary.does_asset_exist(DST):raise RuntimeError("Refusing overwrite of existing QA scene")
 anim=unreal.load_asset(ANIM)
 if not isinstance(anim,unreal.AnimSequence):raise RuntimeError("AnimSequence missing")
 anim_skel=anim.get_editor_property("skeleton")
 out["source_animation_skeleton"]=anim_skel.get_path_name()
 out["animation_seconds"]=anim.get_play_length()
 out["animation_sampled_keys"]=anim.get_editor_property("number_of_sampled_keys")
 duplicated=unreal.EditorAssetLibrary.duplicate_asset(SRC,DST)
 if not duplicated:raise RuntimeError("Map duplication failed")
 world=unreal.EditorLoadingAndSavingUtils.load_map(DST)
 if not world:raise RuntimeError("Duplicated level did not load")
 all_skeletal=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SkeletalMeshActor)
 out["actor_count"]=len(all_skeletal)
 if len(all_skeletal)!=19:raise RuntimeError("Expected 19 full-fidelity skeletal actors")
 for actor in all_skeletal:
  comp=actor.skeletal_mesh_component
  mesh=comp.get_skeletal_mesh_asset()
  if not mesh:raise RuntimeError("Missing mesh component")
  skeleton=mesh.get_editor_property("skeleton")
  if skeleton.get_path_name()!=anim_skel.get_path_name():
   skeleton.add_compatible_skeleton(anim_skel)
   if not unreal.EditorAssetLibrary.save_loaded_asset(skeleton):
    raise RuntimeError("Failed to save compatible skeleton "+skeleton.get_path_name())
  comp.override_animation_data(anim_to_play=anim,is_looping=True,is_playing=True,position=0,play_rate=1)
  info=comp.get_editor_property("animation_data")
  out["binding"].append({"actor":actor.get_actor_label(),"mesh":mesh.get_path_name(),
     "mesh_skeleton":skeleton.get_path_name(),"serialized_mode":str(comp.get_animation_mode()),
     "serialized_animation_data":str(info)[:220]})
  save()
 out["saved_map"]=bool(unreal.EditorLoadingAndSavingUtils.save_map(world,DST))
 if not out["saved_map"]:raise RuntimeError("Map save failed")
 out["status"]="PASS_SERIALIZED_ANIMATED_SCENE"
except Exception as e:
 out["status"]="ERROR";out["error"]=str(e);out["trace"]=traceback.format_exc()[-1800:]
finally:save();unreal.log("G12_KK_BIND_SWAY_"+out["status"])
