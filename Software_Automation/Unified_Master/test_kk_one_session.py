"""Pure offline regression of one-launch asset plan and native dispatch.

All tests use temporary SQLite and injected runtime submit. No game launch,
mod copying or character manipulation occurs.
"""
import json,pathlib,sqlite3,tempfile
import kk_one_session as k
def check(t,description):
 if not t:raise AssertionError(description)
def reject(fn):
 try:fn()
 except (RuntimeError,ValueError,FileExistsError,FileNotFoundError):return True
 return False
checks={}
def test(name,fn):
 try:fn();checks[name]="PASS"
 except Exception as e:checks[name]="FAIL "+str(e)
with tempfile.TemporaryDirectory() as td:
 root=pathlib.Path(td);db=root/"catalog.sqlite"
 con=sqlite3.connect(db)
 con.executescript("""
 CREATE TABLE packages(id INTEGER PRIMARY KEY,guid TEXT,relative_path TEXT,name TEXT,
   size_bytes INTEGER,asset_count INTEGER);
 CREATE TABLE assets(id INTEGER PRIMARY KEY,package_id INTEGER,category_no TEXT,
   original_id TEXT,display_name TEXT);
 INSERT INTO packages VALUES(1,'mod.military','outfit/officer.zipmod','Officer Armor',100,4);
 INSERT INTO packages VALUES(2,'mod.cloak','accessories/burgundy-cape.zipmod','Red Cape',200,3);
 INSERT INTO packages VALUES(3,'mod.hair','hair/silver-01.zipmod','Long Silver Hair',300,1);
 INSERT INTO assets VALUES(1,1,'105','32','Military Officer Jacket');
 INSERT INTO assets VALUES(2,2,'124','33','Burgundy Cape');
 INSERT INTO assets VALUES(3,3,'111','44','Silver Hair');
 """)
 con.commit();con.close()
 test("readonly_catalog_features",lambda:
   check(k.catalog_plan(["military","cape","silver"],[],db=db)["package_count"]==3,"Wrong pool"))
 test("pinned_packages_included",lambda:
   check(k.catalog_plan([],["mod.cloak"],db=db)["package_count"]==1,"Pinned package missed"))
 test("unknown_mandatory_fails_closed",lambda:
   check(k.catalog_plan([],["not.installed"],db=db)["status"]=="UNRESOLVED_REQUIRED_GUIDS","Missing package incorrectly marked ready"))
 test("budget_overshoot_fails_before_launch",lambda:
   check(reject(lambda:k.catalog_plan(["military","cape"],[],max_bytes=2,db=db)),"Exceeded pool budget"))
 test("duplicate_adjacent_calls_elided",lambda:
   check(len(k.validate_recipe([{"op":"set_eye_iris","args":{"value":.4}},
      {"op":"set_eye_iris","args":{"value":.4}},{"op":"render_views","args":{}}]))==2,"No-op duplication"))
 test("save_not_allowed_without_explicit",lambda:
   check(reject(lambda:k.validate_recipe([{"op":"save_derived_card"}])),"Unexpected save authorization"))
 test("dangerous_undo_op_forbidden",lambda:
   check(reject(lambda:k.validate_recipe([{"op":"undo_last_batch"}],allow_save=True)),"Undo not blocked"))
 test("invalid_native_argument_forbidden",lambda:
   check(reject(lambda:k.validate_recipe([{"op":"set_hair","args":{"fake_param":23}}])),"Unknown field accepted"))
 calls=[]
 def fake_submit(data,timeout):
  calls.append((data,timeout))
  return 0
 path=root/"telemetry.json"
 def run():
  receipt=k.run_batch([{"op":"set_eye_iris","args":{"value":.76,"v2":.6,"v3":.2}},
     {"op":"render_views"}],path,submit=fake_submit,guard=False)
  check(receipt["ops_dispatched"]==2 and receipt["app_restarts"]==0,"Native ops not issued")
  check(calls[0][0]["value2"]==.6 and calls[0][0]["value3"]==.2,"Bad native args")
  check(path.exists() and receipt["status"]=="PASS_NATIVE_OPS","Telemetry not persisted")
 test("one_process_native_batch_with_telemetry",run)
 test("duplicate_receipt_no_overwrite",lambda:
   check(reject(lambda:k.run_batch([{"op":"render_views"}],path,submit=fake_submit,guard=False)),"Receipt overwritten"))
 def morph_batch():
  old_runtime=k.RUNTIME
  try:
   k.RUNTIME=root
   output=root/"morphs_receipt.json"
   specs=[{"group":"face","index":8,"value":.7},{"group":"body","index":2,"value":1.0}]
   result=k.run_batch([{"op":"apply_morph_batch","args":{"morphs":specs}}],output,submit=fake_submit,guard=False)
   data=calls[-1][0]
   content=(root/"recipes"/data["recipeFile"]).read_text(encoding="utf-8")
   check(result["status"]=="PASS_NATIVE_OPS" and content=="face\t8\t0.7\nbody\t2\t1\n","Morph TSV incorrect: "+repr(content))
  finally:k.RUNTIME=old_runtime
 test("morph_batch_native_tsv",morph_batch)
 test("source_path_traversal_denied",lambda:
   check(reject(lambda:k.safe_rel("../user/source.zipmod")),"Unsafe source path"))
 test("no_game_process_touch_in_tests",lambda:
   check(len(calls)==3 and all(x[0]["nonce"].startswith("NW1_") for x in calls),"Incorrect mocked queue"))
print("KK_ONE_SESSION_REGRESSIONS="+json.dumps({
 "checks":checks,"passed":sum(x=="PASS" for x in checks.values()),"total":len(checks),
 "game_launched":False,"native_character_modified":False},sort_keys=True))
if not all(x=="PASS" for x in checks.values()):raise SystemExit(2)
