"""Append a new evidenced lesson to the versioned methodology (never overwrite existing IDs).
Called explicitly after new repeatable knowledge is proven or a user corrects the method.
The next gate_start/gatectl run automatically sees the new lesson. No GUI interaction.
"""
import argparse,datetime,json,pathlib,os
from mastery_preflight import ensure_master, LEDGER
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--id",required=True,help="New immutable ID K027, K028, etc.")
    p.add_argument("--topic",required=True)
    p.add_argument("--applies-to",required=True,help="all or gate01|gate09 etc")
    p.add_argument("--requirement",required=True)
    p.add_argument("--procedure",required=True)
    p.add_argument("--evidence",required=True)
    p.add_argument("--status",required=True,help="verified, partial, unverified, or known failure")
    p.add_argument("--regression",required=True,help="Specific check that will prevent reintroducing the issue")
    args=p.parse_args()
    ensure_master()
    doc=json.loads(LEDGER.read_text(encoding='utf-8-sig'))
    present={r['id'] for r in doc['rules']}
    import re
    if not re.fullmatch('K[0-9]{3}',args.id) or args.id in present:
        raise ValueError('Lesson ID must be new and immutable')
    if int(args.id[1:])!=max(int(x[1:]) for x in present)+1:
        raise ValueError('Next sequential lesson ID required')
    if len(args.evidence.strip())<15 or len(args.procedure.strip())<20 or len(args.regression.strip())<15:
        raise ValueError('Evidence, executable procedure, and regression test must be specific')
    allowed={'all'}|{'gate%02d'%i for i in range(1,13)}
    if any(x not in allowed for x in args.applies_to.lower().split('|')):
        raise ValueError('Unknown scope')
    record={'id':args.id,'topic':args.topic,'applies_to':args.applies_to.lower(),
            'requirement':args.requirement,'procedure':args.procedure,
            'evidence':args.evidence,'status':args.status,'regression_test':args.regression}
    doc['rules'].append(record)
    doc['revision']=datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H%MZ')
    temp=LEDGER.with_suffix('.json.tmp')
    if temp.exists():raise FileExistsError('Learning draft exists; review it before modifying')
    temp.write_text(json.dumps(doc,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    ensure_master(temp)
    try:
        temp.replace(LEDGER)
    except PermissionError:
        # Some Windows hosts deny os.replace over an existing JSON file.
        # Copy the already validated draft, verify its bytes and schema, then
        # remove only the temporary draft. Keep it intact if copying fails.
        import shutil
        shutil.copyfile(temp, LEDGER)
        if LEDGER.read_bytes() != temp.read_bytes():
            raise RuntimeError('Learning ledger copy differed from validated draft')
        ensure_master(LEDGER)
        temp.unlink()
    print('NW_LESSON_RECORDED='+json.dumps({'id':args.id,'revision':doc['revision'],
        'status':args.status,'regression':args.regression},ensure_ascii=False))
if __name__=='__main__':main()
