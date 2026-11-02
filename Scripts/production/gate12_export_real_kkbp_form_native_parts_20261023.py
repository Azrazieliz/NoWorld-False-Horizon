"""Export real Koikatsu-derived body and wardrobe as independently imported Unreal FBX parts.
Does not overwrite source, transform, decimate, or invent any geometry.
"""
import bpy,sys,pathlib,json,hashlib,traceback,os
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
form=sys.argv[sys.argv.index("--")+1]
assert form in ("Awakened","Berserker","Grand")
srcdir=R/"SourceAssets/Gate12/LuminousForms20261023"/form/"CANDIDATE_NATIVE_FROM_EXISTING_KKBP_PMX"
src=srcdir/(f"Luminous_{form}_SourceAuthentic_BodyRig.blend")
assert bpy.data.filepath.lower()==str(src).lower()
proof=json.loads((srcdir/"GATE12_NATIVE_KKBP_IMPORT_RECEIPT.json").read_text(encoding="utf8"))
assert proof["status"]=="REAL_KKBP_MESH_RIG_MATERIALS_SAVED_UNPAIRED_TO_CARD"
def sha(f):
 h=hashlib.sha256()
 with f.open("rb") as b:
  for chunk in iter(lambda:b.read(4*1024*1024),b""):h.update(chunk)
 return h.hexdigest()
assert sha(src)==proof["blend_sha256"]
arms=[x for x in bpy.data.objects if x.type=="ARMATURE"]
meshes=sorted([x for x in bpy.data.objects if x.type=="MESH"],key=lambda x:x.name)
assert len(arms)==1 and len(meshes)==proof["mesh_parts"]
assert len(arms[0].data.bones)==proof["armature_bones"]
assert sum(len(x.data.vertices) for x in meshes)==proof["vertices"]
dest=srcdir/"UNREAL_REAL_NATIVE_PARTS"
dest.mkdir(parents=True,exist_ok=True)
report_path=dest/"GATE12_FORM_REAL_KKBP_SEGMENTED_FBX_RECEIPT.json"
rows=[]
try:
 for i,mesh in enumerate(meshes):
  path=dest/(f"G12_Luminous_{form}_OriginalPart_{i:02d}.fbx")
  if not path.exists():
   bpy.ops.object.select_all(action="DESELECT")
   if mesh.name not in bpy.context.view_layer.objects:
    c=bpy.data.collections.new(f"G12_TEMP_{i}")
    bpy.context.scene.collection.children.link(c)
    c.objects.link(mesh)
    bpy.context.view_layer.update()
   if not mesh.visible_get():mesh.hide_set(False)
   arms[0].select_set(True)
   mesh.select_set(True)
   bpy.context.view_layer.objects.active=arms[0]
   bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,
     add_leaf_bones=False,path_mode="AUTO",embed_textures=False,
     bake_anim=False,object_types={"ARMATURE","MESH"},
     use_mesh_modifiers=False,apply_scale_options="FBX_SCALE_ALL")
  assert path.stat().st_size>100000
  count=len(mesh.data.shape_keys.key_blocks)-1 if mesh.data.shape_keys else 0
  rows.append({"part":i,"original_mesh":mesh.name,"source_vertices":len(mesh.data.vertices),
    "morphs":count,"material_slots":len(mesh.material_slots),
    "bones":proof["armature_bones"],"fbx":str(path),
    "fbx_bytes":path.stat().st_size,"fbx_sha256":sha(path)})
  print("FORM_KK_NATIVE_PART",form,i,len(mesh.data.vertices),count,flush=True)
 status="SIX_SOURCE_BACKED_NATIVE_FBX_PARTS_READY_FOR_UNREAL"
 assert len(rows)==6
except Exception:
 status="PARTIAL_SOURCE_BACKED_EXPORT"
 print(traceback.format_exc()[-2100:],flush=True)
finally:
 receipt={"status":status,"character_form":form,
  "original_blend_sha256":proof["blend_sha256"],
  "original_export_sha256":proof["source_sha256"],
  "source_card_to_pmx_proven":False,
  "source_unchanged":sha(src)==proof["blend_sha256"],
  "native_part_count":len(rows),"parts":rows}
 report_path.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf8")
 print("REAL_NATIVE_FBX_RESULT",status,flush=True)
