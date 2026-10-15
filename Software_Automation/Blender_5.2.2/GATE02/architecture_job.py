"""Gate 02: unattended Blender 5.2.2 architecture from the GPL Arch Generator 1.0.1.
Declarative JSON parts -> native adjustable Blender arches and Unreal FBX.
The Blender Arch Generator extension MUST be installed and enabled.
Test geometry is not creative-canon approval.
"""
import bpy,sys,pathlib,json,hashlib,addon_utils,time
from mathutils import Vector
BASE=pathlib.Path(__file__).resolve().parents[1]
def check(path):
 p=pathlib.Path(path).resolve()
 if p.exists():raise FileExistsError("Refuse overwrite: "+str(p))
 p.parent.mkdir(parents=True,exist_ok=True)
 return p
def hashfile(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 if len(args)!=1:raise SystemExit("Usage: blender --python architecture_job.py -- job.json")
 job_path=pathlib.Path(args[0]).resolve()
 payload=json.loads(job_path.read_text(encoding="utf-8"))
 assert bpy.app.version[:2]==(5,2),bpy.app.version_string
 if not 1<=len(payload.get("parts",[]))<=24:raise ValueError("1 to 24 modules per job")
 out=payload["outputs"]
 blend=check(out["blend"]);fbx=check(out["fbx"]);report=check(out["report"])
 module="bl_ext.blender_org.arch_generator"
 addon_utils.enable(module)
 assert addon_utils.check(module)[1],"Arch Generator addon not enabled"
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
 props_allowed={"mode","frame_type","arch_style","width","height","depth",
   "thickness","arch_segments","pillar_style","trim_resolution","base_style",
   "capital_style","arch_count","gap_between_arches","stambha_height",
   "base_width","base_height","capital_width","capital_height","flute_count",
   "pillar_solomonic_twists","mughal_cusps_count","enable_support_beam"}
 created=[]
 for number,item in enumerate(payload["parts"]):
  bpy.context.scene.cursor.location=tuple(item.get("location",[0,0,0]))
  assert "FINISHED" in bpy.ops.mesh.arch_create()
  o=bpy.context.active_object
  o.name=item.get("id",f"NW_Module_{number:02d}")
  prop=o.arch_props
  for k,v in item.items():
   if k in props_allowed:setattr(prop,k,v)
  o["nw_gate_owner"]="Gate02_EnvironmentArchitecture"
  o["nw_world_anchor"]=str(payload.get("world_anchor","UNASSIGNED"))
  o["nw_stored_module_spec"]=json.dumps(item,sort_keys=True)
  if not o.data.polygons:raise RuntimeError(o.name+" generated no faces")
  if not o.data.uv_layers:
   bpy.ops.object.select_all(action="DESELECT")
   o.select_set(True);bpy.context.view_layer.objects.active=o
   bpy.ops.object.mode_set(mode="EDIT");bpy.ops.mesh.select_all(action="SELECT")
   bpy.ops.uv.smart_project(island_margin=.012)
   bpy.ops.object.mode_set(mode="OBJECT")
  if not o.data.uv_layers:raise RuntimeError("Missing UV layer on "+o.name)
  created.append(o)
 # Generated editable meshes and live Arch Properties in a separate source file.
 bpy.context.scene["nw_source_contract"]="Editable architectural modules; Gaea height/UE layout requires Gate02 intake"
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
 bpy.ops.object.select_all(action="DESELECT")
 for o in created:o.select_set(True)
 bpy.context.view_layer.objects.active=created[0]
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,
    object_types={"MESH"},bake_anim=False,apply_unit_scale=True)
 from mathutils import Vector
 result={"tool":"NoWorld Blender/Gate02 Architecture","version":bpy.app.version_string,
   "addon":module,"modules":len(created),
   "source_spec":str(job_path),"source_spec_sha256":hashfile(job_path),
   "blend":str(blend),"blend_sha256":hashfile(blend),
   "fbx":str(fbx),"fbx_sha256":hashfile(fbx),
   "all_uv":all(o.data.uv_layers for o in created),
   "modules_detail":[{"name":o.name,"vertices":len(o.data.vertices),
       "polygons":len(o.data.polygons),
       "style":o.arch_props.arch_style,"pillar":o.arch_props.pillar_style,
       "editable_parameters":True} for o in created],
   "source_design_identity_created":False,"art_quality_accepted":False,
   "gaea_terrain_parity":False,"native_unreal_tested":False}
 report.write_text(json.dumps(result,indent=2),encoding="utf-8")
 print("NW_GATE02_ARCH_PROOF="+json.dumps(result))
if __name__=="__main__":main()
