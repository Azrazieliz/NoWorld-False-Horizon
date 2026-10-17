# Koikatsu Reference -> Independently Scored Features -> Native Candidate Search
**Branch:** software/mastery-integrated
**Date:** 2026-10-17
**Scope:** All NoWorld character-capable Gates 01–12; noncanonical QA only.
**Commercial inference:** $0 per character. Local Python/numpy/scipy/PIL, optional local ONNX CPU model.

## Purpose
The user's final five-reference visual assessment was **5/50, 45 points lost**. Native card saves and API success are not artistic acceptance. Prior whole-frame scores were easily distorted by unchanged default body/background and by hair positions that never matched.

The new pipeline makes visual failures **independent per character feature**. A perfect garment cannot compensate for floating hair, wrong eyes, incorrect face proportions, or missing pose.

## New reproducible contract
1. **FIRST program:** `kk_reference_features.py` reads the *original high-resolution* reference and proposes hair, face, eyes, body/proportions, primary outfit, sleeves, legs, footwear. These proposals are geometric body-area priors, **NOT semantic detection**. All inferred features carry `UNVERIFIED_PROPOSAL`. Users can supply normalized reference boxes using `--roi-overrides` to mark positively identified features, remove hidden features with null, and add unusual features such as wings, horns, tail, scythe, armor details, and throne.
2. **SECOND program:** `kk_feature_gates.py` scores each identified feature independently (0..100) using **local silhouette/coverage, edge structure, palette, location, image color** within corresponding source/native ROIs. No global average can override a critical gate below its minimum. Each region has cropped side-by-side evidence image. Unverified ROIs and uncontrolled background segmentation block fidelity acceptance.
3. **THIRD program:** `kk_gate_native_optimizer.py` takes manifest + native trial plan. It uses the existing `kk_native_visual_search.py` persistent Koikatsu runtime, one Maker, real native cameras, reversible slider/outfit edits and explicit restore baseline, then identifies improvements and regressions for each critical feature. Regressed features veto recommendation. **No card saving, no paid APIs, no autonomous promotion.**
4. **User adjudication:** The user judges actual source-vs-rendered crops for each feature. Scores are local image proxies, NOT a claim of human-reviewed 7/10 identity.

## Executable example with a real full-resolution local source
```powershell
$m='D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unified_Master'
$python='D:\Tools\Python311\python.exe'
# Reference path must be to an actual original artwork image on the authorized PC.
& $python "$m\kk_reference_features.py" --reference 'D:\Path\Reference.png' --out 'D:\QA\features.json' --view front --roi-overrides 'D:\QA\verified_boxes.json'
& $python "$m\kk_feature_gates.py" --manifest 'D:\QA\features.json' --candidate 'D:\QA\real_koikatsu_camera.png' --out 'D:\QA\individual_scores.json' --crops 'D:\QA\feature_comparisons'
# Only after verified single-camera reference/pose, an existing Maker and controlled trial plan:
& $python "$m\kk_gate_native_optimizer.py" --feature-manifest 'D:\QA\features.json' --plan 'D:\QA\source_trials.json' --out-dir 'D:\QA\new_native_gate_sweep'
```

## Critical restrictions and accuracy limitations
- The automatic first pass is **NOT yet a trained anime feature recognizer**. It proposes typical positions; face, bangs and clothing boundaries can be wrong even on an upright source, much worse on a seated or sideways illustration. ROIs require verification. For artworks with a background, use source masks/manual segmentation before trusting scores; the scorer explicitly blocks non-white sources rather than fabricating a score.
- A score of 82 in a matching ROI is **NOT equivalent to 8.2/10 artistic quality**. Thresholds are strict candidate-screening numbers, and may require real-reference calibration. Distinct hair topology or body geometry cannot be generated solely by scoring.
- Local MobileCLIP2 ONNX was previously tested across independently constructed second-pass KK faces with **only 1/4 correct top-1 identity**. It must not be the only candidate selector or a fallback automatic approval.
- Generic clothing category lookup, generic Blender feather primitives, 5,000 polygons or verified card saving never count as better visual fidelity.
- Native alternative rendering remains dependent on preloaded Koikatsu mod library; no implicit mid-session reload or paid image-to-3D API.
- This work **is a real gated visual feedback engine, not a full TRELLIS-equivalent image-to-3D geometry generator**. The missing work is reliable anime semantic segmentation, source-to-native inverse parameter fitting, verified native asset bank and robust nonhuman geometry generation.
- Never alter frozen Foundation, Gate12 export default, registry or canonical game assets for an unapproved reference.

## Measured code evidence
- `test_kk_feature_gates.py`: **9/9** offline evidence checks: 8 independent scores, wrong robe, flying/missing hair, unverified ROI veto, custom horns, provenance and non-acceptance.
- `test_kk_gate_native_optimizer.py`: **9/9** end-to-end mocked native transport trials: wrong robe scored 87.22, correct source 100, floating hair 74.89; correct candidate prioritized without hiding floating hair, original state restored, crop proofs and no card saved.
- The source=render tests are **synthetic fixtures** and DO NOT establish image reconstruction quality. Real held-out input from user's phone is requested for independent validation.

## Next benchmark (must NOT reuse already fitted render as reference)
- User supplies ONE original full-resolution reference via phone.
- Verify file lands on authorized PC or current conversation attachment is accessible; never infer file path.
- Identify and ground per-feature boxes and present them; then test a genuinely new Koikatsu character against original image using the same one-session native workflow, with failed features and actual changed regions visible.
- User independently scores per-feature true likeness, with reference and actual rendered crops. The model must generalize to unseen character(s) before any production acceptance.
