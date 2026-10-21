"""Continue the *already saved* locked user-confirmed Alter CLOTHED Blender source.

No reimport/recreation: opens existing hash-verified 191MB Blender source,
exports exact original weighted geometry, 1130-bone armature, shape keys
and referenced material textures to a standalone native Unreal FBX.
Do not mutate existing Gate01 source, Base character or rejected motion.
"""
import bpy,pathlib,hashlib,json,traceback,os,time
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016\BLENDER_FRESH\CLOTHED")
BLEND=ROOT/"LUMINOUS_ALTER_CLOTHED_original_import.blend"
OUTPUT=ROOT/"LUMINOUS_ALTER_CLOTHED_original_native_ue583.fbx"
REPORT=ROOT/"GATE12_ALTER_CLOTHED_UE_FBX_EXPORT_20261021.json"
APPROVED_HASH="1b4386e1b54e8d2a814f05f1b7d72fcbe4d0f76e0e4f1db806fb3dbe87bea11f"
def sha_file(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for block in iter(lambda:f.read(4*1024*1024),b""):h.update(block)
    return h.hexdigest()
receipt={"source_blend":str(BLEND),"source_blend_expected_sha256":APPROVED_HASH,
         "export":str(OUTPUT),"status":"STARTED","baked_source_rebuilt":False,
         "source_blend_modified":False,"native_unreal_imported":False}
try:
    assert BLEND.is_file() and sha_file(BLEND)==APPROVED_HASH,"Frozen native Alter source hash changed"
    assert bpy.data.filepath.casefold()==str(BLEND).casefold(), "Must open the original approved Blender source directly"
    arm=[x for x in bpy.data.objects if x.type=="ARMATURE"]
    mesh=[x for x in bpy.data.objects if x.type=="MESH"]
    assert len(arm)==1 and len(arm[0].data.bones)==1130,(len(arm),len(arm[0].data.bones))
    assert len(mesh)==6 and sum(len(o.data.vertices) for o in mesh)==92982
    receipt["native_armature_bones"]=len(arm[0].data.bones)
    receipt["source_skin_mesh_count"]=len(mesh)
    receipt["native_vertex_count"]=sum(len(o.data.vertices) for o in mesh)
    receipt["source_morph_key_count"]=sum(
        max(0,len(o.data.shape_keys.key_blocks)-1)
        for o in mesh if o.data.shape_keys)
    assert receipt["source_morph_key_count"]==2146,"Frozen Alter facial/body morph count mismatch"
    receipt["material_count"]=len(bpy.data.materials)
    assert receipt["material_count"]==83
    if OUTPUT.is_file():
        assert OUTPUT.stat().st_size>1_000_000,"Existing FBX incomplete; preserve for review"
        receipt["status"]="PRESERVED_ALREADY_EXPORTED_SOURCE_NATIVE_FBX"
    else:
        start=time.monotonic()
        bpy.ops.export_scene.fbx(filepath=str(OUTPUT),use_selection=False,
            add_leaf_bones=False,path_mode="AUTO",embed_textures=False,
            bake_anim=False,object_types={"ARMATURE","MESH"},
            use_mesh_modifiers=False,apply_scale_options="FBX_SCALE_ALL")
        assert OUTPUT.is_file() and OUTPUT.stat().st_size>1_000_000
        receipt["status"]="ORIGINAL_USER_CONFIRMED_ALTER_CLOTHED_NATIVE_FBX_EXPORTED"
        receipt["elapsed_seconds"]=round(time.monotonic()-start,2)
    receipt["export_bytes"]=OUTPUT.stat().st_size
    receipt["fbx_sha256"]=sha_file(OUTPUT)
    receipt["no_mesh_decimation"]=True
    receipt["no_shape_key_removal"]=True
except Exception:
    receipt["status"]="FAILED_NATIVE_SOURCE_EXPORT"
    receipt["error"]=traceback.format_exc()[-3400:]
finally:
    REPORT.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf8")
    print("GATE12_ALTER_LOCKED_FBX="+receipt["status"],flush=True)
    if receipt.get("error"):print(receipt["error"][-1000:],flush=True)
