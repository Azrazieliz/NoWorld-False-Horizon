import unreal,json,os,traceback
R=r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu"
out={"status":"STARTED","project":str(unreal.Paths.get_project_file_path()),"checks":[],"api":{}}
def write():
 with open(os.path.join(R,"animation_binding_probe.json"),"w",encoding="utf8") as f:json.dump(out,f,indent=2,default=str)
write()
try:
 an=unreal.load_asset("/Game/Experimental/Gate12Koikatsu/Animation/AN_ChikaHaruno_SpineSway_UE583")
 if not an:raise RuntimeError("No existing AnimSequence")
 seq_skel=an.get_editor_property("skeleton")
 out["animation_skeleton"]=seq_skel.get_path_name()
 for cls in (unreal.SkeletalMeshComponent,unreal.PoseableMeshComponent,unreal.Skeleton,unreal.AnimSequence):
  out["api"][cls.__name__]=[n for n in dir(cls) if any(k in n.lower() for k in ("anim","morph","bone","compatible","skeleton","override"))]
 names=[
  "/Game/Experimental/Gate12Koikatsu/FullPrimitives/Mesh03Prim00/Mesh_03_Prim_00/SkeletalMeshes/Mesh_03_Prim_00",
  "/Game/Experimental/Gate12Koikatsu/FullFidelity/Mesh04/FullMesh_04/SkeletalMeshes/FullMesh_04",
  "/Game/Experimental/Gate12Koikatsu/FullFidelity/Mesh05/FullMesh_05/SkeletalMeshes/FullMesh_05"
 ]
 for path in names:
  mesh=unreal.load_asset(path)
  if not mesh:raise RuntimeError("missing mesh "+path)
  skeleton=mesh.get_editor_property("skeleton")
  check={"mesh":path,"skeleton":skeleton.get_path_name(),"morph_count":len(mesh.get_editor_property("morph_targets"))}
  for item,label in ((seq_skel,"animation_skeleton"),(skeleton,"imported_skeleton")):
   for method in ("is_compatible_for_editor","is_compatible_mesh","is_compatible"):
    try:check[label+"_"+method]=str(getattr(item,method)(skeleton if method!="is_compatible_mesh" else mesh))
    except Exception as e:check[label+"_"+method+"_error"]=str(e)[:190]
  out["checks"].append(check);write()
 out["status"]="PASS"
except Exception as e:out["status"]="ERROR";out["error"]=str(e);out["trace"]=traceback.format_exc()[-1800:]
finally:write();unreal.log("G12_ANIM_BIND_PROBE_"+out["status"])
