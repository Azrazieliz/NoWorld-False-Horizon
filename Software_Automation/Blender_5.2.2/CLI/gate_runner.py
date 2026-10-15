"""Gate 01/12 non-overwriting headless Blender 5.2 production command.
blender.exe --background --factory-startup --python gate_runner.py -- --op audit|gradient|static|skeletal|animation --source IN.blend --output OUT --report EVIDENCE.json
"""
import bpy,sys,argparse,pathlib,json
ROOT=pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/"ADDONS"))
from noworld_mastery_hub import core
def main():
    a=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
    p=argparse.ArgumentParser()
    p.add_argument("--op",required=True,choices=["audit","gradient","look","static","skeletal","animation","kk_obj"])
    p.add_argument("--source",required=True);p.add_argument("--output");p.add_argument("--report",required=True)
    p.add_argument("--object",default="");p.add_argument("--rig",default="")
    p.add_argument("--strength",type=float,default=.45)
    p.add_argument("--world",action="store_true");p.add_argument("--slot",type=int,default=0)
    p.add_argument("--preset",choices=["Moonlit","Autumn"],default="Moonlit")
    p.add_argument("--objects",nargs="*")
    o=p.parse_args(a)
    source=pathlib.Path(o.source).resolve()
    if not source.is_file():raise FileNotFoundError(source)
    if bpy.app.version[:2]!=(5,2):raise RuntimeError("Requires Blender 5.2.x")
    report=pathlib.Path(o.report).resolve()
    if report.exists():raise FileExistsError(report)
    if o.op!="audit":
        if not o.output:raise ValueError("--output required")
        target=pathlib.Path(o.output).resolve()
        if target.exists() or target==source:raise FileExistsError("Refusing output overwrite: "+str(target))
    pre=core.sha256(source)
    bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
    if o.op=="audit":result=core.audit()
    elif o.op=="gradient":
        mesh=bpy.data.objects.get(o.object)
        if not mesh:raise ValueError("Unknown --object "+o.object)
        result=core.apply_gradient(mesh,slot=o.slot,strength=o.strength,world=o.world)
        result["source_blend_derivative"]=core.derivative_save(target)
    elif o.op=="look":
        result=core.apply_look_preset(o.preset,world=o.world)
        result["source_blend_derivative"]=core.derivative_save(target)
    elif o.op=="kk_obj":
        result=core.export_koikatsu_obj(target,mesh_names=o.objects if o.objects else None)
    else:
        result=core.export_fbx(target,rig_name=o.rig if o.op!="static" else None,
            action_only=o.op=="animation",mesh_names=o.objects)
    post=core.sha256(source)
    assert pre==post,"Original source changed"
    evidence={"operation":o.op,"blender":bpy.app.version_string,
       "source":str(source),"source_sha256":pre,"source_unchanged":True,
       "result":result,"native_unreal_validated_by_this_run":False,
       "professional_art_acceptance":False}
    report.parent.mkdir(parents=True,exist_ok=True)
    report.write_text(json.dumps(evidence,indent=2,default=str),encoding="utf-8")
    print("NOWORLD_GATE_OUTPUT="+json.dumps({"op":o.op,"report":str(report),
        "source_unchanged":True,"result":result},default=str))
if __name__=="__main__":main()
