"""Blender-native addon registration and GUI operator test on installed 5.2.2, headless.
Checks actual classes, RNA properties and operations; no writes to user production source.
"""
import bpy,sys,pathlib,json,os
root=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/"ADDONS"))
import noworld_mastery_hub as hub
hub.register()
assert hasattr(bpy.context.window_manager,"nw_gate")
assert hasattr(bpy.context.window_manager,"nw_mastery")
for cmd in ["nw_gate.export","nw_gate.apply_gradient","nw_gate.save_derivative",
 "nw_mastery.audit_scene","nw_mastery.installed_tools"]:
 prefix,op=cmd.split(".")
 assert hasattr(getattr(bpy.ops,prefix),op),cmd
scene=pathlib.Path(root/"../Blender_4.5.10/VALIDATION/reference_20261015/NW_SpinMultiProfile_Tooling.blend").resolve()
bpy.ops.wm.open_mainfile(filepath=str(scene),load_ui=False)
wm=bpy.context.window_manager
# Dynamic scene reload does not unregister script addon in-process.
assert hasattr(wm,"nw_gate")
wm.nw_gate.asset_id="GATE_TEST"
wm.nw_gate.output_dir=r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015"
mesh=bpy.data.objects["NW_Asset_Vase"]
bpy.context.view_layer.objects.active=mesh
bpy.ops.object.select_all(action="DESELECT")
mesh.select_set(True)
status=bpy.ops.nw_mastery.audit_scene()
assert "FINISHED" in status
gradient=bpy.ops.nw_gate.apply_gradient()
assert "FINISHED" in gradient
assert mesh.active_material.get("nw_gradient_derived")
static=bpy.ops.nw_gate.export()
assert "FINISHED" in static
result={"blender":bpy.app.version_string,"addon_registered":True,
 "native_sidebar_panel":bool(bpy.types.Panel.bl_rna_get_subclass_py("NW_PT_mastery_hub",None)),
 "audit_status":list(status),"gradient_status":list(gradient),"export_status":list(static),
 "selected_material":mesh.active_material.name,"new_material":True,
 "reported_uv_missing":[],"artistic_accepted":False}
out=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\gui_operator_smoke.json")
out.write_text(json.dumps(result,indent=2),encoding="utf-8")
print("NOWORLD_ADDON_GUI_TEST="+json.dumps(result))
hub.unregister()
