"""Download immutable CC0 1K PBR map triplets from Poly Haven, verify authoritative API MD5.
No bypass of Windows script execution policies; Python 3.11 standard library only.
"""
from pathlib import Path
import urllib.request,json,hashlib,time
root=Path(__file__).with_name("PolyHaven_CC0_1k")
root.mkdir(parents=True,exist_ok=True)
ua={"User-Agent":"NoWorld-BlenderMastery-AssetAudit/0.5"}
def data(url):
 req=urllib.request.Request(url,headers=ua)
 with urllib.request.urlopen(req,timeout=45) as r:return r.read()
rows=[]
for asset in ["granite_wall","rough_block_wall","stony_dirt_path"]:
 meta=json.loads(data("https://api.polyhaven.com/files/"+asset))
 for key in ("Diffuse","Rough","nor_gl"):
  record=meta[key]["1k"].get("jpg") or meta[key]["1k"].get("png")
  assert record, (asset,key)
  ext=record["url"].rsplit(".",1)[-1]
  p=root/f"{asset}_{key}.{ext}"
  if not p.exists() or p.stat().st_size!=record["size"]:
   p.write_bytes(data(record["url"]))
  digest=hashlib.md5(p.read_bytes()).hexdigest()
  assert digest.lower()==record["md5"].lower(),(p,digest,record["md5"])
  rows.append(dict(asset=asset,map=key,file=p.name,bytes=p.stat().st_size,
      md5=digest,source_url=record["url"],license="CC0",credit="Poly Haven"))
  print("PBR_VERIFIED",asset,key,p.stat().st_size,flush=True)
(root/"SOURCE_PROVENANCE.json").write_text(json.dumps({
 "source":"https://polyhaven.com","source_api":"https://api.polyhaven.com",
 "CC0":True,"attribution_required_by_API_user_agent_not_asset":True,"maps":rows},indent=2),encoding="utf-8")
print("PBR_ALL_VERIFIED",len(rows))
