import pathlib
root=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\BepInEx\SM01_SANDBOX\recipes')
files=sorted(root.glob('T06_morph*.tsv'),key=lambda p:p.stat().st_mtime,reverse=True)
if not files:raise FileNotFoundError('T06 original native morph list')
rows=[]
for line in files[0].read_text(encoding='utf-8').splitlines():
 t=line.split('\t');rows.append([t[0],int(t[1]),float(t[2])])
fix={'face':{14:.38,15:.38,19:.48,20:.51,24:.47,25:.47,26:.51,27:.49,28:.51,29:.49,30:.53,31:.51,32:.50,33:.47,34:.72,35:.68,41:.49,42:.48,44:.47,45:.51,46:.51},'body':{1:.52,2:1.,3:.63,5:.52,8:.72,9:.64,10:.73,17:.44,19:.46,21:.45,25:.38,27:.42,29:.70,31:.45,38:.74,39:.71}}
for g,i,v in rows:
 if i in fix.get(g,{}):
  # native writes use distinct array object, processed below
  pass
rows2=[[g,i,fix.get(g,{}).get(i,v)] for g,i,v in rows]
dest=root/'T06_FACE_BODY_REFIT_final.tsv'
dest.write_text(''.join(f'{g}\t{i}\t{v:.6f}\n' for g,i,v in rows2),encoding='utf-8')
print('FACIAL_APPEARANCE_REFIT',dest.name,'controls',sum(len(v) for v in fix.values()))
