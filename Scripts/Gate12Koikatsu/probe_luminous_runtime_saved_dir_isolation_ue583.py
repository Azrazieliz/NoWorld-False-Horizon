import unreal,json,pathlib,time
w=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
r=w/"Saved/Gate12Koikatsu/LuminousV3/userdir_isolation_native_probe.json"
x={"project_dir":str(unreal.Paths.project_dir()),"project_saved_dir":str(unreal.Paths.project_saved_dir()),
   "project_user_dir":str(unreal.Paths.project_user_dir()),"status":"READ_ONLY_PATH_DISCOVERY",
   "utc":time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())}
r.write_text(json.dumps(x,indent=2)+"\n",encoding="utf8")
unreal.log("G12_USERDIR_SAVED_DIRECTORY="+str(x["project_saved_dir"]))
