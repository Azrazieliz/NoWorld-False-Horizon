"""Gate 12 production-only R1 streaming-map runtime game-mode qualification.
Only the existing streaming-runtime .umap can be modified. Preserve authoring map
and every original actor/asset. Never construct fixtures or missing geometry.
"""
import unreal, pathlib, json, hashlib, traceback
root=pathlib.Path(unreal.Paths.project_dir())
source=root/"Content/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly.umap"
dest=root/"Content/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_StreamingRuntime.umap"
receipt=root/"Saved/Gate12Readiness/Gate12_R1_GameplayModeBinding_20261021.json"
expected_source="9d856018c15bcdfcc1ce7bd320d88b3f299808a164c395b90f56d1444d594f83"
out={"map":"/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_StreamingRuntime",
     "source_authoring_map_protected":True,"changed":False,"errors":[]}
try:
    assert source.is_file() and dest.is_file()
    assert hashlib.sha256(source.read_bytes()).hexdigest()==expected_source, "Original source map hash mismatch: refuse editing"
    assert unreal.EditorLevelLibrary.load_level(out["map"]), "Cannot load current streaming map"
    world=unreal.EditorLevelLibrary.get_editor_world()
    assert world
    actors=unreal.EditorLevelLibrary.get_all_level_actors()
    assert any(isinstance(a,unreal.PlayerStart) for a in actors), "Original PlayerStart absent"
    assert any("StartingRegionGenerator" in a.get_class().get_name() for a in actors), "Native Gate02 generator absent"
    for label in ["Gate02_R1_Actual_RoyalCitadel_SourceWorldSpace",
                  "Gate02_R1_Actual_watchwood_hamlet_SourceAnchored",
                  "Gate02_R1_Actual_southern_farmland_windmill_SourceAnchored"]:
        assert any(a.get_actor_label()==label for a in actors), "Original world anchor absent: "+label
    settings=world.get_world_settings()
    existing=settings.get_editor_property("default_game_mode")
    mode=unreal.load_class(None, "/Script/OfflineGame.OGWorldPresentationGameMode")
    assert mode, "Compiled native GameMode class unavailable"
    out["prior_mode"]=existing.get_path_name() if existing else None
    if existing!=mode:
        settings.set_editor_property("default_game_mode",mode)
        assert unreal.EditorLevelLibrary.save_current_level(), "Failed to save streaming runtime map"
        out["changed"]=True
    verified=settings.get_editor_property("default_game_mode")
    assert verified==mode, "Runtime mode override did not persist"
    assert hashlib.sha256(source.read_bytes()).hexdigest()==expected_source, "Protected source map changed"
    out["runtime_mode"]=verified.get_path_name()
    out["original_actor_count"]=len(actors)
    out["status"]="GAMEPLAY_MODE_BOUND_TO_EXISTING_STREAMING_WORLD"
except Exception:
    out["errors"].append(traceback.format_exc()[-4000:])
    out["status"]="FAILED_NO_QUALIFICATION"
receipt.parent.mkdir(parents=True,exist_ok=True)
receipt.write_text(json.dumps(out,indent=2)+"\n",encoding="utf8")
unreal.log("GATE12_R1_GAMEPLAY_MODE="+out["status"]+" "+str(out.get("runtime_mode")))
