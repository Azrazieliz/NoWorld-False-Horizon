# NOWORLD — Real Native Koikatsu Per-Parameter Visual Calibration

**Date:** 2026-10-17. **Branch:** `software/mastery-integrated`. **Status:** partial P0 engineering proof; **NOT** arbitrary anime illustration → professional-quality 3D reconstruction.

## Executed, not inferred

- One authorized Koikatsu sandbox `SM01_KKParty_6436979_20261014` launched and navigated using the existing guarded Maker tool. Native Maker returned `MAKER_READY`, PID 5572. No Steam source install touched, no cards saved, no Maker restart per trial.
- Initial native `snapshot` captured actual control values; no assumed slider defaults.
- Three native baseline camera frames, two changed frames and two restored frames for **each control**, full 768×1024 screenshots, one persistent game/Python session. Two separate reversible `apply_morph_batch` controls exercised.
- FaceBaseW `face[0]`: initial **0.228**, trial **0.95**, visually changed **9.587%** of head ROI after subtracting same-state pixel noise. Narrow skin-area rectangle changed **3.972%**, but **localization failed** (much of the delta is hair/head boundary). It is NOT yet a verified face-identity fitter.
- HeadSize `body[1]`: initial **0.5975805**, trial **0.05**, visibly changed **23.706%** of head ROI; skin-area rectangle **17.974%** and had localized change. This is a real measurable head shape control; does NOT establish correct target proportions.
- A first strict visual return-to-source screen flagged head size after restore because of idle-render drift: 0.95% excess changed pixels over original baseline while repeated same-state native frames varied by 5.145% of pixels. **Do not erase this failure**. A second *immutable* read-only analysis used a conservative 25%-of-measured-noise return threshold and independently read native source state again. Both controls read back EXACT original values, and the returned screenshots were within the measured same-state noise envelope. The original failed strict receipt remains preserved.
- Known Koikatsu hair error remains: indexed hair palette operations change some layers, but observed front bangs can stay brown while rear hair changes. Do not promote a single successful palette set to full hair matching.

## Actual evidence kept on authorized PC

`D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA\native_parameter_live_20261017_v1`

- `face_0/NATIVE_PARAMETER_VISUAL_DELTA.png`, `face_0/NATIVE_PARAMETER_CALIBRATION.json`
- `body_1/NATIVE_PARAMETER_VISUAL_DELTA.png`, `body_1/NATIVE_PARAMETER_CALIBRATION.json`
- `NATIVE_MORPH_CALIBRATION_SUMMARY.json` (first-pass, includes strict restoration rejection)
- `REASSESSMENT_V2_NATIVE_READBACK.json` (separate, read-only corroboration of exact morph restoration)

## Permanent source code and correctness gates

- `kk_native_parameter_calibration.py`: reversible parameter-region probes with repeated full native captures, **spatially measured** same-state noise, visualization and fail-closed restoration; no game/card saves.
- `kk_live_native_parameter_proof_20261017.py`: real one-session native snapshot → morph trial → exact native source restoration.
- `kk_reassess_live_morph_proof_20261017.py`: immutable original-capture readback check and narrower ROI diagnostic.
- `kk_native_visual_search.py`: no longer turns a failed/noisy camera calibration into a fabricated zero-noise baseline; reports whether each candidate has *actually observed* image changes.
- `kk_gate_native_optimizer.py`: high scores with no verified observable native change are **ineligible for user review**. Reversible incremental visible improvements can be reviewed even while hard identity gates remain blocked; no automatic artistic acceptance.
- Regression tests: `test_kk_native_parameter_calibration.py`, `test_kk_native_visual_search.py`, `test_kk_gate_native_optimizer.py`.

## Strict next boundary

An actual **original full-resolution user illustration** must be supplied/recovered, and semantic parts (especially face-skin contour, hair root/layers, accessories, body/clothing) manually verified or segmented by tested vision. Do NOT evaluate using the 144p transfer or use a native rendered character as a substitute for source artwork. Hair topology and exact character identity remain unsolved. The face-width probe needs more specific skin and contour landmark diagnostics. Only promote generalized methods to `MASTER_LEARNED_METHODS_V1.json` after independent real-source improvement and regression on another character.
