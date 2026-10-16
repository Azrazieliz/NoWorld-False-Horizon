"""Read-only fidelity guard: absent source texture channels cannot be accepted."""
import argparse,json,math
def assess(*,source_normal_maps,target_normal_bindings,source_dimensions,target_dimensions,rel_tolerance=0.01):
    if source_normal_maps<0 or target_normal_bindings<0:raise ValueError("Negative material counts")
    if len(source_dimensions)!=3 or len(target_dimensions)!=3:raise ValueError("Requires XYZ dimensions")
    if rel_tolerance<=0 or rel_tolerance>0.1:raise ValueError("Unsafe tolerance")
    ratios=[]
    for a,b in zip(source_dimensions,target_dimensions):
        if not math.isfinite(a) or not math.isfinite(b) or a<=0 or b<=0:raise ValueError("Invalid geometry dimension")
        ratios.append(b/a)
    scale_preserved=all(abs(r-1)<=rel_tolerance for r in ratios)
    lost_normal=source_normal_maps>0 and target_normal_bindings==0
    return {
        "material_normal_channel":"FAIL_SOURCE_NORMAL_MAPS_NOT_BOUND" if lost_normal else "NO_TOTAL_NORMAL_CHANNEL_LOSS_DETECTED",
        "scale":"PASS_RELATIVE_BOUNDING_DIMENSIONS" if scale_preserved else "FAIL_BOUNDING_DIMENSIONS_CHANGED",
        "xyz_target_to_source_ratio":[round(r,8) for r in ratios],
        "visual_fidelity_accepted":False,
        "status":"P0_HOLD_MATERIAL_PARITY" if lost_normal else ("P0_HOLD_SCALE" if not scale_preserved else "STRUCTURAL_CHECK_ONLY_VISUAL_NOT_PROVEN")
    }
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--source-normal-maps",type=int,required=True)
    p.add_argument("--target-normal-bindings",type=int,required=True)
    p.add_argument("--source-dimensions",required=True,help="JSON array XYZ")
    p.add_argument("--target-dimensions",required=True,help="JSON array XYZ")
    a=p.parse_args()
    print("NW_NATIVE_3D_PARITY="+json.dumps(assess(source_normal_maps=a.source_normal_maps,target_normal_bindings=a.target_normal_bindings,source_dimensions=json.loads(a.source_dimensions),target_dimensions=json.loads(a.target_dimensions))))
if __name__=="__main__":main()
