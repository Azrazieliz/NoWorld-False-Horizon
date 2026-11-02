import unreal,json,pathlib
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
o={"status":"STARTED","classes":{},"try_set":{}}
try:
 path="/Game/Experimental/Gate12Koikatsu/FullPrimitives/Mesh03Prim00/Mesh_03_Prim_00/Materials/KK_cf_m_face_00"
 mat=unreal.load_asset(path)
 if not mat:raise RuntimeError("Material missing")
 for n in ("Material","MaterialEditingLibrary","MaterialUsage","MaterialInterface","MaterialInstanceConstant"):
  try:
   cl=getattr(unreal,n)
   o["classes"][n]={"type":str(cl),"relevant_methods":{x:str(getattr(cl,x).__doc__)[:420] for x in dir(cl) if ("usage" in x.lower() or "morph" in x.lower() or "compile" in x.lower() or "set_material" in x.lower() or "used" in x.lower()) and not x.startswith("_")}}
  except Exception as e:o["classes"][n]={"error":str(e)}
 o["material_type"]=mat.__class__.__name__
 o["material_methods"]=[x for x in dir(mat) if ("usage" in x.lower() or "morph" in x.lower() or "used" in x.lower()) and not x.startswith("_")]
 for name in ("used_with_morph_targets","b_used_with_morph_targets","used_with_skeletal_mesh"):
  try:o["try_set"]["get_"+name]=str(mat.get_editor_property(name))
  except Exception as e:o["try_set"]["get_"+name+"_error"]=str(e)[:260]
  try:
   mat.set_editor_property(name,True)
   o["try_set"]["set_"+name]="SUCCESS"
  except Exception as e:o["try_set"]["set_"+name+"_error"]=str(e)[:260]
 o["status"]="PASS"
except Exception as e:o["status"]="FAIL";o["error"]=str(e)
finally:
 (R/"material_morph_usage_api_probe.json").write_text(json.dumps(o,indent=2,default=str),encoding="utf8")
