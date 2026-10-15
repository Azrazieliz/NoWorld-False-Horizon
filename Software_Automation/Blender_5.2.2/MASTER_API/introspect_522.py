"""Blender 5.2.2 fully machine-readable OPERATOR + RNA property introspection.
This is exact installed API evidence, not artistic expertise or UI-position authority.
Includes registered CloudRig, Retarget, Architecture and NoWorld operators.
"""
import bpy,addon_utils,json,pathlib,time,sys
root=pathlib.Path(__file__).resolve().parents[1]
t=time.perf_counter()
for mod in ("bl_ext.blender_org.cloudrig","bl_ext.blender_org.retarget",
            "bl_ext.blender_org.arch_generator","noworld_mastery_hub"):
 try:addon_utils.enable(mod)
 except Exception as e:print("SKIP_ADDON",mod,str(e)[:180])
def info(prop):
 x={"identifier":prop.identifier,"type":prop.type,
    "name":prop.name,"description":prop.description}
 for attr in ("subtype","unit","is_readonly","is_required","default","hard_min","hard_max","soft_min","soft_max","array_length"):
  try:
   v=getattr(prop,attr)
   if isinstance(v,(str,bool,int,float)) or v is None:x[attr]=v
   elif attr=="default":x[attr]=list(v) if hasattr(v,"__iter__") else str(v)
  except Exception:pass
 if prop.type=="ENUM":
  try:x["enum_values"]=[v.identifier for v in prop.enum_items if v.identifier][:90]
  except Exception:pass
 return x
operators=[]
for ns in dir(bpy.ops):
 if ns.startswith("_"):continue
 try:module=getattr(bpy.ops,ns)
 except Exception:continue
 for name in dir(module):
  if name.startswith("_"):continue
  try:
   op=getattr(module,name)
   rna=op.get_rna_type()
   entry={"id":ns+"."+name,
     "description":rna.description,"label":rna.name,
     "props":[info(p) for p in rna.properties if p.identifier!="rna_type"]}
   operators.append(entry)
  except Exception:continue
operators.sort(key=lambda x:x["id"])
type_names=["Object","Mesh","Material","NodeTree","ShaderNodeTree",
 "GeometryNodeTree","Armature","Bone","PoseBone","Action","ActionSlot",
 "FCurve","NlaTrack","NlaStrip","AnimData","Scene","RenderSettings",
 "RigidBodyObject","RigidBodyWorld","ClothModifier",
 "GreasePencilv3","GreasePencilLayer","Camera","Light",
 "Curve","MeshUVLoopLayer","Image","Texture","Collection","World",
 "Modifier","GeometryNodesModifier","ParticleSettings","ObjectConstraints",
 "BoneConstraints","BoneConstraint","IKConstraint","ChildOfConstraint",
 "CopyRotationConstraint","Driver","Key","ShapeKey"]
rna_types={}
for name in type_names:
 obj=getattr(bpy.types,name,None)
 if obj is None or not hasattr(obj,"bl_rna"):continue
 try:
  props=[info(p) for p in obj.bl_rna.properties if p.identifier!="rna_type"]
  rna_types[name]={"properties":props,"count":len(props)}
 except Exception as e:rna_types[name]={"error":repr(e)}
out={"blender":bpy.app.version_string,
 "generated_from_installed_binary":True,
 "operator_count":len(operators),
 "type_count":len(rna_types),"operators":operators,"types":rna_types,
 "elapsed_seconds":round(time.perf_counter()-t,2)}
dest=root/"MASTER_API"/"BLENDER_5_2_RNA_CATALOG.json"
dest.write_text(json.dumps(out,indent=None,separators=(",",":"),ensure_ascii=False),encoding="utf-8")
assert len(operators)>=1500,(len(operators),[x["id"] for x in operators[-20:]])
for required in ("wm.obj_export","mesh.arch_create","nw_gate.auto_look",
                 "mesh.primitive_cube_add","nla.bake"):
 assert any(x["id"]==required for x in operators),required
print("NOWORLD_FULL_522_API_INDEX="+json.dumps({
 "blender":out["blender"],"operators":len(operators),"rna_types":len(rna_types),
 "bytes":dest.stat().st_size,"source":str(dest),"seconds":out["elapsed_seconds"]}))
