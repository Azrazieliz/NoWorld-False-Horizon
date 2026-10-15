"""PRIVATE NONCANONICAL facial matte/alpha repair trial, no source GLB edits.
Look for the 3 original eyeline materials. Add artist-controlled alpha ramp before
Principled Alpha, clip invalid shadow remnants but preserve eyelash silhouettes.
Evaluate visually (cannot claim a fix from alpha routing alone).
"""
import bpy,json,os,time
ROOT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
T=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(ROOT,"NW_ChikaHaruno_Acting72Frames_NONCANONICAL.blend"))
scene=bpy.context.scene
targets=("cf_m_eyeline_00_up","cf_m_eyeline_down","cf_m_eyeline_kage")
records=[]
for ob in [o for o in bpy.data.objects if o.type=="MESH"]:
 for slot in ob.material_slots:
  old=slot.material
  if not old or not any(t in old.name.lower() for t in targets):continue
  m=old.copy();m.name="NW_ALPHA_TUNED_"+old.name
  nodes=m.node_tree.nodes;links=m.node_tree.links
  bsdf=next(n for n in nodes if n.type=="BSDF_PRINCIPLED")
  socket=bsdf.inputs["Alpha"]
  links_in=list(socket.links)
  assert links_in,"eyeline source alpha not connected; no safe repair"
  prev=links_in[0].from_socket
  links.remove(links_in[0])
  ramp=nodes.new("ShaderNodeValToRGB")
  ramp.label="NW eyelash edge cutoff, fine alpha recovery"
  ramp.location=bsdf.location+__import__("mathutils").Vector((-240,-180))
  ramp.color_ramp.interpolation="EASE"
  ramp.color_ramp.elements[0].position=.34
  ramp.color_ramp.elements[1].position=.78
  ramp.color_ramp.elements[0].color=(0,0,0,1)
  ramp.color_ramp.elements[1].color=(1,1,1,1)
  links.new(prev,ramp.inputs["Fac"])
  links.new(ramp.outputs["Color"],socket)
  m["nw_noncanonical_alpha_repair_test"]=True
  slot.material=m
  records.append({"source":old.name,"derived":m.name,"input":prev.name,
                  "cutoff_black":.34,"cutoff_white":.78})
assert len(records)>=3,records
scene.frame_set(50)
scene.render.engine="CYCLES";scene.cycles.samples=18
scene.render.threads_mode="FIXED";scene.render.threads=2
scene.render.resolution_x=600;scene.render.resolution_y=760
scene.render.image_settings.file_format="PNG"
dest=os.path.join(ROOT,"NW_Character_EyelashAlphaExperiment_PRIVATE.blend")
bpy.ops.wm.save_as_mainfile(filepath=dest,compress=True)
preview=os.path.join(ROOT,"NW_Character_EyelashAlphaExperiment_PRIVATE.png")
scene.render.filepath=preview;bpy.ops.render.render(write_still=True)
report={"blender":bpy.app.version_string,"mode":"NONCANONICAL_AlphaMask_Compare",
 "source_glb_preserved":True,"derivative_materials":records,
 "source":dest,"preview":preview,"seconds":round(time.perf_counter()-T,2),
 "visual_quality_accepted":False,"original_KK_parity":False}
with open(os.path.join(ROOT,"eye_alpha_trial.json"),"w") as f:json.dump(report,f,indent=2)
print("NW_EYE_TRIAL="+json.dumps(report))
