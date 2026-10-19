# Gate 09 — Koikatsu adult-mode compatibility + real Unreal facial-source proof
**Date:** 19 October 2026  
**Scope:** User authorized a first adult-mode animation transfer test, preserving original source and Gate12 authority. Technical source/rig investigation and a non-explicit native UE facial proof completed. **No explicit sex-scene asset imported or rendered.**

## 1. Native adult-mode source evidence — READ_ONLY / PARTIAL
- Original installed Koikatsu source: `D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\h\anim`.
- **288 native Unity animation bundles** found.
- **25 genuine bundles sampled**, yielding **750 authentic AnimationClip records**. Each recorded clip is 30 fps and 1.3333 seconds (these are *source facts*, not an approved scene duration). Measured approximately 1,024–1,031 transform bindings per clip.
- For the **first clip in each of those 25 bundles**, the source's **343 unique Transform CRC32 paths resolved 343/343 exactly** against the installed native Koikatsu `oo_base.unity3d` prefab hierarchy. No ambiguous CRC results in this sample. This is **native source-path compatibility**, not a validation of transfer to the Luminous rest pose, multi-actor synchronization, visual quality or Unreal playback.
- Read-only receipts (protected local data only):
  - `G09_InstalledAdultNativeAnimation_MetadataOnly_20261019.json`
  - `G09_AdultNativeRigBindingCompatibility_20261019.json`
- Animation motion key arrays from these adult-mode bundles were **not extracted, baked, rendered, or imported**. No explicit scene or graphic file generated.
- The existing separate inventory's **111 candidate adult/motion/facial mod packages** is a loose metadata classification including irrelevant apparel and other mods; **not an approved pool of 111 adult sequences**. Asset authorship, mod licenses and original provenance require individual review.

## 2. Actual native Unreal facial deliverable — PASS (technical source)
- Copied the original Gate12 **Luminous V3 CLOTHED `Mesh003Prim000` body skeletal mesh, skeleton and physics package** into an independent QA Unreal project, preserving exact Unreal package paths. Three original/copy hashes matched.
- Authenticated the source GLB's actual nonzero eye/mouth morph deformation arrays; Unreal imports **96 genuine morph targets** on the tested body mesh. No reconstruction, renaming of source bones or loss-inducing reskinning.
- Authored a new original-named morph target animation with **native Unreal Engine 5.8.3 `AnimSequence`** in isolated content:
  `/Game/Gate09_QA_20261019/AuthenticFaceSource/AN_G09_Luminous_NativeBlinkSmile_SourceProof`
- The two curves bind real imported morphs **`Eye_Closed`** (blink to 100%) and **`Mouth_Distorted_Smile__Closed`** (smile to 80%) over **2.0 seconds, 30 fps, 61 keys**. Native skeleton morph metadata saved on the *isolated* copy only.
- **Independent cold Unreal editor session PASS**: saved asset reloads, 60-frame model exists, two float curves `Eye_Closed` and `Mouth_Distorted_Smile__Closed` are still present (read-only `unreal.AnimationLibrary.get_animation_curve_names(seq,unreal.RawCurveTrackTypes.RCT_FLOAT)`), reference mesh contains 96 source morphs. This is a real UE animation asset, **not a finished facial performance or graphic scene**.
- Native project (no Gate12 production mutation): `D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019\G09_AnimQA.uproject`.
- Full binary QA archive:
  `D:\UnrealProjects\Gate09_Protected_Production_20261016\GATE09_NATIVE_FACE_AND_ADULT_SOURCE_METADATA_QA_20261019.zip` — **4,306,951 bytes**, SHA-256 `a46933e7fdf170179985630399ffcbfa4d5c13bd7ea88d95961741f5c96bd8f8`, zip integrity PASS, **1 newly authored Unreal facial `.uasset` and 3 copied immutable original facial-mesh/skeleton/physics `.uasset` files**, actual native test receipts and provenance-only source metadata. **No adult-mode motion key data or third-party packages inside**.

## 3. QA / preservation
- `Scripts/Gate09/test_native_adult_metadata_and_face_qadelivery.py`: **5/5 PASS** on native source metadata, exact first-clip hashed path resolution, saved-and-cold-loaded two real UE morph curves, 3/3 unchanged Gate12 source asset copies and protected ZIP integrity.
- Original Luminous/Alter Party cards, all source GLBs, native third-party files, frozen Foundation and PR #16 preserved.
- No gameplay Animation Blueprint change, material redraw, Privacy/wardrobe conflation, source export pipeline replacement, Android cook, packaging or installation.
- Not art approved: full-body deformation, secondary hair/clothing, scene staging, camera/lighting, musical timing, expressiveness, Unreal gameplay playback and Android remain separate Gate12/Gate00 acceptance requirements.

## 4. Precise unresolved blocker / immediate next authority
**EXPLICIT SCENE TRANSFER: BLOCKED**, not failed import. The actual participating character/form's adulthood and relevant third-party source use/distribution rights are **not verified**. Koikatsu's source library alone does not establish either. Do not assign explicit action to an unverified-age or school-themed character, and do not publish unlicensed mod payloads. Gate00/Gate01-3D must supply age/eligibility and source-use authorization before any explicit assets are processed for a production package.

Once clearance is established, Gate09 can use the proven Unity Transform hash-resolution pipeline with scene synchronization/rest-space quality reviews and native UE source export; Gate12 would still own in-game integration. Meanwhile non-explicit facial, combat, locomotion, dance and QA source production can continue independently.
