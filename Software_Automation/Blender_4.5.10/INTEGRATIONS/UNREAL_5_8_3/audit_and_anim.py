"""Native UE 5.8.3 audit of isolated imported skeletal asset and separate animation import."""
import unreal, json, os, traceback
ROOT=r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015"
ROOT_B=r"D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_4.5.10\VALIDATION\phase2"
r={"engine":unreal.SystemLibrary.get_engine_version(),"status":"STARTED"}
try:
 with open(os.path.join(ROOT,"ue_import_results.json")) as f:result=json.load(f)
 skeletal=next(t for t in result['tests'] if t['name']=='Mount_Skeletal')
 meshes=[item for item in skeletal["actual"] if item["class"]=="SkeletalMesh"]
 skeletons=[item for item in skeletal["actual"] if item["class"]=="Skeleton"]
 assert len(meshes)==1 and len(skeletons)==1
 ob=unreal.EditorAssetLibrary.load_asset(meshes[0]["path"])
 sk=unreal.EditorAssetLibrary.load_asset(skeletons[0]["path"])
 r["skeletal_asset"]=str(ob.get_path_name())
 r["skeleton_asset"]=str(sk.get_path_name())
 r["native_morphs"]=list(ob.get_all_morph_target_names())
 r["has_facial_morph"]="NW_Facial_Expressive_Squeeze" in r["native_morphs"]
 try:
  sub=unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
  r["native_lod0_vertices"]=sub.get_num_verts(ob,0)
  r["native_lod0_sections"]=sub.get_num_sections(ob,0)
 except Exception as e:r["lod_info_error"]=repr(e)
 # Import original action FBX as a separate AnimSequence, referencing saved Skeleton.
 item=unreal.AssetImportTask()
 item.filename=os.path.join(ROOT_B,"NW_ArticulatedMount_Skeletal.fbx")
 item.destination_path="/Game/NW_BlenderImportTests/Mount_Animation"
 item.automated=True;item.save=True;item.replace_existing=False
 ui=unreal.FbxImportUI()
 ui.import_mesh=False
 ui.import_as_skeletal=True
 ui.import_animations=True
 ui.automated_import_should_detect_type=False
 ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
 ui.skeleton=sk
 item.options=ui
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([item])
 r["animation_import_paths"]=list(item.imported_object_paths)
 r["animation_assets"]=[{"path":str(p),"class":unreal.EditorAssetLibrary.load_asset(p).get_class().get_name()} for p in item.imported_object_paths if unreal.EditorAssetLibrary.load_asset(p)]
 r["has_native_anim_sequence"]=any(x["class"]=="AnimSequence" for x in r["animation_assets"])
 r["status"]="PASS" if r["has_facial_morph"] and r["has_native_anim_sequence"] else "PARTIAL"
except Exception as e:
 r["status"]="ERROR";r["exception"]=traceback.format_exc()
finally:
 with open(os.path.join(ROOT,"ue_native_deep_audit.json"),'w',encoding='utf-8') as f:json.dump(r,f,indent=2)
 print("NOWORLD_UE_DEEP="+json.dumps(r))
