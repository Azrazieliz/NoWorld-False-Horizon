# Test05 — crystalline dragon/seraph character / capability benchmark
**User request:** 2026-10-17 reference image. User explicitly permits omitting pendants and jewelry. Not a commission for canonical NoWorld assets.
**Assessment:** FAIL reference likeness. Engineering artifacts do not substitute for acceptable visual result.
**Source and render QA:** D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH

## Current inspected result
- Native Blender 5.2.2 file: `TEST05_CrystallineDragonSeraph_Editable_v4_NativeKKBP.blend`, using the genuine fresh Test05 KKBP export. Original source PMX and Party card remain untouched.
- Actual Blender inspected render: `TEST05_ActualNativeBody_ArtPass_v4.png`. It is visibly **underwhelming** and far from the user reference: juvenile/generic face instead of imposing mature character, boxy membrane wings, inadequate details/texture/layers, straight thin hair strands instead of voluminous long locks, crude body void, poor armor integration. Do not claim artistic success.
- Mesh count, rig existence, numerical QA alone do NOT demonstrate likeness. Do not ask the user to accept the output because it is editable.
- Test05 native Koikatsu card `SM01_AUTOTEST_T05_CRYSTAL_SERAPH_NATIVE_20261017.png` was saved and independently reloaded. **Source top garment remained default school uniform** due unsupported `set_clothes index0`. No neutral clothing acceptance.
- New native export directory: `D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\Export_PMX\20261017174837_ChikaHaruno`. Actual model.pmx, texture maps and hierarchy present.

## Test05 tested timings
- One sandbox Maker start, guarded navigation after initial window-not-ready attempt: approximately 41.3 s to Maker ready on second guarded navigation.
- **21 native KK appearance operations in 10.918 seconds total**, one persistent Python process, no intentional restarts.
- Initial editable Blender geometry generated in ~4.8 seconds after .blend load; rendering ~88 seconds.
- Detailed iteration/check of source Blender body revealed **critical armature DOUBLE SCALE**: scaled mesh + scaled referenced armature made evaluated body **20.093 m**, while object local bounds misleadingly appeared normal.
- Reset armature scale to 1 while leaving scaled mesh at 12.45 gave actual evaluated body **1.60018 m**; genuine newer Test05 KKBP body rebind gives **1.59736 m**, 0.414 m wide; 16 original/native material texture maps wired to Blender shader inputs. This fixed scale and material provenance, not visual fidelity.
- Test05 actual native rebind + render ~107 seconds, poor output. Do NOT keep running dozens of expensive render cycles without topology/art direction improvements.

## Source architecture/geometry
- Four-layer upper/lower wing construction, two antler crowns, flowing hair curves, shingled crystalline shell, dark central void, articulated 8-segment serpentine dragon tail and claws; real editable mesh objects with dedicated wing/tail armature.
- These elements are only **rough geometry placeholders**, not acceptable high-detail modeled wings, feather meshes, volumetric hair, elaborate anatomy, realistic material PBR normals, or usable animations. Most armor geometry has no meaningful body bone weights. Wing/tail objects receive ARMATURE modifiers but no pose/animation acceptance.
- Latest v4 retains genuine source body rig and native morphs plus custom provisional geometry. The first v1/v2/v3 scenes are intermediate, not user deliverables.

## Persistent lessons
- K038: never double-scale referenced KKBP armature and mesh; check evaluated world-space deformation not static object bbox.
- K039: rebind live Test05 native card/PMX to Blender and original exported albedo/eyeline/face/eye texture images, not borrowed Test03 body.
- K040: generic Python procedural feather strips and flat bat membrane geometry do not meet complex visual reference; **fail visual QA even if thousands of objects exist**.
- K041: `set_clothes` does not support top index0; native neutral-top control missing and must not be claimed complete.
- Code: `test05_qa_and_regressions.py` source-hash/scale/material proof, visual fidelity explicitly FAIL.
- Master required rules: 41; master regressions 42/42 PASS, Test05 source QA checks 12/12 PASS, visual likeness still FAIL.

## For next authorized benchmark or continuation
Only pursue more modeling if user requests. Real improvement requires (1) high-quality existing anatomical/wing/horn assets or detailed sculpt and UV maps, not bigger loops of procedural slivers; (2) correct facial reference likeness and deliberate face proportions before elaborate wings; (3) actual multilayer wing bone weights, crystal normal/roughness maps, hair cards, detailed body armor shell; (4) matched controlled render and visible difference within a strict short time checkpoint. Do not silently promote or merge.

**No production export default change, no Gate11 Registry intake, no Android cook, no canonical Foundation edits.**
