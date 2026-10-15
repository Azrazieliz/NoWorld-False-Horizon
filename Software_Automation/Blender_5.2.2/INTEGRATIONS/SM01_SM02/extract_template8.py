"""SM01 known-good native eight-bone wing template -> private Blender topology contract.
READS existing private source archive only. No Koikatsu deployment or mutation.
"""
import pathlib,json,sys,zipfile,hashlib,math
SM01=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.3.0_FULL_MAP")
sys.path.insert(0,str(SM01/"AUTOMATION"/"vendor"))
import UnityPy
from UnityPy.helpers.MeshHelper import MeshHandler
src=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\mods\Sideloader Modpack\[HeldMesin]Wings v1.0.zipmod")
member="abdata/chara/hld/accessory_body_wing01_a.unity3d"
out=pathlib.Path(sys.argv[1]).resolve()
if out.exists():raise FileExistsError(out)
with zipfile.ZipFile(src) as arc:
 data=arc.read(member)
env=UnityPy.load(data)
meshes=[obj.read() for obj in env.objects if obj.type.name=="Mesh"]
assert len(meshes)==1, len(meshes)
mesh=meshes[0]
h=MeshHandler(mesh);h.process()
assert len(mesh.m_BindPose)==8 and len(mesh.m_Skin)==536
def mat4(p):
 # Unity raw serialized Matrix4x4 fields e00..e33
 return [float(getattr(p,"e%d%d"%(r,c))) for r in range(4) for c in range(4)]
skin=[]
for sk in mesh.m_Skin:
 vals=[(int(getattr(sk,"boneIndex_%d_"%k)),float(getattr(sk,"weight_%d_"%k))) for k in range(4)]
 total=sum(w for i,w in vals)
 assert abs(total-1)<.075 and all(0<=i<8 for i,w in vals if w>0)
 skin.append({"indices":[i for i,w in vals],"weights":[w/total for i,w in vals]})
bones=[]
# The mesh template authority is by index/bind matrix, not invented names.
for i,b in enumerate(mesh.m_BindPose):
 bones.append({"index":i,"slot_name":"KK_TEMPLATE_BIND_%02d"%i,
  "inverse_bindpose_row_major":mat4(b)})
report={"schema":"NW_KK_NATIVE_8BONE_TEMPLATE_V1",
 "original_zipmod_sha256":hashlib.sha256(src.read_bytes()).hexdigest(),
 "original_member_sha256":hashlib.sha256(data).hexdigest(),
 "source_member":member,"source_archive":str(src),
 "source_vertex_count":len(h.m_Vertices),
 "source_positions":[list(map(float,p)) for p in h.m_Vertices],
 "source_skin":skin,
 "template_bones":bones,"template_bone_count":8,
 "source_renderer_index_contract":"Unity5.6 SkinnedMeshRenderer preserved as-is",
 "source_preserved":True,
 "target_tier":"KK_PRIVATE_SKINNED_MOD",
 "licence":"PRIVATE ONLY - THIRD PARTY TEMPLATE. NO PUBLIC REDISTRIBUTION"}
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,separators=(",",":")),encoding="utf-8")
print("NW_TEMPLATE_8BONE="+json.dumps({"vertices":len(skin),"bones":len(bones),"path":str(out)}))
