"""Gate 01 / 07 modular authored weapon automation, Blender 5.2 source+FBX.
Spec-driven blade loft, fuller, guard, grip wrap, pommel, reusable attachment sockets,
artist-controlled metal roughness/anodized accents. Not a substitute for bespoke art.
"""
import bpy,math,json,sys,pathlib,hashlib
from mathutils import Vector
def dest(x):
 p=pathlib.Path(x).resolve()
 if p.exists():raise FileExistsError(p)
 p.parent.mkdir(parents=True,exist_ok=True);return p
def sha(x):return hashlib.sha256(pathlib.Path(x).read_bytes()).hexdigest()
def material(name,base,metal,rough,wear):
 m=bpy.data.materials.new(name);m.diffuse_color=(*base,1);m.use_nodes=True
 nt=m.node_tree;n=nt.nodes;lk=nt.links
 p=n.get("Principled BSDF")
 p.inputs["Metallic"].default_value=metal;p.inputs["Roughness"].default_value=rough
 tex=n.new("ShaderNodeTexNoise");tex.inputs["Scale"].default_value=wear
 tex.inputs["Detail"].default_value=4
 mapr=n.new("ShaderNodeValToRGB")
 mapr.color_ramp.elements[0].position=.28;mapr.color_ramp.elements[0].color=(*tuple(v*.57 for v in base),1)
 mapr.color_ramp.elements[1].position=.78;mapr.color_ramp.elements[1].color=(*base,1)
 lk.new(tex.outputs["Fac"],mapr.inputs["Fac"])
 lk.new(mapr.outputs["Color"],p.inputs["Base Color"])
 bump=n.new("ShaderNodeBump");bump.inputs["Strength"].default_value=.13
 bump.inputs["Distance"].default_value=.016
 lk.new(tex.outputs["Fac"],bump.inputs["Height"])
 lk.new(bump.outputs["Normal"],p.inputs["Normal"])
 return m
def loft(name,rings,material):
 count=len(rings[0][1]);v=[];faces=[]
 for z,profile in rings:
  assert len(profile)==count
  v.extend([(x,y,z) for x,y in profile])
 for i in range(len(rings)-1):
  for j in range(count):
   k=(j+1)%count
   faces.append((i*count+j,i*count+k,(i+1)*count+k,(i+1)*count+j))
 faces.append(tuple(range(count-1,-1,-1)))
 faces.append(tuple((len(rings)-1)*count+j for j in range(count)))
 mesh=bpy.data.meshes.new(name+"_Mesh");mesh.from_pydata(v,[],faces);mesh.update()
 ob=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(ob)
 mesh.materials.append(material)
 for face in mesh.polygons:face.use_smooth=True
 bevel=ob.modifiers.new("NW_SubtleForgedEdge","BEVEL");bevel.width=.006;bevel.segments=2
 norm=ob.modifiers.new("NW_ForgedNormals","WEIGHTED_NORMAL")
 return ob
def cube(name,location,scale,mat):
 bpy.ops.mesh.primitive_cube_add(size=1,location=location)
 o=bpy.context.object;o.name=name;o.dimensions=scale
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(mat)
 bevel=o.modifiers.new("NW_AccentRadius","BEVEL");bevel.width=.05;bevel.segments=3
 return o
def run(job):
 blend=dest(job["blend"]);fbx=dest(job["fbx"]);report=dest(job["report"])
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
 kind=job.get("style","glaive")
 if kind not in ("glaive","sword","spear"):raise ValueError("Unknown weapon style")
 ln=float(job.get("blade_length",2.5));width=float(job.get("blade_width",.23))
 assert .3<ln<9 and .03<width<.9
 mat=material("NW_BlueSteel_ForgedMetal",(.23,.42,.61),1,.28,35)
 dark=material("NW_DarkLeatherWrap",(.12,.08,.065),0,.65,40)
 gold=material("NW_WarmBronzeAccent",(.63,.43,.17),.82,.33,28)
 grip_l=float(job.get("grip_length",1.2))
 segments=28
 count=8
 rings=[]
 for i in range(segments):
  t=i/(segments-1)
  z=grip_l+ln*t
  w=width*(.72+math.sin(t*math.pi)*.28)*min(1,(1-t)*5)
  if kind=="spear":w*=.5
  if kind=="glaive":w*=1+.4*math.sin(t*math.pi*1.2)
  thick=.065*(1-t)*min(1,6*(1-t))+.004
  bevel=.82
  p=[(-w*.07,-thick),(-w,0),(-w*.07,thick),(0,thick*1.25),
     (w*.07,thick),(w,0),(w*.07,-thick),(0,-thick*1.25)]
  rings.append((z,p))
 blade=loft("NW_Forge_Blade",rings,mat)
 guard=cube("NW_Hilt_Crossguard",(0,0,grip_l-.05),(1.0,.17,.11),gold)
 handle=cube("NW_Grip_Shaft",(0,0,grip_l*.49),(.19,.19,grip_l*.98),dark)
 pommel=cube("NW_Hilt_Pommel",(0,0,-.035),(.25,.25,.22),gold)
 elems=[blade,guard,handle,pommel]
 for i in range(16):
  z=.14+i*((grip_l-.25)/16)
  strip=cube("NW_Grip_RaisedWrap_%02d"%i,(0,0,z),(.219,.222,.027),gold if i%4==0 else dark)
  strip.rotation_euler[2]=(i%2)*.08
  elems.append(strip)
 for i in range(3):
  band=cube("NW_Blade_RuneSlot_%02d"%i,(0,-.068,grip_l+.3+i*.46),(.08,.023,.19),gold)
  elems.append(band)
 for name,z in [("Socket_Grip",grip_l*.55),("Socket_BladeTip",grip_l+ln),
                ("Socket_Trail",grip_l+ln*.6)]:
  ob=bpy.data.objects.new(name,None);bpy.context.scene.collection.objects.link(ob)
  ob.location=(0,0,z);ob.empty_display_size=.12
  ob["nw_semantic_socket"]=name;ob["nw_attach_VFX_marker"]=True
 # Native editable source and procedural shader nodes.
 for ob in elems:
  if ob.type=="MESH" and not ob.data.uv_layers:
   bpy.ops.object.select_all(action="DESELECT")
   ob.select_set(True);bpy.context.view_layer.objects.active=ob
   bpy.ops.object.mode_set(mode="EDIT");bpy.ops.mesh.select_all(action="SELECT")
   bpy.ops.uv.smart_project(island_margin=.012)
   bpy.ops.object.mode_set(mode="OBJECT")
 assert all(o.data.uv_layers for o in elems)
 bpy.context.scene["nw_weapon_system"]="Gate01/Gate07 multi-style spec, preserved socket metadata"
 bpy.context.scene["nw_quality_gate"]="PROCEDURAL_PROTOTYPE_NOT_ARTIST_ACCEPTED"
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
 bpy.ops.object.select_all(action="DESELECT")
 for ob in elems:ob.select_set(True)
 bpy.context.view_layer.objects.active=elems[0]
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,
     object_types={"MESH"},add_leaf_bones=False,use_mesh_modifiers=True,bake_anim=False)
 r={"blender":bpy.app.version_string,"style":kind,"length":ln,
  "editable_source":str(blend),"fbx":str(fbx),"sha256":sha(fbx),
  "mesh_objects":len(elems),"semantic_sockets":["Socket_Grip","Socket_BladeTip","Socket_Trail"],
  "material_count":3,"UV_status":"Requires engine UV inspection",
  "native_unreal_tested":False,"artist_accepted":False}
 report.write_text(json.dumps(r,indent=2),encoding="utf-8")
 print("NW_GATE01_WEAPON="+json.dumps(r))
if __name__=="__main__":
 a=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 assert len(a)==1
 run(json.loads(pathlib.Path(a[0]).read_text(encoding="utf-8-sig")))
