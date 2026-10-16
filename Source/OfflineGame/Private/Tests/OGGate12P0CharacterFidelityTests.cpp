#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Animation/Skeleton.h"
#include "Rendering/SkeletalMeshRenderData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12P0SkeletalSourceScaleRegressionTest,
    "OfflineGame.Gate12.P0.LuminousImportedBodyLOD0AndNativeScale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12P0SkeletalSourceScaleRegressionTest::RunTest(const FString&)
{
    bool bPass = true;
    // Source evidence: three immutable gate12_fresh_export_20261011 GLBs,
    // mesh 03 primitive 11, 9,604 positions and 128.142894cm Y-up span.
    // This only verifies a representative skeletal part, NOT entire art fidelity.
    constexpr float SourceBodyHeightCm = 128.1428947f;
    constexpr float SourceBodyWidthCm = 123.829979f;
    constexpr int32 SourceBodyVertices = 9604;
    constexpr int32 EffectivePositionMorphs = 15;
    const TCHAR* States[] = {
        TEXT("CLOTHED"), TEXT("UNDERWEAR"), TEXT("NUDE")
    };

    for (const TCHAR* State : States)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Experimental/LuminousValentineV3/%s/Parts/")
            TEXT("Mesh003Prim011/Mesh_03_Prim_11/SkeletalMeshes/Mesh_03_Prim_11"),
            State);
        USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Path);
        if (!TestNotNull(
                *FString::Printf(TEXT("%s: real imported skeletal body exists"), State),
                Mesh))
        {
            bPass = false;
            continue;
        }

        const FBoxSphereBounds Imported = Mesh->GetImportedBounds();
        const float HeightCm = Imported.BoxExtent.Z * 2.f;
        const float WidthCm = Imported.BoxExtent.X * 2.f;
        bPass &= TestTrue(
            *FString::Printf(TEXT("%s: source body height survives centimeter import"), State),
            FMath::Abs(HeightCm - SourceBodyHeightCm) < 0.35f);
        bPass &= TestTrue(
            *FString::Printf(TEXT("%s: source body width survives centimeter import"), State),
            FMath::Abs(WidthCm - SourceBodyWidthCm) < 0.35f);
        bPass &= TestNotNull(
            *FString::Printf(TEXT("%s: skeleton is assigned"), State),
            Mesh->GetSkeleton());

        const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
        bPass &= TestTrue(
            *FString::Printf(TEXT("%s: real material slot assigned"), State),
            !Materials.IsEmpty() && Materials[0].MaterialInterface != nullptr);

        const FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
        if (RenderData && !RenderData->LODRenderData.IsEmpty())
        {
            const FSkeletalMeshLODRenderData& LOD = RenderData->LODRenderData[0];
            const int32 ActualVertices = LOD.GetNumVertices();
            int32 ActualTriangles = 0;
            for (const FSkelMeshRenderSection& Section : LOD.RenderSections)
                ActualTriangles += Section.NumTriangles;
            AddInfo(FString::Printf(
                TEXT("P0_IMPORTED_%s: UE_LOD0_VERTICES=%d UE_LOD0_TRIANGLES=%d ")
                TEXT("SOURCE_PRIMITIVE_VERTICES=%d BOUNDS_WIDTH_CM=%.4f ")
                TEXT("BOUNDS_HEIGHT_CM=%.4f"),
                State, ActualVertices, ActualTriangles, SourceBodyVertices,
                WidthCm, HeightCm));
            bPass &= TestTrue(
                *FString::Printf(TEXT("%s: LOD0 retains substantial source vertex density"), State),
                ActualVertices >= SourceBodyVertices * 0.85f);
            bPass &= TestTrue(
                *FString::Printf(TEXT("%s: LOD0 contains real triangulated geometry"), State),
                ActualTriangles >= 5000);
            bPass &= TestEqual(
                *FString::Printf(TEXT("%s: no imported secondary auto-degraded LOD"), State),
                RenderData->LODRenderData.Num(), 1);
        }
        else
        {
            AddWarning(FString::Printf(
                TEXT("P0_%s render LOD data unavailable in NullRHI; ")
                TEXT("cannot claim imported vertex/triangle acceptance."), State));
        }

        const int32 MorphCount = Mesh->GetMorphTargets().Num();
        AddInfo(FString::Printf(
            TEXT("P0_MORPHS_%s: SOURCE_EFFECTIVE_POSITION=%d UE_IMPORTED=%d"),
            State, EffectivePositionMorphs, MorphCount));
        if (MorphCount < EffectivePositionMorphs)
        {
            AddWarning(FString::Printf(
                TEXT("P0_%s: %d nonzero source position morphs vs %d imported; ")
                TEXT("UE 0.015cm morph threshold is under investigation. ")
                TEXT("DO NOT accept full facial/anatomy fidelity yet."),
                State, EffectivePositionMorphs, MorphCount));
        }

        const FString TexturePath = FString::Printf(
            TEXT("/Game/Experimental/LuminousValentineV3/%s/Parts/")
            TEXT("Mesh003Prim011/Mesh_03_Prim_11/Textures/cf_m_body_light"),
            State);
        UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
        if (TestNotNull(
                *FString::Printf(TEXT("%s: real source body texture imported"), State), Texture))
        {
            bPass &= TestEqual(
                *FString::Printf(TEXT("%s: 4K texture width retained"), State),
                Texture->Source.GetSizeX(), static_cast<int64>(4096));
            bPass &= TestEqual(
                *FString::Printf(TEXT("%s: 4K texture height retained"), State),
                Texture->Source.GetSizeY(), static_cast<int64>(4096));
        }
        else
        {
            bPass = false;
        }
    }
    return bPass;
}
#endif
