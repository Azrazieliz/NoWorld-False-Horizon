"""Non-destructive professional layered compositing, original v9 Blender
Luminous render + 12 actual geometric sword motion-trail alpha passes.
Preserve source frames, don't AI interpolate, don't mislabel low-res as full-res.
"""
from pathlib import Path
from PIL import Image,ImageFilter,ImageStat,ImageChops
import subprocess,hashlib,json,statistics
P=Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
BASE=P/'Luminous_A1_GroundedRootMotion_v9_ActualFrames'
FX=P/'Luminous_A1_ScarletCadence_v10_CleanSwordFX_AlphaFrames'
OUT=P/'Luminous_A1_ScarletCadence_v10_CompositedProductionFrames'
VIDEO=P/'Luminous_A1_ScarletCadence_v10_Composited_RealBladeTrail_30fps.mp4'
REC=P/'Luminous_A1_ScarletCadence_v10_CompositedProductionQA.json'
for f in (OUT,VIDEO,REC):
 if f.exists():raise FileExistsError(f)
OUT.mkdir()
assert len(list(BASE.glob('frame_*.png')))==33
fxnums=sorted(int(f.stem.split('_')[-1]) for f in FX.glob('fx_*.png'))
expected=sorted({i for hit in (6,12,20) for i in range(hit-2,hit+2)})
assert fxnums==expected,(fxnums,expected)
proof=[]
for i in range(1,34):
 with Image.open(BASE/f'frame_{i:04d}.png') as bg:
  img=bg.convert('RGBA')
  if i in fxnums:
   with Image.open(FX/f'fx_{i:04d}.png') as file:
    fx=file.convert('RGBA')
    assert fx.size==img.size,(fx.size,img.size)
    a=fx.getchannel('A'); bbox=a.getbbox()
    # A legitimate effect alpha should be present near a strike; no zero-opacity FX.
    assert bbox is not None,(i,'missing physical effect pixels')
    glow=fx.filter(ImageFilter.GaussianBlur(9))
    glow.putalpha(glow.getchannel('A').point(lambda x:round(x*.26)))
    img=Image.alpha_composite(Image.alpha_composite(img,glow),fx)
    proof.append({'frame':i,'opaque_fx_bounds':bbox})
  img.convert('RGB').save(OUT/f'frame_{i:04d}.png',compress_level=3)
ff=P.parent.parent/'Tools/FFmpeg/ffmpeg-9.0.2-full_build/bin/ffmpeg.exe'
ff=Path(r'D:\Tools\FFmpeg\ffmpeg-9.0.2-full_build\bin\ffmpeg.exe')
fp=ff.with_name('ffprobe.exe')
subprocess.run([str(ff),'-hide_banner','-loglevel','error','-y',
 '-framerate','30','-start_number','1','-i',str(OUT/'frame_%04d.png'),
 '-c:v','libx264','-preset','medium','-crf','15','-pix_fmt','yuv420p',
 '-movflags','+faststart',str(VIDEO)],check=True,timeout=95)
data=json.loads(subprocess.check_output([str(fp),'-v','error','-select_streams','v:0',
 '-show_entries','stream=nb_frames,width,height,r_frame_rate,codec_name',
 '-of','json',str(VIDEO)],text=True))['streams'][0]
assert data['nb_frames']=='33' and data['r_frame_rate']=='30/1'
result={'schema':'G09_A1_NARROW_PHYSICAL_SWORD_TIP_TRAIL_COMPOSITE_V10',
 'status':'SOURCE_PREVIEW_33_REAL_CHARACTER_AND_12_PHYSICAL_SWORD_EFFECT_ALPHA_FRAMES',
 'production_video':str(VIDEO),'video_sha256':hashlib.sha256(VIDEO.read_bytes()).hexdigest(),
 'video_bytes':VIDEO.stat().st_size,'video_stream':data,
 'original_render_source':str(BASE),'source_blender_v9':str(P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.blend'),
 'fx_blender_v9':str(P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_v10.blend'),
 'rendered_effect_frame_count':len(proof),'fx_overlay_bounds':proof,
 'unreal_gameplay_ready':False,
 'quality_acceptance':'REQUIRES_FINAL_HUMAN_AND_INGAME_RENDER'}
REC.write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
print('G09_SWORD_HISTORY_COMPOSITE_FINISHED',VIDEO,
 'MP4_SHA',result['video_sha256'],'REAL_FRAMES',data['nb_frames'],
 'EFFECT_FRAMES',len(proof))
