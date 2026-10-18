"""Read-only, subsecond master-method guard for every software-mastery job.

All 46 recorded learned controls must be present. This checks the stored doctrine and known
native source fixes before an automated Gate task is dispatched; it does not claim
to verify visual likeness, active game DLL state, texture parity or Android quality.
"""
import argparse
import hashlib
import json
import pathlib
import re

HERE = pathlib.Path(__file__).resolve().parent
LEDGER = HERE / "MASTER_LEARNED_METHODS_V1.json"
REQUIRED = {"K%03d" % i for i in range(1, 47)}
GATES = {"Gate%02d" % i for i in range(1, 13)}
SOURCE_SNIPPETS = {
    "SM01_Runtime_Source/SM01RuntimeController.cs": [
        "AssignCoordinate(", "native_coordinate_roundtrip",
        "saved_clothing_parts_verified", "saved_accessory_slots_verified",
    ],
    "SM01_Runtime_Source/SM01Placement.cs": [
        'cmd.group.EndsWith("2")', "SetAccessoryPos(", "SetAccessoryRot(",
        "SetAccessoryScl(", "correction",
    ],
    "SM01_Runtime_Source/SM01Appearance.cs": [
        'cmd.op=="set_face_makeup"', 'cmd.op=="set_eye_iris"',
        'cmd.op=="set_hair_palette"', 'cmd.op=="set_iris_asset"',
    ],
    "build_reference_kk_v2.py": [
        "apply_morph_batch", "set_accessory_vector", "render_views",
        "save_derived_card", "verify_derived_card",
    ],
    "native_kk_card_diff_v1.py": [
        "read_only", "coordinate", "accessor",
    ],
    "learn_native_kk_character_recipe.py": [
        "face_shape", "body_shape", "accessories", "pos2", "rot2", "scale2",
    ],
    "coordinate_inheritance_guard.py": [
        "REVIEW_INHERITED_COORDINATES", "no_auto_promotion", "baseline_matching_indices",
    ],
    "native_3d_material_scale_guard.py": [
        "FAIL_SOURCE_NORMAL_MAPS_NOT_BOUND", "visual_fidelity_accepted", "PASS_RELATIVE_BOUNDING_DIMENSIONS",
    ],
    "p0_effective_glb_morph_deltas.py": ["get_accessor", "sparse", "nonzero_deltas", "max_delta"],
    "p0_morph_source_to_glb_parity.py": ["source_effective_shape_keys", "missing_effective_keys", "delta_ratio_median"],
    "p0_eyeline_embedded_alpha.py": ["partial_alpha_fraction", "alphaMode", "bufferView"],
    "p0_targeted_regressions.py": ["no_effective_morph_loss_", "source_glb_upper_eyeline_blend_", "native_capture_sandbox_original_untouched"],
    "kk_one_session.py": ["catalog_plan", "stage_plan", "game_guards", "run_batch", "native_dispatch_seconds", "mid_session_mod_restart_allowed"],
    "test_kk_one_session.py": ["budget_overshoot_fails_before_launch", "one_process_native_batch_with_telemetry", "morph_batch_native_tsv"],
    "test05_repair_native_body_scale.py": ["FIXED_SOURCE_KK_CHAR_DIMENSIONS", "model_arm", "1<maxz-minz<2.5"],
    "test05_rebind_native_kkbp_v4.py": ["NATIVE_TEST05_IMPORTED", "ACTUAL_TEST05_BODY_HEIGHT", "Authentic Test05 Koikatsu exported texture"],
    "test05_qa_and_regressions.py": ["body_not_double_scaled", "source_native_eye_and_face_texture_nodes", "FAIL_REFERENCE_LIKENESS"],
    "kk_reference_features.py": ["GEOMETRIC_HUMANOID_PRIOR", "feature_identity_verified", "reference_sha256", "features"],
    "kk_feature_gates.py": ["score_features", "BLOCK_UNVERIFIED_FEATURE_ROI", "blocked_critical_gates", "artistic_visual_fidelity_accepted"],
    "kk_gate_native_optimizer.py": ["select_compare", "feature_improvements", "feature_regressions", "native_state_restored"],
    "test_kk_feature_gates.py": ["floating_hair_affects_hair_gate", "wrong_robe_main_outfit_detected"],
    "test_kk_gate_native_optimizer.py": ["baseline_restored_after_trials", "wrong_outfit_lower_gate_score"],
    "TEST06_8OF10_GATE_SCORE_CONTINUITY_20261018.md": ["user-rated 8/10", "11.77", "521d725", "Test06"],
    "TEST06_ARTIFACT_RECOVERY_MANIFEST_20261018.json": ["8/10", "new_ref_gold_dancer_trial.py", "T06_ACTUAL_NATIVE_KK_4_VIEW_QA.jpg"],
    "new_ref_gold_dancer_trial.py": ["apply_morph_batch", "set_clothes_top", "render_views"],
    "new_ref_gold_dancer_materials.py": ["surface_sources", "surface_edits", "alpha"],
    "kk_native_feature_alignment.py": ["figure_bbox", "register_boxes", "candidate_rois", "pose_normalization_verified"],
    "kk_registered_gate_optimizer.py": ["register_boxes", "native.run", "BEST_FEATURE_CROPS", "native_state_restored"],
    "kk_benchmark_guard.py": ["require_permitted_step", "scored_native_sweep", "require_reversible_plan", "_LEGACY_NATIVE_SCRIPTS"],
    "kk_scored_benchmark_runner.py": ["kk_test08_gate_dispatch", "gate.run"],
    "kk_native_visual_search.py": ["native_noise_measurement_valid", "benchmark_guard.require_permitted_step(step)", "DIRECT_NATIVE_CLIENT_EDIT_BLOCKED", "_NATIVE_SUBMIT_AUTH"],
    "test_kk_strict_native_dispatch_guard.py": ["ungated_test07plus_blocked", "direct_client_submit_blocked", "genuine_authorized_scored_edit_permitted"],
    "test_kk_native_feature_alignment.py": ["registered_selfscore_exceeds_unregistered_by_15", "wrong_top_lowers_its_own_feature"],
    "test_kk_registered_gate_optimizer.py": ["camera_zero_noise_is_valid", "native_baseline_restored"],
}
def ensure_master(ledger_path=LEDGER, source_dir=HERE):
    """Fail closed on missing contract, missing scope, or known regressed sources.

    No file writes, app launches, remote queries, or expensive catalog scans.
    """
    ledger_path = pathlib.Path(ledger_path)
    source_dir = pathlib.Path(source_dir)
    raw = ledger_path.read_bytes()
    data = json.loads(raw.decode("utf-8-sig"))
    if data.get("schema") != "NOWORLD_AUTOMATION_LEARNING_LEDGER_V1":
        raise ValueError("Master knowledge schema missing or invalid")
    if not isinstance(data.get("mission"), str) or "Gates 01" not in data["mission"]:
        raise ValueError("Shared gate automation purpose has been lost")
    rules = data.get("rules")
    if not isinstance(rules, list):
        raise ValueError("Missing master learning records")
    ids = [r.get("id") for r in rules if isinstance(r, dict)]
    if len(ids) != len(set(ids)) or not REQUIRED.issubset(set(ids)):
        raise ValueError("Master learning records missing, duplicate, or regressed")
    for record in rules:
        if any(not isinstance(record.get(field), str) or not record[field].strip()
               for field in ("id", "topic", "applies_to", "requirement",
                             "procedure", "evidence", "status")):
            raise ValueError("Incomplete learned rule: %r" % record)
    if not GATES.issubset(set(data.get("gates", {}))):
        raise ValueError("Gate 01 through Gate 12 integration contract incomplete")
    startup = data.get("startup_contract", {})
    if startup.get("method", "").find("mastery_preflight.ensure_master()") < 0:
        raise ValueError("Startup methodology not bound")
    if len(startup.get("mandatory", [])) < 5:
        raise ValueError("Startup steps dropped")
    # These checks are regression sentinels, NOT a claim of all actual runtime acceptance.
    for relpath, snippets in SOURCE_SNIPPETS.items():
        source = (source_dir / relpath)
        if not source.is_file():
            raise FileNotFoundError("Master workflow source missing: " + str(source))
        text = source.read_text(encoding="utf-8-sig")
        absent = [token for token in snippets if token not in text]
        if absent:
            raise RuntimeError("Regressed known master implementation: %s missing %s"
                               % (relpath, ", ".join(absent)))
    return {"status": "PASS_MASTER_METHODS",
            "revision": data["revision"],
            "rules": len(rules),
            "gates": len(GATES),
            "source_checks": len(SOURCE_SNIPPETS),
            "knowledge_sha256": hashlib.sha256(raw).hexdigest(),
            "native_runtime_verified_on_this_run": False,
            "visual_fidelity_verified_on_this_run": False}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ledger")
    parser.add_argument("--source-dir")
    args = parser.parse_args()
    result = ensure_master(args.ledger or LEDGER, args.source_dir or HERE)
    print("NW_MASTERY_PREFLIGHT=" + json.dumps(result, sort_keys=True))

if __name__ == "__main__":
    main()
