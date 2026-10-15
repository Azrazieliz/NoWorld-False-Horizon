using System;
using System.IO;
using System.Linq;
using System.Text;
using UnityEngine;

namespace SM01Runtime {
 // Universal placement for any installed accessory, including private new mesh prefab forks.
 // Uses native ChaControl methods so transformations are serialized in .png character cards.
 public static class SM01Placement {
   private static string F(float x) {return x.ToString("R",System.Globalization.CultureInfo.InvariantCulture);}
   private static float Axis(Vector3 v,int i){return i==0?v.x:i==1?v.y:v.z;}
   private static bool Set(ChaControl c,int slot,int correction,int row,int axis,float value) {
      int flags=(1<<axis);
      return row==0?c.SetAccessoryPos(slot,correction,value,false,flags):
             row==1?c.SetAccessoryRot(slot,correction,value,false,flags):
                    c.SetAccessoryScl(slot,correction,value,false,flags);
   }
   private static void Validate(ChaControl c,int slot){
      if(c.nowCoordinate==null||c.nowCoordinate.accessory==null||c.nowCoordinate.accessory.parts==null)
        throw new IOException("No editable accessory");
      var parts=c.nowCoordinate.accessory.parts;
      if(slot<0||slot>=parts.Length||slot>19||c.objAccessory==null||slot>=c.objAccessory.Length
         ||c.objAccessory[slot]==null)throw new IOException("Accessory slot not loaded or beyond 20-slot SM01 safety limit");
   }
   public static bool Execute(ChaControl c,BatchCommand cmd,StringBuilder s){
      if(cmd.op!="set_accessory_vector" && cmd.op!="set_accessory_parent" && cmd.op!="inspect_accessory_pose")
        return false;
      int slot=cmd.accessorySlot;
      Validate(c,slot);
      var parts=c.nowCoordinate.accessory.parts;
      if(cmd.op=="inspect_accessory_pose"){
         s.AppendLine("slot\t"+slot);
         s.AppendLine("parent_key\t"+parts[slot].parentKey);
         for(int secondPass=0;secondPass<2;secondPass++)for(int i=0;i<3;i++){
            var v=parts[slot].addMove[secondPass,i];
            s.AppendLine((i==0?"position":i==1?"rotation":"scale")+(secondPass==1?"2":"")+"\t"+F(v.x)+"\t"+F(v.y)+"\t"+F(v.z));
         }
         GameObject go=c.objAccessory[slot];
         s.AppendLine("world_position\t"+go.transform.position);
         s.AppendLine("world_rotation\t"+go.transform.rotation);
         s.AppendLine("world_scale\t"+go.transform.lossyScale);
         s.AppendLine("status\tINSPECTED_NATIVE_PERSISTED_TRANSFORMS");
         return true;
      }
      if(cmd.op=="set_accessory_parent"){
         string parent=cmd.parentBone;
         if(string.IsNullOrEmpty(parent)||parent.Length>80||parent.Any(x=>!char.IsLetterOrDigit(x)&&x!='_'))
           throw new IOException("Invalid parent anchor/bone");
         string previous=parts[slot].parentKey;
         if(!c.ChangeAccessoryParent(slot,parent))throw new IOException("Native attachment parent not available "+parent);
         if(parts[slot].parentKey!=parent){
            c.ChangeAccessoryParent(slot,previous);
            throw new IOException("Native parent value not persisted");
         }
         s.AppendLine("slot\t"+slot);
         s.AppendLine("parent_before\t"+previous);
         s.AppendLine("parent_after\t"+parts[slot].parentKey);
         s.AppendLine("status\tPARENT_REATTACHED_NATIVE_CARD");
         return true;
      }
      int correction=cmd.group.EndsWith("2")?1:0;
      string operation=correction==1?cmd.group.Substring(0,cmd.group.Length-1):cmd.group;
      int row=operation=="pos"?0:operation=="rot"?1:operation=="scale"?2:-1;
      if(row<0)throw new IOException("Expected pos/rot/scale");
      float[] desired={cmd.value,cmd.value2,cmd.value3};
      float min=row==2?0.001f:row==1?-360f:-300f;
      float max=row==2?10f:row==1?360f:300f;
      foreach(float val in desired){
         if(float.IsInfinity(val)||float.IsNaN(val)||val<min||val>max)
            throw new IOException("Requested accessory axis outside guarded native range");
      }
      Vector3 previousVec=parts[slot].addMove[correction,row];
      float[] before={previousVec.x,previousVec.y,previousVec.z};
      try {
         for(int i=0;i<3;i++){
           if(Math.Abs(before[i]-desired[i])<0.0001f)continue;
           if(!Set(c,slot,correction,row,i,desired[i]))throw new IOException("Native vector axis setter failed "+i);
         }
         Vector3 actual=parts[slot].addMove[correction,row];
         float[] got={actual.x,actual.y,actual.z};
         for(int i=0;i<3;i++)if(Math.Abs(got[i]-desired[i])>.051f)throw new IOException("Axis "+i+" native readback mismatch");
         s.AppendLine("slot\t"+slot);
         s.AppendLine("group\t"+cmd.group);
         s.AppendLine("before\t"+F(before[0])+"\t"+F(before[1])+"\t"+F(before[2]));
         s.AppendLine("after\t"+F(got[0])+"\t"+F(got[1])+"\t"+F(got[2]));
         s.AppendLine("status\tVECTOR_APPLIED_AND_SAVED_IN_NATIVE_COORDINATE_DATA");
      }catch{
         for(int i=0;i<3;i++)Set(c,slot,correction,row,i,before[i]);
         throw;
      }
      return true;
   }
 }
}
