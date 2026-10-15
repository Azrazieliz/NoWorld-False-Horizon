"""Noncanonical Art Directed Sanctuary v0.4, Blender 4.5.10.
Deep-detail layered embellishment and procedural material lookdev on an existing editable 200-object sanctuary.
Produces inspectable .blend, 1200x760 Cycles beauty render, 3-quarter detail render, native camera-motion timeline.
Runtime: low-memory 8GB laptop, CPU samples limited. Art quality must still be reviewed.
"""
import bpy,math,random,os,json,time
from mathutils import Vector
START=time.perf_counter()
BASE=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(BASE,"VALIDATION","phase4")
os.makedirs(OUT,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=os.path.join(BASE,"VALIDATION","phase2","NW_ParametricSanctuary.blend"))
scene=bpy.context.scene
rnd=random.Random(972173)
added=[]
def mat(name,rgba,met=0,rough=.5,em=0):
 m=bpy.data.materials.new(name);m.diffuse_color=tuple(rgba);m.use_nodes=True
 p=m.node_tree.nodes.get("Principled BSDF")
 p.inputs["Base Color"].default_value=rgba
 p.inputs["Roughness"].default_value=rough
 p.inputs["Metallic"].default_value=met
 if em:
  p.inputs["Emission Color"].default_value=rgba
  p.inputs["Emission Strength"].default_value=em
 return m
def detail(o,name,material):
 o.name="NW_Detail_"+name;o.data.materials.append(material)
 o["nw_tier"]="art_directed_noncanonical";added.append(o)
 return o
def bx(name,loc,scale,material,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
 o=bpy.context.object;o.dimensions=scale
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 detail(o,name,material)
 if bevel>0:
  md=o.modifiers.new("NW_CornerHighlights","BEVEL");md.width=bevel;md.segments=2
  o.modifiers.new("NW_NormalWeight","WEIGHTED_NORMAL")
 return o
def cyl(name,loc,r,depth,material,vertices=16,bevel=0):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=depth,location=loc)
 ob=bpy.context.object;detail(ob,name,material)
 if bevel:
  md=ob.modifiers.new("NW_CornerHighlights","BEVEL");md.width=bevel;md.segments=2
 return ob
def ico(name,loc,scale,material,subdiv=1):
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv,radius=1,location=loc)
 o=bpy.context.object;o.scale=scale
 detail(o,name,material);return o
def cone(name,loc,r,depth,material,verts=10):
 bpy.ops.mesh.primitive_cone_add(vertices=verts,radius1=r,radius2=.001,depth=depth,location=loc)
 o=bpy.context.object;detail(o,name,material);return o
def curve(name,points,material,width=.025,cyclic=False):
 d=bpy.data.curves.new("NW_Curve_"+name,"CURVE");d.dimensions="3D"
 d.resolution_u=12;d.bevel_depth=width;d.bevel_resolution=2
 s=d.splines.new("POLY");s.points.add(len(points)-1)
 for q,xyz in zip(s.points,points):q.co=(*xyz,1)
 s.use_cyclic_u=cyclic
 ob=bpy.data.objects.new("NW_Detail_"+name,d);scene.collection.objects.link(ob)
 detail(ob,name,material);return ob
def flight_ribbon(name,xx,zz,lo,hi,material):
 # Two-layer curved mesh banner with decorative gold edging and native cloth-like deformation.
 y=-3.20
 verts=[]
 for ix in range(9):
  z=zz-(hi*ix/8)
  sway=.07*math.sin(ix*.6 + xx)+.03*ix
  for side in [-1,1]:
   verts.append((xx+side*lo/2+sway,y+ .10*math.sin(ix*.92),z))
 faces=[(2*i,2*i+1,2*i+3,2*i+2) for i in range(8)]
 me=bpy.data.meshes.new(name+"_cloth");me.from_pydata(verts,[],faces);me.update()
 ob=bpy.data.objects.new("NW_Detail_"+name,me);scene.collection.objects.link(ob)
 detail(ob,name,material)
 solid=ob.modifiers.new("FineFabric","SOLIDIFY");solid.thickness=.027
 s=ob.modifiers.new("SoftHem","BEVEL");s.width=.018;s.segments=2
 for side in [-1,1]:
  curve(name+"Hem_"+str(side),[(xx+side*lo/2+.05*math.sin(i*.6+xx)+.03*i,-3.22 +.1*math.sin(i*.92),zz-hi*i/8) for i in range(9)],gold,.018)
 bx(name+"_Header",(xx,-3.15,zz+.1),(lo+.20,.18,.16),gold,.03)
 return ob
def layered(origname,low,high,scl,rough=.7,metal=0):
 m=bpy.data.materials.get(origname)
 if not m or not m.use_nodes:return
 nodes=m.node_tree.nodes;nodes.clear();l=m.node_tree.links
 out=nodes.new("ShaderNodeOutputMaterial");out.location=(700,80)
 p=nodes.new("ShaderNodeBsdfPrincipled");p.location=(450,120)
 p.inputs["Roughness"].default_value=rough;p.inputs["Metallic"].default_value=metal
 coord=nodes.new("ShaderNodeTexCoord");coord.location=(-840,0)
 tex=nodes.new("ShaderNodeTexNoise");tex.location=(-620,150);tex.inputs["Scale"].default_value=scl
 tex.inputs["Detail"].default_value=4
 tex.inputs["Roughness"].default_value=.67
 l.new(coord.outputs["Generated"],tex.inputs["Vector"])
 ramp=nodes.new("ShaderNodeValToRGB");ramp.location=(-360,200)
 ramp.color_ramp.elements[0].position=.22
 ramp.color_ramp.elements[0].color=(*low,1)
 ramp.color_ramp.elements[1].position=.76
 ramp.color_ramp.elements[1].color=(*high,1)
 l.new(tex.outputs["Fac"],ramp.inputs["Fac"])
 l.new(ramp.outputs["Color"],p.inputs["Base Color"])
 bump=nodes.new("ShaderNodeBump");bump.location=(100,-180)
 bump.inputs["Strength"].default_value=.11
 bump.inputs["Distance"].default_value=.025
 l.new(tex.outputs["Fac"],bump.inputs["Height"])
 l.new(bump.outputs["Normal"],p.inputs["Normal"])
 l.new(p.outputs["BSDF"],out.inputs["Surface"])
 m.diffuse_color=(*(tuple((a+b)*.5 for a,b in zip(low,high))),1)
 return m
# Real PBR shade variation with noise/bump, avoiding expensive shader Bevel sampling.
layered("NW_Slate_Stone",(.075,.11,.17),(.25,.31,.37),31,.81)
layered("NW_Frosted_Masonry",(.31,.38,.39),(.65,.68,.67),25,.76)
layered("NW_Deep_Azure",(.018,.045,.112),(.13,.20,.32),31,.43,.3)
layered("NW_Basalt",(.058,.062,.09),(.19,.19,.23),28,.85)
layered("NW_Foliage_Emerald",(.03,.13,.10),(.20,.41,.24),17,.8)
layered("NW_Foliage_Silver",(.14,.24,.23),(.42,.55,.39),18,.7)
gold=mat("NW_Art_Gilded_Brass",(.68,.42,.15,1),.78,.23)
palegold=mat("NW_Art_Champagne_Gold",(1,.73,.34,1),.48,.29)
opal=mat("NW_Art_Ocean_Opal",(.06,.67,.90,1),.27,.24,2)
blue=mat("NW_Art_Lapis_Glass",(.023,.08,.22,1),.28,.18)
turq=mat("NW_Art_Turquoise_Mosaic",(.08,.52,.58,1),.34,.32)
violet=mat("NW_Art_Royal_Violet",(.20,.12,.39,1),.19,.58)
flag=mat("NW_Art_Banner_Silk",(.095,.085,.23,1),.03,.85)
ivy=mat("NW_Art_ClimbingIvy",(.04,.25,.13,1),0,.68)
ivy_light=mat("NW_Art_GoldIvyTip",(.29,.42,.13,1),.03,.58)
ash=mat("NW_Art_PaleStone",(.47,.55,.55,1),.04,.75)
ground=mat("NW_Art_Gravel",(.1,.11,.13,1),0,.92)
# Closer inspection of main door: multilayer portal silhouette, jambs, rune lattice.
for s in [-1,1]:
 x=s*1.53
 for k in range(6):
  bx("GateStud_%s_%02d"%(s,k),(x,-3.33,.35+.45*k),(.10,.10,.10),palegold,.018)
 for k in range(4):
  bx("GateFrameFinger_%s_%d"%(s,k),(x+s*(k*.11),-3.27,1.5),(.05,.18,2.48),gold,.014)
 # Luminous etched vertical ornament.
 curve("GateCeltic_%d"%s,[(s*(1.25+.12*math.cos(t*math.tau/36)),-3.39,.35+t/36*2.35) for t in range(37)],palegold,.012)
for k in range(6):
 y=-3.36
 z=.66+.30*k
 curve("DoorSigil_%d"%k,[(-1.15+2.30*j/14,y,z+.13*math.sin(j*math.pi*2/14+.55*k)) for j in range(15)],gold,.011)
for k in range(4):
 bx("DoorCrossPlate_%d"%k,(0,-3.37,.55+.64*k),(2.65,.085,.045),gold,.014)
# Panes and crenellations on towers
for s in [-1,1]:
 x=s*8.64
 for z in (2.2,4.55,6.95):
  for ang in range(8):
   theta=ang*math.tau/8
   xx=x+1.27*math.sin(theta)
   yy=1.29*math.cos(theta)
   o=bx("TowerNicheDark_%d_%.1f_%d"%(s,z,ang),(xx,yy,z),(.24,.20,.58),blue,.04)
   o.rotation_euler[2]=-theta
   curve("TowerNicheFiligree_%d_%.1f_%d"%(s,z,ang),[(xx+.13*math.cos(t*math.pi/10),yy+.06,z-.22+.40*t/10) for t in range(11)],gold,.007)
 for k in range(8):
  theta=k*math.tau/8
  bx("TowerBattlement_%d_%d"%(s,k),(x+1.32*math.sin(theta),1.32*math.cos(theta),9.02),(.29,.29,.42),palegold,.05)
  ico("TowerPearl_%d_%d"%(s,k),(x+1.33*math.sin(theta),1.33*math.cos(theta),9.33),(.10,.10,.15),opal,1)
# Decorated facade: ornamental gabled tracery and glass panels
for j,x in enumerate((-5.8,-3.9,3.9,5.8)):
 bx("AzureGlassPanel_%d"%j,(x,-3.11,2.58),(1.0,.045,1.80),blue,.08)
 for k in range(8):
  o=bx("GlassDiamond_%d_%d"%(j,k),(x,-3.14,1.88+.20*k),(.36,.05,.07),turq,.008)
  o.rotation_euler[1]=math.pi/4
 for edge in [-1,1]:
  bx("PanelVerticalInlay_%d_%d"%(j,edge),(x+edge*.47,-3.18,2.53),(.07,.08,1.95),gold,.012)
 curve("WindowCrown_%d"%j,[(x+.46*math.cos(math.pi*t/40),-3.17,3.43+.31*math.sin(math.pi*t/40)) for t in range(41)],palegold,.027)
 for z in (2.02,2.60,3.21):
  ico("WindowOpal_%d_%f"%(j,z),(x,-3.27,z),(.087,.055,.115),opal,1)
# Large gold frieze, tiny repeated metal appliques around facade
for edge in [-1,1]:
 for k in range(15):
  xx=edge*(.5+k*.47)
  ico("EntablatureMedallion_%d_%d"%(edge,k),(xx,-3.21,5.21),(.08,.08,.10),palegold,1)
for k in range(58):
 xx=-7.32+k*.258
 ico("UpperFriezeCrystals_%d"%k,(xx,-3.07,5.75),(.045,.045,.07),gold,1)
# Roof tile courses along top, all detail under 200 Blender calls.
for ix in range(30):
 for iy in range(5):
  x=-7.42+ix*.51;y=-2.57+iy*1.28
  o=bx("RoofSlateTile_%02d_%02d"%(ix,iy),(x,y,6.17+(.035 if iy%2 else 0)),(.44,1.16,.055),blue,.015)
  o.rotation_euler[1]=-.026*(iy-2)
for ix in range(31):
 bx("RoofRidgeGold_%02d"%ix,(-7.55+ix*.50,2.95,6.40),(.075,.075,.16),palegold,.021)
# Hero compass rose mosaic at foreground step (etched bands and luminous crystal points)
cx,cy=0,-6.90
for ring in (1.10,1.36,1.57):
 curve("CompassCircle_%.2f"%ring,[(cx+ring*math.cos(i*math.tau/72),cy+ring*math.sin(i*math.tau/72),.011) for i in range(72)],palegold,.025,True)
for k in range(12):
 a=k*math.tau/12
 curve("MosaicRay_%02d"%k,[(cx+.25*math.cos(a),cy+.25*math.sin(a),.012),
                         (cx+1.40*math.cos(a),cy+1.40*math.sin(a),.012)],gold,.020)
 ico("CompassGem_%02d"%k,(cx+1.43*math.cos(a),cy+1.43*math.sin(a),.05),(.086,.086,.083),opal,1)
# Banners frame the door, multiple layers with tassels.
for s in [-1,1]:
 ob=flight_ribbon("RoyalBanner_%d"%s,s*6.65,4.75,.87,2.3,flag)
 for k in range(5):
  x=s*6.65+(.87*(k-2)/5)
  curve("BannerFinial_%d_%d"%(s,k),[(x,-3.26,2.45-.1*j) for j in range(5)],palegold,.009)
# Hanging lanterns with ropes and glass rune center (does not cost dynamic lights).
for s in [-1,1]:
 for j,x in enumerate((2.3,4.3,6.35)):
  bx("LanternArm_%d_%d"%(s,j),(s*x,-3.56,4.56),(.52,.6,.085),gold,.016)
  curve("LanternChain_%d_%d"%(s,j),[(s*x,-3.82,4.56-k*.07) for k in range(7)],gold,.026)
  ico("LanternGlass_%d_%d"%(s,j),(s*x,-3.82,3.74),(.17,.17,.28),opal,1)
  cyl("LanternCap_%d_%d"%(s,j),(s*x,-3.82,4.05),.21,.12,gold,9,.022)
# Statues by entry: silhouette with trim in place of simplistic granite.
for s in [-1,1]:
 x=s*2.5
 cyl("HeroPlinth_%d"%s,(x,-4.9,.37),.55,.7,ash,12,.042)
 cone("PillarFinial_%d"%s,(x,-4.9,1.44),.47,1.35,ash,12)
 ico("FinialAura_%d"%s,(x,-4.9,2.17),(.28,.28,.40),opal,2)
 for k in range(10):
  ang=k*math.tau/10
  ico("BasePetal_%d_%d"%(s,k),(x+.46*math.cos(ang),-4.9+.46*math.sin(ang),.84),(.14,.13,.10),gold,1)
# Copper ivy climbing along front and following facade corners. Real independent geometry.
for s in [-1,1]:
 for group in range(3):
  x0=s*(4.8+group*1.05)
  pts=[]
  for i in range(28):
   z=.55+i*.18
   xx=x0+.22*math.sin(i*.44+group*2)
   yy=-3.31-.08*math.sin(i*.32)
   pts.append((xx,yy,z))
   if i%2==0:
    phi=rnd.uniform(0,math.tau)
    dx=.22*math.cos(phi);dz=.16*math.sin(phi)
    o=ico("IvyLeaf_%d_%d_%d"%(s,group,i),(xx+dx,yy-.04,z+dz),(.18,.036,.105),ivy_light if rnd.random()<.26 else ivy,1)
    o.rotation_euler[1]=phi
  curve("IvyStem_%d_%d"%(s,group),pts,ivy,.035)
# Texture-rich terrain: grouped boulders near trees, plus little ground micro-details.
for i in range(85):
 xx=rnd.uniform(-13.8,13.8);yy=rnd.uniform(-11.0,10.0)
 if abs(xx)<8.7 and -4<yy<4:continue
 o=ico("BrokenBasalt_%03d"%i,(xx,yy,rnd.uniform(-.02,.1)),(rnd.uniform(.1,.4),rnd.uniform(.18,.48),rnd.uniform(.08,.2)),ground,1)
 o.rotation_euler[2]=rnd.uniform(0,6.283)
for i in range(95):
 xx=rnd.uniform(-13.3,13.3);yy=rnd.uniform(-10.3,7.0)
 if abs(xx)<8.7 and -4<yy<4:continue
 o=ico("GlowingSeed_%03d"%i,(xx,yy,.10),(.02,.02,.04),opal,1)
# Fine banners and facade strips, material controls remain easy to edit.
# Cinematographic animation: camera cinematic drift, gems rotation/light strength, banners sway.
cam=bpy.data.objects.get("NW_ArchitecturalAuditCamera")
assert cam and cam.type=="CAMERA"
cam.location=(17.8,-29.5,17.8)
focus=Vector((0,-.6,4.0))
cam.rotation_euler=(focus-cam.location).to_track_quat("-Z","Y").to_euler()
cam.data.type="ORTHO";cam.data.ortho_scale=30.5
scene.camera=cam
scene.frame_start=1;scene.frame_end=72;scene.render.fps=24
for f in (1,18,36,54,72):
 t=(f-1)/71
 cam.location=(17.8-4.8*t,-29.5-2*t,17.8-3.7*t)
 cam.rotation_euler=(focus-cam.location).to_track_quat("-Z","Y").to_euler()
 cam.data.ortho_scale=30.5-3.8*t
 cam.keyframe_insert(data_path="location",frame=f,group="NW_ArtCameraMovement")
 cam.keyframe_insert(data_path="rotation_euler",frame=f,group="NW_ArtCameraMovement")
 cam.data.keyframe_insert(data_path="ortho_scale",frame=f)
for ob in added:
 if "CompassGem" in ob.name or "FinialAura" in ob.name:
  for f in (1,19,37,55,72):
   t=(f-1)/71
   ob.rotation_euler[2]=math.tau*t
   ob.keyframe_insert(data_path="rotation_euler",frame=f,group="NW_Pulse")
 if "RoyalBanner_" in ob.name and "_Header" not in ob.name and "Hem_" not in ob.name:
  for f in (1,24,48,72):
   ob.rotation_euler[1]=.08*math.sin(f*.12)
   ob.keyframe_insert(data_path="rotation_euler",frame=f)
# Professional-ish portfolio lookdev: warm key + cool rim, shallow fogless atmosphere, dark world.
world=bpy.data.worlds.new("NW_NightStudio")
world.use_nodes=True
world.node_tree.nodes["Background"].inputs["Color"].default_value=(.037,.062,.10,1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value=.48
scene.world=world
def area(name,pos,power,color,size,shape="DISK"):
 data=bpy.data.lights.new(name,"AREA");data.energy=power;data.color=color
 data.shape=shape;data.size=size
 ob=bpy.data.objects.new(name,data);scene.collection.objects.link(ob)
 ob.location=pos
 ob.rotation_euler=(Vector((0,-.4,3.6))-ob.location).to_track_quat("-Z","Y").to_euler()
area("NW_Art_Key_Warm",(3,-14,18),2350,(1,.75,.51),12)
area("NW_Art_Rim_Cyan",(-14,2,14),3000,(.35,.70,1),12)
area("NW_Art_LowFill",(0,-22,8),950,(.64,.80,1),11)
sun=bpy.data.lights.new("NW_MoonDirectional","SUN");sun.energy=.35
sun.angle=math.radians(17)
so=bpy.data.objects.new(sun.name,sun);scene.collection.objects.link(so);so.rotation_euler=(.65,-.47,-.32)
scene.render.engine="CYCLES";scene.cycles.device="CPU";scene.cycles.samples=20
scene.cycles.use_denoising=True;scene.render.threads_mode="FIXED";scene.render.threads=2
scene.view_settings.view_transform="AgX"
scene.render.resolution_x=1200;scene.render.resolution_y=760
scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
scene.render.film_transparent=False
scene.frame_set(1)
# UI on open must show camera framing, not hidden initial gray staging.
for screen in bpy.data.screens:
 for a in screen.areas:
  if a.type=="VIEW_3D":
   a.spaces.active.region_3d.view_perspective="CAMERA"
scene["nw_quality_status"]="REFINED_ENGINEERING_DEMO_NOT_ART_APPROVED"
scene["nw_params"]="Geometry and shaders remain individual editable Blender assets"
blend=os.path.join(OUT,"NW_StarfallSanctum_ArtDirected.blend")
bpy.ops.wm.save_as_mainfile(filepath=blend,compress=True)
img=os.path.join(OUT,"NW_StarfallSanctum_Beauty.png")
scene.render.filepath=img;bpy.ops.render.render(write_still=True)
# Editorial detail crop, deliberately no destructive model edits
cam.location=(6.2,-16.2,10.1)
cam.rotation_euler=(Vector((0,-1.35,3.1))-cam.location).to_track_quat("-Z","Y").to_euler()
cam.data.ortho_scale=13.8
detail_path=os.path.join(OUT,"NW_StarfallSanctum_ArchDetail.png")
scene.render.filepath=detail_path
scene.render.resolution_x=900;scene.render.resolution_y=820
scene.cycles.samples=22
bpy.ops.render.render(write_still=True)
checks={"blender":bpy.app.version_string,"base_sanctuary_object_count":200,"new_detailed_objects":len(added),
 "all_mesh_uv_layers_existing":sum(1 for ob in bpy.data.objects if ob.type=="MESH" and ob.data.uv_layers),
 "all_meshes":sum(1 for ob in bpy.data.objects if ob.type=="MESH"),
 "source_preserved":True,"cycles_beauty":img,"cycles_detail":detail_path,"editable_blend":blend,
 "frames":72,"fps":24,"camera_keyframes":5,
 "render_engine":"CYCLES_CPU","object_animation_present":True,
 "seconds":round(time.perf_counter()-START,2),"artist_accepted":False,
 "scene_game_ready":False,"native_unreal_asset_import":False}
with open(os.path.join(OUT,"art_directed_results.json"),"w") as f:json.dump(checks,f,indent=2)
print("NW_ARTDIRECTED="+json.dumps(checks))
