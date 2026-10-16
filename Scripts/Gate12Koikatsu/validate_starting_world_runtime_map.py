"""Read-only, no-cook runtime-map boot contract validation.
This inspects the actual default map setting, per-map game mode and
placed actors before changing startup or launching an isolated game.
"""
import json
import pathlib
import configparser
import unreal

ROOT = pathlib.Path(unreal.Paths.project_dir())
OUT = pathlib.Path(unreal.Paths.project_saved_dir()) / "Gate12Readiness" / "starting_world_runtime_contract.json"
map_name="/Game/Maps/StartingWorld"
data={"schema":"GATE12_ISSUE25_STARTING_WORLD_MAP_V1","target_map":map_name,
      "editor_only_inspection":True,"no_asset_changes":True,
      "placed_classes":{},"actors":[],"errors":[]}

try:
    unreal.EditorLevelLibrary.load_level(map_name)
    world=unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        raise RuntimeError("StartingWorld editor World did not load.")
    settings=world.get_world_settings()
    gm=settings.get_editor_property("default_game_mode") if settings else None
    data["map_override_game_mode"]=gm.get_path_name() if gm else None
    placed=unreal.EditorLevelLibrary.get_all_level_actors()
    from collections import Counter
    counts=Counter(a.get_class().get_name() for a in placed)
    data["placed_classes"]=dict(sorted(counts.items()))
    data["actor_total"]=len(placed)
    data["player_starts"]=[a.get_name() for a in placed
                          if isinstance(a,unreal.PlayerStart)]
    data["placed_world_huds"]=[a.get_name() for a in placed
                               if "HUD" in a.get_class().get_name()]
    data["placed_qa_character_actors"]=[a.get_path_name() for a in placed
                                       if "Luminous" in a.get_class().get_name()]
    data["lighting_classes"]={n:c for n,c in counts.items()
                              if any(x in n for x in ("Light", "Sky", "Atmosphere"))}
except Exception as exc:
    data["errors"].append(str(exc))

ini=ROOT/"Config"/"DefaultEngine.ini"
if ini.exists():
    for line in ini.read_text(encoding="utf-8-sig").splitlines():
        for name in ("GameDefaultMap","GlobalDefaultGameMode"):
            if line.startswith(name+"="):
                data[name]=line.split("=",1)[1].strip()

data["startup_contract"] = {
    "game_launches_starting_world": data.get("GameDefaultMap") == map_name,
    "map_uses_real_world_game_mode":
        data.get("map_override_game_mode") ==
        "/Script/OfflineGame.OGWorldPresentationGameMode",
    "has_player_start": bool(data.get("player_starts")),
    "map_has_lighting": bool(data.get("lighting_classes")),
}
for requirement, passed in data["startup_contract"].items():
    if not passed:
        data["errors"].append("Required startup contract failed: " + requirement)
data["status"] = ("READONLY_STARTUP_CONTRACT_PASS"
                  if not data["errors"] else "MAP_STARTUP_CONTRACT_FAILED")
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(data,indent=2)+"\n",encoding="utf-8")
unreal.log("GATE12_STARTING_WORLD_CONTRACT " + json.dumps(data)[:2500])
