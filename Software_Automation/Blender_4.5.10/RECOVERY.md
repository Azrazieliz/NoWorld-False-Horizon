# Recovery and Reproducibility

- Worktree: `D:\UnrealProjects\OfflineGame_BlenderMastery_20261015`, branch `software/mastery-blender`.
- Blender path: `D:\Tools\Blender\blender.exe`. Use only this binary for tests targeting 4.5.10.
- First inspect running processes and available memory; original initial free physical memory was around 1.35 GiB.
- Work in `Software_Automation\Blender_4.5.10`; verification artifacts in `VALIDATION\local`.
- Restore editable fixture by opening `NW_EngineeringTests.blend`. `NW_EditableVariant.blend` is a separate test derivative; do not overwrite base.
- Reproduce scripts using the exact commands in START_HERE; use `--factory-startup --threads 2 --background` and isolate temporary data.
- Failure history: roundtrip Action Slot incorrectly used `.name`; corrected to `.identifier`. Later variant test initially returned stale GN mesh; `obj.update_tag(); bpy.context.view_layer.update()` corrected the evaluation. These fixes are part of verified source.
- Review console logs and JSON reports. Do not delete failed reports during correction; document rerun result in continuation notes.
- If corrupted saved source: regenerate by rerunning `prototype_batch.py`. Scripts do not read or overwrite any production .blend file.
- Git recovery: `git -C D:\UnrealProjects\OfflineGame_BlenderMastery_20261015 status -sb`; inspect dedicated branch commits. Never reset or checkout another gate's branch.
- Render files are engineering previews, not art acceptance. Use saved source and explicit review before further promotion.
