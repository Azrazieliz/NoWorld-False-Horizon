"""Blender 4.5 reference exercise: parametric lathe using exact BMesh spin operator.
Technical tooling; not a professional hero asset. Designed to expand as architectural prop library.
"""
import bpy,bmesh,math,json,os,time
from mathutils import Vector
BASE=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(BASE,"VALIDATION","reference_20261015")
os.makedirs(OUT,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
sources={
 "Baluster":[(.19,0),(.29,.06),(.31,.12),(.23,.15),(.11,.22),(.10,.29),(.22,.4),(.23,.46),(.15,.56),(.12,.72),(.16,.86),(.23,1.00),(.29,1.06)],
 "Vase":[(.12,0),(.22,.06),(.26,.14),(.38,.29),(.42,.45),(.39,.65),(.26,.84),(.22,.94),(.30,1.01)],
 "Finial":[(.22,0),(.32,.05),(.33,.11),(.19,.16),(.10,.22),(.15,.34),(.26,.44),(.23,.55),(.11,.67),(.07,.87),(.02,1.16)]
}
objects=[];data=[]
for i,(name,profile) in enumerate(sources.items()):
 mesh=bpy.data.meshes.new("NW_Asset_"+name)
 obj=bpy.data.objects.new("NW_Asset_"+name,mesh)
 bpy.context.collection.objects.link(obj)
 bm=bmesh.new()
 vs=[bm.verts.new((r,0,z)) for r,z in profile]
 for a,b in zip(vs,vs[1:]):bm.edges.new((a,b))
 bm.verts.ensure_lookup_table();bm.edges.ensure_lookup_table()
 bmesh.ops.spin(bm,geom=list(bm.verts)+list(bm.edges),
   cent=(0,0,0),axis=(0,0,1),dvec=(0,0,0),angle=math.tau,steps=64,use_duplicate=False)
 bmesh.ops.remove_doubles(bm,verts=bm.verts[:],dist=.00001)
 bmesh.ops.recalc_face_normals(bm,faces=bm.faces[:])
 bm.to_mesh(mesh);bm.free()
 assert len(mesh.polygons)>len(profile)*32,(name,len(mesh.polygons))
 uv=mesh.uv_layers.new(name="NW_SpinCylindricalUV")
 hi=max(z for _,z in profile)
 for face in mesh.polygons:
  ps=[mesh.vertices[mesh.loops[j].vertex_index].co for j in face.loop_indices]
  us=[math.atan2(p.y,p.x)%(math.tau)/math.tau for p in ps]
  for j,p,u in zip(face.loop_indices,ps,us):
   if max(us)-min(us)>.5 and u<.5:u+=1
   uv.data[j].uv=(u,p.z/hi)
  face.use_smooth=True
 obj.location.x=(i-1)*1.35
 obj["nw_spin_profile"]=json.dumps(profile)
 mat=bpy.data.materials.new("NW_StoneOrMetal_"+name);mat.diffuse_color=(.37,.50,.57,1)
 obj.data.materials.append(mat)
 mod=obj.modifiers.new("NW_Subdivision","SUBSURF");mod.levels=1;mod.render_levels=1
 objects.append(obj)
 data.append(dict(asset=name,points=len(profile),verts=len(mesh.vertices),
   quads=sum(len(p.vertices)==4 for p in mesh.polygons),faces=len(mesh.polygons),
   uv_layers=len(mesh.uv_layers),source_profile_editable=True))
scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH';scene.display.shading.color_type='MATERIAL'
scene.render.resolution_x=900;scene.render.resolution_y=480
camera_data=bpy.data.cameras.new("NW_ReviewCamera")
camera=bpy.data.objects.new("NW_ReviewCamera",camera_data);scene.collection.objects.link(camera)
camera.location=(3,-7,3);camera.rotation_euler=(Vector((0,0,.5))-camera.location).to_track_quat('-Z','Y').to_euler()
camera_data.type='ORTHO';camera_data.ortho_scale=5.5;scene.camera=camera
blend=os.path.join(OUT,"NW_SpinMultiProfile_Tooling.blend")
bpy.ops.wm.save_as_mainfile(filepath=blend,compress=True)
scene.render.filepath=os.path.join(OUT,"NW_SpinMultiProfile_Tooling.png")
bpy.ops.render.render(write_still=True)
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
fbx=os.path.join(OUT,"NW_SpinMultiProfile_Tooling.fbx")
bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'MESH'},bake_anim=False)
report={"blender":bpy.app.version_string,"method":"bmesh.ops.spin",
 "technique":"2π lathe profile, 64 radial steps, source profile in custom property, UV",
 "assets":data,"blend":blend,"fbx":fbx,
 "professional_art_accepted":False,"unreal_import_validated":False}
with open(os.path.join(OUT,"spin_tooling_result.json"),"w") as f:json.dump(report,f,indent=2)
print("NW_SPIN_TOOLING="+json.dumps(report))
