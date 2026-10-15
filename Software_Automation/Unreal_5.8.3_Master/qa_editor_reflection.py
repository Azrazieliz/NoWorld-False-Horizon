import unreal,json,pathlib,traceback
r={"engine":unreal.SystemLibrary.get_engine_version(),"reflection":{}}
def ref(name,obj):
 try:
  d={"class":str(type(obj)),"methods":[k for k in dir(obj) if not k.startswith("_")][:130]}
  for meth in ("get_all_channels","get_channels","add_key","get_keys","get_input_pins","get_output_pins","add_edge","add_node_of_type"):
   try:
    x=getattr(obj,meth)
    d[meth+"_doc"]=str(getattr(x,"__doc__",""))[:1400]
   except:pass
  return d
 except:return {"error":traceback.format_exc()[-700:]}
s=unreal.load_asset("/Game/NW_MasterHub_QA/SoftwareMaster_Native/NW_Sequencer_CameraKeyed_QA")
if s:
 try:
  sec=s.get_bindings()[0].get_tracks()[0].get_sections()[0]
  r["reflection"]["section"]=ref("section",sec)
  ex=unreal.MovieSceneSectionExtensions
  r["reflection"]["extensions"]=ref("ext",ex)
  channels=ex.get_all_channels(sec)
  r["reflection"]["channel_count"]=len(channels)
  r["reflection"]["channels"]=[ref("channel",x) for x in channels]
 except:r["reflection"]["seq_error"]=traceback.format_exc()
g=unreal.load_asset("/Game/NW_MasterHub_QA/SoftwareMaster_Native/NW_PCG_TerrainAnchors_QA")
if g:
 try:
  r["reflection"]["PCGGraph"]=ref("graph",g)
  n=g.get_input_node()
  r["reflection"]["PCGNode"]=ref("node",n)
  for key in ["input_pins","output_pins","settings","node_title"]:
   try:
    obj=n.get_editor_property(key)
    r["reflection"]["pcg_"+key]=str(obj)[:1500]
    if key=="output_pins":
     r["reflection"]["output_properties"]=[ref("pin",x) for x in obj[:3]]
   except Exception as e:r["reflection"]["pcg_"+key+"_error"]=str(e)
 except:r["reflection"]["pcg_error"]=traceback.format_exc()
dst=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\Unreal_PCG_Sequencer_RuntimeReflection.json")
dst.write_text(json.dumps(r,indent=2))
print("NOWORLD_REFLECT="+json.dumps({"seq_channels":r["reflection"].get("channel_count"),"pcg":bool(g),"report":str(dst)}))
