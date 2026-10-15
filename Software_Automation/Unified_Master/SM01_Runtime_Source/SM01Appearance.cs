using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Collections.Generic;
using UnityEngine;
using KKAPI.Maker;
using Sideloader.AutoResolver;
using KKABMX.Core;
using KK_Plugins.MaterialEditor;

namespace SM01Runtime {
 public static class SM01Appearance {
  private static readonly System.Globalization.CultureInfo CI=System.Globalization.CultureInfo.InvariantCulture;
  private static readonly Dictionary<string,Action<ChaControl>> undo=new Dictionary<string,Action<ChaControl>>();
  private static string f(float x){return x.ToString("R",CI);}
  private static void Store(string what,Action<ChaControl> func){undo[what]=func;}
  private static int Resolve(SM01Runtime.BatchCommand cmd){
    if(string.IsNullOrEmpty(cmd.guid))return cmd.effectiveItemId;
    var ids=UniversalAutoResolver.LoadedResolutionInfo.Where(q=>q!=null && q.GUID==cmd.guid
       && (int)q.CategoryNo==cmd.category && q.Slot==cmd.originalId)
       .Select(q=>q.LocalSlot).Distinct().ToArray();
    if(ids.Length!=1)throw new IOException("Missing or conflicting GUID/category/item local-ID mapping");
    return ids[0];
  }
  private static void ChangeClothing(ChaControl c,int part,int newId){
    switch(part){
    case 1:c.ChangeClothesBot(newId,true);break;
    case 2:c.ChangeClothesBra(newId,true);break;
    case 3:c.ChangeClothesShorts(newId,true);break;
    case 4:c.ChangeClothesGloves(newId,true);break;
    case 5:c.ChangeClothesPanst(newId,true);break;
    case 6:c.ChangeClothesSocks(newId,true);break;
    case 7:c.ChangeClothesShoes(0,newId,true);break;
    case 8:c.ChangeClothesShoes(1,newId,true);break;
    default:throw new IOException("Clothing top requires separate 4-part native operation");
    }
  }
  public static bool Run(ChaControl c,BatchCommand cmd,StringBuilder s){
   if(cmd.op=="kkbp_export_native"){
     var exporterAssembly=AppDomain.CurrentDomain.GetAssemblies().FirstOrDefault(x=>x.GetName().Name=="KKBP_Exporter");
     if(exporterAssembly==null)throw new IOException("KKBP_Exporter assembly not loaded");
     var type=exporterAssembly.GetType("PmxExpoter",true);
     var field=type.GetField("Instance",System.Reflection.BindingFlags.Public|System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Static);
     var obj=field==null?null:field.GetValue(null) as MonoBehaviour;
     if(obj==null)obj=UnityEngine.Object.FindObjectOfType(type) as MonoBehaviour;
     if(obj==null){
       var pluginInfo=BepInEx.Bootstrap.Chainloader.PluginInfos.Values.FirstOrDefault(p=>p.Metadata.GUID=="kkbp.kkbpexporter");
       if(pluginInfo!=null)obj=pluginInfo.Instance as MonoBehaviour;
     }
     if(obj==null)throw new IOException("KKBP exporter not in scene or chainloader");
     var method=type.GetMethod("DoExport",System.Reflection.BindingFlags.Public|System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance);
     if(method==null)throw new IOException("KKBP DoExport unavailable");
     var enumerator=method.Invoke(obj,new object[0]) as System.Collections.IEnumerator;
     if(enumerator==null)throw new IOException("KKBP DoExport did not return coroutine");
     obj.StartCoroutine(enumerator);
     s.AppendLine("exporter\tKKBP_Exporter 5.6");
     s.AppendLine("status\tKKBP_NATIVE_EXPORT_COROUTINE_STARTED_NOT_YET_VALIDATED");
     return true;
   }
   if(cmd.op=="set_face_makeup"){
     var face=c.chaFile.custom.face;
     var makeup=face.baseMakeup;
     if(makeup==null)throw new IOException("Native makeup configuration missing");
     int oldCheek=makeup.cheekId;
     makeup.cheekId=0;
     makeup.cheekColor=new Color(1f,1f,1f,0f);
     face.cheekGlossPower=0f;
     c.AddUpdateCMFaceTexFlags(true,true,true,true,true,true,true);
     c.CreateFaceTexture();
     c.ChangeSettingCheekGlossPower();
     if(makeup.cheekId!=0 || face.cheekGlossPower!=0f)
       throw new IOException("No-blush native readback failed");
     s.AppendLine("cheek_previous\t"+oldCheek);
     s.AppendLine("cheek_id\t"+makeup.cheekId);
     s.AppendLine("cheek_alpha\t"+makeup.cheekColor.a);
     s.AppendLine("cheek_gloss\t"+face.cheekGlossPower);
     s.AppendLine("status\tNATIVE_CHEEK_MAKEUP_CLEARED");
     return true;
   }
   if(cmd.op=="set_eye_iris"){
     var colors=c.chaFile.custom.face.pupil;
     if(colors==null||colors.Length<2)throw new IOException("Iris settings missing");
     float red=cmd.value,green=cmd.value2,blue=cmd.value3;
     if(red<0||red>1||green<0||green>1||blue<0||blue>1)throw new IOException("Iris channels must be 0..1");
     var target=new Color(red,green,blue,1f);
     for(int i=0;i<2;i++){
       colors[i].baseColor=target;
       colors[i].subColor=new Color(red*.66f,green*.6f,blue*.78f,1f);
     }
     bool updated=c.ChangeSettingEye(true,true,true);
     s.AppendLine("iris_rgb\t"+f(red)+"\t"+f(green)+"\t"+f(blue));
     s.AppendLine("eyes_update\t"+updated);
     s.AppendLine("iris_readback\t"+colors[0].baseColor);
     if(!updated)throw new IOException("Native iris update returned false");
     return true;
   }
   if(cmd.op=="inspect_mod_resolution"){
     var mapping=UniversalAutoResolver.LoadedResolutionInfo.Where(q=>q!=null).ToArray();
     s.AppendLine("total_resolutions\t"+mapping.Length);
     foreach(var q in mapping.Where(q=>string.IsNullOrEmpty(cmd.guid)||q.GUID==cmd.guid).Take(200))
       s.AppendLine("mod\t"+q.GUID+"\t"+(int)q.CategoryNo+"\t"+q.Slot+"\t"+q.LocalSlot);
     return true;
   }
   if(cmd.op=="inspect_appearance"){
     s.AppendLine("coordinate\t"+MakerAPI.GetCurrentCoordinateType());
     var hair=c.chaFile.custom.hair.parts;
     for(int i=0;i<hair.Length;i++)s.AppendLine("hair\t"+i+"\t"+hair[i].id+"\t"+hair[i].baseColor);
     var clothes=c.nowCoordinate.clothes.parts;
     for(int i=0;i<clothes.Length;i++)s.AppendLine("clothes\t"+i+"\t"+clothes[i].id);
     s.AppendLine("hair_count\t"+hair.Length);s.AppendLine("clothes_count\t"+clothes.Length);
     return true;
   }
   if(cmd.op=="set_hair_palette"){
     int part=cmd.index;
     var hair=c.chaFile.custom.hair.parts;
     if(part<0||part>=hair.Length)throw new IOException("Bad hair palette slot");
     float red=cmd.value,green=cmd.value2,blue=cmd.value3;
     if(red<0||red>1||green<0||green>1||blue<0||blue>1)throw new IOException("Hair palette channel out of range");
     var item=hair[part];
     item.baseColor=new Color(red,green,blue,1f);
     item.startColor=new Color(red*.64f,green*.64f,blue*.64f,1f);
     item.endColor=new Color(Mathf.Min(1,red*1.14f),Mathf.Min(1,green*1.14f),Mathf.Min(1,blue*1.14f),1f);
     item.outlineColor=new Color(red*.48f,green*.48f,blue*.48f,1f);
     c.ChangeSettingHairColor(part,true,true,true);
     c.ChangeSettingHairOutlineColor(part);
     s.AppendLine("native_hair_rgb\t"+f(red)+"\t"+f(green)+"\t"+f(blue));
     s.AppendLine("native_hair_readback\t"+item.baseColor);
     return true;
   }
   if(cmd.op=="set_iris_asset"){
     int id=cmd.effectiveItemId;
     if(id<0)throw new IOException("Invalid iris ID");
     var p=c.chaFile.custom.face.pupil;
     for(int i=0;i<p.Length;i++)p[i].id=id;
     if(!c.ChangeSettingEye(true,true,true))throw new IOException("Native iris asset did not update");
     s.AppendLine("native_iris_id\t"+p[0].id);
     return true;
   }
   if(cmd.op=="set_hair"){
     int part=cmd.index;
     var hair=c.chaFile.custom.hair.parts;
     if(part<0||part>=hair.Length)throw new IOException("Bad hair part");
     int old=hair[part].id,newid=Resolve(cmd);
     if(old==newid){s.AppendLine("status\tNOOP");return true;}
     c.ChangeHair(part,newid,true);
     int actual=c.chaFile.custom.hair.parts[part].id;
     if(actual!=newid){c.ChangeHair(part,old,true);throw new IOException("Hair ID readback failed");}
     Store("hair",x=>x.ChangeHair(part,old,true));
     s.AppendLine("hair\t"+part+"\t"+old+"\t"+actual);
     return true;
   }
   if(cmd.op=="set_clothes_top"){
     int id=Resolve(cmd);
     if(id<0)throw new IOException("Invalid top category native source ID");
     int previous=c.nowCoordinate.clothes.parts[0].id;
     c.ChangeClothesTop(id,-1,-1,-1,true);
     int actual=c.nowCoordinate.clothes.parts[0].id;
     if(actual!=id) {
       c.ChangeClothesTop(previous,-1,-1,-1,true);
       throw new IOException("Koikatsu clothing top native readback mismatch "+id+" / "+actual);
     }
     Store("clothes_top",x=>x.ChangeClothesTop(previous,-1,-1,-1,true));
     s.AppendLine("clothes_top_before\t"+previous);
     s.AppendLine("clothes_top_after\t"+actual);
     s.AppendLine("status\tCLOTHING_TOP_APPLIED_NATIVE");
     return true;
   }
   if(cmd.op=="set_clothes"){
     int part=cmd.index;
     var clothes=c.nowCoordinate.clothes.parts;
     if(part<1||part>=clothes.Length)throw new IOException("Invalid or unsupported clothing part");
     int old=clothes[part].id,newid=Resolve(cmd);
     ChangeClothing(c,part,newid);
     int actual=c.nowCoordinate.clothes.parts[part].id;
     if(actual!=newid){ChangeClothing(c,part,old);throw new IOException("Clothes ID readback failed");}
     Store("clothes",x=>ChangeClothing(x,part,old));
     s.AppendLine("clothes\t"+part+"\t"+old+"\t"+actual);
     return true;
   }
   if(cmd.op=="set_coordinate"){
     int coordinate=cmd.index;
     if(coordinate<0||coordinate>6)throw new IOException("Coordinate index outside 7 modes");
     int old=(int)MakerAPI.GetCurrentCoordinateType();
     bool result=c.ChangeCoordinateTypeAndReload((ChaFileDefine.CoordinateType)coordinate,true);
     s.AppendLine("from\t"+old);s.AppendLine("to\t"+coordinate);
     s.AppendLine("call_success\t"+result);
     s.AppendLine("readback\t"+MakerAPI.GetCurrentCoordinateType());
     if(!result)throw new IOException("ChangeCoordinate returned false");
     Store("coordinate",x=>x.ChangeCoordinateTypeAndReload((ChaFileDefine.CoordinateType)old,true));
     return true;
   }
   if(cmd.op=="set_clothes_state"){
     int part=cmd.index, val=(int)cmd.value;
     if(part<0||part>8||val<0||val>2||val!=cmd.value)throw new IOException("Invalid clothing state");
     c.SetClothesState(part,(byte)val,true);
     s.AppendLine("state\t"+part+"\t"+val);
     s.AppendLine("status\tPRESENTATION_ONLY_NOT_CARD_DATA");
     return true;
   }
   if(cmd.op=="set_accessory_axis"){
     int slot=cmd.accessorySlot,axis=cmd.index;
     if(slot<0||slot>=c.nowCoordinate.accessory.parts.Length||axis<0||axis>2)throw new IOException("Bad accessory slot/axis");
     if(float.IsNaN(cmd.value)||float.IsInfinity(cmd.value)||cmd.value< -300||cmd.value>300)throw new IOException("Accessory vector limit");
     string group=cmd.group;
     var src=c.nowCoordinate.accessory.parts[slot].addMove;
     int row=group=="pos"?0:group=="rot"?1:group=="scale"?2:-1;
     int flags=1<<axis;
     if(row<0)throw new IOException("Bad transform group");
     Vector3 vec=src[0,row];
     float original=axis==0?vec.x:axis==1?vec.y:vec.z;
     bool ok=row==0?c.SetAccessoryPos(slot,0,cmd.value,false,flags):
       row==1?c.SetAccessoryRot(slot,0,cmd.value,false,flags):
       c.SetAccessoryScl(slot,0,cmd.value,false,flags);
     if(!ok)throw new IOException("Native accessory setter returned false");
     Vector3 updated=c.nowCoordinate.accessory.parts[slot].addMove[0,row];
     float actual=axis==0?updated.x:axis==1?updated.y:updated.z;
     s.AppendLine("transform\t"+slot+"\t"+group+"\t"+axis+"\t"+f(original)+"\t"+f(actual));
     if(Math.Abs(actual-cmd.value)>0.051f){
       if(row==0)c.SetAccessoryPos(slot,0,original,false,flags);
       else if(row==1)c.SetAccessoryRot(slot,0,original,false,flags);
       else c.SetAccessoryScl(slot,0,original,false,flags);
       throw new IOException("Accessory transform readback mismatch");
     }
     Store("accessory_axis",x=>{
       if(row==0)x.SetAccessoryPos(slot,0,original,false,flags);
       else if(row==1)x.SetAccessoryRot(slot,0,original,false,flags);
       else x.SetAccessoryScl(slot,0,original,false,flags);
     });
     return true;
   }
   if(cmd.op=="probe_skin_deformation"){
     int slot=cmd.accessorySlot;
     var items=c.objAccessory;
     if(slot<0||items==null||slot>=items.Length||items[slot]==null)
       throw new IOException("Accessory slot unavailable for deformation test");
     var skins=items[slot].GetComponentsInChildren<SkinnedMeshRenderer>(true);
     if(skins.Length!=1||skins[0].sharedMesh==null)
       throw new IOException("Unique skinned mesh not found");
     var sk=skins[0];if(sk.bones==null||sk.bones.Length<2)
       throw new IOException("No rig bones to animate");
     var pairs=sk.sharedMesh.boneWeights;
     if(pairs==null||pairs.Length!=sk.sharedMesh.vertexCount)
       throw new IOException("No vertex skin mapping");
     int index=-1;
     for(int i=sk.bones.Length-1;i>=1;i--){
       if(sk.bones[i]!=null && pairs.Any(x=>
         (x.boneIndex0==i && x.weight0>0.03f)||
         (x.boneIndex1==i && x.weight1>0.03f)||
         (x.boneIndex2==i && x.weight2>0.03f)||
         (x.boneIndex3==i && x.weight3>0.03f))){index=i;break;}
     }
     if(index<1)throw new IOException("No weighted movable bone");
     Transform bone=sk.bones[index];Quaternion old=bone.localRotation;
     Mesh before=new Mesh();Mesh after=new Mesh();Mesh returned=new Mesh();
     try{
       sk.BakeMesh(before);
       bone.localRotation=old*Quaternion.Euler(0,0,15);
       sk.BakeMesh(after);
       bone.localRotation=old;
       sk.BakeMesh(returned);
       if(before.vertexCount!=after.vertexCount||before.vertexCount!=returned.vertexCount)
         throw new IOException("Skin bake vertex count changed");
       var a=before.vertices;var b=after.vertices;var z=returned.vertices;
       float movement=0f,undoError=0f;
       for(int i=0;i<a.Length;i++){
         movement=Mathf.Max(movement,(a[i]-b[i]).magnitude);
         undoError=Mathf.Max(undoError,(a[i]-z[i]).magnitude);
       }
       s.AppendLine("skinned_vertex_count\t"+before.vertexCount);
       s.AppendLine("bone_count\t"+sk.bones.Length);
       s.AppendLine("tested_bone\t"+index+"\t"+bone.name);
       s.AppendLine("max_vertex_deformation\t"+movement.ToString("R",CI));
       s.AppendLine("max_undo_deviation\t"+undoError.ToString("R",CI));
       if(movement<0.00001f || undoError>0.0001f)
         throw new IOException("Skin deformation/undo did not pass");
       s.AppendLine("status\tPASS_SKIN_DEFORMATION_AND_REVERSAL");
     }finally{
       bone.localRotation=old;
       UnityEngine.Object.Destroy(before);UnityEngine.Object.Destroy(after);UnityEngine.Object.Destroy(returned);
     }
     return true;
   }
   if(cmd.op=="inspect_accessory_geometry"){
     int slot=cmd.accessorySlot;
     var items=c.objAccessory;
     if(slot<0||items==null||slot>=items.Length||items[slot]==null)
       throw new IOException("No game object for requested accessory slot");
     int meshes=0;
     foreach(MeshFilter filter in items[slot].GetComponentsInChildren<MeshFilter>(true)){
       Mesh m=filter.sharedMesh;
       if(m==null)continue;
       s.AppendLine("mesh\t"+filter.gameObject.name+"\t"+m.name+"\t"+m.vertexCount+"\t"+m.triangles.Length/3+"\t"+m.bounds.ToString());
       meshes++;
     }
     foreach(SkinnedMeshRenderer smr in items[slot].GetComponentsInChildren<SkinnedMeshRenderer>(true)){
       Mesh m=smr.sharedMesh;
       if(m==null)continue;
       s.AppendLine("skinned\t"+smr.name+"\t"+m.name+"\t"+m.vertexCount+"\t"+m.triangles.Length/3+"\t"+smr.bones.Length+"\t"+m.bounds.ToString());
       meshes++;
     }
     s.AppendLine("mesh_count\t"+meshes);
     return true;
   }
   if(cmd.op=="inspect_accessory_materials"){
     int slot=cmd.accessorySlot;
     GameObject[] array=c.objAccessory;
     if(array==null||slot<0||slot>=array.Length||array[slot]==null)
       throw new IOException("No actual accessory GameObject at slot");
     Renderer[] renders=array[slot].GetComponentsInChildren<Renderer>(true);
     s.AppendLine("renderer_count\t"+renders.Length);
     foreach(Renderer renderer in renders.Take(60)){
       foreach(Material mat in renderer.sharedMaterials){
         if(mat==null)continue;
         s.AppendLine("material\t"+renderer.name+"\t"+mat.name+"\t"+(mat.shader!=null?mat.shader.name:"NO_SHADER")+"\t"+(mat.HasProperty("_Color")?mat.GetColor("_Color").ToString():"NOCOLOR"));
       }
     }
     return true;
   }
   if(cmd.op=="set_accessory_color"){
     int slot=cmd.accessorySlot;var items=c.objAccessory;
     if(slot<0||items==null||slot>=items.Length||items[slot]==null)throw new IOException("No accessory GameObject");
     string rendererName=cmd.group, matName=cmd.fileName, property=cmd.parentBone;
     if(string.IsNullOrEmpty(rendererName)||string.IsNullOrEmpty(matName)||string.IsNullOrEmpty(property)
       ||!property.StartsWith("_"))throw new IOException("Explicit renderer/material/property required");
     var matches=items[slot].GetComponentsInChildren<Renderer>(true).Where(x=>x.name==rendererName).ToArray();
     if(matches.Length!=1)throw new IOException("Renderer mismatch, wanted "+rendererName+", count="+matches.Length+", available="+string.Join("|",items[slot].GetComponentsInChildren<Renderer>(true).Select(x=>x.name).ToArray()));
     var mats=matches[0].materials.Where(item=>item!=null && item.name.StartsWith(matName,StringComparison.Ordinal)).ToArray();
     if(mats.Length!=1)throw new IOException("Material ambiguous");
     Material m=mats[0];
     if(!m.HasProperty(property))throw new IOException("Material property absent");
     Color before=m.GetColor(property);
     Color wanted=new Color(cmd.value,cmd.value2,cmd.value3,cmd.value4);
     foreach(float channel in new[]{wanted.r,wanted.g,wanted.b,wanted.a})
       if(float.IsInfinity(channel)||float.IsNaN(channel)||channel<0||channel>5)throw new IOException("Bad RGBA channel");
     MaterialEditorCharaController cc=MaterialEditorPlugin.GetCharaController(c);
     if(cc==null)throw new IOException("MaterialEditor controller absent");
     cc.SetMaterialColorProperty(slot,MaterialEditorCharaController.ObjectType.Accessory,m,property,wanted,items[slot],true);
     // MaterialEditor stores persistence metadata but may defer the actual preview shader update.
    m.SetColor(property,wanted);
    Color after=m.GetColor(property);
    if(Mathf.Abs(after.r-wanted.r)>.005f||Mathf.Abs(after.g-wanted.g)>.005f||
       Mathf.Abs(after.b-wanted.b)>.005f||Mathf.Abs(after.a-wanted.a)>.005f)
       throw new IOException("Shader immediate RGBA readback mismatch");
     var saved=cc.GetMaterialColorPropertyValue(slot,MaterialEditorCharaController.ObjectType.Accessory,m,property,items[slot]);
     s.AppendLine("color\t"+m.name+"\t"+property+"\t"+before+"\t"+after);
     s.AppendLine("persisted_metadata\t"+saved.HasValue);
     if(!saved.HasValue)throw new IOException("MaterialEditor did not store property");
     Store("material_color",x=>{cc.SetMaterialColorProperty(slot,MaterialEditorCharaController.ObjectType.Accessory,m,property,before,items[slot],true);m.SetColor(property,before);});
     return true;
   }
   if(cmd.op=="inspect_bones"){
     BoneController b=c.GetComponent<BoneController>();
     if(b==null)throw new IOException("KKABMX controller missing");
     var names=b.GetAllPossibleBoneNames().Take(300).ToArray();
     s.AppendLine("bone_count_sample\t"+names.Length);
     foreach(string n in names)s.AppendLine("bone\t"+n);
     s.AppendLine("modifier_count\t"+b.GetAllModifiers().Count());
     return true;
   }
   if(cmd.op=="set_bone_scale"){
     BoneController b=c.GetComponent<BoneController>();
     if(b==null)throw new IOException("KKABMX controller missing");
     string name=cmd.group;
     if(string.IsNullOrEmpty(name)||name.Length>150||!b.GetAllPossibleBoneNames().Contains(name))
       throw new IOException("Unknown bone");
     float factor=cmd.value;
     if(float.IsNaN(factor)||float.IsInfinity(factor)||factor<0.5f||factor>1.5f)throw new IOException("Bone scale guard 0.5-1.5");
     var modifier=b.GetOrAddModifier(name,BoneLocation.BodyTop);
     var coord=MakerAPI.GetCurrentCoordinateType();
     BoneModifierData data=modifier.GetModifier(coord);
     Vector3 old=data.ScaleModifier;
     data.ScaleModifier=new Vector3(factor,factor,factor);
     s.AppendLine("bone_scale\t"+name+"\t"+old+"\t"+data.ScaleModifier);
     Store("bone_scale",x=>{data.ScaleModifier=old;});
     return true;
   }
   if(cmd.op=="set_bone_deformation"){
     BoneController b=c.GetComponent<BoneController>();
     if(b==null)throw new IOException("Native KKABMX bone controller not installed");
     string name=cmd.group,mode=cmd.parentBone;
     if(string.IsNullOrEmpty(name)||name.Length>140||!b.GetAllPossibleBoneNames().Contains(name))
       throw new IOException("Requested actual body bone does not exist");
     if(mode!="scale" && mode!="position" && mode!="rotation")
       throw new IOException("Bone deformation mode must be scale,position,rotation");
     float x=cmd.value,y=cmd.value2,z=cmd.value3;
     float[] vals=new[]{x,y,z};
     foreach(float v in vals){
       if(float.IsNaN(v)||float.IsInfinity(v))throw new IOException("Nonfinite bone axis");
       if(mode=="scale" && (v<0.1f||v>4f))throw new IOException("Scale outside 0.1..4.0 guard");
       if(mode=="position" && (v<-.55f||v>.55f))throw new IOException("Position outside -0.55..+0.55 guard");
       if(mode=="rotation" && (v< -110f||v>110f))throw new IOException("Rotation outside -110..+110 guard");
     }
     // Existing game body bones are modified via native ABMX serializable coordinate data,
     // not via transient Unity transforms or unbounded raw meshes.
     BoneModifier mod=b.GetOrAddModifier(name,BoneLocation.BodyTop);
     if(mod==null)throw new IOException("Unable to create ABMX modifier");
     var coord=MakerAPI.GetCurrentCoordinateType();
     BoneModifierData dat=mod.GetModifier(coord);
     if(dat==null)throw new IOException("ABMX coordinate data absent");
     Vector3 before=mode=="scale"?dat.ScaleModifier:
       mode=="position"?dat.PositionModifier:dat.RotationModifier;
     Vector3 request=new Vector3(x,y,z);
     if(mode=="scale")dat.ScaleModifier=request;
     if(mode=="position")dat.PositionModifier=request;
     if(mode=="rotation")dat.RotationModifier=request;
     b.NeedsFullRefresh=true;
     Vector3 after=mode=="scale"?dat.ScaleModifier:
       mode=="position"?dat.PositionModifier:dat.RotationModifier;
     if((after-request).sqrMagnitude>.00000001f)throw new IOException("ABMX bone deformation data readback mismatch");
     s.AppendLine("bone_deformation\t"+name+"\t"+mode+"\t"+before+"\t"+after);
     s.AppendLine("full_refresh_requested\t"+b.NeedsFullRefresh);
     s.AppendLine("note\tABMX_SERIALIZED_VALUES_VALIDATED_VISUAL_BONE_POSE_REQUIRES_NEXT_FRAME_TEST");
     Store("bone_deformation",xcha=>{
       if(mode=="scale")dat.ScaleModifier=before;
       if(mode=="position")dat.PositionModifier=before;
       if(mode=="rotation")dat.RotationModifier=before;
       var bc=xcha.GetComponent<BoneController>();if(bc!=null)bc.NeedsFullRefresh=true;
     });
     return true;
   }
   if(cmd.op=="inspect_bone_runtime_pose"){
     BoneController b=c.GetComponent<BoneController>();
     if(b==null)throw new IOException("KKABMX unavailable");
     string name=cmd.group;
     var mod=b.GetModifier(name,BoneLocation.BodyTop);
     if(mod==null)throw new IOException("No existing modifier for named bone");
     var dat=mod.GetModifier(MakerAPI.GetCurrentCoordinateType());
     s.AppendLine("name\t"+name);
     s.AppendLine("mod_scale\t"+dat.ScaleModifier);
     s.AppendLine("mod_position\t"+dat.PositionModifier);
     s.AppendLine("mod_rotation\t"+dat.RotationModifier);
     s.AppendLine("bone_transform_assigned\t"+(mod.BoneTransform!=null));
     if(mod.BoneTransform!=null){
       s.AppendLine("runtime_local_scale\t"+mod.BoneTransform.localScale);
       s.AppendLine("runtime_local_position\t"+mod.BoneTransform.localPosition);
       s.AppendLine("runtime_local_euler\t"+mod.BoneTransform.localEulerAngles);
     }
     return true;
   }
   if(cmd.op=="undo_appearance"){
     Action<ChaControl> action;
     if(string.IsNullOrEmpty(cmd.group)||!undo.TryGetValue(cmd.group,out action))
       throw new IOException("No reversible operation "+cmd.group);
     action(c);undo.Remove(cmd.group);
     s.AppendLine("undone\t"+cmd.group);
     return true;
   }
   return false;
  }
 }
}
