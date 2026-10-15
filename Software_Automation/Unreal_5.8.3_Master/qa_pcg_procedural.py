"""Real native PCG graph authoring by named pins in UE 5.8.3 QA sandbox."""
import unreal,json,traceback,pathlib
OUT=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\UE58_PCG_WiredGraph_v3.json")
r={"status":"START","game_project_changed":False}
try:
 name="NW_PCG_RealSurfaceSamplerAndTransform_V3";dest="/Game/NW_MasterHub_QA/SoftwareMaster_Native"
 full=dest+"/"+name
 if unreal.EditorAssetLibrary.does_asset_exist(full):raise RuntimeError("Refuse overwrite existing "+full)
 graph=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,dest,unreal.PCGGraph,unreal.PCGGraphFactory())
 assert graph
 inp=graph.get_input_node();outp=graph.get_output_node()
 surf,ss=graph.add_node_of_type(unreal.PCGSurfaceSamplerSettings)
 tr,ts=graph.add_node_of_type(unreal.PCGTransformPointsSettings)
 def pins(node,kind):
  arr=node.get_editor_property(kind+"_pins")
  result=[]
  for pin in arr:
   props=pin.get_editor_property("properties")
   result.append({"label":str(props.get_editor_property("label")),
                 "connected":bool(pin.is_connected()),
                 "output":bool(pin.is_output_pin())})
  return result
 r["pins"]={"input_out":pins(inp,"output"),"sampler_in":pins(surf,"input"),
   "sampler_out":pins(surf,"output"),"transform_in":pins(tr,"input"),
   "transform_out":pins(tr,"output"),"output_in":pins(outp,"input")}
 def l(a,kind):
  p=pins(a,kind)
  if not p:raise RuntimeError("Empty "+kind+" pins")
  return p[0]["label"]
 routes=[(inp,"output",surf,"input"),(surf,"output",tr,"input"),(tr,"output",outp,"input")]
 for n1,typ1,n2,typ2 in routes:
  graph.add_edge(n1,l(n1,typ1),n2,l(n2,typ2))
 unreal.EditorAssetLibrary.save_loaded_asset(graph)
 back=unreal.load_asset(graph.get_path_name())
 r["edge_count"]=len(back.get_all_edges())
 r["graph_path"]=back.get_path_name()
 r["node_count"]=4
 r["status"]="PASS_NATIVE_CONNECTED_PCG" if r["edge_count"]>=3 else "FAILED_PCG_EDGES"
except Exception:r["status"]="ERROR";r["error"]=traceback.format_exc()
OUT.write_text(json.dumps(r,indent=2))
print("NW_UE_PCG_REUSABLE="+json.dumps({k:v for k,v in r.items() if k not in ("pins","error")}))
