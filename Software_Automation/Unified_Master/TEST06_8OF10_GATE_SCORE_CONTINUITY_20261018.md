# AUTHORITATIVE GATE 01 / KOIKATSU CONTINUITY — TEST 06 USER 8/10

**Recorded:** 2026-10-18. **Purpose:** Restore exactly the prior conversation's Koikatsu automation benchmark and feature-gate work before further references. **This is not a new character commission.** This document is an additive recovery handoff; do not reinterpret the test as accepted canonical game art.

## The user's latest evaluation controls this benchmark

- **Test 06 WHITE GOLD DANCER: user-rated 8/10 for visible reconstruction** in the prior conversation, explicitly confirmed again in the 2026-10-18 continuation. Rating is a **user subjective visual judgment**, not a computed 80/100 output of `kk_feature_gates.py`, not a claim of exact fidelity, and not a canonical Gate00 acceptance.
- The old saved `T06_RESULT_RECEIPT.json` says `no user acceptance`: it was written on 2026-10-17 **before** the user's subsequently reported 8/10 evaluation. Preserve the receipt verbatim as historical evidence; the later user feedback supersedes its rating field **without overwriting the historical receipt**.
- The visually improved Test06 benchmark is the comparison baseline for future improvements. **Do not replace it with the failed witch card or Test05 art.** Do not run new character trials until the user calls for them.
- The current supplied full-resolution original illustration of a golden-haired white/ivory dancer (raised arms, diaphanous veil, turquoise jewels, gold headdress, ornate warm interior) is available as a conversation attachment, 1051 × 1536 RGB, SHA256 `329b7d53a8053498da81389149ebb0579694463b9cd6b8b2821a078acfad81d7`, in the current ChatGPT runtime as `/mnt/data/601840.jpg`. **This does not mean the remote Windows PC has that image.** The prior T06 result receipt explicitly says source was shared in chat and NOT transferred to PC. Do not synthesize an unrelated substitute, use a 144p thumbnail, or assert native full-res scoring against an absent file.

## Recovered native Test06 evidence (READ-ONLY PRESERVE)

`D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST06_WHITE_GOLD_DANCER\`

- `T06_FIRST_STAGE_RECEIPT.json`: 11.77 seconds for the initial **single Maker-session native operation batch**: morphs, makeup, amber iris, four hair parts/palettes, full clothing selection, accessories and native multi-view rendering. This is *not* wall-clock total from original reference.
- `T06_ACTUAL_NATIVE_KK_4_VIEW_QA.jpg`: exact front, face, side and back image of the Test06 final stored appearance.
- `FOUR_LONG_HAIR_ASSET_TRIALS.jpg`: Takane, Anne, Minami and Iori actual native hair visuals, not imagined mod descriptions.
- `T06_UV_COLOR_VARIANTS.json`: actual source-atlas comparison. Skirt 415,444 modified pixels, top 583,252; alpha and original UV source preserved, red skirt/blue top recolored toward ivory/gold/turquoise, not random flat shaders.
- `T06_WHITE_GOLD_DANCER_EDITABLE_NATIVE_KK_CARD.png`: existing disposable QA card, native reload recorded as passed; preserving file is NOT objective of tests.
- `T06_RESULT_RECEIPT.json` and `T06_WHITE_GOLD_DANCER_DISPOSABLE_QA.zip`: original provenance and selected evidence. **Never overwrite or silently relabel.**
- Original native/UV transform traces also exist in sandbox `BepInEx\SM01_SANDBOX\surface_sources`, `surface_edits`, `recipes` and `captures`. Preserve all user and original-source assets.

## All eight recovered ORIGINAL Test06 scripts — added to version control unchanged

The eight scripts predate later assistant changes and were present on disk but untracked. Their exact content is preserved:
- `new_ref_white_gold_catalog.py`: already-indexed full mod catalog; targeted search veil, dancer, silk, ornaments, hair, lace, etc.
- `new_ref_gold_dancer_trial.py`: persistent single-Python native Maker edit batch and initial native render; uses `[Ω-G16] HS2-KK Set` outfit GUID with top item **14110** and bottom **14112**, decorative slots **14124**, **14126**, **14128**, wrist/hand-related accessories `kks.backport.clothes` IDs **2**, **3**.
- `new_ref_gold_dancer_materials.py`: source-atlas recolor, preserving UV and alpha, with existing source textures.
- `new_ref_long_hair_trials.py` and `new_ref_long_hair_contact.py`: four in-sandbox true visual alternatives rather than name-only ranking.
- `new_ref_gold_ornament_recolor.py`: source hair and crown atlas edits, preserved alpha/strand shading.
- `new_ref_face_body_fit.py`: writes the revised native shape recipe `T06_FACE_BODY_REFIT_final.tsv`. **Its file existence by itself is not evidence that all refinements were applied to final native card**; do not claim unproven native readback.
- `new_ref_gold_dancer_package.py`: packages already-existing verified native card, four-view QA and immutable original receipt.

## Recovered original feature-gate version

**Source baseline:** Git `521d725` (2026-10-17 21:47:32 local), which was the original feature-gate commit *before* Test06 scripts were run 21:52–22:06 local. This baseline contains the non-destructive local feature/gate code: `kk_reference_features.py`, `kk_feature_gates.py`, `kk_gate_native_optimizer.py`, `kk_native_visual_search.py`, `kk_visual_fit.py`, `kk_visual_asset_bank.py`, `kk_local_vision_embeddings.py`.

- `kk_reference_features.py`: reference full-resolution manifest, independent identity ROIs, geometric priors explicitly UNVERIFIED, RGB and feature weights. The reference image must be on the local machine before running automatic full-reference scoring.
- `kk_feature_gates.py`: independent face, eyes, hair, body, primary garment, sleeves, legs, footwear ROI scores; critical failures veto aggregate success, unverified/complex background does NOT pass. These **0–100 metric scores are not user 0–10 artistic ratings**. In template, thresholds: hair 78, face 82, eyes 85, body 80, main outfit 82; optional sleeves 70, legs 77, footwear 70.
- `kk_gate_native_optimizer.py`: native reversible candidate comparison, feature improvement/regression veto, cropped evidence, no card save or automatic visual acceptance.
- `kk_native_visual_search.py`: one Maker native candidates with baseline restore.
- `kk_local_vision_embeddings.py`: free local ONNX MobileCLIP tested but **1/4** cross-version face identity top-1; cannot be sole selector or automatic acceptance.
- Test06's 11.77 second creation script **does not itself call** the feature-gated optimizer. Do not claim the gate scorer produced the 8/10. The experiment sequence involved both the newly built gate version and the T06 practical reconstruction.
- Later assistant commit `3b72a67` modified two shared gate/search modules and their unit checks *without independently validating against Test06 source*. **These four shared files were restored byte-exact to `521d725` in the recovery change.** Additive later calibration files remain archived in Git history and are NOT the default Test06 gate algorithm.
- The subsequent witch scripts/card are a separate, visually failed branch of testing. They are NOT the Test06 benchmark or authority; avoid applying its styling/asset choices to any new benchmark.

## Successful lessons and failure boundaries to retain

1. Build with the **existing full indexed Koikatsu mod library**, shortlist visually, and batch direct calls in ONE Maker session. No per-asset restart.
2. Preserve actual artist-specific facial/anatomical shaping, deliberate golden hair/amber eyes, visual hair candidate trials and correctly placed costume elements. User 8/10 is the reference success threshold recorded for the existing benchmark; it must not be silently degraded.
3. Modify actual native texture/UV/alpha, not generic flat recolors. Apply material colors after asset replacements and positional transforms to avoid regressing hair, sleeves and cloth.
4. The original Test06 4-view still has mismatches: arms-down rather than raised-arm pose, black side panels and opaque dress vs sheer flowing fabric, differences in long hair, veil translucency and face/expression. These are feedback for **automating better solutions**, not an order to keep polishing the disposable Test06 character.
5. Do not claim professional 2D-to-3D reconstruction until real reference-to-native fidelity is confirmed across new artwork. The existing per-feature gates are numeric proxies, and feature identification is still geometric prior/ROI rather than proven semantic recognition. The original image's background and the pose mismatch prohibit ungrounded score claims.
6. No Blender/Gaea/Unreal digression for this specific Koikatsu learning recovery. Only use other tools for substantiated residual geometry when user commissions.
7. Do not repeat downstream QA demonstrations/calibration, create replacement cards, add unnecessary scripts, or alter the successful benchmark before the user orders the next test.
8. Final user authority: judge improvement by visible likeness and repeatable faster automation, NOT saved card counts, native call counts, fabricated score, or renderer acceptance.

## State at recovery

- Original Test06 QA files remain on authorized PC, read only.
- User's 8/10 score is retained separately and authoritatively in the master ledger; original earlier receipt unchanged.
- Original feature-gate optimizer and native visual search restored from `521d725` without disturbing `kk_reference_features.py` or `kk_feature_gates.py`.
- In-progress unrelated working files remain unchanged and uncommitted, not overwritten.
- The Test06 source was not native-scored from the full-resolution phone illustration on PC; **no claim otherwise**.
- Do not begin next character test until explicit user request.
