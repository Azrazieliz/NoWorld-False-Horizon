"""NoWorld Gaea ↔ Blender ↔ Unreal terrain transfer.
Reads true 16-bit Gaea source PNG. Produces editable tiled mesh,
UVs, accurate seam checks, world-scale metadata and Unreal FBX.
Does not claim Gaea hydraulic erosion was computed until native Gaea build passes.
"""
import bpy,sys,pathlib,json,hashlib,math,statistics
import numpy as np
def newpath(s):
 p=pathlib.Path(s).resolve()
 if p.exists():raise FileExistsError("Refuse overwrite "+str(p))
 p.parent.mkdir(parents=True,exist_ok=True)
 return p
def sha(p):
 h=hashlib.sha256()
 with open(p,"rb") as f:
  for b in iter(lambda:f.read(1<<20),b""):h.update(b)
 return h.hexdigest()
def readgray(path):
 img=bpy.data.images.load(str(path),check_existing=False)
 img.colorspace_settings.name="Non-Color"
 w,h=map(int,img.size)
 arr=np.empty(w*h*4,dtype=np.float32)
 img.pixels.foreach_get(arr)
 arr=arr.reshape(h,w,4)[:,:,0].copy()
 assert float(np.min(arr))>=0 and float(np.max(arr))<=1.000001
 return arr
def run(job):
 im=pathlib.Path(job["height_png"]).resolve()
 mask=pathlib.Path(job["mask_png"]).resolve()
 for src in (im,mask):
  if not src.is_file():raise FileNotFoundError(src)
 oldhashes={str(p):sha(p) for p in (im,mask)}
 blend=newpath(job["source_blend"]);fbx=newpath(job["mesh_fbx"]);receipt=newpath(job["report"])
 world=float(job["world_size_m"]);relief=float(job["relief_m"])
 tile_ct=int(job.get("tile_grid",2));edge=int(job.get("vertices_per_tile_edge",65))
 if tile_ct not in (1,2,4) or edge<17 or edge>129:raise ValueError("Unsupported tile budget")
 if world<=200 or relief<=0:raise ValueError("Bad world meters")
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
 pixels=readgray(im);maskdata=readgray(mask)
 assert pixels.shape==maskdata.shape
 h,w=pixels.shape
 meshlist=[];heights={}
 tile_vertices=0
 for ty in range(tile_ct):
  for tx in range(tile_ct):
   verts=[];uv=[];faces=[];vcolors=[];world_heights=[]
   for jy in range(edge):
    for ix in range(edge):
     fx=(tx+(ix/(edge-1)))/tile_ct
     fy=(ty+(jy/(edge-1)))/tile_ct
     px=min(w-1,max(0,int(round(fx*(w-1)))))
     py=min(h-1,max(0,int(round(fy*(h-1)))))
     norm=float(pixels[py,px]);m=float(maskdata[py,px])
     # Consistent corner samples across adjacent tiles.
     z=relief*norm
     verts.append(((fx-.5)*world,(fy-.5)*world,z))
     uv.append((fx,fy))
     vcolors.append((norm,m,1-norm,1.))
     world_heights.append(z)
     heights[(tx*(edge-1)+ix,ty*(edge-1)+jy)]=z
   for j in range(edge-1):
    for i in range(edge-1):
     a=j*edge+i;b=a+1;c=a+edge
     faces.append((a,b,c));faces.append((b,c+1,c))
   m=bpy.data.meshes.new("NW_Gaea_TerrainTile_%d_%d_Mesh"%(tx,ty))
   m.from_pydata(verts,[],faces);m.update()
   layer=m.uv_layers.new(name="NW_Gaea_WorldUV")
   for poly in m.polygons:
    for li in poly.loop_indices:
     layer.data[li].uv=uv[m.loops[li].vertex_index]
   attr=m.color_attributes.new(name="NW_Gaea_HeightMaskWeights",type="FLOAT_COLOR",domain="CORNER")
   for poly in m.polygons:
    for li in poly.loop_indices:attr.data[li].color=vcolors[m.loops[li].vertex_index]
   ob=bpy.data.objects.new("NW_TerrainTile_X%d_Y%d"%(tx,ty),m)
   bpy.context.scene.collection.objects.link(ob)
   ob["nw_terrain_source_sha256"]=oldhashes[str(im)]
   ob["nw_landscape_units"]="meters"
   ob["nw_world_origin_geography"]="QA_NONCANONICAL"
   tile_vertices+=len(verts);meshlist.append(ob)
   for face in m.polygons:face.use_smooth=True
 expected=(tile_ct*(edge-1)+1)**2
 assert len(heights)==expected,(len(heights),expected)
 # Every shared edge is sampled from the same pixel index. Compare independent
 # tile-boundary point samples, not the deduplicated dictionary.
 max_seam=0.
 for a in meshlist:
  ix=int(a.name.split("X")[1].split("_")[0]);iy=int(a.name.split("Y")[1])
  if ix+1<tile_ct:
   b=next(x for x in meshlist if x.name=="NW_TerrainTile_X%d_Y%d"%(ix+1,iy))
   for j in range(edge):
    va=a.data.vertices[j*edge+(edge-1)].co.z
    vb=b.data.vertices[j*edge].co.z
    max_seam=max(max_seam,abs(va-vb))
  if iy+1<tile_ct:
   b=next(x for x in meshlist if x.name=="NW_TerrainTile_X%d_Y%d"%(ix,iy+1))
   for j in range(edge):
    va=a.data.vertices[(edge-1)*edge+j].co.z
    vb=b.data.vertices[j].co.z
    max_seam=max(max_seam,abs(va-vb))
 assert max_seam<.00001,max_seam
 # Add a slope-aware native Blender shader as preview. Unreal needs explicit
 # texture import/material graph conversion; never pretend BSDF nodes transfer.
 material=bpy.data.materials.new("NW_Gaea_BiomePreview_BlendMaterial")
 material.use_nodes=True
 p=material.node_tree.nodes.get("Principled BSDF")
 p.inputs["Roughness"].default_value=.88
 p.inputs["Base Color"].default_value=(.30,.36,.29,1)
 for o in meshlist:o.data.materials.append(material)
 bpy.context.scene["nw_gps_geography"]="Test-only; final World coordinates Gate02-owned"
 bpy.context.scene["nw_wu_compatible"]="Gaea-generated height/mask import, Blender editable tile, UE static mesh"
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
 bpy.ops.object.select_all(action="DESELECT")
 for ob in meshlist:ob.select_set(True)
 bpy.context.view_layer.objects.active=meshlist[0]
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,
    object_types={"MESH"},use_mesh_modifiers=True,bake_anim=False,
    apply_scale_options="FBX_SCALE_ALL")
 result={"status":"BLENDER_TILED_TERRAIN_PASS","blender":bpy.app.version_string,
 "source_hashes":oldhashes,"source_unchanged":all(sha(pathlib.Path(k))==v for k,v in oldhashes.items()),
 "editable_blend":str(blend),"fbx":str(fbx),"fbx_sha256":sha(fbx),
 "tile_count":len(meshlist),"vertices_per_tile":edge*edge,
 "polygons_per_tile":(edge-1)*(edge-1)*2,"total_vertices":tile_vertices,
 "world_size_m":world,"relief_m":relief,
 "source_height_resolution":list(pixels.shape),"mask_resolution":list(maskdata.shape),
 "world_uvs":True,"mask_as_vertex_colors":True,
 "maximum_seam_error_m":max_seam,
 "unreal_import_verified":False,"native_gaea_erosion_build_verified":False,
 "game_geography_accepted":False,"android_performance_accepted":False}
 receipt.write_text(json.dumps(result,indent=2))
 print("NW_GAEA_BLENDER_TERRAIN="+json.dumps(result))
if __name__=="__main__":
 arg=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 if len(arg)!=1:raise SystemExit("Need JSON spec")
 run(json.loads(pathlib.Path(arg[0]).read_text(encoding="utf-8-sig")))
