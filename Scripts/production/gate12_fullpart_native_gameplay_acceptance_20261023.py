"""True whole-wardrobe playback on existing APPROVED production Blueprint.
Production C++ creates every original native mesh component. No Python
component fabrication; editor-only isolated context; no canonical DB edits.
Only previously approved Traversal.Walk is played. QA new clips remain gated.
"""
import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
OUT=R/"Saved/Gate12AnimationAcceptance/GATE12_WHOLE_CHARACTER_SHARED_WALK_20261023.json"
OUT.parent.mkdir(exist_ok=True,parents=True)
receipt={"schema":"noworld.gate12.full_real_blueprint_native_walk.v2",
"status":"IN_PROGRESS","wardrobes":[],"rendered_visual_quality":"NOT_RENDERED_NULLRHI",
"new_unapproved_animations_enabled":False,"canonical_ownership_mutated":False}
actors=[]
def save():OUT.write_text(json.dumps(receipt,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
    cls=unreal.EditorAssetLibrary.load_blueprint_class(
       "/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production")
    ctrl=unreal.load_class(None,"/Script/OfflineGame.OGSharedMotionControllerComponent")
    enum=unreal.OGLuminousV3Wardrobe
    assert cls and ctrl and enum
    for label,wardrobe in (("CLOTHED",enum.CLOTHED),
                           ("UNDERWEAR",enum.UNDERWEAR),("NUDE",enum.NUDE)):
        actor=unreal.EditorLevelLibrary.spawn_actor_from_class(
            cls,unreal.Vector(300000+len(actors)*400,300000,300000))
        assert actor
        actors.append(actor)
        version=str(actor.get_editor_property("canonical_luminous_version_id"))
        assert version=="character:luminous_valentine.version_alter_v1",version
        assert actor.initialize_native_wardrobe_for_isolated_qa(wardrobe),(
             label,actor.get_last_luminous_error())
        ctl=actor.get_component_by_class(ctrl)
        assert ctl
        assert actor.get_active_luminous_part_count()==6
        assert ctl.get_native_part_count()==6
        can=ctl.can_play_action(unreal.Name("Traversal.Walk"),
            unreal.Name("Traversal"),False,1,unreal.Name(""))
        played=ctl.play_action(unreal.Name("Traversal.Walk"),
            unreal.Name("Traversal"),False,1,unreal.Name("")) if can else False
        record={"wardrobe":label,"blueprint":str(actor.get_class().get_path_name()),
            "native_parts":actor.get_active_luminous_part_count(),
            "controller_parts":ctl.get_native_part_count(),
            "can_play_approved_action":can,"played":played,
            "source_id":str(ctl.get_editor_property("current_source_clip_id")) if played else "NONE"}
        receipt["wardrobes"].append(record)
        save()
    receipt["status"]="PASS_REAL_PRODUCTION_BP_SIX_PARTS_EACH_WARDROBE" if (
      all(x["played"] for x in receipt["wardrobes"]) and
      len({x["source_id"] for x in receipt["wardrobes"]})==1
      ) else "FAIL_NATIVE_ANIMATION_ACTION_ON_REAL_WARDROBE"
except Exception as error:
    receipt["status"]="FAIL_TECHNICAL"
    receipt["error"]=str(error)
    receipt["trace"]=traceback.format_exc(limit=10)
finally:
    for actor in actors:
        try:actor.destroy_actor()
        except:pass
    save()
    unreal.log("G12_REAL_FULLWARDROBE_ACCEPTANCE "+receipt["status"])
