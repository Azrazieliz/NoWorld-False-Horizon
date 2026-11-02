using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using BepInEx;
using KKAPI.Maker;
using UnityEngine;

// NoWorld 5-star: queued ORIGINAL .png card -> genuine native KKBP PMX export.
// Uses installed Koikatsu Maker + KKBP: no simulated geometry, thumbnails or
// user-interface clicks. Keeps original cards and external plugin DLLs intact.
[BepInPlugin("noworld.contentfactory.kk.card_queue", "NoWorld Original Card API Export Queue", "1.0.0")]
public sealed class NoworldCardExportQueue : BaseUnityPlugin
{
    static readonly string Queue = @"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Saved\NoWorldFactory\koikatsu_queue";
    static readonly string GameExports = @"D:\Tools\Steam\steamapps\common\Koikatsu Party\Export_PMX";
    static readonly string AllowedProject = @"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Assets\NoWorld";
    static readonly string AllowedGameCards = @"D:\Tools\Steam\steamapps\common\Koikatsu Party\UserData\chara";
    bool entered, working, completed;
    float nextPoll;
    static string Hash(string file)
    {
        using(var stream=File.OpenRead(file))
        using(var sha=SHA256.Create())
            return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-","").ToLowerInvariant();
    }
    static string Json(string s) { return s.Replace("\\","\\\\").Replace("\"","\\\"").Replace("\n","\\n"); }
    static void Note(string s)
    {
        Directory.CreateDirectory(Queue);
        File.AppendAllText(Path.Combine(Queue,"noworld_factory_export.log"),
            DateTime.UtcNow.ToString("o")+" "+s+Environment.NewLine);
    }
    static bool Allowed(string candidate,string root)
    {
        string full=Path.GetFullPath(candidate), basePath=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar);
        return full.StartsWith(basePath+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase);
    }
    void Awake(){Note("ORIGINAL_MAKER_QUEUE_LOADED");}
    void Update()
    {
        if(completed||working||Time.realtimeSinceStartup<nextPoll)return;
        nextPoll=Time.realtimeSinceStartup+1f;
        if(!Directory.Exists(Queue) || Directory.GetFiles(Queue,"*.tsv").Length==0)return;
        try
        {
            if(MakerAPI.InsideAndLoaded && MakerAPI.GetCharacterControl()!=null)
            { working=true; StartCoroutine(ExportRequests()); return; }
            if(!entered && Time.realtimeSinceStartup>2f)
            {
                var title=UnityEngine.Object.FindObjectOfType<TitleScene>();
                if(title!=null){entered=true;Note("MAKER_API_ENTER");title.OnCustomFemale();}
            }
        }
        catch(Exception e){Note("MAKER_INIT_FAILED "+e); completed=true;}
    }
    IEnumerator ExportRequests()
    {
        foreach(string request in Directory.GetFiles(Queue,"*.tsv").OrderBy(x=>x))
        {
            string[] parts=File.ReadAllText(request).Trim().Split('\t');
            if(parts.Length!=4)throw new IOException("Bad queued card source contract");
            string id=parts[0], card=Path.GetFullPath(parts[1]), sha=parts[2], receipt=Path.GetFullPath(parts[3]);
            if(id.Length<3||id.Length>80||id.Any(c=>!(Char.IsLower(c)||Char.IsDigit(c)||c=='.'||c=='_'||c=='-')))
                throw new IOException("Untrusted factory job ID");
            if(sha.Length!=64||sha.Any(c=>!Uri.IsHexDigit(c)))
                throw new IOException("Bad original card SHA256");
            if(!Allowed(card,AllowedProject) && !Allowed(card,AllowedGameCards))
                throw new IOException("Card is outside protected original source directories");
            if(!Allowed(receipt,Path.Combine(AllowedProject,@"..\..\Saved\NoWorldFactory")) &&
               !Allowed(receipt,Path.Combine(AllowedProject,@"..\..\Saved\NoWorldContentFactory")))
                throw new IOException("Receipt must remain under dedicated saved factory state");
            if(!File.Exists(card)||Hash(card)!=sha.ToLowerInvariant())
                throw new IOException("Original Koikatsu card missing/mismatch: "+id);
            if(File.Exists(receipt))
            {
                Note("PRIOR_RECEIPT_EXISTS "+id);
                File.Move(request,request+".done");
                continue;
            }
            var actor=MakerAPI.GetCharacterControl();
            if(actor==null)throw new IOException("Real Maker character controller absent");
            Note("LOADING_EXACT_CARD "+id);
            actor.chaFile.LoadFileLimited(card,actor.sex,true,true,true,true,true);
            if(actor.chaFile.GetLastErrorCode()>0)throw new IOException("Real Maker card load rejected "+id);
            actor.ChangeCoordinateType(true);
            if(!actor.Reload(false,false,false,false))throw new IOException("Native card model reload failed "+id);
            yield return new WaitForSeconds(2f);
            var kkbp=AppDomain.CurrentDomain.GetAssemblies().FirstOrDefault(
                x=>x.GetName().Name=="KKBP_Exporter");
            if(kkbp==null)throw new IOException("Required installed KKBP exporter assembly absent");
            var typ=kkbp.GetType("PmxExpoter",true);
            var installed=UnityEngine.Object.FindObjectOfType(typ) as MonoBehaviour;
            if(installed==null)throw new IOException("Installed KKBP native exporter instance absent");
            var cfg=typ.GetField("exportConfig",BindingFlags.Instance|BindingFlags.NonPublic).GetValue(installed);
            foreach(string key in new[]{"batchExport","exportAllOutfits","exportAllVariations",
                "exportHitBoxes","exportCurrentPose","exportWithEnabledShapekeys"})
            {
                var f=cfg.GetType().GetField(key,BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic);
                if(f!=null)f.SetValue(cfg,false);
            }
            var builder=Activator.CreateInstance(kkbp.GetType("PmxBuilder",true),true);
            typ.GetField("builder",BindingFlags.Instance|BindingFlags.NonPublic).SetValue(installed,builder);
            string[] before=Directory.GetDirectories(GameExports);
            var runner=(IEnumerator)typ.GetMethod("StartBuild",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(installed,null);
            yield return installed.StartCoroutine(runner);
            string[] created=Directory.GetDirectories(GameExports).Except(before).ToArray();
            string[] models=created.SelectMany(x=>Directory.GetFiles(x,"model.pmx",SearchOption.TopDirectoryOnly))
                .Where(x=>new FileInfo(x).Length>10000).ToArray();
            if(models.Length!=1)throw new IOException("Expected one real newly exported PMX; found "+models.Length);
            string pmx=models[0];
            Directory.CreateDirectory(Path.GetDirectoryName(receipt));
            File.WriteAllText(receipt,"{\"schema\":\"noworld.koikatsu.factory.export.v1\",\"status\":\"EXPORTED_NATIVE_EXACT_ORIGINAL_CARD\",\"job_id\":\""+
                Json(id)+"\",\"source_card_sha256\":\""+Hash(card)+"\",\"pmx_path\":\""+
                Json(pmx)+"\",\"pmx_sha256\":\""+Hash(pmx)+"\"}");
            File.Move(request,request+".done");
            Note("SOURCE_EXPORTED "+id+" "+pmx);
            yield return null;
        }
        working=false;
        completed=true;
        Note("MAKER_QUEUE_FINISHED");
        // Optional only on an isolated factory process; never close a user's
        // existing Maker session without a dedicated factory request.
        if(File.Exists(Path.Combine(Queue,"NOWORLD_FACTORY_EXIT_AFTER_BATCH.flag")))
            Application.Quit();
    }
}
