"""Read-only Blender 5.2.2 structural smoke test against existing 4.5.10 native authored assets.
Uses a dedicated 5.2.2 executable. Must not save/rewrite older sources.
"""
import bpy,os,json,sys,time,traceback
BASE=os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),"..","Blender_4.5.10")
BASE=os.path.normpath(BASE)
TARGETS=[
 ("sanctuary",os.path.join(BASE,"VALIDATION","phase4","NW_StarfallSanctum_ArtDirected.blend")),
 ("mount_skeletal",os.path.join(BASE,"VALIDATION","phase2","NW_ArticulatedMount_IK_Baked.blend")),
 ("hybrid_gp",os.path.join(BASE,"VALIDATION","phase2","NW_Hybrid_Strike.blend")),
 ("spin_profiles",os.path.join(BASE,"VALIDATION","reference_20261015","NW_SpinMultiProfile_Tooling.blend"))
]
out={"runtime_blender":bpy.app.version_string,
 "runtime_version":list(bpy.app.version),"source_version":"4.5.10",
 "no_source_files_modified":True,"tests":[]}
assert bpy.app.version[:3]==(5,2,2),bpy.app.version_string
for label,path in TARGETS:
 entry={"name":label,"source_file":path,"exists":os.path.isfile(path),"pass":False}
 try:
  assert os.path.isfile(path),path
  started=time.perf_counter()
  bpy.ops.wm.open_mainfile(filepath=path,load_ui=False)
  rigs=[o for o in bpy.data.objects if o.type=="ARMATURE"]
  mesh=[o for o in bpy.data.objects if o.type=="MESH"]
  gp=[o for o in bpy.data.objects if o.type=="GREASEPENCIL"]
  actions=[a.name for a in bpy.data.actions]
  assigned_slots=[o.name for o in rigs if o.animation_data and o.animation_data.action and o.animation_data.action_slot]
  if label=="mount_skeletal":
   assert len(rigs)==1 and len(rigs[0].data.bones)==16 and len(assigned_slots)==1
  if label=="hybrid_gp":
   assert len(gp)>=2 and len(actions)>0,{"gp":len(gp),"actions":actions}
  if label=="sanctuary":assert len(mesh)>800
  if label=="spin_profiles":assert len(mesh)==3
  entry.update(pass_=True,mesh_count=len(mesh),rig_bone_counts=[len(o.data.bones) for o in rigs],
      gp_count=len(gp),actions=actions,assigned_slots=assigned_slots,
      material_count=len(bpy.data.materials),
      procedural_node_material_count=sum(1 for m in bpy.data.materials if m.use_nodes),
      seconds=round(time.perf_counter()-started,3))
  entry["pass"]=True
 except Exception:
  entry["error"]=traceback.format_exc()
 out["tests"].append(entry)
out["passed"]=sum(t["pass"] for t in out["tests"])
out["total"]=len(out["tests"])
out["structural_compatibility_success"]=out["passed"]==out["total"]
out["artistic_compatibility_accepted"]=False
out["cycles_or_eevee_render_equivalence_proven"]=False
out["FBX_gameplay_parity_proven"]=False
path=os.path.join(os.path.dirname(__file__),"blender522_compatibility_results.json")
with open(path,"w",encoding="utf-8") as f:json.dump(out,f,indent=2)
print("NW_BLENDER522_COMPAT="+json.dumps({k:v for k,v in out.items() if k!="tests"}))
for r in out["tests"]:print("COMPAT_SCENE="+json.dumps({k:v for k,v in r.items() if k not in ("actions","error")}))
if out["passed"]!=out["total"]:raise SystemExit(2)
