import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
SOURCE="/Game/Experimental/Gate12Koikatsu/FullFidelityQA/Maps/L_KK_CompleteSource_UE583"
TARGET="/Game/Experimental/Gate12Koikatsu/FullFidelityQA/Maps/L_KK_AnimatedRigMorph_UE583"
out={"status":"STARTED","source":SOURCE,"target":TARGET,"fixture_only":True}
try:
 project=str(unreal.Paths.get_project_file_path())
 if "OfflineGame_Gate12_Koikatsu_20261010" not in project:raise RuntimeError("Wrong project!")
 if unreal.EditorAssetLibrary.does_asset_exist(TARGET):raise RuntimeError("Destination exists; refuse overwrite")
 duplicate=unreal.EditorAssetLibrary.duplicate_asset(SOURCE,TARGET)
 if not duplicate:raise RuntimeError("Map duplication failed")
 world=unreal.EditorLoadingAndSavingUtils.load_map(TARGET)
 if not world:raise RuntimeError("Cannot load duplicated map")
 actors=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SkeletalMeshActor)
 if len(actors)!=19:raise RuntimeError("Expected 19 original full-quality actors; got "+str(len(actors)))
 subject=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 actor=subject.spawn_actor_from_class(unreal.KKRuntimeAnimationQAActor,
                                       unreal.Vector(0,0,0),unreal.Rotator(pitch=0,yaw=0,roll=0))
 if not actor:raise RuntimeError("Runtime animator spawn failed")
 actor.set_actor_label("G12_KK_ActualSkinAndFacial_MorphAnimator")
 out["animator_path"]=actor.get_path_name()
 out["mesh_actors"]=len(actors)
 out["camera_actors"]=len(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor))
 out["player_starts"]=len(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerStart))
 out["saved"]=bool(unreal.EditorLoadingAndSavingUtils.save_map(world,TARGET))
 if not out["saved"]:raise RuntimeError("Map save failed")
 out["status"]="PASS"
except Exception as e:
 out["status"]="ERROR";out["error"]=str(e);out["trace"]=traceback.format_exc()[-1800:]
finally:
 (R/"animated_quality_map_authoring_report.json").write_text(json.dumps(out,indent=2,default=str),encoding="utf8")
 unreal.log("G12_ANIMATED_QA_MAP_"+out["status"])
