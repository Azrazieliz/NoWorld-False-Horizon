# Gaea 2.3.1 automation — Gate 02

**Real installed executable:** `D:\Tools\Gaea 2\Gaea.exe` + native build tools.
**Automation:** MIT-licensed gaea-mcp in `D:\UnrealProjects\NW_Blender_ReferenceReview_20261015\external_sources\gaea-mcp`, Python 3.11 and portable .NET 8 installed.
**Safety:** test graphs in `D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015`; active Gate 02 GUI sessions must not be interrupted.

**Validated end to end on isolated 512² example:** Gaea native erosion → exported 16-bit `PNG` and flow/wear/deposit EXR layers → Blender UV/masked 4-tile mesh (0.0 m seam error) → four Unreal 5.8.3 native StaticMeshes. Gaea's own `report.json` confirms seven nodes at 512², `AlreadyBuilt` result on checked state. Evidence `VALIDATION/GAEA_NATIVE_512_OFFICIAL_REPORT.json`, `BLENDER_NATIVE_EROSION_TILES.json`, `UNREAL_NATIVE_EROSION_IMPORT.json`.

**Automated calls:**
- `gaea_graph_job.py <new-config.json>` — generate source `.terrain`, 16-bit image/mask and metadata; an example exists in `gaea_qa_config.json`
- `gaea_native_unattended_build.py <exact-private-project-folder>` — real GUI graph-specific build, only with exclusive GUI permission and verified project window title. This is not a general headless CLI API.
- `gaea_blender_tiled_mesh.py <new-config.json>` — generated erosion heightfield to editable terrain tiles + UV/mask + Unreal FBX
- `verify_native_outputs.py` and `VALIDATION` JSON — verify source/build output; inspect original files for reproducibility.

**Mapped safe node subset:** `File, Adjust, Erosion2, Height, Tint, Combine, Export, Mesher, Constant, Blur`. The source-backed `gaea_mcp.terrain.NODE_SPECS` includes their known-compatible ports and defaults, plus unsafe node exclusions. Access these via `Unified_Master/software_catalog.py --search <term> --software gaea`. This is not the full Gaea node universe.

**Gate 02 responsibilities:** choose true geography and real terrain quality; use Blender to add cliffs, caves, rock formations, settlement architecture and volumetric geometry; Gate 12 decides Unreal Landscape/World Partition/PCG/native runtime treatment. UE Landscape and World Partition are not yet native-tested in this pipeline; real native **StaticMeshes** are.

**Next test:** new original Gate02 terrain graph with altered parameters and unattended native exports, then landscape import. Do not mark new terrain worlds canonical without Gate00/02 approval.
