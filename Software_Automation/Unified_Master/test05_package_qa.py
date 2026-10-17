"""Only proven noncanonical Test05 outputs. Never bundle unrelated source/canonical materials."""
import pathlib,zipfile,hashlib,json
r=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
native=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\UserData\chara\female\SM01_AUTOTEST_T05_CRYSTAL_SERAPH_NATIVE_20261017.png')
items=[
 r/'TEST05_CrystalSeraph_Editable_Portable_FINAL_QA.blend',
 r/'TEST05_ActualNativeBody_ArtPass_v4.png',
 r/'TEST05_FINAL_QA_RECEIPT.json',
 r/'TEST05_REAL_NATIVE_REBIND_RECEIPT.json',
 r/'TEST05_PORTABLE_ARTIFACT_QA.json',
 r/'TEST05_NATIVE_BATCH_TIMINGS.json',
 r/'TEST05_KOIKATSU_BASE_RECIPE.json',
 native
]
for f in items:
 if not f.is_file():raise FileNotFoundError(str(f))
z=r/'TEST05_CrystalSeraph_Disposable_QA_20261017.zip'
if z.exists():raise FileExistsError('Test output already packaged')
notice="""NOWORLD TEST 05 — DISPOSABLE CAPABILITY QA / NOT CANONICAL
This 3D result FAILED artistic reference-likeness acceptance.
The .blend is genuine native Blender 5.2.2, editable and contains real Koikatsu
skinned/morphed face/body geometry, independently modeled rigged wing/tail
placeholders and 16 packed original source color/face/iris textures.

Opening: unpack ZIP, then open TEST05_CrystalSeraph_Editable_Portable_FINAL_QA.blend
in Blender 5.2.2. All image textures are packed.
Press NUMPAD 0 or use 'Test05 Full-Body Camera' in scene to see the test framing.
The attached PNG is the actual Blender render, NOT an approved fidelity result.

The included Koikatsu native card is disposable. The top clothing slot was NOT
successfully neutralized; a default school uniform remains on this card.
No jewelry or pendants from the reference were intentionally modeled.
No production source, original game installation or registry was edited.
Source and pipeline fixes are recorded in the persistent Software Mastery registry.
"""
with zipfile.ZipFile(z,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=3) as out:
 out.writestr('READ_ME_FIRST.txt',notice)
 for f in items:out.write(f,f.name)
sha=hashlib.sha256(z.read_bytes()).hexdigest()
receipt={'schema':'NW_TEST05_PACKAGED_QA_V1','package':str(z),
 'bytes':z.stat().st_size,'sha256':sha,
 'archived_items':[p.name for p in items],
 'visual_fidelity_accepted':False,'native_card_wardrobe_neutral':False,'canonical_content_modified':False}
(r/'TEST05_QA_PACKAGE_RECEIPT.json').write_text(json.dumps(receipt,indent=2))
print('TEST05_QA_ZIP',z,'size_MB',round(z.stat().st_size/1048576,2),'sha256',sha,flush=True)
