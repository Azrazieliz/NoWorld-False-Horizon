# NoWorld migration — single source of truth, 16 October 2026

**Four previous Gate conversations:** Gate00 / Gate01 / Gate11 / Gate12. **New separation:** Gate01-2D and Gate01-3D use distinct conversations, neither blocks the other's approved work.

**The new conversations must not start a new project.** Latest direct user direction supersedes previous instructions: observable progress during operations, useful Git commits/checkpoints, stop when an owned task is truly DONE, preserve all authentic content and fixes. All *non-destructive routine Android dev builds, test signing, install/update and device QA are ALREADY AUTHORIZED*. Only destructive user data changes / public release need further permission.

Use [live board](../LIVE_EXECUTION_BOARD_20261016.md), [operating rules](../ACTIVE_OPERATING_RULES_20261016.md), [recovery contract](../RECOVERY_BOOTSTRAP_20261016.md), and [issue #25](https://github.com/Azrazieliz/Offline-Game/issues/25). **Do not ask the user to paste unrelated reports or restart approvals.** Each specialist's first action is read its numbered document below, inspect actual local/GitHub status and *continue work*, with concise observable status updates. In a new conversation, all it needs is this link plus the Gate name.

## Documents

- [Gate00 — coordinator](MIGRATE_GATE00.md)
- [Gate01-2D — art director / illustration](MIGRATE_GATE01_2D.md)
- [Gate01-3D — native Koikatsu + 3D](MIGRATE_GATE01_3D.md)
- [Gate11 — Registry v2](MIGRATE_GATE11.md)
- [Gate12 — UE5.8.3 / Android runtime](MIGRATE_GATE12.md)
- [Cleanup classifications](CLEANUP_KEEP_ARCHIVE_DELETE.md)

**Issue #25 must remain one project-level live board, NOT an instruction to repeatedly send identical messages to every specialist.** Dedicated Gate10 art freezes, original Game Foundation and draft PR16 remain immutable.

## Migration data scope / provenance

Gate12 source branch `production/gate12-koikatsu-ue583-fixture-20261010` HEAD observed `cdcb22bf3f4541e7334b27768d9f43bcfc4a18df` (**54 local changed/untracked entries found later**; so HEAD is NOT the whole state).
Gate01 branch `production/gate01-v6-2d-first-20261011` HEAD `3a6bf0f0e4a6c31221c02e00eaa2c41eeb58e4fc`.
No standalone Gate11 GitHub specialist branch found by branch-name search; Gate11 last authoritative acceptance still via Registry v2 and published Library ZIPs, NOT a manufactured Git commit.
Gate09 local `production/gate09-rig-motion-20261016` has untracked Scripts/Gate09; preserve.
Coordinating branch `coordination/gate00-live-ops-20261016` never merges frozen Foundation.

## Cleanup results 2026-10-16

Protected local recovery files:
`D:\Tools\GameAssetPipeline\ProductionAutomation\checkpoint\migration_20261016\GATE00_RECOVERY_AND_CLEANUP_AUDIT_20261016.json`
`...\GATE12_UNCOMMITTED_TRACKED_20261016.patch`
`...\GATE12_LOCAL_RECOVERY_SOURCE_20261016.zip`
`...\gate00_recovery_audit_20261016.py`

Saved before deleting only two generated `__pycache__` directories (621728 + 9535 bytes). **Untracked UE binaries have ALSO been copied into a separate verified protected recovery archive**: `GATE12_UNTRACKED_BINARY_RECOVERY_20261016.zip` (724 files; 248,944,219 bytes; SHA-256 `97525c80318bf64f651b93e4972650a4b9f59fe046dcb6601d4d321ae95acff6`; ZIP CRC PASS). The original worktree remains unchanged. A recovery archive is not a tested Git commit: Gate12 must still push intended source and asset changes. This backup contains scripts/patch, NOT all generated Unreal binaries; never claim it is a complete restorable project snapshot.
