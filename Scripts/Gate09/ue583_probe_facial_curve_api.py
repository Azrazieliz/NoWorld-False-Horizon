import unreal,pathlib,json,time,traceback
r=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019\Saved\Gate09QA\face_curve_api_probe.json')
obj=unreal.load_asset('/Game/Gate09_QA_20261019/AuthenticFaceSource/AN_G09_Luminous_NativeBlinkSmile_SourceProof')
out={'status':'STARTED','probes':{}}
try:
 model=obj.get_editor_property('controller').get_model_interface()
 for name,v in [('model',model),('sequence',obj),('library',unreal.AnimationLibrary)]:
  out['probes'][name]={x:str(getattr(v,x).__doc__)[:420] for x in dir(v) if ('curve' in x.lower() or 'float' in x.lower()) and not x.startswith('_')}
 for fn,arg in [('get_animation_curve_names',[]),('get_animation_curve_names',[unreal.RawCurveTrackTypes.RCT_FLOAT])]:
  try:out['probe_'+fn+'_'+str(len(arg))]=str(getattr(unreal.AnimationLibrary,fn)(obj,*arg))[:800]
  except Exception as e:out['probe_'+fn+'_'+str(len(arg))+'_error']=str(e)
 for prop in ['curve_data','raw_curve_data','data_model']:
  try:out['property_'+prop]=str(obj.get_editor_property(prop))[:500]
  except Exception as e:out['property_'+prop+'_error']=str(e)
 out['status']='PASS_API_DISCOVERED'
except Exception as e:out['status']='ERROR';out['error']=traceback.format_exc()
finally:r.write_text(json.dumps(out,indent=2)+'\n',encoding='utf8');unreal.log('G09_FACE_API_'+out['status'])
