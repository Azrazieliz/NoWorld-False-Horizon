# Phase 3: Real Koikatsu-derived scene and texture-preserving adaptive-material experiments

**Date:** 2026-10-15. **Professional acceptance:** 0/25. **Source assets:** never redistributed, only automation recipes and sanitized metrics versioned.

## GUI review in the authorized PC
Two **interactive Blender 4.5.10 LTS windows** were launched from Windows PowerShell and confirmed responding:
- `NW_ParametricSanctuary.blend` — 200-object prototype architecture, editable; do not portray as final environment.
- `D:\UnrealProjects\NW_Blender_Private_Review_20261015\NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend` — standalone local assembled real Koikatsu-derived model, not canonical or approved. The source is located in a separate Gate 12 work directory and the original data was not changed.

A third variant was not opened in Blender simultaneously to respect the 8 GiB / Intel graphics machine's low memory headroom. Its rendered PNG was opened in the default Windows image application for immediate inspection.

## Non-destructive six-shard source assembly
`INTEGRATIONS/koikatsu_private_assembly.py` reads the six GLB shards (0 to 5) from Gate 12 data and verifies **identical bone names and bind-rest matrices** before merging. All six stored 121-bone rigs; the script keeps a single main rig and reconnects modifiers, preserving mesh components, images, textures, keys, UV and materials. Auxiliary low-poly export-debug Icospheres are dropped.
- Output: six meshes, one 121-bone armature; 33,755 vertices and 53,190 polygons; 25 material slots; 1,797 nonbasis shape-key channels across independent mesh parts. (The same name may appear on several meshes: 1,797 is not a distinct-expression count.)
- Source .glb files unchanged; private .blend and PNG saved only under local `D:\UnrealProjects\NW_Blender_Private_Review_20261015`.
- A neutral Workbench snapshot shows geometry; `INTEGRATIONS/koikatsu_material_diagnostic.py` produced a separate lit **Cycles CPU** scene/render in 6–7 sec at 450x640 and 8 samples. Original imported shaders/textures show hair/face/clothes but visually deviate from the game, particularly in leg clothing/stocking texture zones.
- This is **assembly and diagnostic lighting**, not repaired KK identity, shader equivalence, in-game Koikatsu mod, professional character modeling, texture approval or native Unreal intake.

## Editable, image-preserving gradients
`MATERIALS/adaptive_texture_gradient.py` builds one reusable native Blender 4.5 ShaderNodeTree group: `NW_AsymMultiObject_Gradient_v1`. Applied by duplicate material (preserving original material data and incoming Principled base-image link). UI-accessible group inputs include:
- Original Base Texture link, Bottom Tint, Top Tint, Tint Strength.
- Coordinate, Gradient Axis, Gradient Offset, Asymmetry, Noise Scale, Noise Strength.
- A per-material coordinate mix: 0 = local Generated space for within-object gradients; 1 = global shared `NW_Shared_GradientAnchor` space for a coordinated gradient across objects.
- Color is computed by smoothstep(gradient-axis dot coordinate + independent X asymmetry + noise + offset); continuously mixes color endpoints and multiplies this continuous tint with the incoming original image. No recoloring of separated objects masquerading as a gradient.

The group's outputs are reusable without new topology. Tested on **two hair image materials and one skirt image material** (three total), creating original-preserving, local-coordinate, global-coordinate, and strongly visible noncanonical gradient variants.

**Actual render comparison** at 500x720: subtle-local vs global-color variant produced RGB mean absolute differences ~[0.226,0.135,0.035] over full image, with mean abs RGB in hair [1.347,1.154,0.387] and skirt [2.311,1.068,0.194]. The deliberately strong visible test produced max pixel change **68/255**, with regional mean abs red difference 7.337 in hair, 13.728 in skirt. Background mean abs RGB remained ~zero. This confirms real within-material effects, but the subtle version's impact was too weak for an unequivocal artistic demonstration. The strong version provides an unmistakable visible diagnostic, not a canonical design proposal.

All meshes retained their geometry; the original image links remained connected for each of the three material variants. User has native Material node GUI access in Blender. This does **not** enable editing within the Koikatsu game or a Koikatsu user interface, nor does it establish Unreal parity or compatible pixel bakes.

## Context from independent source research
- **Blender Studio Fundamentals 4.5 procedural shading:** https://studio.blender.org/training/blender-fundamentals-45-lts/blender_4-5_lts_shading_watering_can/ — practice of using Generated coordinates and Separate XYZ for art-directable material gradients and procedural roughness.
- **KKBP material breakdown:** https://kkbpwiki.github.io/material — custom game appearance may require color_to_KK / image_to_KK conversion and separate light/dark shading, perma-light and perma-dark masks, specialized face/eye and outline treatment; stock imported Principled shaders are not equivalent.
- **KK Blender Porter Pack:** https://github.com/MediaMoots/KK-Blender-Shader-Pack — an independent Blender addon/export ecosystem requiring import versions, data rights, rig/material baking QA; not installed by this script.
- **Blender reusable node groups:** https://docs.blender.org/manual/en/4.5/interface/controls/nodes/groups.html — material group with adjustable named inputs is reusable across source assets, instead of duplicating entire graphs manually.

## Unresolved
Source GLB shader fidelity vs KK, anime-level render/acting quality, morph control per expression, hair/stocking geometry repairs, gradient baking/Unreal import, texture/normal/pipeline LOD tradeoffs, complete modification from within Koikatsu, actual cloth ripping and topology cuts. No representation of artistic completion and no Gate 00 approval.

## Private artifacts accessible on PC
`D:\UnrealProjects\NW_Blender_Private_Review_20261015\`:
- `NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend` and material diagnostic PNG;
- `NW_KK_ChikaHaruno_AdaptiveMaterials_PRIVATE.blend` and PNG;
- `NW_KK_ChikaHaruno_GlobalGradients_PRIVATE.blend` and PNG;
- `NW_KK_ChikaHaruno_StrongGradient_Diagnostic.blend` and PNG;
- JSON reports per run and pixel comparison (source metrics remain local).

**Operating rule:** Never publish source-derived character .blend or images to the public software-mastery Git branch without explicit asset-distribution authorization. Only version scripts, source references and aggregate technical QA.
