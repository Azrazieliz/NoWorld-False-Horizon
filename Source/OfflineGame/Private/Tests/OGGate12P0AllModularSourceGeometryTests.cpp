#include "CoreMinimal.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
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
    { TEXT("CLOTHED"), 0, 0, 240 },
    { TEXT("CLOTHED"), 0, 1, 240 },
    { TEXT("CLOTHED"), 0, 2, 240 },
    { TEXT("CLOTHED"), 1, 0, 1162 },
    { TEXT("CLOTHED"), 2, 0, 247 },
    { TEXT("CLOTHED"), 3, 0, 2695 },
    { TEXT("CLOTHED"), 3, 1, 298 },
    { TEXT("CLOTHED"), 3, 2, 90 },
    { TEXT("CLOTHED"), 3, 3, 242 },
    { TEXT("CLOTHED"), 3, 4, 364 },
    { TEXT("CLOTHED"), 3, 5, 313 },
    { TEXT("CLOTHED"), 3, 6, 338 },
    { TEXT("CLOTHED"), 3, 7, 214 },
    { TEXT("CLOTHED"), 3, 8, 179 },
    { TEXT("CLOTHED"), 3, 9, 179 },
    { TEXT("CLOTHED"), 3, 10, 247 },
    { TEXT("CLOTHED"), 3, 11, 9604 },
    { TEXT("CLOTHED"), 4, 0, 4504 },
    { TEXT("CLOTHED"), 4, 1, 325 },
    { TEXT("CLOTHED"), 4, 2, 411 },
    { TEXT("CLOTHED"), 4, 3, 411 },
    { TEXT("CLOTHED"), 4, 4, 280 },
    { TEXT("CLOTHED"), 4, 5, 280 },
    { TEXT("CLOTHED"), 4, 6, 7637 },
    { TEXT("CLOTHED"), 4, 7, 2642 },
    { TEXT("CLOTHED"), 4, 8, 280 },
    { TEXT("CLOTHED"), 4, 9, 280 },
    { TEXT("CLOTHED"), 4, 10, 1880 },
    { TEXT("CLOTHED"), 4, 11, 2419 },
    { TEXT("CLOTHED"), 5, 0, 652 },
    { TEXT("CLOTHED"), 5, 1, 381 },
    { TEXT("CLOTHED"), 5, 2, 610 },
    { TEXT("CLOTHED"), 5, 3, 109 },
    { TEXT("CLOTHED"), 5, 4, 3462 },
    { TEXT("CLOTHED"), 5, 5, 2475 },
    { TEXT("CLOTHED"), 5, 6, 1418 },
    { TEXT("CLOTHED"), 5, 7, 2324 },
    { TEXT("CLOTHED"), 5, 8, 1200 },
    { TEXT("CLOTHED"), 5, 9, 798 },
    { TEXT("CLOTHED"), 5, 10, 2833 },
    { TEXT("CLOTHED"), 5, 11, 2539 },
    { TEXT("CLOTHED"), 5, 12, 75 },
    { TEXT("CLOTHED"), 5, 13, 75 },
    { TEXT("CLOTHED"), 5, 14, 843 },
    { TEXT("CLOTHED"), 5, 15, 895 },
    { TEXT("UNDERWEAR"), 0, 0, 240 },
    { TEXT("UNDERWEAR"), 0, 1, 240 },
    { TEXT("UNDERWEAR"), 0, 2, 240 },
    { TEXT("UNDERWEAR"), 1, 0, 1162 },
    { TEXT("UNDERWEAR"), 2, 0, 247 },
    { TEXT("UNDERWEAR"), 3, 0, 2695 },
    { TEXT("UNDERWEAR"), 3, 1, 298 },
    { TEXT("UNDERWEAR"), 3, 2, 90 },
    { TEXT("UNDERWEAR"), 3, 3, 242 },
    { TEXT("UNDERWEAR"), 3, 4, 364 },
    { TEXT("UNDERWEAR"), 3, 5, 313 },
    { TEXT("UNDERWEAR"), 3, 6, 338 },
    { TEXT("UNDERWEAR"), 3, 7, 214 },
    { TEXT("UNDERWEAR"), 3, 8, 179 },
    { TEXT("UNDERWEAR"), 3, 9, 179 },
    { TEXT("UNDERWEAR"), 3, 10, 247 },
    { TEXT("UNDERWEAR"), 3, 11, 9604 },
    { TEXT("UNDERWEAR"), 4, 0, 4504 },
    { TEXT("UNDERWEAR"), 4, 1, 325 },
    { TEXT("UNDERWEAR"), 4, 2, 411 },
    { TEXT("UNDERWEAR"), 4, 3, 411 },
    { TEXT("UNDERWEAR"), 4, 4, 280 },
    { TEXT("UNDERWEAR"), 4, 5, 280 },
    { TEXT("UNDERWEAR"), 4, 6, 7638 },
    { TEXT("UNDERWEAR"), 4, 7, 2642 },
    { TEXT("UNDERWEAR"), 4, 8, 280 },
    { TEXT("UNDERWEAR"), 4, 9, 280 },
    { TEXT("UNDERWEAR"), 4, 10, 1875 },
    { TEXT("UNDERWEAR"), 4, 11, 2419 },
    { TEXT("UNDERWEAR"), 5, 0, 652 },
    { TEXT("UNDERWEAR"), 5, 1, 381 },
    { TEXT("UNDERWEAR"), 5, 2, 611 },
    { TEXT("UNDERWEAR"), 5, 3, 109 },
    { TEXT("UNDERWEAR"), 5, 4, 2803 },
    { TEXT("UNDERWEAR"), 5, 5, 4628 },
    { TEXT("UNDERWEAR"), 5, 6, 75 },
    { TEXT("UNDERWEAR"), 5, 7, 75 },
    { TEXT("UNDERWEAR"), 5, 8, 843 },
    { TEXT("UNDERWEAR"), 5, 9, 895 },
    { TEXT("NUDE"), 0, 0, 240 },
    { TEXT("NUDE"), 0, 1, 240 },
    { TEXT("NUDE"), 0, 2, 240 },
    { TEXT("NUDE"), 1, 0, 1162 },
    { TEXT("NUDE"), 2, 0, 247 },
    { TEXT("NUDE"), 3, 0, 2695 },
    { TEXT("NUDE"), 3, 1, 298 },
    { TEXT("NUDE"), 3, 2, 90 },
    { TEXT("NUDE"), 3, 3, 242 },
    { TEXT("NUDE"), 3, 4, 364 },
    { TEXT("NUDE"), 3, 5, 324 },
    { TEXT("NUDE"), 3, 6, 318 },
    { TEXT("NUDE"), 3, 7, 214 },
    { TEXT("NUDE"), 3, 8, 179 },
    { TEXT("NUDE"), 3, 9, 179 },
    { TEXT("NUDE"), 3, 10, 247 },
    { TEXT("NUDE"), 3, 11, 9604 },
    { TEXT("NUDE"), 4, 0, 4504 },
    { TEXT("NUDE"), 4, 1, 325 },
    { TEXT("NUDE"), 4, 2, 411 },
    { TEXT("NUDE"), 4, 3, 411 },
    { TEXT("NUDE"), 4, 4, 280 },
    { TEXT("NUDE"), 4, 5, 280 },
    { TEXT("NUDE"), 4, 6, 7638 },
    { TEXT("NUDE"), 4, 7, 2642 },
    { TEXT("NUDE"), 4, 8, 280 },
    { TEXT("NUDE"), 4, 9, 280 },
    { TEXT("NUDE"), 4, 10, 1880 },
    { TEXT("NUDE"), 4, 11, 2419 },
    { TEXT("NUDE"), 5, 0, 652 },
    { TEXT("NUDE"), 5, 1, 381 },
    { TEXT("NUDE"), 5, 2, 611 },
    { TEXT("NUDE"), 5, 3, 109 },
    { TEXT("NUDE"), 5, 4, 75 },
    { TEXT("NUDE"), 5, 5, 75 },
    { TEXT("NUDE"), 5, 6, 843 },
    { TEXT("NUDE"), 5, 7, 895 },
};

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
