# GATE 01 / GATE 12 — IMMEDIATE BLENDER 5.2.2 AUTOMATION HANDOFF

**Status: engineering operations proven in installed Blender. Not high-end art signoff.**
**Branch:** `software/mastery-blender` in `Azrazieliz/Offline-Game` — do not merge into frozen Foundation.
**Blender binary:** `D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe`
**Package root:** `Software_Automation/Blender_5.2.2/`
**Command runner:** `CLI/gate_runner.py`
**Native add-on:** `ADDONS/noworld_mastery_hub/`; in Blender 3D View press **N → NoWorld**.
**Output:** always choose a distinct derivative name. Existing source and outputs NEVER overwritten.

## Gate 01 — usable immediately

**Rig/model inspection:** produces JSON describing real UV maps, missing materials, morph channels, bone counts, Action+Slot, external images. Example:

```powershell
$B = "D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe"
$Package = "D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_5.2.2"
& $B --background --factory-startup --threads 2 --python "$Package\CLI\gate_runner.py" -- --op audit --source "YOUR_CHARACTER.blend" --report "YOUR_AUDIT.json"
```

**Non-destructive hair/outfit/material gradient:** works on existing mesh materials. Duplicates material, preserves any source image-based Base Color connection and creates reusable native `NW_Gate_Gradient_1` node group. Artist controls: Shadow/Highlight tints, Gradient Axis, Offset, Asymmetry, Noise Scale/Strength and Strength. `--world` anchors continuity across separate objects through a shared Empty, while default Generated coordinates are per mesh. Produces a NEW editable .blend; source not altered.

```powershell
& $B --background --factory-startup --threads 2 --python "$Package\CLI\gate_runner.py" -- --op gradient --source "YOUR_CHARACTER.blend" --object "HAIR_MESH_OBJECT_NAME" --strength 0.85 --world --output "CHARACTER_GRADIENT_DERIVATIVE.blend" --report "GRADIENT_EVIDENCE.json"
```

**Skeletal FBX for Gate 12:** preserve existing armature, bounded deformed meshes, Blender source Action Slot. Exports native FBX without modifying original .blend.

```powershell
& $B --background --factory-startup --threads 2 --python "$Package\CLI\gate_runner.py" -- --op skeletal --source "YOUR_CHARACTER.blend" --rig "YOUR_ARMATURE_OBJECT" --output "CHARACTER_SKELETAL.fbx" --report "SKELETAL_EVIDENCE.json"
```

**UNRESOLVED FOR GATE 01:** original KK shader parity (eye line, stockings), animation rest alignment and high-end visual output, per-bone corrective shapes, direct KK in-game editing and gradient propagation into original Koikatsu mod context. Do NOT promote unapproved character quality based on successful export.

## Gate 12 — usable immediately

**Static meshes**, with exact names optional (`--objects A B C`; otherwise all mesh objects). The native runner restores Blender selection and leaves the source untouched:

```powershell
& $B --background --factory-startup --threads 2 --python "$Package\CLI\gate_runner.py" -- --op static --source "YOUR_ENVIRONMENT.blend" --output "ENV_STATIC.fbx" --report "STATIC_EVIDENCE.json"
```

**Actual reusable game animation action**, not an MP4. Requires a deform armature with assigned Blender 5.2 Action+Slot; exports ARMATURE only, baked frames, no NLA-strips/all-actions ambiguity:

```powershell
& $B --background --factory-startup --threads 2 --python "$Package\CLI\gate_runner.py" -- --op animation --source "YOUR_RIGGED_ACTION.blend" --rig "YOUR_ARMATURE_OBJECT" --output "ACTION_ONLY.fbx" --report "ACTION_EVIDENCE.json"
```

**Native UE 5.8.3 testing:** isolated `NW_BlenderInterop` project has imported this runner's static output as **3 native StaticMesh** assets and its ARMATURE-only output as **1 native AnimSequence** associated with a compatible Skeleton (see `VALIDATION/unreal_583_native_handoff_results.json`). This validates importer compatibility only, not visual materials, contact, retarget pose, gameplay, Android or final production packages.

**Further native UE prior work:** `INTEGRATIONS/UE583/test_notify_authoring.py` previously authored and saved 2 real timed notifies. Separate noncanonical CMU punch testing produced target 121-bone clip and 3 notifies but **visual retarget quality failed**, and no AAA motion signoff is implied. Gate 12 must still validate game controls, X/Y-facing, contact, scale, materials, Android runtime and no import degradation.

## Blender GUI controls (installed)
In 3D View press **N**, open **NoWorld** tab.
- Select domain and workflow to see exact UI/automation route and source reference.
- `One-click Read-only Audit` writes JSON to temporary runtime_audits.
- `Gate 01/12 — Do Real Work`: select output folder, output name, STATIC/SKELETAL/ANIMATION. Click `Export Native FBX + Evidence`. A JSON evidence file is written alongside FBX.
- Choose a textured mesh and click `Create Non-Destructive Gradient`, adjust group inputs in Shading. `Save Separate Editable Source` creates new .blend.
- `Open Complete Workflow Book` provides searchable 28-domain-control references.
- `Installed Tools Report` checks CloudRig, Retarget and Rigify presence.

## Tool selection
- **KK:** canonical source, faces/wardrobe, original shaders and mod-side parameters. Blender does not overwrite ownership or invent character identity.
- **Blender:** native quality source editing, pose/action source, shape keys, procedural materials, UV/FBX with preserved source.
- **Unreal 5.8.3:** IK Rig/Retargeter, Control Rig final polish, AnimSequences, Montages, runtime hit event via AnimNotify, Niagara and Android material/performance.
- **Gaea 2.3.1:** native terrain erosion/height/masks; Swarm currently blocked in remote console (no accepted native terrain package).

## Safety and provenance
No public/private character data, imported copyrighted YouTube footage, or actual Gate 01/12 production assets are bundled. Only software scripts, workflow metadata, empty-safe recipes, sample synthetic noncanonical QA assets and normalized evidence.
Do NOT replace a Gate file with a test derivative or import a test into the frozen Foundation. Do not merge this branch without Gate 00 authorization.

## Minimum acceptance beyond technical execution
References and artistic-quality approval, correct identity, matching engine shaders under controlled light, original and Unreal bone/pose parity, gameplay/replay, no foot sliding, Android test, and Gate 01/12/00 relevant signoff. Technical outputs are **not** accepted professional demos (0/25).
