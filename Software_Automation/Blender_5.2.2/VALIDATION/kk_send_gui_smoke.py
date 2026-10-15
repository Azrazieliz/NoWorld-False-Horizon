import bpy,pathlib,sys,json,hashlib
root=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/"ADDONS"))
import noworld_mastery_hub
source=pathlib.Path(r"D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_4.5.10\VALIDATION\reference_20261015\NW_SpinMultiProfile_Tooling.blend")
hash_before=hashlib.sha256(source.read_bytes()).hexdigest()
noworld_mastery_hub.register()
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
o=bpy.data.objects["NW_Asset_Baluster"]
bpy.ops.object.select_all(action="DESELECT");o.select_set(True)
bpy.context.view_layer.objects.active=o
cfg=bpy.context.window_manager.nw_gate
cfg.asset_id="Baluster_Studio_AUTO"
out=bpy.ops.nw_gate.send_kk()
assert "FINISHED" in out
after=hashlib.sha256(source.read_bytes()).hexdigest()
assert hash_before==after
inbox=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\BepInEx\NoWorldBridge\inbox")
ready=list(inbox.glob("NW_GUI_Baluster_Studio_AUTO_*.ready"))
assert len(ready)==1,(len(ready),[p.name for p in ready])
report={"status":"BLENDER_GUI_OPERATOR_PASS","operator":"bpy.ops.nw_gate.send_kk",
 "blender":bpy.app.version_string,"source_unchanged":True,
 "import_job":ready[0].stem,"queued_request":str(ready[0]),
 "needs_koikatsu_runtime_receipt":True}
p=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\blender_gui_kk_enqueue.json")
p.write_text(json.dumps(report,indent=2),encoding="utf-8")
print("NW_KK_GUI_AUTO_SEND="+json.dumps(report))
noworld_mastery_hub.unregister()
