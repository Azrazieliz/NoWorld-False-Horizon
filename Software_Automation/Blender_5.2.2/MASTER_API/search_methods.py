"""Gate 01/02/09/12 searchable *installed* Blender 5.2.2 API and workflow atlas.
Pure Python 3.11, no Blender startup. Returns exact RNA properties, enums and source docs.
Usage: python search_methods.py --query "arch gothic castle" --limit 5
"""
import pathlib,json,argparse,re,math,collections,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
CAT=ROOT/"MASTER_API"/"BLENDER_5_2_RNA_CATALOG.json"
FLOW=ROOT/"ADDONS"/"noworld_mastery_hub"/"WORKFLOWS.json"
ALIASES={
 "castle":["arch","architecture","pillar","capital","wall","bevel","array"],
 "house":["architecture","mesh","extrude","wall","roof","beam"],
 "terrain":["displace","mesh","volume","height","geonodes"],
 "morph":["shape","key","sculpt","lattice"],
 "wing":["rig","armature","shape","mesh","constraint","animation"],
 "flight":["animation","rig","action","armature","keyframe"],
 "swim":["animation","rig","action","armature","keyframe"],
 "dance":["animation","rig","action","retarget","nla"],
 "cloth":["cloth","physics","solver","sim","geometry"],
 "hair":["curves","geometry","material","groom"],
 "blend":["mix","shader","nodes","material"],
 "kk":["koikatsu","obj","export","shader","materials"],
 "vfx":["particle","geometry","material","effects","niagara"],
 "unreal":["fbx","export","skeleton","action","animation"],
 "uv":["uv","unwrap","texture"],
}
def tokens(text):
 return {t for t in re.findall(r"[a-z0-9_]+",text.lower()) if len(t)>1}
def rank(query,text,title=""):
 q=tokens(query)
 for t in list(q):
  q.update(ALIASES.get(t,[]))
 body=tokens(text);ttl=tokens(title)
 if not q:return 0
 common=q&body;title_common=q&ttl
 if not common and not title_common:return 0
 return sum(9 if x in ttl else 3 for x in common)+len(title_common)*6+8*int(query.lower() in text.lower())
def lookup(term,limit):
 cat=json.loads(CAT.read_text(encoding="utf-8"))
 workflows=json.loads(FLOW.read_text(encoding="utf-8"))
 scored=[]
 for x in workflows["entries"]:
  doc=" ".join(str(x.get(k,"")) for k in ("id","name","domain","method","ui","api","url"))
  score=rank(term,doc,x["name"]+" "+x["id"])
  if score:scored.append((score,{"kind":"workflow","id":x["id"],"domain":x["domain"],
    "status":x["status"],"name":x["name"],"ui":x.get("ui"),
    "api":x.get("api"),"method":x.get("method"),"source":x.get("url")}))
 for x in cat["operators"]:
  doc=x["id"]+" "+x["label"]+" "+x["description"]+" "+" ".join(p["name"]+" "+p["identifier"] for p in x["props"])
  score=rank(term,doc,x["id"]+" "+x["label"])
  if score:scored.append((score,{"kind":"operator","id":x["id"],
    "description":x["description"],"call":"bpy.ops."+x["id"],
    "parameters":x["props"][:28]}))
 for name,td in cat["types"].items():
  doc=name+" "+" ".join(p["identifier"]+" "+p["description"] for p in td.get("properties",[]))
  score=rank(term,doc,name)
  if score:scored.append((score,{"kind":"RNA_type","id":name,
    "access":"bpy.types."+name,"properties":td.get("properties",[])[:30]}))
 scored.sort(key=lambda a:(-a[0],a[1]["id"]))
 return {"blender":cat["blender"],"query":term,
  "total_indexed_operators":len(cat["operators"]),
  "results":[dict({"rank":s},**r) for s,r in scored[:limit]],
  "note":"API lookup, not artwork-quality or execution acceptance"}
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--query",required=True)
 p.add_argument("--limit",type=int,default=8)
 a=p.parse_args()
 print(json.dumps(lookup(a.query,min(max(a.limit,1),30)),indent=2,ensure_ascii=False))
if __name__=="__main__":main()
