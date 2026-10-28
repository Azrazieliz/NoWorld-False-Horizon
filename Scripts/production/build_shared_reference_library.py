#!/usr/bin/env python3
"""Build source-unique reference packets from repository-available calibrated data.

This is a shared reference library, not complete original Unity source recovery.
Legacy inputs contain relative rotation deltas; missing translation/scale/event
channels are never synthesized. Source files and native assets are never edited.
"""
import argparse
from collections import defaultdict
import gzip
import hashlib
import io
import json
import math
import os
from pathlib import Path
import re
import struct
import sys
import tempfile
import zlib

SOURCE_SCHEMA = "noworld.gate12.koikatsu.uncooked_original_studio_unity_native_motion.v1"
PACKET_SCHEMA = "noworld.shared_reference_motion.v1"
LIBRARY_SCHEMA = "noworld.shared_reference_library.v1"
CATALOG_SCHEMA = "noworld.shared_motion_catalog.v1"
MAGIC = b"NWMRJ001"
HEADER = struct.Struct("<8sII")
MAX_PLAIN_BYTES = 64 * 1024 * 1024
DEFAULT_SOURCE = Path("SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks")
DEFAULT_CATALOG = Path("Content/NoWorld/Animation/SharedMotionCatalog.json")
DEFAULT_OUTPUT = Path("Content/NoWorld/Animation/SharedSource")
MANIFEST_NAME = "SharedReferenceLibrary.json"
SOURCE_SUFFIX = ".genuine_unity_kk_stream_native_deltas.json.gz"
ID_RE = re.compile(r"kk\.[0-9a-f]{64}\.[0-9a-f]{16}\Z")


class LibraryError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise LibraryError(message)


def _pairs(pairs):
    value = {}
    for key, item in pairs:
        require(key not in value, "Duplicate JSON member.")
        value[key] = item
    return value


def _constant(_):
    raise LibraryError("Nonfinite JSON constant.")


def parse_json(data):
    try:
        return json.loads(data, object_pairs_hook=_pairs, parse_constant=_constant)
    except (ValueError, UnicodeError, RecursionError) as error:
        if isinstance(error, LibraryError):
            raise
        raise LibraryError("Invalid JSON input.") from None


def json_bytes(value):
    try:
        return json.dumps(value, sort_keys=True, ensure_ascii=False,
                          separators=(",", ":"), allow_nan=False).encode("utf-8")
    except (ValueError, UnicodeError, RecursionError):
        raise LibraryError("Cannot encode packet JSON.") from None


def sha(value):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-fA-F]{64}", value) is not None


def signed_id(value, allow_string=False):
    if allow_string and isinstance(value, str):
        require(re.fullmatch(r"-?(?:0|[1-9][0-9]*)", value) is not None, "Invalid decimal path ID.")
        value = int(value)
    require(type(value) is int and -(1 << 63) <= value < (1 << 63), "Path ID must be a signed int64, never a float or bool.")
    return value


def source_identity(bundle_sha, path_id, allow_string=False):
    require(sha(bundle_sha), "Missing original bundle SHA-256.")
    number = signed_id(path_id, allow_string)
    return "kk." + bundle_sha.lower() + "." + format(number & ((1 << 64) - 1), "016x")


def finite_number(value, label, positive=False):
    require(type(value) in (int, float) and math.isfinite(value), label + " must be finite.")
    require(not positive or value > 0, label + " must be positive.")
    return value


def bone_name(value):
    require(isinstance(value, str) and value and len(value) <= 1024 and
            not any(ord(c) < 32 for c in value), "Invalid source bone/alias name.")
    return value


def samples(value, count, width, label):
    require(isinstance(value, list) and len(value) == count, label + " frame count mismatch.")
    for sample in value:
        require(isinstance(sample, list) and len(sample) == width, label + " width mismatch.")
        for component in sample:
            finite_number(component, label)
            require(abs(component) <= 3.4028234663852886e38, "Sample exceeds native float range.")
        if width == 4:
            norm = sum(component * component for component in sample)
            require(abs(norm - 1.0) <= 0.01, "Invalid source quaternion; normalization is not synthesized.")
    return value


def validate_motion(motion):
    require(isinstance(motion, dict) and motion.get("schema") == PACKET_SCHEMA, "Unsupported packet schema.")
    require(isinstance(motion.get("id"), str) and ID_RE.fullmatch(motion["id"]), "Invalid packet ID.")
    rate = finite_number(motion.get("rate"), "rate", True)
    duration = finite_number(motion.get("duration"), "duration")
    require(duration >= 0, "duration must be nonnegative.")
    frames = motion.get("frames")
    require(type(frames) is int and 1 <= frames <= 1000000, "frames must fit the native decoded clip limit.")
    require(abs((frames - 1) / rate - duration) <= 1.0 / rate + 1e-6, "Source rate/duration/frame extent mismatch.")
    tracks = motion.get("tracks")
    require(isinstance(tracks, list) and 1 <= len(tracks) <= 8192, "Packet source track count exceeds native limits.")
    names = set()
    channel_counts = {"translation": 0, "scale": 0}
    for track in tracks:
        require(isinstance(track, dict), "Invalid track.")
        name = bone_name(track.get("bone"))
        require(name.casefold() not in names, "Duplicate case-insensitive source bone.")
        names.add(name.casefold())
        aliases = track.get("aliases")
        require(isinstance(aliases, list) and all(isinstance(a, str) for a in aliases), "Invalid aliases.")
        for alias in aliases:
            bone_name(alias)
        samples(track.get("rotations"), frames, 4, "rotations")
        for field, channel in (("translations", "translation"), ("scales", "scale")):
            if field in track:
                samples(track[field], frames, 3, field)
                channel_counts[channel] += 1
    absent = motion.get("source_fields_originally_absent")
    require(isinstance(absent, list) and len(absent) == len(set(absent)) and
            set(absent) == {k for k, count in channel_counts.items() if count == 0},
            "Missing-channel declaration does not match actual source data.")
    provenance = motion.get("provenance")
    require(isinstance(provenance, dict), "Missing packet provenance.")
    expected_id = source_identity(provenance.get("source_bundle_sha256"),
                                  provenance.get("source_path_id"), True)
    require(expected_id == motion["id"], "Packet ID/provenance mismatch.")
    require(motion.get("source_data_complete") is False, "Calibrated references must not claim complete original source.")
    return motion


def make_motion(document, input_sha):
    require(isinstance(document, dict) and document.get("schema") == SOURCE_SCHEMA, "Unsupported calibrated source schema.")
    require(document.get("relative_source_pose_only") is True, "Expected an explicitly relative calibrated reference.")
    clip_id = source_identity(document.get("source_sha256"), document.get("source_path_id"))
    require(sha(document.get("source_clip_sha256")) and sha(input_sha), "Missing declared source provenance.")
    family = document.get("rig_family")
    require(family is None or (isinstance(family, str) and bool(family.strip())), "Invalid declared original rig family.")
    frames = document.get("frame_count")
    require(type(frames) is int and frames >= 1, "Invalid calibrated frame count.")
    bones = document.get("bones")
    require(isinstance(bones, list) and bones, "No original source bones.")
    tracks = []
    for source in bones:
        require(isinstance(source, dict), "Invalid source bone record.")
        # Unknown channel spellings must not silently become 'originally absent'.
        require(not any(k in source for k in ("translation", "position", "positions", "scale")),
                "Unsupported channel spelling; preserve/recover its source schema first.")
        name = bone_name(source.get("source_bone"))
        alias = source.get("target_bone")
        require(alias is None or isinstance(alias, str), "Invalid declared target alias.")
        track = {"bone": name, "aliases": [bone_name(alias)] if alias else [],
                 "rotations": source.get("deltas")}
        for field in ("translations", "scales"):
            if field in source:
                track[field] = source[field]
        tracks.append(track)
    tracks.sort(key=lambda t: t["bone"])
    absent = [channel for field, channel in (("translations", "translation"), ("scales", "scale"))
              if not any(field in track for track in tracks)]
    motion = {"schema": PACKET_SCHEMA, "id": clip_id,
              "rate": document.get("frame_rate"), "duration": document.get("original_duration_s"),
              "frames": frames, "tracks": tracks,
              "source_fields_originally_absent": absent,
              "relative_source_pose_only": True, "source_data_complete": False,
              "provenance": {"source_bundle_sha256": document["source_sha256"].lower(),
                             "source_path_id": str(document["source_path_id"]),
                             "source_clip_sha256": [document["source_clip_sha256"].lower()],
                             "calibrated_input_sha256": [input_sha.lower()],
                             "original_rig_families": [family] if family else [],
                             "source_schema": SOURCE_SCHEMA,
                             "source_events_available": False}}
    return validate_motion(motion)


def encode_packet(motion):
    validate_motion(motion)
    plain = json_bytes(motion)
    require(len(plain) <= MAX_PLAIN_BYTES, "Packet exceeds the bounded plain-byte limit.")
    return HEADER.pack(MAGIC, len(plain), zlib.crc32(plain) & 0xffffffff) + zlib.compress(plain, 9)


def decode_packet(packet):
    require(isinstance(packet, bytes) and len(packet) > HEADER.size, "Truncated packet.")
    magic, length, checksum = HEADER.unpack_from(packet)
    require(magic == MAGIC and 0 < length <= MAX_PLAIN_BYTES, "Invalid packet header.")
    try:
        decoder = zlib.decompressobj()
        plain = decoder.decompress(packet[HEADER.size:], length + 1)
        require(decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail and
                len(plain) == length, "Packet compressed length or stream boundary mismatch.")
    except zlib.error:
        raise LibraryError("Invalid zlib stream.") from None
    require((zlib.crc32(plain) & 0xffffffff) == checksum, "Packet CRC32 mismatch.")
    return validate_motion(parse_json(plain))


def _core(motion):
    # Different declared aliases/provenance may coalesce, different motion may not.
    result = {key: value for key, value in motion.items() if key != "provenance"}
    result["tracks"] = [{key: value for key, value in track.items() if key != "aliases"}
                        for track in motion["tracks"]]
    return json_bytes(result)


def merge_motion(previous, current):
    require(_core(previous) == _core(current), "Conflicting calibrated motion for one original source ID.")
    result = previous
    for left, right in zip(result["tracks"], current["tracks"]):
        left["aliases"] = sorted(set(left["aliases"]) | set(right["aliases"]))
    for field in ("source_clip_sha256", "calibrated_input_sha256", "original_rig_families"):
        result["provenance"][field] = sorted(set(result["provenance"][field]) |
                                            set(current["provenance"][field]))
    return validate_motion(result)


def read_calibrated(path):
    compressed = path.read_bytes()
    try:
        with gzip.GzipFile(fileobj=io.BytesIO(compressed), mode="rb") as stream:
            plain = stream.read(MAX_PLAIN_BYTES + 1)
    except (OSError, EOFError, zlib.error):
        raise LibraryError("Unreadable calibrated gzip packet.") from None
    require(len(plain) <= MAX_PLAIN_BYTES, "Calibrated input exceeds the plain-byte limit.")
    return make_motion(parse_json(plain), hashlib.sha256(compressed).hexdigest())


def catalog_identity(row):
    # source_hash in this catalog identifies a different source object. Do not
    # use it as a substitute for the explicit original bundle SHA.
    if not sha(row.get("source_bundle_sha256")) or "source_unity_path_id" not in row:
        return None
    return source_identity(row["source_bundle_sha256"], row["source_unity_path_id"], True)


def route_from_row(row, clip_id):
    require(isinstance(row.get("action"), str) and row["action"].strip() and
            isinstance(row.get("context"), str) and row["context"].strip(),
            "An available-source route lacks a declared action/context.")
    participants = row.get("participants", 1)
    require(type(participants) is int and 1 <= participants <= 0x7fffffff, "Invalid route participant count.")
    rig, anatomy = row.get("rig", ""), row.get("anatomy", "")
    require(isinstance(rig, str) and rig.strip() and isinstance(anatomy, str), "Invalid declared rig/anatomy.")
    scene_id, scene_role = row.get("scene_id", ""), row.get("scene_role", "")
    require(isinstance(scene_id, str) and isinstance(scene_role, str), "Invalid declared scene metadata.")
    require((not scene_id and not scene_role and participants == 1) or
            (bool(scene_id) and bool(scene_role) and participants > 1),
            "Inconsistent source scene metadata; scene IDs/roles cannot be invented.")
    caps = row.get("capabilities", [])
    require(isinstance(caps, list) and all(isinstance(c, str) and c for c in caps), "Invalid declared capabilities.")
    priority = row.get("priority", 0)  # priority_if_approved is not the active legacy priority.
    require(type(priority) is int and -(1 << 31) <= priority < (1 << 31), "Invalid route priority.")
    looping = row.get("loop", False)
    approved = row.get("approved_for_gameplay", False)
    require(type(looping) is bool and type(approved) is bool, "Legacy loop/approval metadata must be boolean.")
    play_rate = finite_number(row.get("play_rate", 1), "play_rate", True)
    legacy = row.get("source_clip_id", "")
    require(isinstance(legacy, str), "Invalid legacy source ID.")
    result = {"action": row["action"], "context": row["context"], "clip": clip_id,
            "participants": participants, "scene_id": scene_id, "scene_role": scene_role,
            "rig": rig, "anatomy": anatomy, "capabilities": sorted(set(caps)),
            "priority": priority, "loop": looping, "play_rate": play_rate,
            "approved_legacy": approved, "legacy_source_id": legacy}
    if "contact_notify" in row:
        require(isinstance(row["contact_notify"], str), "Contact-notify metadata must be a string.")
        result["contact_notify"] = row["contact_notify"]
    for field in ("mature_content", "adult_restricted"):
        if field in row:
            require(type(row[field]) is bool, "Source content classification must be boolean.")
            result[field] = row[field]
    if "adult_restricted" in row and "mature_content" not in row:
        result["mature_content"] = row["adult_restricted"]  # Original catalog's classification fallback.
    if "root_motion" in row:
        require(type(row["root_motion"]) is bool, "Root-motion metadata must be boolean.")
        result["root_motion"] = row["root_motion"]
    if "contact_seconds" in row:
        result["contact_seconds"] = finite_number(row["contact_seconds"], "contact_seconds")
    if "morph_tracks" in row:
        require(isinstance(row["morph_tracks"], list) and all(isinstance(t, dict) for t in row["morph_tracks"]), "Invalid morph-track metadata.")
        result["morph_tracks"] = parse_json(json_bytes(row["morph_tracks"]))
    return result


def _write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=".nwmrj-", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def _clip_path(root, entry):
    clip_id, relative = entry.get("id"), entry.get("path")
    require(isinstance(clip_id, str) and ID_RE.fullmatch(clip_id), "Invalid manifest clip ID.")
    require(relative == "Clips/" + clip_id + ".nwmotion", "Noncanonical or unsafe packet path.")
    path = root / relative
    require(path.resolve().is_relative_to(root.resolve()), "Packet symlink escapes the output root.")
    return path


def validate_library(output_root):
    root = Path(output_root).resolve()
    manifest = parse_json((root / MANIFEST_NAME).read_bytes())
    require(isinstance(manifest, dict) and manifest.get("schema") == LIBRARY_SCHEMA, "Unsupported library schema.")
    require(isinstance(manifest.get("clips"), list) and manifest["clips"], "Empty or invalid library.")
    require(isinstance(manifest.get("routes"), list), "Invalid library routes.")
    ids, paths = set(), set()
    for entry in manifest["clips"]:
        require(isinstance(entry, dict), "Invalid clip entry.")
        path = _clip_path(root, entry)
        require(entry["id"] not in ids and path not in paths, "Duplicate library source identity/path.")
        ids.add(entry["id"]); paths.add(path)
        motion = decode_packet(path.read_bytes())
        require(motion["id"] == entry["id"] and motion["duration"] == entry.get("duration") and
                motion["rate"] == entry.get("rate"), "Manifest/packet metadata mismatch.")
        require(isinstance(entry.get("source_path_id"), str) and
                source_identity(entry.get("source_hash"), entry["source_path_id"], True) == entry["id"],
                "Manifest provenance mismatch or nonstring int64 path ID.")
        profiles = entry.get("rig_profiles")
        require(isinstance(profiles, list) and all(isinstance(p, str) and p for p in profiles),
                "Invalid declared rig profiles.")
    actual = set((root / "Clips").rglob("*.nwmotion"))
    require(actual == paths, "Unlisted or missing generated packet.")
    for route in manifest["routes"]:
        require(isinstance(route, dict) and route.get("clip") in ids, "Dangling route clip.")
        expected = route_from_row({
            "action": route.get("action"), "context": route.get("context"),
            "participants": route.get("participants"), "scene_id": route.get("scene_id"),
            "scene_role": route.get("scene_role"), "rig": route.get("rig"),
            "anatomy": route.get("anatomy"), "capabilities": route.get("capabilities"),
            "priority": route.get("priority"), "loop": route.get("loop"),
            "play_rate": route.get("play_rate"), "approved_for_gameplay": route.get("approved_legacy"),
            "source_clip_id": route.get("legacy_source_id"),
            **{k: route[k] for k in ("root_motion", "contact_seconds", "morph_tracks", "contact_notify", "mature_content", "adult_restricted") if k in route}},
            route["clip"])
        require(expected == route, "Invalid route fields.")
    return manifest


def build_library(source_root=DEFAULT_SOURCE, catalog_path=DEFAULT_CATALOG, output_root=DEFAULT_OUTPUT):
    source_root, catalog_path, output_root = Path(source_root).resolve(), Path(catalog_path).resolve(), Path(output_root).resolve()
    require(not output_root.is_relative_to(source_root) and not source_root.is_relative_to(output_root) and
            not catalog_path.is_relative_to(output_root), "Output overlaps original inputs.")
    files = sorted(source_root.rglob("*" + SOURCE_SUFFIX))
    require(files, "No repository-available calibrated source packets.")
    catalog = parse_json(catalog_path.read_bytes())
    require(isinstance(catalog, dict) and catalog.get("schema") == CATALOG_SCHEMA and
            isinstance(catalog.get("motions"), list), "Unsupported source catalog.")
    packets, profiles = {}, defaultdict(set)
    for path in files:
        require(path.resolve().is_relative_to(source_root), "Source symlink escapes the source root.")
        motion = read_calibrated(path)
        clip_id = motion["id"]
        if clip_id in packets:
            motion = merge_motion(decode_packet(packets[clip_id]), motion)
        packets[clip_id] = encode_packet(motion)
        profiles[clip_id].update(motion["provenance"]["original_rig_families"])
    routes, available_rows, unkeyed, unavailable = {}, 0, 0, 0
    for row in catalog["motions"]:
        require(isinstance(row, dict), "Catalog row must be an object.")
        clip_id = catalog_identity(row)
        if clip_id is None:
            unkeyed += 1
            continue
        if clip_id not in packets:
            unavailable += 1
            continue
        available_rows += 1
        route = route_from_row(row, clip_id)
        profiles[clip_id].add(route["rig"])
        # Coalesce wardrobe derivatives only when all actual route semantics
        # agree; neither approval nor capabilities are unioned into new grants.
        routes[json_bytes(route)] = route
    clips = []
    for clip_id in sorted(packets):
        motion = decode_packet(packets[clip_id])
        clips.append({"id": clip_id, "path": "Clips/" + clip_id + ".nwmotion",
                      "rig_profiles": sorted(profiles[clip_id]), "duration": motion["duration"],
                      "rate": motion["rate"], "source_hash": motion["provenance"]["source_bundle_sha256"],
                      "source_path_id": motion["provenance"]["source_path_id"]})
    manifest = {"schema": LIBRARY_SCHEMA, "clips": clips,
                "routes": [routes[key] for key in sorted(routes)],
                "coverage": {"repository_input_files": len(files), "source_unique_clips": len(clips),
                             "catalog_rows": len(catalog["motions"]), "catalog_rows_with_available_source": available_rows,
                             "catalog_rows_without_actual_id": unkeyed, "catalog_rows_without_available_source": unavailable,
                             "coalesced_semantic_routes": len(routes),
                             "source_inventory_complete": False, "native_acceptance": False,
                             "boundary": "repository_available_relative_reference_channels_only_no_missing_source_synthesis"}}
    # Inputs and all packets validate before any generated file is changed.
    previous = []
    old_manifest = output_root / MANIFEST_NAME
    if old_manifest.exists():
        previous = validate_library(output_root)["clips"]
    for entry in clips:
        path = _clip_path(output_root, entry)
        _write(path, packets[entry["id"]])
    _write(old_manifest, json.dumps(manifest, sort_keys=True, indent=2, ensure_ascii=False, allow_nan=False).encode("utf-8") + b"\n")
    current_ids = set(packets)
    for entry in previous:
        if entry["id"] not in current_ids:
            _clip_path(output_root, entry).unlink()
    validate_library(output_root)  # Reopen and verify EVERY emitted length/CRC/schema.
    return manifest


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--validate-only", action="store_true")
    args = parser.parse_args(argv)
    try:
        manifest = validate_library(args.output_root) if args.validate_only else build_library(args.source_root, args.catalog, args.output_root)
    except (LibraryError, OSError, EOFError) as error:
        print("Shared reference library failed: " + str(error), file=sys.stderr)
        return 2
    print(json.dumps({"schema": LIBRARY_SCHEMA, "clips": len(manifest["clips"]),
                      "routes": len(manifest["routes"]), "coverage": manifest.get("coverage", {}),
                      "all_packet_crc_checks_passed": True}, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
