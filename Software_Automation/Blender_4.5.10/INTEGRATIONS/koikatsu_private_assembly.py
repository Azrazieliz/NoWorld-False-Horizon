"""PRIVATE, noncanonical read-only reconstruction experiment from six local preexisting GLB shards.
Sources are owned by another active Gate and remain untouched. The assembled .blend is LOCAL ONLY.
Run with Blender 4.5.10 as the connected authorized Windows user.
"""
import bpy,os,json,time,math
from mathutils import Vector
START=time.perf_counter()
SOURCE=r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu\full_fidelity_shards"
OUT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
os.makedirs(OUT,exist_ok=True)
bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene
selected_meshes=[];ignored_meshes=[];rigs=[];per_shard=[]
for i in range(6):
 fp=os.path.join(SOURCE,"FullMesh_%02d.glb"%i)
 assert os.path.isfile(fp),fp
 before=set(bpy.data.objects)
 bpy.ops.import_scene.gltf(filepath=fp)
 new=list(set(bpy.data.objects)-before)
 rig=[o for o in new if o.type=="ARMATURE"]
 assert len(rig)==1,[(o.name,o.type) for o in new]
 rig=rig[0]
 rig.name="NW_KK_SHARD_RIG_%02d"%i
 rigs.append(rig)
 per_shard.append({"source":"FullMesh_%02d.glb"%i,"rig_bones":len(rig.data.bones),
                   "imported_meshes":[],"discarded_debug_meshes":[]})
 for obj in new:
  if obj.type!="MESH":continue
  if len(obj.data.materials)==0 and len(obj.data.vertices)<100:
   ignored_meshes.append(obj)
   per_shard[-1]["discarded_debug_meshes"].append(obj.name)
  else:
   obj.name="NW_SHARD%02d_"%i+obj.name
   obj["nw_original_shard"]=i
   obj["nw_intake_mode"]="READ_ONLY_ASSEMBLY"
   selected_meshes.append(obj)
   per_shard[-1]["imported_meshes"].append(obj.name)
# Measure bind-pose / bone topology equivalence.
ref=rigs[3]
refbones={b.name:b for b in ref.data.bones}
rig_comparisons=[]
for rig in rigs:
 other={b.name:b for b in rig.data.bones}
 same_names=set(refbones)==set(other)
 max_rest=max((sum(abs(x-y) for row_a,row_b in zip(refbones[n].matrix_local,other[n].matrix_local) for x,y in zip(row_a,row_b)) for n in (set(refbones)&set(other))),default=999)
 rig_comparisons.append({"rig":rig.name,"all_121_bone_names_identical":same_names,
                          "max_bone_rest_matrix_L1":round(max_rest,7)})
if any(not x["all_121_bone_names_identical"] or x["max_bone_rest_matrix_L1"]>.0001 for x in rig_comparisons):
 raise RuntimeError("Cannot safely attach all meshes to one rig: per-shard bind skeleton topology/rest-pose differs")
deformer_changes=[]
for ob in selected_meshes:
 prev=[m.object.name for m in ob.modifiers if m.type=="ARMATURE" and m.object]
 # Transfer bone weights via same bone mapping and skeleton, rewire Armature modifiers.
 for m in ob.modifiers:
  if m.type=="ARMATURE" and m.object:
   m.object=ref
 if ob.parent in rigs:
  world=ob.matrix_world.copy()
  ob.parent=ref
  ob.matrix_world=world
 deformer_changes.append({"mesh":ob.name,"previous_armatures":prev,
    "new_parent":ob.parent.name if ob.parent else None,
    "armature_modifiers":[m.object.name if m.object else None for m in ob.modifiers if m.type=="ARMATURE"]})
# Do not erase others' imported rigs until after all mesh deformer pointers are replaced.
for o in ignored_meshes:bpy.data.objects.remove(o,do_unlink=True)
for o in rigs:
 if o!=ref:bpy.data.objects.remove(o,do_unlink=True)
ref.name="NW_KK_INTENDED_SKELETON_121"
# Surface group / provenance hierarchy.
for group in ["NW_HAIR","NW_EYES","NW_BODY","NW_CLOTHING","NW_OTHER"]:
 if group not in bpy.data.collections:
  col=bpy.data.collections.new(group);scene.collection.children.link(col)
for ob in selected_meshes:
 n=ob.name.lower()
 target=("NW_HAIR" if "hair" in n else "NW_CLOTHING" if "outfit" in n else
          "NW_EYES" if any(t in n for t in ("eyes","tear","tongue")) else
          "NW_BODY" if n.endswith("model") else "NW_OTHER")
 dest=bpy.data.collections[target]
 for old in list(ob.users_collection):old.objects.unlink(ob)
 dest.objects.link(ob)
# Count actual imported geometry, rigging and materials. Not a claim of matching original KK shading.
bbox=[]
for ob in selected_meshes:
 for corner in ob.bound_box:
  bbox.append(ob.matrix_world @ Vector(corner))
xyz_min=Vector((min(v[i] for v in bbox) for i in range(3)))
xyz_max=Vector((max(v[i] for v in bbox) for i in range(3)))
size=xyz_max-xyz_min
center=(xyz_min+xyz_max)*.5
h=max(size.z,.5)
# Optional material-color showcase. Shader graphs retained for users to inspect.
cam_data=bpy.data.cameras.new("NW_KK_ReviewCamera")
cam=bpy.data.objects.new("NW_KK_ReviewCamera",cam_data);scene.collection.objects.link(cam)
cam.location=center+Vector((h*.15,-h*2.4,h*.18))
cam.rotation_euler=(center-cam.location).to_track_quat("-Z","Y").to_euler()
cam_data.type="ORTHO";cam_data.ortho_scale=h*1.25;scene.camera=cam
scene.render.engine="BLENDER_WORKBENCH"
scene.display.shading.color_type="MATERIAL";scene.display.shading.light="STUDIO"
scene.display.shading.show_cavity=True
scene.render.resolution_x=700;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
blend=os.path.join(OUT,"NW_KK_ChikaHaruno_AssemblyReview_NONCANONICAL.blend")
preview=os.path.join(OUT,"NW_KK_ChikaHaruno_AssemblyReview_NONCANONICAL.png")
bpy.ops.wm.save_as_mainfile(filepath=blend,compress=True)
scene.render.filepath=preview;bpy.ops.render.render(write_still=True)
report={"blender":bpy.app.version_string,"status":"ASSEMBLED_REVIEW_ONLY",
  "meshes":len(selected_meshes),"rigs_remaining":len([o for o in bpy.data.objects if o.type=="ARMATURE"]),
  "rig_bones":len(ref.data.bones),"vertex_count":sum(len(o.data.vertices) for o in selected_meshes),
  "poly_count":sum(len(o.data.polygons) for o in selected_meshes),
  "uv_missing":[o.name for o in selected_meshes if not o.data.uv_layers],
  "blend_shape_channels_total":sum(max(0,len(o.data.shape_keys.key_blocks)-1) if o.data.shape_keys else 0 for o in selected_meshes),
  "material_slots":sum(len(o.material_slots) for o in selected_meshes),
  "rig_comparisons":rig_comparisons,"per_shard":per_shard,"deformers":deformer_changes,
  "bounds":{ "min":[round(v,5) for v in xyz_min],"max":[round(v,5) for v in xyz_max]},
  "source_files_modified":False,"artistic_match_approved":False,
  "texture_and_shader_parity_approved":False,"unreal_accepted":False,
  "public_git_redistribution":False,
  "review_blend":blend,"preview":preview,"seconds":round(time.perf_counter()-START,3)}
with open(os.path.join(OUT,"private_assembly_review.json"),"w",encoding="utf-8") as fd:json.dump(report,fd,indent=2)
print("NW_PRIVATE_ASSEMBLY="+json.dumps({k:v for k,v in report.items() if k not in ("rig_comparisons","per_shard","deformers")}))
