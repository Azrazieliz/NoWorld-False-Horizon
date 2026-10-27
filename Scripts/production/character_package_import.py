"""Explicit, hash-locked native character packages; stdlib validation outside UE.

No module-level Unreal dependency. CLI is validation/dry-run only. See
docs/production/CHARACTER_ASSET_PACKAGE.md for the contract and engine limits.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tempfile


SCHEMA = Path(__file__).resolve().parents[2] / "schemas/character_asset_package.schema.json"
RECEIPT_SCHEMA = "noworld.character_asset_import_receipt.v1"
METADATA_KEY = "NoWorld.CharacterPackage.ManifestSHA256"
OBJECT_PATH = re.compile(r"/Game(?:/[A-Za-z0-9_]+)+\.[A-Za-z0-9_]+")
RESERVED = re.compile(r"(?:CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])(?:\..*)?", re.I)


class ValidationError(ValueError):
    """An invalid source/contract or unsafe import request; fails closed."""


class ImportFailure(RuntimeError):
    """Carries the partial receipt without claiming an Unreal transaction rollback."""

    def __init__(self, message, receipt):
        super().__init__(message)
        self.receipt = receipt


def _require(condition, message):
    if not condition:
        raise ValidationError(message)


def _json_pairs(pairs):
    result = {}
    for key, value in pairs:
        _require(key not in result, "Duplicate JSON key: " + key)
        result[key] = value
    return result


def _bad_constant(value):
    raise ValidationError("Nonfinite JSON number: " + value)


def load_manifest(path):
    """Read data only; duplicate keys and NaN/Infinity are invalid."""
    try:
        return json.loads(Path(path).read_text(encoding="utf-8-sig"),
                          object_pairs_hook=_json_pairs, parse_constant=_bad_constant)
    except (OSError, UnicodeError, ValueError) as exc:
        raise ValidationError("Cannot read manifest: " + str(exc)) from exc


def _canonical(value):
    try:
        return json.dumps(value, sort_keys=True, separators=(",", ":"),
                          ensure_ascii=False, allow_nan=False).encode("utf-8")
    except (TypeError, ValueError, OverflowError) as exc:
        raise ValidationError("Manifest must contain finite JSON data") from exc


def _finite(value):
    try:
        return type(value) in (int, float) and math.isfinite(value)
    except OverflowError:
        return False


def _shape(value, rule, schema, where="manifest"):
    """Evaluate the small documented JSON Schema vocabulary used by SCHEMA.

    This is deliberately not a general JSON Schema implementation. The shipped
    schema also works with an independent draft-2020-12 validator.
    """
    if "$ref" in rule:
        return _shape(value, schema["$defs"][rule["$ref"].split("/")[-1]], schema, where)
    types = {"object": lambda x: type(x) is dict,
             "array": lambda x: type(x) is list,
             "string": lambda x: type(x) is str,
             "boolean": lambda x: type(x) is bool,
             "integer": lambda x: type(x) is int,
             "number": _finite}
    if "type" in rule:
        _require(types[rule["type"]](value), where + ": expected " + rule["type"])
    if "const" in rule:
        _require(value == rule["const"], where + ": unsupported value")
    if "enum" in rule:
        _require(value in rule["enum"], where + ": unsupported value")
    if type(value) is dict:
        _require(all(type(k) is str for k in value), where + ": keys must be strings")
        for key in rule.get("required", []):
            _require(key in value, where + ": missing " + key)
        props = rule.get("properties", {})
        if rule.get("additionalProperties") is False:
            _require(not (value.keys() - props.keys()), where + ": unknown fields")
        _require(len(value) >= rule.get("minProperties", 0), where + ": empty object")
        for key in value.keys() & props.keys():
            _shape(value[key], props[key], schema, where + "." + key)
    elif type(value) is list:
        _require(len(value) >= rule.get("minItems", 0), where + ": too few entries")
        _require(len(value) <= rule.get("maxItems", len(value)), where + ": too many entries")
        if rule.get("uniqueItems"):
            _require(len({_canonical(x) for x in value}) == len(value), where + ": duplicate entries")
        for index, item in enumerate(value):
            _shape(item, rule.get("items", {}), schema, f"{where}[{index}]")
    elif type(value) is str:
        _require(bool(value.strip()), where + ": blank string")
        _require(len(value) >= rule.get("minLength", 0), where + ": short string")
        if "pattern" in rule:
            _require(re.fullmatch(rule["pattern"], value) is not None, where + ": invalid format")
    elif type(value) in (int, float):
        _require(_finite(value), where + ": nonfinite/oversized number")
        if "minimum" in rule:
            _require(value >= rule["minimum"], where + ": below minimum")
        if "exclusiveMinimum" in rule:
            _require(value > rule["exclusiveMinimum"], where + ": must be positive")


def _unique(rows, key, label):
    values = [row[key].casefold() for row in rows]
    _require(len(values) == len(set(values)), "Duplicate " + label)
    return {row[key]: row for row in rows}


def _names(names, label):
    _require(len({name.casefold() for name in names}) == len(names), "Duplicate " + label)
    _require(all(name.strip() == name and not any(ord(c) < 32 for c in name)
                 for name in names), "Invalid " + label)


def split_object_path(path):
    """Strict top-level soft object path -> (str folder, str asset name)."""
    _require(type(path) is str and OBJECT_PATH.fullmatch(path) is not None,
             "Invalid /Game/ soft object path: " + str(path))
    package, name = path.split(".")
    folder, leaf = package.rsplit("/", 1)
    _require(leaf == name, "Object name must equal package name: " + path)
    return folder, name


def _resolve_source(row, roots):
    _require(row["root"] in roots, "Missing explicit source root: " + row["root"])
    _require(isinstance(roots[row["root"]], (str, Path)), "Source roots must be directory paths")
    root = Path(roots[row["root"]])
    _require(root.is_absolute(), "Source roots must be absolute")
    path = row["path"]
    components = path.split("/")
    _require(all(part not in ("", ".", "..") and re.fullmatch(r"[A-Za-z0-9_.-]+", part)
                 and not part.endswith((".", " ")) and not RESERVED.fullmatch(part)
                 for part in components), "Unsafe source path: " + path)
    try:
        root = root.resolve(strict=True)
        _require(root.is_dir(), "Source root must be a directory")
        resolved = root.joinpath(*components).resolve(strict=True)
        _require(resolved.is_relative_to(root), "Source path escapes root: " + path)
        _require(resolved.is_file(), "Source is not a file: " + path)
    except (OSError, RuntimeError) as exc:
        raise ValidationError("Cannot resolve source: " + path) from exc
    return resolved


def _sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_manifest(manifest, source_roots):
    """Return a deterministic plan; no Unreal, writes, discovery or source execution.

    All source roots are caller-supplied absolute directories. Every source is
    byte-hashed, including evidence files. Validation is not native FBX inspection.
    """
    _canonical(manifest)
    schema = load_manifest(SCHEMA)
    _shape(manifest, schema, schema)
    doc = copy.deepcopy(manifest)
    _require(type(source_roots) is dict, "source_roots must be an explicit mapping")
    sources = _unique(doc["sources"], "id", "source id")
    textures = _unique(doc["textures"], "id", "texture id")
    materials = _unique(doc["materials"], "id", "material id")
    _unique(doc["wardrobes"], "id", "wardrobe id")
    physical_paths, verified, used_sources, used_textures, used_materials = set(), [], set(), set(), set()
    for source in doc["sources"]:
        path = _resolve_source(source, source_roots)
        key = str(path).casefold()
        _require(key not in physical_paths, "Duplicate source path: " + str(path))
        physical_paths.add(key)
        try:
            digest = _sha256(path)
        except OSError as exc:
            raise ValidationError("Cannot hash source: " + str(path)) from exc
        _require(digest == source["sha256"], "SHA256 mismatch: " + source["id"])
        verified.append(dict(source, resolved_path=str(path),
                             resolved_root=str(Path(source_roots[source["root"]]).resolve(strict=True))))
    proof_rows = _unique(doc["provenance"], "source_id", "provenance source")
    for row in doc["provenance"]:
        _require(row["source_id"] in sources and sources[row["source_id"]]["role"] != "provenance",
                 "Provenance must describe an import source")
        for sid in row["evidence_source_ids"]:
            _require(sid in sources and sources[sid]["role"] == "provenance",
                     "Evidence reference must name a provenance source")
            used_sources.add(sid)
    _require(set(proof_rows) == {s["id"] for s in doc["sources"] if s["role"] != "provenance"},
             "Every import source requires provenance")

    def source_ref(sid, role):
        _require(sid in sources and sources[sid]["role"] == role, "Wrong/missing source role: " + sid)
        _require(sid not in used_sources, "Source imported more than once: " + sid)
        used_sources.add(sid)
        return sources[sid]

    assets, output_paths, external_skeletons, skeleton_contracts = [], set(), [], {}

    def output(path, kind):
        split_object_path(path)
        _require(path.startswith(doc["destination_root"] + "/"), "Output outside destination_root: " + path)
        key = path.split(".")[0].casefold()
        _require(key not in output_paths, "Duplicate output package: " + path)
        output_paths.add(key)
        assets.append({"kind": kind, "object_path": path})

    for texture in doc["textures"]:
        source = source_ref(texture["source_id"], "texture")
        _require(Path(source["path"]).suffix.lower() in (".png", ".tga", ".jpg", ".jpeg", ".exr"),
                 "Unsupported texture extension")
        _require(not texture["srgb"] or texture["compression"] == "default", "Normal/mask textures require srgb=false")
        output(texture["object_path"], "texture")
    for material in doc["materials"]:
        output(material["object_path"], "material")
        for channel, tid in material["textures"].items():
            _require(tid in textures, "Missing texture reference: " + tid)
            used_textures.add(tid)
            texture = textures[tid]
            if channel == "normal":
                _require(texture["compression"] == "normalmap" and not texture["srgb"],
                         "Normal channel requires a linear normalmap texture")
            elif channel in ("roughness", "metallic", "opacity_mask"):
                _require(not texture["srgb"] and texture["compression"] != "normalmap",
                         "Scalar channels require linear textures")
            else:
                _require(texture["compression"] != "normalmap", "Color channels cannot sample normalmaps")
        _require(("opacity_mask" in material["textures"]) == (material["blend_mode"] == "masked"),
                 "Masked materials require exactly an opacity_mask channel")
    for wardrobe in doc["wardrobes"]:
        parts, skeleton = wardrobe["parts"], wardrobe["skeleton"]
        _unique(parts, "id", "part id in " + wardrobe["id"])
        _require(len(parts) == wardrobe["expected_part_count"], "Exact wardrobe part count mismatch")
        _names(skeleton["bone_names"], "bone names")
        _require(len(skeleton["bone_names"]) == skeleton["expected_bone_count"], "Exact bone count mismatch")
        path = skeleton["object_path"]
        split_object_path(path)
        if skeleton["mode"] == "create":
            folder, name = split_object_path(parts[0]["object_path"])
            expected = folder + "/" + name + "_Skeleton." + name + "_Skeleton"
            _require(path == expected, "Created skeleton must have exact first-part FBX skeleton name")
            output(path, "skeleton")
        else:
            key = path.casefold()
            contract = {k: v for k, v in skeleton.items() if k not in ("mode", "source_bone_count")}
            _require(key not in skeleton_contracts or skeleton_contracts[key] == contract,
                     "Shared skeleton contracts disagree")
            skeleton_contracts[key] = contract
            if path not in external_skeletons:
                external_skeletons.append(path)
        for part in parts:
            source = source_ref(part["source_id"], "skeletal_mesh")
            _require(Path(source["path"]).name == part["filename"], "Per-part filename mismatch")
            output(part["object_path"], "skeletal_mesh")
            _names(part["morph_targets"], "morph targets")
            _names([slot["name"] for slot in part["material_slots"]], "material slots")
            for slot in part["material_slots"]:
                _require(slot["material_id"] in materials, "Missing material reference: " + slot["material_id"])
                used_materials.add(slot["material_id"])
    for path in external_skeletons:
        _require(path.split(".")[0].casefold() not in output_paths, "Reuse skeleton collides with an output")
    _require(used_sources == set(sources), "Unreferenced manifest source")
    _require(used_textures == set(textures), "Unreferenced manifest texture")
    _require(used_materials == set(materials), "Unreferenced manifest material")
    return {"manifest": doc, "manifest_sha256": hashlib.sha256(_canonical(doc)).hexdigest(),
            "sources": verified, "assets": assets, "external_skeletons": external_skeletons}


def check_existing_assets(plan, inventory, policy="reject"):
    """Pure conflict policy. Preservation requires the entire stamped package.

    The Unreal adapter additionally verifies classes, paths and native contracts.
    There is intentionally no overwrite policy or partial resume.
    """
    _require(policy in ("reject", "preserve_verified"), "Unsupported existing-asset policy")
    rows = {key.casefold(): row for key, row in inventory.items()}
    present = [asset for asset in plan["assets"] if asset["object_path"].casefold() in rows]
    if not present:
        return "import"
    _require(policy == "preserve_verified", "Existing asset conflict; originals preserved")
    _require(len(present) == len(plan["assets"]), "Partial existing package; originals preserved")
    for asset in present:
        row = rows[asset["object_path"].casefold()]
        _require(row.get("manifest_sha256") == plan["manifest_sha256"] and row.get("kind") == asset["kind"],
                 "Existing asset provenance/class conflict: " + asset["object_path"])
    return "preserve"


def _receipt(plan, status):
    return {"schema": RECEIPT_SCHEMA, "status": status, "native_verified": False,
            "package_id": plan["manifest"]["package_id"],
            "character_id": plan["manifest"]["character_id"],
            "character_version": plan["manifest"]["character_version"],
            "manifest_sha256": plan["manifest_sha256"], "sources": copy.deepcopy(plan["sources"]),
            "provenance": copy.deepcopy(plan["manifest"]["provenance"]),
            "settings": copy.deepcopy(plan["manifest"]["settings"]),
            "wardrobes": copy.deepcopy(plan["manifest"]["wardrobes"]),
            "assets": copy.deepcopy(plan["assets"]), "completed": [], "observations": []}


def _set(obj, **properties):
    for key, value in properties.items():
        obj.set_editor_property(key, value)


def _load_exact(u, path, asset_class):
    asset = u.load_asset(path)
    _require(isinstance(asset, asset_class), "Missing/wrong Unreal class: " + path)
    _require(asset.get_path_name() == path, "Unexpected asset/redirector path: " + path)
    return asset


def _verify_skeleton(u, asset, contract):
    names = [str(name) for name in u.AnimPoseExtensions.get_bone_names(asset.get_reference_pose())]
    _require(names == contract["bone_names"], "Native skeleton bone order/names/count mismatch: " + contract["object_path"])
    return names


def _verify_native(u, plan, receipt):
    doc = plan["manifest"]
    textures = {row["id"]: _load_exact(u, row["object_path"], u.Texture2D) for row in doc["textures"]}
    materials = {row["id"]: _load_exact(u, row["object_path"], u.Material) for row in doc["materials"]}
    compression = {"default": u.TextureCompressionSettings.TC_DEFAULT,
                   "normalmap": u.TextureCompressionSettings.TC_NORMALMAP,
                   "masks": u.TextureCompressionSettings.TC_MASKS}
    for row in doc["textures"]:
        asset = textures[row["id"]]
        _require(asset.get_editor_property("srgb") == row["srgb"] and
                 asset.get_editor_property("compression_settings") == compression[row["compression"]],
                 "Native texture settings mismatch: " + row["id"])
    for row in doc["materials"]:
        asset = materials[row["id"]]
        expected = {textures[tid].get_path_name() for tid in row["textures"].values()}
        actual = {t.get_path_name() for t in u.MaterialEditingLibrary.get_used_textures(asset)}
        _require(actual == expected, "Native material texture references mismatch: " + row["id"])
        _require(asset.get_editor_property("two_sided") == row["two_sided"] and
                 asset.get_editor_property("blend_mode") == getattr(u.BlendMode, "BLEND_" + row["blend_mode"].upper()),
                 "Native material settings mismatch: " + row["id"])
    for wardrobe in doc["wardrobes"]:
        contract = wardrobe["skeleton"]
        skeleton = _load_exact(u, contract["object_path"], u.Skeleton)
        bones = _verify_skeleton(u, skeleton, contract)
        for part in wardrobe["parts"]:
            mesh = _load_exact(u, part["object_path"], u.SkeletalMesh)
            _require(mesh.get_editor_property("skeleton") == skeleton, "Native mesh skeleton reference mismatch")
            morphs = list(mesh.get_all_morph_target_names())
            _require(sorted(morphs) == sorted(part["morph_targets"]), "Exact native morph targets mismatch")
            slots = mesh.get_editor_property("materials")
            _require(len(slots) == len(part["material_slots"]), "Exact native material slot count mismatch")
            for slot, declared in zip(slots, part["material_slots"]):
                _require(str(slot.get_editor_property("material_slot_name")) == declared["name"], "Native material slot name mismatch")
                _require(slot.get_editor_property("material_interface") == materials[declared["material_id"]],
                         "Native mesh material reference mismatch")
            receipt["observations"].append({"wardrobe_id": wardrobe["id"], "part_id": part["id"],
                                            "object_path": mesh.get_path_name(), "bone_count": len(bones),
                                            "morph_targets": sorted(morphs), "material_slots": len(slots)})


def _task(u, filename, path, factory, options, allowed_paths):
    folder, name = split_object_path(path)
    task = u.AssetImportTask()
    # destination_path is FString: explicitly pass str, never Name/SoftObjectPath.
    _set(task, filename=str(filename), destination_path=str(folder), destination_name=str(name),
         factory=factory, options=options, automated=True, save=False, async_=False,
         replace_existing=False, replace_existing_settings=False)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    actual = {str(p) for p in task.get_editor_property("imported_object_paths")}
    _require(path in actual and actual <= set(allowed_paths), "Importer returned missing/undeclared object paths: " + repr(actual))


def _import_native(u, plan, staged, receipt):
    doc = plan["manifest"]
    tools = u.AssetToolsHelpers.get_asset_tools()
    compression = {"default": u.TextureCompressionSettings.TC_DEFAULT,
                   "normalmap": u.TextureCompressionSettings.TC_NORMALMAP,
                   "masks": u.TextureCompressionSettings.TC_MASKS}
    textures, materials = {}, {}
    for row in doc["textures"]:
        _task(u, staged[row["source_id"]], row["object_path"], u.TextureFactory(), None, [row["object_path"]])
        texture = _load_exact(u, row["object_path"], u.Texture2D)
        _set(texture, srgb=row["srgb"], compression_settings=compression[row["compression"]])
        textures[row["id"]] = texture
        receipt["completed"].append(row["object_path"])
    channels = {"base_color": "MP_BASE_COLOR", "normal": "MP_NORMAL", "roughness": "MP_ROUGHNESS",
                "metallic": "MP_METALLIC", "emissive_color": "MP_EMISSIVE_COLOR", "opacity_mask": "MP_OPACITY_MASK"}
    for row in doc["materials"]:
        folder, name = split_object_path(row["object_path"])
        material = tools.create_asset(str(name), str(folder), u.Material, u.MaterialFactoryNew())
        _require(material is not None and material.get_path_name() == row["object_path"], "Material creation failed")
        _set(material, two_sided=row["two_sided"],
             blend_mode=getattr(u.BlendMode, "BLEND_" + row["blend_mode"].upper()))
        for channel, tid in row["textures"].items():
            expression = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionTextureSample)
            _require(expression is not None, "Texture expression creation failed")
            _set(expression, texture=textures[tid])
            texture_row = next(t for t in doc["textures"] if t["id"] == tid)
            sampler = {"normalmap": "SAMPLERTYPE_NORMAL", "masks": "SAMPLERTYPE_MASKS"}.get(
                texture_row["compression"], "SAMPLERTYPE_COLOR" if texture_row["srgb"] else "SAMPLERTYPE_LINEAR_COLOR")
            _set(expression, sampler_type=getattr(u.MaterialSamplerType, sampler))
            output = "R" if channel in ("roughness", "metallic", "opacity_mask") else "RGB"
            _require(u.MaterialEditingLibrary.connect_material_property(
                expression, output, getattr(u.MaterialProperty, channels[channel])), "Material connection failed")
        u.MaterialEditingLibrary.recompile_material(material)
        materials[row["id"]] = material
        receipt["completed"].append(row["object_path"])
    settings = doc["settings"]
    for wardrobe in doc["wardrobes"]:
        contract = wardrobe["skeleton"]
        skeleton = _load_exact(u, contract["object_path"], u.Skeleton) if contract["mode"] == "reuse" else None
        for part in wardrobe["parts"]:
            ui = u.FbxImportUI()
            _set(ui, automated_import_should_detect_type=False,
                 mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH,
                 import_as_skeletal=True, import_mesh=True, import_animations=False,
                 import_materials=False, import_textures=False, create_physics_asset=False,
                 override_full_name=True, skeleton=skeleton)
            data = ui.get_editor_property("skeletal_mesh_import_data")
            _set(data, import_morph_targets=True, import_mesh_lods=False,
                 update_skeleton_reference_pose=False, use_t0_as_ref_pose=False,
                 import_uniform_scale=float(settings["uniform_scale"]),
                 convert_scene=settings["convert_scene"], convert_scene_unit=settings["convert_scene_unit"],
                 force_front_x_axis=settings["force_front_x_axis"],
                 import_translation=u.Vector(*settings["translation"]),
                 import_rotation=u.Rotator(*settings["rotation"]))
            allowed = [part["object_path"]]
            if skeleton is None:
                allowed.append(contract["object_path"])
            _task(u, staged[part["source_id"]], part["object_path"], u.FbxFactory(), ui, allowed)
            mesh = _load_exact(u, part["object_path"], u.SkeletalMesh)
            skeleton = _load_exact(u, contract["object_path"], u.Skeleton)
            _verify_skeleton(u, skeleton, contract)
            slots = mesh.get_editor_property("materials")
            _require(len(slots) == len(part["material_slots"]), "Exact imported material slot count mismatch")
            for slot, declared in zip(slots, part["material_slots"]):
                _require(str(slot.get_editor_property("material_slot_name")) == declared["name"], "Imported material slot name mismatch")
                slot.set_editor_property("material_interface", materials[declared["material_id"]])
            mesh.set_editor_property("materials", slots)
            receipt["completed"].append(part["object_path"])


def _stage_sources(plan, directory):
    """Private verified snapshots prevent later original-file edits changing imports.

    Snapshot paths stay isolated per source; no neighboring source discovery.
    Only importable entries are copied. Evidence is hashed again, never executed.
    """
    staged = {}
    for row in plan["sources"]:
        path = _resolve_source(row, {row["root"]: Path(row["resolved_root"])})
        _require(str(path) == row["resolved_path"], "Source path changed before import: " + row["id"])
        if row["role"] == "provenance":
            _require(_sha256(path) == row["sha256"], "SHA256 changed before import: " + row["id"])
            continue
        target = directory / row["id"] / Path(row["path"]).name
        target.parent.mkdir()
        digest = hashlib.sha256()
        with path.open("rb") as source, target.open("xb") as destination:
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(chunk)
                destination.write(chunk)
        _require(digest.hexdigest() == row["sha256"], "SHA256 changed before import: " + row["id"])
        staged[row["id"]] = target
    return staged


def import_package(manifest, source_roots, *, dry_run=True, existing_policy="reject"):
    """Validate, then optionally import from the Unreal Editor Python environment.

    Dry-run is the default and never imports unreal. Native failures raise
    ImportFailure with a serializable .receipt; no asset is overwritten/deleted.
    """
    plan = validate_manifest(manifest, source_roots)
    _require(type(dry_run) is bool, "dry_run must be boolean")
    check_existing_assets(plan, {}, existing_policy)
    receipt = _receipt(plan, "DRY_RUN_VALIDATED" if dry_run else "PREFLIGHT")
    receipt["existing_policy"] = existing_policy
    if dry_run:
        receipt["native_preflight"] = "NOT_RUN: requires Unreal asset registry"
        return receipt
    try:
        import unreal as u
        classes = {"texture": u.Texture2D, "material": u.Material,
                   "skeleton": u.Skeleton, "skeletal_mesh": u.SkeletalMesh}
        inventory = {}
        for row in plan["assets"]:
            path = row["object_path"]
            if u.EditorAssetLibrary.does_asset_exist(path):
                asset = _load_exact(u, path, classes[row["kind"]])
                inventory[path] = {"manifest_sha256": str(u.EditorAssetLibrary.get_metadata_tag(asset, METADATA_KEY)),
                                   "kind": row["kind"]}
        action = check_existing_assets(plan, inventory, existing_policy)
        for wardrobe in plan["manifest"]["wardrobes"]:
            contract = wardrobe["skeleton"]
            if contract["mode"] == "reuse":
                _verify_skeleton(u, _load_exact(u, contract["object_path"], u.Skeleton), contract)
        if action == "preserve":
            _verify_native(u, plan, receipt)
            receipt.update(status="PRESERVED_VERIFIED", native_verified=True)
            return receipt
        with tempfile.TemporaryDirectory(prefix="noworld_character_") as directory:
            staged = _stage_sources(plan, Path(directory))
            receipt["status"] = "IMPORTING"
            _import_native(u, plan, staged, receipt)
            _verify_native(u, plan, receipt)
            # Rebind import-data filenames to verified originals, not expired snapshots.
            sources = {s["id"]: s for s in plan["sources"]}
            rows = list(plan["manifest"]["textures"])
            rows += [p for w in plan["manifest"]["wardrobes"] for p in w["parts"]]
            for row in rows:
                asset = u.load_asset(row["object_path"])
                asset.get_editor_property("asset_import_data").scripted_update_filename_only(
                    sources[row["source_id"]]["resolved_path"])
            for row in plan["assets"]:
                asset = _load_exact(u, row["object_path"], classes[row["kind"]])
                u.EditorAssetLibrary.set_metadata_tag(asset, METADATA_KEY, plan["manifest_sha256"])
                _require(u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False),
                         "Native asset save failed: " + row["object_path"])
        receipt.update(status="IMPORTED_VERIFIED", native_verified=True)
        return receipt
    except Exception as exc:
        receipt.update(status="FAILED", native_verified=False, error=str(exc),
                       recovery="No rollback claimed. Inspect newly created unsaved/partial assets; reruns reject conflicts.")
        raise ImportFailure(str(exc), receipt) from exc


def main(argv=None):
    parser = argparse.ArgumentParser(description="Validate a character package; dry-run only, no Unreal import")
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--source-root", action="append", default=[], metavar="ID=ABSOLUTE_DIRECTORY")
    parser.add_argument("--existing-policy", choices=("reject", "preserve_verified"), default="reject")
    args = parser.parse_args(argv)
    try:
        roots = {}
        for item in args.source_root:
            key, separator, path = item.partition("=")
            _require(separator and key and path and key not in roots, "Expected unique ID=ABSOLUTE_DIRECTORY")
            roots[key] = Path(path)
        result = import_package(load_manifest(args.manifest), roots, existing_policy=args.existing_policy)
        print(json.dumps(result, indent=2, ensure_ascii=False, allow_nan=False))
        return 0
    except (ValidationError, OSError) as exc:
        import sys
        print("Character package rejected: " + str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
