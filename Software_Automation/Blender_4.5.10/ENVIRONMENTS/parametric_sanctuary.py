"""Parametric fantasy architectural asset FAMILY engineering probe; no canonical game content.
Blender 4.5.10. Stays in independent worktree. Change CONFIG to generate variants.
"""
import bpy, math, random, json, os, time, sys
from mathutils import Vector
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase2")
os.makedirs(OUT,exist_ok=True)
CONFIG=dict(width=15.0, depth=5.8, height=5.4, bays=5, tower_radius=1.25,
  tower_height=9.2, arch_radius=2.1, arch_stones=15, roof_pitch=1.05,
  trees=11, seed=3207, erosion=0.08)
rng=random.Random(CONFIG["seed"]);started=time.perf_counter(); objects=[]; notes=[]
def mat(name, rgba, rough=.55, metallic=0):
 m=bpy.data.materials.new(name);m.diffuse_color=tuple(rgba);m.use_nodes=True
 p=m.node_tree.nodes.get("Principled BSDF");p.inputs["Base Color"].default_value=rgba
 p.inputs["Roughness"].default_value=rough;p.inputs["Metallic"].default_value=metallic
 return m
def cube(name,xyz,dims,material,bev=0,role="ENV"):
 bpy.ops.mesh.primitive_cube_add(size=1,location=xyz);o=bpy.context.object;o.name=name;o.dimensions=dims
 bpy.ops.object.transform_apply(location=False, rotation=False,scale=True)
 o.data.materials.append(material)
 if bev:
  m=o.modifiers.new("NW_Bevel","BEVEL");m.width=bev;m.segments=2
  o.modifiers.new("NW_WeightedNormals","WEIGHTED_NORMAL")
 o["nw_role"]=role;objects.append(o);return o
def cyl(name,xyz,r,depth,material,verts=20,bev=0):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=xyz)
 o=bpy.context.object;o.name=name;o.data.materials.append(material)
 if bev:
  b=o.modifiers.new("NW_Bevel","BEVEL");b.width=bev;b.segments=2
  o.modifiers.new("NW_WeightedNormals","WEIGHTED_NORMAL")
 o["nw_role"]="ENV";objects.append(o);return o
def cone(name,xyz,rad1,rad2,height,material,verts=16):
 bpy.ops.mesh.primitive_cone_add(vertices=verts,radius1=rad1,radius2=rad2,depth=height,location=xyz)
 o=bpy.context.object;o.name=name;o.data.materials.append(material);o["nw_role"]="ENV";objects.append(o);return o
def beam(name,start,end,r,material,verts=8):
 a,b=Vector(start),Vector(end);d=b-a
 o=cyl(name,(a+b)*.5,r,d.length,material,verts)
 o.rotation_euler=d.to_track_quat('Z','Y').to_euler()
 return o
def arch(prefix,cx,cy,zbase,r,thickness,segments,material):
 # Semicircle of beveled wedge-shaped voussoir blocks: local Y-axis tangent is rotated around depth.
 cx=float(cx); r=float(r)
 for j in range(segments):
  a=math.pi-(j+.5)*math.pi/segments
  x=cx+r*math.cos(a);z=zbase+r*math.sin(a)
  o=cube(prefix+"_Voussoir_%02d"%j,(x,cy,z),(.48,thickness,2*math.pi*r/segments*.96),material,.035)
  o.rotation_euler[1]=-a
 # Vertical posts directly under arch springline
 for sign in [-1,1]:
  cube(prefix+"_Abutment_%s"%sign,(cx+sign*r,cy,zbase*.5),(.7,thickness,zbase),material,.065)
def tree(i,x,y,stone,leaf,leaf2):
 height=rng.uniform(3.2,5.2);jitter=rng.uniform(-.2,.2)
 beam("NW_Tree_%02d_Trunk"%i,(x,y,0),(x+jitter,y,height*.74),.14,stone,8)
 for k in range(3):
  rr=(1.42-k*.32)*height/4
  cone("NW_Tree_%02d_Crown_%d"%(i,k),(x+jitter*(k/3),y,height*(.42+k*.18)),rr,.02,height*.42,leaf if k!=1 else leaf2,9)
def build():
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 stone=mat("NW_Slate_Stone",(0.15,.22,.28,1))
 light=mat("NW_Frosted_Masonry",(.42,.61,.63,1))
 dark=mat("NW_Dark_Recess",(.02,.055,.083,1))
 copper=mat("NW_Ember_Gold",(.83,.48,.16,1),.34,.35)
 roof=mat("NW_Deep_Azure",(.06,.14,.34,1),.37,.21)
 glow=mat("NW_Arcane_Cyan",(.09,.82,.77,1),.28,.16)
 leaf=mat("NW_Foliage_Emerald",(.035,.25,.20,1))
 leaf2=mat("NW_Foliage_Silver",(.14,.44,.38,1))
 mulch=mat("NW_Basalt",(.09,.11,.17,1))
 mats=[stone,light,dark,copper,roof,glow,leaf,leaf2,mulch]
 W,D,H=CONFIG['width'],CONFIG['depth'],CONFIG['height']
 cube("NW_Courtyard_Terrace",(0,0,-.36),(W+14,D+16,.72),mulch,.25)
 for k in range(3):
  cube("NW_Entry_Stair_%d"%k,(0,-D/2-1.8-k*.9,-.18-k*.095),(5.7+k*1.05,.84,.38),stone,.06)
 cube("NW_Main_Palace_Core",(0,0,H*.50),(W,D,H),stone,.15)
 for sign in [-1,1]:
  x=sign*(W/2+1.15)
  for level in range(5):
   z=(level+.5)*CONFIG["tower_height"]/5
   cyl("NW_Tower_%s_Segment_%d"%(sign,level),(x,0,z),CONFIG['tower_radius']*(1+.025*(level%2)),CONFIG['tower_height']/5+.08,
       stone if level%2==0 else light,16,.035)
  for level in range(3):
   cyl("NW_Tower_%s_Belt_%d"%(sign,level),(x,0,2.1+level*2.45),1.35,.18,copper,24,.025)
  cone("NW_Tower_%s_Roof"%sign,(x,0,10.0),1.66,.03,2.7,roof,16)
  beam("NW_Tower_%s_Finial"%sign,(x,0,11.2),(x,0,12.3),.115,copper)
  cone("NW_Tower_%s_Crystal"%sign,(x,0,12.40),.28,0,.72,glow,8)
 # Architectural portal is black recessed volume, with actual curved stone above.
 cube("NW_Door_Recess",(0,-D/2-.115,1.4),(3.25,.13,2.8),dark,.03)
 arch("NW_Gateway",0,-D/2-.30,2.25,CONFIG['arch_radius'],.64,CONFIG["arch_stones"],light)
 for x in [-5.8,-3.9,3.9,5.8]:
  cube("NW_Arcade_Niche_%s"%x,(x,-D/2-.07,2.62),(1.2,.15,2.1),dark,.035)
  arch("NW_Arcade_%s"%x,x,-D/2-.18,3.36,.60,.23,7,light)
  cube("NW_Arcade_Sill_%s"%x,(x,-D/2-.34,1.57),(1.55,.47,.15),copper,.025)
 for x in [-6.8,-4.6,-2.5,2.5,4.6,6.8]:
  cube("NW_Pilaster_%s"%x,(x,-D/2-.24,2.65),(.32,.44,5.05),light,.04)
  cube("NW_PilasterCapital_%s"%x,(x,-D/2-.32,5.06),(.65,.62,.29),copper,.025)
 for z,width in [(H-.13,W+.85),(H+.32,W+1.2),(H+.72,W+1.1)]:
  cube("NW_RoofCornice_%.2f"%z,(0,0,z),(width,D+.9,.17),copper if z!=H+.32 else roof,.045)
 # Central layered roof crown.
 cube("NW_RoofPlate",(0,0,H+1),(W+1.25,D+1.2,.55),roof,.1)
 for x in [-5.2,-2.6,0,2.6,5.2]:
  beam("NW_Roof_Rib_%s"%x,(x,-D/2-.55,H+.95),(x,D/2+.55,H+.95),.065,copper)
 for side in [-1,1]:
  bx=side*(W*.3)
  for k in range(6):
   y=-D/2+.55+k*(D-1.1)/5
   cube("NW_SidePin_%s_%d"%(side,k),(bx,y,H+1.27),(.18,.2,.31),copper,.045)
 # Crystal centerpiece + patterned roof.
 cone("NW_Spire_Base",(0,0,H+1.82),.94,.17,1.26,light,12)
 cone("NW_Spire_Crystal",(0,0,H+2.75),.57,0,1.85,glow,6)
 for s in [-1,1]:
  beam("NW_Crossbar_%s"%s,(0,0,H+2.1),(s*1.0,0,H+1.8),.085,copper)
 # Extra architecture and environment dressing seeded, without destructive scattering.
 for k in range(6):
  x=(k-2.5)*2.3
  cube("NW_Paving_Band_%d"%k,(x,-D/2-4.4,-.005),(1.6,5.0,.05),light,.02)
 for i in range(CONFIG['trees']):
  sign=-1 if i%2==0 else 1
  x=sign*rng.uniform(W/2+3.6,W/2+6.6)
  y=rng.uniform(-D/2-3.5,D/2+7)
  tree(i,x,y,stone,leaf,leaf2)
 for i in range(26):
  x=rng.uniform(-W/2-5,W/2+5);y=rng.uniform(-D/2-5,D/2+8)
  if abs(x)<W/2+1 and abs(y)<D/2+1:continue
  o=cube("NW_Rock_%d"%i,(x,y,rng.uniform(.12,.3)),(rng.uniform(.3,.8),rng.uniform(.32,.86),rng.uniform(.25,.6)),mulch,.08)
  o.rotation_euler=(rng.uniform(-.2,.2),rng.uniform(-.2,.2),rng.uniform(0,6.28))
 # Editable "materials" are exact scene data, controls live in CONFIG at source level.
 bpy.context.scene["nw_generator_parameters"]=json.dumps(CONFIG)
 bpy.context.scene["nw_generator"]="NW_ParametricSanctuary_0.2"
 return mats
def output():
 scene=bpy.context.scene;scene.render.engine="BLENDER_WORKBENCH"
 scene.display.shading.light='STUDIO'
 scene.display.shading.color_type='MATERIAL'
 scene.display.shading.show_cavity=True;scene.display.shading.cavity_type='BOTH'
 scene.display.shading.curvature_ridge_factor=1.25
 scene.render.resolution_x=1000;scene.render.resolution_y=620
 scene.render.resolution_percentage=100
 scene.render.image_settings.file_format='PNG'
 cam_d=bpy.data.cameras.new("NW_ArchitecturalAuditCamera")
 cam=bpy.data.objects.new("NW_ArchitecturalAuditCamera",cam_d);scene.collection.objects.link(cam)
 cam.location=(20,-29,19);look=Vector((0,0,4.5))-cam.location
 cam.rotation_euler=look.to_track_quat('-Z','Y').to_euler()
 cam_d.type='ORTHO';cam_d.ortho_scale=32;scene.camera=cam
 saved=os.path.join(OUT,"NW_ParametricSanctuary.blend")
 bpy.ops.wm.save_as_mainfile(filepath=saved,compress=True)
 preview=os.path.join(OUT,"NW_ParametricSanctuary.png")
 scene.render.filepath=preview;bpy.ops.render.render(write_still=True)
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0]
 fbx=os.path.join(OUT,"NW_ParametricSanctuary.fbx")
 bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'MESH'},apply_unit_scale=True,bake_anim=False)
 quality={"blender":bpy.app.version_string,"generator":"NW_ParametricSanctuary_0.2",
   "parameters":CONFIG,"object_count":len(objects),
   "editable_source":saved,"preview":preview,"fbx":fbx,
   "polygon_count":sum(len(o.data.polygons) for o in objects if o.type=="MESH"),
   "all_source_meshes_have_uv":all(len(o.data.uv_layers)>0 for o in objects if o.type=="MESH"),
   "seconds":round(time.perf_counter()-started,3),"artistic_accepted":False,"unreal_accepted":False}
 with open(os.path.join(OUT,"sanctuary_results.json"),"w") as f:json.dump(quality,f,indent=2)
 print("NW_SANCTUARY="+json.dumps(quality))
if __name__=="__main__":
 build();output()
