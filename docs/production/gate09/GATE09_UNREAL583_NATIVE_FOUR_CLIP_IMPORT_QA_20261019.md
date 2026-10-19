# Gate09 — Unreal Engine 5.8.3 import and native pose verification (2026-10-19)

**Commission:** User explicitly requested to test Koikatsu source movement animations in native Unreal and, if successful, import everything available. Preserved Gate12's authority over live game runtime and Android, all approved native source cards and Foundation.
**Result:** **PASS (all FOUR currently converted/verified compatible FBX sequences in isolated native UE5.8.3 project).** Not a general certification of the installed 1,537 Koikatsu clips or third-party mods. Not yet imported into the Gate12 working game nor linked to Animation Blueprints; artistic/device success not claimed.

## Verified native project / files

- **Unreal Engine:** `5.8.3-58210709+++UE5+Release-5.8`.
- **Isolated project on shared authorized PC:** `D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019\G09_AnimQA.uproject`; Python Editor importer, no Android cook and no material/mesh redesign.
- **Target exact copied native skeleton:** `/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/Mesh_00_Prim_00_Skeleton` — 975 bones. All six copied skeletal-reference `.uasset` files hash-identical to actual Gate12 accepted worktree files, which were **not edited**.
- **4/4 imported and stored native `AnimSequence` assets** in isolated `/Game/Gate09_QA_20261019`:
  1. `TestSingle/G09_LuminousV3_Idle_01_Thoughtful`, native 150 frames, 30 fps, 5.0 s.
  2. `NativeKoikatsu/G09_LuminousV3_Idle_02_Greeting`, native 142 frames, 30 fps, 4.733 s.
  3. `NativeKoikatsu/G09_LuminousV3_Idle_03_Expressive`, native 141 frames, 30 fps, 4.7 s.
  4. `NativeKoikatsu/G09_LuminousV3_Emote_Bow_Source`, native 141 frames, 30 fps, 4.7 s.
- FBX source SHA-256 per clip and actual game-source hash preserved in `docs/production/gate09/GATE09_UNREAL_INTERCHANGE_MANIFEST_20261019.json`; all four original source FAB exports remain in protected storage.
- Each UE `AnimSequence` cold-loads in a **second independent Unreal editor process**, with **975 imported bone tracks** that match **975/975 exact target names case-insensitively**. An initial script falsely reported 231 matches due to Python case-sensitive string comparison; corrected explicitly, **no bone paths renamed**. Direct native rig compatibility verified for this CLOTHED skeleton only.
- Native `AnimSequence.get_anim_pose_at_frame` evaluation at sampled time points produced nonzero **head** and **Center** motion for all four clips. Largest sampled Head quaternion/translation vector component displacement: Thoughtful 0.25485; Greeting 0.07483; Expressive 0.03114; Bow 0.01805 (numeric pose-vector distance, **not a physical meter or animation quality score**). Thus saved animation tracks have actual changing poses rather than being empty/static.
- `Scripts/Gate09/test_ue583_native_motion_qadelivery.py`: **5/5 PASS** — verifies four physical uassets, imported frame rate/track count, cold pose changes, 6/6 original skeleton hashes, protected ZIP integrity and non-production status.
- First single probe produced a **false negative** because `unreal.AnimationBlueprintLibrary` and `get_sampling_frame_rate` were unavailable in this UE Python binding. Correct tested method: `seq.get_editor_property("controller").get_model_interface()` then `get_bone_track_names()`, `get_number_of_frames()`, `get_frame_rate()`; `seq.get_anim_pose_at_frame(frame, unreal.AnimPoseEvaluationOptions())`. This correction is not speculative.

## Portable native Unreal QA proof

`D:\UnrealProjects\Gate09_Protected_Production_20261016\GATE09_UNREAL583_NATIVE_FOUR_CLIPS_20261019.zip` — **3,936,945 bytes**, SHA-256 `c197b1ae2445ec454656b92372ae2b8834c62121c07a59065ae3b356be0ea57d`. Python ZIP integrity PASS; 30 archive entries, exactly **4 native AnimSequence uassets + 6 original source skeletal mesh/skeleton/physics uassets**, sandbox `.uproject`, configs and full JSON native importer/cold pose receipts. Protected local private binary only; not uploaded to public GitHub.

Existing original source package `D:\UnrealProjects\Gate09_Protected_Production_20261016\GATE09_AUTHENTIC_KOIKATSU_TO_UNREAL_IDLE_PROOF_20261019.zip` — 52,548,122 bytes; source Blender file and original four FBXs/source curves.

## Exact Gate12 consumption contract

If authorized for gameplay integration by Gate12, consume the **cold-validated saved AnimSequence assets** via the protected native QA project/package. Maintain private character-asset provenance and originally approved target visual mesh/wardrobe. Do not replace existing Gate12 skeleton or runtime movement controllers. Review actual animated full skinned character, forward axes, foot locks, loop/blend markers, root policy, gaze/facial/cloth/hair secondary, native visual fidelity and scale before marking production or Android PASS. The separately approved wardrobe meshes have **different bone paths/rest** and do not receive automatic universal retarget. Gate12 is responsible for gameplay Animation Blueprint hookup and Android independent evidence.

## Remaining user request (not falsely completed)

The known installed **1,537 original Unity animation clips** were scanned but only **four** fully converted/QA-imported as Unreal assets in this commission. Additional authentic locomotion, swimming, flying, climbing, combat, dance etc. are not automatically Unreal-compatible or approved, so they remain pending staged, genuine source conversion/artistic QA. The previously indexed adult animation mods are a separate dependency with individual license/character eligibility and face morph compatibility unverified; **no blanket adult mod binary import** was performed. `KK_SexFaces` plugin is installed in Koikatsu and Luminous has morph targets, but UE facial curves have **not** been authored. Registry v2, PR#16, Foundation, Party source cards and accepted Gate12 production assets have not been altered.
