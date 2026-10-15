# Gaea 2.3.1 — Functional Automated Terrain Bridge

Installed `D:\Tools\Gaea 2`, version 2.3.1.0. External MIT gaea-mcp at `D:\UnrealProjects\NW_Blender_ReferenceReview_20261015\external_sources\gaea-mcp`, installed in Python 3.11. Portable .NET SDK 8.0.426 at `D:\Tools\DotNet8SDK_NoWorld_Portable` enables real WPF UIAutomation helper. Its `gaea-doctor` returns READY with the tool helper compiled; 8 offline self-test groups passed.

`gaea_graph_job.py gaea_qa_config.json`: tested 512x512 true 16-bit source and erosion masks + 6-node Gaea `.terrain` graph with hydraulic erosion, settlement protection, color + height exports, metadata. Original game geography remains untouched. Native build attempt on 2026-10-15 failed at **GUI Open dialog did not appear**. Do not mark Gaea native build/World Partition as done.

Next: use the installed WPF UI Automation helper to open generated graph in Gaea and press Build, collect actual report + height PNG; use Landscape Editor integration after native build.
