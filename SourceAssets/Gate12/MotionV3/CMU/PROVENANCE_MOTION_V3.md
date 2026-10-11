# Gate 12 Motion V3 — Immutable authentic human capture samples

This directory is SOURCE motion (not approved Unreal production animations).
It is not cooked into the normal Android test package. The original Koikatsu
GLB, character card, textures, morphs and ten-file frozen conversion baseline
are not modified.

## Data provenance and rights

- Original human captured motion: Carnegie Mellon University (CMU)
  Graphics Lab Motion Capture Database: https://mocap.cs.cmu.edu/
- Supported by NSF grant EIA-0196217.
- Bruce Hahne's 2010 MotionBuilder-friendly BVH conversion (cgspeed).
- GitHub distribution: https://github.com/una-dinosauria/cmu-mocap
- Walk: https://raw.githubusercontent.com/una-dinosauria/cmu-mocap/master/data/002/02_01.bvh
- Run/Jog: https://raw.githubusercontent.com/una-dinosauria/cmu-mocap/master/data/002/02_03.bvh
- Jump/Balance: https://raw.githubusercontent.com/una-dinosauria/cmu-mocap/master/data/002/02_04.bvh
- Exact converter permission file:
  https://raw.githubusercontent.com/una-dinosauria/cmu-mocap/master/READMEFIRST.txt

CMU and Bruce Hahne permit use in research and commercially sold projects,
including video games, but not resale of the original dataset as a product.
Retain READMEFIRST.txt and check current primary-source terms before release.
Do not confuse this with licenses for other animation asset collections.

## Immutable SHA-256

- 02_01_walk.bvh: cf56db43157acc3d200b3d4215523f54d354a8926bea67e1f6c8031bd335ba7e
- 02_03_jog.bvh: 6f4d38b92b041221030de2e1058903e0f7a8fa08f9c6c9efabf7df9a128011ac
- 02_04_jump.bvh: 1b9f80f045608eaf50e2912c2f9180e97cffc708ce717c227f52ce1bc6fe091b
- READMEFIRST.txt: 8e6fe2e640b3728ef2e43e722cb5ae9cdf0010028c0517c4226ed165f9037930

The BVH source is captured at 120 Hz. Bruce's added first frame (frame 0)
is an ARTIFICIAL T-pose calibration, not authentic locomotion: playback must
start from frame 1 (the first actual human performance frame).
Preserve all 120 Hz source; author 30 Hz derivatives separately.

## Retarget stage, NOT a quality approval

- Interpret source as Y-up / front-facing +Z; preserve source root-motion
  metadata separately from the character game's capsule authority.
- Calibrated from 21 actual native UE bone rest poses:
  GLB translation source (x,y,z) metres -> UE (x,z,y)*100 cm;
  GLB local quaternion (x,y,z,w) -> native UE (-x,-z,-y,w).
- Real captured orientations are retargeted to exact source Koikatsu
  skeletal reference bones, not generated sine-wave joint oscillations.
- Distinct knee flexion, elbow articulation, left-right phasing and actual
  source-foot ground contact candidates are measured offline.
- Need proper native per-part assets, physical gait calibration, foot locking,
  mocap cycle segmentation, state blending, flying/swimming clip sources,
  visual side/front/rear validation and physical S26 renderer QA before use.
- Native new-character import and Gate 11 ARTIFACT_READY remain independent.
