"""Preserve and join source-authored Gate02 R1 glTF modules; no replacements.
The original protected GLBs remain immutable. Creates editable .blend + FBX
for real UE StaticMesh import with consistent local origins/retained materials."""
import bpy,sys,json,hashlib,traceback
from pathlib import Path
args=sys.argv[sys.argv.index("--")+1:]
name=args[0] if args else "shrub"
kind="anchors" if name in {"royal_citadel","eastern_market_town","southern_farmland_windmill","watchwood_hamlet","regional_roads"} else "modules"
root=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019")
out=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Readiness\FirstWorldR1_MergedSource")
out.mkdir(parents=True,exist_ok=True)
source=root/kind/(name+".glb")
manifest=out/(name+"_conversion.json")
status={"source":str(source),"source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),"source_bytes":source.stat().st_size,"status":"STARTED"}
manifest.write_text(json.dumps(status,indent=2))
try:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    allmesh=[x for x in bpy.data.objects if x.type=="MESH"]
    if not allmesh:raise RuntimeError("No real mesh imported")
    status["input_mesh_objects"]=len(allmesh)
    bpy.ops.object.select_all(action="DESELECT")
    for ob in allmesh:ob.select_set(True)
    bpy.context.view_layer.objects.active=allmesh[0]
    bpy.ops.object.join()
    merged=bpy.context.view_layer.objects.active
    merged.name="Gate02_R1_Source_"+name
    # glTF anchor objects are LOCAL to their metadata center. Join keeps each
    # local node transform; UE should spawn at catalog center * 100cm.
    status["merged_verts"]=len(merged.data.vertices)
    status["merged_polys"]=len(merged.data.polygons)
    status["material_slots"]=len(merged.material_slots)
    blend=out/(name+"_EDITABLE_JOINED.blend")
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    fbx=out/(name+"_NativeUE_Source.fbx")
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,
        object_types={"MESH"},apply_unit_scale=True,axis_forward="-Y",
        axis_up="Z",add_leaf_bones=False,bake_anim=False)
    status.update(status="ACTUAL_GLTF_GEOMETRY_CONVERTED_NOT_UE_VERIFIED",
        blender_source=str(blend),fbx=str(fbx),
        fbx_sha256=hashlib.sha256(fbx.read_bytes()).hexdigest(),
        fbx_bytes=fbx.stat().st_size)
except Exception as ex:
    status.update(status="FAILED",error=str(ex),trace=traceback.format_exc()[-2500:])
finally:
    manifest.write_text(json.dumps(status,indent=2))
    print("GATE02_GEOMETRY_CONVERSION",json.dumps(status))
