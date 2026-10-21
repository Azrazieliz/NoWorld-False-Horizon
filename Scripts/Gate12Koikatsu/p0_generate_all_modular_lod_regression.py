"""Generate a structural UE5.8.3 native test from actual sourced 3-state GLB
primitive manifests. Source-derived table, no synthetic assets.
"""
import json,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[2]
report=json.loads((ROOT/"Saved/P0CharacterQuality/gltf_v3_all_states_quantified.json").read_text())
table=[]
for state,entry in zip(("CLOTHED","UNDERWEAR","NUDE"),report["files"]):
    for part in entry["primitive_details"]:
        table.append((state,int(part["mesh_index"]),int(part["primitive_index"]),int(part["vertices"])))
assert len(table)>=100
cpp=r'''#include "CoreMinimal.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/Skeleton.h"

struct FGate12P0MeshPart
{
    const TCHAR* State;
    int32 MeshIndex;
    int32 PrimitiveIndex;
    int32 SourceVertices;
};

static constexpr FGate12P0MeshPart GATE12_P0_PARTS[] =
{
'''
for state,mesh,prim,count in table:
    cpp+=f'    {{ TEXT("{state}"), {mesh}, {prim}, {count} }},\n'
cpp+=r'''};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGate12P0AllModularSourceGeometryRegression,
    "OfflineGame.Gate12.P0.AllNativeModularPartsLOD0Integrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGate12P0AllModularSourceGeometryRegression::RunTest(const FString&)
{
    bool bOkay=true;
    for (const FString State : {FString(TEXT("CLOTHED")),
                                FString(TEXT("UNDERWEAR")),
                                FString(TEXT("NUDE"))})
    {
        int32 Expected=0, Loaded=0;
        int64 SourceVertices=0, ImportedLOD0Vertices=0, ImportedLOD0Triangles=0;
        int32 MissingMaterials=0, WrongBones=0, MissingUV0=0, MissingLOD=0;
        int32 MorphTargets=0, GenericShaderParts=0;
        for (const FGate12P0MeshPart& Part : GATE12_P0_PARTS)
        {
            if (State != Part.State) continue;
            ++Expected;
            SourceVertices += Part.SourceVertices;
            const FString AssetName=FString::Printf(
                TEXT("Mesh_%02d_Prim_%02d"),Part.MeshIndex,Part.PrimitiveIndex);
            const FString Folder=FString::Printf(
                TEXT("Mesh%03dPrim%03d"),Part.MeshIndex,Part.PrimitiveIndex);
            const FString Object=FString::Printf(
                TEXT("/Game/Experimental/LuminousValentineV3/%s/Parts/%s/%s/SkeletalMeshes/%s"),
                *State,*Folder,*AssetName,*AssetName);
            USkeletalMesh* Mesh=LoadObject<USkeletalMesh>(nullptr,*Object);
            if (!Mesh)
            {
                AddError(FString::Printf(TEXT("P0 missing sourced part %s/%s"),
                                        *State,*AssetName));
                bOkay=false;continue;
            }
            ++Loaded;
            if (!Mesh->GetSkeleton() || Mesh->GetRefSkeleton().GetNum()!=975)
            {
                ++WrongBones;bOkay=false;
            }
            const TArray<FSkeletalMaterial>& Mats=Mesh->GetMaterials();
            if (Mats.IsEmpty() || !Mats[0].MaterialInterface)
            {
                ++MissingMaterials;bOkay=false;
            }
            else if (Mats[0].MaterialInterface->GetPathName().Contains(
                         TEXT("/InterchangeAssets/gltf/M_Default")))
            {
                ++GenericShaderParts;
            }
            MorphTargets += Mesh->GetMorphTargets().Num();
            const FSkeletalMeshRenderData* Render=Mesh->GetResourceForRendering();
            if (!Render || Render->LODRenderData.IsEmpty())
            {
                ++MissingLOD;bOkay=false;
                continue;
            }
            const FSkeletalMeshLODRenderData& LOD=Render->LODRenderData[0];
            ImportedLOD0Vertices+=LOD.GetNumVertices();
            for (const FSkelMeshRenderSection& Section : LOD.RenderSections)
                ImportedLOD0Triangles+=Section.NumTriangles;
            if (LOD.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()<1)
            {
                ++MissingUV0;bOkay=false;
            }
            if (LOD.GetNumVertices()<static_cast<uint32>(
                    Part.SourceVertices*0.80f))
            {
                AddError(FString::Printf(
                    TEXT("P0 unexpected LOD0 vertex loss in %s/%s source %d UE %u"),
                    *State,*AssetName,Part.SourceVertices,LOD.GetNumVertices()));
                bOkay=false;
            }
            if (Render->LODRenderData.Num()!=1)
            {
                AddWarning(FString::Printf(
                    TEXT("P0 %s/%s has %d LODs; explicitly verify LOD0 quality"),
                    *State,*AssetName,Render->LODRenderData.Num()));
            }
        }
        AddInfo(FString::Printf(
            TEXT("P0_FULL_%s: SOURCED=%d LOADED=%d SRC_VERTS=%lld ")
            TEXT("UE_LOD0_VERTS=%lld UE_LOD0_TRIS=%lld MISSING_MATERIAL=%d ")
            TEXT("WRONG_BONES=%d NO_UV0=%d NO_LOD0=%d MORPHS_IMPORTED=%d"),
            *State,Expected,Loaded,SourceVertices,ImportedLOD0Vertices,
            ImportedLOD0Triangles,MissingMaterials,WrongBones,MissingUV0,
            MissingLOD,MorphTargets));
        bOkay &= TestEqual(*FString::Printf(TEXT("%s sourced parts all installed"),*State),
                           Loaded,Expected);
        bOkay &= TestEqual(*FString::Printf(TEXT("%s body/parts skeletal bones"),*State),
                           WrongBones,0);
        bOkay &= TestEqual(*FString::Printf(TEXT("%s material slots"),*State),
                           MissingMaterials,0);
        bOkay &= TestEqual(*FString::Printf(TEXT("%s UV0 preservation"),*State),
                           MissingUV0,0);
        bOkay &= TestEqual(*FString::Printf(TEXT("%s LOD0 exists"),*State),
                           MissingLOD,0);
    }
    return bOkay;
}
#endif
'''
path=ROOT/"Source/OfflineGame/Private/Tests/OGGate12P0AllModularSourceGeometryTests.cpp"
if path.exists() and path.read_text()!=cpp:
    raise RuntimeError("Existing authored source differs; refuse to overwrite")
path.write_text(cpp,encoding="utf-8")
print("GENERATED",path,"PRIMITIVES",len(table),
      {s:sum(1 for x in table if x[0]==s) for s in ("CLOTHED","UNDERWEAR","NUDE")})
