"""Live exact-native, cross-wardrobe shared motion acceptance; no asset changes.
Tests SAME semantic action on THREE genuine unrelated Alter wardrobe skeletons
through production constructor-owned native controller, not a fabricated mesh.
"""
import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
OUT=R/"Saved/Gate12AnimationAcceptance/GATE12_POINT1_NATIVE_CROSSWARDROBE_20261023.json"
OUT.parent.mkdir(parents=True,exist_ok=True)
r={"schema":"noworld.gate12.point1.crosswardrobe.realnative.v1","status":"START","participants":[],"scene_roles":"NOT_TESTED_NO_AUTHORED_PARTICIPANT_TRACKS",
   "root_motion":"NOT_TESTED_NO_ACCEPTED_ROOT_TRANSLATION_SEQUENCE",
   "rendered_visual_quality":"NOT_TESTED_NULLRHI"}
actors=[]
def save(): OUT.write_text(json.dumps(r,indent=2,default=str)+"\n",encoding="utf-8")
save()
try:
 pawn_cls=unreal.load_class(None,"/Script/OfflineGame.OGLuminousV3PlayableCharacter")
 ctrl_cls=unreal.load_class(None,"/Script/OfflineGame.OGSharedMotionControllerComponent")
 assert pawn_cls and ctrl_cls
 for wardrobe in ("CLOTHED","UNDERWEAR","NUDE"):
  asset=f"/Game/NoWorld/Characters/Luminous/Alter/NativeMocapRealCMU/AN_G12_Alter_{wardrobe}_RealCMU_Walk"
  clip=unreal.load_asset(asset)
  assert isinstance(clip,unreal.AnimSequence)
  sk=clip.get_editor_property("skeleton")
  folder="/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal"+wardrobe.title()
  mesh=next((x for path in unreal.EditorAssetLibrary.list_assets(folder,recursive=False)
             if isinstance((x:=unreal.load_asset(path)),unreal.SkeletalMesh) and
                x.get_editor_property("skeleton")==sk),None)
  if not mesh:raise RuntimeError("No genuine same-skeleton mesh for "+wardrobe)
  actor=unreal.EditorLevelLibrary.spawn_actor_from_class(
      pawn_cls,unreal.Vector(500000+len(actors)*350,500000,500000))
  assert actor
  actors.append(actor)
  part=actor.get_component_by_class(unreal.SkeletalMeshComponent.static_class())
  ctl=actor.get_component_by_class(ctrl_cls)
  assert part and ctl
  part.set_skeletal_mesh_asset(mesh)
  ctl.set_editor_property("rig_profile",unreal.Name("koikatsu_original"))
  ctl.set_editor_property("anatomy_profile",unreal.Name("humanoid"))
  ctl.set_editor_property("outfit_profile",unreal.Name(wardrobe))
  ctl.set_editor_property("capabilities",[unreal.Name("grounded")])
  assert ctl.register_native_parts([part])
  assert ctl.can_play_action(unreal.Name("Traversal.Walk"),unreal.Name("Traversal"),
                             False,1,unreal.Name(""))
  assert ctl.play_action(unreal.Name("Traversal.Walk"),unreal.Name("Traversal"),
                          False,1,unreal.Name(""))
  assert ctl.is_playing_action()
  record={"wardrobe":wardrobe,"mesh":mesh.get_path_name(),"skeleton":sk.get_path_name(),
       "action":str(ctl.get_editor_property("current_action")),
       "source":str(ctl.get_editor_property("current_source_clip_id")),
       "parts":ctl.get_native_part_count(),"native_playback":"ACTIVE"}
  r["participants"].append(record)
  save()
 assert len(r["participants"])==3
 assert len({e["source"] for e in r["participants"]})==1
 assert len({e["skeleton"] for e in r["participants"]})==3
 r["status"]="PASS_3_REAL_WARDROBE_SKELETONS_ONE_SEMANTIC_ACTION"
except Exception as error:
 r["status"]="FAIL"
 r["error"]=str(error)
 r["trace"]=traceback.format_exc(limit=10)
 unreal.log_error("G12_POINT1_CROSSWARDROBE_FAIL "+str(error))
finally:
 for actor in actors:
  try:actor.destroy_actor()
  except Exception as error:r.setdefault("cleanup_warnings",[]).append(str(error))
 save()
 unreal.log("G12_POINT1_CROSSWARDROBE_RESULT "+r["status"])
