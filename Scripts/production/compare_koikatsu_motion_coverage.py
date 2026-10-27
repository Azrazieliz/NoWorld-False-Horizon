#!/usr/bin/env python3
"""Aggregate local intake/catalog metadata without opening curves or native assets.

Identity is (declared original bundle SHA-256, signed Unity path ID). Catalog
source_hash is a different hash and is NEVER used as its bundle identity.
No source names, original paths, raw IDs, curves or approval decisions are
exported. Native .uasset files are checked for existence only, never hashed.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import stat
import sys


H_MANIFEST = Path("SourceAssets/Gate12/Animations20261021/KoikatsuHOriginalSourceCurves/GATE12_INSTALLED_H_ANIMATION_SOURCE_CURVE_INDEX_COMPLETE.json")
GATE09_MANIFEST = Path("SourceAssets/Gate12/Animations20261021/ImportedGate09OriginalUnityCurves/GATE09_ALL_SOURCE_INTAKE_PROGRESS.json")
ADDITIONAL_DIRECTORY = Path("SourceAssets/Gate12/Animations20261022/KoikatsuAdditionalOriginalUnityCurves")
ADDITIONAL_PATTERN = "GATE12_ALL_INSTALLED_KK_ADDITIONAL_ANIMATIONS_PART_*.json"
H_SCHEMA = "noworld.gate12.kkh_local_source_manifest_join.v1"
GATE09_SCHEMA = "noworld.gate12.gate09_source_curve_intake.v2"
ADDITIONAL_SCHEMA = "noworld.gate12.all_installed_kk_original_additional_clips.v1"
CATALOG_SCHEMA = "noworld.shared_motion_catalog.v1"
RECEIPT_SCHEMA = "noworld.koikatsu_motion_coverage.v1"
PATH_STATES = ("present", "missing", "outside_root", "invalid", "error")
METADATA_FIELDS = ("scene_id", "scene_role", "participants", "source_bundle_sha256",
                   "source_unity_path_id", "source_hash", "source_clip_id",
                   "native_sequence", "native_duration_seconds", "approved_for_gameplay")


class CoverageError(ValueError):
    """Sanitized failure: never includes proprietary document values or paths."""


def _pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise CoverageError("Duplicate JSON member in an input document.")
        result[key] = value
    return result


def _constant(_):
    raise CoverageError("Nonfinite JSON constant in an input document.")


def read_json(path):
    try:
        before = path.stat()
        data = json.loads(path.read_text(encoding="utf-8-sig"),
                          object_pairs_hook=_pairs, parse_constant=_constant)
        after = path.stat()
        if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            raise CoverageError("An input manifest changed during the read.")
    except (OSError, ValueError, RecursionError) as error:
        if isinstance(error, CoverageError):
            raise
        raise CoverageError("An input manifest could not be read as strict JSON.") from None
    if not isinstance(data, dict):
        raise CoverageError("An input manifest must be a JSON object.")
    return data


def sha(value):
    return isinstance(value, str) and re.fullmatch(r"[a-fA-F0-9]{64}", value) is not None


def path_id(value):
    if isinstance(value, str) and re.fullmatch(r"-?(?:0|[1-9][0-9]*)", value):
        try:
            value = int(value)
        except ValueError:
            return None
    if type(value) is not int or not -(2**63) <= value < 2**63:
        return None
    return value


def identity(row, hash_field, id_field):
    digest, number = row.get(hash_field), path_id(row.get(id_field))
    return (digest.lower(), number) if sha(digest) and number is not None else None


def _nonempty(value):
    return isinstance(value, str) and bool(value.strip())


def metadata(rows):
    result = {}
    for field in METADATA_FIELDS:
        present = populated = valid = 0
        for row in rows:
            if field not in row:
                continue
            present += 1
            value = row[field]
            filled = value is not None and value != ""
            populated += bool(filled)
            if field == "participants":
                ok = type(value) is int and 1 <= value <= 2**31 - 1
            elif field == "source_unity_path_id":
                ok = path_id(value) is not None
            elif field in ("source_hash", "source_bundle_sha256"):
                ok = sha(value)
            elif field == "approved_for_gameplay":
                ok = type(value) is bool
            elif field == "native_duration_seconds":
                ok = type(value) in (int, float) and 0 < value < float("inf")
            else:
                ok = _nonempty(value)
            valid += bool(ok)
        result[field] = {"present": present, "populated": populated, "valid": valid}
    return result


class PathAudit:
    def __init__(self, root):
        self.root = root.resolve()
        self.cache = {}
        self.parents = {}
        self.rows = Counter()
        self.unique = set()

    def check(self, value):
        if not isinstance(value, str) or not value.strip():
            state, key = "invalid", ""
        elif value in self.cache:
            state, key = self.cache[value]
        else:
            try:
                # Never probe network/URL paths, and follow no path outside the
                # explicitly authorized root, including a symlink escape.
                if "://" in value or value.startswith(("\\\\", "//")):
                    state, key = "outside_root", value
                else:
                    candidate = Path(value)
                    candidate = candidate if candidate.is_absolute() else self.root / candidate
                    if candidate.name in ("", ".", ".."):
                        raise ValueError("Not a file reference")
                    parent = candidate.parent
                    if parent not in self.parents:
                        self.parents[parent] = parent.resolve()
                    candidate = self.parents[parent] / candidate.name
                    key = str(candidate)
                    if not candidate.is_relative_to(self.root):
                        state = "outside_root"
                    else:
                        try:
                            info = candidate.lstat()
                            if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0):
                                candidate = candidate.resolve()
                                key = str(candidate)
                                state = ("present" if candidate.is_file() else "missing") if candidate.is_relative_to(self.root) else "outside_root"
                            else:
                                state = "present" if stat.S_ISREG(info.st_mode) else "missing"
                        except FileNotFoundError:
                            state = "missing"
            except (OSError, ValueError, RuntimeError):
                state, key = "error", value
            self.cache[value] = state, key
        self.rows[state] += 1
        self.unique.add((state, key))
        return state

    def invalid(self):
        return self.check(None)

    def summary(self):
        unique = Counter(state for state, _ in self.unique)
        return {"rows": {s: self.rows[s] for s in PATH_STATES},
                "unique_paths": {s: unique[s] for s in PATH_STATES}}


def _document(path, schema, array):
    data = read_json(path)
    if data.get("schema") != schema:
        raise CoverageError("Unrecognized intake or catalog schema.")
    rows = data.get(array)
    if not isinstance(rows, list) or any(not isinstance(row, dict) for row in rows):
        raise CoverageError("Expected a manifest array of object rows.")
    return data, rows


def _content_root(catalog, source_root):
    return next((p for p in catalog.parents if p.name.lower() == "content"), source_root / "Content").resolve()


def local_path(value):
    path = Path(value)
    if str(path).startswith(("\\\\", "//")) or "://" in str(value):
        raise CoverageError("Only explicitly local filesystem paths are supported.")
    path = path.resolve()
    if str(path).startswith(("\\\\", "//")):
        raise CoverageError("A local input resolves to a network path.")
    return path


def _native_file(value):
    if not isinstance(value, str) or not value.startswith("/Game/"):
        return None
    package, dot, object_name = value.rpartition(".")
    relative = package[len("/Game/"):]
    if not dot or not relative or not re.fullmatch(r"[A-Za-z0-9_]+", object_name):
        return None
    parts = relative.split("/")
    if any(not re.fullmatch(r"[A-Za-z0-9_]+", p) for p in parts) or parts[-1] != object_name:
        return None
    return Path(*parts).with_suffix(".uasset")


def compare(source_root, catalog):
    source_root, catalog = local_path(source_root), local_path(catalog)
    if not source_root.is_dir():
        raise CoverageError("The authorized source project root is unavailable.")
    additional = sorted((source_root / ADDITIONAL_DIRECTORY).glob(ADDITIONAL_PATTERN))
    if not additional:
        raise CoverageError("No additional intake batch manifests were found.")
    specs = [
        ("h", [source_root / H_MANIFEST], H_SCHEMA, "clip_rows", "source_digest_sha256", "native_path_id", "curve_file", "actual_native_curve_clip_count", "ALL_INSTALLED_H_SOURCE_CURVES_CAPTURED_NOT_YET_RETARGETED"),
        ("gate09", [source_root / GATE09_MANIFEST], GATE09_SCHEMA, "source_files", "source_hash", "original_clip_path_id", "raw_curve_path", "actual_curve_objects_extracted", "ALL_SOURCE_BUNDLES_PROCESSED"),
        ("additional", additional, ADDITIONAL_SCHEMA, "clips", "source_bundle_sha256", "path_id", "native_curve_path", "clip_count", "SOURCE_ARCHIVE_COMPLETE_NOT_NATIVE_UNREAL"),
    ]
    all_ids, groups, sets = set(), {}, {}
    curve_paths = PathAudit(source_root)
    all_rows, invalid_total = 0, 0
    for label, files, schema, array, hash_field, id_field, curve_field, count_field, complete_status in specs:
        group_ids, group_rows, issues, manifests, intervals, endpoints = set(), [], set(), [], [], set()
        invalid = 0
        for file in files:
            if not file.resolve().is_relative_to(source_root):
                raise CoverageError("Intake manifest resolves outside the authorized source root.")
            data, rows = _document(file, schema, array)
            if type(data.get(count_field)) is not int or data[count_field] != len(rows):
                issues.add("declared_count_mismatch")
            if data.get("status") != complete_status:
                issues.add("incomplete_intake_status")
            if label == "gate09" and data.get("bundles_expected") != data.get("bundles_attempted"):
                issues.add("incomplete_bundle_attempts")
            if label == "additional":
                start, end = data.get("batch_start"), data.get("batch_end")
                if type(start) is not int or type(end) is not int or not 0 <= start < end:
                    raise CoverageError("Invalid additional intake batch interval.")
                intervals.append((start, end))
                expected_name = f"GATE12_ALL_INSTALLED_KK_ADDITIONAL_ANIMATIONS_PART_{start:04d}_{end:04d}.json"
                if file.name != expected_name or data.get("progress_next_index") != end:
                    issues.add("batch_not_finished_or_filename_mismatch")
                if type(data.get("additional_bundles")) is not int:
                    issues.add("missing_additional_bundle_total")
                else:
                    endpoints.add(data["additional_bundles"])
            manifest_ids = set()
            for row in rows:
                key = identity(row, hash_field, id_field)
                if key is None:
                    invalid += 1
                else:
                    group_ids.add(key); manifest_ids.add(key)
                curve_paths.check(row.get(curve_field))
            manifests.append({"manifest": file.relative_to(source_root).as_posix(),
                              "schema": schema, "rows": len(rows), "unique_actual_ids": len(manifest_ids)})
            group_rows.extend(rows)
        if label == "additional":
            cursor = 0
            for start, end in sorted(intervals):
                if start != cursor:
                    issues.add("batch_gap_or_overlap")
                cursor = max(cursor, end)
            if len(endpoints) != 1 or cursor not in endpoints:
                issues.add("additional_bundle_endpoint_mismatch")
        if invalid:
            issues.add("invalid_actual_identity_rows")
        groups[label] = {"rows": len(group_rows), "unique_actual_ids": len(group_ids),
                         "identity_fields": {"bundle_sha256": hash_field, "unity_path_id": id_field},
                         "rows_without_valid_actual_id": invalid,
                         "duplicate_identity_rows": len(group_rows) - invalid - len(group_ids),
                         "issues": sorted(issues), "manifests": manifests,
                         "metadata": metadata(group_rows)}
        if intervals:
            groups[label]["batch_intervals"] = [list(pair) for pair in sorted(intervals)]
        all_ids.update(group_ids); sets[label] = group_ids
        all_rows += len(group_rows); invalid_total += invalid

    _, motions = _document(catalog, CATALOG_SCHEMA, "motions")
    native_paths = PathAudit(_content_root(catalog, source_root))
    catalog_ids, native_present_ids, approved_ids = set(), set(), set()
    labels, native_references, arities = set(), set(), Counter()
    unkeyed = scene_complete = multi_missing = 0
    for row in motions:
        # Explicit bundle hash only: source_hash identifies the source clip in
        # this schema and cannot be substituted for source_bundle_sha256.
        key = identity(row, "source_bundle_sha256", "source_unity_path_id")
        if key is None:
            unkeyed += 1
        else:
            catalog_ids.add(key)
        if _nonempty(row.get("source_clip_id")):
            labels.add(row["source_clip_id"])
        value = row.get("native_sequence")
        if _nonempty(value):
            native_references.add(value)
        native_file = _native_file(value)
        state = native_paths.check(str(native_file)) if native_file is not None else native_paths.invalid()
        if key is not None and state == "present":
            native_present_ids.add(key)
        if key is not None and row.get("approved_for_gameplay") is True:
            approved_ids.add(key)
        arity = row.get("participants", 1)
        valid_arity = type(arity) is int and 1 <= arity <= 2**31 - 1
        arities[str(arity) if valid_arity else "invalid"] += 1
        complete = valid_arity and arity > 1 and _nonempty(row.get("scene_id")) and _nonempty(row.get("scene_role"))
        scene_complete += bool(complete)
        multi_missing += bool(valid_arity and arity > 1 and not complete)

    overlaps = {f"{a}__{b}": len(sets[a] & sets[b])
                for a, b in (("h", "gate09"), ("h", "additional"), ("gate09", "additional"))}
    return {
        "schema": RECEIPT_SCHEMA,
        "generated_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "identity": "declared_original_bundle_sha256_and_signed_unity_path_id",
        "acceptance": "metadata_coverage_only_not_native_or_gameplay_acceptance",
        "native_files_hashed": 0,
        "path_checks": "local_existence_only_no_curve_bundle_or_native_asset_contents_read",
        "privacy": "aggregate_counts_only_no_source_names_paths_curves_or_raw_ids",
        "inventory": {"rows": all_rows, "unique_actual_ids": len(all_ids),
            "rows_without_valid_actual_id": invalid_total,
            "duplicate_identity_rows": all_rows - invalid_total - len(all_ids),
            "pairwise_group_overlap": overlaps,
            "triple_group_overlap": len(sets["h"] & sets["gate09"] & sets["additional"]),
            "manifest_intake_complete": all(not g["issues"] for g in groups.values()),
            "intake_complete": all(not g["issues"] for g in groups.values()) and
                all(curve_paths.rows[state] == 0 for state in PATH_STATES if state != "present"),
            "groups": groups, "curve_paths": curve_paths.summary()},
        "catalog": {"schema": CATALOG_SCHEMA, "rows": len(motions),
            "unique_actual_ids": len(catalog_ids), "rows_without_valid_actual_id": unkeyed,
            "duplicate_identity_rows": len(motions) - unkeyed - len(catalog_ids),
            "unique_source_clip_labels": len(labels), "unique_native_references": len(native_references),
            "participants": dict(sorted(arities.items())), "metadata": metadata(motions),
            "rows_with_complete_scene_metadata": scene_complete,
            "multi_participant_rows_missing_scene_metadata": multi_missing,
            "native_paths": native_paths.summary()},
        "coverage": {"matched_unique_actual_ids": len(all_ids & catalog_ids),
            "inventory_ids_without_catalog_entry": len(all_ids - catalog_ids),
            "catalog_ids_outside_inventory": len(catalog_ids - all_ids),
            "matched_ids_with_existing_native_path": len(all_ids & native_present_ids),
            "matched_ids_declared_approved_in_catalog": len(all_ids & approved_ids),
            "approval_note": "existing_catalog_declarations_only_no_new_approval_or_playback_verification",
            "by_group": {label: {"matched": len(ids & catalog_ids), "without_catalog_entry": len(ids - catalog_ids)}
                         for label, ids in sets.items()}},
        "next_batch": {"unique_source_ids_without_catalog_entry": len(all_ids - catalog_ids),
            "scene_jobs_derivable_from_manifests": False,
            "required_original_metadata": ["participants", "scene_id", "scene_role", "anchor_relative_role_placements"],
            "instruction": "Recover original role/group/placement metadata before scheduling synchronized scene conversion; counts alone do not define scene jobs."},
    }


def write_receipt(output, receipt, source_root, catalog):
    output, source_root, catalog = local_path(output), local_path(source_root), local_path(catalog)
    if output.suffix.lower() != ".json" or output == catalog or any(
        output.is_relative_to(root.resolve()) for root in (source_root / "SourceAssets", _content_root(catalog, source_root))
    ):
        raise CoverageError("Receipt output must not overwrite source assets, intake manifests or catalog content.")
    if output.exists() and read_json(output).get("schema") != RECEIPT_SCHEMA:
        raise CoverageError("Refusing to overwrite a file that is not this tool's receipt.")
    # Serialize before opening the one user-selected output. No original or
    # native files are rewritten, and Windows receipt writes need no rename.
    payload = json.dumps(receipt, indent=2, sort_keys=True, allow_nan=False) + "\n"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(payload, encoding="utf-8")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", required=True, type=Path, help="Authorized local project containing frozen SourceAssets intake manifests.")
    parser.add_argument("--catalog", required=True, type=Path, help="Local SharedMotionCatalog.json; /Game paths resolve under its Content ancestor.")
    parser.add_argument("--output", required=True, type=Path, help="Aggregate JSON receipt, outside original source and Content directories.")
    args = parser.parse_args(argv)
    try:
        receipt = compare(args.source_root, args.catalog)
        write_receipt(args.output, receipt, args.source_root, args.catalog)
    except (CoverageError, OSError):
        print("Coverage comparison failed: invalid/inaccessible input or unsafe output; no source values are printed.", file=sys.stderr)
        return 2
    print(json.dumps({"inventory_unique_actual_ids": receipt["inventory"]["unique_actual_ids"],
                      "catalog_unique_actual_ids": receipt["catalog"]["unique_actual_ids"],
                      "matched_unique_actual_ids": receipt["coverage"]["matched_unique_actual_ids"],
                      "inventory_ids_without_catalog_entry": receipt["coverage"]["inventory_ids_without_catalog_entry"],
                      "intake_complete": receipt["inventory"]["intake_complete"],
                      "native_files_hashed": 0}, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
