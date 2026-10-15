"""NO WORLD Gate01 — noncanonical Anti-Spiral visual fixture from actual KK rig.
Not a 2D likeness claim. Preserves 121-bone KK rig and hundreds of source morphs.
Creates procedural 3D galaxy body shader, native animated-compatible hair wisps,
seven spiral insignias, two spiral eyes and orbital props, plus editable .blend.
All derivatives kept private; source SHA unchanged.
"""
import bpy,math,random,json,pathlib,hashlib,sys
from mathutils import Vector
src=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261015\NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend")
qa=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261015\AntiSpiral_Feminine_Private")
qa.mkdir(parents=True,exist_ok=True)
blend=qa/"AntiSpiral_Feminine_Cosmic_EDITABLE_PRIVATE.blend"
report=qa/"AntiSpiral_Actual3D_Validation.json"
views=[qa/f"AntiSpiral_3D_{v}.png" for v in ("front","side","rear")]
if any(x.exists() for x in [blend,report,*views]):raise FileExistsError("Non-destructive output already exists")
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
before=sha(src)
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
rig=bpy.data.objects.get("NW_KK_INTENDED_SKELETON_121")
assert rig and len(rig.data.bones)==121
mesh=bpy.data.objects["NW_SHARD03_model"]
hair=bpy.data.objects["NW_SHARD04_Hair Outfit 00 ChikaHaruno.004"]
for o in bpy.data.objects:
 if o.type=="MESH" and o.name not in (mesh.name,hair.name):o.hide_render=True
s=bpy.context.scene
# Build starfield material entirely from real deformation-safe Generated coordinates.
def nt_material(name):
 mat=bpy.data.materials.new(name)
 mat.use_nodes=True;n=mat.node_tree.nodes;n.clear();lk=mat.node_tree.links
 out=n.new("ShaderNodeOutputMaterial");out.location=(720,0)
 shader=n.new("ShaderNodeBsdfPrincipled");shader.location=(440,0)
 lk.new(shader.outputs["BSDF"],out.inputs["Surface"])
 return mat,n,lk,shader
m,n,lk,p=nt_material("NW_AntiSpiral_3D_NebulaBody_Editable")
coord=n.new("ShaderNodeTexCoord");coord.location=(-900,80)
noise=n.new("ShaderNodeTexNoise");noise.location=(-690,60)
noise.noise_dimensions="3D";noise.inputs["Scale"].default_value=4.2
noise.inputs["Detail"].default_value=7;noise.inputs["Roughness"].default_value=.72
lk.new(coord.outputs["Generated"],noise.inputs["Vector"])
ramp=n.new("ShaderNodeValToRGB");ramp.location=(-440,110)
colors=[(.18,(.004,.005,.018,1)),(.44,(.018,.012,.055,1)),(.63,(.105,.042,.27,1)),(.80,(.20,.14,.42,1))]
ramp.color_ramp.elements.remove(ramp.color_ramp.elements[1])
for i,(pos,color) in enumerate(colors):
 elem=ramp.color_ramp.elements[0] if i==0 else ramp.color_ramp.elements.new(pos)
 elem.position=pos;elem.color=color
lk.new(noise.outputs["Fac"],ramp.inputs["Fac"])
vor=n.new("ShaderNodeTexVoronoi");vor.location=(-680,-170)
vor.feature="F1";vor.inputs["Scale"].default_value=180
lk.new(coord.outputs["Generated"],vor.inputs["Vector"])
star=n.new("ShaderNodeMath");star.operation="LESS_THAN";star.inputs[1].default_value=.042
lk.new(vor.outputs["Distance"],star.inputs[0])
star2=n.new("ShaderNodeMath");star2.operation="MULTIPLY";star2.inputs[1].default_value=3.4
lk.new(star.outputs[0],star2.inputs[0])
emission_mix=n.new("ShaderNodeMixRGB");emission_mix.blend_type="ADD";emission_mix.inputs["Fac"].default_value=1
emission_mix.inputs["Color2"].default_value=(.36,.29,.65,1)
lk.new(ramp.outputs["Color"],emission_mix.inputs["Color1"])
# Sparse star emission + low nebula base.
coloradd=n.new("ShaderNodeMixRGB");coloradd.blend_type="ADD";coloradd.inputs["Fac"].default_value=1
lk.new(ramp.outputs["Color"],coloradd.inputs["Color1"])
starcolor=n.new("ShaderNodeVectorMath");starcolor.operation="SCALE"
starcolor.inputs[0].default_value=(.75,.80,1)
lk.new(star2.outputs[0],starcolor.inputs[3])
lk.new(starcolor.outputs["Vector"],coloradd.inputs["Color2"])
lk.new(coloradd.outputs["Color"],p.inputs["Base Color"])
lk.new(coloradd.outputs["Color"],p.inputs["Emission Color"])
p.inputs["Emission Strength"].default_value=.7
p.inputs["Metallic"].default_value=.08
p.inputs["Roughness"].default_value=.37
# Use the same three-dimensional body shader on visible skin and face.
skin_indices=[]
for i,original in enumerate(mesh.data.materials):
 if original and any(t in original.name.lower() for t in ("cf_m_body","cf_m_face_00")):
  mesh.data.materials[i]=m;skin_indices.append(i)
assert len(skin_indices)>=2
# Create desaturated lilac true 3D hair base.
hm,nn,ll,hp=nt_material("NW_AntiSpiral_LavenderHair")
hp.inputs["Base Color"].default_value=(.65,.60,.85,1)
hp.inputs["Roughness"].default_value=.45
hp.inputs["Metallic"].default_value=.05
hp.inputs["Emission Color"].default_value=(.20,.16,.38,1)
hp.inputs["Emission Strength"].default_value=.14
for i in range(len(hair.data.materials)):hair.data.materials[i]=hm
# Fine 3D hair ribbons, geometry (curves) not image plane.
rng=random.Random(22618)
curve=bpy.data.curves.new("NW_TransparentLongHair_CustomStrands","CURVE")
curve.dimensions="3D";curve.resolution_u=8;curve.bevel_depth=.0013;curve.bevel_resolution=2
strand_count=150
for i in range(strand_count):
 angle=math.pi*2*i/strand_count +rng.uniform(-.018,.018)
 side=math.cos(angle)
 front=math.sin(angle)
 length=rng.uniform(.45,1.35)
 start=Vector((.085*side,.040+.070*front,1.53))
 points=10
 sp=curve.splines.new("BEZIER");sp.bezier_points.add(points-1)
 for j,bp in enumerate(sp.bezier_points):
  t=j/(points-1)
  sweep=(.08+.15*t*t)*side + .03*math.sin(t*10+i*.4)*t
  bp.co=(start.x+sweep*t,start.y+.08*front*t+.03*math.sin(t*6+i),start.z-length*t)
  bp.handle_left_type="AUTO";bp.handle_right_type="AUTO"
  bp.radius=max(.02,1-t**3)
obj=bpy.data.objects.new("NW_LongHair_150DeformableCurves",curve)
s.collection.objects.link(obj)
curve.materials.append(hm)
# Place controlled bright spiral strokes as native 3D geometry.
white=bpy.data.materials.new("NW_Spiral_LuminousWhite")
white.use_nodes=True;bs=white.node_tree.nodes.get("Principled BSDF")
bs.inputs["Base Color"].default_value=(.73,.74,1,1)
bs.inputs["Emission Color"].default_value=(.77,.75,1,1)
bs.inputs["Emission Strength"].default_value=8
bs.inputs["Roughness"].default_value=.24
def spiral(name,center,radius,turns=2.4,orientation="XZ",count=80,width=.0028):
 cu=bpy.data.curves.new(name+"_Shape","CURVE");cu.dimensions="3D";cu.bevel_depth=width;cu.bevel_resolution=2
 sp=cu.splines.new("POLY");sp.points.add(count-1)
 for k,v in enumerate(sp.points):
  f=k/(count-1);theta=2*math.pi*turns*f
  r=radius*(.03+.97*f)
  u,vv=r*math.cos(theta),r*math.sin(theta)
  if orientation=="XZ":point=(center[0]+u,center[1],center[2]+vv)
  elif orientation=="XY":point=(center[0]+u,center[1]+vv,center[2])
  else:point=(center[0],center[1]+u,center[2]+vv)
  v.co=(*point,1)
 ob=bpy.data.objects.new(name,cu);s.collection.objects.link(ob)
 cu.materials.append(white)
 return ob
# Front of head is -Y in KK assembled character; both iris spirals.
spiral("NW_EyeSpiral_Left",(.041,-.026,1.414),.021,3.4,width=.0014)
spiral("NW_EyeSpiral_Right",(-.041,-.026,1.414),.021,3.4,width=.0014)
for name,center,r,plane in [
 ("ShoulderL",(.105,-.028,1.23),.042,"XZ"),
 ("ShoulderR",(-.106,-.028,1.23),.041,"XZ"),
 ("HipL",(.14,-.09,.94),.045,"XZ"),
 ("HipR",(-.14,-.09,.94),.045,"XZ"),
 ("KneeL",(.11,-.035,.48),.036,"XZ"),
 ("KneeR",(-.11,-.035,.48),.036,"XZ"),
 ("Thorax",(0,-.095,1.2),.055,"XZ")]:
 spiral("NW_Glyph_"+name,center,r,width=.002)
# Orbiting geometric universe props as individual editable mesh objects.
orbmat=bpy.data.materials.new("NW_BlackMoonShader")
orbmat.use_nodes=True;mp=orbmat.node_tree.nodes.get("Principled BSDF")
mp.inputs["Base Color"].default_value=(.026,.018,.065,1)
mp.inputs["Metallic"].default_value=.36;mp.inputs["Roughness"].default_value=.34
mp.inputs["Emission Color"].default_value=(.09,.055,.25,1)
mp.inputs["Emission Strength"].default_value=.35
for i,(px,py,pz,r) in enumerate([(.68,.38,1.64,.115),(-.62,.21,1.12,.065),(.46,-.08,.72,.044)]):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=28,ring_count=14,radius=r,location=(px,py,pz))
 moon=bpy.context.object;moon.name=f"NW_OrbitingVoidBody_{i:02d}"
 moon.data.materials.append(orbmat)
# Existing camera and area lighting preserved, force frontal presentation.
cam=s.camera
cam.location=(.05,-3.85,1.02)
cam.rotation_euler=(Vector((0,0,.90))-cam.location).to_track_quat("-Z","Y").to_euler()
s.render.engine="CYCLES"
s.cycles.samples=24;s.cycles.use_denoising=True
s.render.threads_mode="FIXED";s.render.threads=2
s.render.resolution_x=565;s.render.resolution_y=760
s.render.resolution_percentage=100
s.render.image_settings.file_format="PNG"
s.render.image_settings.color_mode="RGBA"
s.view_settings.view_transform="AgX"
positions=[(.0,-3.85,1.02),(-3.1,-2.2,1.10),(.0,3.85,1.03)]
for i,position in enumerate(positions):
 cam.location=position
 cam.rotation_euler=(Vector((0,0,.9))-cam.location).to_track_quat("-Z","Y").to_euler()
 s.render.filepath=str(views[i])
 bpy.ops.render.render(write_still=True)
cam.location=positions[0]
cam.rotation_euler=(Vector((0,0,.9))-cam.location).to_track_quat("-Z","Y").to_euler()
s["NW_GAME"]= "NoWorld: False Horizon"
s["NW_GATE"]="Gate01 private technical candidate"
s["NW_NOT_ACCEPTED"]="visual_2D_similarity_UNVERIFIED"
bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
assert sha(src)==before
data={"candidate":"Anti-Spiral feminine reconstruction",
 "source":str(src),"source_hash_unchanged":True,
 "blend":str(blend),"rig_bones":len(rig.data.bones),
 "original_face_morphs_preserved":len(mesh.data.shape_keys.key_blocks) if mesh.data.shape_keys else 0,
 "new_starfield_shader":m.name,"unique_glyphs":9,"3d_hair_splines":strand_count,
 "views":[str(x) for x in views],
 "visual_likeness_accepted":False,"native_kk_card_reimported":False,
 "unreal_import_accepted":False,
 "status":"EDITABLE_3D_TECHNICAL_CHARACTER_CANDIDATE"}
report.write_text(json.dumps(data,indent=2),encoding="utf-8")
print("NW_REAL_3D_CHARACTER="+json.dumps(data))
