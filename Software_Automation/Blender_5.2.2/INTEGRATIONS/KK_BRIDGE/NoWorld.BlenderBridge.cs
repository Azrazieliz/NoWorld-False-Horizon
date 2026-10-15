// NoWorld Blender Bridge: BepInEx 5 / Koikatsu Party automatic inbox importer.
// GPL-compatible source integration with existing ObjImport; no existing game's DLLs modified.
// Required plugins installed already: ObjImport, KKAPI, MaterialEditor.
using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;
using System.Linq;
using BepInEx;
using UnityEngine;
using KKAPI;

namespace NoWorld.Automation {
 [BepInPlugin("org.noworld.blenderbridge","NoWorld Blender Pipeline Bridge","1.0.0")]
 [BepInDependency("org.njaecha.plugins.objimport")]
 public sealed class BlenderBridge : BaseUnityPlugin {
    private string root, inbox, assets, eventsDir;
    private ObjImport.ObjImport importer;
    private float nextPoll;
    private float studioEnteredAt=-1f;
    private string last="Bridge initialized, awaiting Gate job.";
    private int completed=0, failed=0;
    private bool visible=true;
    private Rect rect=new Rect(20, 60, 440, 205);
    void Awake() {
        root=Path.Combine(Paths.BepInExRootPath,"NoWorldBridge");
        inbox=Path.Combine(root,"inbox");
        assets=Path.Combine(root,"assets");
        eventsDir=Path.Combine(root,"events");
        Directory.CreateDirectory(inbox);
        Directory.CreateDirectory(assets);
        Directory.CreateDirectory(eventsDir);
        Logger.LogMessage("NoWorld Blender bridge ready: "+root);
    }
    void Update() {
        if(Input.GetKeyDown(KeyCode.F10)) visible=!visible;
        if(Time.unscaledTime<nextPoll)return;
        nextPoll=Time.unscaledTime+0.5f;
        try { PollOnce(); }
        catch(Exception e){ last="Bridge poll failed: "+e.Message; Logger.LogError(e); }
    }
    private static string Hash(string p) {
        using(var s=File.OpenRead(p))
        using(var h=SHA256.Create())return BitConverter.ToString(h.ComputeHash(s)).Replace("-","").ToLowerInvariant();
    }
    void PollOnce() {
        if(KoikatuAPI.GetCurrentGameMode()!=GameMode.Studio) {
            studioEnteredAt=-1f;return;
        }
        // The game reports Studio mode before Studio.AddObjectItem is initialized.
        // Waiting for stable scene activation avoids premature NRE and false imports.
        if(studioEnteredAt<0f) {studioEnteredAt=Time.unscaledTime;return;}
        if(Time.unscaledTime-studioEnteredAt<45f) {
            last="Waiting for Studio scene to finish loading";
            return;
        }
        if(importer==null) importer=UnityEngine.Object.FindObjectOfType<ObjImport.ObjImport>();
        if(importer==null) { last="ObjImport unavailable; waiting for Studio plugin.";return;}
        string[] files=Directory.GetFiles(inbox,"*.ready");
        Array.Sort(files,StringComparer.Ordinal);
        foreach(string f in files.Take(2))ImportOne(f);
    }
    void ImportOne(string ready) {
        string id=Path.GetFileNameWithoutExtension(ready);
        string ext=Path.Combine(eventsDir,id+".imported");
        string err=Path.Combine(eventsDir,id+".failed");
        if(File.Exists(ext)||File.Exists(err)){File.Delete(ready);return;}
        try {
            string[] req=File.ReadAllLines(ready,Encoding.UTF8);
            if(req.Length<2)throw new IOException("Request must contain OBJ path and SHA256");
            string obj=Path.GetFullPath(req[0].Trim().Trim('"'));
            string allowed=Path.GetFullPath(assets)+Path.DirectorySeparatorChar;
            if(!obj.StartsWith(allowed,StringComparison.OrdinalIgnoreCase))
                throw new IOException("Refused out-of-spool asset path");
            if(!obj.EndsWith(".obj",StringComparison.OrdinalIgnoreCase)||!File.Exists(obj))
                throw new FileNotFoundException("No ready OBJ at "+obj);
            if(new FileInfo(obj).Length<90)throw new IOException("OBJ too small");
            string actual=Hash(obj);
            if(!actual.Equals(req[1].Trim(),StringComparison.OrdinalIgnoreCase))
                throw new IOException("OBJ hash mismatch");
            // ObjImport.LoadMesh() runs on Unity's main thread; Studio creates new OCIItem
            // if no Studio item is currently selected. Never auto-replace Maker accessories.
            importer.path=obj;
            int before=ObjImport.ObjImport.sceneRemeshedObjects.Count;
            importer.LoadMesh();
            int after=ObjImport.ObjImport.sceneRemeshedObjects.Count;
            string report="RESULT=LOAD_MESH_INVOKED\nUTC="+DateTime.UtcNow.ToString("o")+
               "\nSOURCE="+obj+"\nSHA256="+actual+"\nOBJECT_REPLACEMENTS_BEFORE="+before+
               "\nOBJECT_REPLACEMENTS_AFTER="+after+
               "\nNOTE=Importer method returns void; inspect scene/log for final art confirmation\n";
            File.WriteAllText(ext,report,Encoding.UTF8);
            File.Delete(ready);completed++;last="Imported OBJ request "+id+" (Studio)";
            Logger.LogMessage(last);
        }catch(Exception e){
            File.WriteAllText(err,DateTime.UtcNow.ToString("o")+"\n"+e,Encoding.UTF8);
            File.Delete(ready);failed++;
            last="OBJ import FAILED "+id+": "+e.Message;
            Logger.LogError(e);
        }
    }
    void OnGUI() {
        if(visible)rect=GUI.Window(492031,rect,Window,"NoWorld • Blender/KKoikatsu Bridge");
    }
    void Window(int id) {
        GUILayout.Label("Automatic OBJ import only when Koikatsu Studio is active");
        GUILayout.Label("Jobs: "+completed+" processed, "+failed+" errors");
        GUILayout.Label("Inbox: "+inbox);
        GUILayout.Label(last);
        if(GUILayout.Button("Poll pending imported Blender OBJ now")) { nextPoll=0;PollOnce(); }
        if(GUILayout.Button("Close")) visible=false;
        GUI.DragWindow();
    }
 }
}
