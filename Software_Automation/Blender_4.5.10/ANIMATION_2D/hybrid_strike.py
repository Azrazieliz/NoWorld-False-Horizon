"""Editable hybrid 2D/3D timing study: Grease Pencil v3 keyed impact art with 3D weapon motion.
48 frames at 24fps, no requirement to draw all 48 images independently. Not final anime art.
"""
import bpy,math,os,json,time
from mathutils import Vector,Matrix
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase2");os.makedirs(OUT,exist_ok=True)
t=time.perf_counter()
def mat(n,rgba):
 m=bpy.data.materials.new(n);m.diffuse_color=rgba;m.use_nodes=True
 m.node_tree.nodes.get("Principled BSDF").inputs["Base Color"].default_value=rgba
 return m
def cube(name,loc,dim,m):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
 o=bpy.context.object;o.name=name;o.dimensions=dim;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(m);return o
def gp_mat(name,color):
 m=bpy.data.materials.new(name);bpy.data.materials.create_gpencil_data(m)
 m.grease_pencil.color=color;m.grease_pencil.show_stroke=True;m.grease_pencil.show_fill=False
 return m
def gp_obj(name,material):
 gp=bpy.data.grease_pencils_v3.new(name+"_data")
 ob=bpy.data.objects.new(name,gp);bpy.context.scene.collection.objects.link(ob)
 gp.materials.append(material)
 layer=gp.layers.new("Ink",set_active=True)
 return ob,layer
def add_draw(layer,frame,paths):
 drawing=layer.frames.new(frame).drawing
 drawing.add_strokes([len(path) for path in paths])
 for s,pts in zip(drawing.strokes,paths):
  for p,xy in zip(s.points,pts):
   p.position=(xy[0],-.92,xy[1])
   p.radius=.018
 return drawing
def run():
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=48;scene.render.fps=24
 gold=mat("NW_Blade_Ivory",(.82,.89,.88,1))
 edge=mat("NW_Blade_Edge",(.15,.59,.72,1))
 ink=mat("NW_Hilt_Gold",(.95,.55,.19,1))
 dark=mat("NW_Stage_Backdrop",(.035,.055,.09,1))
 steel=cube("NW_SwordBlade",(0,-.10,1.9),(.22,.19,2.55),gold)
 grip=cube("NW_Grip",(0,-.1,.40),(.21,.26,.58),ink)
 guard=cube("NW_Guard",(0,-.12,.72),(1.20,.26,.16),edge)
 tip=cube("NW_BladePoint",(0,-.12,3.25),(.14,.14,.40),edge)
 objects=[steel,grip,guard,tip]
 pivot=bpy.data.objects.new("NW_SwordRig_Pivot",None);scene.collection.objects.link(pivot)
 for o in objects:o.parent=pivot;o.matrix_parent_inverse=pivot.matrix_world.inverted()
 for f,rot,scale in [(1,-.75,.88),(8,-.46,.94),(16,.36,1.0),(24,1.30,1.08),(32,1.55,1.05),(40,1.64,1),(48,1.75,1)]:
  pivot.rotation_euler[1]=rot
  pivot.scale=(scale,scale,scale)
  pivot.keyframe_insert(data_path="rotation_euler",frame=f)
  pivot.keyframe_insert(data_path="scale",frame=f)
 # Stylized 2D trails, key drawings at independent poses only, not per-frame drawings.
 c1=gp_mat("NW_Stroke_Teal",(.18,.95,.87,1))
 c2=gp_mat("NW_Stroke_Gold",(.97,.75,.34,1))
 trail,tl=gp_obj("NW_GP_SlashTrail",c1)
 sparks,sl=gp_obj("NW_GP_ImpactStreaks",c2)
 keyframes=[1,8,16,24,32,40,48]
 for f in keyframes:
  phase=(f-1)/47
  # Arcs in X-Z camera plane, multiple radii and staggered endpoints.
  segments=[]
  for ridge in [0,.15,.30]:
   pp=[]
   for i in range(25):
    a=(-.95+phase*2.5)+(i/24)*(.65+phase*.9)
    r=2.5+ridge
    pp.append((r*math.cos(a)-.1,r*math.sin(a)+1.95))
   segments.append(pp)
  add_draw(tl,f,segments)
  strokes=[]
  center=(-.2+math.sin(phase*math.pi)*.9,2.15)
  for i in range(13):
   angle=i*math.tau/13+.11*f
   r1=.18+phase*.25;r2=.56+phase*1.10
   strokes.append([(center[0]+r1*math.cos(angle),center[1]+r1*math.sin(angle)),
                   (center[0]+r2*math.cos(angle),center[1]+r2*math.sin(angle))])
  add_draw(sl,f,strokes)
 backdrop=cube("NW_CinematicPlane",(0,2.15,1.7),(14,.12,8.4),dark)
 # Parallax camera: sword animation interpolates every render frame, GP stroke drawings hold.
 camdata=bpy.data.cameras.new("NW_2D_View");camera=bpy.data.objects.new("NW_2D_View",camdata)
 scene.collection.objects.link(camera);camera.location=(0,-18,2.0)
 d=Vector((0,0,2.0))-camera.location
 camera.rotation_euler=d.to_track_quat("-Z","Y").to_euler()
 camdata.type='ORTHO';camdata.ortho_scale=8.8;scene.camera=camera
 scene.render.engine="BLENDER_WORKBENCH";scene.display.shading.color_type="MATERIAL"
 scene.display.shading.show_cavity=True
 scene.render.resolution_x=640;scene.render.resolution_y=360;scene.render.resolution_percentage=100
 scene.frame_set(24)
 path=os.path.join(OUT,"NW_Hybrid_Strike.blend")
 bpy.ops.wm.save_as_mainfile(filepath=path,compress=True)
 scene.render.image_settings.file_format='PNG'
 still=os.path.join(OUT,"NW_Hybrid_Strike_Keyframe.png")
 scene.render.filepath=still;bpy.ops.render.render(write_still=True)
 scene.render.image_settings.file_format="FFMPEG";scene.render.ffmpeg.format="MPEG4";scene.render.ffmpeg.codec="H264"
 video=os.path.join(OUT,"NW_Hybrid_Strike.mp4");scene.render.filepath=video
 bpy.ops.render.render(animation=True)
 checks={"blender":bpy.app.version_string,"grease_pencil_objects":2,"distinct_drawings":len(keyframes)*2,
 "frames_rendered":48,"fps":24,"animated_3d_pivot":bool(pivot.animation_data),
 "blend_bytes":os.path.getsize(path),"still_bytes":os.path.getsize(still),
 "video_bytes":os.path.getsize(video),"total_seconds":round(time.perf_counter()-t,3),
 "artistic_accepted":False}
 with open(os.path.join(OUT,"hybrid_results.json"),"w") as fd:json.dump(checks,fd,indent=2)
 print("NOWORLD_2D="+json.dumps(checks))
if __name__=="__main__":run()
