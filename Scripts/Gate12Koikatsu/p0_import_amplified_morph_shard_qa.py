"""QA import of existing-source body shard with six isolated amplified deltas.
No production asset/source overwrite, no cook, and not a genuine art candidate.
Tests native Interchange importer amplitude/name/accessor hypothesis.
"""
import unreal,pathlib,json,hashlib,time,traceback
ROOT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"
SRC=ROOT/"MorphHypothesisAmplitude"/"LuminousV3_CLOTHED_BodyShard_P0QA_AmplifiedSix20x.glb"
M=ROOT/"MorphHypothesisAmplitude"/"amplitude_hypothesis_manifest.json"
DEST="/Game/Experimental/Gate12P0MorphMagnitudeQA/AmplifiedSix20x"
OUT=ROOT/"morph_amplitude_native_import_result.json"
result={"schema":"G12_P0_AMPLITUDE_IMPORT_RESULT_V1","status":"START",
        "source_untouched":True,"production_assets_modified":False,
        "qa_destination":DEST,"qa_glb":str(SRC),
        "expected_nonzero_body_position_targets":15,
        "baseline_native_imported":9,"morph_targets":[],"errors":[]}
def save():
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(result,indent=2)+"\n")
save()
try:
    manifest=json.loads(M.read_text("utf-8"))
    if not SRC.is_file() or hashlib.sha256(SRC.read_bytes()).hexdigest()!=manifest["qa_sha256"]:
        raise RuntimeError("QA shard hash doesn't match immutable prepared hypothesis")
    if unreal.EditorAssetLibrary.does_directory_exist(DEST):
        raise RuntimeError("QA destination already exists; refusing existing-asset overwrite")
    pipeline=unreal.InterchangeGenericAssetsPipeline()
    mesh=pipeline.get_editor_property("mesh_pipeline")
    mesh.set_editor_property("morph_threshold_position",0.)
    if mesh.get_editor_property("morph_threshold_position")!=0.:
        raise RuntimeError("Diagnostic import threshold was not accepted")
    task=unreal.AssetImportTask()
    task.filename=str(SRC)
    task.destination_path=DEST
    task.automated=True
    task.async_=False
    task.save=True
    task.replace_existing=False
    task.options=pipeline
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    native=[]
    assets=unreal.EditorAssetLibrary.list_assets(DEST,recursive=True,include_folder=False)
    for name in assets:
        a=unreal.EditorAssetLibrary.load_asset(name)
        if isinstance(a,unreal.SkeletalMesh):
            names=[str(x.get_name()) for x in a.get_editor_property("morph_targets")]
            native.append({"asset":str(name),"morph_names":names,"morph_count":len(names),
                           "imported_bounds":str(a.get_imported_bounds())})
    result["morph_targets"]=native
    result["asset_count"]=len(assets)
    result["native_morph_total"]=sum(x["morph_count"] for x in native)
    result["status"]="QA_AMPLITUDE_RECOVERED" if result["native_morph_total"]>=15 else "QA_AMPLITUDE_STILL_MISSING"
except Exception as e:
    result["status"]="QA_IMPORT_FAILED_PRESERVE_SOURCE"
    result["errors"].append(str(e))
    result["traceback"]=traceback.format_exc()[-1700:]
result["finished_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
save()
unreal.log("P0_MORPH_AMPLITUDE_IMPORT "+result["status"]+
           " count="+str(result.get("native_morph_total"))+" errors="+str(result["errors"]))
