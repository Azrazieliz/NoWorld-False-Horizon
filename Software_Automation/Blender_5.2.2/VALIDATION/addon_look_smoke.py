"""Blender 5.2 addon native operator E2E on 3 KK private hair+skirt slots."""
import bpy,sys,pathlib,json,hashlib
root=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/"ADDONS"))
import noworld_mastery_hub as addon
source=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261015\NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend")
before=hashlib.sha256(source.read_bytes()).hexdigest()
addon.register()
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
cfg=bpy.context.window_manager.nw_gate
cfg.look_preset="Autumn";cfg.shared_anchor=True
cfg.output_dir=r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015"
cfg.asset_id="AUTOMATED_UI_CharacterLook"
out=bpy.ops.nw_gate.auto_look()
assert "FINISHED" in out,out
materials=[m.name for m in bpy.data.materials if m.get("nw_gradient_derived")]
assert len(materials)==3,materials
after=hashlib.sha256(source.read_bytes()).hexdigest()
assert before==after
res={"blender":bpy.app.version_string,"operator_status":list(out),
 "styled_material_count":len(materials),"source_sha256_unchanged":True,
 "look":"Autumn","no_manual_mesh_edits":True}
path=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\addon_look_smoke.json")
path.write_text(json.dumps(res,indent=2))
print("NW_ADDON_AUTO_LOOK_UI_PROOF="+json.dumps(res))
addon.unregister()
