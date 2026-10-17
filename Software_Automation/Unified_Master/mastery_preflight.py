"""Read-only, subsecond master-method guard for every software-mastery job.

All 37 recorded learned controls must be present. This checks the stored doctrine and known
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
REQUIRED = {"K%03d" % i for i in range(1, 38)}
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
