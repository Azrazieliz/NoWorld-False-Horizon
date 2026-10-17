"""Finalize Test05 noncanonical Blender benchmark as internally texture-packed portable .blend.
Never overwrite old source. No visual-acceptance claim.
"""
import bpy,pathlib,json,hashlib
root=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
images=[x for x in bpy.data.images if x.source=='FILE' and x.type=='IMAGE']
before=sum(x.packed_file is not None for x in images)
missing=[(x.name,x.filepath) for x in images if not x.packed_file and not pathlib.Path(bpy.path.abspath(x.filepath)).is_file()]
if missing:raise RuntimeError('Cannot make portable; external original images unavailable: '+str(missing[:5]))
bpy.ops.file.pack_all()
after=sum(x.packed_file is not None for x in images)
if after!=len(images):raise RuntimeError('Portable pack did not embed all linked source images')
dest=root/'TEST05_CrystalSeraph_Editable_Portable_FINAL_QA.blend'
if dest.exists():raise FileExistsError(dest)
bpy.ops.wm.save_as_mainfile(filepath=str(dest))
sha=hashlib.sha256(dest.read_bytes()).hexdigest()
q={'schema':'NW_TEST05_PORTABLE_SOURCE_QA_V1','source_native_blend':bpy.data.filepath,
 'final_qa_blend':str(dest),'size_bytes':dest.stat().st_size,'sha256':sha,
 'source_images':len(images),'images_packed_before':before,'images_packed_after':after,
 'native_character_card_source':'SM01_AUTOTEST_T05_CRYSTAL_SERAPH_NATIVE_20261017.png',
 'visual_reference_likeness_accepted':False,
 'canonical_content_modified':False,'native_material_parity_accepted':False}
(root/'TEST05_PORTABLE_ARTIFACT_QA.json').write_text(json.dumps(q,indent=2))
print('TEST05_FINAL_PORTABLE_QA',dest,'size_MB',round(dest.stat().st_size/1e6,1),'images',after,'sha',sha[:18],flush=True)
