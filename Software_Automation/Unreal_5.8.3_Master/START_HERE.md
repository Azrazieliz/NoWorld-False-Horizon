# Unreal 5.8.3 automation — Gate 12

**Engine:** `D:\Epic Games\UE_5.8`; isolated project `D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015\NW_BlenderInterop.uproject`.

### Discover what Unreal actually exposes, rather than only using it as a compiler

`MASTER_API/UE58_ExposedAPIs_Installed.json`: **11,174 exposed Python bindings**, grouped by domain with methods and brief signatures. This is a searchable **discovery index**, not proof that all APIs have production-ready workflows.

```powershell
& 'D:\Tools\Python311\python.exe' 'D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unreal_5.8.3_Master\MASTER_API\search_unreal.py' --query 'PCG WorldPartition' --limit 8
```

### Executable native workflows proven in isolated project

- `native_asset_author.py` + `native_author_job_runner_v2.json`: **5 native asset families** persisted and reloaded: parametric PBR Material; 4-node / 3-edge PCG graph; Sequencer CineCamera transform animation with **18 real keys**; Niagara System base asset (**emitter configuration NOT validated**); Actor Blueprint with generated class. Native Blueprint compile separately verified.
- `qa_batch_import.py`: source-hash-safe FBX import and native asset verification, proven on architecture, modular weapon and Gaea terrain tiles. Only sandbox destination `/Game/NW_MasterHub_QA/`.
- `qa_animation_validate.py`: inspect actual native pose transforms at multiple times, not just clip frames. Test returned **7 moving bone tracks** for basic flight and **15** for swimming. These are motion-technical proofs, not acceptable artistic locomotion.
- `qa_paper2d_flipbook.py`: Blender Grease Pencil v3 source → seven PaperSprite textures → 56-frame native Paper2D Flipbook.
- `qa_pcg_procedural.py`, `qa_sequencer_keyed_v3.py`: standalone native PCG and cinematic-script acceptance.

### Additional Unreal capability areas already indexed but needing game-specific tests

Gameplay Framework, Enhanced Input, GAS, Gameplay Tags, AI/StateTree/Behavior Trees, World Partition, landscape, water, Geometry Script, Chaos, Niagara emitter programming, Control Rig, Motion Matching/Pose Search, animation graphs, material graphs, UI, audio, SaveGame, async C++, Android profiling/builds. Use the real engine subsystems, C++/Blueprint if Python is inadequate. This is NOT authorization for software mastery to rewrite Gate12 code or frozen Foundation.

**Gate 12 should use `Unified_Master/gatectl.py` or `pipeline_runner.py` for automation** with deterministic source hashes and native postconditions. User functional tests determine what needs to be implemented next. Existing visual source quality is not accepted as AAA proof.
