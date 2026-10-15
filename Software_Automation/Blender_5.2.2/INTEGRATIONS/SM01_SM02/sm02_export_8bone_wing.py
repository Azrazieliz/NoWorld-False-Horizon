"""One known-wing Blender derivative -> explicit native 8-bone weights and UV JSON.
Never edits a canonical character or SM01's original Generic Kit. Eight-bone index
and rest pose bytes come from known compatible Unity mesh rather than invented bones.
"""
import bpy,json,sys,pathlib,math,hashlib
from mathutils import Vector,Matrix
src=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\MODDING\GENERIC_KIT\SM01_GENERIC_MODULAR_KIT.blend")
workspace=pathlib.Path(sys.argv[sys.argv.index("--")+1]).resolve()
template=json.loads((workspace/"template8_private.json").read_text(encoding="utf-8"))
blend=workspace/"NW_Wing_Blender_8boneEditable_PRIVATE.blend"
geo=workspace/"NW_Wing_ExplicitGeometry8.json"
assert not blend.exists() and not geo.exists()
sourcehash=hashlib.sha256(src.read_bytes()).hexdigest()
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
orig=[o for o in bpy.data.objects if o.type=="MESH" and (o.name.startswith("SM01_Feather_") or o.name.startswith("SM01_BatMembrane_"))]
assert len(orig)>=45,len(orig)
# One purposefully asymmetric feather tip with an editable original-relative shape key.
target=bpy.data.objects["SM01_Feather_L_0_01"]
if not target.data.shape_keys:target.shape_key_add(name="Basis")
key=target.shape_key_add(name="NW_ArtDirectedFeatherTip_Test",from_mix=False)
center=sum((v.co for v in target.data.vertices),Vector((0,0,0)))/len(target.data.vertices)
tip_i=max(range(len(target.data.vertices)),key=lambda i:(target.data.vertices[i].co-center).length)
tip=target.data.vertices[tip_i].co
tip_delta=(tip-center).normalized()*.145
key.data[tip_i].co+=tip_delta
key.value=1
# Axes: Blender (x,y,z) -> Unity local (0.74*x, .26+.74*(z-1.42),-.20+.60*(y+.13)).
def to_unity(p):
 return (.74*p.x, .26+.74*(p.z-1.42), -.20+.60*(p.y+.13))
def unity_normal(v):
 n=Vector((v.x/.74,v.z/.74,v.y/.60))
 return tuple(n.normalized())
points_old=[Vector(x) for x in template["source_positions"]]
lo_old=[min(p[k] for p in points_old) for k in range(3)]
hi_old=[max(p[k] for p in points_old) for k in range(3)]
# Use the known-valid output geometry bounds for nearest-template similarity rather
# than naively comparing very different coordinate ranges.
baseline_path=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\MODDING\GENERIC_KIT\SM01_INDEPENDENT_WING_GEOMETRY_V4_POSITIONED.json")
baseline=json.loads(baseline_path.read_text(encoding="utf-8-sig"))
bounds=[(min(p[k] for p in baseline["vertices"]),max(p[k] for p in baseline["vertices"])) for k in range(3)]
def nearest_weight(p):
 q=[]
 for j in range(3):
  a,b=bounds[j]
  q.append(lo_old[j]+(.5 if b==a else (p[j]-a)/(b-a))*(hi_old[j]-lo_old[j]))
 nearest=min(range(len(points_old)),key=lambda i:sum((points_old[i][k]-q[k])**2 for k in range(3)))
 return list(template["source_skin"][nearest]["indices"]),list(template["source_skin"][nearest]["weights"])
verts=[];normals=[];uvs=[];faces=[];all_weights=[];sources=[];modified=0
all_materials=[]
for ob in sorted(orig,key=lambda o:o.name):
 ob_eval=ob.evaluated_get(bpy.context.evaluated_depsgraph_get())
 mesh=ob_eval.to_mesh(preserve_all_data_layers=True,depsgraph=bpy.context.evaluated_depsgraph_get())
 try:
  mesh.calc_loop_triangles()
  base=len(verts)
  material=ob.active_material.name if ob.active_material else "SM01_Reused_Unity_Wing_Material"
  sources.append({"name":ob.name,"source_material":material,"vertex_start":base,
    "vertex_count":len(mesh.vertices),"tip":bool(ob==target)})
  all_materials.append(material)
  for i,v in enumerate(mesh.vertices):
   q=ob.matrix_world@v.co
   p=to_unity(q)
   verts.append([float(x) for x in p])
   nr=unity_normal(ob.matrix_world.to_3x3()@v.normal)
   normals.append(list(nr))
   u=max(0.,min(1.,(q.x+2.2)/4.4))
   vv=max(0.,min(1.,(q.z-.65)/1.6))
   if ob==target:
    u=min(1,u+.12)
    modified+=1
   uvs.append([u,vv])
   bone_indices,weights=nearest_weight(p)
   if ob==target and i==tip_i:
    # User-editable direct 8-bone weight change on precisely one feather tip.
    cands=sorted(range(4),key=lambda k:-weights[k])
    a,b=cands[0],cands[1]
    move=min(.10,weights[a]*.3)
    weights[a]-=move;weights[b]+=move
   total=sum(weights)
   weights=[w/total for w in weights]
   assert abs(sum(weights)-1)<1e-5
   assert all(0<=idx<8 for idx in bone_indices)
   all_weights.append({"indices":bone_indices,"weights":weights})
  for tr in mesh.loop_triangles:
   a,b,c=map(int,tr.vertices)
   # Negative handedness determinant of (x,z,y) => inverse winding in native output.
   faces.append([base+a,base+c,base+b])
   faces.append([base+a,base+b,base+c])
 finally:ob_eval.to_mesh_clear()
assert len(verts)==280 and len(faces)==448,(len(verts),len(faces))
# Editable combined weight-bearing mesh in original Blender coordinates for source correction.
# Body/material mesh objects and original shape keys remain untouched in separate collection.
nativeverts=[(p[0]/.74,(p[2]+.20)/.60-.13,(p[1]-.26)/.74+1.42) for p in verts]
native_faces=[tuple(t) for t in faces]
me=bpy.data.meshes.new("NW_KK_8Bone_SkinnedWing_Geo");me.from_pydata(nativeverts,[],native_faces);me.update()
newob=bpy.data.objects.new("NW_KK_8Bone_EditableWing_Export",me)
bpy.context.scene.collection.objects.link(newob)
for i in range(8):newob.vertex_groups.new(name="KK_TEMPLATE_BIND_%02d"%i)
for i,sk in enumerate(all_weights):
 for bi,w in zip(sk["indices"],sk["weights"]):
  if w>0:newob.vertex_groups[bi].add([i],w,"REPLACE")
# Keep source unchanged, add explicit index-bone proxy armature for Blender weight editing.
arm=bpy.data.armatures.new("NW_KK_ExactTemplate8_IndexRig")
rig=bpy.data.objects.new("NW_KK_ExactTemplate8_IndexRig",arm)
bpy.context.scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active=rig
rig.select_set(True);bpy.ops.object.mode_set(mode="EDIT")
for i in range(8):
 b=arm.edit_bones.new("KK_TEMPLATE_BIND_%02d"%i)
 # Proxy bones are only visualization. Actual Unity bind matrices are carried unchanged in JSON.
 b.head=(i%4*.25-.4,0,1+i//4*.12);b.tail=(i%4*.25-.4,0,1.2+i//4*.12)
bpy.ops.object.mode_set(mode="OBJECT")
rig.select_set(False)
mod=newob.modifiers.new("NW_8Bone_DeformationPreview","ARMATURE");mod.object=rig
newob.parent=rig
newob["nw_template_bindposes_authority"]="EXACT_UNITY_8BONE_V1_JSON"
newob["nw_native_tier"]="KK_PRIVATE_SKINNED_MOD"
newob["nw_proxy_armature_is_native"]="FALSE"
# Hide merged duplicate from render; original source geometry remains as the preview.
newob.hide_render=True
geo_data={"schema":"NW_SM02_NATIVE_KK_8BONE_V1_2","target_tier":"KK_PRIVATE_SKINNED_MOD",
 "source_blend":str(src),"source_sha256":sourcehash,
 "template_sha256":hashlib.sha256((workspace/"template8_private.json").read_bytes()).hexdigest(),
 "source_member_sha256":template["original_member_sha256"],
 "blender_version":bpy.app.version_string,
 "transform":{"blender_up":"Z","unity_up":"Y","blender_unit_m":1.0,"unity_unit_m":1.0,
 "matrix_R3_row_major":[.74,0,0, 0,0,.74, 0,.60,0],"offset":[0,.26-.74*1.42,-.20+.60*.13],
 "determinant":-.74*.74*.60,"triangle_winding":"reversed+two_sided"},
 "positions":verts,"normals":normals,"uv0":uvs,"triangles":faces,
 "influences":all_weights,
 "bindposes_4x4":[x["inverse_bindpose_row_major"] for x in template["template_bones"]],
 "bone_names":[x["slot_name"] for x in template["template_bones"]],
 "bone_identity":"PRESERVE_SOURCE_RENDERER_ORDER_AND_POINTERS",
 "material_sections":sources,
 "one_feather_tip_delta":[float(v) for v in tip_delta],
 "one_feather_uv_shift":.12,"one_feather_weight_delta":.10,
 "vertex_count":len(verts),"triangle_count":len(faces),
 "texture_status":"SM01_MaterialEditor_NATIVE_CARD_SOURCED",
 "native_game_tested":False,"artistic_likeness_accepted":False}
geo.write_text(json.dumps(geo_data,separators=(",",":")),encoding="utf-8")
bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
assert hashlib.sha256(src.read_bytes()).hexdigest()==sourcehash
print("NW_SM02_KK_WING_SOURCE="+json.dumps({"editable_blend":str(blend),"geometry_json":str(geo),"vertices":len(verts),"triangles":len(faces),"bone_groups":8,"source_untouched":True}))
