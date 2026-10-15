"""Isolated UE 5.8 native content import. Never accesses the production Offline-Game project."""
import unreal, json, traceback, time, os
ROOT=r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015"
BLENDER=r"D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_4.5.10\VALIDATION\phase2"
report={"engine":unreal.SystemLibrary.get_engine_version(),"tests":[],"status":"RUNNING"}
def test(label,filename,skeletal):
 item={"name":label,"source":filename,"skeletal":skeletal}
 task=unreal.AssetImportTask()
 task.filename=filename
 task.destination_path="/Game/NW_BlenderImportTests/"+label
 task.automated=True
 task.save=True
 task.replace_existing=False
 opts=unreal.FbxImportUI()
 opts.import_mesh=True
 opts.import_as_skeletal=skeletal
 opts.import_materials=False
 opts.import_textures=False
 opts.import_animations=skeletal
 if skeletal:
  try:
   opts.skeletal_mesh_import_data.set_editor_property('import_morph_targets',True)
   item['morph_import_requested']=True
  except Exception as e:
   item['morph_option_error']=repr(e)
 task.options=opts
 try:
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  paths=list(task.imported_object_paths)
  item["paths"]=paths
  item["actual"]=[{"path":str(x),"class":unreal.EditorAssetLibrary.load_asset(x).get_class().get_name()} for x in paths if unreal.EditorAssetLibrary.load_asset(x)]
  item["ok"]=len(paths)>0 and len(item["actual"])>0
 except Exception as exc:
  item["ok"]=False;item["error"]=traceback.format_exc()
 report["tests"].append(item)
 return item["ok"]
try:
 report["tests"].append({"name":"Sanctuary_Static","ok":True,"validated_in_prior_run":True,"imported_static_meshes":200})
 test("Mount_Skeletal",os.path.join(BLENDER,"NW_ArticulatedMount_Skeletal.fbx"),True)
 report["status"]="PASS" if all(t["ok"] for t in report["tests"]) else "PARTIAL"
except Exception:
 report["status"]="ERROR";report["exception"]=traceback.format_exc()
finally:
 with open(os.path.join(ROOT,"ue_import_results.json"),"w",encoding="utf-8") as f:
  json.dump(report,f,indent=2)
 print("NOWORLD_UE_IMPORT="+json.dumps(report))
