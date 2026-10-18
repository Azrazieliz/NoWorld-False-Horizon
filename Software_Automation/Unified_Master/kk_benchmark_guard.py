#!/usr/bin/env python3
"""Koikatsu benchmark native-operation enforcement.

Test07+ direct reconstruction scripts cannot mutate Maker via
kk_native_visual_search.invoke unless a locally verified full-resolution source,
user/artist-grounded feature ROIs and a bounded reversible gate plan authorize
the actual operation. This guard is additive; Test01-06 files stay unchanged.

This is a safety/continuity barrier, NOT a character-identity recognizer.
"""
from __future__ import annotations
import contextlib,contextvars,hashlib,json,pathlib,re,sys

_SCORED_SWEEP=contextvars.ContextVar("nw_kk_scored_sweep",default=None)
_EDIT_PREFIX=("set_","apply_","save_","load_","create_","delete_","reset_")
_READ_ONLY={"render_camera_view","inspect_appearance","inspect_accessory",
 "inspect_accessory_pose","inspect_surface_targets","inspect_surface_shader_profile",
 "export_surface_texture","inspect_morph","inspect_hair"}
_FORBIDDEN={"save_derived_card","save_card","save_coordinate","delete_card"}

def script_requires_gate(script_name=None):
    """Detect benchmark entry points, rather than ordinary offline QA modules."""
    if script_name is None:
        script_name=pathlib.Path(getattr(sys.modules.get("__main__"),"__file__","")).name
    name=pathlib.Path(script_name).name.lower()
    m=re.match(r"^(?:test|t)0*(\d{1,3})(?:[_\-.]|$)",name)
    return bool(m and int(m.group(1))>=7)

def step_signature(step):
    return json.dumps({"op":step["op"],"args":step.get("args",{})},
                      sort_keys=True,separators=(",",":"),ensure_ascii=False)

def restore_key(step):
    """Native restoration keys: even different assets must restore same live slot."""
    op=step["op"];a=step.get("args",{})
    if op in ("apply_accessory","apply_accessory_native"):
        return ("accessory_slot",a.get("slot"))
    if op=="set_clothes_top":return ("clothing_top",0)
    if op in ("set_clothes","set_clothes_state"):
        return ("clothing",a.get("index"))
    if op in ("set_hair","set_hair_palette"):
        return (op,a.get("index"))
    if op=="set_accessory_vector":
        return (op,a.get("slot"),a.get("group"))
    if op=="set_accessory_parent":
        return ("accessory_parent",a.get("slot"))
    if op.startswith("set_surface_") or op=="apply_surface_texture":
        # A texture modification must have a matching original texture restore.
        kind="texture" if op=="apply_surface_texture" else op
        return (kind,a.get("group"),a.get("slot"),a.get("file_name"),
                a.get("parent"),a.get("guid"))
    if op=="apply_morph_batch":
        morphs=a.get("morphs",[])
        if not isinstance(morphs,list):
            raise ValueError("Scored morph restore requires an explicit list")
        return ("morph_batch",tuple(sorted((v["group"],v["index"]) for v in morphs)))
    if op in ("set_eye_iris","set_iris_asset"):
        return (op,)
    raise ValueError("Unsupported change lacks verified restoration: "+op)

def require_reversible_plan(plan):
    baseline_keys={restore_key(s) for s in plan["restore_baseline"]}
    for choice in plan["candidates"]:
        for step in choice["steps"]:
            key=restore_key(step)
            if key not in baseline_keys:
                raise ValueError("Candidate edit has NO baseline restoration: "+str(key))
    return True

def authorize_plan(plan,manifest):
    """Authorize only planned trial setters, never unscored reconstruction."""
    import kk_native_visual_search as native
    import kk_reference_features as source
    native.validate_plan_for_gate(plan) if hasattr(native,"validate_plan_for_gate") else None
    if manifest.get("schema")!="NW_KK_REFERENCE_FEATURES_V1":
        raise ValueError("Feature gate source absent")
    if pathlib.Path(plan["reference"]).resolve()!=pathlib.Path(manifest["reference_path"]).resolve():
        raise ValueError("Candidate and scored reference differ")
    if not pathlib.Path(plan["reference"]).is_file() or source.digest(plan["reference"])!=manifest["reference_sha256"]:
        raise ValueError("Full-resolution source hash not verified")
    from PIL import Image
    with Image.open(plan["reference"]) as im:
        if min(im.size)<384:
            raise ValueError("Missing full-resolution original source")
    features=manifest.get("features",[])
    if not features or not all(f.get("feature_identity_verified") for f in features):
        raise ValueError("ROI proposals do not authorize native editing")
    if not any(f.get("critical") for f in features):
        raise ValueError("No critical feature gate")
    if plan.get("semantic_weight",0):
        raise ValueError("Unverified semantic weighting prohibited")
    if not plan.get("restore_baseline") or not plan.get("candidates"):
        raise ValueError("No reversible scored candidate set")
    require_reversible_plan(plan)
    allowed=set()
    for s in plan["restore_baseline"]:
        if s.get("op") not in native.OP_SET:
            raise ValueError("Unsupported baseline native setter")
        allowed.add(step_signature(s))
    for candidate in plan["candidates"]:
        for s in candidate["steps"]:
            if s.get("op") not in native.OP_SET:
                raise ValueError("Unsupported gate candidate setter")
            allowed.add(step_signature(s))
    return frozenset(allowed)

@contextlib.contextmanager
def scored_native_sweep(plan,manifest):
    allowed=authorize_plan(plan,manifest)
    if _SCORED_SWEEP.get() is not None:
        raise RuntimeError("Nested unsupervised native sweep")
    token=_SCORED_SWEEP.set(allowed)
    try:
        yield
    finally:
        _SCORED_SWEEP.reset(token)

def require_permitted_step(step,script_name=None):
    op=step.get("op")
    if not isinstance(op,str) or not op:
        raise ValueError("Invalid native operation")
    if op in _FORBIDDEN:
        raise RuntimeError("Native card promotion is forbidden in image benchmark")
    active=_SCORED_SWEEP.get()
    if active is not None:
        if op in _READ_ONLY:return
        if step_signature(step) not in active:
            raise RuntimeError("Native edit outside scored reversible candidate plan: "+op)
        return
    if not script_requires_gate(script_name):
        return
    if op in _READ_ONLY:return
    # Fail closed on unknown write operations too, including materials and accessory changes.
    raise RuntimeError("SCORING_GATE_REQUIRED: Test07+ native "+op+
                       " blocked. Run the verified full-reference scored gate runner.")
