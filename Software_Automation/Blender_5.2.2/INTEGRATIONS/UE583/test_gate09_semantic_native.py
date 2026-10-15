"""Gate09 flight/swim semantic Blender Actions -> Unreal 5.8.3 native AnimSequences.
Sandbox only; neither cosmetic/source nor gameplay creative accepted.
"""
import unreal,os,json,traceback
QA=r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015"
r={"engine":unreal.SystemLibrary.get_engine_version(),"source":"Gate09 Blender 5.2 Action Slots"}
try:
 skeleton=unreal.EditorAssetLibrary.load_asset("/Game/NW_BlenderImportTests/Mount_Skeletal/NW_ArticulatedMount_Skeletal_Skeleton")
 assert skeleton,"Original 16-bone native test skeleton missing"
 rows=[]
 for kind in ("flight","swim"):
  f=os.path.join(QA,"NW_Gate09_"+kind+"_Semantic.fbx")
  task=unreal.AssetImportTask()
  task.filename=f;task.destination_path="/Game/NW_MasterHub_QA/Gate09_Motion"
  task.automated=True;task.replace_existing=False;task.save=True
  opt=unreal.FbxImportUI();opt.import_mesh=False
  opt.import_as_skeletal=True;opt.import_animations=True
  opt.automated_import_should_detect_type=False
  opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
  opt.skeleton=skeleton;task.options=opt
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  paths=[]
  for p in task.imported_object_paths:
   a=unreal.load_asset(p)
   if a:paths.append({"path":str(p),"class":a.get_class().get_name()})
  asset=next((unreal.load_asset(x["path"]) for x in paths if x["class"]=="AnimSequence"),None)
  assert asset,"No AnimSequence imported: "+str(paths)
  row={"kind":kind,"path":asset.get_path_name(),"frames":int(asset.get_editor_property("number_of_sampled_frames")),
       "skeleton":asset.get_editor_property("skeleton").get_path_name(),"type":asset.get_class().get_name()}
  assert row["frames"]>=45
  rows.append(row)
 r["animations"]=rows
 r["status"]="PASS_NATIVE_TWO_ANIMSEQUENCES"
 r["art_quality_accepted"]=False
 r["pose_delta_verified_in_ue"]=False
 r["gameplay_runtime_tested"]=False
except Exception:r["status"]="ERROR";r["error"]=traceback.format_exc()
finally:
 with open(os.path.join(QA,"Gate09_UE583_native_motion.json"),"w",encoding="utf-8") as f:json.dump(r,f,indent=2)
 print("NW_GATE09_UNREAL="+json.dumps({k:v for k,v in r.items() if k!="error"}))
