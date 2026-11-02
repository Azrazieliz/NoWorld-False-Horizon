"""Split *original* independently approved Alter underwear/nude Blender PMX imports.

Works on one Blender scene at a time, each FBX carries the SAME
per-state original armature (1130 UNDERWEAR, 1122 NUDE). No mesh replacement, decimation, rig reauthoring,
morph deletion, synthetic outfit or modification of canonical .blend.
"""
import bpy,sys,pathlib,json,hashlib,traceback,os
args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
assert len(args)==1 and args[0] in ("UNDERWEAR","NUDE")
STATE=args[0]
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016")/"BLENDER_FRESH"/STATE
SOURCE=ROOT/f"LUMINOUS_ALTER_{STATE}_original_import.blend"
PROOF=json.loads((ROOT/"GATE12_FRESH_BLENDER_CONVERSION_RECEIPT.json").read_text(encoding="utf8"))
assert PROOF["status"]=="PASS_REAL_PRIMARY_PARTY_SOURCE_BLENDER_FBX_GLB_EXPORTED_PENDING_INDEPENDENT_VALIDATION"
assert PROOF["state"]==STATE
detail=next(row["detail"] for row in PROOF["steps"] if row["step"]=="EDITABLE_BLENDER_SOURCE_SAVED")
expected=detail["sha256"]
def sha(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for data in iter(lambda:f.read(1024*1024),b""):h.update(data)
    return h.hexdigest()
assert sha(SOURCE)==expected,"Canonical exact Blender source hash mismatch"
assert pathlib.Path(bpy.data.filepath).resolve()==SOURCE.resolve()
arms=[a for a in bpy.data.objects if a.type=="ARMATURE"]
meshes=sorted([a for a in bpy.data.objects if a.type=="MESH"],key=lambda obj:obj.name)
geometry=PROOF["native_blender_geometry"]
assert len(arms)==1 and len(arms[0].data.bones)==geometry["armature_bones"]
assert len(meshes)==geometry["mesh_count"] and len(meshes)>=3
assert sum(len(m.data.vertices) for m in meshes)==geometry["vertices"]
assert sum(max(0,len(m.data.shape_keys.key_blocks)-1) for m in meshes if m.data.shape_keys)==geometry["shape_keys"]
DEST=ROOT/"UNREAL_ORIGINAL_PARTS"
DEST.mkdir(parents=True,exist_ok=True)
manifest_path=DEST/f"GATE12_ALTER_{STATE}_NATIVE_PARTS_20261022.json"
assert not manifest_path.exists(),"Approved source export inventory is already sealed"
rows=[]
status="STARTED"
try:
    for i,mesh in enumerate(meshes):
        morph_count=max(0,len(mesh.data.shape_keys.key_blocks)-1) if mesh.data.shape_keys else 0
        out=DEST/f"G12_Alter_UserConfirmed_{STATE}_OriginalPart_{i:02d}.fbx"
        if out.exists():
            assert out.stat().st_size>100000,"Preserve existing incomplete original export"
        else:
            bpy.ops.object.select_all(action="DESELECT")
            if mesh.name not in bpy.context.view_layer.objects:
                temp=bpy.data.collections.new("__G12_SESSION_ONLY_"+str(i))
                bpy.context.scene.collection.children.link(temp)
                temp.objects.link(mesh)
                bpy.context.view_layer.update()
            if not mesh.visible_get():mesh.hide_set(False)
            arms[0].select_set(True)
            mesh.select_set(True)
            bpy.context.view_layer.objects.active=arms[0]
            bpy.ops.export_scene.fbx(filepath=str(out),use_selection=True,
                 add_leaf_bones=False,path_mode="AUTO",embed_textures=False,
                 bake_anim=False,object_types={"ARMATURE","MESH"},
                 use_mesh_modifiers=False,apply_scale_options="FBX_SCALE_ALL")
            assert out.stat().st_size>100000
        rows.append({
            "part":i,"mesh_original_name":mesh.name,
            "vertices":len(mesh.data.vertices),"faces":len(mesh.data.polygons),
            "morph_targets":morph_count,"material_slots":len(mesh.material_slots),
            "original_armature_bones":len(arms[0].data.bones),
            "fbx":str(out),"bytes":out.stat().st_size,"fbx_sha256":sha(out)})
        print("G12_APPROVED_ALTER",STATE,"PART",i,"VERTICES",len(mesh.data.vertices),"MORPHS",morph_count,flush=True)
    assert sum(x["vertices"] for x in rows)==geometry["vertices"]
    assert sum(x["morph_targets"] for x in rows)==geometry["shape_keys"]
    status="EXACT_ORIGINAL_APPROVED_ALTER_WARDROBE_SKINNED_PARTS_EXPORTED"
except Exception:
    status="INCOMPLETE_ORIGINAL_NATIVE_PARTS_RETAINED"
    print(traceback.format_exc()[-2300:],flush=True)
finally:
    report={"schema":"noworld.gate12.alter.approved.wardrobe.parts.v1",
        "status":status,"state":STATE,
        "source_blend":str(SOURCE),"source_blend_sha256":expected,
        "source_card_sha256":PROOF["source_card_sha256"],
        "expected_vertices":geometry["vertices"],
        "expected_shape_keys":geometry["shape_keys"],
        "shared_source_armature_bones":geometry["armature_bones"],
        "source_unchanged":True,"base_mesh_substituted":False,
        "parts":rows}
    temp=manifest_path.with_suffix(".tmp")
    temp.write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
    os.replace(temp,manifest_path)
    print("G12_ALTER_NATIVE_PARTS",STATE,status,flush=True)
