"""Inspect complete installed UE 5.8.3 Python surface, including plugin APIs.
Machine-readable, search-first asset for Gate12, no GUI clicks or production writes.
"""
import unreal,json,datetime,pathlib,inspect,re
PATH=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\UE58_Complete_PythonAPI_Registry.json")
cands=[n for n in dir(unreal) if not n.startswith("_")]
terms=("create","set_","get_","import","export","save","load","spawn","find","compile",
 "build","run_","evaluate","update","query","add_","remove","validate","bake","render","cook","retarget")
registry={}
bydomain={}
def domain(name):
 lower=name.lower()
 rules={"Blueprint_Cpp":["blueprint","kismet","subobject","native","compiler"],
 "Gameplay_AI":["ability","effect","tag","statetree","ai","behavior","blackboard","nav","mass","input","savegame"],
 "World_Procedural":["pcg","landscape","worldpartition","foliage","geometryscript","water","river","terrain","grid"],
 "Animation":["anim","skeleton","retarget","ikrig","controlrig","pose","motionmatching","morph","rig"],
 "Rendering_VFX":["material","texture","niagara","render","shader","postprocess","light","lumen"],
 "Cinematics":["sequence","moviescene","camera","levelsequence","moviepipeline","cinematic"],
 "Sound_UI":["metasound","audio","sound","widget","umg","dialogue"],
 "Asset_Engineering":["asset","editor","package","automation","test","validation","cook","android","platform","build","data"]}
 for label,keys in rules.items():
  if any(x in lower for x in keys):return label
 return "Other"
for name in cands:
 try:
  obj=getattr(unreal,name)
  typ=type(obj).__name__
  doc=(getattr(obj,"__doc__",None) or "")
  methods=[]
  if typ in ("type","_UnrealClass") or inspect.isclass(obj):
   methods=[m for m in dir(obj) if any(m.lower().startswith(t) for t in terms)]
  registry[name]={"kind":typ,"domain":domain(name),"description":str(doc)[:280],
    "methods":methods[:170]}
  bydomain[registry[name]["domain"]]=bydomain.get(registry[name]["domain"],0)+1
 except Exception as ex:registry[name]={"kind":"ERROR","message":str(ex)[:120]}
payload={"engine":unreal.SystemLibrary.get_engine_version(),
 "created_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
 "scope":"loaded Unreal Python bindings; a missing name may require enabling another plugin",
 "count":len(registry),"domains":bydomain,"classes":registry}
PATH.write_text(json.dumps(payload,separators=(",",":")),encoding="utf-8")
print("NW_UNREAL_API_ALL="+json.dumps({"engine":payload["engine"],"entries":len(registry),
 "domains":bydomain,"bytes":PATH.stat().st_size,"path":str(PATH)}))
