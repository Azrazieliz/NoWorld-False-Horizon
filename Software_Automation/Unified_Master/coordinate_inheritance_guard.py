"""Read-only guard against inherited coordinate presets after a character/form edit.

Detects which saved Koikatsu coordinate slots really changed. Distinct forms or
wardrobe states must not silently inherit old source presets as approved states.
"""
import argparse
import json
from native_kk_card_diff_v1 import mkdiff

def evaluate(diff, total_coordinates=7):
    if not isinstance(diff.get("coordinates"), dict):
        raise ValueError("Native coordinate diff is required")
    changed=sorted(int(i) for i in diff["coordinates"])
    if any(i<0 or i>=total_coordinates for i in changed):
        raise ValueError("Invalid native coordinate index")
    inherited=[i for i in range(total_coordinates) if i not in changed]
    partial=bool(changed) and bool(inherited)
    return {
        "status":"REVIEW_INHERITED_COORDINATES" if partial else ("ALL_COORDINATES_CHANGED" if changed else "NO_COORDINATE_CHANGE"),
        "changed_coordinate_indices":changed,
        "baseline_matching_indices":inherited,
        "no_auto_promotion":partial,
        "instruction":"Explicitly map each authored intended state and verify native saved-card readback; inherited presets are not form-approved."
            if partial else "Verify authored wardrobe semantics separately; structural comparison does not prove visual acceptance.",
        "read_only":True
    }

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--baseline",required=True)
    p.add_argument("--target",required=True)
    args=p.parse_args()
    result=evaluate(mkdiff(args.baseline,args.target))
    print("NW_COORDINATE_INHERITANCE_GUARD="+json.dumps(result))
if __name__=="__main__":
    main()
