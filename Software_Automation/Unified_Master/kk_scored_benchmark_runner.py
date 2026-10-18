#!/usr/bin/env python3
"""ONLY approved Koikatsu Test07+ benchmark entry point.

Requires local full-resolution user reference, source-bound explicitly grounded
ROIs, relevant visible subject mask when background is complex, and reversible
candidate plan. Mutations are authorized only DURING original per-feature native
scoring. Direct testNN_*.py scripts cannot edit Maker.

This enforces the pipeline; it cannot invent artistic likeness. No automatic card.
"""
from __future__ import annotations
import argparse,json,re
import kk_test08_gate_dispatch as gate

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--benchmark",required=True,help="TEST09, TEST10, etc.")
 p.add_argument("--reference",required=True,help="actual source bytes on Windows PC")
 p.add_argument("--annotations",required=True,help="verified feature ROIs bound by original SHA256")
 p.add_argument("--plan",required=True,help="reversible native candidate plan, same original reference")
 p.add_argument("--out-dir",required=True,help="new evidence directory, never overwritten")
 p.add_argument("--mask",help="full-resolution subject mask, required for nonwhite scene")
 args=p.parse_args()
 if not re.fullmatch(r"TEST(?:0[7-9]|[1-9][0-9]+)",args.benchmark):
  p.error("Only TEST07+ is admitted here. Prior validated Test06 is preserved.")
 gate.status()
 # gate.run validates source, digest, ROIs, camera, mask and candidate plan
 # before starting actual game operations; uses the original optimizer's
 # reversal/ranking contract with corrected subject-registered feature ROIs.
 result=gate.run(args.reference,args.annotations,args.plan,args.out_dir,
                 args.mask,benchmark_id=args.benchmark)
 print("GATED_BENCHMARK_RECEIPT="+json.dumps(result,sort_keys=True))
 return 0 if result["status"]=="REVIEW_NATIVE_IMAGES" else 2
if __name__=="__main__":
 raise SystemExit(main())
