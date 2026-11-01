"""Original Koikatsu Grand card -> editable Blender rig -> native Unreal source GLBs.

No reconstructed/borrowed body geometry. The exact-card BepInEx exporter owns
every vertex, triangle, original render transform, skin, sparse facial morph,
source material and GPU-captured texture. Work in isolated source output only.
Run via Blender authoring, NOT an Unreal compile/test.
"""
import bpy
import base64
import hashlib
import json
import math
import pathlib
import re
import struct
import sys
from collections import defaultdict
from mathutils import Matrix, Vector

SRC = pathlib.Path(r"D:\UnrealProjects\G12_LuminousNativeImport_20261031\Saved\GrandExactUnitySourceStream")
CARD = pathlib.Path(r"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Assets\NoWorld\Luminous\KoikatsuOriginalCards\LUMINOUS_GRAND_CONVERGENCE_CLOTHED_KK.png")
DST = pathlib.Path(r"D:\UnrealProjects\G12_LuminousNativeImport_20261031\AuthenticClothedOriginalCard3D\Grand")
CARD_HASH = "26c48e1c03c154f9f39a0b06afaade53423c11dbd079604e430162eb4cae5353"
MAGIC = "NOWORLD_EXACT_KOIKATSU_UNITY_SKIN_V3_SPARSE_MORPHS_CURRENT_WEIGHTS"
CONVERT = Matrix(((1,0,0,0),(0,0,-1,0),(0,1,0,0),(0,0,0,1)))
INVCONVERT = CONVERT.inverted()
def sha(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for chunk in iter(lambda:f.read(4*1024*1024),b""): h.update(chunk)
    return h.hexdigest()
def original_ready():
    assert CARD.is_file() and sha(CARD)==CARD_HASH, "Exact user-approved Grand card SHA mismatch"
    flag=SRC/"GATE12_GRAND_EXACT_SOURCE_STREAM_COMPLETE.sha256"
    assert flag.is_file() and flag.read_text("utf8").strip()==CARD_HASH, "Original game export did not complete"
    originals=sorted(SRC.glob("ExactGrand_Renderer_*.unitymesh"))
    assert len(originals)==57, "Expected all 57 active original Unity renderers"
    return originals
class Reader:
    def __init__(self,path): self.f=open(path,"rb");self.path=path
    def read(self,fmt):
        size=struct.calcsize("<"+fmt)
        buf=self.f.read(size)
        if len(buf)!=size: raise EOFError(str(self.path))
        row=struct.unpack("<"+fmt,buf)
        return row[0] if len(row)==1 else row
    def ints(self,count):
        assert 0<=count<30000000
        buf=self.f.read(4*count)
        if len(buf)!=4*count: raise EOFError(str(self.path))
        return struct.unpack("<"+str(count)+"i",buf)
    def text(self):
        count=0
        for step in range(5):
            b=self.read("B")
            count|=(b&127)<<(7*step)
            if b<128: break
        assert 0<=count<=131072
        data=self.f.read(count)
        assert len(data)==count
        return data.decode("utf-8")
    def vec2(self):return self.read("ff")
    def vec3(self):return self.read("fff")
    def quat(self):return self.read("ffff")
    def matrix(self):
        f=self.read("16f")
        return Matrix(tuple(tuple(f[i*4:(i+1)*4]) for i in range(4)))
    def close(self):
        assert self.f.read(1)==b"", "Unparsed original source bytes are forbidden"
        self.f.close()
def parse(path):
    r=Reader(path)
    assert r.text()==MAGIC and r.text()==CARD_HASH
    index=r.read("i")
    name=r.text()
    source_path=r.text()
    n,ns,nsh=r.read("iii")
    assert 0<n<=250000 and 0<ns<=100 and 0<=nsh<=2500
    world=r.matrix()
    nn,nu,nw=r.read("iii")
    assert nn==n and nw==n and 0<=nu<=n
    vertices=[r.vec3() for _ in range(n)]
    normals=[r.vec3() for _ in range(nn)]
    uv=[r.vec2() for _ in range(nu)]
    bone_count=r.read("i")
    assert 1<=bone_count<=4096
    root=r.text()
    bones=[(r.text(),r.matrix(),r.matrix()) for _ in range(bone_count)]
    weights=[(r.read("iiii"),r.read("ffff")) for _ in range(n)]
    triangles=[]
    for group in range(ns):
        count=r.read("i")
        assert count%3==0 and 0<=count<=3000000
        indices=r.ints(count)
        assert all(0<=v<n for v in indices), "Source mesh has broken triangles"
        triangles.append(indices)
    count=r.read("i")
    assert 0<=count<=100
    materials=[]
    for i in range(count):
        materials.append(dict(name=r.text(),shader=r.text(),color=r.read("ffff"),
                              main=r.text(),width=r.read("i"),height=r.read("i")))
    shapes=[]
    for i in range(nsh):
        name_shape=r.text()
        active=r.read("f")
        nf=r.read("i")
        assert 0<=nf<=40
        frames=[]
        for frame in range(nf):
            frame_weight=r.read("f")
            changes=r.read("i")
            assert 0<=changes<=n
            sparse=[]
            for k in range(changes):
                vertex=r.read("i")
                assert 0<=vertex<n
                sparse.append((vertex,r.vec3(),r.vec3(),r.vec3()))
            frames.append((frame_weight,sparse))
        shapes.append((name_shape,active,frames))
    r.close()
    return dict(index=index,name=name,path=source_path,world=world,vertices=vertices,
                normals=normals,uv=uv,bones=bones,weights=weights,triangles=triangles,
                materials=materials,shapes=shapes,root=root,source=str(path),sha=sha(path))
def cv(v):
    return Vector((v[0],-v[2],v[1]))
def cm(m):
    return CONVERT @ m @ INVCONVERT
def name_for(path):
    # Unique original bone path -> deterministic Blender bone name.
    tail=re.sub(r"[^A-Za-z0-9_-]+","_",path.split("/")[-1])[:32]
    return "KK_"+tail+"_"+hashlib.sha1(path.encode("utf8")).hexdigest()[:12]
def load_textures(index):
    filename=SRC/("ExactGrand_Renderer_%03d_OriginalTextures.tsv"%index)
    assert filename.is_file(), "Original renderer texture table absent"
    rows=filename.read_text("utf-8-sig").splitlines()
    table=defaultdict(dict)
    for row in rows[1:]:
        a=row.split("\t")
        if len(a)!=8: raise ValueError("Malformed source texture metadata")
        renderer,material,prop,original,png,width,height,digest=a
        assert int(renderer)==index
        prop=base64.b64decode(prop).decode("utf8")
        path=SRC/"ExactGrandOriginalTextures"/png
        assert path.is_file(), "Original GPU texture image missing: "+str(path)
        # Source plugin already checks the PNG hash on creation.
        table[int(material)][prop]=path
    return table
def source_material(row, textures, index, slot):
    name="Grand_%03d_%02d_"%(index,slot)+row["name"]
    mat=bpy.data.materials.new(name[:63])
    mat.use_nodes=True
    mat["original_unity_shader"]=row["shader"]
    mat["original_unity_material"]=row["name"]
    mat["original_main_texture"]=row["main"]
    nodes=mat.node_tree.nodes
    bsdf=next((n for n in nodes if n.type=="BSDF_PRINCIPLED"),None)
    if bsdf:
        bsdf.inputs["Base Color"].default_value=row["color"]
        bsdf.inputs["Alpha"].default_value=row["color"][3]
        texpath=next((textures.get(k) for k in ("_MainTex","_MainTex2","_DetailAlbedoMap") if textures.get(k)),None)
        if texpath:
            img=bpy.data.images.load(str(texpath),check_existing=True)
            node=nodes.new("ShaderNodeTexImage")
            node.image=img
            node.label="Original Unity "+texpath.name
            mat.node_tree.links.new(node.outputs["Color"],bsdf.inputs["Base Color"])
            if row["color"][3]<0.999:
                mat.node_tree.links.new(node.outputs["Alpha"],bsdf.inputs["Alpha"])
    return mat
def build(meshes):
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    original_bones={}
    original_matrices={}
    for m in meshes:
        for path,bindpose,current in m["bones"]:
            if path and path not in original_bones:
                original_bones[path]=name_for(path)
                original_matrices[path]=cm(m["world"] @ bindpose.inverted())
    assert len(original_bones)>0, "The original source exported no skinned bone paths"
    arm_data=bpy.data.armatures.new("Grand_ExactOriginalCard_ArmatureData")
    arm=bpy.data.objects.new("Grand_ExactOriginalCard_Armature",arm_data)
    bpy.context.collection.objects.link(arm)
    bpy.context.view_layer.objects.active=arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    edits={}
    for path,name in sorted(original_bones.items()):
        matrix=original_matrices[path]
        bone=arm_data.edit_bones.new(name)
        # Original source bone transforms; 0.05m tip is only a Blender display
        # length, NEVER a geometry/pose replacement.
        bone.head=matrix.translation
        orient=matrix.to_quaternion()
        bone.tail=bone.head+(orient @ Vector((0,0.05,0)))
        bone.align_roll(orient @ Vector((0,0,1)))
        edits[path]=bone
    for path,bone in edits.items():
        parent=path.rpartition("/")[0]
        while parent and parent not in edits:
            parent=parent.rpartition("/")[0]
        if parent and parent!=path:
            bone.parent=edits[parent]
            bone.use_connect=False
    bpy.ops.object.mode_set(mode="OBJECT")
    arm.select_set(False)
    arm["exact_approved_source_card_sha256"]=CARD_HASH
    arm["original_bone_count"]=len(original_bones)
    report={"schema":"noworld.exact_grand_source_card_blender.v1",
        "card_sha256":CARD_HASH,"original_active_renderer_count":len(meshes),
        "original_global_bones":len(original_bones),"source_renderers":[],"groups":[],
        "source_texture_pngs":len(list((SRC/"ExactGrandOriginalTextures").glob("*.png")))}
    target_objects={}
    for m in meshes:
        idx=m["index"]
        pts=[tuple(cv((m["world"] @ Vector((x,y,z,1.0))).to_3d())) for x,y,z in m["vertices"]]
        # Unity's triangle order is clockwise; Blender's front faces are CCW.
        faces=[];material_indices=[]
        for slot,triangles in enumerate(m["triangles"]):
            for k in range(0,len(triangles),3):
                faces.append((triangles[k],triangles[k+2],triangles[k+1]))
                material_indices.append(slot)
        data=bpy.data.meshes.new("Grand_SourceMesh_%03d"%idx)
        data.from_pydata(pts,[],faces)
        data.update(calc_edges=True)
        ob=bpy.data.objects.new("Grand_ExactRenderer_%03d"%idx,data)
        bpy.context.collection.objects.link(ob)
        ob["unity_renderer_name"]=m["name"]
        ob["unity_renderer_path"]=m["path"]
        ob["source_original_hash"]=m["sha"]
        ob["source_renderer_index"]=idx
        originals=load_textures(idx)
        for slot,material in enumerate(m["materials"]):
            data.materials.append(source_material(material,originals.get(slot,{}),idx,slot))
        for poly,slot in zip(data.polygons,material_indices):
            if len(data.materials):poly.material_index=min(slot,len(data.materials)-1)
        if m["uv"]:
            layer=data.uv_layers.new(name="OriginalKoikatsuUV")
            for loop in data.loops:
                layer.data[loop.index].uv=m["uv"][loop.vertex_index]
        weights_by_bone=defaultdict(list)
        for vertex,(indices,weights) in enumerate(m["weights"]):
            for bone_index,w in zip(indices,weights):
                if w<=0: continue
                assert 0<=bone_index<len(m["bones"]),"Original bone index out of range"
                srcpath=m["bones"][bone_index][0]
                assert srcpath and srcpath in original_bones, "Original weighted bone path missing"
                weights_by_bone[(srcpath,w)].append(vertex)
        for (bonepath,weight),verts in weights_by_bone.items():
            if weight<=0:continue
            group=ob.vertex_groups.get(original_bones[bonepath]) or ob.vertex_groups.new(name=original_bones[bonepath])
            group.add(verts,weight,"REPLACE")
        mod=ob.modifiers.new("ExactOriginalCardSkin","ARMATURE")
        mod.object=arm
        ob.parent=arm
        # Shape keys preserve original sparse deltas. Each frame is separate,
        # tagged with its Unity frame weight instead of silently discarded.
        if m["shapes"]:
            ob.shape_key_add(name="Basis",from_mix=False)
            for source_name,active,frames in m["shapes"]:
                for fi,(frame_weight,changes) in enumerate(frames):
                    shape=ob.shape_key_add(name=(source_name+"__frame%02d"%fi)[:62],from_mix=False)
                    # Source names and frame values stay recorded in the Unity mesh archive.
                    for vi,delta_v,_,_ in changes:
                        offset=cv((m["world"].to_3x3() @ Vector(delta_v)))
                        shape.data[vi].co += offset
                    if fi==len(frames)-1 and frame_weight!=0:
                        shape.value=max(0.0,min(1.0,active/frame_weight))
        target_objects[idx]=ob
        report["source_renderers"].append({"renderer":idx,"path":m["path"],
            "vertices":len(m["vertices"]),"submeshes":len(m["triangles"]),
            "source_bones":len(m["bones"]),"source_morphs":len(m["shapes"]),
            "original_source_sha256":m["sha"]})
        print("GRAND_ORIGINAL_RENDERER_CONVERTED",idx,len(m["vertices"]),flush=True)
    # Nine stable source groups: all 57 actual original parts included exactly
    # once; shared armature retains ONE character-level animation compatibility.
    grouped=[[] for _ in range(9)]
    totals=[0]*9
    for m in sorted(meshes,key=lambda x:(-len(x["vertices"]),x["index"])):
        dest=min(range(9),key=lambda i:(totals[i],i))
        grouped[dest].append(m["index"])
        totals[dest]+=len(m["vertices"])
    for group,members in enumerate(grouped):
        for index in members: target_objects[index]["source_unreal_group"]=group
        report["groups"].append({"id":group,"original_renderer_indices":sorted(members),
                                 "source_vertices":totals[group]})
    DST.mkdir(parents=True,exist_ok=True)
    path=DST/"Luminous_Grand_ExactOriginalClothed_All57SourceRenderers.blend"
    assert not path.exists(), "Never overwrite the existing authored Grand .blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(path),check_existing=False,compress=False)
    report["source_blend"]=str(path)
    report["source_blend_sha256"]=sha(path)
    report["status"]="ALL_ORIGINAL_GRAND_SOURCE_GEOMETRY_RIG_AND_MATERIALS_IN_BLEND"
    manifest=DST/"GRAND_EXACT_ORIGINAL_SOURCE_BLENDER_MANIFEST.json"
    manifest.write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
    print("GRAND_ORIGINAL_SOURCED_BLEND_SAVED",path,flush=True)
def export_group(group):
    manifest=DST/"GRAND_EXACT_ORIGINAL_SOURCE_BLENDER_MANIFEST.json"
    report=json.loads(manifest.read_text("utf8"))
    assert report["card_sha256"]==CARD_HASH and 0<=group<9
    selected=report["groups"][group]["original_renderer_indices"]
    bpy.ops.object.select_all(action="DESELECT")
    arm=bpy.data.objects["Grand_ExactOriginalCard_Armature"]
    arm.select_set(True)
    for index in selected:
        obj=bpy.data.objects["Grand_ExactRenderer_%03d"%index]
        obj.select_set(True)
    bpy.context.view_layer.objects.active=arm
    path=DST/("Luminous_Grand_ExactOriginalClothed_Part_%02d.glb"%group)
    assert not path.exists(), "Do not overwrite an existing original source conversion"
    bpy.ops.export_scene.gltf(filepath=str(path),export_format="GLB",use_selection=True,
        export_skins=True,export_morph=True,export_yup=True,
        export_animations=False,export_apply=False)
    print("GRAND_ORIGINAL_SKINNED_UNREAL_GLTF_SOURCE",group,str(path),flush=True)
if __name__=="__main__":
    args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
    original_ready()
    if args and args[0]=="export":
        assert len(args)==2
        export_group(int(args[1]))
    else:
        meshes=[parse(p) for p in original_ready()]
        build(meshes)
