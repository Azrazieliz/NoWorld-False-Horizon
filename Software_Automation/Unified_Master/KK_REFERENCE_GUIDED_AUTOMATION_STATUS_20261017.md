# Koikatsu Reference-Guided Character Fitting — Code-First Status, 2026-10-17

**Authority:** User requests one-time reusable local Koikatsu-based system for hundreds of characters; paid per-character image-to-3D services explicitly rejected. **No new game character or asset was accepted.**

## Status: working local foundation, NOT image-to-3D mastery

Previous work failed artistically: 5/50 on five character test, despite native automation succeeding. Character similarity, not commands/cards/object counts, is the primary metric.

**New local implementation on software/mastery-integrated worktree:**
1. `kk_visual_fit.py` — explicit white-background source/native comparison using aligned silhouette, aspect ratio, garment palette, local edge structure and color matching; guards against 144p/low-resolution references and inappropriate scene backgrounds. Never automatically approves a character.
2. `kk_native_visual_search.py` — bounded test of actual native Koikatsu clothing/hair/face parameters, passive fixed camera captures, score and rollback after every trial. Requires explicit baseline and sandbox game; never overwrites user source cards or changes frozen game canon.
3. `kk_local_vision_embeddings.py` — **free local CPU-only** MobileCLIP2-S0 ONNX image embeddings, model version/pin cached, cropped full/head/lower views, 3 embeddings per image, persistent exact-image cache. Not a 2D-to-3D generator. Uses **zero paid API calls**.
4. `kk_visual_asset_bank.py` — versioned SQLite design for **incremental** cache of visually tested Koikatsu assets, their actual screenshot, native recipe, model hash and embedding. Only calibrated measurable changes should be indexed; older failed image-mismatch entries must not be treated as accepted. No default catalog switch.
5. `kk_install_local_vision.py` — model and pinned CPU runtime installation on authorized PC; `test_kk_visual_fit.py` and `test_kk_native_visual_search.py` pure offline regression.

**Pinned CPU model:** https://huggingface.co/felixhrdyn/mobileclip2-s0-onnx (MIT license). Installed FP32 `D:\Tools\OfflineGameAI\models\KoikatsuVisualMobileCLIP2\vision_model.onnx`, 45,555,784 B, SHA256 `13d20ebfa8a8f63890eb2727fe4dc63009ff970f43e0f7d9d2ed999659f70c8a`; ONNX Runtime 1.23.2 CPU. INT8 quantized variant was rejected: `ConvInteger` unsupported in this local ORT runtime. Never select INT8 without a real operator acceptance test.

## Measured evidence — do not overstate

- Offline image scorer **13/13** synthetic checks passed: wrong garment color, shape/headpiece and face offset detected, low-resolution and uncontrolled backgrounds rejected. These checks do not prove actual illustrated reference identity.
- Native sweep runner **5/5** mocked guard/restore checks passed.
- **One live Maker session**, startup ~67s; **five actual hair-palette trial renders in 14.81s**, native code picked the same silver setting as its own already-rendered reference and restored the baseline without saving cards. This is *same-source setting recovery*, NOT full illustrated-art reconstruction or a 7/10 character.
- Original render/identical-color repeat had **95.31** whole-face similarity because idle animation/pose/image alignment drift; other colors 89.69–94.12. This demonstrates overall face score can hide substantial hair mismatch.
- CROPPED hair-contour scoring improved separation (reference 96.89 versus 87.52–91.9), but still requires reference-aligned region selection.
- Identical color under aligned v2 delta analysis still changed **10.413%** of subject pixels due dynamic/camera differences; intentionally different palette values changed **21.912–27.246%**. Noise floor calibration is necessary; no auto-accept based on changed polygon count.
- Local CPU image embeddings ranked five actual Koikatsu crops in **4.286s**, with exact-image self retrieval score 1.0.
- **Critical negative control:** First-to-second-pass Koikatsu face identification only **1/4** correct top-1. This model is NOT reliable for independent identity reconstruction and remains optional/experimental. Do not make CLIP ranking the default production selector or claim it solved 2D-to-3D. Evidence: `TEST05_FIVE_CHARACTER_BATCH\LOCAL_CPU_CROSS_VERSION_RETRIEVAL_QA.json`.
- User reference file images not automatically available as full-resolution native files on the Windows PC. Do not recreate from 144p thumbnails or claim the system matched an inaccessible original.

## Minimal usage when high-resolution source exists ON this computer

```powershell
$master='D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unified_Master'
& 'D:\Tools\Python311\python.exe' "$master\kk_visual_fit.py" --reference 'X:\original_full_res.png' --candidate 'X:\candidate_native_white_camera.png' --out 'X:\my_new_visual_qa.json'
& 'D:\Tools\Python311\python.exe' "$master\kk_local_vision_embeddings.py" --reference 'X:\original_full_res.png' --candidate 'X:\candidate_A.png' --candidate 'X:\candidate_B.png' --out 'X:\new_semantic_rank.json'
# To perform actual non-destructive live native exploration:
& 'D:\Tools\Python311\python.exe' "$master\kk_native_visual_search.py" --plan 'X:\explicit_baseline_and_trial_config.json' --receipt 'X:\new_sweep.json'
```

`kk_native_visual_search.py` rejects low-resolution source files, unsupported commands, missing explicit restoration steps, original Steam app instead of the sandbox, and receipt overwrites; rank isn't a production approval. The source and native camera angle/pose must be comparable; do not optimize wrong camera projections.

## Still missing (P0, blocking actual 2D-to-3D automation)

1. Image-to-KK face/body shape inverse mapping from original drawing to native sliders (trained/verified on actual artist-corrected examples, not simply same-source palette tests).
2. Visually validated candidate accessory/outfit asset index at scale, with correct native surface/placement/pose and source-authentic multi-view previews.
3. Automatic segmentation and recognition of anime hair, clothing panels and silhouette across different render styles. Current generic MobileCLIP did NOT pass cross-version identity test.
4. Character identity/equipment pose normalization and matching multiple image views. A seated image cannot be judged honestly against only a standing Maker model.
5. True visual threshold judged by user on previously failed reference, and reproducible result on a new held-out reference. No acceptance yet.

**Current conclusion:** A free, local, code-driven fitting loop exists and runs, but professional art reconstruction is NOT solved. Technical completion alone is expressly insufficient. Do not modify Gate00 exporter default or misstate as production-ready.
