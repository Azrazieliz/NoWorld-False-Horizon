"""Acceptance of isolated, non-shipping source-authentic QA playback on real BP.
Does not mark gameplay approvals or alter user-owned characters.
"""
import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
FILE=R/"Saved/Gate12AnimationAcceptance/GATE12_REAL_NATIVE_QA_SOURCE_PREVIEW_EDITOR_FIXED_20261023.json"
doc={"schema":"noworld.gate12.real.6part.qa.preview.v1","state":"INIT",
     "adult_clip_exposed":False,"gameplay_approval_modified":False}
actor=None
try:
    cls=unreal.EditorAssetLibrary.load_blueprint_class(
        "/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production")
    assert cls
    actor=unreal.EditorLevelLibrary.spawn_actor_from_class(cls,
        unreal.Vector(350000,350000,350000))
    assert actor.initialize_native_wardrobe_for_isolated_qa(unreal.OGLuminousV3Wardrobe.CLOTHED)
    # C++ editor-only InitializeNativeWardrobeForIsolatedQA now grants this
    # transient, nonpersistent actor a QA session on successful real rig load.
    # Do not mutate EditDefaultsOnly Blueprint flags or user-owned records.
    doc["isolated_editor_flag"]="native_editor_only_transient_qa_session"
    names=actor.get_native_qa_preview_motion_names()
    ids=actor.get_native_qa_preview_motion_ids()
    doc["candidate_count"]=len(ids)
    doc["label_count"]=len(names)
    doc["first_names"]=names[:12]
    assert len(ids)==len(names)
    assert ids, "NO_QA_CANDIDATES_WITH_ISOLATED_GUARD"
    # Confirm mature-tagged entries are *visible for QA*, while none silently
    # become gameplay-approved. Reading manifest metadata does not play them.
    all_entries=json.loads((R/"Content/NoWorld/Animation/SharedMotionCatalog.json").read_text(encoding="utf8"))["motions"]
    selected={str(m) for m in ids}
    mature=[entry for entry in all_entries if entry.get("adult_restricted",False)
            and entry.get("motion_id") in selected]
    doc["adult_clip_exposed"]=bool(mature)
    doc["adult_qa_candidates"]=len(mature)
    doc["adult_qa_entries_unapproved"]=all(not m.get("approved_for_gameplay",False) for m in mature)
    assert doc["adult_clip_exposed"] and doc["adult_qa_entries_unapproved"]
    doc["first_id"]=str(ids[0])
    doc["played"]=actor.preview_native_qa_motion_by_id(ids[0])
    assert doc["played"],"SOURCE_QA_SEQUENCE_NOT_PLAYABLE_ON_FULL_NATIVE_CHARACTER"
    doc["active_native_parts"]=actor.get_active_luminous_part_count()
    assert doc["active_native_parts"]==6
    doc["state"]="PASS_REAL_SIX_PART_QA_CATALOG_SELECTION_AND_PLAYBACK"
except Exception as exc:
    doc["state"]="FAIL"
    doc["error"]=str(exc)
    doc["trace"]=traceback.format_exc(limit=8)
finally:
    if actor:
        try:actor.destroy_actor()
        except:pass
    FILE.parent.mkdir(parents=True,exist_ok=True)
    FILE.write_text(json.dumps(doc,indent=2,default=str)+"\n",encoding="utf8")
    unreal.log("G12_REAL_NATIVE_QA_BROWSER_RESULT "+doc["state"])
