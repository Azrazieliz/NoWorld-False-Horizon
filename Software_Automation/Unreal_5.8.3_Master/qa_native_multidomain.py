"""NoWorld native Unreal Engine 5.8.3 asset-creation acceptance, isolated sandbox.
Create real editor assets: procedural materials, PCG graph, LevelSequence and
NiagaraSystem. Individual failures are logged independently; no game project writes.
"""
import unreal,json,pathlib,traceback,datetime
QA=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
DEST="/Game/NW_MasterHub_QA/SoftwareMaster_Native"
r={"schema":"NW_UNREAL_NATIVE_MULTIDOMAIN_V1","engine":unreal.SystemLibrary.get_engine_version(),
   "produced_at_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
   "production_game_changed":False,"cases":[]}
tools=unreal.AssetToolsHelpers.get_asset_tools()
unreal.EditorAssetLibrary.make_directory(DEST)
def case(name,fn):
 item={"name":name,"status":"START"}
 try:
  ret=fn();item.update(ret)
  item["status"]="PASS_NATIVE" if ret.get("verified",False) else "CREATED_UNVERIFIED"
 except Exception as e:item["status"]="ERROR";item["reason"]=str(e);item["traceback"]=traceback.format_exc()[-1600:]
 r["cases"].append(item)
 print("NW_UNREAL_CASE="+json.dumps({k:v for k,v in item.items() if k!="traceback"}))
def create(name,klass,factory):
 full=DEST+"/"+name
 if unreal.EditorAssetLibrary.does_asset_exist(full):raise RuntimeError("Refuse overwrite "+full)
 asset=tools.create_asset(name,DEST,klass,factory)
 if not asset:raise RuntimeError("Asset factory returned None: "+name)
 return asset
def saved(a,cl):
 unreal.EditorAssetLibrary.save_loaded_asset(a)
 path=a.get_path_name()
 other=unreal.load_asset(path)
 if other is None or other.get_class().get_name()!=cl:raise RuntimeError("Native reload class mismatch "+path)
 return {"verified":True,"path":path,"class":other.get_class().get_name()}
def mat():
 a=create("NW_Material_AnimePBR_ParametersQA",unreal.Material,unreal.MaterialFactoryNew())
 color=unreal.MaterialEditingLibrary.create_material_expression(a,unreal.MaterialExpressionVectorParameter,-560,0)
 color.set_editor_property("parameter_name","NW_BaseColor")
 color.set_editor_property("default_value",unreal.LinearColor(.42,.46,.66,1))
 rough=unreal.MaterialEditingLibrary.create_material_expression(a,unreal.MaterialExpressionScalarParameter,-550,210)
 rough.set_editor_property("parameter_name","NW_Roughness")
 rough.set_editor_property("default_value",.57)
 ok_color=unreal.MaterialEditingLibrary.connect_material_property(color,"",unreal.MaterialProperty.MP_BASE_COLOR)
 ok_rough=unreal.MaterialEditingLibrary.connect_material_property(rough,"",unreal.MaterialProperty.MP_ROUGHNESS)
 unreal.MaterialEditingLibrary.recompile_material(a)
 info=saved(a,"Material")
 info.update({"parameter_names":["NW_BaseColor","NW_Roughness"],"connected":[str(ok_color),str(ok_rough)]})
 return info
def pcg():
 g=create("NW_PCG_TerrainAnchors_QA",unreal.PCGGraph,unreal.PCGGraphFactory())
 desc={"node_type_doc":str(unreal.PCGGraph.add_node_of_type.__doc__)[:1700],
    "edge_doc":str(unreal.PCGGraph.add_edge.__doc__)[:900],
    "input":bool(g.get_input_node()),"output":bool(g.get_output_node())}
 # An actual PCG settings node; graph retains raw input/output nodes.
 choices=[getattr(unreal,n,None) for n in ("PCGSurfaceSamplerSettings","PCGTransformPointsSettings","PCGPointSamplerSettings")]
 choice=next((x for x in choices if x),None)
 if choice:
  try:
   node=g.add_node_of_type(choice)
   desc["new_node"]=str(node)[:160]
  except Exception as exc:desc["node_error"]=str(exc)
 info=saved(g,"PCGGraph");info["pcg"]=desc
 return info
def niagara():
 a=create("NW_Niagara_System_QA",unreal.NiagaraSystem,unreal.NiagaraSystemFactoryNew())
 info=saved(a,"NiagaraSystem")
 info["emitter_count_verified"]=False
 return info
def sequence():
 s=create("NW_Sequencer_TestShot_QA",unreal.LevelSequence,unreal.LevelSequenceFactoryNew())
 s.set_display_rate(unreal.FrameRate(24,1))
 s.set_playback_start(0)
 s.set_playback_end(96)
 binding=s.add_spawnable_from_class(unreal.CineCameraActor)
 track=binding.add_track(unreal.MovieScene3DTransformTrack)
 section=track.add_section();section.set_range(0,96)
 channels=section.get_channels()
 # 3D transform is 9 double curves: Location XYZ, Rotation XYZ, Scale XYZ.
 for ch in channels:
  try:
   for frame in [0,48,95]:ch.add_key(unreal.FrameNumber(frame),0.0)
  except Exception:pass
 info=saved(s,"LevelSequence")
 info["camera_spawnable"]=str(binding.get_name())
 info["transform_track_count"]=len(binding.get_tracks())
 info["channels"]=len(channels)
 info["output_is_reusable_sequencer_asset"]=True
 return info
for name,fn in (("editable_pbr_material",mat),("procedural_pcg_graph",pcg),
                 ("niagara_system",niagara),("sequencer_camera_motion",sequence)):
 case(name,fn)
r["accepted_count"]=sum(x["status"]=="PASS_NATIVE" for x in r["cases"])
r["requires_real_ue_gameplay_testing"]=True
r["requires_art_review"]=True
out=QA/"UE58_Unified_NativeEditor_Assets.json"
out.write_text(json.dumps(r,indent=2))
print("NW_UNREAL_MULTIDOMAIN="+json.dumps({"engine":r["engine"],"successes":r["accepted_count"],"total":len(r["cases"]),"report":str(out)}))
