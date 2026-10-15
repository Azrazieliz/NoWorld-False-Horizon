"""Real Unreal native texture+material roundtrip for latest KK material, QA-only."""
import unreal,json,pathlib,hashlib,traceback,os,re
params=json.loads(os.environ.get('NW_UNREAL_JOB_PARAMS','{}'))
qa=pathlib.Path(r'D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015').resolve()
sm01=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\VALIDATION').resolve()
label=str(params.get('job_id','KKCosmicNativeMaterial_2140'))
if not re.fullmatch('[A-Za-z0-9_]{4,72}',label):raise ValueError('Unsafe job id')
src=pathlib.Path(params.get('texture',str(sm01/'AutoCosmicMaterial_04b94ac272a6'/'texture_id_4.png'))).resolve()
receipt=pathlib.Path(params.get('receipt',str(qa/(label+'.json')))).resolve()
if not src.is_relative_to(sm01) or not receipt.is_relative_to(qa):raise ValueError('External source/destination forbidden')
dest='/Game/NW_MasterHub_QA/'+label
result={'schema':'NW_KK_SKIN_TO_UE_NATIVE_MATERIAL_V1','sandbox_only':True,'game_project_changed':False}
try:
 if receipt.exists():raise FileExistsError(receipt)
 if not src.is_file():raise FileNotFoundError(src)
 expected=hashlib.sha256(src.read_bytes()).hexdigest()
 unreal.EditorAssetLibrary.make_directory(dest)
 task=unreal.AssetImportTask()
 task.set_editor_property('filename',str(src))
 task.set_editor_property('destination_path',dest)
 task.set_editor_property('destination_name','NW_KK_CosmicBodyTex')
 task.set_editor_property('automated',True)
 task.set_editor_property('replace_existing',False)
 task.set_editor_property('save',True)
 tools=unreal.AssetToolsHelpers.get_asset_tools()
 tools.import_asset_tasks([task])
 path=dest+'/NW_KK_CosmicBodyTex'
 texture=unreal.load_asset(path)
 if not texture or not isinstance(texture,unreal.Texture2D):raise RuntimeError('Native texture import failed')
 material=tools.create_asset('NW_KK_CosmicBodyPBR',dest,unreal.Material,unreal.MaterialFactoryNew())
 if not material:raise RuntimeError('Material creation failed')
 sample=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSample,-280,0)
 sample.set_editor_property('texture',texture)
 unreal.MaterialEditingLibrary.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
 unreal.MaterialEditingLibrary.recompile_material(material)
 if not unreal.EditorAssetLibrary.save_loaded_asset(material):raise RuntimeError('Unreal refused material save')
 if not unreal.load_asset(material.get_path_name()):raise RuntimeError('Material reload failed')
 if hashlib.sha256(src.read_bytes()).hexdigest()!=expected:raise RuntimeError('Source texture changed')
 result.update(status='PASS_NATIVE_TEXTURE_AND_MATERIAL',source_sha256=expected,
  texture_asset=texture.get_path_name(),material_asset=material.get_path_name(),
  material_texture_connected=True,texture_source_unchanged=True)
except Exception:
 result.update(status='FAIL',error=traceback.format_exc()[-2400:])
receipt.write_text(json.dumps(result,indent=2))
print('NW_KK_UE_MATERIAL='+json.dumps(result),flush=True)
