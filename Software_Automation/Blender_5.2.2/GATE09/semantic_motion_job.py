"""NoWorld Gate09 automated semantic animation source generator.
Generate edit-ready Blender 5.2 Action + Action Slot and UE-compatible anim-only FBX.
Blends rig-semantic high-level controls; no per-bone manual keyframing or videos.
Artistic/IK acceptance remains separate. Winged creature and humanoid profiles supported.
"""
import bpy,sys,pathlib,json,hashlib,math,time
from mathutils import Vector
def check(p):
 p=pathlib.Path(p).resolve()
 if p.exists():raise FileExistsError(p)
 p.parent.mkdir(parents=True,exist_ok=True)
 return p
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def match(rig,term):
 exact=rig.pose.bones.get(term)
 if exact:return exact
 for b in rig.pose.bones:
  if b.name.lower()==term.lower():return b
 return None
def run(job):
 source=pathlib.Path(job["source_blend"]).resolve()
 blend=check(job["outputs"]["blend"]);fbx=check(job["outputs"]["fbx"]);report=check(job["outputs"]["report"])
 if not source.is_file():raise FileNotFoundError(source)
 before=sha(source)
 bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
 rig=bpy.data.objects.get(job["rig"])
 if not rig or rig.type!="ARMATURE":raise ValueError("Expected actual armature "+str(job["rig"]))
 kind=job["motion"].lower()
 if kind not in {"flight","swim","dance","run"}:raise ValueError("Unsupported motion type "+kind)
 fps=int(job.get("fps",24));intervals=int(job.get("frames",48))
 assert 12<=intervals<=240
 scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=intervals+1;scene.render.fps=fps
 speed=float(job.get("speed",1.0))
 amp=float(job.get("intensity",.85))
 if not 0<=amp<=2:raise ValueError("Intensity outside safe range")
 allbones={b.name:b for b in rig.pose.bones}
 anatomy=job.get("anatomy","winged_quadruped")
 spec={
   "winged_quadruped":{"root":"Root","body":"Body","neck":"Neck","tail":["TailA","TailB"],
      "wings":["WingL","WingR"],
      "legs":["FrontL_Upper","FrontL_Lower","FrontR_Upper","FrontR_Lower",
         "HindL_Upper","HindL_Lower","HindR_Upper","HindR_Lower"]},
   "humanoid":{"root":"root","body":"spine_02","neck":"neck",
      "tail":[],"wings":["wing_l","wing_r"],
      "legs":["thigh_l","calf_l","thigh_r","calf_r"]}
 }
 if anatomy not in spec:raise ValueError("Semantic anatomy missing "+anatomy)
 tags=spec[anatomy]
 if kind=="flight" and not all(match(rig,n) for n in tags["wings"]):
  raise RuntimeError("True flight needs wing articulation or explicit rig mapping; refusing generic arm imitation")
 if kind=="swim" and not match(rig,tags["body"]):
  raise RuntimeError("Swimming requires torso source and propulsion structure")
 anim=rig.animation_data_create()
 anim.action=None
 action=bpy.data.actions.new("NW_"+kind.title()+"_"+anatomy+"_Parametric24")
 slot=action.slots.new("OBJECT",rig.name)
 anim.action=action;anim.action_slot=slot
 # All controls are authored by phase equations, not operator clicking or hand posing.
 targets=set()
 for value in tags.values():
  if isinstance(value,str):targets.add(value)
  elif isinstance(value,list):targets.update(value)
 active={name:match(rig,name) for name in targets}
 active={name:b for name,b in active.items() if b}
 for bone in active.values():bone.rotation_mode="XYZ"
 root=match(rig,tags["root"]);assert root is not None
 frames=list(range(1,intervals+1,3))
 if 1 not in frames:frames.insert(0,1)
 if intervals+1 not in frames:frames.append(intervals+1)
 # Additional branch-level speed modulation, shape parameters and designed asymmetry.
 for frame in frames:
  t=(frame-1)/intervals
  ph=math.tau*t*speed
  def setrot(name,x=0,y=0,z=0):
   b=match(rig,name)
   if b is None:return
   b.rotation_euler=(x,y,z)
   b.keyframe_insert(data_path="rotation_euler",frame=frame,group="NW_"+kind)
  def setloc(name,x=0,y=0,z=0):
   b=match(rig,name)
   if b is None:return
   b.location=(x,y,z)
   b.keyframe_insert(data_path="location",frame=frame,group="NW_"+kind)
  travel=float(job.get("root_travel_per_cycle",0.0)) * t
  if kind=="flight":
   flap=math.sin(ph)
   setrot(tags["wings"][0],0,amp*.67*flap,amp*.13*math.sin(ph+.3))
   setrot(tags["wings"][1],0,-amp*.67*flap,-amp*.13*math.sin(ph+.3))
   setrot(tags["body"],.045*amp*math.sin(ph+.25),0,.04*amp*math.sin(ph))
   setrot(tags["neck"],-.06*amp*math.sin(ph+.45),0,0)
   for i,name in enumerate(tags["tail"]):setrot(name,.13*amp*math.sin(ph-.35-.45*i),0,.08*amp*math.sin(ph-.4*i))
   for i,name in enumerate(tags["legs"]):setrot(name,(-.10 if i%2 else .15)*amp,0,0)
   setloc(tags["root"],travel,0,.08*amp*math.sin(ph+.3))
  elif kind=="swim":
   setrot(tags["body"],.13*amp*math.sin(ph),.03*amp*math.sin(ph),0)
   for i,name in enumerate(tags["tail"]):setrot(name,.31*amp*math.sin(ph-.5*i),0,0)
   for i,name in enumerate(tags["wings"]):setrot(name,.10*amp*math.sin(ph+i*math.pi),0,0)
   for i,name in enumerate(tags["legs"]):setrot(name,.22*amp*math.sin(ph+i*math.pi*.55),0,0)
   setrot(tags["neck"],-.08*amp*math.sin(ph-.6),0,0)
   setloc(tags["root"],travel,0,.025*amp*math.sin(ph))
  elif kind=="dance":
   setrot(tags["body"],.15*amp*math.sin(ph),.07*amp*math.sin(ph+.6),.13*amp*math.sin(ph*2))
   setrot(tags["neck"],-.07*amp*math.sin(ph+.25),0,.05*amp*math.sin(ph))
   for i,name in enumerate(tags["wings"]):setrot(name,.18*amp*math.sin(ph+i*math.pi),.15*amp*math.sin(ph*.5),0)
   for i,name in enumerate(tags["legs"]):setrot(name,.11*amp*math.sin(ph+i*math.pi*.5),0,0)
   setloc(tags["root"],travel,0,.07*amp*(1-math.cos(ph*2)))
  else:
   # Running is a fallback BLOCKING pass. For polished foot contact use preexisting
   # physical contact IK bake or retarget professional motion sources.
   setrot(tags["body"],.08*amp*math.sin(ph*2),0,0)
   for i,name in enumerate(tags["legs"]):setrot(name,.25*amp*math.sin(ph*2+(i%4)*math.pi*.5),0,0)
   setloc(tags["root"],travel,0,.05*amp*(1-math.cos(ph*2)))
 # Save action as native reusable asset; Blender Asset Browser can reuse without rerigging.
 action.asset_mark()
 action.asset_data.description="NoWorld semantic "+kind+" loop, procedural parameters: "+json.dumps({
    k:job.get(k) for k in ("intensity","speed","root_travel_per_cycle")})
 rig["nw_motion_kind"]=kind
 rig["nw_motion_semantics"]=json.dumps(tags)
 rig["nw_action_source_kind"]="PARAMETRIC_BLOCKING_NOT_ARTIST_APPROVED"
 scene.frame_set(1)
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
 bpy.ops.object.select_all(action="DESELECT")
 rig.select_set(True);bpy.context.view_layer.objects.active=rig
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,
    object_types={"ARMATURE"},add_leaf_bones=False,bake_anim=True,
    bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
    bake_anim_use_all_actions=False,bake_anim_simplify_factor=0)
 assert sha(source)==before
 results={"tool":"Gate09 semantic automated Blender motions","blender":bpy.app.version_string,
    "source":str(source),"source_preserved":True,"action":action.name,
    "slot":slot.identifier,"kind":kind,"anatomy":anatomy,
    "animated_semantic_bones":list(active.keys()),
    "total_semantic_bones":len(active),"frames":scene.frame_end,
    "fps":fps,"parameters":{k:job.get(k) for k in ("speed","intensity","root_travel_per_cycle")},
    "source_blend":str(blend),"source_sha256":sha(blend),
    "anim_only_fbx":str(fbx),"fbx_sha256":sha(fbx),
    "art_quality_accepted":False,"unreal_native_imported":False,
    "foot_contact_accepted":False,"looped_game_animation_possible":True}
 report.write_text(json.dumps(results,indent=2),encoding="utf-8")
 print("NOWORLD_GATE09_SEMANTIC="+json.dumps(results))
if __name__=="__main__":
 a=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 if len(a)!=1:raise SystemExit("Need JSON config filepath")
 run(json.loads(pathlib.Path(a[0]).read_text(encoding="utf-8")))
