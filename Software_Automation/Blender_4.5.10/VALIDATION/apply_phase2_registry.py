"""Apply evidence-linked 2026-10-15 capability status without declaring production readiness."""
import json,os
base=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
path=os.path.join(base,"CAPABILITY_REGISTRY.json")
obj=json.load(open(path,encoding="utf-8-sig"))
rows={
 "Environment_Sanctuary":{"status":"VALIDATED","script":"ENVIRONMENTS/parametric_sanctuary.py","evidence":"VALIDATION/phase2/sanctuary_results.json","limitation":"200-object stylized source and FBX; no professional level architecture acceptance"},
 "Mount_16BoneRig":{"status":"VALIDATED","script":"CREATURES/rigged_beast.py","evidence":"VALIDATION/phase2/beast_results.json","limitation":"36 mesh parts, 16 bones, 48 frames; contact slippage and artistic issues"},
 "Hybrid_GP_3D_48Frames":{"status":"VALIDATED","script":"ANIMATION_2D/hybrid_strike.py","evidence":"VALIDATION/phase2/hybrid_results.json","limitation":"14 GP drawings and actual 2second MP4; not professional anime quality"},
 "Static_MaterialBatch_UV":{"status":"VALIDATED","script":"INTEGRATIONS/optimized_static_export.py","evidence":"VALIDATION/phase2/Unreal/ue_batch_results.json","limitation":"200→9 UE native StaticMesh assets; no Unreal material equivalence or Android testing"},
 "Native_Unreal_Morph":{"status":"VALIDATED","script":"INTEGRATIONS/UNREAL_5_8_3/audit_and_anim.py","evidence":"VALIDATION/phase2/Unreal/ue_native_deep_audit.json","limitation":"Morph imported into UE SkeletalMesh; actual facial expressive fidelity unverified"},
 "Native_Unreal_AnimSequence":{"status":"VALIDATED","script":"INTEGRATIONS/export_mount_animation.py","evidence":"VALIDATION/phase2/Unreal/ue_anim_only_results.json","limitation":"One ARMATURE-only FBX imported AnimSequence; gameplay and retarget quality unverified"},
 "Koikatsu_Shard_ReadOnly_Import":{"status":"VALIDATED","script":"INTEGRATIONS/koikatsu_glb_audit.py","evidence":"VALIDATION/phase2/koikatsu_shards_summary.json","limitation":"6 shards, 121 bones per shard, 602 distinct key names, no complete reconstruct"},
 "FootContact_QuantMetrics":{"status":"VALIDATED","script":"VALIDATION/animation_contact_audit.py","evidence":"VALIDATION/phase2/mount_contact_metrics.json","limitation":"Detection only, failed quality gate; no corrective algorithm authored"}
}
existing={e['id'] for e in obj['entries']}
for k,v in rows.items():
 if k not in existing:obj['entries'].append(dict(id=k,production_ready=False,**v))
obj['phase2_milestone']="2026-10-15"
with open(path,"w",encoding="utf-8") as f:json.dump(obj,f,indent=2)
matrix_path=os.path.join(base,"RESEARCH_COVERAGE_MATRIX.json")
matrix=json.load(open(matrix_path,encoding="utf-8-sig"))
up={"architecture_landmarks":"EXECUTED","creatures":"EXECUTED","mounts":"EXECUTED",
    "2d_3d_hybrid":"EXECUTED","grease_pencil_2d":"VALIDATED",
    "koikatsu_interop":"EXECUTED","unreal_interop":"VALIDATED",
    "automated_visual_qc":"EXECUTED","animation_skeletal":"VALIDATED",
    "production_asset_pipeline":"EXECUTED"}
for d in matrix["domains"]:
 if d["name"] in up:
  d["status"]=up[d["name"]]
  d["evidence"]=d.get("evidence","")+"; phase2 independent execution in PHASE2_EXECUTED_EVIDENCE.md"
  d["production_ready"]=False
matrix["accepted_professional_demonstrations"]=0
with open(matrix_path,"w",encoding="utf-8") as f:json.dump(matrix,f,indent=2)
print("registry:"+str(len(obj["entries"]))+" | researched domains:"+str(len(matrix["domains"])))
