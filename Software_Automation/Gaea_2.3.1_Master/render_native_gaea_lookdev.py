"""Gaea-native erosion artistic material preview, entirely adjustable Blender 5.2.
Noncanonical QA scene. Proper physically shaded height+normal rock/soil,
regional masks, atmospheric sun, reflective lowland water.
"""
import bpy,pathlib,math,json,time,hashlib
from mathutils import Vector
start=time.perf_counter()
q=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
texture=pathlib.Path(r"D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_4.5.10\MATERIALS\PolyHaven_CC0_1k")
source=q/"NW_NativeEroded_4TileLandscape.blend"
out=q/"NW_GaeaNative_Erosion_ArtLookdev_V2.blend"
preview=q/"NW_GaeaNative_Erosion_ArtLookdev_V2.png"
report=q/"NW_GaeaNative_ArtLookdev_Receipt_V2.json"
if any(p.exists() for p in (out,preview,report)):raise FileExistsError("Refuse to overwrite")
src_sha=hashlib.sha256(source.read_bytes()).hexdigest()
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
tiles=[x for x in bpy.data.objects if x.name.startswith("NW_TerrainTile_")]
assert len(tiles)==4
mat=bpy.data.materials.new("NW_GaeaNative_StoneSoil_RegionalPBR")
mat.use_nodes=True
nodes=mat.node_tree.nodes;links=mat.node_tree.links;nodes.clear()
def node(t,pos,label=None):
 n=nodes.new(t);n.location=pos
 if label:n.label=label
 return n
def connect(a,ao,b,bi):links.new(a.outputs[ao],b.inputs[bi])
geo=node("ShaderNodeNewGeometry",(-1100,220),"World-continuous terrain position and normal")
sep=node("ShaderNodeSeparateXYZ",(-900,330))
connect(geo,"Position",sep,"Vector")
height=node("ShaderNodeMapRange",(-700,380))
height.inputs["From Min"].default_value=55
height.inputs["From Max"].default_value=605
height.inputs["To Min"].default_value=0
height.inputs["To Max"].default_value=1
connect(sep,"Z",height,"Value")
altitude=node("ShaderNodeValToRGB",(-490,380),"Lowland→grass→upland→snow")
r=altitude.color_ramp;r.interpolation="EASE"
points=[(0.0,(.42,.47,.27,1)),(.18,(.29,.37,.20,1)),(.40,(.29,.35,.22,1)),(.62,(.44,.39,.31,1)),(.8,(.70,.68,.62,1)),(1,(.89,.9,.9,1))]
r.elements.remove(r.elements[1]);r.elements[0].position=points[0][0];r.elements[0].color=points[0][1]
for p,c in points[1:]:x=r.elements.new(p);x.color=c
connect(height,"Result",altitude,"Fac")
nsep=node("ShaderNodeSeparateXYZ",(-900,-75))
connect(geo,"Normal",nsep,"Vector")
steep=node("ShaderNodeMapRange",(-700,-90),"Exposed rock on terrain slopes")
steep.inputs["From Min"].default_value=.98
steep.inputs["From Max"].default_value=.44
steep.inputs["To Min"].default_value=0
steep.inputs["To Max"].default_value=1
connect(nsep,"Z",steep,"Value")
mapped=node("ShaderNodeVectorMath",(-920,-480),"World-space texture continuity")
mapped.operation="SCALE";mapped.inputs[3].default_value=.008
connect(geo,"Position",mapped,0)
def load_tex(name,pos,kind="sRGB"):
 im=bpy.data.images.load(str(texture/name),check_existing=True)
 im.colorspace_settings.name=kind
 im.pack()
 t=node("ShaderNodeTexImage",pos,name);t.image=im
 connect(mapped,"Vector",t,"Vector")
 return t
soil=load_tex("stony_dirt_path_Diffuse.jpg",(-520,-430))
rock=load_tex("rough_block_wall_Diffuse.jpg",(-520,-700))
rocknorm=load_tex("rough_block_wall_nor_gl.jpg",(-520,-1000),"Non-Color")
noise=node("ShaderNodeTexNoise",(-900,-800),"Broad natural color variation")
noise.inputs["Scale"].default_value=.0025
noise.inputs["Detail"].default_value=5
connect(geo,"Position",noise,"Vector")
var=node("ShaderNodeMapRange",(-710,-830))
var.inputs["From Min"].default_value=.15
var.inputs["From Max"].default_value=.85
var.inputs["To Min"].default_value=.78
var.inputs["To Max"].default_value=1.05
connect(noise,"Fac",var,"Value")
blend=node("ShaderNodeMixRGB",(-260,-200),"Physical rocky exposed slopes")
blend.blend_type="MIX"
connect(steep,"Result",blend,0)
connect(soil,"Color",blend,1)
connect(rock,"Color",blend,2)
altmix=node("ShaderNodeMixRGB",(-70,180),"Material color plus geography")
altmix.blend_type="MULTIPLY";altmix.inputs[0].default_value=.58
connect(blend,"Color",altmix,1)
connect(altitude,"Color",altmix,2)
pbr=node("ShaderNodeBsdfPrincipled",(300,160))
pbr.inputs["Roughness"].default_value=.83
pbr.inputs["Metallic"].default_value=0
connect(altmix,"Color",pbr,"Base Color")
normal=node("ShaderNodeNormalMap",(-130,-750))
normal.inputs["Strength"].default_value=.47
connect(rocknorm,"Color",normal,"Color")
bump=node("ShaderNodeBump",(70,-550))
bump.inputs["Strength"].default_value=.15
bump.inputs["Distance"].default_value=.028
connect(noise,"Fac",bump,"Height")
connect(normal,"Normal",bump,"Normal")
connect(bump,"Normal",pbr,"Normal")
outnode=node("ShaderNodeOutputMaterial",(600,180))
connect(pbr,"BSDF",outnode,"Surface")
for tile in tiles:
 tile.data.materials.clear()
 tile.data.materials.append(mat)
 tile["nw_provenance"]="Gaea2.3 native erosion output"
# Water plateau covers only physically low-altitude basins.
water=bpy.data.materials.new("NW_SilverRiver_LowlandWater")
water.use_nodes=True
p=water.node_tree.nodes.get("Principled BSDF")
p.inputs["Base Color"].default_value=(.06,.16,.19,1)
p.inputs["Metallic"].default_value=.06
p.inputs["Roughness"].default_value=.125
p.inputs["Transmission Weight"].default_value=.24
bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,78))
w=bpy.context.object;w.name="NW_LowlandWaterSurface_Optional"
w.scale=(3000,3000,1)
w.data.materials.append(water)
w["nw_gate02_parameter"]="water_level_m_78_adjustable"
# Physically simple warm sun and soft studio world light; no camera travel.
sun_data=bpy.data.lights.new("NW_AerialWarmSun","SUN")
sun=bpy.data.objects.new("NW_AerialWarmSun",sun_data)
bpy.context.scene.collection.objects.link(sun)
sun.rotation_euler=(math.radians(45),math.radians(-17),math.radians(-50))
sun_data.energy=2.5
sun_data.angle=math.radians(4)
world=bpy.context.scene.world
if world is None:world=bpy.data.worlds.new("NW_AtmosphericWorld");bpy.context.scene.world=world
world.use_nodes=True
world.node_tree.nodes["Background"].inputs["Color"].default_value=(.47,.58,.71,1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value=.48
camera_data=bpy.data.cameras.new("NW_CinematicTerrainReviewCamera")
camera=bpy.data.objects.new("NW_CinematicTerrainReviewCamera",camera_data)
bpy.context.scene.collection.objects.link(camera)
camera.location=(5300,-6100,3900)
target=Vector((0,0,250))
camera.rotation_euler=(target-camera.location).to_track_quat("-Z","Y").to_euler()
camera_data.type="ORTHO"
camera_data.ortho_scale=8500
camera_data.clip_end=30000
bpy.context.scene.camera=camera
sc=bpy.context.scene
sc.render.engine="CYCLES"
sc.cycles.device="CPU"
sc.cycles.samples=12
sc.cycles.use_denoising=True
sc.render.threads_mode="FIXED"
sc.render.threads=2
sc.render.resolution_x=860
sc.render.resolution_y=640
sc.render.resolution_percentage=100
sc.render.image_settings.file_format="PNG"
sc.render.film_transparent=False
sc.view_settings.view_transform="AgX"
sc.camera.data.lens=35
sc["nw_acceptance"]="Technical/visual automation preview only - not canonical Gate02 scenery"
sc["nw_native_height_sha256"]=hashlib.sha256((q/"exports"/"NW_Height.png").read_bytes()).hexdigest()
# Keep adjustable source separate from original; cycles preview is rendered only once.
sc.frame_set(1)
sc.render.filepath=str(preview)
bpy.ops.wm.save_as_mainfile(filepath=str(out),compress=True)
bpy.ops.render.render(write_still=True)
assert hashlib.sha256(source.read_bytes()).hexdigest()==src_sha
receipt_data={"status":"RENDERED_GAEA_NATIVE_PBR_LOOKDEV",
 "source_unchanged":True,"editable_blend":str(out),"preview_png":str(preview),
 "terrain_tiles":len(tiles),"pbr_image_maps":3,"water_surface":True,
 "camera_static":True,"render_size":[860,640],
 "elapsed_s":round(time.perf_counter()-start,2),"game_art_accepted":False}
report.write_text(json.dumps(receipt_data,indent=2))
print("NW_NATIVE_GAEA_PBR_VISUAL="+json.dumps(receipt_data),flush=True)
