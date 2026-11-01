"""NoWorld 5-star: real Koikatsu KKBP PMX -> Blender -> skinned GLB parts.
Execute ONLY with installed Blender and real KKBP add-on. Never guess geometry,
duplicate a character Version, generate a stand-in rig or silently drop morphs.
Command: blender -b --factory-startup --python THIS -- job export proof output
"""
import bpy
import sys
import json
import hashlib
import pathlib
import traceback
import os
import addon_utils

job_file,export_file,receipt_file,out_path=[pathlib.Path(x) for x in
    sys.argv[sys.argv.index("--")+1:sys.argv.index("--")+5]]
output=out_path.resolve()
receipt=receipt_file.resolve()
job=json.loads(job_file.read_text("utf-8-sig"))
export=json.loads(export_file.read_text("utf-8-sig"))
if export.get("status")!="EXPORTED_NATIVE_EXACT_ORIGINAL_CARD":
    raise ValueError("No actual original-card Koikatsu source export")
source=pathlib.Path(export["pmx_path"]).resolve(strict=True)
if source.name!="model.pmx":
    raise ValueError("Expected exact KKBP source model.pmx")
def sha(p):
    with pathlib.Path(p).open("rb") as f:
        return hashlib.file_digest(f,"sha256").hexdigest()
if sha(source)!=export["pmx_sha256"]:
    raise ValueError("Original Koikatsu export changed")
card=next(x for x in job["sources"] if x["role"]=="koikatsu_card")
if export["source_card_sha256"]!=card["sha256"]:
    raise ValueError("Koikatsu card provenance mismatch")
output.mkdir(parents=True,exist_ok=True)
if any(output.iterdir()) or receipt.exists():
    raise FileExistsError("Never overwrite original or completed Blender source output")
addons=pathlib.Path(os.environ.get(
    "NOWORLD_KKBP_ADDON_ROOT",
    r"D:\Tools\GameAssetPipeline\KoikatsuParty\addons\KKBP_v9.5.0"
)).resolve()
sys.path[:0]=[str(addons/"unpacked_mmd_tools"),
              str(addons/"unpacked_mmd_tools"/"opencc_dependency"),
              str(addons/"unpacked_importer")]
result={"schema":"noworld.factory.blender.v1","job_id":job["job_id"],
        "original_card_sha256":card["sha256"],
        "pmx_source_sha256":export["pmx_sha256"],"status":"STARTED",
        "parts":[],"original_source_unchanged":True,
        "gameplay_verified":False}
def write():
    receipt.parent.mkdir(parents=True,exist_ok=True)
    receipt.write_text(json.dumps(result,indent=2)+"\n","utf-8")
try:
    for addon in ("mmd_tools","kkbp_importer"):
        addon_utils.enable(addon,default_set=True,persistent=False)
    cfg=bpy.context.scene.kkbp
    cfg.import_dir=str(source.parent)
    cfg.shader_dropdown="E"
    cfg.use_rigify=False
    cfg.sfw_mode=False
    cfg.fix_seams=True
    cfg.use_outline=False
    cfg.separate_clothes=False
    import kkbp_importer.common as common
    common.toggle_console=lambda:None
    bpy.ops.kkbp.kkbpimport("EXEC_DEFAULT")
    rigs=[x for x in bpy.data.objects if x.type=="ARMATURE"]
    meshes=sorted((x for x in bpy.data.objects if x.type=="MESH"),key=lambda x:x.name)
    if len(rigs)!=1 or len(meshes)<1:
        raise RuntimeError("Actual source must supply exactly one armature and real mesh parts")
    scene=output/(job["job_id"]+"_source.blend")
    bpy.ops.wm.save_as_mainfile(filepath=str(scene),check_existing=False,compress=False)
    result["blend"]=str(scene)
    result["blend_sha256"]=sha(scene)
    result["source_bones"]=len(rigs[0].data.bones)
    result["source_meshes"]=len(meshes)
    for idx,mesh in enumerate(meshes):
        if mesh.name not in bpy.context.view_layer.objects:
            collection=bpy.data.collections.new("NoWorldOriginalSource_"+str(idx))
            bpy.context.scene.collection.children.link(collection)
            collection.objects.link(mesh)
            bpy.context.view_layer.update()
        bpy.ops.object.select_all(action="DESELECT")
        rigs[0].hide_set(False)
        mesh.hide_set(False)
        rigs[0].select_set(True)
        mesh.select_set(True)
        bpy.context.view_layer.objects.active=rigs[0]
        out=output/("original_part_%03d.glb"%idx)
        bpy.ops.export_scene.gltf(filepath=str(out),export_format="GLB",
            use_selection=True,export_apply=False,export_skins=True,
            export_morph=True,export_yup=True)
        if not out.is_file() or out.stat().st_size<30000:
            raise RuntimeError("Native source GLB missing: "+str(out))
        morphs=len(mesh.data.shape_keys.key_blocks)-1 if mesh.data.shape_keys else 0
        result["parts"].append({"index":idx,"source_mesh_name":mesh.name,
            "source_vertices":len(mesh.data.vertices),"source_morphs":morphs,
            "path":str(out),"sha256":sha(out)})
        result["status"]="PARTIAL_SOURCE_EXPORT"
        write()
    result["status"]="SOURCE_RIG_MORPHS_SKIN_EXPORTED"
except Exception as exc:
    result["status"]="BLOCKED"
    result["error"]=str(exc)
    result["trace"]=traceback.format_exc()[-4000:]
    write()
    raise
finally:
    write()
