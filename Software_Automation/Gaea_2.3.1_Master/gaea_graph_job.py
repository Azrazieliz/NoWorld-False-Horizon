"""Native Gaea 2.3.1 terrain authoring using installed gaea-mcp ProjectBuilder.
Creates actual .terrain with 16-bit terrain/mask and 2 Export nodes. Proven
offline structure, runtime GUI validation is a separate outcome.
"""
import pathlib,sys,hashlib,json,math
import numpy as np
from scipy.ndimage import gaussian_filter
from gaea_mcp.terrain import ProjectBuilder
from gaea_mcp.outputs import Grid,write_heightmap_png,write_mask_png,write_meta
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def create(cfg):
 folder=pathlib.Path(cfg["output_directory"]).resolve()
 if folder.exists():raise FileExistsError(folder)
 folder.mkdir(parents=True)
 n=int(cfg.get("resolution",512));assert n in (256,512,1024,2048)
 width=float(cfg.get("world_width_m",6000))
 relief=float(cfg.get("relief_m",900))
 assert width>400 and 5<relief<6000
 seed=int(cfg.get("seed",4242))
 rng=np.random.default_rng(seed)
 axis=np.linspace(-1,1,n,dtype=np.float32)
 x,y=np.meshgrid(axis,axis,indexing="xy")
 ridge=np.exp(-((y-.36-.34*np.sin(3*x))/.35)**2)*(0.8+.12*np.cos(x*7))
 basin=-.4*np.exp(-(x/.22)**2-((y+.19)/.75)**2)
 foothills=.16*np.sin(4*x+3*y)*np.sin(7*y-2*x)
 detail=np.zeros((n,n),dtype=np.float32)
 for span,amp in [(55,.12),(23,.07),(9,.035),(3,.014)]:
  noise=gaussian_filter(rng.normal(0,1,size=(n,n)).astype(np.float32),sigma=span)
  peak=np.percentile(abs(noise),98);detail+=noise/max(peak,1e-7)*amp
 data=ridge+basin+foothills+detail
 data-=data.min();data/=max(data.max(),.0001)
 plain=np.exp(-((x-.03)/.28)**6-((y+.42)/.2)**6)
 settlement_target=.32
 data=(1-plain)*data+plain*settlement_target
 river=np.exp(-((x-.2*np.sin(4*y))/.035)**2)
 data=np.maximum(.02,data-.11*river)
 grid=Grid(np.clip(data.astype(np.float32),0,1),0,relief,width,{"seed":seed})
 height=folder/"NW_Gaea_Input_Height16.png"
 mask=folder/"NW_Gaea_ErosionProtectMask16.png"
 meta=folder/"NW_Gaea_TerrainMetadata.json"
 write_heightmap_png(grid,str(height),bits=16)
 write_mask_png((1-plain).astype(np.float32),str(mask))
 write_meta(grid,str(meta),{"generator":"NoWorld controlled continental mountain+basin+flat settlement",
   "geography_status":"QA_NONCANONICAL_ONLY","heightmap_range_normalized":[0,1]})
 dest=folder/"NW_Gaea_QA_Graph.terrain"
 b=ProjectBuilder(name="NoWorld Gaea Integration QA",width_m=width,height_m=relief)
 fid=b.add("File","Native16bitHeight",node_id=100);b.set_file(fid,height.name,relative=True)
 adj=b.add("Adjust","PreErosionHeightControl",node_id=105);b.connect(fid,adj)
 erosion=b.add("Erosion2","NaturalHydraulicErosion",node_id=110,Duration=12.0,Downcutting=.22,Seed=seed,Enable=True,Version=2)
 b.connect(adj,erosion)
 mid=b.add("File","SettlementProtectedMask",node_id=120);b.set_file(mid,mask.name,relative=True)
 b.connect(mid,erosion,dst_port="Mask")
 ex=b.add("Export","TerrainHeight16",node_id=130,Format="PNG16");b.connect(erosion,ex)
 b.set_export(ex,str(folder/"exports"/"NW_Height"),fmt="PNG16")
 tint=b.add("Tint","HighLowTint",node_id=140,RenderIntentOverride="Color")
 b.connect(erosion,tint)
 excol=b.add("Export","TerrainAlbedo",node_id=150,Format="PNG8",RenderIntentOverride="Color")
 b.connect(tint,excol);b.set_export(excol,str(folder/"exports"/"NW_Color"),fmt="PNG8")
 b.save(str(dest),resolution=n,selected=erosion,destination=str(folder/"exports"),
  notes_markdown="Gate02 owns geographic decisions. This fixture has one erosion-protected buildable settlement plain.")
 audit=b.audit()
 result={"status":"OFFLINE_GRAPH_PREPARED","project":str(dest),"height_png":str(height),
 "erosion_mask_png":str(mask),"project_sha256":sha(dest),"source_sha256":sha(height),
 "heightmap_resolution":n,"source_bit_depth":"16-bit integer PNG",
 "world_width_m":width,"relief_m":relief,"node_count":6,
 "audit":audit,"native_gaea_build":False,"native_unreal_landscape":False,
 "world_geography_canonical":False}
 receipt=folder/"NW_Gaea_GraphValidation.json"
 receipt.write_text(json.dumps(result,indent=2))
 print("NW_GAEA_GRAPH="+json.dumps({k:v for k,v in result.items() if k!="audit"}))
 return result
if __name__=="__main__":
 config=json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
 create(config)
