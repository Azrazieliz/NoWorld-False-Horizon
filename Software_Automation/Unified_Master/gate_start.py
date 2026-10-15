"""Fast startup context for Gates 01-12. Read-only; emits learned routes and pitfalls."""
import argparse,json,pathlib
from mastery_preflight import ensure_master, LEDGER
def gate_start(name):
    if name.isdigit():name="Gate%02d"%int(name)
    methods=ensure_master()
    ledger=json.loads(LEDGER.read_text(encoding="utf-8-sig"))
    if name not in ledger['gates']:raise ValueError("Unknown Gate: "+name)
    code=name.lower()
    selected=[r for r in ledger['rules'] if "all" in r['applies_to'].split('|') or code in r['applies_to'].split('|')]
    return {"status":"LEARNED_METHODS_LOADED","gate":name,"purpose":ledger['mission'],
            "gate_responsibility":ledger['gates'][name],
            "master_sha256":methods["knowledge_sha256"],"revision":methods["revision"],
            "startup":ledger["startup_contract"]["mandatory"],
            "known_methods":[{"id":r["id"],"topic":r["topic"],"status":r["status"],
                "requirement":r["requirement"],"procedure":r["procedure"]} for r in selected],
            "methodology_preflight":"PASS","runtime_visual_acceptance":False}
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--gate",required=True,help="1..12 or Gate01..Gate12")
    p.add_argument("--compact",action="store_true")
    a=p.parse_args()
    result=gate_start(a.gate)
    if a.compact:
        print("GATE_MASTER_READY=%s revision=%s rules=%s" %
              (result['gate'],result['revision'],','.join(i['id'] for i in result['known_methods'])))
    else:print("GATE_MASTER_CONTEXT="+json.dumps(result,ensure_ascii=False))
if __name__=="__main__":main()
