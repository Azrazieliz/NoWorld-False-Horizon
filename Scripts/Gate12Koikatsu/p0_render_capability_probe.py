"""No-cook editor renderer/screenshot capability introspection."""
import unreal,json,pathlib
p=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"render_api_probe.json"
out={"schema":"P0_RENDER_API_PROBE_V1","classes":{},"settings":{}}
classes=("AutomationLibrary","RenderingLibrary","SceneCaptureComponent2D",
         "TextureRenderTarget2D","EditorLevelLibrary","EditorViewportLibrary",
         "MaterialEditingLibrary","MaterialFactoryNew","MaterialExpressionTextureSample",
         "MaterialExpressionTextureSampleParameter2D","SceneCaptureSource")
for n in classes:
    cls=getattr(unreal,n,None)
    if cls is None:out["classes"][n]=None;continue
    out["classes"][n]=[x for x in dir(cls) if any(k in x.lower() for k in (
        "render","screenshot","capture","export","frame","viewport","material",
        "texture","scene","camera","create","connect","color"))][:75]
for n in ("BlendMode","MaterialProperty","TextureRenderTargetFormat","TextureRenderTargetFormat",
          "SceneCaptureSource","TextureMipGenSettings"):
    cls=getattr(unreal,n,None)
    if cls:out["settings"][n]=[x for x in dir(cls) if x.isupper() or "TRANS" in x]
p.parent.mkdir(parents=True,exist_ok=True)
p.write_text(json.dumps(out,indent=2)+"\n")
unreal.log("P0_RENDER_PROBE "+str(p))
