import unreal
dst="/Game/UI/Gate10/Materials"
name="M_Gate10_CharacterCaptureAlphaUI"
target=dst+"/"+name
if unreal.EditorAssetLibrary.does_asset_exist(target):
    print("GATE10_CHARACTER_ALPHA_UI_ALREADY_PRESENT",target)
else:
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name,dst,unreal.Material,unreal.MaterialFactoryNew())
    if mat is None: raise RuntimeError("Cannot create UI cutout material")
    mat.set_editor_property("material_domain",unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode",unreal.BlendMode.BLEND_TRANSLUCENT)
    sample=unreal.MaterialEditingLibrary.create_material_expression(
        mat,unreal.MaterialExpressionTextureSampleParameter2D,-560,-40)
    sample.set_editor_property("parameter_name","LiveScene")
    sample.set_editor_property("texture",unreal.load_asset(
        "/Engine/EngineResources/DefaultTexture"))
    invert=unreal.MaterialEditingLibrary.create_material_expression(
        mat,unreal.MaterialExpressionOneMinus,-270,160)
    if not unreal.MaterialEditingLibrary.connect_material_property(
        sample,"RGB",unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Failed to link UI final color")
    # Unreal 5.8 exposes OneMinus as a single unnamed material input on
    # some editor builds. Discover the actual link rather than assuming
    # a fixed pin label that failed in the first authored attempt.
    inputs=unreal.MaterialEditingLibrary.get_material_expression_input_names(
        invert)
    print("INVERSE_ALPHA_INPUTS",inputs)
    linked=False
    for output_pin in ("A","Alpha"):
        for input_pin in ("","Input","A"):
            if unreal.MaterialEditingLibrary.connect_material_expressions(
                sample,output_pin,invert,input_pin):
                print("CONNECTED_SCENE_ALPHA",output_pin,input_pin)
                linked=True
                break
        if linked:break
    if not linked:
        raise RuntimeError("Native UE5.8 texture-alpha-to-OneMinus failed")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        invert,"",unreal.MaterialProperty.MP_OPACITY):
        raise RuntimeError("Failed to link UI opacity")
    unreal.MaterialEditingLibrary.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
        raise RuntimeError("Failed to save UI opacity material")
    print("GATE10_CHARACTER_SCENECAPTURE_ALPHA_MATERIAL_NATIVE",target)
