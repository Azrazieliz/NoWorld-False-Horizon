"""Pack SM02's explicit per-vertex Blender skin weights using SM01's KNOWN VALID
8-bone Unity5.6 SkinnedMesh prefab. Original Steam archive read-only. Emits PRIVATE
zipmod into versioned job; never includes third-party bundle in public Git.
"""
import pathlib,sys,json,zipfile,hashlib,struct,math,copy,xml.etree.ElementTree as ET,time
work=pathlib.Path(sys.argv[1]).resolve()
SM01=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.3.0_FULL_MAP")
sys.path.insert(0,str(SM01/"AUTOMATION"/"vendor"))
import UnityPy
from UnityPy.helpers.MeshHelper import MeshHandler
source=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\mods\Sideloader Modpack\[HeldMesin]Wings v1.0.zipmod")
geo_path=work/"NW_Wing_ExplicitGeometry8.json"
out=work/"NW_SM01_SM02_8Bone_Wing_Private_v12.zipmod"
receipt=work/"NW_Wing_UnityFS_OfflineValidation.json"
assert not out.exists() and not receipt.exists()
sha=lambda p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
originalsha=sha(source)
geo=json.loads(geo_path.read_text(encoding="utf-8"))
template=json.loads((work/"template8_private.json").read_text(encoding="utf-8"))
assert geo["source_member_sha256"]==template["original_member_sha256"]
assert geo["template_sha256"]==sha(work/"template8_private.json")
pos=geo["positions"];nrm=geo["normals"];uv=geo["uv0"]
skin=geo["influences"];faces=geo["triangles"]
n=len(pos);indices=[i for tri in faces for i in tri]
assert n==len(nrm)==len(uv)==len(skin)==280
assert len(indices)==448*3 and n<65535
assert all(len(p)==3 and all(math.isfinite(x) for x in p) for p in pos)
assert all(len(p)==2 and all(0<=x<=1 for x in p) for p in uv)
assert all(0<=i<n for i in indices)
with zipfile.ZipFile(source) as archive:
    content=archive.read("abdata/chara/hld/accessory_body_wing01_a.unity3d")
env=UnityPy.load(content)
checks=[]
for obj in env.objects:
 if obj.type.name!="Mesh":continue
 mesh=obj.read()
 h=MeshHandler(mesh);h.process()
 assert mesh.m_VertexData.m_VertexCount==536
 assert len(mesh.m_BindPose)==8 and len(mesh.m_Skin)==536
 assert [(c.stream,c.offset,c.format,c.dimension) for c in mesh.m_VertexData.m_Channels][0:2]==[(0,0,0,3),(0,12,0,3)]
 assert (mesh.m_VertexData.m_Channels[3].stream,mesh.m_VertexData.m_Channels[3].offset,mesh.m_VertexData.m_Channels[3].format,mesh.m_VertexData.m_Channels[3].dimension)==(1,0,0,2)
 # Explicit Blender-authored weight channel; preserve renderer pointers and exact
 # bindpose matrices of the known SM01 8-bone template.
 vertex_skin=[];positive_count={i:0 for i in range(8)}
 for influences in skin:
  entry=copy.deepcopy(mesh.m_Skin[0])
  ids=influences["indices"];weights=influences["weights"]
  if len(ids)!=4 or len(weights)!=4:raise ValueError("Expected 4 skin slots")
  if abs(sum(weights)-1)>1e-4:raise ValueError("Unnormalized Blender weights")
  for k,(idx,w) in enumerate(zip(ids,weights)):
   if not isinstance(idx,int) or idx<0 or idx>7 or w<0:
    raise ValueError("Invalid skin bone palette index")
   setattr(entry,"boneIndex_%d_"%k,idx)
   setattr(entry,"weight_%d_"%k,w)
   if w>.005:positive_count[idx]+=1
  vertex_skin.append(entry)
 # Template bindpose exact order must remain intact, not re-exported through FBX.
 def matrix(p):
  return [float(getattr(p,"e%d%d"%(r,c))) for r in range(4) for c in range(4)]
 for i,m in enumerate(mesh.m_BindPose):
  template_mat=geo["bindposes_4x4"][i]
  if max(abs(a-b) for a,b in zip(template_mat,matrix(m)))>1e-7:
   raise ValueError("KK template bindpose drift "+str(i))
 raw0=b"".join(struct.pack("<ffffff",*(tuple(p)+tuple(nn))) for p,nn in zip(pos,nrm))
 raw1=b"".join(struct.pack("<ff",*v) for v in uv)
 assert len(raw0)==n*24 and len(raw1)==n*8
 mesh.m_VertexData.m_VertexCount=n
 mesh.m_VertexData.m_DataSize=raw0+raw1
 mesh.m_Skin=vertex_skin
 mesh.m_IndexBuffer=list(struct.pack("<%dH"%len(indices),*indices))
 sub=mesh.m_SubMeshes[0]
 sub.firstByte=0;sub.firstVertex=0;sub.indexCount=len(indices);sub.vertexCount=n
 centers=[(min(v[k] for v in pos)+max(v[k] for v in pos))/2 for k in range(3)]
 extents=[(max(v[k] for v in pos)-min(v[k] for v in pos))/2+.03 for k in range(3)]
 for bounds in (mesh.m_LocalAABB,sub.localAABB):
  for key,value in zip(("x","y","z"),centers):setattr(bounds.m_Center,key,value)
  for key,value in zip(("x","y","z"),extents):setattr(bounds.m_Extent,key,value)
 mesh.save()
 checks.append({"vertex_count":n,"triangles":len(faces),"bindposes":8,
                "normalized_weights":True,"bone_palette_usage":positive_count})
assert len(checks)==1,checks
encoded=env.file.save(packer="original")
verify=UnityPy.load(encoded)
parsed=[]
for obj in verify.objects:
 if obj.type.name!="Mesh":continue
 mm=obj.read()
 assert mm.m_VertexData.m_VertexCount==n
 assert len(mm.m_Skin)==n and len(mm.m_BindPose)==8
 hh=MeshHandler(mm);hh.process()
 assert len(hh.m_BoneWeights or [])==n
 assert mm.m_SubMeshes[0].indexCount==len(indices)
 for i in (0,n//2,n-1):
  original_influence=skin[i]
  actual=mm.m_Skin[i]
  for k in range(4):
   assert getattr(actual,"boneIndex_%d_"%k)==original_influence["indices"][k]
   assert abs(getattr(actual,"weight_%d_"%k)-original_influence["weights"][k])<1e-5
 parsed.append({"mesh_name":mm.m_Name,"vertex_count":n,"native_weights_readable":len(hh.m_BoneWeights)})
assert len(parsed)==1,parsed
guid="noworld.sm01sm02.interop.wing.8bone.v12"
itemid=97521
asset_member="abdata/chara/sm01sm02/wing_interop_8bone_v12.unity3d"
manifest=ET.Element("manifest",{"schema-ver":"1"})
for key,value in [("guid",guid),("name","SM01 SM02 Native Wing Interop Fixture"),("version","1.2.0"),
 ("author","SM01 SM02 private QA"),("description","PRIVATE QA - third-party template prefab, no redistribution")]:
 ET.SubElement(manifest,key).text=value
csv="\r\n".join(["125,,,,,,,,,","0,,,,,,,,,","0,,,,,,,,,",
 "ID,Kind,Possess,Name,MainManifest,MainAB,MainData,Parent,ThumbAB,ThumbTex",
 f"{itemid},0,1,SM02 Editable Wing Native Skin,abdata,chara/sm01sm02/wing_interop_8bone_v12.unity3d,accessory_body_wing01_a,a_n_back,chara/sm01sm02/wing_interop_8bone_v12.unity3d,accessory_body_wing01_a_thumb"])+"\r\n"
with zipfile.ZipFile(out,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
 archive.writestr("manifest.xml",ET.tostring(manifest,encoding="utf-8",xml_declaration=True))
 archive.writestr("abdata/list/characustom/sm01sm02/wing_interop_8bone_v12.csv",csv)
 archive.writestr(asset_member,encoded)
 archive.writestr("PRIVATE_ONLY.txt","Private SM01-SM02 validation fixture using proprietary compatible template; do not distribute.")
with zipfile.ZipFile(out) as check:
 assert check.testzip() is None and check.read(asset_member)==encoded
assert sha(source)==originalsha,"ORIGINAL STEAM SOURCE CHANGED"
record={"schema":"NW_SM01_SM02_OFFLINE_WING_ACCEPTANCE_V1_2",
 "status":"OFFLINE_NATIVE_PACKER_PASS_NOT_MAKER_TESTED",
 "source_original_sha256":originalsha,"source_archive_unchanged":True,
 "blender_geometry_sha256":sha(geo_path),
 "native_zipmod_sha256":sha(out),"zipmod":str(out),"guid":guid,
 "item_id":itemid,"category":125,"parent_bone":"a_n_back",
 "components":checks,"reopen":parsed,"material_native_reused":True,
 "blender_explicit_weights_preserved":True,
 "native_maker_loaded":False,"native_two_pose_verified":False,
 "native_card_persistence":False,"licence":"PRIVATE_TEMPLATE_NONREDISTRIBUTABLE"}
receipt.write_text(json.dumps(record,indent=2),encoding="utf-8")
print("NW_SM01_SM02_OFFLINE="+json.dumps({"status":record["status"],
 "zipmod":str(out),"sha256":sha(out),"num_vertices":n,"template_bones":8,
 "source_original_untouched":True}))
