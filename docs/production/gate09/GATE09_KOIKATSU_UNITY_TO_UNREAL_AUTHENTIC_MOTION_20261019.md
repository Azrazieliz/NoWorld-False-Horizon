# Gate 09 — Genuine Koikatsu → Luminous → Unreal motion-source submission (2026-10-19)

**Issue:** [Gate00 #25](https://github.com/Azrazieliz/Offline-Game/issues/25) with [animation requirements #20](https://github.com/Azrazieliz/Offline-Game/issues/20) and [locomotion requirements #21](https://github.com/Azrazieliz/Offline-Game/issues/21).
**Status:** SOURCE_PRODUCED / BLENDER_REIMPORT_PASS / ART_QA_PARTIAL / UNREAL_NOT_TESTED.
**Owner:** Gate09 authentic animation source and artistic production; Gate12 is responsible for UE 5.8.3 animation runtime, native import/retarget implementation and Android.

## Actual new assets — protected local-only, no public game/mod binaries

Protected original-asset-safe root: `D:\UnrealProjects\Gate09_Protected_Production_20261016`.

- **Editable Blender 5.2.2 source** `G09_AuthenticIdleAndBow_Source_Review_v4.blend`; SHA-256 `d7bb6a98ef362410fbe23d649ab70569d2ee30716ecbd2df9ab3c404bf43f8b2`. This contains four genuine bone-animated Actions on the authentic imported **Luminous V3 CLOTHED 975-bone** skeleton and actual skinned character mesh, not a mannequin.
- **Native Unity clip sources**: authentic `f_tachi_00_02` 5.0s, `f_tachi_00_09` 4.733s, `f_tachi_00_19_L` 4.7s, `f_tachi_00_18_L` 4.7s; 30fps. Original installed bundle SHA-256 `42e61bda9efbaf443b8ba5577f44be0ab7bdf85c87b5a2af1adc1a19c35473d3`. UnityPy 1.25.4 read-only; source copies and complete sampled curves remain protected on PC.
- **Three actual animated idle FBXs + one distinct bow emote**, each inside `UnrealInterchange_20261019` with `INTERCHANGE_MANIFEST.json`, frame range/fps and SHA-256. Explicit skeleton mapping and source integrity verified.
- **Four actual 3D preview frames** at `G09_REAL_KK_f_tachi_00_{02,09,18_L,19_L}_FrameReview_v4.png` and `G09_Koikatsu_IdlesAndBow_VisualSourceReview_v4.json`. Visual check showed coherent rig poses after native rest-axis correction. Grayscale Workbench material is **diagnostic only**, not game material fidelity or approved premium animation.
- **Blender 5.2.2 independent FBX reimport receipt** `UnrealInterchange_20261019\BLENDER_FBX_REIMPORT_QA.json`: **4/4** genuine animation sequences imported as **975-bone** Actions with actual changing skeletal poses. Sample maximum motion ranged from 0.0182 m to 0.1681 m. No importer return-code-only claim.
- **Source-specific orientation:** original Unity prefab `abdata/chara/oo_base.unity3d` yielded **234/234 exact Unity CRC32 transform-path matches** against a native 702-binding standing clip. Unity streamed key coeffs, dense sample arrays, constant channels rebuilt to original 30fps; quaternion norms exactly ~1; duplicated prefab rest values consistent. Native world vs Blender armature rest alignment from **19 measured anatomical joint pairs**, determinant -1 (reflected transform), scale 0.828293, fitted RMS 0.214806m; **not a universal axis patch**. 192/230 actual mapped bones per 576/702-binding source clip.

## Animation role / UE 5.8.3 interchange

| Role (not canonical gameplay ID) | Original KK clip | Animation-only FBX | SHA-256 |
|---|---|---|---|
| Idle 01 — Thoughtful | `f_tachi_00_02` | `G09_LuminousV3_Idle_01_Thoughtful.fbx` | `6622933189721721844a706049b88df12f58eedffd0cc55c2ff2321f86b3b184` |
| Idle 02 — Greeting | `f_tachi_00_09` | `G09_LuminousV3_Idle_02_Greeting.fbx` | `78c693a224adf73f060183bb68f9b5a9097c65d534dd52f70618fa6ffdcda2ea` |
| Idle 03 — Expressive | `f_tachi_00_19_L` | `G09_LuminousV3_Idle_03_Expressive.fbx` | `70c11ccd5c4c0867672b85a0ddb51d6ba1a7f1f07bfcfe54ffbf894c13883c74` |
| Bow emote (separate) | `f_tachi_00_18_L` | `G09_LuminousV3_Emote_Bow_Source.fbx` | `1a3f041c5b9d3597c23675ce8d63ce2ac0129e3e67405ad2fc94ce2b13b5f357` |

**FBX contract:** 30fps skeletal Action on unchanged 975-joint Luminous CLOTHED imported skeleton; axis_forward `-Z`, axis_up `Y`; armature-only baked FBX 7400, no leaf bones; in-place without authored root travel. No pose-fabrication, no mesh resizing or names normalized. CLIPS NOT ALREADY IMPORTED TO UE; Gate12 must import **isolated** test-only animation assets, compare bone-rest profiles and observed movement, then decide whether to consume. These source clips are **not** a production-ready shared skeleton; UNDERWEAR/NUDE still differ by 193 paths per pair.

**Visual QA disposition:** 3 standing variations have recognizably independent character gestures. The bow is NOT a fourth idle. Still missing full-length multi-camera evaluation, natural hand and foot contact, looping and blend-window polish, face/gaze curves, secondary hair/skirt simulation and Painterly Anime PBR frame-parity validation. No artistic final acceptance.

## Face and installed mature animation sources — separate content lane

- Native installed `KK_SexFaces.dll` **exists**; Luminous CLOTHED GLB contains morph target structures (6 meshes with shape targets; **2,160 mesh-target instances**, not 2,160 unique/effective facial morphs). Original source facial names/effective deltas, eye/gaze and playback need separate exact target mapping to UE morph curves. The DLL itself is not an Unreal plugin.
- Protected `G09_Facial_Source_Unreal_Contract_20261019.json` includes detected plugin SHA, mesh/morph names and exact source hash; **facial Unreal animation NOT_TESTED**.
- Protected `G09_Koikatsu_Adult_Face_Mod_Source_Inventory_20261019.json` is an inventory of **4,524 installed ZIPMOD archives, 111 candidate adult animation/facial/related packages with hashes, and 328 native H content files**. This inventory **does not mean 4,524 packages are adult, transferable, or staged**. It preserves original files in-place, with third-party licensing and source/provenance unresolved. No third-party mod ZIPs or explicit binary content have been uploaded to the public repository or silently imported into UE. Adult work remains separate and requires verified adult characters and rights review before use.
- No altering original wardrobe states or conflating adult/privacy masks with clothes.

## Verified guards and actual regressions

- `Scripts/Gate09/test_authentic_koikatsu_unity_transfer.py`: **7/7 PASS** original source hashes/animation sampler/234 CRC mappings/quaternion precision/four real skeleton FBX reimports/four visual frames/facial+mod provenance guard.
- Shared Software Mastery `K047`: verified native Unity clip extraction, rest-basis remap and Blender output QA, with explicit Unreal/visual boundaries; master **43/43 PASS**, runner **10/10 PASS**.
- No modification to Foundation `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`, PR#16, Registry v2, native Koikatsu cards, approved appearance states or Gate12 SOURCE_ACCEPTED assets. No Android cook/install.

## Remaining user requirements (not falsely counted complete)

**P0/P1 native mobility:** artistically accepted walk, back/strafe, run/sprint, start/stop/pivot, dodge, jump and landing, swim, fly, climb, contextual ledge, injury/recovery and unusual-anatomy overrides **NOT DELIVERED** in this submission. Existing Koikatsu animation library now has 1,537 source clips indexed as discovery foundation; available source clip count does not equal matching character movement library. Source-by-source retarget/contact/visual refinement required.

**Combat:** Gate04 canonical mechanics/timing and Gate07 equipment profile dependency; distinct bare hand, ranged/throw, magic, firearm, advanced tech, natural attacks, hit/stun/hit-cancel. **NOT DELIVERED**.

**Dance:** genuine Koikatsu Studio dance clips identified earlier, but premium 8–20s genuine Luminous animation / beat synchronization still **NOT TESTED**; lower priority than movement. Mature animation assets stay in a separate protected lane with no blanket eligibility or license acceptance.

**Integration:** Gate12 UE 5.8.3 import/AnimBP/retarget/root axes and Android tests **NOT TESTED**; Gate11 content acceptance and Gate00 artistic approval **NOT GRANTED**.

**Next source task:** author/acquire locomotion-specific authentic clips and establish stride/contact lengths using actual Luminous rig + Gate12 measured gameplay speed; validate full-body front/side/diagonal animated footage before claiming premium locomotion.
