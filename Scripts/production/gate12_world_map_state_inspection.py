import unreal,json,pathlib,collections,traceback
root=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
receipt=root/"Saved/Gate12Readiness/FirstWorldRealWorldModeState_20261022.json"
out={"status":"started"}
try:
    path="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_StreamingRuntime"
    out["map"]=path
    out["level_loaded"]=bool(unreal.EditorLevelLibrary.load_level(path))
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    settings=world.get_world_settings()
    mode=settings.get_editor_property("default_game_mode")
    out["map_override_game_mode"]=str(mode.get_path_name()) if mode else "NONE"
    actors=unreal.EditorLevelLibrary.get_all_level_actors()
    out["actor_count"]=len(actors)
    out["actor_types"]=dict(collections.Counter(a.get_class().get_name() for a in actors))
    out["source_generators"]=[a.get_actor_label() for a in actors if "StartingRegionGenerator" in a.get_class().get_name()]
    out["player_starts"]=[a.get_actor_label() for a in actors if "PlayerStart" in a.get_class().get_name()]
    out["status"]="real_map_loaded"
except Exception:
    out["status"]="inspection_failed"
    out["error"]=traceback.format_exc()[-1800:]
receipt.parent.mkdir(parents=True,exist_ok=True)
receipt.write_text(json.dumps(out,indent=2),encoding="utf8")
print("GATE12_WORLD_STATE",out["status"])
