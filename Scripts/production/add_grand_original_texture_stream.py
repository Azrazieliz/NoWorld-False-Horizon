"""Extend the exact-card Grand source exporter with each live material texture.
No faked pixels, no texture slots silently omitted, one GPU readback at a time."""
from pathlib import Path
p=Path(r"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Scripts\production\Gate12GrandExactUnityMeshStream.cs")
s=p.read_text("utf8")
anchor='''    static void DumpMesh(SkinnedMeshRenderer renderer,Transform owner,int index)
    {'''
assert s.count(anchor)==1
code='''    static string TextureKey(string value)
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
            rows.WriteLine("renderer\\\\tmaterial\\\\tproperty_utf8_base64\\\\ttexture_utf8_base64\\\\tpng_file\\\\twidth\\\\theight\\\\tpng_sha256");
            for(int materialIndex=0;materialIndex<materials.Length;materialIndex++)
            {
                Material material=materials[materialIndex];
                if(material==null)continue;
                string[] properties=material.GetTexturePropertyNames();
                foreach(string property in properties)
                {
                    Texture original=material.GetTexture(property);
                    if(original==null)continue;
                    if(!(original is Texture2D))
                        throw new IOException("Unsupported original texture type: "+material.name+" "+property);
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
                        rows.Write("\\\\t");rows.Write(materialIndex);
                        rows.Write("\\\\t");rows.Write(Convert.ToBase64String(Encoding.UTF8.GetBytes(property)));
                        rows.Write("\\\\t");rows.Write(Convert.ToBase64String(Encoding.UTF8.GetBytes(original.name)));
                        rows.Write("\\\\t");rows.Write(filename);
                        rows.Write("\\\\t");rows.Write(width);
                        rows.Write("\\\\t");rows.Write(height);
                        rows.Write("\\\\t");rows.WriteLine(Sha(output));
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
'''
# Avoid source formats with double-escaped tab; C# string literal '\\t' is
# intentional and produces actual tabs in the verified TSV.
s=s.replace(anchor,code+anchor)
anchor2='''        var size=new FileInfo(path).Length;
        Note("EXPORTED_ORIGINAL_UNITY_SOURCE_RENDERER="+index+'''
assert s.count(anchor2)==1
s=s.replace(anchor2,'''        SaveOriginalMaterialTextures(renderer.sharedMaterials,index);
        var size=new FileInfo(path).Length;
        Note("EXPORTED_ORIGINAL_UNITY_SOURCE_RENDERER="+index+''')
# Do not hide third-party game window or produce invisible launcher operations.
s=s.replace('''    [System.Runtime.InteropServices.DllImport("user32.dll")]
    static extern bool ShowWindow(IntPtr handle,int command);
''','')
s=s.replace('''            var handle=System.Diagnostics.Process.GetCurrentProcess().MainWindowHandle;
            if(handle!=IntPtr.Zero) ShowWindow(handle,0);
''','')
p.write_text(s,"utf8")
print("GRAND_AUTHENTIC_SOURCE_MESH_AND_ALL_MATERIAL_TEXTURE_PIXELS_STREAMED")
