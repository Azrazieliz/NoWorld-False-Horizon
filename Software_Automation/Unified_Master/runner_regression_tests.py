"""Offline no-GUI safety regressions for the four-software dispatcher."""
import copy
import importlib.util
import json
import pathlib
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent
def load(name):
    spec=importlib.util.spec_from_file_location(name, ROOT/(name+".py"))
    m=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m
gate=load("gatectl")
pipeline=load("pipeline_runner")
with tempfile.TemporaryDirectory(prefix="nw_master_safety_") as t:
    temp=pathlib.Path(t)
    baseline={"schema":"NOWORLD_SOFTWARE_JOB_V1","job_id":"SafetyTest001",
      "owner_gate":"Gate01","test_only":True,"software":"koikatsu",
      "operation":"mod_lookup","requirements":{"preserve_originals":True,
      "max_walltime_seconds":30},"params":{"query":"wing"},
      "source_paths":[],"outputs":{"receipt_directory":str(temp/"receipt")}}
    cases=[]
    def reject(name,change):
        job=copy.deepcopy(baseline)
        change(job)
        try:gate.validate(job)
        except (ValueError,KeyError,TypeError):cases.append((name,True))
        else:cases.append((name,False))
    reject("empty_postconditions",lambda j:j.update(postconditions=[]))
    reject("zero_timeout",lambda j:j["requirements"].update(max_walltime_seconds=0))
    reject("excess_timeout",lambda j:j["requirements"].update(max_walltime_seconds=7201))
    reject("empty_receipt",lambda j:j["outputs"].update(receipt_directory=""))
    reject("non_test_job",lambda j:j.update(test_only=False))
    reject("non_preserving_job",lambda j:j["requirements"].update(preserve_originals=False))
    missing=copy.deepcopy(baseline)
    missing["source_paths"]=[str(temp/"missing_source.blend")]
    try:gate.run(missing)
    except FileNotFoundError:cases.append(("missing_source_fails_before_receipt",not (temp/"receipt").exists()))
    else:cases.append(("missing_source_fails_before_receipt",False))
    stale=temp/'stale.json'
    stale.write_text(json.dumps({'status':'PASS'}))
    import os,time
    old=time.time()-120
    os.utime(stale,(old,old))
    probe=copy.deepcopy(baseline)
    probe['postconditions']=[{'path':str(stale),'field':'status','equals':'PASS'}]
    observed=gate.verify_postconditions(probe,started_wall=time.time())
    cases.append(('stale_receipt_rejected',not observed[0]['passed']))
    plan={"schema":"NOWORLD_SOFTWARE_PIPELINE_V1",
      "steps":[baseline,copy.deepcopy(baseline)],"checkpoint":str(temp/"checkpoint.json")}
    path=temp/"bad_pipeline.json"
    path.write_text(json.dumps(plan))
    try:pipeline.plan_validate(path)
    except ValueError as e:cases.append(("duplicate_job_id", "Duplicate job_id" in str(e)))
    else:cases.append(("duplicate_job_id",False))
    result={"total":len(cases),"passed":sum(ok for _,ok in cases),
       "checks":dict(cases),"native_execution":False}
    print("NW_RUNNER_REGRESSION="+json.dumps(result))
    if result["passed"]!=result["total"]:raise SystemExit(2)
