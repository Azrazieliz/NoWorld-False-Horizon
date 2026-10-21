from pathlib import Path
import shutil,json,hashlib,time
gate=Path(r'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
src=Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G12_Luminous_Dance_FullCharacter_VisualRecovery_20261020')
dst=gate/'SourceAssets/Gate12/Animations20261020/LuminousDanceFullBodyQA'
if dst.exists():
 assert all(f.is_file() for f in dst.iterdir()),'Unexpected intake directory'
dst.mkdir(parents=True,exist_ok=True)
names=[
 'G12_LuminousV3_Koikatsu_Authentic_Dance_FULLBODY_INTERACTIVE_Front_20261020.blend',
 'G12_LuminousV3_Dance_RealKoikatsu975_FullBody_MotionReview_30fps.mp4',
 'G12_Luminous_Koikatsu_Dance_FRONT_110.png',
 'G12_ActualKoikatsu_DanceFullBody_030.png',
 'G12_ActualKoikatsu_DanceFullBody_110.png',
 'G12_ActualKoikatsu_DanceFullBody_180.png',
 'G12_NATIVE_ORIGINAL_SOURCE_DANCE_MOTION_VIDEO_MANIFEST.json']
records=[]
for name in names:
 f=src/name
 assert f.exists() and f.stat().st_size>(10000 if f.suffix.lower()!='.json' else 50),(name,f)
 out=dst/name
 shutil.copy2(f,out)
 def digest(path):
  h=hashlib.sha256()
  with path.open('rb') as fp:
   for b in iter(lambda:fp.read(1<<20),b''):h.update(b)
  return h.hexdigest()
 assert digest(out)==digest(f)
 records.append({'file':name,'bytes':out.stat().st_size,'sha256':digest(f)})
print('GATE12_DANCE_FULL_SOURCE_INTAKE_COPIED',len(records),
 sum(x['bytes'] for x in records),'BYTES',flush=True)
source_map='/Game/Gate12NativeKoikatsu20261020/VisualQA/L_LuminousV3_Dance_Full45_UE583_VisualQA'
qa=json.loads((gate/'Saved/Gate12AnimationAcceptance/Gate12_FULL45_DANCE_COLD_PERSISTENCE_20261020.json').read_text())
assert qa['status']=='PASS_COLD_45_NATIVE_MESHES_SINGLE_ANIMATION_AND_44_FOLLOWERS'
method=f"""NOWORLD / GATE 12 — LUMINOUS BASE V3 — ACTUAL DANCE ANIMATION INTAKE (2026-10-20)
=======================================================================================

ACTUAL READY DATA
- 7.300s "Dancing 2" original Koikatsu-compatible mod motion, 975 bone tracks.
- 6 preserved original Koikatsu skinned meshes and original materials in this BLEND file.
- 220-frame / 30fps motion-review MP4 in this folder. Review rendering uses
  fast Workbench approximated appearance; source Blender + PNG stills preserve
  detailed original shader look.
- UE5.8 native AnimSequence (in existing G12 project):
  /Game/Gate12NativeKoikatsu20261020/FlightDanceSwimming/
  G12_LuminousV3_Koikatsu_Dance_Performance
- UE5.8 ORIGINAL 45-PART CLOTHED full-character verification level:
  {source_map}
- Cold UE5.8 QA receipt:
  Saved/Gate12AnimationAcceptance/Gate12_FULL45_DANCE_COLD_PERSISTENCE_20261020.json
  PASS — 45 original USkeletalMeshActors, 44 persistent LeaderPoseComponent
  pointers, exactly one persistent animation sequence on the leader.
- Native QA authoring Python:
  Scripts/Gate12Koikatsu/author_full45_existing_luminous_real_dance_unreal_20261020.py
  Native cold persistence Python:
  Scripts/Gate12Koikatsu/cold_verify_full45_luminous_native_dance_20261020.py

EXACT GAME INTEGRATION METHOD (PRESERVE PREEXISTING GATE12 NATIVE FOUNDATION)
1. Inspect open 45-mesh QA level, press Play/Simulate and confirm continuously
   readable 7.3s motion on the FULL Luminous body. Do not repeat earlier single
   Mesh_00_Prim_00 (395 triangle) animation editor view.
2. Keep the existing AOGLuminousV3PlayableCharacter C++ and its
   ApplyVisualWardrobe/SetLuminousNativeMotionClip. It already selects the true
   Clip->GetSkeleton() leader from 45 active CLOTHED native components; checks
   same topology/reference pose for all; sets followers via
   SetLeaderPoseComponent(Leader); plays Clip on leader only.
3. For editor/Python-authored QA maps use
   Leader->OverrideAnimationData(Clip,true,true,0,1) rather than temporary
   PlayAnimation; persistent 44 followers use SetLeaderPoseComponent.
4. In actual World Mode, call the existing
   PlayGate09AuthoredEmote(EOGLuminousV3AuthoredEmote::DancePerformance) on
   a READY, GROUNDED, STATIONARY, CLOTHED Luminous actor. Use a proper user
   action or QA trigger; don't create a substitute pawn or fake mesh.
5. Verify an Unreal PLAY/PIE screen recording from that exact gameplay actor
   with 45/45 parts synchronized, costume/hair attached, full start/end,
   7.3s animation, and clean return to traversal.
6. Source-side full character Blender black-preview correction:
   source "Icosphere" is unmaterialed in front of avatar; hide it in the
   QA scene only. Clear old timeline camera markers, use real camera,
   original six skinned meshes + 975 bone rig + their materials and lights.
   Never change the sealed canonical character source.

ACCEPTANCE LIMITS
- Full authentic dance works and is visibly rendered in Blender.
- 45 native Unreal mesh actors, 44 persistent links, 1 persistent native
  animation asset have passed independent cold reload in the ACTUAL Gate12 game.
- Actual 7.3s LIVE PIE/gameplay visual playback and final art approval are not
  yet verified. Do not claim this as a final accepted in-game emote until then.
- Existing Gate12 C++ source modifications remain unbuilt due a prior broader
  module compilation error. Gate12 owns that integration checkpoint.
- This QA map is separate from the first World and may be safely discarded
  only after the actual character runtime has passed.
"""
(dst/'GATE12_NATIVE_LUMINOUS_FULL45_DANCE_HANDOFF_20261020.txt').write_text(method,encoding='utf8')
manifest={'schema':'NOWORLD_GATE12_AUTHENTIC_KOIKATSU_FULL_CHARACTER_DANCE_NATIVE_QA_V1',
 'created_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),
 'actual_original_source':records,'animation':'Dancing 2',
 'real_motion_seconds':7.3,'fps':30,'source_skeleton':975,
 'unreal_full45_cold_check':qa['status'],'qa_unreal_map':source_map,
 'original_existing_world_or_skeleton_overwritten':False,
 'Unreal_live_PIE_visual_accepted':False}
(dst/'GATE12_DANCE_SOURCE_INTAKE_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print('GATE12_COLD_VERIFIED_DANCE_SOURCE_INGESTED_WITH_METHOD',dst,flush=True)
