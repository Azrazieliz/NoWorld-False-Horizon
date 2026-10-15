"""NoWorld reusable *declarative* native Unreal 5.8.3 authoring API.
No mouse clicks. Supports PCG graphs, editable materials, cinematic sequences,
C++-backed Blueprint classes, Niagara native systems in a sandbox.
Called via UE Editor Python and job JSON path in NW_UNREAL_AUTHOR_JOB.
Never writes outside /Game/NW_MasterHub_QA/ in software mastery.
"""
import unreal,pathlib,json,os,traceback,datetime,hashlib,sys
def load():
 config=os.environ.get("NW_UNREAL_AUTHOR_JOB")
 if not config:raise ValueError("NW_UNREAL_AUTHOR_JOB must be absolute JSON spec")
 return json.loads(pathlib.Path(config).read_text(encoding="utf-8-sig"))
def build(spec):
 label=str(spec["job_id"])
 prefix=str(spec["destination"])
 if not prefix.startswith("/Game/NW_MasterHub_QA/"):
  raise ValueError("Production destination forbidden in software master")
 if spec.get("allow_overwrite"):raise ValueError("Refusing overwrite")
 requests=spec.get("assets",[])
 if not isinstance(requests,list) or not requests or len(requests)>40:
  raise ValueError("Need 1–40 assets")
 tools=unreal.AssetToolsHelpers.get_asset_tools()
 unreal.EditorAssetLibrary.make_directory(prefix)
 def create(item,klass,factory):
  name=item["name"]
  if not name.startswith("NW_") or not name.replace("_","").isalnum():
   raise ValueError("Unsafe test asset name")
  path=prefix+"/"+name
  if unreal.EditorAssetLibrary.does_asset_exist(path):raise FileExistsError(path)
  instance=tools.create_asset(name,prefix,klass,factory)
  if instance is None:raise RuntimeError("Unreal asset factory returned null: "+name)
  return instance
 def check(asset,expected):
  unreal.EditorAssetLibrary.save_loaded_asset(asset)
  a=unreal.load_asset(asset.get_path_name())
  if not a or a.get_class().get_name()!=expected:raise RuntimeError("Reload mismatch")
  return {"path":a.get_path_name(),"asset_class":expected,"saved_and_reloaded":True}
 def pbr(item):
  a=create(item,unreal.Material,unreal.MaterialFactoryNew())
  base=item.get("base_color",[.45,.45,.45]);rough=float(item.get("roughness",.55))
  metal=float(item.get("metallic",0))
  params=[
   (unreal.MaterialExpressionVectorParameter,"NW_BaseColor",
    unreal.LinearColor(float(base[0]),float(base[1]),float(base[2]),1),
    unreal.MaterialProperty.MP_BASE_COLOR),
   (unreal.MaterialExpressionScalarParameter,"NW_Roughness",rough,unreal.MaterialProperty.MP_ROUGHNESS),
   (unreal.MaterialExpressionScalarParameter,"NW_Metallic",metal,unreal.MaterialProperty.MP_METALLIC)]
  for i,(klass,name,value,dest) in enumerate(params):
   node=unreal.MaterialEditingLibrary.create_material_expression(a,klass,-540,120*i)
   node.set_editor_property("parameter_name",name)
   node.set_editor_property("default_value",value)
   unreal.MaterialEditingLibrary.connect_material_property(node,"",dest)
  unreal.MaterialEditingLibrary.recompile_material(a)
  result=check(a,"Material")
  result.update(material_parameters=["NW_BaseColor","NW_Roughness","NW_Metallic"])
  return result
 def pcg(item):
  a=create(item,unreal.PCGGraph,unreal.PCGGraphFactory())
  idmap={"Input":a.get_input_node(),"Output":a.get_output_node()}
  possible={"SurfaceSampler":unreal.PCGSurfaceSamplerSettings,
    "TransformPoints":unreal.PCGTransformPointsSettings,
    "DensityFilter":unreal.PCGDensityFilterSettings,
    "CopyPoints":unreal.PCGCopyPointsSettings}
  for n in item["nodes"]:
   name=n["id"]
   if name in idmap:raise ValueError("Duplicate node "+name)
   t=n["type"]
   if t not in possible:raise ValueError("Unsupported PCG node "+t)
   node,settings=a.add_node_of_type(possible[t]);idmap[name]=node
   # Only set safe, inspected native PCG settings, never dynamically mutate arbitrary types.
   for k,v in n.get("settings",{}).items():
    if k not in ("points_per_square_meter","looseness","seed","b_unbounded"):
     raise ValueError("Unapproved PCG setting "+k)
    settings.set_editor_property(k,v)
  def first_pin(node,dir):
   arr=node.get_editor_property(dir+"_pins")
   if not arr:raise RuntimeError("Missing "+dir+" PCG pin")
   return str(arr[0].get_editor_property("properties").get_editor_property("label"))
  for edge in item["edges"]:
   src,dst=edge
   a.add_edge(idmap[src],first_pin(idmap[src],"output"),
              idmap[dst],first_pin(idmap[dst],"input"))
  result=check(a,"PCGGraph")
  back=unreal.load_asset(result["path"])
  count=len(back.get_all_edges())
  if count<len(item["edges"]):raise RuntimeError("Native PCG edges not retained")
  result.update(nodes=len(idmap),edges=count)
  return result
 def sequencer(item):
  fps=int(item.get("fps",24));frames=int(item.get("frames",96))
  a=create(item,unreal.LevelSequence,unreal.LevelSequenceFactoryNew())
  a.set_display_rate(unreal.FrameRate(fps,1))
  a.set_playback_start(0);a.set_playback_end(frames)
  camera=a.add_spawnable_from_class(unreal.CineCameraActor)
  track=camera.add_track(unreal.MovieScene3DTransformTrack)
  section=track.add_section();section.set_range(0,frames)
  channels=unreal.MovieSceneSectionExtensions.get_all_channels(section)
  if len(channels)<9:raise RuntimeError("Sequencer transform channels absent")
  keys=item.get("keys",[{"frame":0,"transform":[0,-200,160,0,0,0,1,1,1]},
   {"frame":frames-1,"transform":[50,-120,180,0,0,12,1,1,1]}])
  for k in keys:
   f=int(k["frame"]);values=k["transform"]
   if f<0 or f>=frames or len(values)!=9:raise ValueError("Invalid transform key")
   for idx,val in enumerate(values):channels[idx].add_key(unreal.FrameNumber(f),float(val))
  result=check(a,"LevelSequence")
  back=unreal.load_asset(result["path"])
  binding=back.get_bindings()[0]
  transform=next(t for t in binding.get_tracks() if isinstance(t,unreal.MovieScene3DTransformTrack))
  saved=unreal.MovieSceneSectionExtensions.get_all_channels(transform.get_sections()[0])
  n=sum(len(ch.get_keys()) for ch in saved)
  if n<len(keys)*9:raise RuntimeError("Unreal saved no cinematic keyframes")
  result.update(camera_spawnable=True,fps=fps,frames=frames,native_keys=n)
  return result
 def niagara(item):
  a=create(item,unreal.NiagaraSystem,unreal.NiagaraSystemFactoryNew())
  result=check(a,"NiagaraSystem")
  result["functional_emitter_created"]=False
  return result
 def actor(item):
  parent={"Actor":unreal.Actor,"Pawn":unreal.Pawn,"Character":unreal.Character}.get(item.get("parent","Actor"))
  if not parent:raise ValueError("Unsupported Blueprint parent")
  fac=unreal.BlueprintFactory()
  fac.set_editor_property("parent_class",parent)
  a=create(item,unreal.Blueprint,fac)
  result=check(a,"Blueprint")
  result["parent"]=str(item.get("parent","Actor"))
  result["blueprint_compiled"]=False
  # A freshly generated Blueprint usually has a generated_class, but game compilation is separate.
  result["generated_class_present"]=bool(a.generated_class())
  return result
 options={"material":pbr,"pcg":pcg,"sequence":sequencer,"niagara":niagara,
    "blueprint":actor}
 outcome={"schema":"NW_UE_EDITOR_AUTHOR_V1","engine":unreal.SystemLibrary.get_engine_version(),
  "job_id":label,"game_project_changed":False,
  "sandbox_only":True,"results":[]}
 for item in requests:
  record={"name":item["name"],"type":item["type"]}
  try:
   if item["type"] not in options:raise ValueError("Unsupported authoring type")
   record.update(options[item["type"]](item))
   record["status"]="PASS_NATIVE"
  except Exception:
   record["status"]="FAIL"
   record["traceback"]=traceback.format_exc()[-2000:]
  outcome["results"].append(record)
  print("NW_UE_AUTHOR_ASSET="+json.dumps({k:v for k,v in record.items() if k!="traceback"}))
 outcome["success_count"]=sum(x["status"]=="PASS_NATIVE" for x in outcome["results"])
 outcome["requested"]=len(requests)
 outcome["full_success"]=outcome["success_count"]==len(requests)
 return outcome
def main():
 spec=load()
 receipt=pathlib.Path(spec["receipt"]).resolve()
 if receipt.exists():raise FileExistsError(receipt)
 receipt.parent.mkdir(parents=True,exist_ok=True)
 try:r=build(spec)
 except Exception:r={"full_success":False,"fatal":traceback.format_exc(),"results":[]}
 receipt.write_text(json.dumps(r,indent=2),encoding="utf-8")
 print("NW_UE_AUTHOR_RUN="+json.dumps({k:v for k,v in r.items() if k not in ("results","fatal")}))
if __name__=="__main__":main()
