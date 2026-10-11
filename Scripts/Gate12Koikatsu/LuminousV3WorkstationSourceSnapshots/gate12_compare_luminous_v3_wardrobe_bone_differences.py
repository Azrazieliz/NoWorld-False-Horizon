import pathlib,struct,json,collections,math
base=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\BLENDER_FRESH")
data={}
for st in ("CLOTHED","UNDERWEAR","NUDE"):
 p=base/st/f"LUMINOUS_V3_{st}_skinned.glb"
 raw=p.read_bytes()
 n=struct.unpack_from("<I",raw,12)[0]
 j=json.loads(raw[20:20+n])
 nodes=j["nodes"]
 parents={}
 for i,x in enumerate(nodes):
  for ch in x.get("children",[]):parents[ch]=i
 skin=j["skins"][0]
 rows={}
 duplicates=[]
 for idx in skin["joints"]:
  node=nodes[idx];name=node.get("name",f"index{idx}")
  if name in rows:duplicates.append(name)
  par=parents.get(idx)
  rows[name]={"parent":nodes[par].get("name") if par is not None else None,
              "translation":node.get("translation",(0,0,0)),
              "rotation":node.get("rotation",(0,0,0,1)),
              "scale":node.get("scale",(1,1,1))}
 data[st]={"rows":rows,"dup":duplicates}
for st in ("UNDERWEAR","NUDE"):
 a=data["CLOTHED"]["rows"];b=data[st]["rows"]
 common=set(a)&set(b);added=set(b)-set(a);removed=set(a)-set(b)
 pd=[];td=[];rd=[];sd=[]
 for name in sorted(common):
  u=a[name];v=b[name]
  if u["parent"]!=v["parent"]:pd.append((name,u["parent"],v["parent"]))
  t=max(abs(float(x)-float(y)) for x,y in zip(u["translation"],v["translation"]))
  q=max(abs(float(x)-float(y)) for x,y in zip(u["rotation"],v["rotation"]))
  s=max(abs(float(x)-float(y)) for x,y in zip(u["scale"],v["scale"]))
  if t>1e-6:td.append((name,t))
  if q>1e-6:rd.append((name,q))
  if s>1e-6:sd.append((name,s))
 print(st,"COMMON",len(common),"ADDED",len(added),"REMOVED",len(removed),
       "DUPES",len(data[st]["dup"]),
       "PARENT",len(pd),"TRANSL",len(td),"ROT",len(rd),"SCALE",len(sd))
 print("ADDED",sorted(added)[:20],"REMOVED",sorted(removed)[:20])
 print("PARENT_DIFF",pd[:15])
 print("TRANSL_TOP",sorted(td,key=lambda a:-a[1])[:18])
 print("ROT_TOP",sorted(rd,key=lambda a:-a[1])[:15])
 print("SCALE_TOP",sorted(sd,key=lambda a:-a[1])[:8])
