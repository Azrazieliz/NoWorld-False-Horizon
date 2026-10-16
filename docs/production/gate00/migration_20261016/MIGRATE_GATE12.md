# Gate 12 — CRITICAL native worktree preservation and next Android build

**Role:** Unreal 5.8.3 implementation, functional Ruler/World UI, offline persistence, native character preview/actor, runtime animation/retarget, native testing and authorized development Android APK/device QA. Not responsible for art approval, original 3D modeling or Registry IDs.

## CRITICAL SOURCE TRUTH / RECOVERY BEFORE TOUCHING ANY FILE

GitHub branch `production/gate12-koikatsu-ue583-fixture-20261010` observed HEAD `cdcb22bf3f4541e7334b27768d9f43bcfc4a18df`. **CURRENT LOCAL WORKTREE IS NOT CLEAN:** Git status at migration found **54 modified/untracked change entries** including 7 C++ tracked modified files, many new `Scripts/Gate12Koikatsu/*.py`, `Build/Android` files, experimental maps/assets `.uasset`/`.umap`, and new 2026-10-16 integrity report. These may contain significant new development after GitHub HEAD. **NEVER reset --hard, git clean -fdx, checkout away, overwrite files or bulk-delete caches without preserving these changes.**

Protected local recovery on authorized PC:
`D:\Tools\GameAssetPipeline\ProductionAutomation\checkpoint\migration_20261016\GATE00_RECOVERY_AND_CLEANUP_AUDIT_20261016.json`,
`...\GATE12_UNCOMMITTED_TRACKED_20261016.patch`,
`...\GATE12_LOCAL_RECOVERY_SOURCE_20261016.zip`.
The first local ZIP includes the tracked diff + untracked code/text. A second **verified protected binary archive** now contains the untracked UE binaries: `D:\\Tools\\GameAssetPipeline\\ProductionAutomation\\checkpoint\\migration_20261016\\GATE12_UNTRACKED_BINARY_RECOVERY_20261016.zip`, 724 files, 248,944,219 bytes, SHA256 `97525c80318bf64f651b93e4972650a4b9f59fe046dcb6601d4d321ae95acff6`, full ZIP CRC PASS. Its receipt is `GATE12_BINARY_RECOVERY_RECEIPT_20261016.json`. Source originals remain untouched. **Still commit/push actual tested code/assets rather than treating these local backups as a replacement for Git.** Two generated `__pycache__` dirs were safely deleted, nothing else.

## Highest-priority FIRST ACTION IN REPLACEMENT

1. Open the above audit and verify actual worktree state; identify all meaningful C++, script, UI, Unreal `.uasset`, level and dev build changes.
2. In the current OWN isolated branch, run appropriate read-only tests on the current real source and commit/push coherent, tested substantial changes (or first commit `WIP_RECOVERY` accurate if testing can't run). Write crash-safe `docs/production/gate12/SESSION_CHECKPOINT.md` with exact HEAD, untracked assets, test outcomes and ONE next action. Keep large private asset binaries in protected local storage plus SHA/reference if unsuitable for repo. Do not pretend remote GitHub currently represents all latest local source.
3. Resume the functional Android vertical slice; user **explicitly authorizes repeated non-destructive development build/cook/test-sign/install/update and S26 device testing**, so STOP-BEFORE-COOK in 2026-10-16 historical QA docs is superseded for normal dev APK iteration. There is no finished production game app. Protect true saves/unrelated apps; public release/destructive wipe separate permission.
4. Produce a real runnable private dev APK with Ruler→Character→World→return flow, portrait/landscape handling, camera/touch, persistence, honest fixture if no Registry-installed character. Retain previously requested visible Adult Mode and 13-logical-participant test scope (can be PARTIAL if not ready; do not fake ready).
5. Test actual current APK/device and fix measured defects, logging exact package/build SHA, video/screenshot and QA PASS/PARTIAL/FAIL per feature. Stop treating separate 3D/Registry/dance dependencies as a universal build blocker.

## Concrete P0 fidelity evidence still open

- Native tested **body** GLB→UE dimensions match (~128.143 cm body height); apparent smaller size strongly sensitive to distance/FOV. Do not scale source characters or force arbitrary 90° yaw.
- Imported Unreal eyeline material is opaque/single-sided, whereas native source contains large transparent gradient; restore per-material semantic fidelity in QA/production appropriately, not global translucent sweep.
- Six of 15 effective body morphs and nine of 91 effective face morphs lost in tested path, despite zero threshold and sparse expansion experiments; investigate exact importer cause.
- Three Base wardrobes each 975 joints yet 193 differently pathed joints and 38–89 rest-transform mismatches: Gate09 actual animation/retarget QA needed.
- Latest previously reported 148 native suite nonfailure (130 clean/18 warning) belongs to `cdcb22bf...`; fidelity gate 13 failing/10 pending; current uncommitted source needs fresh tests.
- Gate01 native source mapping now complete; current user-authored Alter CLOTHED/UNDERWEAR/NUDE hashed in [Gate01-3D migration](MIGRATE_GATE01_3D.md). No repeated image/card survey.
- Next actual character scene/catalog production binding must be authentic; Gate11 ARTIFACT_READY pending. QA-only fixture is not a fabricated owned Manifestation.

## Protected

Frozen Foundation SHA `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`, draft PR16, saves, source cards/asset manifests, adult actual wardrobe separate from Privacy, no chips/E03/E04 resurrection, frozen Gate10 art direction. **Android dev cook now authorized**; neither arbitrary renderer/pipeline switch nor public release is authorized.

## Stop rule

When the next private dev APK genuinely installs, launches and shows a working vertical-slice acceptance path, mark **that specific milestone** DONE with physical evidence and STOP that task. Advance to next actual gameplay gap, not another meta coordination report. Do not claim entire game complete.
