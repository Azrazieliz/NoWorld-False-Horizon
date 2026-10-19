"""Gate09 initial editable 16 m Sovereign Eclipse rose architecture.
Author original spatial geometry in Blender; NOT functional Domain or completed Niagara.
"""
import bpy,math,pathlib,hashlib,json,random
from mathutils import Vector
random.seed(4919)
ROOT=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
SRC=ROOT/'G09_SovereignEclipse_Architectural_SOURCE_v6.blend'
GLB=ROOT/'G09_SovereignEclipse_Architectural_INTERCHANGE_v6.glb'
REC=ROOT/'G09_SovereignEclipse_Architectural_PREVIEW_v6.png'
MAN=ROOT/'G09_SovereignEclipse_Architectural_MANIFEST_v6.json'
for f in (SRC,GLB,REC,MAN):
 if f.exists():raise FileExistsError('Immutable VFX output exists '+str(f))
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
def mat(name,color,metal=.15,rough=.38,trans=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1.0-trans);m.use_nodes=True
 n=m.node_tree.nodes.get('Principled BSDF');n.inputs['Base Color'].default_value=(*color,1)
 n.inputs['Metallic'].default_value=metal;n.inputs['Roughness'].default_value=rough
 n.inputs['Alpha'].default_value=1-trans
 n.inputs['Emission Color'].default_value=(*color,1)
 n.inputs['Emission Strength'].default_value=.13 if trans==0 else .36
 if trans:m.surface_render_method='DITHERED'
 return m
gold=mat('SovereignDay_IvoryGold',(.90,.74,.40),.56,.24)
ivory=mat('SovereignDay_TranslucentPetals',(.92,.87,.68),.14,.39,.22)
rose=mat('SovereignDay_ScarletRoses',(.55,.06,.15),.27,.32)
burg=mat('SovereignNight_DeepBurgundy',(.22,.025,.065),.46,.28)
violet=mat('SovereignNight_VioletPetals',(.19,.10,.32),.24,.34,.22)
black=mat('SovereignNight_BloodInkThorns',(.055,.025,.085),.62,.22)
seam=mat('SovereignBoundary_GoldVioletSeam',(.85,.68,.79),.28,.12)
metal=mat('SovereignSteel_RimAccent',(.66,.69,.75),.89,.22)
def meshobj(name,verts,faces,material):
 me=bpy.data.meshes.new(name+'_mesh');me.from_pydata(verts,[],faces);me.update()
 o=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(o);me.materials.append(material)
 return o
def limit(p):
 x,y,z=p;r=math.hypot(x,y)
 if r>15.97:
  x*=15.97/r;y*=15.97/r
 return (x,y,z)
def ribbon(name,points,width,material):
 vs=[];fs=[]
 for i,point in enumerate(points):
  p=Vector(point)
  t=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(i-1,0)])).normalized()
  n=t.cross(Vector((0,0,1)))
  if n.length<.02:n=Vector((1,0,0))
  n.normalize()
  w=width*(0.17+0.83*math.sin(math.pi*(i+.20)/(len(points)+.4))**.7)
  vs.extend([limit(p-n*w/2),limit(p+n*w/2)])
 for i in range(len(points)-1):fs.append((2*i,2*i+1,2*i+3,2*i+2))
 return meshobj(name,vs,fs,material)
def stem(name,points,rad,material,segments=9):
 verts=[];faces=[];ups=Vector((0,0,1))
 for i,point in enumerate(points):
  p=Vector(point);tan=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(i-1,0)])).normalized()
  n=tan.cross(ups)
  if n.length<.1:n=tan.cross(Vector((0,1,0)))
  n.normalize();b=tan.cross(n).normalized()
  r=rad*(1-.70*i/max(1,len(points)-1))
  for j in range(segments):
   a=math.tau*j/segments
   verts.append(limit(p+r*(math.cos(a)*n+math.sin(a)*b)))
 for i in range(len(points)-1):
  for j in range(segments):
   x=i*segments+j;y=i*segments+(j+1)%segments
   faces.append((x,y,y+segments,x+segments))
 faces.append(tuple(reversed(range(segments))))
 return meshobj(name,verts,faces,material)
def petal(name,c,theta,extent,h,material,fold=1):
 # Authored curved petals, with folded tip and raised midrib, not flat sectors.
 center=Vector(c);base=Vector((math.cos(theta),math.sin(theta),0))
 side=Vector((-math.sin(theta),math.cos(theta),0));up=Vector((0,0,1))
 verts=[];faces=[];nu=13;nv=9
 for i in range(nu):
  t=i/(nu-1)
  l=extent*(.06+.94*t)
  spread=(math.sin(math.pi*t)**.65)*extent*.42
  for j in range(nv):
   v=2*j/(nv-1)-1
   p=center+base*l+side*(v*spread)
   p+=up*(h*(math.sin(math.pi*t)*.8+t*.18)+0.16*fold*(1-v*v)*math.sin(math.pi*t))
   verts.append(limit(p))
 for i in range(nu-1):
  for j in range(nv-1):
   k=i*nv+j;faces.append((k,k+nv,k+nv+1,k+1))
 return meshobj(name,verts,faces,material)
objects=[]
# Twelve 3D rose ribs from real emergence points, subdivided for local Authority collision.
for i in range(12):
 theta=math.tau*i/12
 day=(math.cos(theta)<0)
 ribmat=gold if day else burg;leafmat=ivory if day else violet
 pts=[]
 for k in range(31):
  t=k/30
  r=9.25+5.72*t+.24*math.sin(math.pi*t)*math.cos(i*2.65)
  a=theta+.035*math.sin(t*math.pi*2+i*.4)
  z=8.3*math.sin(math.pi*t/2)**1.25 +.18*math.sin(i*.6)
  pts.append((r*math.cos(a),r*math.sin(a),max(.03,z)))
 objects.append(stem('Rib_%02d_%s'%(i,'Day' if day else 'Night'),pts,.145,ribmat))
 # Three articulating botanical offshoots, each its own editable mesh.
 for j,u in enumerate((.33,.53,.74)):
  pos=Vector(pts[round(30*u)]);angle=theta+(-1 if j%2 else 1)*(.33+.07*j)
  arm=[]
  for k in range(14):
   t=k/13
   r=.9*t+0.28*t*t
   arm.append(tuple(pos+Vector((r*math.cos(angle),r*math.sin(angle),.9*math.sin(math.pi*t/2)))))
  objects.append(stem('Stem_%02d_%02d'%(i,j),arm,.06,black if not day else gold,7))
  # 2 botanical leaf pairs per offshoot; shaped and folded
  for q in range(2):
   x=Vector(arm[5+q*4]);objects.append(petal('Petal_%02d_%02d_%d'%(i,j,q),x,angle+(-1 if q else 1)*.62,1.04,.37,leafmat,1))
 # One distinctive branching crown per petal rib
 crown=Vector(pts[29])
 for j in range(4):
  objects.append(petal('Crown_%02d_%02d'%(i,j),crown,theta+(.52*j-.78),1.85,.95,leafmat))
 # Thorns are separate exact-profile, editable tipped stems
 if not day:
  for j in (7,13,18,23):
   base=Vector(pts[j])
   tip=base+Vector((.33*math.cos(theta+.6),.33*math.sin(theta+.6),.53))
   objects.append(stem('BloodInkThorn_%02d_%02d'%(i,j),[base,base+(tip-base)*.55,tip],.09,black,7))
# Two layered circular botanical crown rings, not the physical world boundary.
for radius,height,label in ((8.8,.09,'Inner'),(15.6,.11,'Outer')):
 for i in range(12):
  day=math.cos(math.tau*(i+.5)/12)<0
  pts=[(radius*math.cos(math.tau*(i+t/20)/12),
        radius*math.sin(math.tau*(i+t/20)/12),height+.05*math.sin(math.tau*t/20)) for t in range(21)]
  objects.append(ribbon('RoseRing_%s_%02d'%(label,i),pts,.18,metal if label=='Outer' else (gold if day else burg)))
# Central rose for continuity; offset petals create true layered blossom.
for layer in range(4):
 for i in range(11):
  a=math.tau*(i/11+layer*.071)
  extent=1.65+layer*.36
  objects.append(petal('CentralRose_%d_%02d'%(layer,i),
       (.06*math.cos(a),.06*math.sin(a),.14+layer*.19),
       a,extent,.75+layer*.15,rose if layer==0 else (ivory if i%2 else burg),1))
# Braided irregular luminous boundary, separate for runtime seam deformation.
line=[]
for j in range(101):
 y=-15.6+31.2*j/100
 x=.46*math.sin(y*.44)+.22*math.sin(y*1.63)+.09*math.sin(y*3.2)
 line.append((x,y,.05+.04*math.cos(y*.43)))
objects.append(ribbon('Boundary_Contour_Editable',line,.20,seam))
for lane,offset in enumerate((-.17,.17)):
 pts=[(x+offset,y,z+.13+.06*math.sin(y*.9)) for x,y,z in line]
 objects.append(ribbon('Boundary_Interference_%d'%lane,pts,.065,gold if lane==0 else violet))
# Distinct positive/negative field tint—no visual promise of negation success.
for i in range(12):
 theta=math.tau*i/12;day=(math.cos(theta)<0)
 for k in (1,2):
  p=(12.1*math.cos(theta),12.1*math.sin(theta),.03)
  objects.append(petal('FieldTile_%02d_%d'%(i,k),p,theta+(k-1.5)*.18,
      2.7,.13,ivory if day else violet,.2))
# Editable source with componentized ribs and petals, cinematic camera only.
camera_data=bpy.data.cameras.new('FieldProductionPreviewCamera')
camera=bpy.data.objects.new('FieldProductionPreviewCamera',camera_data);bpy.context.collection.objects.link(camera)
camera.location=(28,-31,24);camera.rotation_euler=(Vector((0,0,2.6))-camera.location).to_track_quat('-Z','Y').to_euler()
camera_data.type='ORTHO';camera_data.ortho_scale=42;scene.camera=camera
scene.render.engine='BLENDER_WORKBENCH';scene.render.resolution_x=1100;scene.render.resolution_y=740
scene.display.shading.light='STUDIO';scene.display.shading.color_type='MATERIAL';scene.display.shading.show_cavity=True
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(REC)
bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(SRC),compress=True)
# GLB asset not camera / lights; all geometry remains with physically separate material zones.
bpy.ops.object.select_all(action='DESELECT')
for ob in objects:ob.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.export_scene.gltf(filepath=str(GLB),export_format='GLB',use_selection=True,
 export_yup=True,export_apply=False)
R=max(math.hypot((ob.matrix_world@v.co).x,(ob.matrix_world@v.co).y)
 for ob in objects for v in ob.data.vertices)
assert R<=16.02,(R)
record={'schema':'G09_SOVEREIGN_ECLIPSE_ARCHITECTURAL_SOURCE_PROOF_V1',
 'grade':'World Negation','radius_m':16,'geometry_measured_max_radius_m':R,
 'rib_count':12,'editable_mesh_component_count':len(objects),
 'has_separate_boundary_seam':True,'day_night_look_separated':True,
 'field_runtime_terrain_intersection':'NOT_IMPLEMENTED',
 'field_gameplay_authority':'NOT_IMPLEMENTED',
 'animated_growth':'NOT_IMPLEMENTED','collision_success_failure_vfx':'NOT_IMPLEMENTED',
 'niagara':'NOT_IMPLEMENTED','art_approved':False,
 'assets':[{'path':str(x),'sha256':hashlib.sha256(x.read_bytes()).hexdigest(),'bytes':x.stat().st_size}
           for x in (SRC,GLB,REC)]}
MAN.write_text(json.dumps(record,indent=2)+'\n',encoding='utf8')
print('G09_SOVEREIGN_FIELD_GEOMETRY_CREATED',len(objects),'R',round(R,4),
 'GLB_MB',round(GLB.stat().st_size/1048576,2),flush=True)
