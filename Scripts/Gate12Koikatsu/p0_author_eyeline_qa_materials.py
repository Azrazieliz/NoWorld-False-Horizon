"""Create ONLY isolated eyeline QA material assets, never source/defaults.
Full alpha-gradient vs threshold-masked diagnostic. Not Painterly PBR approved.
"""
import unreal,json,pathlib
ROOT="/Game/Experimental/Gate12P0MaterialQA"
SOURCE="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh003Prim005/Mesh_03_Prim_05/Textures/cf_m_eyeline_00_up_light"
OUT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"eyeline_qa_materials.json"
out={"schema":"P0_EYELINE_MATERIAL_QA_V1","production_materials_modified":False,
     "assets":[],"errors":[],"status":"PENDING"}
try:
    tex=unreal.load_asset(SOURCE)
    if not tex:raise RuntimeError("Actual source eyeline texture not found: "+SOURCE)
    unreal.EditorAssetLibrary.make_directory(ROOT)
    tool=unreal.AssetToolsHelpers.get_asset_tools()
    for name,mode in [("M_P0_Eyeline_AlphaBlend_2S",unreal.BlendMode.BLEND_TRANSLUCENT),
                      ("M_P0_Eyeline_Masked_2S_Diagnostic",unreal.BlendMode.BLEND_MASKED)]:
        path=ROOT+"/"+name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            mat=unreal.load_asset(path)
            if not mat:raise RuntimeError("Existing QA asset cannot load: "+path)
            out["assets"].append({"path":path,"status":"PRESERVED_EXISTING"})
            continue
        mat=tool.create_asset(name,ROOT,unreal.Material,unreal.MaterialFactoryNew())
        if not mat:raise RuntimeError("QA asset could not be authored: "+name)
        mat.set_editor_property("blend_mode",mode)
        mat.set_editor_property("two_sided",True)
        sample=unreal.MaterialEditingLibrary.create_material_expression(
            mat,unreal.MaterialExpressionTextureSample,-500,40)
        sample.set_editor_property("texture",tex)
        unreal.MaterialEditingLibrary.connect_material_property(
            sample,"RGB",unreal.MaterialProperty.MP_BASE_COLOR)
        alpha_dest=(unreal.MaterialProperty.MP_OPACITY if mode==unreal.BlendMode.BLEND_TRANSLUCENT
                    else unreal.MaterialProperty.MP_OPACITY_MASK)
        unreal.MaterialEditingLibrary.connect_material_property(sample,"A",alpha_dest)
        for value,prop,y in [(0.65,unreal.MaterialProperty.MP_ROUGHNESS,240),
                             (0.18,unreal.MaterialProperty.MP_SPECULAR,360)]:
            node=unreal.MaterialEditingLibrary.create_material_expression(
                mat,unreal.MaterialExpressionConstant,-440,y)
            node.set_editor_property("r",value)
            unreal.MaterialEditingLibrary.connect_material_property(node,"",
                                                                   prop)
        if mode==unreal.BlendMode.BLEND_MASKED:
            mat.set_editor_property("opacity_mask_clip_value",0.20)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
            raise RuntimeError("QA material could not be saved: "+name)
        out["assets"].append({"path":path,"status":"QA_CREATED",
                              "alpha_mode":str(mode),"double_sided":True,
                              "original_source_texture":SOURCE})
    out["status"]="ISOLATED_QA_ASSETS_CREATED_UNAPPROVED"
except Exception as e:
    out["errors"].append(str(e));out["status"]="FAILURE"
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(out,indent=2)+"\n")
unreal.log("P0_MATERIAL_QA_RESULT "+json.dumps(out))
