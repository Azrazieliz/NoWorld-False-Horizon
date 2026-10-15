"""Preserve cross-session technical evidence without committing private character files."""
import pathlib,json,os
BASE=pathlib.Path(__file__).resolve().parents[1]
PRIVATE=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261015")
def load(n):return json.loads((PRIVATE/n).read_text(encoding="utf-8"))
assembled=load("private_assembly_review.json")
shader=load("adaptive_material_results.json")
strong=load("strong_gradient_test.json")
diff=load("adaptive_gradient_visual_qc.json")
out={"blender":"4.5.10 LTS","review_privacy":"PRIVATE local source and .blend excluded from public repository",
     "current_demo_acceptance":"TECHNICAL_PROTOTYPES_ONLY",
     "koikatsu":{"source_shards":6,"merged_meshes":assembled["meshes"],
        "rigs_remaining":assembled["rigs_remaining"],"bones":assembled["rig_bones"],
        "uniform_bone_names_and_rest_matrices":all(
           x["all_121_bone_names_identical"] and x["max_bone_rest_matrix_L1"]<.0001
           for x in assembled["rig_comparisons"]),
        "vertices":assembled["vertex_count"],"polygons":assembled["poly_count"],
        "uv_missing":len(assembled["uv_missing"]),"per_part_morph_channels":assembled["blend_shape_channels_total"],
        "material_slots":assembled["material_slots"],"source_modified":False,
        "original_game_fidelity_verified":False},
     "materials":{"editable_group":shader["node_group"],"material_slots_modified":shader["material_count"],
        "original_image_base_links_preserved":shader["preserves_images"],"topology_rebuilt":0,
        "global_and_local_coordinates_executed":True,
        "strong_variant_max_pixel_difference":diff["compared"]["strong"]["global_max_difference"],
        "strong_variant_hair_mean_abs_red":diff["compared"]["strong"]["region"]["hair"]["mean_abs_rgb"][0],
        "strong_variant_skirt_mean_abs_red":diff["compared"]["strong"]["region"]["skirt"]["mean_abs_rgb"][0],
        "in_game_koikatsu_editing":False,"unreal_material_equivalence":False,
        "visual_artist_approved":False},
     "professional_demonstrations_accepted":0}
(BASE/"VALIDATION"/"phase3_sanitized_evidence.json").write_text(json.dumps(out,indent=2),encoding="utf-8")
regfile=BASE/"CAPABILITY_REGISTRY.json";reg=json.loads(regfile.read_text(encoding="utf-8-sig"))
for id,stat,script,lim in [
 ("Koikatsu_ReadOnlyFullShardAssembly","VALIDATED","INTEGRATIONS/koikatsu_private_assembly.py","Six actual GLB shards, 1x121-bone character and original textures; no likeness/artistic approval"),
 ("KK_TexturePreservingLocalWorldGradient","VALIDATED","MATERIALS/adaptive_texture_gradient.py","Three real materials, no mesh rebuild, gradient visibly demonstrated in strong variant; no in-game KK or UE materials acceptance"),
 ("KK_CyclesTextureVisualAudit","VALIDATED","INTEGRATIONS/koikatsu_material_diagnostic.py","Actual Cycles material diagnostic with texture/shading defects; do not call equivalent to game")
]:
 if not any(x["id"]==id for x in reg["entries"]):
  reg["entries"].append({"id":id,"status":stat,"script":script,
    "evidence":"VALIDATION/phase3_sanitized_evidence.json","limitation":lim,
    "production_ready":False})
regfile.write_text(json.dumps(reg,indent=2),encoding="utf-8")
demo=BASE/"VALIDATION"/"phase2"/"DEMONSTRATION_STATUS.json"
d=json.loads(demo.read_text(encoding="utf-8-sig"))
for x in d["entries"]:
 if x["name"]=="Koikatsu reconstruction":
  x["status"]="PARTIAL_PRIVATE_ASSEMBLY"
  x["description"]="Six original source GLBs assembled into one 121-bone rig with facial shape keys and textures; KK shader parity still fails review"
 if x["name"]=="Multi-layer procedural material":
  x["status"]="ENGINEERING_VALIDATED"
  x["description"]="Editable same-mesh local/global asymmetric material gradients across textured hair and skirt; no Unreal parity"
d["professional_accepted"]=0
demo.write_text(json.dumps(d,indent=2),encoding="utf-8")
print("NW_PHASE3_EVIDENCE="+json.dumps(out))
