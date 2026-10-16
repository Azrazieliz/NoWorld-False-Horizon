using System;
using System.IO;
using System.Linq;
using System.Text;
using UnityEngine;

namespace SM01Runtime {
 public static class P0NativeAssembledProbe {
  // QA only: imports a COPY from the sandbox into the existing sandbox Maker character.
  // Original Steam character files are never opened for writing.
  public static bool Run(ChaControl c, BatchCommand cmd, StringBuilder log) {
   if(cmd.op=="p0_load_source_clone"){
    if(string.IsNullOrEmpty(cmd.fileName) ||
       Path.GetFileName(cmd.fileName)!=cmd.fileName ||
       !cmd.fileName.StartsWith("P0_",StringComparison.Ordinal) ||
       !cmd.fileName.EndsWith(".png",StringComparison.OrdinalIgnoreCase))
     throw new IOException("P0 only loads pre-staged, manifest-hashed sandbox clone cards");
    string game=Path.GetFullPath(Path.Combine(Application.dataPath,".."));
    string root=Path.Combine(Path.Combine(Path.Combine(game,"BepInEx"),"SM01_SANDBOX"),"P0_INPUTS");
    string path=Path.GetFullPath(Path.Combine(root,cmd.fileName));
    if(!path.StartsWith(Path.GetFullPath(root)+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))
      throw new IOException("P0 clone path traversal blocked");
    if(!File.Exists(path))throw new FileNotFoundException(path);
    // NOTE direct ChaFile.Load + Reload does not yet prove BepInEx post-load plugin parity.
    bool loaded=c.chaFile.LoadCharaFile(path,(byte)1,false,false);
    if(!loaded)throw new IOException("Native card clone failed to deserialize");
    c.Reload(false,false,false,false);
    log.AppendLine("clone_loaded\t"+cmd.fileName);
    log.AppendLine("load_path\t"+path);
    log.AppendLine("load_type\tNative_ChaFile_LoadCharaFile_then_ChaControl_Reload");
    log.AppendLine("plugin_customization_parity\tUNVERIFIED");
    return true;
   }
   if(cmd.op!="p0_inspect_assembled")return false;
   var renderers=c.GetComponentsInChildren<SkinnedMeshRenderer>(true);
   var meshFilters=c.GetComponentsInChildren<MeshFilter>(true);
   int vertices=0,faces=0,blends=0,bones=0,uv=0,weights=0,materialCount=0;
   Vector3 min=new Vector3(float.MaxValue,float.MaxValue,float.MaxValue);
   Vector3 max=new Vector3(float.MinValue,float.MinValue,float.MinValue);
   foreach(var r in renderers){
    var mesh=r.sharedMesh;
    if(mesh==null)continue;
    int triangles=0;
    for(int i=0;i<mesh.subMeshCount;i++)triangles+=mesh.GetTriangles(i).Length/3;
    vertices+=mesh.vertexCount;faces+=triangles;blends+=mesh.blendShapeCount;
    bones+=r.bones==null?0:r.bones.Length;
    uv+=mesh.uv==null?0:mesh.uv.Length;
    weights+=mesh.boneWeights==null?0:mesh.boneWeights.Length;
    min=Vector3.Min(min,r.bounds.min);max=Vector3.Max(max,r.bounds.max);
    log.AppendLine("skinned\t"+r.gameObject.name+"\tmesh="+mesh.name+"\tverts="+mesh.vertexCount+
     "\ttri="+triangles+"\tblendshapes="+mesh.blendShapeCount+"\tbone_refs="+(r.bones==null?0:r.bones.Length)+
     "\tuv="+(mesh.uv==null?0:mesh.uv.Length)+"\tweights="+(mesh.boneWeights==null?0:mesh.boneWeights.Length)+
     "\tworld_height="+r.bounds.size.y);
    foreach(var m in r.sharedMaterials){
     if(m==null)continue;
     materialCount++;
     Texture texture=m.mainTexture;
     log.AppendLine("material\t"+m.name+"\tshader="+(m.shader==null?"":m.shader.name)+
      "\tqueue="+m.renderQueue+"\tMainTex="+(texture==null?"none":texture.name)+"\twidth="+
       (texture==null?0:texture.width)+"\theight="+(texture==null?0:texture.height));
    }
   }
   log.AppendLine("native_summary\tskinned_meshes="+renderers.Length+"\tother_meshes="+meshFilters.Length+
    "\tvertices="+vertices+"\ttriangles="+faces+"\tblendshapes="+blends+
    "\tbone_refs="+bones+"\tuv0="+uv+"\tweights="+weights+
    "\tmaterials="+materialCount);
   log.AppendLine("native_world_bounds\tmin="+min+"\tmax="+max+
     "\tworld_height="+(max.y-min.y));
   log.AppendLine("source_authority\tSOURCES_UNMODIFIED");
   log.AppendLine("geometry_capture_status\tASSEMBLED_RUNTIME_RENDERERS_INSPECTED");
   return true;
  }
 }
}
