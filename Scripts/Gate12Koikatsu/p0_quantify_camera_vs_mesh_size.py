"""Measure camera apparent size separately from actual centimeter dimensions.
Reads existing Gate12 C++ defaults and GLB geometric rest bounds. NO app edits.
"""
import json,math,pathlib,re,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
GLB=ROOT/"Saved/P0CharacterQuality/gltf_v3_all_states_quantified.json"
PREVIEW=ROOT/"Source/OfflineGame/Private/UI/OGGate12CharacterPreviewStage.cpp"
WORLD=ROOT/"Source/OfflineGame/Private/World/OGStartingRegionPresentation.cpp"
OUT=ROOT/"Saved/P0CharacterQuality/camera_scale_vs_apparent_size.json"


def find_float(source,pattern):
    match=re.search(pattern,source)
    if not match:
        raise ValueError("Cannot verify camera setting: "+pattern)
    return float(match.group(1))


def framing(name,height_cm,width_cm,dist,hfov,widthpx,heightpx):
    aspect=widthpx/heightpx
    tanh=math.tan(math.radians(hfov/2))
    tanv=tanh/aspect
    fullwidth=2*dist*tanh
    fullheight=2*dist*tanv
    return {
        "name":name,
        "physical_rest_height_cm":round(height_cm,3),
        "physical_rest_width_cm":round(width_cm,3),
        "camera_distance_cm":dist,"horizontal_fov_degrees":hfov,
        "vertical_fov_degrees":round(math.degrees(2*math.atan(tanv)),2),
        "viewport_px":[widthpx,heightpx],
        "visible_width_cm_at_target":round(fullwidth,2),
        "visible_height_cm_at_target":round(fullheight,2),
        "unposed_height_fraction":round(height_cm/fullheight,4),
        "unposed_width_fraction":round(width_cm/fullwidth,4),
        "approx_apparent_height_pixels":round(heightpx*height_cm/fullheight),
        "status":"PINHOLE_CAMERA_GEOMETRY_ESTIMATE_NOT_RENDERED_SCREENSHOT",
    }


def main():
    glb=json.loads(GLB.read_text("utf-8"))
    clothed=glb["files"][0]["rest_geometry_bounds"]["extent_cm"]
    # Source +Y-up converts to Unreal +Z-up in these imported meshes.
    height=clothed[1]
    width=clothed[0]
    stage=PREVIEW.read_text("utf-8")
    world=WORLD.read_text("utf-8")
    preview_distance=abs(find_float(stage,
        r'Capture->SetRelativeLocation\(FVector\((-?[0-9.]+)f'))
    preview_fov=find_float(stage,r'Capture->FOVAngle\s*=\s*([0-9.]+)f')
    world_distance=find_float(world,r'CameraBoom->TargetArmLength\s*=\s*([0-9.]+)f')
    world_fov=find_float(world,r'FollowCamera->FieldOfView\s*=\s*([0-9.]+)f')
    preview=framing("current_native_roster_stage",height,width,
                    preview_distance,preview_fov,512,768)
    world_actual=framing("current_world_landscape_16_9",height,width,
                         world_distance,world_fov,1920,1080)
    options=[
        framing("diagnostic_roster_camera_260cm_31deg",height,width,260,31,512,768),
        framing("diagnostic_world_300cm_70deg",height,width,300,70,1920,1080),
        framing("diagnostic_world_close_270cm_65deg",height,width,270,65,1920,1080),
    ]
    result={
        "schema":"GATE12_P0_CAMERA_APPARENT_SIZE_SEPARATION_V1",
        "physical_geometry_source":"GLB mesh-node unposed rest AABB; native Unreal component total not yet measured",
        "baseline":[preview,world_actual],"alternatives_for_review_not_implemented":options,
        "previews_source_is_blank_catalog":True,
        "no_rendered_pixel_acceptance":True,
        "no_scale_or_camera_changes_applied":True,
        "limitations":[
            "Perspective estimate: centered camera distance, assumes plane normal to optical axis.",
            "Actual limb pose, rotations, cropping, projection depth, scene capture postprocess and streaming may change pixels.",
            "Actor bounds cannot be derived from a single imported primitive.",
        ],
    }
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(result,indent=2)+"\n")
    for f in result["baseline"]+options:
        print(f["name"],"actual_cm",height,
              "pixelheight",f["approx_apparent_height_pixels"],
              "height_fraction",f["unposed_height_fraction"])
    return 0


if __name__=="__main__":
    raise SystemExit(main())
