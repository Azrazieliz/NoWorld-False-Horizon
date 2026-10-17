"""Append tested Test5 knowledge, without touching any canonical content."""
import subprocess,sys,pathlib,json
ROOT=pathlib.Path(__file__).resolve().parent
current=json.loads((ROOT/'MASTER_LEARNED_METHODS_V1.json').read_text(encoding='utf8'))
items=[
('K038','KKBP_mesh_and_armature_double_scale_regression','gate01|gate09|gate12',
 'Never scale imported KKBP skinned mesh AND referenced armature with the same global factor: actual evaluated geometry may become squared-scale despite a correct static object bounding box.',
 'After KKBP import use evaluated depsgraph skinned world-space vertices to measure height; for legacy Test03 import with scale 0.08, use mesh object scale 12.45 and leave original armature scale 1.0. Require physically plausible height before rendering or UE export. Do not infer final height from undeformed local bounds.',
 '2026-10-17 Test05 source armature and mesh scaled 12.45 each, actual evaluated body 20.093 m; resetting armature scale to 1 yielded 1.600178 m and newly reimported Test05 native body 1.597361 m, both without removing 452 old shape keys.',
 'verified_native_depsgraph_repair','test05_qa_and_regressions.py body_not_double_scaled and native_kkbp_body_height; never accept evaluated height outside 1.4..2.0 m for this validated Test05 body.'),
('K039','KK_native_pmx_source_identity_and_materials','gate01|gate09|gate12',
 'A Blender scene using a previous Test character body is a provisional construction base, not source-authentic Test5 character even when rigged. Bind to the fresh actual native Koikatsu saved-card revision and real exported PMX with original face/eye texture atlas rather than flat colors.',
 'Run one Maker session, native morph/hair/iris batch, save+independent card reload, export PMX; verify PMX creation and hash; import into Blender with mmd_tools then connect original native exported face/iris and skin PNGs to the correct original UV material slots. Compare real evaluated body dimensions; keep earlier QA versions separate.',
 '2026-10-17 Test05 native 21 operation batch 10.918 seconds, fresh native card independently reloaded, KKBP 5.6 export model.pmx hash-locked and imported into Test05 v4 with 1.597 m evaluated height and 16 real native face/eye/body material image nodes; no art fidelity acceptance.',
 'native_source_editability_verified_visual_failed','test05_qa_and_regressions.py source_native_pmx_sha256, source_native_eye_and_face_texture_nodes and latest native v4 rig height, plus card source receipt.'),
('K040','complex_anime_anatomical_detail_not_procedural_sticks','gate01|gate09',
 'Reference-level dragon-seraph cannot be produced by filling scene with thousands of generic feather strips or flat triangular bat membranes. Object count and successful rendering are not artistic fidelity. Complex character identity demands mature facial sculpt, plate topology, coherent plum-to-lilac volumetric hair locks, real lamellar armor, membrane transparency/detail, wings and tail integration.',
 'For complex Test5-like creatures require at least one immediate full-body/face/hair visual inspection BEFORE declaring success. Reject toothpick horns, thin wire hair, unlayered bat triangles, crude central black polygon, default young face and oversimplified wings; substitute matching detailed rigged assets or professional sculpt/texture workflow. Stop pointless material tweaking when topology itself is wrong.',
 '2026-10-17 Test05 v1 and v4 actual Blender render inspected: genuine native skinned asset, 1882 mesh objects and 452 source morphs plus 4 articulated wings but major reference mismatch. TEST05_FINAL_QA_RECEIPT status ENGINEERING_FIXES_VERIFIED_VISUAL_BENCHMARK_FAILED.',
 'known_visual_failure_no_canonical_acceptance','test05_qa_and_regressions.py asserts visual_fidelity_status FAIL_REFERENCE_LIKENESS and no_art_fidelity_approval, never accept rendering/data counts as high-fidelity evidence.'),
('K041','KK_clothing_top_neutral_api_gap','gate01',
 'Current Koikatsu set_clothes native operation does NOT support index 0 for top/clothing neutralization; setting slots 1-8 to zero leaves the default school uniform. Never claim a neutral starting state until TOP is visually and natively verified.',
 'Do not call set_clothes index0 and treat it as success. Rework/source-inspect the native runtime to provide a tested top slot none/neutral option or use a truly neutral original card/coordinate. Verify actual clothing 0..8 IDs, Maker render, native saved-card independent reload before calling neutral base accepted.',
 '2026-10-17 Test05 set_clothes index0 --effective-id 0 returned Invalid or unsupported clothing part, while actual native screenshot retained school uniform and Top remained unchanged in saved user QA card. Corrective neutral top API still missing.',
 'UNRESOLVED_TOP_SLOT_API','test05_qa_and_regressions.py native_KK_source marks PASS_CARD_SAVED_BUT_TOP_SLOT_NOT_NEUTRAL; do not claim accepted neutral garment until source patch and visual proof.')
]
already={r['id'] for r in current['rules']}
for rec in items:
 id,topic,scope,req,proc,evidence,status,regression=rec
 if id in already:print('EXISTS',id);continue
 args=[sys.executable,str(ROOT/'record_learning.py'),'--id',id,'--topic',topic,
 '--applies-to',scope,'--requirement',req,'--procedure',proc,'--evidence',evidence,
 '--status',status,'--regression',regression]
 p=subprocess.run(args,capture_output=True,text=True,timeout=25)
 print('APPEND',id,p.returncode,p.stdout[-160:],p.stderr[-160:],flush=True)
 if p.returncode:raise SystemExit(p.returncode)
