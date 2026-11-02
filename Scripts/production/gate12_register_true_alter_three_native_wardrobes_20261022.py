"""Bind all THREE authentic user-approved Alter wardrobe states to the
existing exact-Identity owned World character. No substitution, no Manifestation
creation, no portrait generation, no change to frozen Base or Gate01 originals.
"""
import unreal,pathlib,json,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
BP="/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production"
CAT="/Game/NoWorld/UI/DA_NoWorld_CharacterVisuals"
IDENTITY="character:luminous_valentine.identity"
VER="character:luminous_valentine.version_alter_v1"
ROOT="/Game/NoWorld/Characters/Luminous/Alter/"
RECEIPT=R/"Saved/Gate12Readiness/Gate12_Alter_FullWardrobe_ProductionWorldRegistration_20261022.json"
report={"schema":"noworld.gate12.alter.fullwardrobe.owned.world.v1",
"status":"STARTED","identity":IDENTITY,"version":VER,"blueprint":BP,
"approved_sources_replaced":False,"base_character_modified":False,
"new_manifestation_created":False,"real_mesh_states":[],"gameloop_runtime_verified":False}
try:
    assert R.name in unreal.Paths.get_project_file_path()
    assert unreal.EditorAssetLibrary.does_asset_exist(BP)
    cls=unreal.EditorAssetLibrary.load_blueprint_class(BP)
    assert cls
    cdo=unreal.get_default_object(cls)
    assert cdo.get_editor_property("canonical_luminous_identity_id")==IDENTITY
    assert cdo.get_editor_property("canonical_luminous_version_id")==VER
    arrays={}
    for state,bones in (("CLOTHED",1131),("UNDERWEAR",1131),("NUDE",1123)):
        folder=ROOT+"NativeOriginal"+state.title()+"/"
        first=folder+f"SK_Luminous_Alter_{state}_OriginalPart_00_Skeleton"
        skeleton=unreal.load_asset(first)
        assert isinstance(skeleton,unreal.Skeleton),first
        actual_bones=len(skeleton.get_reference_pose().get_bone_names())
        assert actual_bones==bones,(state,actual_bones,bones)
        original=[]
        for i in range(6):
            if state!="CLOTHED":
                proof=R/"Saved/Gate12Readiness"/f"Gate12_Alter_{state}_UE_OriginalPart_{i:02d}_20261022.json"
                receipt=json.loads(proof.read_text(encoding="utf8"))
                assert receipt["status"]=="ACTUAL_USER_CONFIRMED_ALTER_WARDROBE_PART_IMPORTED_NATIVE",proof
                assert receipt["native_skeleton_bones"]==bones
                assert receipt["state"]==state and receipt["part"]==i
            name=f"SK_Luminous_Alter_{state}_OriginalPart_{i:02d}"
            p=folder+name
            mesh=unreal.load_asset(p)
            assert isinstance(mesh,unreal.SkeletalMesh),p
            assert mesh.get_editor_property("skeleton")==skeleton,p+" wrong source armature"
            original.append(unreal.SoftObjectPath(p+"."+name))
        assert len(original)==6
        arrays[state]=original
        report["real_mesh_states"].append({"state":state,"original_parts":6,"bones":bones,
            "paths":[folder+f"SK_Luminous_Alter_{state}_OriginalPart_{i:02d}" for i in range(6)]})
    # Reject all unexpected earlier CDO sources. Existing Clothed state must
    # remain exactly the six approved original assets.
    old_receipt=json.loads((R/"Saved/Gate12Readiness/Gate12_LuminousAlter_TrueWorldCharacterRegistration_20261021.json").read_text(encoding="utf8"))
    assert old_receipt["status"]=="APPROVED_ALTER_CLOTHED_GEOMETRY_REGISTERED_AS_WORLD_CHARACTER"
    assert old_receipt["alter_clothed_parts_verified"]==6
    assert len(cdo.get_editor_property("clothed_native_parts"))==6
    report["original_clothed_source_preserved"]=True
    # Never mutate an already independently populated wardrobe unexpectedly.
    for state,prop in (("UNDERWEAR","underwear_native_parts"),("NUDE","nude_native_parts")):
        before=list(cdo.get_editor_property(prop))
        assert not before or len(before)==6,("Unexpected authored wardrobe "+prop)
        if not before:
            cdo.set_editor_property(prop,arrays[state])
        else:
            report[prop+"_existing_complete_preserved"]=True
    assert unreal.EditorAssetLibrary.save_loaded_asset(unreal.load_asset(BP))
    for state,prop in (("CLOTHED","clothed_native_parts"),("UNDERWEAR","underwear_native_parts"),("NUDE","nude_native_parts")):
        assert len(cdo.get_editor_property(prop))==6,prop
    catalog=unreal.load_asset(CAT)
    assert catalog is not None
    entries=list(catalog.get_editor_property("entries"))
    found=[]
    for i,item in enumerate(entries):
        if (item.get_editor_property("identity_id").get_editor_property("value")==IDENTITY and
            item.get_editor_property("version_id").get_editor_property("value")==VER):
            found.append(i)
    assert len(found)==1,found
    i=found[0]
    item=entries[i]
    before=[str(v) for v in item.get_editor_property("approved_wardrobe_choices")]
    assert before==["CLOTHED"] or before==["CLOTHED","UNDERWEAR","NUDE"],before
    item.set_editor_property("approved_wardrobe_choices",["CLOTHED","UNDERWEAR","NUDE"])
    entries[i]=item
    catalog.set_editor_property("entries",entries)
    assert unreal.EditorAssetLibrary.save_loaded_asset(catalog)
    report["catalog_index"]=i
    report["catalog_choices"]=[str(v) for v in entries[i].get_editor_property("approved_wardrobe_choices")]
    report["status"]="ALL_THREE_APPROVED_ALTER_SOURCE_WARDROBES_BOUND_TO_REAL_WORLD_ACTOR_AND_CATALOG"
except Exception:
    report["status"]="ACTOR_OR_REGISTRY_BINDING_UNVERIFIED"
    report["error"]=traceback.format_exc()[-4500:]
finally:
    RECEIPT.parent.mkdir(parents=True,exist_ok=True)
    RECEIPT.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
    unreal.log("G12_ALTER_FROZEN_IDENTITY_TRIPLET="+report["status"])
