# Gate 12 — Luminous Valentine V3 isolated signed Android APK ready for physical installation
Date: 2026-10-11, Europe/Paris (signed APK verification UTC 21:58:39). **Engineering experimental APK, NOT Gate11 ARTIFACT_READY.**

## Executed outcome

- Resumed without reset from branch checkpoint `33a69f9cceb9400af6f77053f046bb4cb239a70f`.
- 121/121 genuine 975-joint native Skeletal Mesh parts present, three wardrobe states under one character. Original cards, GLBs, textures, hash receipts and previous imports preserved, no LOD/surface simplification or hair replacement.
- All nine Luminous-native real CMU 30fps Walk/Run/Jump animations preserved, exactly per-wardrobe native skeletons. Real Unreal headless game playback validated 3/3 CLOTHED clips, 3/3 wardrobe transitions.
- Built new native mobile-interactive wardrobe QA `UOGLuminousV3WardrobeQAWidget`. The isolated Android pawn uses three live touchscreen buttons (CLOTHED, UNDERWEAR, NUDE), plus gamepad D-pad Right and keyboard T. This is a **Gate12 QA affordance**, not Gate10 final visual UI. Changes operate on the existing World Mode pawn and inherited movement/camera/touch/combat; normal gameplay save/persistent wardrobe intentionally gated until Gate11 canonical IDs are authoritative.
- New isolated Android-native Blueprint derived from the same `AOGLuminousV3PlayableCharacter`, with **auto wardrobe and auto motion cycles disabled**, touch QA controls enabled, entering World Mode via the existing `UOGPresentationModeSubsystem`. Cold-level setup: `/Game/Experimental/LuminousValentineV3/AndroidReady_20261011/Maps/L_LuminousV3_IsolatedAndroid`.
- Actual `UnrealEditor.exe -game -nullrhi` test launched this exact AndroidReady map, confirmed CLOTHED 45/45, World Mode activation and creation of the touch controls. User touch presses and visual renderer are NOT_TESTED until physical-device session. Journal: `Saved\Gate12Koikatsu\LuminousV3\luminous_android_ready_headless_preflight.json`.
- Unreal Build Tool Windows Editor: PASS; final Android ARM64 native link: PASS (post-widget source compiled, no cosmetic-only mockup).
- UAT `BuildCookRun` Android ASTC full cook **1,140/1,140 packages with 0 errors, 0 warnings**. Gradle signed-development APK build and archive PASS.
- Saved `Config/DefaultEngine.ini` and `Config/DefaultGame.ini` were temporarily overlaid to use *only this isolated app ID/map*, then independently SHA-verified and **restored byte-for-byte**, even after first failed wrapper attempt. No Foundation merge, destructive Git reset or alterations to the installed normal phone app.
- Independent Android Build Tools `aapt`, `apksigner`, ZIP file structure and SHA-256 verification: **PASS**. Single APK, arm64 libUnreal.so, cooked data inside APK, package name `com.azrazieliz.gate12luminousv3`.

## Physically installable APK, authoritative

`D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu\LuminousV3\PhoneReady\LuminousValentineV3_Gate12_Android_ASTC_Isolated.apk`

- File size: **232,832,069 bytes**.
- SHA-256: `8c1a2f1294c0c7976d28a5bf6557f12434660ceaf26b8a9f5d771b4261535b48`.
- Signer certificate SHA-256: `9bc439712a0a460cc4703ebf254aaaf9bf3cc6e67b0ced6fc12162ed3ca3b9ad`.
- Package ID: `com.azrazieliz.gate12luminousv3`. Distinct from normal `com.azrazieliz.OfflineGame` and earlier `com.azrazieliz.gate12fixture`.
- `MinSDKVersion=26`, `TargetSDKVersion=36`, Android ARM64 ASTC, Development/debug-signing, packaged with internal cooked game data. APK structure 571 entries.
- Independent verification receipt: `Saved\Gate12Koikatsu\LuminousV3\luminous_signed_installable_apk_verification.json`.
- UAT proof: `Saved\Gate12Koikatsu\LuminousV3\AndroidLuminousIsolatedASTC_v2\uat_stdout.txt`; cook and package `BUILD SUCCESSFUL`, UAT exit code 0. Watchdog + saved config restore journal: `Saved\Gate12Koikatsu\LuminousV3\luminous_isolated_astc_apk_build_receipt_v2.json`.
- This APK contains the **new native Luminous AndroidReady map**, NOT the prior Chika or Koikatsu fixture.

## Paused at the intentional physical-device boundary

**No phone installation, uninstall, clear data or modification of `com.azrazieliz.OfflineGame` occurred.** Previous ADB checks found no Galaxy S26 Ultra connected. When the owner connects/unlocks the Samsung Galaxy S26 Ultra (ADB serial `R3GYC0LSC7K`), execute only this guarded installer, which independently rechecks package ID, archive SHA-256, cert, target serial/model and normal app preservation:

`powershell.exe -NoProfile -ExecutionPolicy Bypass -File "D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu\LuminousV3\INSTALL_AFTER_PHONE_CONNECTED_LUMINOUS_ONLY.ps1" -Install`

It uses `adb install -t` for the new app, or `-r -t` only for a previously installed *same isolated Luminous package*. No uninstall and no data wipe. **Do not run until phone attached and user requests installation.** Physical visual/movement/thermal/rendering and touch QA still must then be measured.

## Outstanding quality limitations — do not misstate final acceptance

- Facial-morph fidelity remains PARTIAL: index 7 misses 9 and index 16 misses 6 source-effective targets in each wardrobe, **45 total missing**. Earlier zero-threshold and semantic-byte-equivalent dense accessor probes both failed to recover them; original source/glb unchanged. No tolerance-based pass claimed.
- Motion V3 native clips and forward-travel state binding have passed engine tests, but animation blending, directional strafe/backward, foot IK, independently authored swimming/flying motion, facial expression animation and artistic visual approval are not complete. Unsupported flight/swim states deliberately do NOT use ground gait.
- Correct actual Galaxy S26 Ultra on-device rendering, clipping, walking/running/jumping/flying/swimming, character camera/collision/combat, touch press responses, CPU/GPU frame time, sustained thermal/memory and installed save namespace are still **NOT_TESTED**.
- Persistent character/wardrobe via canonical SQLite was implemented in C++ and protected against cross-identity/invalid-version writes, but **NOT_TESTED** because Gate11 has not assigned/approved the canonical Luminous IdentityId/VersionId. It remains fail-closed; QA wardrobe switches are session-local.
- Gate11 alone has authority to grant `ARTIFACT_READY`; this receipt gives **installation readiness for an isolated engineering test only**, not final production acceptance.

## Private backup and Git-reproducible Android scripts

Original prior 121-native-part plus nine Motion V3 `.uasset` binary source backup remains at `D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE12_LUMINOUS_V3_121_NATIVE_PARTS_NINE_CMUMOCAP_WORLDMODE_20261011.zip` (SHA `c209bc8f5eaa6fe5975c89d608c30b9b4302ec7f056a579fe02fcbaebd251aff`).

Additional AndroidReady native map, GameMode/Blueprint, UI C++ and safe packaging scripts archive: `D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE12_LUMINOUS_V3_ANDROID_READY_NATIVE_AND_AUTOMATION_20261011.zip`, SHA `311fbce0574ec2f750da7d1e07d3f1a75025172b9ddd1cca105b5f5f6bd2bcf4`. This archive is additive and does not replace the separate original-character private binary source backup.

Separate byte-identical signed APK private backup: `D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE12_LUMINOUS_V3_ISOLATED_ANDROID_ASTC_SIGNED_20261011.apk`, same verified SHA-256 `8c1a2f1294c0c7976d28a5bf6557f12434660ceaf26b8a9f5d771b4261535b48`.

Reproducible source scripts on this Git branch: `Scripts/Gate12Koikatsu/run_luminous_v3_isolated_signed_astc_package_ue583.ps1`, `invoke_luminous_v3_android_astc_uat_ue583.ps1`, `verify_luminous_v3_signed_isolated_apk_ue583.ps1`, `install_luminous_v3_only_after_phone_connected_ue583.ps1`, and `author_luminous_v3_isolated_android_entry_ue583.py`. Existing intermediate ZIP, APK, keys, confidential original Party cards, Koikatsu GLBs and generated native `.uasset` binaries are not added to Git.

**Stop point:** a verifiably signed, unique-ID Luminous engineering APK is present in the laptop's `PhoneReady` folder. The phone can now be plugged in for the *next physical-install and actual on-device QA phase*. Do not call the experiment final gameplay/art approval before physical testing, Gate09 motion approval, remaining 45 facial morph repair and Gate11 acceptance.
