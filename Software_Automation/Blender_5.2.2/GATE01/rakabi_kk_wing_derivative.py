"""Gate01 Rakabi private 3D rough candidate: native KK 121-bone body + SM01
wing assets (actual geometry), horned crown, pallid hair and feather armor.
Reference-likeness still not accepted; source rig, card and mods preserved.
"""
import bpy,pathlib,json,hashlib,math,random
from mathutils import Vector
root=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261015")
src=root/"NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"
wings=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\MODDING\GENERIC_KIT\SM01_GENERIC_MODULAR_KIT.blend")
out=root/"Rakabi_Full3D_Private"
out.mkdir(exist_ok=True)
source3d=out/"Rakabi_WingedKnight_EDITABLE_PRIVATE.blend"
preview=out/"Rakabi_WingedKnight_AutoFront.png"
report=out/"Rakabi_WingedKnight_QA.json"
if any(x.exists() for x in (source3d,preview,report)):raise FileExistsError("Refuse overwrite")
old={str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in [src,wings]}
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
s=bpy.context.scene
rig=bpy.data.objects["NW_KK_INTENDED_SKELETON_121"]
human=bpy.data.objects["NW_SHARD03_model"]
hair=bpy.data.objects["NW_SHARD04_Hair Outfit 00 ChikaHaruno.004"]
outfit=bpy.data.objects["NW_SHARD05_Outfit 00 ChikaHaruno.005"]
for o in list(bpy.data.objects):
 if o.type=="MESH" and o.name.startswith(("NW_SHARD00","NW_SHARD01","NW_SHARD02")):o.hide_render=True
def principled(name,color,rough=.6,metal=0,em=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
 p=m.node_tree.nodes.get("Principled BSDF")
 p.inputs["Base Color"].default_value=(*color,1)
 p.inputs["Roughness"].default_value=rough;p.inputs["Metallic"].default_value=metal
 if em:
  p.inputs["Emission Color"].default_value=(*color,1);p.inputs["Emission Strength"].default_value=em
 return m
pale=principled("NW_Rakabi_PaleSkin",(.58,.51,.49))
ivory=principled("NW_Rakabi_PaleWingIvory",(.66,.61,.53),.67)
black=principled("NW_Rakabi_DarkFeathers",(.075,.062,.065),.65)
armor=principled("NW_Rakabi_BurnishedSteel",(.18,.17,.20),.26,.83)
gold=principled("NW_Rakabi_PaleGold",(.60,.44,.28),.33,.76)
icehair=principled("NW_Rakabi_WhiteHair",(.82,.79,.73),.42)
for i,m in enumerate(human.data.materials):
 if m and any(x in m.name for x in ("cf_m_body","cf_m_face_00")):human.data.materials[i]=pale
for i in range(len(hair.data.materials)):hair.data.materials[i]=icehair
for i,m in enumerate(outfit.data.materials):
 if i in (0,2,3):outfit.data.materials[i]=armor
 elif i in (4,5):outfit.data.materials[i]=black
 else:outfit.data.materials[i]=gold
# Physical SM01 wing meshes, original source remains untouched.
with bpy.data.libraries.load(str(wings),link=False) as (source,dest):
 wanted=[name for name in source.objects if name.startswith("SM01_Feather_") or name.startswith("SM01_BatMembrane_")]
 dest.objects=wanted
imported=0
for ob in dest.objects:
 if ob and ob.type=="MESH":
  s.collection.objects.link(ob)
  ob.name="NW_Rakabi_"+ob.name
  ob.data.materials.clear()
  ob.data.materials.append(black if "Membrane" in ob.name else ivory)
  imported+=1
assert imported>=50,imported
# Longer genuine 3D white hair wisps.
cu=bpy.data.curves.new("NW_Rakabi_WhiteHair_MultiStrands","CURVE")
cu.dimensions="3D";cu.resolution_u=8;cu.bevel_depth=.0022;cu.bevel_resolution=2
for i in range(95):
 ang=2*math.pi*i/95
 sp=cu.splines.new("BEZIER");sp.bezier_points.add(7)
 for j,pt in enumerate(sp.bezier_points):
  t=j/7
  pt.co=(math.cos(ang)*(.09+.1*t*t), .035+math.sin(ang)*(.07+.045*t),
        1.55-(.3+.75*(.5+.5*math.sin(ang*3+i)))*t)
  pt.handle_left_type="AUTO";pt.handle_right_type="AUTO"
  pt.radius=max(.03,(1-t)**1.2)
ob=bpy.data.objects.new("NW_Rakabi_Hair_95Strands",cu)
s.collection.objects.link(ob)
cu.materials.append(icehair)
# Anatomically positional crown spines, editable source not planar decoration.
def cone(name,base,tip,radius,material,vertices=12):
 axis=Vector(tip)-Vector(base)
 mid=(Vector(tip)+Vector(base))*.5
 bpy.ops.mesh.primitive_cone_add(vertices=vertices,radius1=radius,radius2=.001,
                                  depth=axis.length,location=mid)
 ob=bpy.context.object;ob.name=name
 ob.rotation_euler=axis.to_track_quat("Z","Y").to_euler()
 ob.data.materials.append(material)
 bevel=ob.modifiers.new("NW_Rakabi_OrganicEdge","BEVEL");bevel.width=.007;bevel.segments=2
 return ob
for i in range(11):
 a=2*math.pi*i/11
 start=(.085*math.cos(a),.045+.075*math.sin(a),1.56)
 tip=(.19*math.cos(a),.045+.155*math.sin(a),1.69+(.10 if i%2==0 else .04))
 cone(f"NW_Rakabi_CrownSpine_{i:02}",start,tip,.025,gold)
for side in (-1,1):
 for i in range(9):
  st=(side*(.12+.013*i),.016,1.22-i*.045)
  top=(st[0]+side*.10,st[1]-.07,st[2]+.12)
  cone(f"NW_Rakabi_ShoulderFeatherArmor_{side}_{i}",st,top,.035,ivory)
# More readable cinematic lighting, source scene lights retained.
cam=s.camera
cam.location=(.2,-4.4,1.2)
cam.rotation_euler=(Vector((0,0,1.04))-cam.location).to_track_quat("-Z","Y").to_euler()
cam.data.type="ORTHO";cam.data.ortho_scale=2.65
s.render.engine="CYCLES";s.cycles.samples=24;s.cycles.use_denoising=True
s.render.resolution_x=620;s.render.resolution_y=850
s.render.resolution_percentage=100
s.render.threads_mode="FIXED";s.render.threads=2
s.render.image_settings.file_format="PNG"
s.render.filepath=str(preview)
bpy.ops.render.render(write_still=True)
s["nw_reference_status"]="Rakabi candidate: Not visually accepted"
s["nw_candidate_type"]="Character geometry; editable 3D not an image"
bpy.ops.wm.save_as_mainfile(filepath=str(source3d),compress=True)
for x,sha in old.items():
 if hashlib.sha256(pathlib.Path(x).read_bytes()).hexdigest()!=sha:raise RuntimeError("Protected source overwritten")
r={"candidate":"White Feather Rakabi",
 "source_blend_preserved":True,"sm01_wing_source_preserved":True,
 "native_rig_bones":len(rig.data.bones),"wing_mesh_parts":imported,
 "procedural_hair_strands":95,"crown_spines":11,"feather_armor_spines":18,
 "editable_source":str(source3d),"preview":str(preview),
 "reference_likeness_accepted":False,"original_canonical_character":False,
 "Unreal_Android_accepted":False,"status":"EDITABLE_RIGGED_3D_ROUGH_CANDIDATE"}
report.write_text(json.dumps(r,indent=2))
print("NW_RAKABI_CREATED="+json.dumps(r))
