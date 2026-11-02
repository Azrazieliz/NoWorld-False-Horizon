using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using BepInEx;
using KKAPI.Maker;
using UnityEngine;

// Source-side, one-shot direct Unity mesh extraction from exact author-approved
// GRAND_CLOTHED Koikatsu card. This avoids KKBP's giant in-memory PMX builder.
// All vertices, every submesh, skin weights, all shape-key frames and original
// bone transforms are saved without downsampling or substituting geometry.
[BepInPlugin("noworld.gate12.grand.direct.meshexport", "NoWorld Original Grand Direct Skinned Mesh Stream", "1.0.0")]
public sealed class Gate12GrandExactUnityMeshStream : BaseUnityPlugin
{
    const string Card = @"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Assets\NoWorld\Luminous\KoikatsuOriginalCards\LUMINOUS_GRAND_CONVERGENCE_CLOTHED_KK.png";
    const string CardSha = "26c48e1c03c154f9f39a0b06afaade53423c11dbd079604e430162eb4cae5353";
    const string Out = @"D:\UnrealProjects\G12_LuminousNativeImport_20261031\Saved\GrandExactUnitySourceStream";
    const string Authorization = @"D:\UnrealProjects\G12_LuminousNativeImport_20261031\Saved\CorrectKoikatsuExports\Gate12_Grand_Exact_Unity_Mesh_Extraction_Authorized.flag";
    bool entered, exporting, completed;
    float poll;
    static string Sha(string file)
    {
        using (FileStream fs=File.OpenRead(file))
        using (SHA256 sha=SHA256.Create())
            return BitConverter.ToString(sha.ComputeHash(fs)).Replace("-","").ToLowerInvariant();
    }
    static void Note(string s)
    {
        Directory.CreateDirectory(Out);
        File.AppendAllText(Path.Combine(Out,"Grand_Exact_Unity_Source_Progress.log"),
            DateTime.UtcNow.ToString("o")+" "+s+Environment.NewLine);
    }
    void Awake(){Note("DIRECT_UNITY_PLUGIN_LOADED no_KKBP_builder");}
    void Update()
    {
        if (completed || exporting || Time.realtimeSinceStartup<poll) return;
        poll=Time.realtimeSinceStartup+1f;
        if (!File.Exists(Authorization)) return;
        try
        {
            if (!File.Exists(Card) || Sha(Card)!=CardSha)
                throw new IOException("Grand original card SHA256 invalid");
            if(MakerAPI.InsideAndLoaded && MakerAPI.GetCharacterControl()!=null)
            {
                exporting=true; StartCoroutine(Run()); return;
            }
            if(!entered && Time.realtimeSinceStartup>2)
            {
                TitleScene title=UnityEngine.Object.FindObjectOfType<TitleScene>();
                if(title!=null) {entered=true;Note("ENTER_FEMALE_MAKER");title.OnCustomFemale();}
            }
        }
        catch(Exception ex){Note("ERROR "+ex);completed=true;}
    }
    // Unity's Vector3 == and != operators are approximate. These exact float
    // comparisons preserve every arbitrarily small nonzero morph delta.
    static bool HasSourceMorphValue(Vector3 v)
    {return v.x!=0f || v.y!=0f || v.z!=0f;}
    static void WriteVec(BinaryWriter w,Vector2 v){w.Write(v.x);w.Write(v.y);}
    static void WriteVec(BinaryWriter w,Vector3 v){w.Write(v.x);w.Write(v.y);w.Write(v.z);}
    static void WriteQuaternion(BinaryWriter w,Quaternion q)
    {w.Write(q.x);w.Write(q.y);w.Write(q.z);w.Write(q.w);}
    static void WriteMat(BinaryWriter w,Matrix4x4 m)
    {for(int a=0;a<4;a++) for(int b=0;b<4;b++) w.Write(m[a,b]);}
    static string PathOf(Transform t,Transform root)
    {
        if(t==null)return "";
        var names=new List<string>();
        var guard=0;
        for(var x=t;x!=null && guard++<512;x=x.parent)
        {
            names.Add(x.name);
            if(x==root)break;
        }
        names.Reverse();
        return String.Join("/",names.ToArray());
    }
    static string TextureKey(string value)
    {
        using(SHA256 sha=SHA256.Create())
        {
            byte[] bytes=Encoding.UTF8.GetBytes(value);
            byte[] digest=sha.ComputeHash(bytes);
            return BitConverter.ToString(digest,0,12).Replace("-","").ToLowerInvariant();
        }
    }
    static void SaveOriginalMaterialTextures(Material[] materials,int rendererIndex)
    {
        string folder=Path.Combine(Out,"ExactGrandOriginalTextures");
        Directory.CreateDirectory(folder);
        string table=Path.Combine(Out,
            "ExactGrand_Renderer_"+rendererIndex.ToString("D3")+"_OriginalTextures.tsv");
        using(FileStream file=new FileStream(table,FileMode.CreateNew,FileAccess.Write,FileShare.None))
        using(StreamWriter rows=new StreamWriter(file,Encoding.UTF8))
        {
            rows.WriteLine(String.Join(((char)9).ToString(),new string[]{"renderer","material","property_utf8_base64","texture_utf8_base64","png_file","width","height","png_sha256"}));
            for(int materialIndex=0;materialIndex<materials.Length;materialIndex++)
            {
                Material material=materials[materialIndex];
                if(material==null)continue;
                string[] properties=new string[]{"_MainTex","_BumpMap","_NormalMap","_DetailNormalMap","_DetailAlbedoMap","_EmissionMap","_MetallicGlossMap","_SpecGlossMap","_OcclusionMap","_ParallaxMap","_ShadowTex","_ShadowColorTex","_RampTex","_MatCap","_MaskTex","_AlphaMask","_ColorMask","_OverlayTex","_LineMask","_BlushTex","_MakeupTex","_PatternTex","_DecalTex","_MainTex2","_NormalMap2","_SkinColorMask","_ToonShadowTex","_AlphaTex","_LiquidTex","_LutTex"};
                foreach(string property in properties)
                {
                    Texture original=material.HasProperty(property)?material.GetTexture(property):null;
                    if(original==null)continue;
                    // Any Unity Texture subtype can be captured through Graphics.Blit below.
                    // RenderTexture/Texture2D material masks must not abort original geometry.
                    int width=original.width,height=original.height;
                    if(width<1 || height<1 || width>8192 || height>8192)
                        throw new IOException("Original texture out of safe full-resolution range: "+original.name);
                    string key=TextureKey(rendererIndex+"|"+materialIndex+"|"+property+"|"+original.name);
                    string filename="Grand_SourceTexture_"+key+".png";
                    string output=Path.Combine(folder,filename);
                    if(File.Exists(output))
                        throw new IOException("Original source texture already exists; refusing overwrite "+output);
                    RenderTexture previous=RenderTexture.active;
                    RenderTexture buffer=null;
                    Texture2D copy=null;
                    try
                    {
                        buffer=RenderTexture.GetTemporary(width,height,0,RenderTextureFormat.ARGB32);
                        Graphics.Blit(original,buffer);
                        RenderTexture.active=buffer;
                        copy=new Texture2D(width,height,TextureFormat.RGBA32,false);
                        copy.ReadPixels(new Rect(0,0,width,height),0,0,false);
                        copy.Apply(false,false);
                        byte[] png=copy.EncodeToPNG();
                        if(png==null || png.Length<100)
                            throw new IOException("Exact source texture pixel extraction failed "+original.name);
                        using(FileStream image=new FileStream(output,FileMode.CreateNew,FileAccess.Write,FileShare.None))
                            image.Write(png,0,png.Length);
                        rows.Write(rendererIndex);
                        rows.Write((char)9);rows.Write(materialIndex);
                        rows.Write((char)9);rows.Write(Convert.ToBase64String(Encoding.UTF8.GetBytes(property)));
                        rows.Write((char)9);rows.Write(Convert.ToBase64String(Encoding.UTF8.GetBytes(original.name)));
                        rows.Write((char)9);rows.Write(filename);
                        rows.Write((char)9);rows.Write(width);
                        rows.Write((char)9);rows.Write(height);
                        rows.Write((char)9);rows.WriteLine(Sha(output));
                        rows.Flush();
                        Note("EXACT_GRAND_MATERIAL_TEXTURE renderer="+rendererIndex+
                             " material="+materialIndex+" property="+property+
                             " source_name="+original.name+" width="+width+" height="+height+
                             " png_sha256="+Sha(output));
                    }
                    finally
                    {
                        RenderTexture.active=previous;
                        if(copy!=null)UnityEngine.Object.DestroyImmediate(copy);
                        if(buffer!=null)RenderTexture.ReleaseTemporary(buffer);
                    }
                }
            }
        }
        Note("EXACT_GRAND_SOURCE_MATERIAL_TEXTURE_TABLE_COMPLETED="+table);
    }
    static void DumpMesh(SkinnedMeshRenderer renderer,Transform owner,int index)
    {
        Mesh m=renderer.sharedMesh;
        if (m==null || m.vertexCount<1) throw new IOException("Missing actual source mesh");
        string path=Path.Combine(Out,"ExactGrand_Renderer_"+index.ToString("D3")+".unitymesh");
        if(File.Exists(path))throw new IOException("Previous renderer exists; no overwrite");
        using (var s=new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.None,128*1024))
        using (var w=new BinaryWriter(s))
        {
            w.Write("NOWORLD_EXACT_KOIKATSU_UNITY_SKIN_V3_SPARSE_MORPHS_CURRENT_WEIGHTS");
            w.Write(CardSha);w.Write(index);w.Write(renderer.name);
            w.Write(PathOf(renderer.transform,owner));
            w.Write(m.vertexCount);w.Write(m.subMeshCount);
            w.Write(m.blendShapeCount);
            WriteMat(w,renderer.transform.localToWorldMatrix);
            Vector3[] v=m.vertices,n=m.normals;
            Vector2[] uv=m.uv;
            BoneWeight[] weights=m.boneWeights;
            if(weights.Length!=v.Length)throw new IOException("Missing authentic skin weights");
            w.Write(n.Length);w.Write(uv.Length);w.Write(weights.Length);
            for(int i=0;i<v.Length;i++)WriteVec(w,v[i]);
            for(int i=0;i<n.Length;i++)WriteVec(w,n[i]);
            for(int i=0;i<uv.Length;i++)WriteVec(w,uv[i]);
            Transform[] bones=renderer.bones;
            Matrix4x4[] bindposes=m.bindposes;
            if(bones.Length!=bindposes.Length)throw new IOException("Source skin bone-bindpose mismatch");
            w.Write(bones.Length);
            w.Write(PathOf(renderer.rootBone,owner));
            for(int i=0;i<bones.Length;i++)
            {
                Transform bone=bones[i];
                w.Write(PathOf(bone,owner));
                WriteMat(w,bindposes[i]);
                WriteMat(w,bone!=null?bone.localToWorldMatrix:Matrix4x4.identity);
            }
            for(int i=0;i<weights.Length;i++)
            {
                BoneWeight a=weights[i];
                w.Write(a.boneIndex0);w.Write(a.boneIndex1);w.Write(a.boneIndex2);w.Write(a.boneIndex3);
                w.Write(a.weight0);w.Write(a.weight1);w.Write(a.weight2);w.Write(a.weight3);
            }
            for(int sub=0;sub<m.subMeshCount;sub++)
            {
                int[] triangles=m.GetTriangles(sub);
                w.Write(triangles.Length);
                foreach(int x in triangles)w.Write(x);
                Note("EXACT_GRAND_SUBMESH renderer="+index+" sub="+sub+" indices="+triangles.Length);
            }
            Material[] materials=renderer.sharedMaterials;
            w.Write(materials.Length);
            foreach(Material mat in materials)
            {
                w.Write(mat!=null?mat.name:"");
                w.Write(mat!=null && mat.shader!=null?mat.shader.name:"");
                var color=mat!=null && mat.HasProperty("_Color")?mat.color:Color.white;
                w.Write(color.r);w.Write(color.g);w.Write(color.b);w.Write(color.a);
                Texture texture=mat!=null?mat.mainTexture:null;
                w.Write(texture!=null?texture.name:"");
                w.Write(texture!=null?texture.width:0);w.Write(texture!=null?texture.height:0);
            }
            var deltaV=new Vector3[v.Length];
            var deltaN=new Vector3[v.Length];
            var deltaT=new Vector3[v.Length];
            for(int shape=0;shape<m.blendShapeCount;shape++)
            {
                w.Write(m.GetBlendShapeName(shape));
                // Current authored facial/body proportions are not necessarily
                // the neutral Basis. Preserve original active morph weights.
                w.Write(renderer.GetBlendShapeWeight(shape));
                int frames=m.GetBlendShapeFrameCount(shape);
                w.Write(frames);
                for(int frame=0;frame<frames;frame++)
                {
                    w.Write(m.GetBlendShapeFrameWeight(shape,frame));
                    Array.Clear(deltaV,0,deltaV.Length);
                    Array.Clear(deltaN,0,deltaN.Length);
                    Array.Clear(deltaT,0,deltaT.Length);
                    m.GetBlendShapeFrameVertices(shape,frame,deltaV,deltaN,deltaT);
                    int changed=0;
                    for(int i=0;i<v.Length;i++)
                        if(HasSourceMorphValue(deltaV[i]) || HasSourceMorphValue(deltaN[i]) ||
                           HasSourceMorphValue(deltaT[i]))changed++;
                    w.Write(changed);
                    for(int i=0;i<v.Length;i++)
                    {
                        if(!HasSourceMorphValue(deltaV[i]) && !HasSourceMorphValue(deltaN[i]) &&
                           !HasSourceMorphValue(deltaT[i]))continue;
                        w.Write(i);
                        WriteVec(w,deltaV[i]);
                        WriteVec(w,deltaN[i]);
                        WriteVec(w,deltaT[i]);
                    }
                }
                if(shape%100==0 && shape>0) Note("MORPHS_EXACT renderer="+index+" "+shape+"/"+m.blendShapeCount);
            }
            w.Flush();
        }
        SaveOriginalMaterialTextures(renderer.sharedMaterials,index);
        var size=new FileInfo(path).Length;
        Note("EXPORTED_ORIGINAL_UNITY_SOURCE_RENDERER="+index+
            " file="+path+" bytes="+size+" sha256="+Sha(path)+
            " vertices="+m.vertexCount+" bones="+renderer.bones.Length+
            " morphs="+m.blendShapeCount+" submeshes="+m.subMeshCount);
    }
    IEnumerator Run()
    {
        var cha=MakerAPI.GetCharacterControl();
        try
        {
            Note("LOAD_EXACT_GRAND_CARD_SHA256="+CardSha);
            bool loaded=cha.chaFile.LoadFileLimited(Card,cha.sex,true,true,true,true,true);
            int nativeError=cha.chaFile.GetLastErrorCode();
            Note("SOURCE_CARD_LOADED result="+loaded+" error_code="+nativeError);
            if(nativeError>0)throw new IOException("ChaFile loader failure code "+nativeError);
            cha.ChangeCoordinateType(true);
            if(!cha.Reload(false,false,false,false))
                throw new IOException("Source Grand character could not reload");
        }
        catch(Exception error){Note("LOAD_FAILURE "+error);completed=true;yield break;}
        yield return new WaitForSeconds(3f);
        SkinnedMeshRenderer[] renderers=cha.GetComponentsInChildren<SkinnedMeshRenderer>(true);
        Note("NATIVE_EXACT_SOURCE_RENDERERS="+renderers.Length);
        var indices=new List<int>();
        int vertices=0, morphs=0;
        for(int i=0;i<renderers.Length;i++)
        {
            var r=renderers[i];
            if(!r.enabled || !r.gameObject.activeInHierarchy ||
                r.sharedMesh==null || r.sharedMesh.vertexCount==0) continue;
            indices.Add(i);vertices+=r.sharedMesh.vertexCount;morphs+=r.sharedMesh.blendShapeCount;
        }
        Note("ACTIVE_AUTHENTIC_RENDERERS="+indices.Count+" vertices="+vertices+" morphs="+morphs);
        if(indices.Count<3 || vertices<5000){Note("FAILED_MISSING_ORIGINAL_GEOMETRY");completed=true;yield break;}
        foreach(int index in indices)
        {
            try{DumpMesh(renderers[index],cha.transform,index);}
            catch(Exception error){Note("EXPORT_FATAL_RENDERER="+index+" "+error);completed=true;yield break;}
            yield return null;
        }
        File.WriteAllText(Path.Combine(Out,"GATE12_GRAND_EXACT_SOURCE_STREAM_COMPLETE.sha256"),CardSha);
        Note("PASS_EXACT_GRAND_ORIGINAL_CARD_ALL_ACTIVE_RENDERERS_EXPORTED renderer_count="+indices.Count);
        completed=true;
    }
}
