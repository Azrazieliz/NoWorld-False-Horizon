# Gate 12 native UE material from EXISTING Gate 02 P03 authored surface sources.
# Preserves source Blender and P03 textures, never alters accepted canonical maps.
import unreal, pathlib, traceback, json, hashlib
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
P03=pathlib.Path(r"D:\NoWorld_Art_Production\P03_Kingdom_River_Citadel_Environment\assets\surface_textures")
PCC=pathlib.Path(r"D:\NoWorld_Art_Production\Shared_Assets\PolyHaven_CC0\Textures")
TEXROOT="/Game/NoWorld/FirstWorld/R1/Terrain/Textures"
MATROOT="/Game/NoWorld/FirstWorld/R1/Terrain"
MATPATH=MATROOT+"/M_R1_Gate02_SourceBiomes"
receipt=R/"Saved/Gate12Readiness/Gate12_GenuineR1SurfaceMaterial_20261023.json"
data={"material_path":MATPATH,"status":"STARTED","source_textures":[]}
assets=[
 ("T_R1_Gate02_MudLeaves_D",P03/"brown_mud_leaves_01"/"brown_mud_leaves_01_diff_2k.jpg",False),
 ("T_R1_Gate02_ForestGround_D",P03/"forest_ground_04"/"forest_ground_04_diff_2k.jpg",False),
 ("T_R1_Gate02_Escarpment_D",PCC/"cliff_side"/"cliff_side_diffuse_2k.jpg",False),
 ("T_R1_Gate02_MudLeaves_N",P03/"brown_mud_leaves_01"/"brown_mud_leaves_01_nor_gl_2k.jpg",True),
 ("T_R1_Gate02_ForestGround_N",P03/"forest_ground_04"/"forest_ground_04_nor_gl_2k.jpg",True),
 ("T_R1_Gate02_Escarpment_N",PCC/"cliff_side"/"cliff_side_nor_gl_2k.jpg",True)
]
try:
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    for name,source,bnormal in assets:
        if not source.exists(): raise RuntimeError("Original P03 source missing: "+str(source))
        path=TEXROOT+"/"+name
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            task=unreal.AssetImportTask()
            task.set_editor_property("filename",str(source))
            task.set_editor_property("destination_path",TEXROOT)
            task.set_editor_property("destination_name",name)
            task.set_editor_property("automated",True)
            task.set_editor_property("save",True)
            task.set_editor_property("replace_existing",False)
            tools.import_asset_tasks([task])
        tex=unreal.EditorAssetLibrary.load_asset(path)
        if not tex: raise RuntimeError("Unreal texture missing: "+path)
        if bnormal:
            tex.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb",False)
            unreal.EditorAssetLibrary.save_loaded_asset(tex)
        data["source_textures"].append({
            "asset":path,"source":str(source),
            "sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
            "normal":bnormal})
    if unreal.EditorAssetLibrary.does_asset_exist(MATPATH):
        m=unreal.EditorAssetLibrary.load_asset(MATPATH)
        if not m: raise RuntimeError("Existing material path does not load")
        data["status"]="EXISTING_MATERIAL_PRESERVED"
    else:
        m=tools.create_asset("M_R1_Gate02_SourceBiomes",MATROOT,
            unreal.Material,unreal.MaterialFactoryNew())
        if not m: raise RuntimeError("Material factory failed")
        uv=unreal.MaterialEditingLibrary.create_material_expression(
            m,unreal.MaterialExpressionTextureCoordinate,-1050,20)
        uv.set_editor_property("u_tiling",100.)
        uv.set_editor_property("v_tiling",100.)
        vertex=unreal.MaterialEditingLibrary.create_material_expression(
            m,unreal.MaterialExpressionVertexColor,-1100,380)
        def samples(prefix,row):
            out=[]
            for k in range(3):
                tex=unreal.EditorAssetLibrary.load_asset(TEXROOT+"/"+assets[k+(3 if row else 0)][0])
                n=unreal.MaterialEditingLibrary.create_material_expression(
                    m,unreal.MaterialExpressionTextureSample,-820,k*320+row*940)
                n.set_editor_property("texture",tex)
                unreal.MaterialEditingLibrary.connect_material_expressions(uv,"",n,"UVs")
                out.append(n)
            return out
        colors=samples("color",0)
        normals=samples("normal",1)
        def layered(source_nodes,y):
            soil=unreal.MaterialEditingLibrary.create_material_expression(
                m,unreal.MaterialExpressionLinearInterpolate,-370,y)
            rock=unreal.MaterialEditingLibrary.create_material_expression(
                m,unreal.MaterialExpressionLinearInterpolate,-100,y)
            # Source P03 lower-earth, forest ground and source cliff textures.
            unreal.MaterialEditingLibrary.connect_material_expressions(source_nodes[0],"RGB",soil,"A")
            unreal.MaterialEditingLibrary.connect_material_expressions(source_nodes[1],"RGB",soil,"B")
            unreal.MaterialEditingLibrary.connect_material_expressions(vertex,"R",soil,"Alpha")
            unreal.MaterialEditingLibrary.connect_material_expressions(soil,"",rock,"A")
            unreal.MaterialEditingLibrary.connect_material_expressions(source_nodes[2],"RGB",rock,"B")
            unreal.MaterialEditingLibrary.connect_material_expressions(vertex,"G",rock,"Alpha")
            return rock
        color=layered(colors,0)
        normal=layered(normals,940)
        unreal.MaterialEditingLibrary.connect_material_property(
            color,"",unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.MaterialEditingLibrary.connect_material_property(
            normal,"",unreal.MaterialProperty.MP_NORMAL)
        rough=unreal.MaterialEditingLibrary.create_material_expression(
            m,unreal.MaterialExpressionConstant,-80,1620)
        rough.set_editor_property("r",0.89)
        unreal.MaterialEditingLibrary.connect_material_property(
            rough,"",unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(m)
        if not unreal.EditorAssetLibrary.save_loaded_asset(m):
            raise RuntimeError("Material save failed")
        data["status"]="CREATED_NATIVE_TERRAIN_MATERIAL"
except Exception:
    data["status"]="FAILED"
    data["error"]=traceback.format_exc()[-3500:]
finally:
    receipt.parent.mkdir(parents=True,exist_ok=True)
    receipt.write_text(json.dumps(data,indent=2),encoding="utf8")
    print("GATE12_GENUINE_SOURCE_MATERIAL_STATUS",data["status"])
    if data.get("error"): print(data["error"])
