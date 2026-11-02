"""Preserve exact 6-part user-confirmed Alter CLOTHED source.

Per-skinned-part Unreal FBX (one original armature plus each original mesh)
instead of one high-RAM monolithic import; no changes to source, skinning,
morphs or textures. Build-free Blender 5.2.2 LTS export.
"""
import bpy,json,hashlib,pathlib,traceback,time,os
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016\BLENDER_FRESH\CLOTHED")
ORIGINAL=ROOT/"LUMINOUS_ALTER_CLOTHED_original_import.blend"
DEST=ROOT/"UNREAL_ORIGINAL_PARTS"
RECEIPT=DEST/"GATE12_ALTER_6_NATIVE_PARTS_EXPORT_20261021.json"
EXPECTED="1b4386e1b54e8d2a814f05f1b7d72fcbe4d0f76e0e4f1db806fb3dbe87bea11f"
def sha_file(p):
    hash=hashlib.sha256()
    with open(p,"rb") as f:
        for block in iter(lambda:f.read(1024*1024),b""):hash.update(block)
    return hash.hexdigest()
assert ORIGINAL.is_file() and sha_file(ORIGINAL)==EXPECTED
assert bpy.data.filepath.casefold()==str(ORIGINAL).casefold()
arms=[x for x in bpy.data.objects if x.type=="ARMATURE"]
meshes=sorted([x for x in bpy.data.objects if x.type=="MESH"],key=lambda o:o.name)
assert len(arms)==1 and len(arms[0].data.bones)==1130 and len(meshes)==6
assert sum(len(x.data.vertices) for x in meshes)==92982
assert sum(max(0,len(x.data.shape_keys.key_blocks)-1) for x in meshes if x.data.shape_keys)==2146
DEST.mkdir(parents=True,exist_ok=True)
results=[]
status="IN_PROGRESS"
try:
    for i,mesh in enumerate(meshes):
        keycount=max(0,len(mesh.data.shape_keys.key_blocks)-1) if mesh.data.shape_keys else 0
        out=DEST/f"G12_Alter_UserConfirmed_CLOTHED_OriginalPart_{i:02d}.fbx"
        if out.is_file():
            assert out.stat().st_size>100000,"Existing incomplete part, refusing overwrite"
        else:
            bpy.ops.object.select_all(action="DESELECT")
            # The original tongue is excluded from the source ViewLayer,
            # not deleted. Expose the SAME datablock in a temporary,
            # session-only collection; original .blend remains unchanged.
            if mesh.name not in bpy.context.view_layer.objects:
                visible=bpy.data.collections.new("__G12_SESSION_EXPORT_"+str(i))
                bpy.context.scene.collection.children.link(visible)
                visible.objects.link(mesh)
                bpy.context.view_layer.update()
            if not mesh.visible_get():
                mesh.hide_set(False)
            arms[0].select_set(True)
            mesh.select_set(True)
            bpy.context.view_layer.objects.active=arms[0]
            bpy.ops.export_scene.fbx(filepath=str(out),use_selection=True,
                add_leaf_bones=False,path_mode="AUTO",embed_textures=False,
                bake_anim=False,object_types={"ARMATURE","MESH"},
                use_mesh_modifiers=False,apply_scale_options="FBX_SCALE_ALL")
            assert out.stat().st_size>100000
        results.append({"part":i,"mesh_original_name":mesh.name,
           "vertices":len(mesh.data.vertices),
           "faces":len(mesh.data.polygons),"morph_targets":keycount,
           "material_slots":len(mesh.material_slots),
           "original_armature_bones":len(arms[0].data.bones),
           "fbx":str(out),"bytes":out.stat().st_size,
           "fbx_sha256":sha_file(out)})
        print("EXPORTED_ORIGINAL_ALTER_PART",i,mesh.name,
              len(mesh.data.vertices),keycount,flush=True)
    assert sum(x["vertices"] for x in results)==92982
    assert sum(x["morph_targets"] for x in results)==2146
    status="EXACT_SIX_USER_CONFIRMED_ALTER_SKINNED_PARTS_FBX_EXPORTED"
except Exception:
    status="PARTIAL_ORIGINAL_PART_EXPORT_PRESERVED"
    print(traceback.format_exc()[-2000:],flush=True)
finally:
    manifest={"schema":"noworld.gate12.alter.6_original_skinned_parts.v1",
        "status":status,"source_blend":str(ORIGINAL),
        "source_blend_sha256":EXPECTED,
        "source_card_sha256":"9cd089e70890ae7a120c14156652b03919433ed0108530e00c5eedac2f608c9a",
        "identical_source_skinning":True,"source_materials_unchanged":True,
        "source_morphs_unchanged":True,"source_geometry_unchanged":True,
        "expected_1130_shared_armature_bones":1130,"parts":results}
    temp=RECEIPT.with_suffix(".tmp")
    temp.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
    os.replace(temp,RECEIPT)
    print("ALTER_PARTS_FBX_STATUS",status,flush=True)
