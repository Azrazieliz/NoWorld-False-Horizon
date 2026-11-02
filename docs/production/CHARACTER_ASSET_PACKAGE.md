# Native character asset packages

`Scripts/production/character_package_import.py` implements a reusable, data-only
ingestion contract. It contains no character registry, canon, content approvals,
wardrobe defaults or hardcoded production bone counts. Existing one-off importers
remain unchanged. Their `AssetImportTask` / `FbxImportUI` pattern is retained, but
they are not imported because they execute Unreal operations at module load.

The validator uses Python 3.9+ and the standard library. The native adapter requires
Unreal Editor Python, Editor Scripting Utilities, the legacy FBX factories and
Animation Blueprint Library (`AnimPoseExtensions`). This engineering change does
not establish that any production asset has passed an Unreal import or runtime QA.

## Validate without Unreal

Run from the repository directory:

```powershell
python -B Scripts/production/character_package_import.py C:/packages/character.json --source-root exported=D:/ApprovedExports --source-root evidence=D:/SourceReceipts
```

The CLI always performs a dry-run. There is no CLI switch that launches Unreal or
imports assets. It prints a JSON provenance receipt and exits `0` on validation
success, `2` on rejection. It neither creates nor changes a receipt file; capture
stdout to an explicitly chosen location when a saved receipt is required.

The Python interface is:

```python
from pathlib import Path
from character_package_import import load_manifest, validate_manifest, import_package

manifest = load_manifest(Path("C:/packages/character.json"))
roots = {"exported": Path("D:/ApprovedExports"), "evidence": Path("D:/SourceReceipts")}
plan = validate_manifest(manifest, roots)
receipt = import_package(manifest, roots)  # dry_run=True by default
```

Source roots are supplied by the caller, never by the manifest, environment,
current directory or an embedded script. Keep the importer and its adjacent schema
at their repository locations. Its schema path is resolved relative to the script.

## Manifest contract

The authoritative shape is
[`schemas/character_asset_package.schema.json`](../../schemas/character_asset_package.schema.json),
draft 2020-12. Unknown fields are errors at every contract level. The stdlib
validator evaluates the subset of schema vocabulary actually used in this file,
then checks filesystem and cross-reference semantics. It is not a general-purpose
JSON Schema library. Independent JSON Schema validation checks shape only and
cannot replace the Python validation.

| Field | Required meaning |
| --- | --- |
| `schema_version` | Exactly `"1.0"`. |
| `package_id` | Caller-owned package identifier. |
| `character_id`, `character_version` | Existing source identity/version; no automatic canon registration. |
| `destination_root` | Exact `/Game/` folder containing every newly created asset. |
| `sources` | Explicit source IDs, root aliases, relative filenames, roles, versions and SHA256 values. |
| `provenance` | One row per importable source, linking hashed evidence source IDs and a source-derived statement. |
| `settings` | Explicit scale, coordinate conversion, translation and rotation. |
| `textures` | Declared native texture outputs and source references. |
| `materials` | Declared native material outputs and channel-to-texture references. |
| `wardrobes` | Independently versioned part lists and exact skeleton contracts. |

All lists are explicit; array order determines deterministic import order. JSON
object key order does not affect the canonical manifest SHA256. The receipt also
records the resolved source locations, so the overall receipt changes if root
locations change even though the manifest fingerprint is unchanged.

### Sources and evidence

Every source entry contains exactly `id`, `root`, `path`, `role`, `version` and
`sha256`. Roles are `skeletal_mesh`, `texture`, or `provenance`. A mesh part must
reference an FBX source. A texture must reference PNG, TGA, JPG/JPEG or EXR bytes.
Each importable source is used exactly once; share a declared texture ID across
materials rather than importing the same source twice. Unused sources, materials
or textures are rejected. Evidence sources are hashed and referenced but never
imported, parsed as commands or executed.

Paths use forward slashes and ASCII letters, digits, `_`, `-` and `.` within each
component. Absolute paths, drive prefixes, UNC paths, backslashes, empty segments,
`.`/`..`, alternate data streams, trailing dots and Windows device names are
rejected. Resolved source paths must remain within their explicitly supplied root,
including through symlinks/junctions. Roots must be existing absolute directories.
Case-insensitive duplicate resolved filenames are rejected across root aliases.

SHA256 is computed over exact file bytes in streaming blocks. Text line endings
are not normalized. Uppercase or malformed digest strings are rejected. All
sources, including provenance receipts, must exist and match before Unreal is
loaded. Sources are re-resolved and rehashed before native import. Importable
files are copied into isolated temporary snapshots and those verified bytes feed
the factories. Neighboring source directories are never scanned.

A provenance row has `source_id`, nonempty `evidence_source_ids` and `statement`.
Every evidence reference must identify a source with the `provenance` role.
Statements record what the caller's evidence asserts. Matching hashes establish
byte identity, not ownership, authenticity, approval, source-card pairing, license
clearance or runtime acceptance. The importer supplies none of these assertions.

### Wardrobes, parts and skeletons

Every wardrobe supplies its own `id`, `version`, `expected_part_count`, `skeleton`
and `parts`. `expected_part_count` must equal its list length exactly. There is no
global six-part assumption and no copying of one wardrobe's bone count to another.

Each part explicitly supplies `id`, `source_id`, `filename`, `object_path`,
`morph_targets` and ordered `material_slots`. The filename must equal that source
entry's basename, with a lowercase `.fbx` extension. An empty morph list explicitly
means zero morph targets. Each material slot provides a source slot `name` and a
declared `material_id`; list order is the source/native slot order. Morph names,
bone names and material slot names must be unique, including case folding.

Each skeleton supplies:

- `mode`: exactly `create` or `reuse`.
- `object_path`: the exact native skeleton path.
- `source_bone_count`: positive count established by the source exporter/evidence.
- `expected_bone_count`: exact expected native count.
- `bone_names`: complete ordered native bone names; its length must equal the
  native count, and native inspection must match the entire ordered list.

Source and native counts are separate because the exporter may include an
additional native root. No implicit `+1`, minimum threshold or universal wardrobe
count is applied. Pure validation verifies the declared source count's type and
the native count/name contract; it does not parse FBX geometry or prove the
exporter's source bone count. Native inspection is required for actual mesh data.

For `create`, the first part creates a skeleton in the same folder named exactly
`<first-part-name>_Skeleton`, which must be the declared skeleton path. Subsequent
parts use that exact skeleton. For `reuse`, the path must already resolve to an
actual `Skeleton` with the exact ordered native bone list before any import. It
may be outside `destination_root`, but must still be under `/Game/`. Shared reuse
paths must declare identical native contracts. They cannot collide with outputs.
There is no nearest-name search, compatible-skeleton fallback, automatic retarget
or silent cross-wardrobe substitution. Reference-pose updates are disabled.

Native top-level object paths must have the form `/Game/Folder/Asset.Asset`.
Package and object names must agree. Class-qualified paths, subobjects, redirects,
`/Engine`, `/GameX`, path traversal and case-insensitive output collisions are
rejected. A folder path is not interchangeable with an object path.

### Materials, textures and coordinate settings

Textures declare `id`, `source_id`, `object_path`, `srgb` and `compression` (one
of `default`, `normalmap`, `masks`). Normalmaps and masks require `srgb=false`.
Native textures are imported only from their listed, verified source entry.

Materials declare `id`, `object_path`, `two_sided`, `blend_mode` (`opaque` or
`masked`) and a nonempty `textures` mapping. Allowed channels are `base_color`,
`normal`, `roughness`, `metallic`, `emissive_color`, `opacity_mask`. Each value is
a texture ID. Normal inputs require linear normalmap textures; scalar inputs
require linear, non-normalmap textures and use the red channel. Color inputs use
RGB. Masked materials require an explicit `opacity_mask`; opaque materials reject
that input. The adapter creates only these declared material graphs and binds
them to exact named mesh slots. More elaborate shaders require a later version of
the contract, not embedded code or arbitrary material operations.

`settings` requires positive finite `uniform_scale`, boolean `convert_scene`,
`convert_scene_unit`, `force_front_x_axis`, three-number `translation` and
three-number `rotation`. Translation maps to `unreal.Vector(x, y, z)` and rotation
to `unreal.Rotator(pitch, yaw, roll)` in degrees. The flags explicitly control FBX
coordinate/unit conversion; this is not an unverified claim about the source
file's axis convention. No manifest value is evaluated as Python.

## Use inside Unreal Editor Python

Call the same function with `dry_run=False` from an already authorized Unreal
Editor Python session. The module never starts an Editor, invokes a build, cooks
assets, changes project settings or registers gameplay content.

```python
from character_package_import import ImportFailure, import_package

try:
    receipt = import_package(manifest, roots, dry_run=False)
except ImportFailure as error:
    receipt = error.receipt  # FAILED, with source provenance and partial progress
    # Persist this data to an explicitly selected audit location, then stop.
    raise
```

Preflight checks every output before creating anything. Each FBX task uses explicit
`FbxFactory`, `FbxImportUI`, skeletal mesh mode and `AssetImportTask`. The task's
`destination_path` is a Python `str` folder, as required by the Unreal API, and
`destination_name` is its separate string asset name. Tasks are synchronous,
automated, `save=False`, `replace_existing=False` and
`replace_existing_settings=False`. Automatic animation, physics asset, LOD,
material and texture imports are disabled. Morph import is explicitly enabled.
Textures use an explicit `TextureFactory`; materials use `MaterialFactoryNew`.

The adapter verifies returned task paths against the allowed declared outputs,
loads exact asset paths/classes, checks skeleton references and exact bone lists,
compares exact morph names and ordered material slots, and checks material texture
references and texture settings. Unsupported required engine APIs fail closed;
there is no exception handler that converts a missing morph query into acceptance.
After all checks, import-data filenames are rebound to the verified original
source paths, provenance metadata is stamped, and declared new assets are saved.
An explicitly reused external skeleton is not stamped or saved by this pipeline.

## Existing assets, receipts and recovery

The default `existing_policy="reject"` rejects any existing output, even one of
the expected class. It never overwrites, deletes, fixes or merges conflicting
assets. Use a reviewed, empty destination or inspect conflicts separately.

An explicit `existing_policy="preserve_verified"` permits a no-write return only
when **every** declared output exists at its exact path, has the correct class and
contains the identical canonical manifest SHA256 metadata. Native contracts and
references are then checked again. A partially imported package, an unstamped
asset or a different package hash is rejected. There is no partial-resume or
overwrite option. Provenance metadata is a local engineering receipt, not a
tamper-proof signature or proof that binary content has never been edited.

Returned receipts contain the schema identifier, canonical manifest hash, source
roles/versions/byte hashes/resolved paths, provenance assertions, import settings,
wardrobe/part contracts, output inventory, completed operations and native
observations. Dry-run receipts are deterministic for unchanged inputs and root
locations. Status values are:

| Status | Meaning |
| --- | --- |
| `DRY_RUN_VALIDATED` | Pure checks passed; native preflight/import was not run. |
| `IMPORTED_VERIFIED` | Native outputs passed this contract and save calls succeeded. |
| `PRESERVED_VERIFIED` | A complete stamped package passed native checks; no writes. |
| `FAILED` | Native preflight, import, verification or save failed; see `error` and `completed`. |

An `ImportFailure` carries the failed receipt. Pure contract failures raise
`ValidationError` before Unreal is loaded. The caller chooses receipt persistence;
the function never guesses a production audit path or silently replaces a receipt.

Unreal imports and saves are not an atomic filesystem transaction. Unexpected
factory outputs or a failed native check may leave new assets in memory; a failed
save may leave a partially saved package. An explicit skeleton reuse request also
allows the FBX importer to reference that existing skeleton, which the engine may
dirty while processing source data. No rollback, skeleton backup or whole-project
transaction is claimed. Stop on failure, inspect newly created assets and any
explicitly reused skeleton, then choose a clean destination or perform a separately
reviewed recovery. Automatic cleanup is intentionally absent. Concurrency with
other writers to the same Unreal destination/source roots is unsupported.

## Tests and limits

```powershell
python -B Scripts/production/test_character_package_import.py
```

The test file generates a small **noncanonical**, validator-only package in a
temporary directory. Its FBX/PNG-named files are synthetic marker bytes, not usable
art. Two independently counted wardrobes demonstrate different part counts and
different bone counts. No fixture is added to production content or a registry.
The suite covers path/hash/collision failures, malformed contracts, source roles,
reference integrity, provenance coverage, conflict policy, isolated source
snapshots, deterministic Unreal-free dry-runs, CLI exit codes and task argument
boundaries. An optional installed `jsonschema` provides an independent schema
shape check; no package installation is required. The actual symlink test skips
when Windows does not permit creating test symlinks.

Pure tests and a task boundary double cannot establish that an Unreal version
imports a particular FBX correctly. Real-source native ingestion, material visual
review, animation compatibility, runtime integration, platform performance and
content acceptance remain separate checks. No Unreal execution is part of this
engineering validation.

API references: [AssetImportTask](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AssetImportTask?application_version=5.4),
[FBX skeletal import settings](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/FbxSkeletalMeshImportData),
[AnimPoseExtensions](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AnimPoseExtensions?application_version=5.4),
[MaterialEditingLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary?application_version=5.4).
