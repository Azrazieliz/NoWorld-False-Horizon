"""Procedural articulated quadruped/mount rig and animation source, isolated probe.
Animation has NOT passed professional contact/locomotion quality gates.
"""
import bpy, math, os, json, time, traceback
from mathutils import Vector,Matrix
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase2");os.makedirs(OUT,exist_ok=True)
t=time.perf_counter();parts=[]
C={"body_length":2.80,"height":2.15,"leg_spread":0.75,"gait_period":24,"wing_span":3.3,"steps":48}
def mat(n,c):
 m=bpy.data.materials.new(n);m.diffuse_color=tuple(c);m.use_nodes=True
 m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=c
 return m
def mesh_obj(name,kind,center,scale,matl,bone,vertices=16):
 if kind=="ellipsoid":bpy.ops.mesh.primitive_uv_sphere_add(segments=vertices,ring_count=12)
 elif kind=="cone":bpy.ops.mesh.primitive_cone_add(vertices=vertices,radius1=1,radius2=0,depth=2)
 elif kind=="cube":bpy.ops.mesh.primitive_cube_add(size=2)
 else:raise ValueError(kind)
 ob=bpy.context.object;ob.name="NW_"+name;ob.data.name="Mesh_"+name
 ob.data.transform(Matrix.Diagonal((scale[0],scale[1],scale[2],1)))
 ob.data.transform(Matrix.Translation(Vector(center)))
 ob.location=(0,0,0)
 ob.data.materials.append(matl)
 vg=ob.vertex_groups.new(name=bone)
 vg.add(list(range(len(ob.data.vertices))),1,"REPLACE")
 ob["nw_deform_bone"]=bone;ob["nw_source_part"]=name;parts.append(ob)
 return ob
def fin(name,verts,faces,matl,bone):
 me=bpy.data.meshes.new(name+"_mesh");me.from_pydata(verts,[],faces);me.update()
 ob=bpy.data.objects.new("NW_"+name,me);bpy.context.collection.objects.link(ob)
 me.materials.append(matl);vg=ob.vertex_groups.new(name=bone)
 vg.add(list(range(len(verts))),1,'REPLACE')
 ob["nw_source_part"]=name;parts.append(ob);return ob
def make_rig():
 a=bpy.data.armatures.new("NW_Skyrunner_Mount_Skeleton")
 rig=bpy.data.objects.new("NW_Skyrunner_Mount_Rig",a)
 bpy.context.collection.objects.link(rig)
 bpy.context.view_layer.objects.active=rig;rig.select_set(True)
 bpy.ops.object.mode_set(mode="EDIT")
 specs={
 "Root":((0,0,.2),(0,0,1.5),None),
 "Body":((0,0,1.4),(0,0,2.3),"Root"),
 "Neck":((0,-1.0,2.1),(0,-1.72,2.65),"Body"),
 "Jaw":((0,-1.62,2.52),(0,-2.15,2.35),"Neck"),
 "TailA":((0,1.12,2.1),(0,1.95,2.1),"Body"),
 "TailB":((0,1.95,2.1),(0,2.75,2.32),"TailA"),
 "WingL":((-.65,.12,2.38),(-2.4,.15,3.18),"Body"),
 "WingR":((.65,.12,2.38),(2.4,.15,3.18),"Body")
 }
 for xside,tag in [(-1,"L"),(1,"R")]:
  for yside,position in [(-1,"Front"),(1,"Hind")]:
   x=xside*C["leg_spread"];y=yside*.92
   specs[position+tag+"_Upper"]=((x,y,1.92),(x,y,1.06),"Body")
   specs[position+tag+"_Lower"]=((x,y,1.06),(x,y,.20),position+tag+"_Upper")
 for name,(head,tail,parent) in specs.items():
  b=a.edit_bones.new(name);b.head=head;b.tail=tail
  if parent:b.parent=a.edit_bones[parent]
 bpy.ops.object.mode_set(mode="OBJECT")
 rig.select_set(False)
 return rig
def build():
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 dark=mat("NW_Indigo_Cel",(0.12,.22,.35,1))
 light=mat("NW_Silver_Highlight",(.49,.72,.78,1))
 gold=mat("NW_Mount_Gold",(.9,.53,.2,1))
 deep=mat("NW_Wing_Deep",(.09,.30,.38,1))
 teal=mat("NW_Teal_Gem",(.08,.78,.69,1))
 black=mat("NW_Black_Iris",(.015,.02,.04,1))
 ground=mat("NW_Studio_Floor",(.18,.22,.30,1))
 rig=make_rig()
 mesh_obj("Barrel","ellipsoid",(0,0,2.05),(.79,1.36,.71),dark,"Body")
 mesh_obj("Chest","ellipsoid",(0,-.87,2.1),(.76,.68,.75),light,"Body")
 mesh_obj("Neck_Collar","ellipsoid",(0,-1.27,2.47),(.40,.57,.54),dark,"Neck")
 head=mesh_obj("Head","ellipsoid",(0,-1.87,2.73),(.46,.58,.44),light,"Neck")
 mesh_obj("Muzzle","ellipsoid",(0,-2.30,2.52),(.33,.48,.20),dark,"Jaw")
 for sign,side in [(-1,"L"),(1,"R")]:
  mesh_obj("Eye_%s"%side,"ellipsoid",(sign*.43,-1.96,2.84),(.08,.19,.16),teal,"Neck")
  mesh_obj("Pupil_%s"%side,"ellipsoid",(sign*.491,-2.03,2.85),(.046,.082,.09),black,"Neck")
  mesh_obj("Horn_%s"%side,"cone",(sign*.32,-1.84,3.20),(.11,.12,.45),gold,"Neck",9)
  mesh_obj("Ear_%s"%side,"ellipsoid",(sign*.46,-1.40,3.15),(.23,.12,.38),dark,"Neck")
 for tag,sy in [("Front",-1),("Hind",1)]:
  for side,sx in [("L",-1),("R",1)]:
   x=sx*C["leg_spread"];y=sy*.92
   mesh_obj(tag+side+"_Thigh","ellipsoid",(x,y,1.40),(.24,.29,.54),dark,tag+side+"_Upper")
   mesh_obj(tag+side+"_Shin","ellipsoid",(x,y,.64),(.17,.20,.49),light,tag+side+"_Lower")
   mesh_obj(tag+side+"_Claw","ellipsoid",(x,y-.20,.21),(.25,.35,.16),gold,tag+side+"_Lower")
 for k,(y,z,bone,r) in enumerate([(1.38,2.09,"TailA",.32),(2.00,2.20,"TailB",.24),(2.60,2.36,"TailB",.16)]):
  mesh_obj("TailSegment_%d"%k,"ellipsoid",(0,y,z),(r,.53,r*.95),dark if k<2 else gold,bone)
 for side,sx in [("L",-1),("R",1)]:
  verts=[(sx*.65,-.05,2.34),(sx*1.22,-.65,2.90),(sx*2.10,-.45,3.06),
         (sx*2.9,.33,3.41),(sx*2.22,.76,3.12),(sx*.85,.62,2.45)]
  ob=fin("MembraneWing_"+side,verts,[(0,1,2,3),(0,3,4,5)],deep,"Wing"+side)
  # Wings are intentionally separate topological surfaces, manually editable.
  mesh_obj("WingTip_"+side,"cone",(sx*2.96,.24,3.42),(.13,.18,.43),gold,"Wing"+side,8)
 # Saddle and harness, proof of rider mounting asset relationships.
 mesh_obj("Saddle","ellipsoid",(0,.05,2.85),(.66,.85,.15),gold,"Body")
 mesh_obj("Saddle_Cushion","ellipsoid",(0,-.05,2.93),(.46,.69,.07),dark,"Body")
 for side,sx in [("L",-1),("R",1)]:
  mesh_obj("Rein_Buckle_"+side,"cube",(sx*.74,-.63,2.22),(.06,.19,.17),gold,"Body")
 # Facial control with editable shape key, noncanonical test.
 head.shape_key_add(name="Basis")
 key=head.shape_key_add(name="NW_Facial_Expressive_Squeeze")
 for i,v in enumerate(head.data.vertices):
  co=key.data[i].co
  if co.z>2.67:co.z-=.065
  co.y-=.025
 key.value=0
 for f,v in [(1,0),(9,.8),(15,0),(25,.7),(32,0),(48,0)]:
  key.value=v;key.keyframe_insert(data_path="value",frame=f)
 # Armature and deformation links
 for part in parts:
  mod=part.modifiers.new("NW_Mount_Deform","ARMATURE");mod.object=rig
 scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=C['steps'];scene.render.fps=24
 for f in range(1,49,3):
  phase=2*math.pi*(f-1)/C['gait_period']
  for tag in ["FrontL","FrontR","HindL","HindR"]:
   upper=rig.pose.bones[tag+"_Upper"];lower=rig.pose.bones[tag+"_Lower"]
   upper.rotation_mode="XYZ";lower.rotation_mode="XYZ"
   ofs=math.pi if tag in ("FrontR","HindL") else 0
   upper.rotation_euler.x=.19*math.sin(phase+ofs)
   lower.rotation_euler.x=.24*max(0,math.cos(phase+ofs))
   upper.keyframe_insert(data_path="rotation_euler",frame=f,group=tag)
   lower.keyframe_insert(data_path="rotation_euler",frame=f,group=tag)
  for bname,ax,val in [("Body",0,.025*math.sin(phase*2)),
                      ("TailA",2,.18*math.sin(phase)),
                      ("TailB",2,.22*math.sin(phase+.45)),
                      ("Neck",0,.06*math.sin(phase+.3)),
                      ("WingL",1,.12*math.sin(phase)),
                      ("WingR",1,-.12*math.sin(phase))]:
   p=rig.pose.bones[bname];p.rotation_mode="XYZ";p.rotation_euler[ax]=val
   p.keyframe_insert(data_path="rotation_euler",frame=f,group=bname)
 # Optional rider seat anchor is explicit parentable empty tied to Body deform bone.
 seat=bpy.data.objects.new("NW_Rider_SeatAnchor",None);scene.collection.objects.link(seat)
 seat.location=(0,.02,3.0);seat.empty_display_type="SPHERE";seat.empty_display_size=.15
 seat["nw_role"]="MOUNT_SOCKET";seat["nw_bone"]="Body"
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,0,-.18))
 floor=bpy.context.object;floor.name="NW_DisplayFloor";floor.dimensions=(8,9,.36)
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 floor.data.materials.append(ground)
 return rig,head
def output(rig,head):
 scene=bpy.context.scene;scene.frame_set(10)
 camd=bpy.data.cameras.new("NW_CreatureReviewCamera");cam=bpy.data.objects.new("NW_CreatureReviewCamera",camd)
 scene.collection.objects.link(cam);cam.location=(8,-13,7)
 vec=Vector((0,0,1.45))-cam.location;cam.rotation_euler=vec.to_track_quat("-Z","Y").to_euler()
 camd.type='ORTHO';camd.ortho_scale=8;scene.camera=cam
 scene.render.engine='BLENDER_WORKBENCH';scene.display.shading.color_type='MATERIAL'
 scene.display.shading.show_cavity=True
 scene.render.resolution_x=900;scene.render.resolution_y=600;scene.render.resolution_percentage=100
 scene.render.image_settings.file_format="PNG"
 blend=os.path.join(OUT,"NW_ArticulatedMount.blend");bpy.ops.wm.save_as_mainfile(filepath=blend,compress=True)
 png=os.path.join(OUT,"NW_ArticulatedMount.png");scene.render.filepath=png;bpy.ops.render.render(write_still=True)
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 rig.select_set(True);bpy.context.view_layer.objects.active=rig
 fbx=os.path.join(OUT,"NW_ArticulatedMount_Skeletal.fbx")
 bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'ARMATURE','MESH'},
     add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,apply_unit_scale=True)
 stats={'blender':bpy.app.version_string,'mesh_parts':len(parts),'bones':len(rig.data.bones),
 'frames':C['steps'],'shape_keys':len(head.data.shape_keys.key_blocks),
 'animated':rig.animation_data is not None,
 'saved_blend_bytes':os.path.getsize(blend),'fbx_bytes':os.path.getsize(fbx),
 'seconds':round(time.perf_counter()-t,3),
 'foot_contact_validated':False,'native_unreal_import_validated':False,'artistic_accepted':False}
 with open(os.path.join(OUT,"beast_results.json"),"w") as f:json.dump(stats,f,indent=2)
 print("NOWORLD_BEAST="+json.dumps(stats))
if __name__=="__main__":
 rig,head=build();output(rig,head)
