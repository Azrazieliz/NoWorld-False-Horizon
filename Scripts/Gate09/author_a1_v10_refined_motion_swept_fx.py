"""Gate09 A1 sword arc source: geometry extracted from ACTUAL existing 975-bone
animated sword transform at contact frames; FX-only alpha render pass.
Blender 5.2 fully editable real mesh/petals, physical blade motion only.
Use standalone overlay/compositor: avoid re-rendering the character for FX polish.
"""
import bpy,pathlib,hashlib,json,math,random
from mathutils import Vector
random.seed(12001)
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
SRC=P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.blend'
DST=P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_v10.blend'
GLB=P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_Unreal_v10.glb'
REC=P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_v10.json'
for a in (DST,GLB,REC):
 if a.exists():raise FileExistsError(a)
bpy.ops.wm.open_mainfile(filepath=str(SRC),load_ui=False)
s=bpy.context.scene
s.frame_start=1;s.frame_end=33
sword=bpy.data.objects['G09_A1_PhysicalSword_GripFollowingRightWrist']
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE' and len(o.pose.bones)==975)
assert sword.parent==rig and sword.parent_type=='BONE' and sword.parent_bone=='Right wrist'
def mat(name,color,alpha=.5,glow=1):
 m=bpy.data.materials.new(name);m.use_nodes=True
 ns=m.node_tree.nodes;ns.clear();links=m.node_tree.links
 out=ns.new('ShaderNodeOutputMaterial')
 em=ns.new('ShaderNodeEmission');em.inputs['Color'].default_value=(*color,1);em.inputs['Strength'].default_value=glow
 tr=ns.new('ShaderNodeBsdfTransparent')
 mix=ns.new('ShaderNodeMixShader');mix.inputs[0].default_value=alpha
 links.new(tr.outputs[0],mix.inputs[1]);links.new(em.outputs[0],mix.inputs[2]);links.new(mix.outputs[0],out.inputs['Surface'])
 m.diffuse_color=(*color,alpha);m.surface_render_method='DITHERED'
 return m
silver=mat('FX_A1_BladeSweep_DiffusedForgedSilver',(.68,.79,.94),.64,1.2)
crimson=mat('FX_A1_BloodSigil_ControlledCrimson',(.57,.018,.042),.55,1.4)
sparks=mat('FX_A1_RoseLight_FineSparks',(.90,.43,.49),.66,1.75)
def mesh(name,vs,fs,material):
 me=bpy.data.meshes.new(name+'_mesh');me.from_pydata(vs,[],fs);me.update();me.materials.append(material)
 ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob)
 return ob
fx=[]
hits=[6,12,20]
contact_records=[]
for index,hit in enumerate(hits,1):
 # Evaluate world blade local at authentic input frames, not manual guessed positions.
 samples=[]
 for f in range(hit-3,hit+2):
  s.frame_set(f);bpy.context.view_layer.update()
  matx=sword.matrix_world.copy()
  pbase=matx@Vector((0,0,.31));ptip=matx@Vector((0,0,1.13))
  samples.append((f,pbase,ptip))
 vertices=[];faces=[]
 for i,(frame,pbase,ptip) in enumerate(samples):
  prevtip=samples[max(0,i-1)][2]
  nexttip=samples[min(len(samples)-1,i+1)][2]
  tangent=nexttip-prevtip
  if tangent.length<1e-5:tangent=Vector((.1,-.2,.1))
  tangent.normalize()
  normal=tangent.cross(Vector((0,0,1)))
  if normal.length<1e-5:normal=tangent.cross(Vector((0,1,0)))
  normal.normalize()
  taper=(.22+.78*math.sin(math.pi*i/(len(samples)-1)))
  width=.038*taper
  vertices.extend([tuple(ptip-normal*width),tuple(ptip+normal*width)])
 for k in range(len(samples)-1):
  faces.append((2*k,2*k+1,2*k+3,2*k+2))
  faces.append((2*k+2,2*k+3,2*k+1,2*k))
 main=mesh('FX_A1_RealSwordMotion_%02d_SilverSweptSurface'%index,vertices,faces,silver)
 fx.append(main)
 # Distinct thin red sword-tip trace, based on same motion history, with bevelled curved spline.
 c=bpy.data.curves.new('FX_A1_RealSwordMotion_%02d_CrimsonCurve'%index,'CURVE')
 c.dimensions='3D';c.resolution_u=20;c.bevel_depth=.010;c.bevel_resolution=3
 spline=c.splines.new('BEZIER');spline.bezier_points.add(len(samples)-1)
 for j,(_f,_base,tip) in enumerate(samples):
  point=spline.bezier_points[j];point.co=tip;point.handle_left_type='AUTO';point.handle_right_type='AUTO'
 ob=bpy.data.objects.new('FX_A1_RealSwordMotion_%02d_CrimsonTracery'%index,c)
 bpy.context.collection.objects.link(ob);c.materials.append(crimson)
 bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
 bpy.ops.object.convert(target='MESH');ob=bpy.context.object
 fx.append(ob)
 # Physically located 9 refined tiny petal sparks from the real contact-point blade tip.
 center=samples[3][2]
 forward=samples[-1][2]-samples[0][2]
 if forward.length<.03:forward=Vector((.1,-.2,.06))
 forward.normalize()
 verts=[];fac=[]
 for j in range(9):
  theta=(2*math.pi*j/9)+random.uniform(-.13,.13)
  direct=Vector((math.cos(theta),math.sin(theta),random.uniform(-.55,.6))).normalized()
  reach=random.uniform(.05,.22)
  size=random.uniform(.008,.025)
  base=center+direct*.08
  outward=center+direct*reach+forward*(.03*math.sin(theta))
  side=direct.cross(Vector((0,0,1)))
  if side.length<.05:side=Vector((1,0,0))
  side.normalize()
  o=len(verts);verts.extend([tuple(base),tuple(outward+side*size),tuple(outward-side*size)])
  fac.append((o,o+1,o+2))
 pet=mesh('FX_A1_RealSwordMotion_%02d_ScarletPetalSparks'%index,verts,fac,sparks)
 fx.append(pet)
 contact_records.append({'hit':index,'frame':hit,
  'tip_contact_xyz_m':[round(float(a),5) for a in center],
  'blade_sweep_source_frames':[q[0] for q in samples],
  'actual_source_world_sweep_distance_m':round((samples[-1][2]-samples[0][2]).length,4)})
 # Owner of asset visibility is the actual contact point, NOT static continuous stream.
 for ob in (main,fx[-2],pet):
  for f,v in ((1,False),(hit-3,False),(hit-2,True),(hit,True),(hit+2,False),(33,False)):
   ob.hide_render=not v;ob.keyframe_insert('hide_render',frame=f,group='PhysicalContactEffectWindow')
   ob.hide_viewport=not v;ob.keyframe_insert('hide_viewport',frame=f,group='PhysicalContactEffectWindow')
# Preserve master source rig/shape but exclude model and stage from transparency-only FX pass.
for obj in bpy.data.objects:
 if obj in fx:continue
 if obj.type=='MESH':obj.hide_render=True
# Explicitly no re-posing character, only splines from evaluated actual equipment.
s.render.engine='BLENDER_EEVEE';s.render.film_transparent=True
s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGBA'
s.render.resolution_x=640;s.render.resolution_y=720;s.render.resolution_percentage=100
s.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(DST),compress=True)
# Export only actual effect geometry into reusable UE staticmesh sources.
bpy.ops.object.select_all(action='DESELECT')
for ob in fx:
 ob.animation_data_clear() # The authored source was saved before export, so clear visibility FCurves only on the export instances.
 ob.hide_render=False;ob.hide_viewport=False;ob.select_set(True)
bpy.context.view_layer.objects.active=fx[0]
bpy.ops.export_scene.gltf(filepath=str(GLB),export_format='GLB',use_selection=True,
 export_yup=True,export_apply=False)
assert len(fx)==9 and GLB.stat().st_size>10000
record={'schema':'G09_A1_ACTUAL_SWORD_TIP_THIN_SWEEP_CONTROLLED_CRIMSON_V10',
'blender_version':bpy.app.version_string,
'source_sha256':hashlib.sha256(SRC.read_bytes()).hexdigest(),
'fx_blend':str(DST),'fx_blend_sha256':hashlib.sha256(DST.read_bytes()).hexdigest(),
'fx_glb':str(GLB),'fx_glb_sha256':hashlib.sha256(GLB.read_bytes()).hexdigest(),
'fx_editable_meshes':9,'three_contact_records':contact_records,
'blender_character_original_materials_untouched':True,
'actual_niagara_system':'NOT_YET','runtime_authority':'GATE12',
'fx_time_windows_source_frames':[6,12,20]}
REC.write_text(json.dumps(record,indent=2)+'\n',encoding='utf8')
print('REAL_BLADE_HISTORY_9FX_GEOMETRIES',GLB.stat().st_size,flush=True)
