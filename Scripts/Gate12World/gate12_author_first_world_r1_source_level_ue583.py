"""Create real source-art first-World LEVEL distinct from StartingWorld.
The R1 generator is the EXISTING OGStartingRegionGenerator, tagged to
activate true heightfield+ecology after its native source revision is compiled.
No NPC/faction/quest/spawn ownership/canon manufactured."""
import unreal,json,struct,hashlib,traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
P="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
RECEIPT=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/first_world_source_level_receipt.json"
PREVIOUS=json.loads(RECEIPT.read_text()) if RECEIPT.is_file() else {}
D={"status":"STARTED","world_map":P,"actors":[],"source_hashes":{}}
def save():RECEIPT.write_text(json.dumps(D,indent=2,default=str),encoding="utf-8")
save()
try:
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 if unreal.EditorAssetLibrary.does_asset_exist(P):
  prior=PREVIOUS
  assert prior.get("status")=="PARTIAL_OR_FAILED" and not prior.get("actors"),"Protected or authored level cannot be replaced"
  assert unreal.EditorLevelLibrary.load_level(P),"Could not resume failed-source level"
 else:
  assert unreal.EditorLevelLibrary.new_level(P),"Could not create source-art level"
 W=unreal.EditorLevelLibrary.get_editor_world()
 WS=W.get_world_settings()
 Mode=unreal.load_class(None,"/Script/OfflineGame.OGWorldPresentationGameMode")
 assert Mode is not None,"Existing authentic presentation GameMode unavailable"
 WS.set_editor_property("default_game_mode",Mode)
 Height=ROOT/"Content/NoWorld/FirstWorld/Source/height_513_float32.raw"
 Raw=Height.read_bytes()
 assert len(Raw)==513*513*4
 D["source_hashes"]["Gate02R1Height"]=hashlib.sha256(Raw).hexdigest()
 # The World needs a genuinely playable Ruler start but it is not a forced
 # tutorial or a declaration that any settlement is canonically the origin.
 Mid=struct.unpack_from("<f",Raw,(256*513+256)*4)[0]
 PC=unreal.EditorLevelLibrary.spawn_actor_from_class(
     unreal.PlayerStart,unreal.Vector(0,0,Mid*100+175))
 PC.set_actor_label("OriginalRuler_PlayerStart_OpenExploration_NotMandatorySettlement")
 D["actors"].append(PC.get_actor_label())
 GenClass=unreal.load_class(None,"/Script/OfflineGame.OGStartingRegionGenerator")
 assert GenClass is not None
 G=unreal.EditorLevelLibrary.spawn_actor_from_class(
     GenClass,unreal.Vector(0,0,0))
 assert G is not None
 # Tag is available in EXISTING compiled class; newer Gate12 C++ recognizes
 # it without a Blueprint reparent or new generator framework.
 G.set_editor_property("tags",[unreal.Name("Gate02R1Source")])
 G.set_editor_property("editor_preview_seed",1412)
 G.set_actor_label("Gate02_R1_True513Height_Seed1412_25x25EcologyTiles")
 D["actors"].append(G.get_actor_label())
 # Original roads FBX stores GLOBAL positions in its mesh vertices, so every
 # genuine imported road piece spawns at identity; no duplicate world offset.
 roads=unreal.EditorAssetLibrary.list_assets(
     "/Game/Experimental/FirstWorldR1/Roads",
     recursive=True,include_folder=False)
 road_count=0
 for obj_path in roads:
  mesh=unreal.EditorAssetLibrary.load_asset(obj_path)
  if not mesh or not isinstance(mesh,unreal.StaticMesh):continue
  if not mesh.get_name().startswith("ROAD___"):continue
  A=unreal.EditorLevelLibrary.spawn_actor_from_class(
      unreal.StaticMeshActor,unreal.Vector(0,0,0))
  A.get_editor_property("static_mesh_component").set_static_mesh(mesh)
  A.set_actor_label("R1_Gate02_WorldSpace_"+mesh.get_name())
  road_count+=1
 D["actual_road_meshes_world_space"]=road_count
 assert road_count>=4,"Genuine original regional roads missing"
 # Only existing UE environmental primitives for physically accurate sky/
 # natural light; never replace a missing city with boxes or mannequin NPCs.
 light=unreal.EditorLevelLibrary.spawn_actor_from_class(
     unreal.DirectionalLight,unreal.Vector(0,0,100000),
     unreal.Rotator(-45,-36,0))
 light.set_actor_label("FirstWorld_DirectionalSun")
 sky=unreal.EditorLevelLibrary.spawn_actor_from_class(
     unreal.SkyLight,unreal.Vector(0,0,0))
 sky.set_actor_label("FirstWorld_SkyFill")
 atmo=unreal.EditorLevelLibrary.spawn_actor_from_class(
     unreal.SkyAtmosphere,unreal.Vector(0,0,0))
 atmo.set_actor_label("FirstWorld_Atmosphere")
 fog=unreal.EditorLevelLibrary.spawn_actor_from_class(
     unreal.ExponentialHeightFog,unreal.Vector(0,0,0))
 fog.set_actor_label("FirstWorld_AtmosphericPerspective")
 if not unreal.EditorLevelLibrary.save_current_level():
  raise RuntimeError("Failed to save World source map")
 D["status"]="FIRST_WORLD_SOURCE_MAP_SAVED_UNBUILT_NOT_ART_APPROVED"
except Exception as exc:
 D["status"]="PARTIAL_OR_FAILED"
 D["errors"]=[str(exc),traceback.format_exc()[-6000:]]
finally:
 save()
 unreal.log("GATE12_FIRST_WORLD_NATIVE_LEVEL="+D["status"])
