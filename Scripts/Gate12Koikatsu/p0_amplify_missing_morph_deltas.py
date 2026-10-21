"""New hypothesis: UE filters very small target deltas independently of UI threshold.
Multiply *only* six missing effective body morph POSITION deltas in an isolated
GLB copy. Keep names/accessor format unchanged. NO author/production modification.
"""
import pathlib,json,hashlib,struct,sys
SLOTS=(64,72,122,135,138,144)
FACTOR=20.
SRC=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\BLENDER_FRESH\CLOTHED\LUMINOUS_V3_CLOTHED_skinned.glb")
ROOT=pathlib.Path(__file__).resolve().parents[2]/"Saved"/"P0CharacterQuality"/"MorphHypothesisAmplitude"
OUT=ROOT/"LuminousV3_CLOTHED_P0QA_AmplifiedSix20x.glb"
raw=bytearray(SRC.read_bytes())
if raw[:4]!=b"glTF":raise RuntimeError("Not glTF2")
length,typ=struct.unpack_from("<II",raw,12)
assert typ==0x4e4f534a
g=json.loads(raw[20:20+length])
assert g["meshes"][3]["primitives"][11]["targets"]
bin_chunk_at=20+length
bin_len,bin_type=struct.unpack_from("<II",raw,bin_chunk_at)
assert bin_type==0x004E4942
bin_start=bin_chunk_at+8
changes=[]
for slot in SLOTS:
    prim=g["meshes"][3]["primitives"][11]
    accessor_index=prim["targets"][slot]["POSITION"]
    a=g["accessors"][accessor_index]
    assert a["componentType"]==5126 and a["type"]=="VEC3"
    if "sparse" in a:
        sparse=a["sparse"]
        count=sparse["count"]
        sv=sparse["values"]
        view=g["bufferViews"][sv["bufferView"]]
        base=bin_start+view.get("byteOffset",0)+sv.get("byteOffset",0)
        stride=view.get("byteStride",12)
    else:
        view=g["bufferViews"][a["bufferView"]]
        count=a["count"]
        base=bin_start+view.get("byteOffset",0)+a.get("byteOffset",0)
        stride=view.get("byteStride",12)
    modified=0
    biggest=0.
    for i in range(count):
        offset=base+i*stride
        xyz=struct.unpack_from("<fff",raw,offset)
        if any(abs(v)>1e-12 for v in xyz):
            modified+=1
            biggest=max(biggest,max(abs(v) for v in xyz))
            struct.pack_into("<fff",raw,offset,*(v*FACTOR for v in xyz))
    changes.append({"target_slot":slot,"position_accessor":accessor_index,
                    "vectors_count":count,"nonzero_vectors_changed":modified,
                    "before_max_delta_m":biggest,
                    "after_expected_max_delta_m":biggest*FACTOR,
                    "sparse":bool("sparse" in a)})
assert changes and all(c["nonzero_vectors_changed"]>0 for c in changes)
assert len(raw)==len(SRC.read_bytes())
ROOT.mkdir(parents=True,exist_ok=True)
if OUT.exists():
    if OUT.read_bytes()!=raw:raise RuntimeError("Existing QA variant differs; refuse overwrite")
else:OUT.write_bytes(raw)
receipt={
 "schema":"G12_P0_NEW_MORPH_MAGNITUDE_HYPOTHESIS_V1",
 "source_path":str(SRC),
 "source_sha256":hashlib.sha256(SRC.read_bytes()).hexdigest(),
 "qa_glb":str(OUT),"qa_sha256":hashlib.sha256(raw).hexdigest(),
 "source_original_untouched":True,"gltf_json_unchanged":True,
 "target_names_unchanged":True,"geometry_except_six_deltas_unchanged":True,
 "magnitude_factor":FACTOR,"modified":changes,
 "test_hypothesis":"If the identical six named targets now import, importer applies an additional amplitude cutoff independent of configured zero threshold.",
 "not_visual_or_character_acceptance":True,
}
(ROOT/"amplitude_hypothesis_manifest.json").write_text(json.dumps(receipt,indent=2)+"\n")
print(json.dumps(receipt,indent=2))
