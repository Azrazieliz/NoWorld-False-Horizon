"""Read-only Unreal 5.8.3 material / shader / texture linkage review.
Examines representative body, face and transparency-sensitive hair materials.
Does not mutate parents, compression, alpha mode, LODs or render settings.
"""
import json,pathlib
import unreal

BASE="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts"
MAT={
  "body":BASE+"/Mesh003Prim011/Mesh_03_Prim_11/Materials/KK_cf_m_body",
  "face":BASE+"/Mesh003Prim000/Mesh_03_Prim_00/Materials/KK_cf_m_face_00",
  "hair":BASE+"/Mesh004Prim000/Mesh_04_Prim_00/Materials/KK_cf_m_hair_f_82_3560",
  "eyeline_upper":BASE+"/Mesh003Prim005/Mesh_03_Prim_05/Materials/KK_cf_m_eyeline_00_up"
}
OUT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"unreal_material_pipeline.json"


def inspect_asset(path):
    m=unreal.load_asset(path)
    if not m:
        return {"path":path,"error":"MATERIAL_NOT_LOADED"}
    record={"path":path,"class":m.get_class().get_name(),"parents":[]}
    n=m
    for _ in range(5):
        record["parents"].append(n.get_path_name())
        parent=None
        try: parent=n.get_editor_property("parent")
        except Exception: pass
        if not parent or parent==n:
            break
        n=parent
    root=n
    for name in ("blend_mode","shading_model","two_sided","opacity_mask_clip_value",
                 "cast_dynamic_shadow_as_masked","dithered_lod_transition"):
        try:
            record[name]=str(root.get_editor_property(name))
        except Exception as exc:
            record[name]="NOT_EXPOSED:"+str(exc)[:105]
    for name in ("texture_parameter_values","scalar_parameter_values",
                 "vector_parameter_values","base_property_overrides"):
        try:
            value=m.get_editor_property(name)
            record[name+"_count"]=len(value) if hasattr(value,"__len__") else 1
            record[name+"_sample"]=str(value)[:1200]
        except Exception:
            pass
    try:
        tex=unreal.MaterialEditingLibrary.get_used_textures(m)
        record["used_textures"]=[
            {"path":t.get_path_name(),"class":t.get_class().get_name(),
             "width":t.blueprint_get_size_x() if isinstance(t,unreal.Texture2D) else None,
             "height":t.blueprint_get_size_y() if isinstance(t,unreal.Texture2D) else None,
             "lod_group":str(t.get_editor_property("lod_group")) if isinstance(t,unreal.Texture2D) else None,
             "compression":str(t.get_editor_property("compression_settings")) if isinstance(t,unreal.Texture2D) else None}
            for t in tex if t
        ]
    except Exception as exc:
        record["used_textures_error"]=str(exc)[:200]
    return record


result={
    "schema":"GATE12_P0_MATERIAL_NATIVE_INSPECTION_V1",
    "status":"NO_COOK_READONLY_UNREAL_MATERIALS",
    "materials":{key:inspect_asset(path) for key,path in MAT.items()},
    "unreal_render_settings_note":"Effective resident streaming mip and full KK Unity shader parity require actual frame capture.",
    "asset_modification":False
}
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
unreal.log("P0_NATIVE_MATERIALS "+str(OUT))
