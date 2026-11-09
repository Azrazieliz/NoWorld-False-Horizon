# NoWorld 5★ — Permanent Native Character / Action / Animation / Koikatsu Binding
**Importance: 5/7 — critical reusable production interface.**
**Date: 2026-11-01.** Required read after the [sole 7★ game authority](NOWORLD_7STAR_AUTHORITY.md) and [6★ advancement](ADVANCEMENT_PROGRESS.json). Preserve and reuse; never reimplement a new motion catalog per character or per form.

## Current 5★ rule: source validity ≠ owner approval (2026-11-01)

The canonical 3,222-entry animation bank has been normalized in GitHub by the reusable `Scripts/production/normalize_motion_owner_status.py` program. **Do not repopulate legacy `approved_for_gameplay` metadata** or require pre-phone-review artistic approval before source import, semantic binding, native resolution or character playback. This changes an invalid approval gate only; native files, source provenance, exact Skeleton, authored contact, action legality, adult safeguards and gameplay persistence still have to function. Do not fake missing animation/contact or user acceptance.

**5★ executable tools:**
- `Scripts/production/normalize_motion_owner_status.py` — removes 3,222 legacy QA approval booleans from current GitHub source without changing any native motion identity, original path, source or contact metadata; uses a temporary Git index and never resets the PC project. Completed source migration: `c18b43a6`.
- `Scripts/production/bind_native_motion.py` — now admits valid original source bindings without a mandatory manual approval receipt; it still validates canonical IDs, original SHA256, native Unreal UAsset, rig/role and authentic impact timing. Explicit `natural_completion_contact=true` is preserved in the global catalog and enforced by the native turn-contact runtime; an unmarked clip cannot commit combat damage simply by ending. `contact_seconds=-1` is never contact proof.
- `Scripts/production/submit_noworld_content.py`, `noworld_unreal_content_import.py` — optional provenance record is supported, but not an artificial barrier to importing an existing original asset. No automatic owner acceptance/rejection is manufactured.

**Phone review statuses (6★):** `SOURCE_PRESENT`, `INTEGRATED_UNTESTED`, `TECHNICALLY_BLOCKED`, `AWAITING_OWNER_PHONE_TEST`; only an explicit owner decision after a real phone game test may record `OWNER_REJECTED_AFTER_PHONE_TEST` or `OWNER_ACCEPTED_AFTER_PHONE_TEST`. Do not label any of the 937 combat source entries rejected for lacking impact metadata; the latter is unfinished authoring, not a veto.

**Working environment:** the PC is an ephemeral Unreal/Koikatsu/Blender/import/packaging worker. GitHub commits and original native assets, not the current PC checkout, are durable work product. Save any genuine newly generated source back to GitHub (LFS for native binaries) and index it in 6★; do not repeat the 3,222 source imports.

## Mandatory workflow: reuse automation and record 6★ advancement every time

The 5★ is **both instructions and executable programs**. For every work item: (1) READ the current 6★ ledger BEFORE touching any file to avoid implementing existing work twice, (2) identify and run/reuse an existing 5★ automation where possible, (3) where NO existing 5★ program does the job, record the precise missing capability and implement a **new generalized reusable program** under `Scripts/production/`, `Tools/` or `Scripts/Gate12Koikatsu/`, then add it to this 5★ index, (4) push the actual changed code/content, and (5) UPDATE 6★ with the real commit and 7★ system impact; do not inflate owner-accepted completion for source-only changes.

**Reusable checkpoint CLI:** `Scripts/production/record_noworld_progress.py`.

- Start/resume: `python Scripts/production/record_noworld_progress.py --resume` displays the active 36-system ledger and recent recorded commits before any reimplementation.
- After reusing a genuine existing program: `python Scripts/production/record_noworld_progress.py --commit <REAL_SHA> --system M03 --result "<actual source gameplay change>" --next "<remaining tangible gameplay>" --reuse-5star Scripts/production/bind_native_motion.py`.
- When reuse is genuinely impossible, **first write a new reusable program**, then: `python Scripts/production/record_noworld_progress.py --commit <REAL_SHA> --system M05 --result "<actual source gameplay change>" --next "<remaining tangible gameplay>" --new-5star-program Scripts/production/<new_program>.py --why-new "<exact limitation in the existing factory and why the new program is reusable>"`.

The recorder checks the historical 36/111 source counts and real Git commit, refuses duplicate commit progress, requires exact remaining work and the 5★ reuse/new-program explanation, and writes the JSON ledger atomically. It **never marks source-only systems complete or claims owner acceptance**. The committed 6★ ledger must accompany the next PR/source revision. A document-only edit or simple Git reorganization is logged as no-gameplay-enabler without accepting new systems.

**Safe original native animation-catalog reconciliation (NEW reusable 5★):** `Scripts/production/reconcile_native_motion_catalog.py`, read-only by default. It compares the protected PC native catalog against a fetched GitHub tree, checks all referenced native Unreal package paths are present both places, reports exact source/approval/contact counts, and refuses any conflicting same-ID original source. In an actual catalog-loss incident `--restore --push` creates a fast-forward child commit through a **temporary Git index** without resetting the user's PC working tree. The older `bind_native_motion.py` could NOT be reused for this recovery: it deliberately requires individual approved clips and genuine authored combat contact data, while the 3,222-entry original inventory includes 937 unapproved combat motions; running that binder as a bulk restore would either reject legitimate inventory or risk fabricating approvals. The original catalog was restored via commit `ce190e87` before this reusable safe recovery interface was added.

**Permanent inventory / code-drift aid:** [`REPO_EXECUTION_MAP.md`](REPO_EXECUTION_MAP.md) records exact source roots, a real PC-only vs GitHub inventory, known implementation drifts and the original 3,222 source-motion recovery. Review it rather than re-running an enormous general audit. The original motion catalog is NOW RESTORED on GitHub; never reinitialize it to an empty array. The 937 combat entries are source-indexed without authored contact data; generate missing authentic impact mappings from the existing content instead of redownloading/rebuilding the same animation bank.

## PRIMARY: reusable instructions AND executable native content automation (2026-11-01)

**5★ contains both the enduring agent instructions and executable, reusable Python/C#/Unreal production programs.** The original manual-only binder was insufficient alone. The repository now includes reusable Python worker programs, a real Koikatsu Maker/KKBP C# API queue plugin, a Blender KKBP source conversion script, UnrealEditor Python asset import, native C++ appearance-asset authoring, exact runtime source discovery and a Windows self-hosted GitHub Action. The workflows are BUILT IN SOURCE but **not yet native compiled/run or owner accepted**. They cannot claim lossless visual gameplay if required exporter/engine dependencies are absent.

### Direct executable entry points
- **Create a factory job from a newly created actual Koikatsu card:** `python Scripts/production/submit_noworld_content.py --publish-to-github character --card <ORIGINAL_CARD.png> --identity <CANONICAL_CHARACTER_ID> --version <CANONICAL_VERSION_ID> --rig koikatsu_original --anatomy humanoid --wardrobe CLOTHED --wardrobe-field <EXISTING_CANONICAL_OUTFIT_FIELD> --capabilities combat grounded swimming flying`. No separate library selection: `AUTO` authors/reference the ONE global shared motion UAsset. No per-Version animation catalogs. A card alone does not provide a kit or unique approved motions.
- **Submit an original native FBX animation:** `python Scripts/production/submit_noworld_content.py --publish-to-github motion --fbx <ORIGINAL.fbx> --skeleton <REAL_UNREAL_SKELETON_OBJECT> --motion-id <NATIVE_DERIVATIVE_ID> --source-clip-id <ORIGINAL_SOURCE_CLIP_ID> --source-original-hash <ORIGINAL_SHA256> --action <SEMANTIC_ACTION> --context Combat --rig koikatsu_original --anatomy humanoid --outfit CLOTHED --contact-notify <REAL_NOTIFY>`. Alternatively, supply explicit `--natural-completion-contact` if original clip has no authored contact marker, so the true natural sequence finish is the event. Source FBX hash is computed automatically. The factory imports an actual `AnimSequence`, generates the source binding recipe, then invokes the global catalog binder; no hand-writing separate binding JSON.
- **Submit original art:** `python Scripts/production/submit_noworld_content.py --publish-to-github artwork --image <ORIGINAL.png> --identity <EXACT_ID> --version <EXACT_VERSION> `. The Unreal worker imports one real Texture2D, attaches it to the same native identity/version visual registry without rewriting older art, and keeps privacy/SFW eligibility separate from file format.
- **Execute the one command on the authorized Windows worker:** `python Scripts/production/noworld_content_factory.py --inbox ContentFactory/Jobs --config ContentFactory/windows_worker.json`. Or add an authorized job JSON under `ContentFactory/Jobs/` / GitHub `workflow_dispatch` to invoke [the dedicated Actions worker](../../.github/workflows/noworld-content-factory.yml) on a connected **self-hosted Windows Unreal PC**. Nothing runs on ChatGPT in the background between conversations merely because these sources were committed.
- **Automatic versioned publication:** [publish_noworld_factory_results.py](../../Scripts/production/publish_noworld_factory_results.py) stages only source-produced genuine UE packages, exact original semantic catalog entries and scrubbed receipts for a Git LFS commit. It refuses overwrites and requires the job's explicit `publish_to_github=true`; raw Koikatsu cards/Blender user sources stay in the PC source archive.

### True, reusable production path
```text
original card PNG + canonical Identity/Version
  → SHA-256 authenticated job ID, persistent resumable state
  → installed Koikatsu Maker KKAPI + KKBP original mesh API queue
  → real original PMX + receipt
  → installed Blender KKBP imports source, saves .blend and real skin/morph GLB parts
  → UnrealEditor Python imports skeletal parts + textures/skeletons
  → native C++ AuthorNativeAppearance creates exact-bone native appearance UAsset
  → one global shared animation DataAsset (AUTO, never per form)
  → ContentFactory/Characters.json maps exact Identity/Version to real assets
  → existing UOGCharacterAppearanceComponent + AOGContentPlayableCharacter
  → native roster / World discovery + Android cook stage
  → when real kit/skills/source motions and owned Manifestation exist:
     existing World camera/input, true combat, and shared motion contact path
  → native receipt in Saved; approved source packages committed to GitHub LFS

original approved FBX action + real target Skeleton + original contact/approval
  → actual UE AnimSequence → exact rig and semantic binding
  → Content/NoWorld/Animation/SharedMotionCatalog.json
  → native traversal/combat/interaction dispatch from original gameplay systems

approved original artwork
  → real UE Texture2D → canonical visual registry/roster
  → native UMG without replacing source or inventing a new character
```

**Actual boundaries:** Imported geometry alone cannot invent exact skills, player ownership, action contact, approved scene performance, a World actor's gameplay authority, or the original lore/stat rules. The factory uses the existing runtime only when original definitions exist and are valid. The current bundle compiles **after native source integration is completed**, as explicitly directed, then the user alone accepts actual phone gameplay. Any missing installed exporter/mod, unusual multi-skin GLB, incompatible source Skeleton, non-authentic contact, missing canonical definition or source-art approval produces a precise `BLOCKED` state in `Saved/NoWorldContentFactory/<job-id>/status.json`, not a phantom playable character. UE source format compatibility and true visual behavior remain unverified until authorized native execution.

### Source entrypoints (5★)
- `Scripts/production/submit_noworld_content.py`: one API submission from original assets and automatically computed source hashes.
- `Scripts/production/noworld_content_factory.py`: idempotent stage scheduler and durable status; no destructive deletion/reset/reimport.
- `Tools/Koikatsu/NoworldContentFactory/NoworldCardExportQueue.cs` and `Scripts/production/install_noworld_factory_koikatsu.ps1`: isolated real Maker API queue, existing mod references, safe local plugin installation.
- `Scripts/production/noworld_kk_blender_generic.py`: authentic source Blender/KKBP skin, morph and geometry conversion.
- `Scripts/production/noworld_unreal_content_import.py`: native Unreal import for skinned character, original FBX animation, art.
- `Source/OfflineGame/{Public,Private}/Content/OGNativeContentFactoryEditorLibrary.{h,cpp}`: exact rig-bone/morph native DataAsset author, one persistent global shared motion UAsset.
- `Source/OfflineGame/Private/Characters/OGCharacterAppearanceComponent.cpp`, `Source/OfflineGame/Private/UI/OGGate12CharacterVisualCatalog.cpp`, `Config/DefaultGame.ini`: exact runtime actor/roster/cook link, originals take precedence.
- `Scripts/production/bind_native_motion.py`: real native PC source-to-semantic action catalog; extended to target authorized native project root.
- `Scripts/production/publish_noworld_factory_results.py` and [GitHub Actions](../../.github/workflows/noworld-content-factory.yml): native-only artifact publication and future off-device execution.
- `ContentFactory/windows_worker.json`: current documented Windows project paths; actual PC installations and executor presence are NOT verified here.

---
## Animation variation and reuse — current Creative Director clarification

The game has exactly **one shared semantic motion library**. Characters link to it directly; Version-dependent retargeting is allowed only when anatomy or actual Skeleton compatibility requires it. A retargeted motion is a reusable derivative of one original source clip, not a copied per-character library or a new gameplay action. Once produced, every truly compatible character, wardrobe or Version may reuse that derivative without re-exporting.

Different expressions of a semantic action are also permitted: authenticated authored alternate takes, character-specific timing and contact details, idle and transition personalities, secondary morph layers, posture and look variation, and contextual selection between compatible native derivatives. The goal is to avoid robotic repeated playback while retaining exact authored contacts, coherent multipart synchronization and real authoritative action results.

**Current implementation boundary:** the existing C++ shared motion resolver selects eligible clips by authored priority and exact native Skeleton; it is not yet an authoritative weighted/context-aware variety scheduler. Do not advertise automatic per-action variety as completed merely because multiple source files can be imported. Future implementation must choose a performance once per full character/multi-participant action (not independently per mesh part), require the same original source/scene phase across all parts, use a recorded deterministic source selection where gameplay continuity requires it, and never fabricate source motion or contact timings.

## Already integrated; use, don't duplicate
1. Canonical owned identity and native actor: `UOGCharacterAppearanceComponent` for generic characters, `AOGLuminousV3PlayableCharacter` for original Luminous, `FOGExactOriginalLuminousParts` for genuine imported source paths, `UOGCanonicalCombatActorBindingComponent` to connect actual battle World bodies. A form is a Version of an identity, not an animation library or cloned Character Identity.
2. One controller: `UOGSharedMotionControllerComponent`. Its reusable **BlueprintCallable** `BindNativeRigAndParts(Rig, Anatomy, Outfit, Capabilities, NativeParts)` installs all actual skeletal parts in one call, with fail-safe previous rig data. `BindCanonicalCharacter(Identity)` is also BlueprintCallable. Generic content actors bind the exact authored Identity before installation; original Luminous binds its canonical identity during ownership binding. Alternate Versions/wardrobes reuse that identity. The ordinary content-appearance installer and original Luminous native pawn now both use this same method.
3. One original animation bank: `Content/NoWorld/Animation/SharedMotionCatalog.json` (packaged UFS), read by `FOGSharedMotionResolver::GetInstalledLibrary()` in `Source/OfflineGame/Private/Animation/OGSharedMotionLibrary.cpp`. Preserve built-in native fallback entries. A single semantic action points to multiple source-preserving, exact-Skeleton native derivatives selected by rig, anatomy, capabilities, outfit and scene role — never by character Version.
4. Real combat: `FOGNativeBattleCommand::MotionSemantic()` and reusable `FOGNativeBattleCommands::MotionForResolvedAction()` preserve the original exact skill branch/mode → native UMG `UOGNativeBattleWidget` availability → `AOGWorldPrototypeCharacter::SubmitApprovedLuminousWorldCommand` → `UOGTurnContactActionComponent::StageActualTurnAction` → shared motion authored contact/explicitly sourced completion → original `FOGTurnBattle` executor. Native opponent AI and battle actor presentation reuse the same branch-specific source action; World real-time casts keep their separate existing physical executor. No second damage simulation or UI HP ledger.
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
      "loop": false
    }
  ]
}
```
**This is an illustrative format, not an available animation or a phone-test verdict.** Substitute real IDs, authentic source hashes, actual existing native assets and genuine authored contact data. The optional example `approval_evidence` is historical source provenance, never required owner acceptance. If an authored notify exists, use `contact_notify` instead of guessed timestamps. Explicit natural completion means the real source clip finishes normally before authoritative settlement; never use an invented hit time.

Single command/API call, runnable in the existing authorized pipeline by the developer or CI, not by the phone-only user:
```bash
python Scripts/production/bind_native_motion.py path/to/original_bindings.json
```
The binder checks schema, uniqueness, genuine .uasset paths, source SHA256 syntax, actual roles and impact information, and rejects conflicting duplicates; provenance is checked when supplied, without pre-phone owner approval. It edits **only** the globally packaged catalog via an atomic write. It never grants ownership, adds unapproved clips, creates an actor, remaps visual art, changes stats or retargets a skeleton invisibly. Actual UE native compatibility remains checked at source playback.

## Source-to-character fast binding
Content creators produce **only original content and reliable source receipts**:
- Koikatsu card plus original SHA, anatomy, wardrobe, genuine skinned export.
- Real Blender/Unreal skeletal assets with exact source provenance, retarget derivative only if needed, verified native object paths.
- Original approved motion clip metadata, action semantic, exact rig/scene role, contact notify or real sequence lifecycle, approval reference.

The established importer generates native asset receipts and source path registry; the binder associates gameplay semantic actions with the single global animation catalog. `UOGCharacterAppearanceComponent` / `AOGLuminousV3PlayableCharacter` installs the real visual parts using `BindNativeRigAndParts`, and the native combat/input/interaction routes automatically request those semantic actions.

**Do not** create 5 copies of a motion, hand-wire each attack per Version, invent extra Luminous forms, promote QA source clips, invent contact frames, introduce a disposable mini-game, or require the owner to operate a PC.

## Current unresolved content, clearly separated from binding infrastructure
The existing 937 UE-native combat motion entries are already present; they need authentic impact/contact binding or real completion behavior, not an artificial approval status. Grand original 57-renderer geometry is now recorded in the branch but must still reach actual complete actor, animation and gameplay integration. Remaining exact five-form executors must follow original approved v4 numeric sources. An Android APK requires real complete Unreal project cook and Android package; source changes here are not a testable release.

## Durable workflow for next conversation
Read 7★ → 6★ → this 5★ file → relevant native classes. Identify the shortest unintegrated **game system**; commit actual source and update 6★ with precise commit and remaining gameplay. Do not count a new binder, compiling, a receipt, a test or documentation as a completed 7★ system.

### Original Gate02 R1 native World geometry (reusable 5★ programs, 2026-11-02)
The existing `noworld_unreal_content_import.py` imports character Skeletons, original FBX actions and artwork. It does NOT prepare source-authored static World GLBs for the exact missing R1 module and twelve-section market runtime paths. Do not use that character importer to make empty gameplay proxies or reinterpret a source anchor as a character.

- `Scripts/production/noworld_r1_prepare_glb.py` is executed by actual Blender 5.2.2 in background with `--source-root <exact original 2026-10-19 RegionKit> --output-root <isolated staging> --role foliage|market|all`. Bakes original GLB coordinate transforms and joins original mesh objects without discarding vertices/polygons/triangles, producing three foliage static source GLBs and twelve spatial source partitions for the original Eastern Market. It emits original-source SHA256 and every derived GLB SHA256 with a geometry-count receipt. A source prep receipt is NOT an Unreal UAsset, Android render, owner acceptance or art approval.
- `Scripts/production/noworld_r1_unreal_import.py` runs **inside UnrealEditor Python** from an isolated native UE5.8 authoring project, with `NOWORLD_R1_PREPARED=<isolated staging>` and `NOWORLD_R1_RECEIPT=<isolated receipt.json>`. It imports exactly one real StaticMesh per original joined source GLB, preserving material/texture dependencies and putting it at the existing `/Game/NoWorld/FirstWorld/R1/Modules/SM_R1_{Fir,Oak,Shrub}Source` and twelve `/Game/NoWorld/FirstWorld/R1/Anchors/Market/SM_R1_EasternMarket_SourceSection_00..11` asset names. It refuses prior destination overwrite and missing source hashes; saving/importing does not validate phone gameplay.
- Native result packages must be copied back into the CURRENT GitHub branch using Git LFS for binary Unreal assets, with full project tree retained; never use a no-checkout empty-index Git commit that would delete existing files. Save the provenance and native receipt. Do not overwrite protected original PC projects. Continue exact Gate02 R1 513×513 RAW grid and authored 10,454 stable scatter IDs; no synthetic foliage or market proxies.
- Distinguish genuine authoring results from native compilation, packaged Android source-asset hydration and owner acceptance. Original authored kit itself notes shipping source-license validation and mobile optimization remain outstanding.
### Complete shared native-motion cook coverage (reusable 5★, 2026-11-02)
The original shared catalog contains 3,222 genuine native assets and 1,077 source clips (313 traversal, 1,070 interaction, 937 combat, 902 scene interaction); 3,201 of these assets reside in `/Game/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA`. The previous 900-original PrimaryAssetLabel was NOT a complete cook. `Config/DefaultGame.ini` now explicitly cooks this original Studio root as well as the other five authored source directories, while staging `NoWorld/Animation` as UFS. Do not equate full cook configuration with a real successful Android package.

- **Run** `python Scripts/production/verify_shared_motion_cook_coverage.py --source-project <read-only original full UE project> --config-project <current GitHub branch checkout>`. This independent read-only tool checks all 3,222 canonical motion identities, genuine non-pointer native Unreal package headers, all original paths, all six cook root inclusions and UFS manifest staging. It returns an explicit failing exit status if an original motion would be excluded from offline cooking.
- The pre-existing `reconcile_native_motion_catalog.py` remains the inventory/drift tool; this new reusable verification fills the **previously untested packaging gap**. It never reimports, modifies or falsely accepts animation assets. Genuine UE5.8 cook, physical animation playback/contact, device performance and owner approval remain separate requirements.
- `UOGSharedMotionControllerComponent::ResolveCoherentNativeAction` selects a single authentic `SourceClipId` compatible with every distinct source rig before interrupting playback, then aligns the true different native derivative durations. Exact outfit/context/capability/participant role and authored scene constraints remain enforced. Forced looping cannot manufacture a combat-contact completion event.

### Source-authentic combat contact recovery (reusable 5★ program, 2026-11-02)
- `Scripts/production/noworld_original_combat_contact_author.py` reads all **937 existing original Combat** entries from the one 3,222-entry catalog, groups by actual original Koikatsu Unity bundles and verifies each bundle's SHA256 before inspecting the exact source `AnimationClip` `path_id` with UnityPy. It only recognizes explicitly named source `AnimationEvent` markers such as `HitContact` and `WeaponImpact`, validates marker times against genuine native durations, preserves combat reactions and defenses as non-impact presentation, records ambiguous/unresolved clips, and refuses to infer impact timestamps from clip lengths or motion statistics. Default is read-only; `--apply` adds only proven source contact seconds to EXISTING catalog entries. Original native .uassets are never recreated. **No event evidence = no contact approval**. A source inventory/reconciliation program is not completed authentic skill-to-motion binding, editor rendering or phone acceptance.
- The generic `bind_native_motion.py` remains the transactional native source binder for genuine new authored imports. This additional extractor is necessary because the original 937 already have native import paths and source bundle IDs but no authored contact metadata; reimporting all original clips as new motions would duplicate the bank.


## Android owner-test APK production — native packaging, no assistant gameplay validation (2026-11-02)

- Reuse [`Scripts/production/build_noworld_android_test.ps1`](../../Scripts/production/build_noworld_android_test.ps1), committed at `798258871e8f438758e7ffcd1b3016f403478b7c`. This is an executable native Unreal Engine 5.8 PowerShell frontend for `BuildCookRun` targeting Android ASTC Development. It builds, cooks, stages, packages and archives the **existing** game, not a mock-up. The owner tests the result on the phone; no pre-owner gameplay test suite, artistic acceptance or new content approval gate is part of this workflow.
- Example: `powershell -ExecutionPolicy Bypass -File Scripts/production/build_noworld_android_test.ps1 -ProjectRoot D:\\UnrealProjects\\NoWorld_P0_NativeCompile_20261102 -EngineRoot 'D:\\Epic Games\\UE_5.8' -ArchiveRoot D:\\UnrealProjects\\NoWorld_Android_TestBuild_20261102`.
- Preserve the existing source/compiled binaries, source UAsset files and 3,222-entry source catalog. Native original content must be present in the isolated packaging checkout (hydrate existing Git LFS artifacts, or stage originals non-destructively from the protected source). No `clean` or reimport step is required. Logs, temporary files, and DDC stay in the isolated project on D: by default; APK existence is an output condition, not a gameplay-validation requirement.
- The first Android BuildCookRun was started manually with equivalent arguments on the authorized Windows worker, while keeping the protected source checkout unchanged. **A successful APK is not yet claimed.** Continue build/cook troubleshooting if it fails, and deliver the actual APK for the owner's test.
