"""Generate version-specific parameter evidence from Blender RNA, not guessed defaults."""
import bpy, json, os, time
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),'..'))
OPS={'object':['modifier_add','transform_apply','mode_set','shape_key_add','parent_set','join'],
'mesh':['primitive_cube_add','subdivide','remove_doubles','quads_convert_to_tris','normals_make_consistent'],
'uv':['unwrap','smart_project','pack_islands'],
'sculpt':['dynamic_topology_toggle'],
'export_scene':['fbx','gltf'],
'import_scene':['fbx','gltf'],
'wm':['save_as_mainfile','open_mainfile','obj_export','obj_import','usd_export'],
'render':['render'],'nla':['bake'],'rigidbody':['object_add','bake_to_keyframes'],
'grease_pencil':['bake_grease_pencil_animation','interpolate_sequence']}
TYPE_NAMES=['BevelModifier','SubsurfModifier','RemeshModifier','DecimateModifier',
'ArmatureModifier','ClothModifier','FluidModifier','GeometryNodesModifier',
'GreasePencilv3','GreasePencilDrawing','Action','ActionSlot','NlaStrip','FCurve',
'Mesh','Material','Image','ShaderNodeTexNoise','ShaderNodeValToRGB',
'GeometryNodeMeshLine','GeometryNodeInstanceOnPoints','GeometryNodeRealizeInstances',
'GeometryNodeStoreNamedAttribute','GeometryNodeBake','GeometryNodeRepeatInput',
'GeometryNodeSimulationInput','GeometryNodeUVUnwrap','ShaderNodeBsdfPrincipled',
'SceneEEVEE','CyclesRenderSettings','World','RenderSettings','SpaceView3D']
def desc_prop(p):
    d={'id':p.identifier,'type':p.type,'description':p.description,'read_only':p.is_readonly}
    for a in ('subtype','unit','hard_min','hard_max','soft_min','soft_max','array_length'):
        try:
            v=getattr(p,a)
            if isinstance(v,(int,float,str,bool)):d[a]=v
        except Exception:pass
    try:
        if p.type=='ENUM':
            d['options']=[e.identifier for e in p.enum_items][:60]
        elif p.type not in ('POINTER','COLLECTION'):
            v=p.default
            if isinstance(v,(int,float,str,bool)):d['default']=v
    except Exception:pass
    return d
def run():
    start=time.perf_counter(); data={'blender':bpy.app.version_string,'build_hash':bpy.app.build_hash.decode() if isinstance(bpy.app.build_hash,bytes) else str(bpy.app.build_hash),
        'operations':{},'rna_types':{},'operation_failures':{},'missing_types':[]}
    for cat,names in OPS.items():
        for name in names:
            key=cat+'.'+name
            try:
                operation=getattr(getattr(bpy.ops,cat),name)
                rna=operation.get_rna_type()
                data['operations'][key]=[desc_prop(x) for x in rna.properties if x.identifier!='rna_type']
            except Exception as e:data['operation_failures'][key]=str(e)
    for typ in TYPE_NAMES:
        cls=getattr(bpy.types,typ,None)
        if not cls or not hasattr(cls,'bl_rna'):data['missing_types'].append(typ);continue
        try:data['rna_types'][typ]=[desc_prop(x) for x in cls.bl_rna.properties if x.identifier!='rna_type']
        except Exception as e:data['missing_types'].append(typ+':'+str(e))
    path=os.path.join(ROOT,'PARAMETER_REFERENCE.json')
    with open(path,'w',encoding='utf-8') as f:json.dump(data,f,indent=2,ensure_ascii=False)
    summary={'blender':data['blender'],'operator_count':len(data['operations']),
      'rna_type_count':len(data['rna_types']),
      'properties_total':sum(map(len,data['operations'].values()))+sum(map(len,data['rna_types'].values())),
      'missing_types':data['missing_types'],'operation_failures':data['operation_failures'],
      'seconds':round(time.perf_counter()-start,3)}
    with open(os.path.join(ROOT,'VALIDATION','local','rna_catalog_results.json'),'w') as f:json.dump(summary,f,indent=2)
    print('NOWORLD_RNA='+json.dumps(summary))
if __name__=='__main__':run()
