using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using KK_Plugins.MaterialEditor;
using BepInEx;
using KKAPI.Maker;
using Sideloader.AutoResolver;
using UnityEngine;

namespace SM01Runtime {
 [Serializable] public class MorphParam {public string group;public int index; public float value;}
 [Serializable] public class BatchCommand {
  public string op;public string nonce; public string group;
  public int index;public float value;public float value2;public float value3;public float value4;
  public MorphParam[] morphs;
  public string recipeFile; public string fileName;
  public string guid; public int originalId;public int category;
  public int accessorySlot; public int accessoryType;public int effectiveItemId;public string parentBone;
 }
 [BepInPlugin("noworld.sm01.sandboxruntime","SM01 Sandbox Runtime Controller","0.4.0")]
 public sealed class RuntimeController : BaseUnityPlugin {
  private readonly CultureInfo ci=CultureInfo.InvariantCulture;
  private string dir, pending, receiptDir;
  private float nextCheck;
  private readonly List<MorphParam> undo=new List<MorphParam>();
  private int undoAccessorySlot=-1;
  private int undoAccessoryType,undoAccessoryId;
  private string undoAccessoryParent;
  private const string sandboxRoot=@"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014";
  private bool guard;
  void Awake(){
   guard=Application.dataPath.Replace('/', '\\').StartsWith(sandboxRoot,StringComparison.OrdinalIgnoreCase);
   if(!guard) { Logger.LogError("SM01 REJECTED: not running from authorized sandbox"); enabled=false;return;}
   dir=Path.Combine(Paths.BepInExRootPath,"SM01_SANDBOX");receiptDir=Path.Combine(dir,"receipts");
   pending=Path.Combine(dir,"command.json");
   Directory.CreateDirectory(dir);Directory.CreateDirectory(receiptDir);
   Logger.LogInfo("SM01 isolated sandbox runtime probe READY");
  }
  void Update(){
   if(!guard||Time.unscaledTime<nextCheck)return;
   nextCheck=Time.unscaledTime+0.30f;
   if(!File.Exists(pending))return;
   string payload="";string activeNonce=null;
   try{
    payload=File.ReadAllText(pending,Encoding.UTF8);
    string running=Path.Combine(dir,"running_"+DateTime.UtcNow.ToString("yyyyMMddHHmmssfff")+".json");
    File.Move(pending,running);
    var cmd=JsonUtility.FromJson<BatchCommand>(payload);
    if(cmd==null||string.IsNullOrEmpty(cmd.op))throw new IOException("Missing op");
    string name=ValidNonce(cmd.nonce);
    activeNonce=name;
    string result=Handle(cmd);
    File.WriteAllText(Path.Combine(receiptDir,name+".tsv"),result,new UTF8Encoding(false));
    Logger.LogInfo("SM01 command complete: "+cmd.op+" "+name);
   }catch(Exception ex){
    Logger.LogError("SM01 command error: "+ex);
    File.WriteAllText(Path.Combine(receiptDir,"ERROR_"+(activeNonce??DateTime.UtcNow.ToString("yyyyMMddHHmmssfff"))+".txt"),
      ex.ToString(),Encoding.UTF8);
   }
  }
  private static string ValidNonce(string s){
   if(string.IsNullOrEmpty(s)||s.Length>60||s.Any(c=>!(char.IsLetterOrDigit(c)||c=='_'||c=='-')))
      return DateTime.UtcNow.ToString("yyyyMMddHHmmssfff");
   return s;
  }
  private static ChaControl Character(){
   if(!MakerAPI.InsideAndLoaded)throw new InvalidOperationException("Maker not fully loaded");
   ChaControl c=MakerAPI.GetCharacterControl();
   if(c==null)throw new InvalidOperationException("No Maker character");
   return c;
  }
  private float GetMorph(ChaControl c,string group,int idx){
   if(group=="face"&&idx>=0&&idx<52)return c.GetShapeFaceValue(idx);
   if(group=="body"&&idx>=0&&idx<44)return c.GetShapeBodyValue(idx);
   throw new ArgumentOutOfRangeException("index",idx,"Unknown group/index");
  }
  private bool SetMorph(ChaControl c,string group,int idx,float value){
   if(float.IsNaN(value)||float.IsInfinity(value)||value<0f||value>2f)
    throw new ArgumentOutOfRangeException("value",value,"Sandbox bound is [0,2]");
   if(group=="face"&&idx>=0&&idx<52)return c.SetShapeFaceValue(idx,value);
   if(group=="body"&&idx>=0&&idx<44)return c.SetShapeBodyValue(idx,value);
   throw new ArgumentOutOfRangeException("index",idx,"Unknown group/index");
  }
  private string Handle(BatchCommand cmd){
   ChaControl c=Character();
   var s=new StringBuilder(16384);
   s.AppendLine("schema\tSM01_SANDBOX_RUNTIME_RECEIPT_V1");
   s.AppendLine("nonce\t"+ValidNonce(cmd.nonce));
   s.AppendLine("op\t"+cmd.op);
   s.AppendLine("utc\t"+DateTime.UtcNow.ToString("o"));
   s.AppendLine("game_path\t"+Application.dataPath);
   if(cmd.op=="snapshot"){
    s.AppendLine("coordinate\t"+MakerAPI.GetCurrentCoordinateType());
    for(int i=0;i<52;i++)s.AppendLine("face\t"+i+"\t"+c.GetShapeFaceValue(i).ToString("R",ci));
    for(int i=0;i<44;i++)s.AppendLine("body\t"+i+"\t"+c.GetShapeBodyValue(i).ToString("R",ci));
    int found=0;
    foreach(ResolveInfo info in UniversalAutoResolver.LoadedResolutionInfo){
      if(info==null)continue;
      s.AppendLine("resolved\t"+info.GUID+"\t"+info.CategoryNo+"\t"+info.LocalSlot+"\t"+info.Slot+"\t"+info.Property);
      found++;
    }
    s.AppendLine("resolved_count\t"+found);
   }else if(cmd.op=="probe_morph"){
    float old=GetMorph(c,cmd.group,cmd.index);
    s.AppendLine("old\t"+old.ToString("R",ci));
    bool ok=SetMorph(c,cmd.group,cmd.index,cmd.value);
    float actual=GetMorph(c,cmd.group,cmd.index);
    s.AppendLine("set_result\t"+ok);
    s.AppendLine("set_readback\t"+actual.ToString("R",ci));
    SetMorph(c,cmd.group,cmd.index,old);
    float restored=GetMorph(c,cmd.group,cmd.index);
    s.AppendLine("undo_readback\t"+restored.ToString("R",ci));
    s.AppendLine("roundtrip\t"+(Math.Abs(restored-old)<0.00001f));
   }else if(cmd.op=="apply_morph_batch"){
    // Unity 5.6 JsonUtility fails to populate nested command arrays on this install.
    // Read a plain TSV recipe from the sandbox ONLY, validate the entire batch,
    // then perform native edits with rollback on any failure.
    string recipeName=cmd.recipeFile;
    if(string.IsNullOrEmpty(recipeName)||Path.GetFileName(recipeName)!=recipeName||!recipeName.EndsWith(".tsv"))
       throw new IOException("Unsafe batch recipe filename");
    string recipePath=Path.Combine(Path.Combine(dir,"recipes"),recipeName);
    if(!File.Exists(recipePath))throw new IOException("Missing sandbox batch recipe");
    var actions=new List<MorphParam>();
    var seen=new HashSet<string>();
    foreach(string line in File.ReadAllLines(recipePath)){
      if(string.IsNullOrEmpty(line)||line.StartsWith("#"))continue;
      string[] fields=line.Split('\t');
      if(fields.Length!=3)throw new IOException("Invalid batch recipe fields");
      int idx=int.Parse(fields[1],ci);
      float value=float.Parse(fields[2],ci);
      string key=fields[0]+":"+idx;
      if(!seen.Add(key))throw new IOException("Duplicate morph entry "+key);
      GetMorph(c,fields[0],idx);
      if(float.IsNaN(value)||float.IsInfinity(value)||value<0f||value>2f)
        throw new IOException("Invalid value in batch");
      actions.Add(new MorphParam{group=fields[0],index=idx,value=value});
    }
    if(actions.Count<1||actions.Count>96)throw new IOException("Invalid number of batch actions");
    undo.Clear();
    try{
      foreach(MorphParam p in actions){
        float old=GetMorph(c,p.group,p.index);
        undo.Add(new MorphParam{group=p.group,index=p.index,value=old});
        bool result=SetMorph(c,p.group,p.index,p.value);
        float actual=GetMorph(c,p.group,p.index);
        if(!result||Math.Abs(actual-p.value)>0.00001f)throw new IOException("Readback mismatch for "+p.group+":"+p.index);
        s.AppendLine("applied\t"+p.group+"\t"+p.index+"\t"+old.ToString("R",ci)+"\t"+actual.ToString("R",ci)+"\t"+result);
      }
    }catch{
      for(int i=undo.Count-1;i>=0;i--){
        MorphParam p=undo[i];
        SetMorph(c,p.group,p.index,p.value);
      }
      undo.Clear();
      throw;
    }
    s.AppendLine("undo_count\t"+undo.Count);
    s.AppendLine("status\tAPPLIED_READBACK_VERIFIED_NOT_SAVED");
   }else if(cmd.op=="undo_last_batch"){
    for(int i=undo.Count-1;i>=0;i--){
     MorphParam p=undo[i];
     bool result=SetMorph(c,p.group,p.index,p.value);
     s.AppendLine("undo\t"+p.group+"\t"+p.index+"\t"+GetMorph(c,p.group,p.index).ToString("R",ci)+"\t"+result);
    }
    s.AppendLine("undo_count\t"+undo.Count);undo.Clear();
   }else if(cmd.op=="inspect_accessory"){
    var cc=c.nowCoordinate;
    if(cc==null||cc.accessory==null)throw new InvalidOperationException("Missing character accessory data");
    var parts=cc.accessory.parts;
    s.AppendLine("part_count\t"+parts.Length);
    for(int i=0;i<parts.Length;i++){
      var p=parts[i];
      if(p==null)continue;
      s.AppendLine("part\t"+i+"\t"+p.type+"\t"+p.id+"\t"+p.parentKey);
    }
   }else if(cmd.op=="apply_accessory"||cmd.op=="apply_accessory_native"){
    bool direct=cmd.op=="apply_accessory_native";
    if(!direct&&(string.IsNullOrEmpty(cmd.guid)||cmd.guid.Length>150))
      throw new IOException("Mod GUID required for resolver mode");
    if(cmd.accessorySlot<0||cmd.accessorySlot>19)throw new IOException("Sandbox accessory slot limit is 20");
    var parts=c.nowCoordinate.accessory.parts;
    if(parts==null||cmd.accessorySlot>=parts.Length)throw new IOException("Requested accessory slot unavailable");
    int effectiveId;
    if(direct){
      effectiveId=cmd.effectiveItemId;
      if(effectiveId<0)throw new IOException("Invalid native effective accessory ID");
    }else{
      var ids=UniversalAutoResolver.LoadedResolutionInfo.Where(
        x=>x!=null&&x.GUID==cmd.guid&&(int)x.CategoryNo==cmd.category&&x.Slot==cmd.originalId)
        .Select(x=>x.LocalSlot).Distinct().ToArray();
      if(ids.Length!=1)throw new IOException("Accessory GUID/category/source-ID did not map to one effective local ID");
      effectiveId=ids[0];
    }
    int oldType=parts[cmd.accessorySlot].type;
    int oldId=parts[cmd.accessorySlot].id;
    string oldParent=parts[cmd.accessorySlot].parentKey;
    string par=string.IsNullOrEmpty(cmd.parentBone)?"a_n_back":cmd.parentBone;
    try{
      c.ChangeAccessory(cmd.accessorySlot,cmd.category,effectiveId,par,true);
      var current=c.nowCoordinate.accessory.parts[cmd.accessorySlot];
      if(current.type!=cmd.category||current.id!=effectiveId)
        throw new IOException("Native ChangeAccessory did not retain desired type/ID");
    }catch{
      c.ChangeAccessory(cmd.accessorySlot,oldType,oldId,oldParent,true);
      throw;
    }
    undoAccessorySlot=cmd.accessorySlot;undoAccessoryType=oldType;
    undoAccessoryId=oldId;undoAccessoryParent=oldParent;
    s.AppendLine("resolved_guid\t"+cmd.guid);
    s.AppendLine("resolved_category\t"+cmd.category);
    s.AppendLine("original_item_id\t"+cmd.originalId);
    s.AppendLine("effective_runtime_id\t"+effectiveId);
    s.AppendLine("slot\t"+cmd.accessorySlot);
    s.AppendLine("previous_type_id\t"+oldType+"\t"+oldId);
    s.AppendLine("status\tAPPLIED_IN_SANDBOX_WITH_UNDO");
   }else if(cmd.op=="undo_last_accessory"){
    if(undoAccessorySlot<0)throw new IOException("No accessory undo recorded");
    c.ChangeAccessory(undoAccessorySlot,undoAccessoryType,undoAccessoryId,undoAccessoryParent,true);
    var now=c.nowCoordinate.accessory.parts[undoAccessorySlot];
    s.AppendLine("restored_slot\t"+undoAccessorySlot);
    s.AppendLine("restored_type_id\t"+now.type+"\t"+now.id);
    s.AppendLine("roundtrip\t"+(now.type==undoAccessoryType&&now.id==undoAccessoryId));
    undoAccessorySlot=-1;
   }else if(cmd.op=="save_derived_card"){
    string folder=Path.Combine(sandboxRoot,@"UserData\chara\female");
    Directory.CreateDirectory(folder);
    string filename="SM01_AUTOTEST_"+ValidNonce(cmd.nonce)+".png";
    string path=Path.Combine(folder,filename);
    if(File.Exists(path))throw new IOException("Refuse overwriting existing card");
    // Capture a valid PNG card header from the sandbox framebuffer.
    var tex=new Texture2D(Screen.width,Screen.height,TextureFormat.RGB24,false);
    tex.ReadPixels(new Rect(0,0,Screen.width,Screen.height),0,0);
    tex.Apply();
    var png=tex.EncodeToPNG();Destroy(tex);
    c.chaFile.pngData=png;
    // Native MaterialEditor is a KKAPI CharaCustomFunctionController. A direct ChaFileControl
    // SaveCharaFile call bypasses the standard Maker save event. Invoke the plugin's protected
    // serialization hook explicitly; without it edited textures are NOT persisted in KKEx.
    var matCtl=MaterialEditorPlugin.GetCharaController(c);
    bool materialSaved=false;
    if(matCtl!=null){
      var hook=typeof(MaterialEditorCharaController).GetMethod("OnCardBeingSaved",
        BindingFlags.Instance|BindingFlags.NonPublic);
      if(hook==null)throw new IOException("MaterialEditor card-save callback not found");
      hook.Invoke(matCtl,new object[]{KKAPI.GameMode.Maker});
      materialSaved=true;
    }
    // The Maker edits c.nowCoordinate. SaveCharaFile serializes c.chaFile.coordinate[];
    // without this native copy, accessory/clothing changes are absent from saved cards.
    var selectedCoordinate=MakerAPI.GetCurrentCoordinateType();
    int selectedIndex=(int)selectedCoordinate;
    if(selectedIndex<0||selectedIndex>=c.chaFile.coordinate.Length)
      throw new IOException("Invalid active coordinate for native card persistence");
    if(!c.AssignCoordinate(selectedCoordinate,c.nowCoordinate))
      throw new IOException("Native AssignCoordinate failed before saving");
    bool ok=c.chaFile.SaveCharaFile(path,(byte)1,false);
    if(!ok)throw new IOException("Native Koikatsu card save returned false");
    // Independently re-open and compare all saved outfit IDs, accessory IDs and vectors.
    var persisted=new ChaFileControl();
    if(!persisted.LoadCharaFile(path,(byte)1,false,false))
      throw new IOException("Saved card failed independent native reload");
    var savedCoordinate=persisted.coordinate[selectedIndex];
    var liveCoordinate=c.nowCoordinate;
    int verifiedClothes=0,verifiedAccessories=0;
    for(int j=0;j<liveCoordinate.clothes.parts.Length;j++){
      if(liveCoordinate.clothes.parts[j].id!=savedCoordinate.clothes.parts[j].id)
        throw new IOException("Saved clothing ID mismatch at part "+j);
      verifiedClothes++;
    }
    for(int j=0;j<liveCoordinate.accessory.parts.Length;j++){
      var source=liveCoordinate.accessory.parts[j];
      var actual=savedCoordinate.accessory.parts[j];
      if(source.type!=actual.type||source.id!=actual.id||source.parentKey!=actual.parentKey)
        throw new IOException("Saved accessory identity/parent mismatch at slot "+j);
      for(int correction=0;correction<2;correction++)for(int row=0;row<3;row++){
        if((source.addMove[correction,row]-actual.addMove[correction,row]).sqrMagnitude>0.0001f)
          throw new IOException("Saved accessory XYZ mismatch at slot "+j+" correction "+correction+" row "+row);
      }
      verifiedAccessories++;
    }
    s.AppendLine("native_coordinate_committed\t"+selectedIndex);
    s.AppendLine("saved_clothing_parts_verified\t"+verifiedClothes);
    s.AppendLine("saved_accessory_slots_verified\t"+verifiedAccessories);
    s.AppendLine("native_coordinate_roundtrip\tPASS");
    s.AppendLine("material_editor_save_hook\t"+materialSaved);
    s.AppendLine("save_result\t"+ok);
    s.AppendLine("exists\t"+File.Exists(path));
    s.AppendLine("file_bytes\t"+(File.Exists(path)?new FileInfo(path).Length:0));
    s.AppendLine("filename\t"+filename);
   }else if(cmd.op=="verify_derived_card"){
    string filename=cmd.fileName;
    if(string.IsNullOrEmpty(filename)||filename!=Path.GetFileName(filename)
       ||!filename.StartsWith("SM01_AUTOTEST_")||!filename.EndsWith(".png"))
       throw new IOException("Unsafe source filename for sandbox card verification");
    string path=Path.Combine(Path.Combine(sandboxRoot,@"UserData\chara\female"),filename);
    if(!File.Exists(path))throw new IOException("Derived sandbox card not found");
    var check=new ChaFileControl();
    bool ok=check.LoadCharaFile(path,(byte)1,false,false);
    s.AppendLine("independent_load_result\t"+ok);
    s.AppendLine("saved_file_bytes\t"+new FileInfo(path).Length);
    s.AppendLine("input_file\t"+filename);
    if(!ok)throw new IOException("Native Party reparse of sandbox derived card failed");
    s.AppendLine("face_index_zero\t"+check.custom.face.shapeValueFace[0].ToString("R",ci));
    s.AppendLine("body_index_zero\t"+check.custom.body.shapeValueBody[0].ToString("R",ci));
    s.AppendLine("status\tPASS_NATIVE_CARD_RELOAD");
   }else if(!SM01SurfaceShader.Execute(c,cmd,s) && !SM01UniversalSurface.Execute(c,cmd,s) && !SM01Placement.Execute(c,cmd,s) && !SM01CameraBank.Execute(c,cmd,s) && !SM01Capture.Execute(c,cmd,s) && !SM01Appearance.Run(c,cmd,s)) throw new InvalidOperationException("Unsupported sandbox op "+cmd.op);
   return s.ToString();
  }
 }
}
