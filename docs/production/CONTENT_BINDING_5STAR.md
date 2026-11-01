# NoWorld 5★ — Permanent Native Character / Action / Animation / Koikatsu Binding
**Importance: 5/7 — critical reusable production interface.**
**Date: 2026-11-01.** Required read after the [sole 7★ game authority](NOWORLD_7STAR_AUTHORITY.md) and [6★ advancement](ADVANCEMENT_PROGRESS.json). Preserve and reuse; never reimplement a new motion catalog per character or per form.

## Already integrated; use, don't duplicate
1. Canonical owned identity and native actor: `UOGCharacterAppearanceComponent` for generic characters, `AOGLuminousV3PlayableCharacter` for original Luminous, `FOGExactOriginalLuminousParts` for genuine imported source paths, `UOGCanonicalCombatActorBindingComponent` to connect actual battle World bodies. A form is a Version of an identity, not an animation library or cloned Character Identity.
2. One controller: `UOGSharedMotionControllerComponent`. Its reusable **BlueprintCallable** `BindNativeRigAndParts(Rig, Anatomy, Outfit, Capabilities, NativeParts)` installs all actual skeletal parts in one call, with fail-safe previous rig data. For an owned character, first `BindCanonicalCharacter(Identity)` when the authoritative identity is known. The ordinary content-appearance installer and original Luminous native pawn now both use this same method.
3. One original animation bank: `Content/NoWorld/Animation/SharedMotionCatalog.json` (packaged UFS), read by `FOGSharedMotionResolver::GetInstalledLibrary()` in `Source/OfflineGame/Private/Animation/OGSharedMotionLibrary.cpp`. Preserve built-in native fallback entries. A single semantic action points to multiple source-preserving, exact-Skeleton native derivatives selected by rig, anatomy, capabilities, outfit and scene role — never by character Version.
4. Real combat: `FOGNativeBattleCommand::MotionSemantic()` → native UMG `UOGNativeBattleWidget` availability → `AOGWorldPrototypeCharacter::SubmitApprovedLuminousWorldCommand` → `UOGTurnContactActionComponent::StageActualTurnAction` → shared motion authored contact/sequence completion → original `FOGTurnBattle` executor. World real-time casts use the existing separate world physical executor. No second damage simulation or UI HP ledger.
5. Real original source pipeline and receipts: 15 original Koikatsu source cards under `Assets/NoWorld/Luminous/KoikatsuOriginalCards`; existing scripts under `Scripts/Gate12Koikatsu/`, `Scripts/production/gate12_import_exact_original_luminous_3d.py`, `Scripts/production/generate_gate12_exact_original_luminous_bindings.py`; Grand 57-renderer exact card UE native source registry and provenance in `docs/production/gate12/evidence/G12_Grand_ExactCard_57Renderer_NativeRegistry_20261101.json`. Preserve exact imported .uasset paths and metadata. None of these receipts proves an approved playable animation.

## One-call new action binding (after real native import)
Existing imported `/Game/.../ActualSequence.ActualSequence` must have a real `Content/.../ActualSequence.uasset` and original approval receipt. Provide a JSON recipe file with:
```json
{
  "schema": "noworld.native_motion_bindings.v1",
  "motions": [
    {
      "motion_id": "source-specific.motion-id",
      "source_clip_id": "original.source-clip-id",
      "source_hash": "64-hex-digit-authentic-original-source-sha256",
      "approval_evidence": "docs/production/path/to/existing-approval-record.json",
      "action": "Combat.Skill.canonical_ability_id",
      "category": "Combat",
      "context": "Combat",
      "rig": "koikatsu_original",
      "anatomy": "humanoid",
      "outfit": "CLOTHED",
      "native_sequence": "/Game/Real/Imported/ActualSequence.ActualSequence",
      "capabilities": ["combat"],
      "participants": 1,
      "contact_seconds": -1,
      "natural_completion_contact": true,
      "approved_for_gameplay": true,
      "loop": false
    }
  ]
}
```
**This is an illustrative format, not an available animation or an approval.** Substitute only real IDs, exact source hash, approved evidence, correct current native asset and actual authored contact. If an authored notify exists, use `contact_notify` instead of guessed timestamps. Explicit natural completion means the real source clip finishes normally before authoritative settlement; never use an invented hit time.

Single command/API call, runnable in the existing authorized pipeline by the developer or CI, not by the phone-only user:
```bash
python Scripts/production/bind_native_motion.py path/to/approved_bindings.json
```
The binder checks schema, uniqueness, real .uasset path, authentic source hash syntax, existing provenance file, explicit approval, compatible role information, and rejects conflicting duplicates. It edits **only** the globally packaged catalog via an atomic write. It never grants ownership, adds unapproved clips, creates an actor, remaps visual art, changes stats or retargets a skeleton invisibly. Actual UE native compatibility remains checked at source playback.

## Source-to-character fast binding
Content creators produce **only original content and reliable source receipts**:
- Koikatsu card plus original SHA, anatomy, wardrobe, genuine skinned export.
- Real Blender/Unreal skeletal assets with exact source provenance, retarget derivative only if needed, verified native object paths.
- Original approved motion clip metadata, action semantic, exact rig/scene role, contact notify or real sequence lifecycle, approval reference.

The established importer generates native asset receipts and source path registry; the binder associates gameplay semantic actions with the single global animation catalog. `UOGCharacterAppearanceComponent` / `AOGLuminousV3PlayableCharacter` installs the real visual parts using `BindNativeRigAndParts`, and the native combat/input/interaction routes automatically request those semantic actions.

**Do not** create 5 copies of a motion, hand-wire each attack per Version, invent extra Luminous forms, promote QA source clips, invent contact frames, introduce a disposable mini-game, or require the owner to operate a PC.

## Current unresolved content, clearly separated from binding infrastructure
Original approved combat motions must still be supplied/linked before an authentic source-bound turn action will execute. The existing 937 UE-native combat motion entries were not automatically approved and lacked authored contacts at latest PR inventory. Grand original 57-renderer geometry is now recorded in the branch but must still reach actual complete actor, animation and gameplay integration. Remaining exact five-form executors must follow original approved v4 numeric sources. An Android APK requires real complete Unreal project cook and Android package; source changes here are not a testable release.

## Durable workflow for next conversation
Read 7★ → 6★ → this 5★ file → relevant native classes. Identify the shortest unintegrated **game system**; commit actual source and update 6★ with precise commit and remaining gameplay. Do not count a new binder, compiling, a receipt, a test or documentation as a completed 7★ system.
