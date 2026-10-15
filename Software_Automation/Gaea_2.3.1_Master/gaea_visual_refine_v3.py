import bpy,pathlib,hashlib,json
p=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
src=p/"NW_GaeaNative_Erosion_ArtLookdev_V2.blend"
out=p/"NW_GaeaNative_Erosion_ArtLookdev_V3.blend"
preview=p/"NW_GaeaNative_Erosion_ArtLookdev_V3.png"
if out.exists() or preview.exists():raise FileExistsError
h=hashlib.sha256(src.read_bytes()).hexdigest()
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
m=bpy.data.materials["NW_GaeaNative_StoneSoil_RegionalPBR"]
mix=next(n for n in m.node_tree.nodes if n.label=="Material color plus geography")
mix.blend_type="MIX";mix.inputs[0].default_value=.73
sun=bpy.data.objects["NW_AerialWarmSun"]
sun.data.energy=3.1
bpy.context.scene.camera.data.ortho_scale=7200
bpy.context.scene.cycles.samples=16
bpy.context.scene.render.filepath=str(preview)
bpy.ops.wm.save_as_mainfile(filepath=str(out),compress=True)
bpy.ops.render.render(write_still=True)
assert hashlib.sha256(src.read_bytes()).hexdigest()==h
print("NW_GAEA_NATIVE_REFINED_REVIEW="+json.dumps({"preview":str(preview),"editable":str(out),"source_preserved":True}),flush=True)
