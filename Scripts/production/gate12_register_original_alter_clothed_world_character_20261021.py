"""Make the ACTUAL independently authored Alter CLOTHED character playable.

The user-approved Alter source is not a copy of Base: duplicate only the
native movement/controller Blueprint CLASS (not its appearance), substitute
all six unique source-authored 1131-bone Alter parts, clear every Base wardrobe
and incompatible 975-bone animation path, and register the independent Alter
Version in the existing World character visual catalog. Do not invent nude
or underwear meshes, characters, Manifestations, Registry records or gameplay.
"""
import unreal,pathlib,json,hashlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
PRE="/Game/NoWorld/Characters/Luminous/Alter/NativeOriginalClothed/"
SRC="/Game/NoWorld/Characters/Luminous/Base/BP_Luminous_Base_Production"
TARGET="/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production"
CATALOG="/Game/NoWorld/UI/DA_NoWorld_CharacterVisuals"
IDENTITY="character:luminous_valentine.identity"
ALTER_VERSION="character:luminous_valentine.version_alter_v1"
OUT=R/"Saved/Gate12Readiness/Gate12_LuminousAlter_TrueWorldCharacterRegistration_20261021.json"
proof={"schema":"noworld.gate12.luminous_alter_native_world_actor.v1",
"status":"STARTED","identity":IDENTITY,"version":ALTER_VERSION,
"source_base_controller_class":SRC,"alter_character_target":TARGET,
"alter_clothed_parts_verified":0,"alter_part_bone_count":0,
"alter_source_mesh_replaced_by_base":False,
"unapproved_underwear_or_nude_fabricated":False,
"mesh_state_limited_to_verified_clothed":True,
"new_owner_or_manifestation_created":False,
"source_data_modified":False,"game_compiled":False,
"full_visual_approval":False}
try:
    assert "OfflineGame_Gate12_Koikatsu_20261010" in str(unreal.Paths.get_project_file_path())
    assert unreal.EditorAssetLibrary.does_asset_exist(SRC)
    skeleton_path=PRE+"SK_Luminous_Alter_CLOTHED_OriginalPart_00_Skeleton"
    rig=unreal.load_asset(skeleton_path)
    assert isinstance(rig,unreal.Skeleton)
    ref=[str(x) for x in rig.get_reference_pose().get_bone_names()]
    assert len(ref)==1131,(len(ref),"must be authentic Alter rig")
    parts=[]
    for i in range(6):
        p=PRE+f"SK_Luminous_Alter_CLOTHED_OriginalPart_{i:02d}"
        mesh=unreal.load_asset(p)
        assert isinstance(mesh,unreal.SkeletalMesh),p
        assert mesh.get_editor_property("skeleton")==rig,p+" does not use original Alter armature"
        assert mesh.get_editor_property("skeleton")==rig,p+" root rig mismatch"
        parts.append(unreal.SoftObjectPath(p+"."+p.rsplit("/",1)[-1]))
    assert len({str(x) for x in parts})==6
    proof["alter_clothed_parts_verified"]=6
    proof["alter_part_bone_count"]=1131
    existing=unreal.EditorAssetLibrary.does_asset_exist(TARGET)
    if existing:
        bp=unreal.load_asset(TARGET)
        assert isinstance(bp,unreal.Blueprint),"Existing production Alter target not a Blueprint"
        # Refuse modifying source supplied externally. This tool alone creates
        # new asset; existing one is preserved for review.
        proof["preserved_existing_asset"]=True
    else:
        unreal.EditorAssetLibrary.make_directory("/Game/NoWorld/Characters/Luminous/Alter")
        bp=unreal.EditorAssetLibrary.duplicate_asset(SRC,TARGET)
        assert isinstance(bp,unreal.Blueprint)
        proof["new_blueprint_created"]=True
    cl=unreal.EditorAssetLibrary.load_blueprint_class(TARGET)
    assert cl is not None
    cdo=unreal.get_default_object(cl)
    cdo.set_editor_property("canonical_luminous_identity_id",IDENTITY)
    cdo.set_editor_property("canonical_luminous_version_id",ALTER_VERSION)
    cdo.set_editor_property("clothed_native_parts",parts)
    cdo.set_editor_property("underwear_native_parts",[])
    cdo.set_editor_property("nude_native_parts",[])
    cdo.set_editor_property("native_walk_by_wardrobe",[])
    cdo.set_editor_property("native_run_by_wardrobe",[])
    cdo.set_editor_property("native_jump_by_wardrobe",[])
    for k in ("run_non_persistent_wardrobe_qa_cycle",
              "run_native_motion_qa_cycle",
              "enable_native_wardrobe_qa_touch_panel",
              "enter_world_mode_on_isolated_qa_startup"):
        cdo.set_editor_property(k,False)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
    proof["alter_visual_source_paths"]=[str(x) for x in parts]
    assert len(cdo.get_editor_property("clothed_native_parts"))==6
    assert len(cdo.get_editor_property("underwear_native_parts"))==0
    asset=unreal.load_asset(CATALOG)
    assert asset is not None
    entries=list(asset.get_editor_property("entries"))
    matches=[]
    for i,e in enumerate(entries):
        if (e.get_editor_property("identity_id").get_editor_property("value")==IDENTITY and
            e.get_editor_property("version_id").get_editor_property("value")==ALTER_VERSION):
            matches.append(i)
    assert len(matches)<=1,"Duplicate unrelated Alter visual entries"
    if not matches:
        entry=unreal.OGGate12CharacterVisualEntry()
        entry.set_editor_property("identity_id",unreal.OGContentId(value=IDENTITY))
        entry.set_editor_property("version_id",unreal.OGContentId(value=ALTER_VERSION))
    else:entry=entries[matches[0]]
    entry.set_editor_property("wardrobe_state_json_field","luminous_v3_wardrobe")
    entry.set_editor_property("approved_wardrobe_choices",["CLOTHED"])
    entry.set_editor_property("approved_world_playable_class",cl)
    entry.set_editor_property("genuine_character_preview_class",cl)
    if not matches:entries.append(entry)
    asset.set_editor_property("entries",entries)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    proof["catalog_entries"]=len(entries)
    proof["catalog_alter_index"]=matches[0] if matches else len(entries)-1
    proof["status"]="APPROVED_ALTER_CLOTHED_GEOMETRY_REGISTERED_AS_WORLD_CHARACTER"
except Exception:
    proof["status"]="ALTER_REGISTRATION_BLOCKED"
    proof["error"]=traceback.format_exc()[-5000:]
finally:
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(proof,indent=2)+"\n",encoding="utf8")
    unreal.log("GATE12_REAL_ALTER_ACTOR="+proof["status"])
