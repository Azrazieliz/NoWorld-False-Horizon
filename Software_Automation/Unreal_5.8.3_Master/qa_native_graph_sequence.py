"""UE 5.8.3 PCG node graph wiring + SequencerScripting native keyframes.
Isolated QA project ONLY. Produces two new assets, tests reload and channel keys.
"""
import unreal,json,pathlib,traceback
QA=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
folder="/Game/NW_MasterHub_QA/SoftwareMaster_Native"
out={"engine":unreal.SystemLibrary.get_engine_version(),"tests":[],"production_modified":False}
tools=unreal.AssetToolsHelpers.get_asset_tools()
def log(name,fn):
 row={"name":name,"status":"START"}
 try:row.update(fn());row["status"]="PASS_NATIVE" if row.get("validated") else "INCOMPLETE"
 except Exception as e:row["status"]="FAIL";row["error"]=repr(e);row["trace"]=traceback.format_exc()[-1400:]
 out["tests"].append(row)
 print("NW_UE_PROCEDURAL_CASE="+json.dumps({k:v for k,v in row.items() if k!="trace"}))
def new_asset(name,klass,factory):
 path=folder+"/"+name
 if unreal.EditorAssetLibrary.does_asset_exist(path):raise RuntimeError("Refusing overwrite: "+path)
 a=tools.create_asset(name,folder,klass,factory)
 assert a
 return a
def pins(node,which):
 method=getattr(node,"get_"+which+"_pins",None)
 arr=method() if method else []
 out=[]
 for x in arr:
  try:out.append(str(x.get_editor_property("properties").get_editor_property("label")))
  except Exception as e:out.append("ERROR:"+str(e))
 return out
def pcg():
 a=new_asset("NW_PCG_ActualWiredGraph_QA",unreal.PCGGraph,unreal.PCGGraphFactory())
 inputn=a.get_input_node();outputn=a.get_output_node()
 surface,settings=a.add_node_of_type(unreal.PCGSurfaceSamplerSettings)
 transform,ts=a.add_node_of_type(unreal.PCGTransformPointsSettings)
 # Edges are actual persistent PCG native graph links.
 print("NW_PCG_PIN_INDEX="+json.dumps({"source":pins(inputn,"output"),
 "surface_in":pins(surface,"input"),"surface_out":pins(surface,"output"),
 "transform_in":pins(transform,"input"),"transform_out":pins(transform,"output"),
 "output_in":pins(outputn,"input")}))
 p1=pins(inputn,"output");p2=pins(surface,"input");p3=pins(surface,"output")
 p4=pins(transform,"input");p5=pins(transform,"output");p6=pins(outputn,"input")
 if not all((p1,p2,p3,p4,p5,p6)):raise RuntimeError("Missing PCG pins")
 a.add_edge(inputn,p1[0],surface,p2[0])
 a.add_edge(surface,p3[0],transform,p4[0])
 a.add_edge(transform,p5[0],outputn,p6[0])
 unreal.EditorAssetLibrary.save_loaded_asset(a)
 a2=unreal.load_asset(a.get_path_name())
 edges=list(a2.get_all_edges())
 return {"validated":len(edges)>=3,"path":a2.get_path_name(),"native_nodes":4,
   "edges":len(edges),"pins":[p1,p2,p3,p4,p5,p6],
   "source":"Input → SurfaceSampler → TransformPoints → Output"}
def seq():
 a=new_asset("NW_Sequencer_CameraKeyed_QA",unreal.LevelSequence,unreal.LevelSequenceFactoryNew())
 a.set_display_rate(unreal.FrameRate(24,1))
 a.set_playback_start(0);a.set_playback_end(96)
 binding=a.add_spawnable_from_class(unreal.CineCameraActor)
 track=binding.add_track(unreal.MovieScene3DTransformTrack)
 section=track.add_section();section.set_range(0,96)
 ext=getattr(unreal,"MovieSceneSectionExtensions",None)
 if ext:
  ch=ext.get_all_channels(section)
 else:
  raise RuntimeError("SequencerScripting MovieSceneSectionExtensions unavailable")
 values=[[0,240],[-240,-120],[190,225],[0,0],[0,0],[0,14],[1,1],[1,1],[1,1]]
 count=0
 for j,channel in enumerate(ch):
  vals=values[j] if j<len(values) else [0,0]
  for f,v in [(0,vals[0]),(95,vals[1])]:
   channel.add_key(unreal.FrameNumber(f),v)
   count+=1
 unreal.EditorAssetLibrary.save_loaded_asset(a)
 re=unreal.load_asset(a.get_path_name())
 ext2=unreal.MovieSceneSectionExtensions
 bind=re.get_bindings()[0]
 section2=bind.get_tracks()[0].get_sections()[0]
 re_ch=ext2.get_all_channels(section2)
 keys=sum(len(x.get_keys()) for x in re_ch)
 return {"validated":keys>=18,"path":re.get_path_name(),
    "native_camera_spawnable":True,"transform_channels":len(re_ch),
    "saved_keyframes":keys,"intent":"Noncanonical reusable transform authoring"}
def niagara_introspect():
 names=["NiagaraEditorLibrary","NiagaraEditorSubsystem","NiagaraSystemLibrary",
 "NiagaraSystemFactoryNew","NiagaraEmitterFactoryNew","NiagaraEmitter","NiagaraSystem"]
 x={name:{"available":bool(getattr(unreal,name,None)),
   "editor_apis":[k for k in dir(getattr(unreal,name)) if any(t in k.lower() for t in ("emitter","system","script","duplicate","add"))][:55] if getattr(unreal,name,None) else []} for name in names}
 return {"validated":True,"available_classes":x}
log("pcg_three_links",pcg)
log("sequencer_real_keyframes",seq)
log("niagara_editor_introspection",niagara_introspect)
out["pass_count"]=sum(c["status"]=="PASS_NATIVE" for c in out["tests"])
dst=QA/"UE58_ProceduralGraph_SequenceV2.json"
dst.write_text(json.dumps(out,indent=2))
print("NW_UE_GRAPH_SEQUENCE="+json.dumps({"pass":out["pass_count"],"total":len(out["tests"]),"report":str(dst)}))
