"""Build a self-contained, searchable 5.2.2 mastery manual from exact executable registry.
No external JS/CDN, no assets, no external dependencies.
"""
from pathlib import Path
import json,html
root=Path(__file__).parent
data=json.loads((root/"WORKFLOWS.json").read_text(encoding="utf-8"))
rows=data["entries"]
group_names=sorted({x["domain"] for x in rows})
items=[]
for rec in rows:
 def esc(key):return html.escape(str(rec[key]),quote=True)
 items.append('''<article class="item" data-search="{search}" data-domain="{domain}" data-status="{status}">
<header><span class="tag">{domain}</span><h3>{name}</h3><small>{status}</small></header>
<div class="body"><h4>Exact Blender UI route</h4><p class="route">{ui}</p>
<h4>Production-efficient method</h4><p>{method}</p><h4>Automatable native route</h4>
<code>{api}</code><p class="src"><a target="_blank" href="{url}">Primary documentation / source →</a></p></div>
</article>'''.format(**{key:esc(key) for key in ("domain","status","name","ui","method","api","url")},
 search=html.escape((rec["name"]+" "+rec["domain"]+" "+rec["method"]+" "+rec["api"]).lower(),quote=True)))
dom=''.join('<button class="filter" data-filter="'+html.escape(x,quote=True)+'">'+html.escape(x)+'</button>' for x in group_names)
page='''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NoWorld | Blender 5.2.2 Software Mastery</title>
<style>
:root{color-scheme:dark;--bg:#101419;--line:#334351;--panel:#1b222b;--ink:#edf1f5;--muted:#b9cad6;--accent:#75c4ff}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.55 system-ui,-apple-system,Segoe UI,Arial}
main{max-width:1340px;margin:auto;padding:28px 22px 60px}h1{font-size:32px;letter-spacing:-.02em;margin:12px 0}h2{font-size:23px;margin-top:36px}
p{max-width:1040px}a{color:var(--accent);text-decoration:none}a:hover{text-decoration:underline}
strong{color:#fff}.tag{font-size:11px;font-weight:700;letter-spacing:.04em;text-transform:uppercase;border:1px solid var(--line);padding:3px 6px;border-radius:6px;color:var(--muted)}
.hero{border-bottom:1px solid var(--line);padding-bottom:18px}
.status{font-size:13px;color:#ffd59e;background:#392c1f;padding:7px 10px;display:inline-block;border-radius:6px}
.controls{display:flex;gap:10px;flex-wrap:wrap;margin:24px 0 12px}
input{border:1px solid var(--line);border-radius:7px;background:#232c34;color:#fff;padding:10px 13px;min-width:300px;flex:1;font-size:15px}
button{border:1px solid var(--line);background:#253340;color:#fff;padding:9px 11px;border-radius:7px;cursor:pointer}
button.active{background:#11618c;border-color:#64b7e6}button:hover{border-color:#b2dafa}
.cards{display:grid;grid-template-columns:repeat(auto-fill,minmax(370px,1fr));gap:14px}
.item{border:1px solid var(--line);background:var(--panel);border-radius:12px;overflow:hidden}
.item header{padding:15px 17px 12px;border-bottom:1px solid var(--line)}
.item header h3{margin:7px 0;font-size:18px;line-height:1.25}.item header small{color:var(--muted)}
.body{padding:2px 17px 15px}.body h4{font-size:12px;color:#a9c6d8;margin:14px 0 3px;text-transform:uppercase;letter-spacing:.05em}
.body p{margin:2px 0 10px}.body code{display:block;border-radius:7px;background:#111820;padding:10px;white-space:pre-wrap;overflow-wrap:anywhere;color:#aee4c9;font-size:12px}
.route{font-weight:600}.src{font-size:13px}
.notes{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:10px}.notes section{border:1px solid var(--line);padding:10px 15px;border-radius:8px}
.meta{font-size:13px;color:var(--muted)}.hidden{display:none!important}footer{margin-top:40px;border-top:1px solid var(--line);padding:20px;color:var(--muted)}
</style></head><body><main>
<section class="hero"><span class="tag">BLENDER 5.2.2 LTS • WORKING PC</span>
<h1>NoWorld: software mastery & automation control</h1>
<p>This is the <strong>software operation handbook</strong>—not a completed game asset library. It documents real UI paths, high-leverage automation methods, native controls and their current validation states. Use the Blender <strong>N</strong> sidebar → <strong>NoWorld</strong> for the matching live control panel.</p>
<div class="status">Professional art demonstrations accepted: 0 / 25. Software automation is not artistic sign-off.</div>
<p class="meta">Designed for 8 GB RAM / i3-1115G4 with Blender 5.2.2, Unreal 5.8.3, Gaea 2.3.1, and Koikatsu input. All production asset generation requires a distinct commission.</p></section>
<h2>Fastest reliable automation principles</h2>
<div class="notes">
<section><h3>1. Work from high-quality sources</h3><p>Use licensed mocap, scanned PBR, curated shapes, dedicated add-ons and asset libraries; automate import and adaptation rather than drawing every key or rebuilding each character.</p></section>
<section><h3>2. Maintain source parameter systems</h3><p>Use modifier stack, Geometry Nodes group interfaces, material mask node groups, reusable corrective shape keys and Asset Browser catalogs. No destructive edits to canonical source.</p></section>
<section><h3>3. Let the right engine do the job</h3><p>Blender authors and bakes; Unreal does Control Rig, retarget, gameplay notifies, Niagara and Android quality. Gaea provides eroded terrain; Koikatsu preserves identity and original face assets.</p></section>
<section><h3>4. Remove meaningless manual clicking</h3><p>Native bpy/RNA, background batch, reusable CLI manifests, named controls and validated add-ons take precedence over fragile pixel-coordinate clicks.</p></section>
<section><h3>5. Check final fidelity</h3><p>Automated topology/action checks are necessary but cannot evaluate expressive performance, refined design, toon shaders or native game playback. Artistic comparison remains a separate gate.</p></section>
<section><h3>6. Safe by default</h3><p>The installed hub is read-only. It doesn't create meshes, play animations into canonical files, retarget into Gate 12 or overwrite frozen project assets.</p></section>
</div>
<h2>Workflow finder</h2>
<div class="controls"><input id="search" type="search" placeholder="Search control, command, software, method…"><button class="filter active" data-filter="ALL">All</button>__DOMAINS__</div>
<p id="result" class="meta"></p>
<div class="cards">__CARDS__</div>
<h2>Known limitations</h2><p>Installed native Blender, CloudRig and Retarget extensions do not automatically guarantee high-end rigging. Experimental cloth/hair physics are not equivalent to Unreal or KK runtime. CMU mocap retarget to the Koikatsu sample failed visual pose inspection even though native UE assets were generated. Gaea 2.3.1 Swarm failed in the remote process console. Mastery is not certified until those workflows become reproducible, context-correct and artistically accepted.</p>
<h2>Operational links</h2><p><a href="https://docs.blender.org/manual/en/5.2/">Version-matched Blender 5.2 manual</a> · <a href="https://docs.blender.org/api/5.2/">Blender 5.2 Python API</a> · <a href="https://studio.blender.org/training/">Blender Studio training</a> · <a href="https://developer.blender.org/docs/release_notes/5.2/">5.2 exact release notes</a> · <a href="https://dev.epicgames.com/documentation/en-us/unreal-engine/">Unreal 5.8 docs</a> · <a href="https://docs.gaea.app/">Gaea manual</a>.</p>
<footer>Generated locally from WORKFLOWS.json. No videos, models, source characters, paid content or binary add-ons redistributed.</footer>
</main><script>
const cards=[...document.querySelectorAll('.item')];const search=document.querySelector('#search');
let current='ALL';const result=document.querySelector('#result');
function refresh(){const q=search.value.trim().toLowerCase();let n=0;for(const c of cards){const ok=(current==='ALL'||c.dataset.domain===current)&&c.dataset.search.includes(q);c.classList.toggle('hidden',!ok);if(ok)n++}result.textContent=n+' workflows shown / '+cards.length+' total';}
for(const button of document.querySelectorAll('.filter'))button.addEventListener('click',()=>{current=button.dataset.filter;for(const x of document.querySelectorAll('.filter'))x.classList.toggle('active',x===button);refresh()});
search.addEventListener('input',refresh);refresh();
</script></body></html>'''.replace('__DOMAINS__',dom).replace('__CARDS__',"\n".join(items))
out=root/"MASTERY_PLAYBOOK.html"
out.write_text(page,encoding="utf-8")
print("NW_MASTERY_BOOK",len(rows),"WORKFLOWS",len(page),"HTML_BYTES",out.stat().st_size)
