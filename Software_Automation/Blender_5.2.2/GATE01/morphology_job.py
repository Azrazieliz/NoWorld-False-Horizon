"""Gate01 monster/mount/nonhuman topology adjustment engine.
Source-preserving RBF anatomical vertex-group morphs, morph-key asset library,
and optional skinned tapered procedural horn structures.
Uses existing supplied Blender rig/source rather than replacing KK models.
Character-specific sculpt/review required for artistic approval.
"""
import bpy,sys,json,math,pathlib,hashlib
from mathutils import Vector,Matrix
def outpath(s):
 p=pathlib.Path(s).resolve()
 if p.exists():raise FileExistsError("Refusing to overwrite "+str(p))
 p.parent.mkdir(parents=True,exist_ok=True);return p
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def weight(o,bone,i):
 vg=o.vertex_groups.get(bone)
 if not vg:return 0
 try:return vg.weight(i)
 except Exception:return 0
def morph(arm,config):
 bone=config["bone"]
 if bone not in arm.pose.bones:raise ValueError("No skeleton bone "+bone)
 mode=config.get("mode","inflate")
 if mode not in ("inflate","elongate","taper","twist","offset","flatten"):
  raise ValueError("Unsupported morph "+mode)
 strength=float(config.get("strength",.4))
 if abs(strength)>2:raise ValueError("Morph strength exceeds bounded safe range")
 affected=[]
 meshes=[o for o in bpy.data.objects if o.type=="MESH" and
    any(m.type=="ARMATURE" and m.object==arm for m in o.modifiers)]
 for mesh in meshes:
  vg=mesh.vertex_groups.get(bone)
  if not vg:continue
  if not mesh.data.shape_keys:
   mesh.shape_key_add(name="Basis",from_mix=False)
  k=mesh.shape_key_add(name="NW_"+config.get("id",mode+"_"+bone),from_mix=False)
  inv=mesh.matrix_world.inverted_safe()
  bone_start=arm.matrix_world @ arm.data.bones[bone].head_local
  bone_end=arm.matrix_world @ arm.data.bones[bone].tail_local
  along=(bone_end-bone_start).normalized()
  changed=0
  radius=float(config.get("radius",max((bone_end-bone_start).length,.15)))
  for i,v in enumerate(mesh.data.vertices):
   w=weight(mesh,bone,i)
   if w<=0:continue
   p=mesh.matrix_world @ v.co
   depth=(p-bone_start).dot(along)
   center=bone_start+along*depth
   radial=p-center
   fade=math.exp(-(max(depth,0)/max(radius,0.01))**2*.16)
   if mode=="inflate":target=p+radial*strength*w*.5*fade
   elif mode=="elongate":target=p+along*strength*w*max(radius,.05)
   elif mode=="taper":target=p-radial*strength*w*max(0,min(1,depth/max(radius,.01)))
   elif mode=="flatten":target=p+Vector((0,0,1))*strength*w*.25-radial*strength*w*.13
   elif mode=="offset":target=p+Vector(config.get("offset",[0,0,.1]))*strength*w
   else:
    from mathutils import Quaternion
    angle=strength*w*depth/max(radius,.01)
    target=center+Quaternion(along,angle)@radial
   k.data[i].co=inv @ target;changed+=1
  if changed:
   k.value=float(config.get("preview_value",0.6))
   affected.append({"object":mesh.name,"shape_key":k.name,"vertices":changed,
     "value":k.value,"preserves_basis":True})
 return affected
def horn(arm,config):
 bone=config["bone"]
 b=arm.data.bones.get(bone)
 if not b:raise ValueError("Horn anchor bone missing "+bone)
 length=float(config.get("length",.9));radius=float(config.get("radius",.16))
 assert .05<length<10 and .015<radius<2
 side=float(config.get("side",1.0))
 center=(b.head_local+b.tail_local)/2+Vector((side*.20,0,.2))
 n=int(config.get("resolution",28));n=max(12,min(n,120))
 rings=9
 verts=[];faces=[]
 for j in range(rings):
  t=j/(rings-1)
  p=center+Vector((side*.23*t+.18*math.sin(t*math.pi),-.24*t,length*t))
  rad=radius*(1-t)**1.4+.003
  for k in range(n):
   a=2*math.pi*k/n
   verts.append((p.x+rad*math.cos(a),p.y+rad*math.sin(a),p.z))
 for j in range(rings-1):
  for k in range(n):
   a=j*n+k;b0=j*n+(k+1)%n
   faces.append((a,b0,b0+n,a+n))
 m=bpy.data.meshes.new("NW_Horn_Geometry")
 m.from_pydata(verts,[],faces);m.update()
 ob=bpy.data.objects.new("NW_CustomAnatomy_"+str(config.get("id","Horn")),m)
 bpy.context.scene.collection.objects.link(ob)
 ob.matrix_world=arm.matrix_world.copy()
 vg=ob.vertex_groups.new(name=bone)
 vg.add(list(range(len(verts))),1,"REPLACE")
 mod=ob.modifiers.new("NW_DeformedByExistingCreatureRig","ARMATURE")
 mod.object=arm
 ob["nw_nonhuman_anatomy_tool"]=True
 ob["nw_anchor_bone"]=bone
 # Keep removable and independently editable; never rewrite original character mesh.
 bevel=ob.modifiers.new("NW_FineEdgeControl","BEVEL")
 bevel.width=.007;bevel.segments=2
 normal=ob.modifiers.new("NW_NormalFlow","WEIGHTED_NORMAL")
 return {"name":ob.name,"vertex_count":len(verts),"faces":len(faces),
 "anchor_bone":bone,"user_editable":True,"existing_skeleton_preserved":True}
def main():
 args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 assert len(args)==1
 job=json.loads(pathlib.Path(args[0]).read_text(encoding="utf-8"))
 src=pathlib.Path(job["source"]).resolve()
 if not src.exists():raise FileNotFoundError(src)
 before=sha(src)
 blend=outpath(job["output_blend"]);report=outpath(job["report"])
 bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
 arm=bpy.data.objects.get(job["rig"])
 assert arm and arm.type=="ARMATURE"
 original_bones=[b.name for b in arm.data.bones]
 res=[]
 for cfg in job.get("morphs",[]):
  a=morph(arm,cfg)
  if not a:raise RuntimeError("No vertices affected by "+str(cfg))
  res+=a
 extra=[horn(arm,s) for s in job.get("anatomy",[])]
 assert res or extra
 assert [b.name for b in arm.data.bones]==original_bones
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
 assert sha(src)==before
 r={"blender":bpy.app.version_string,"source_unchanged":True,
    "source_sha256":before,"skeleton_bones_preserved":len(original_bones),
    "controlled_shape_keys":res,"new_anatomical_additions":extra,
    "derived_blend":str(blend),"derived_sha256":sha(blend),
    "creation_method":"bone-weighted spatial procedural morphology, zero manual sculpt clicks",
    "professional_organic_sculpt_accepted":False,"unreal_imported":False,
    "original_kk_character_unchanged":True}
 report.write_text(json.dumps(r,indent=2),encoding="utf-8")
 print("NW_GATE01_NONHUMAN="+json.dumps(r))
if __name__=="__main__":main()
