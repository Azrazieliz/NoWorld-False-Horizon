// Real Gate02 RiverVale source-mask water surface for the existing World.
// Does not replace terrain, world identity, Ruler, saved anchors, or foliage.
#include "World/OGStartingRegionPresentation.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

void AOGStartingRegionGenerator::BuildR1SourceRiver()
{
    constexpr int32 Side=513;
    constexpr int32 Tiles=8;
    constexpr int32 CellsPerTile=64;
    constexpr float StepCm=625.f;
    constexpr float HalfExtentCm=160000.f;
    if(!bGate02R1RegionSource || !bR1DatasetLoaded || !R1RiverSurface ||
       R1RiverMask.Num()!=Side*Side || R1HeightMeters.Num()!=Side*Side)
        return;

    UMaterialInterface* Water=LoadObject<UMaterialInterface>(
        nullptr,TEXT("/Game/Foundation/Materials/M_FoundationWaterSurface.M_FoundationWaterSurface"));
    if(Water) R1RiverSurface->SetMaterial(0,Water);
    else UE_LOG(LogTemp,Warning,
        TEXT("GATE02_R1 river source geometry active but approved water material unavailable"));

    int32 WaterCells=0, Sections=0;
    for(int32 TileY=0;TileY<Tiles;++TileY)
    for(int32 TileX=0;TileX<Tiles;++TileX)
    {
        TArray<FVector> Vertices,Normals;
        TArray<int32> Triangles;
        TArray<FVector2D> UVs;
        TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;

        for(int32 Y=TileY*CellsPerTile;Y<(TileY+1)*CellsPerTile;++Y)
        for(int32 X=TileX*CellsPerTile;X<(TileX+1)*CellsPerTile;++X)
        {
            const int32 A=Y*Side+X,B=A+1,C=A+Side,D=C+1;
            const float Coverage=(
                R1RiverMask[A]+R1RiverMask[B]+R1RiverMask[C]+R1RiverMask[D])*.25f;
            if(Coverage<.5f) continue;

            const int32 Start=Vertices.Num();
            const int32 WX[4]={X,X+1,X,X+1};
            const int32 WY[4]={Y,Y,Y+1,Y+1};
            const float Alpha=FMath::Clamp((Coverage-.5f)*2.f,.15f,1.f);
            for(int32 Corner=0;Corner<4;++Corner)
            {
                const int32 Gx=WX[Corner],Gy=WY[Corner];
                const float GroundM=R1HeightAt(Gx,Gy);
                // Original RiverVale basin (-18m) and source highland tributaries:
                // local rivers follow relief; never fabricate a global flood
                // plane or move the authored river to an arbitrary world height.
                const float WaterM=FMath::Max(-17.85f,GroundM+.12f);
                Vertices.Add(FVector(-HalfExtentCm+Gx*StepCm,
                                     -HalfExtentCm+Gy*StepCm,
                                     WaterM*100.f));
                Normals.Add(FVector::UpVector);
                UVs.Add(FVector2D(Gx*StepCm/800.f,Gy*StepCm/800.f));
                Colors.Add(FLinearColor(.13f,.30f,.38f,Alpha));
                Tangents.Add(FProcMeshTangent(FVector::ForwardVector,false));
            }
            Triangles.Add(Start);
            Triangles.Add(Start+1);
            Triangles.Add(Start+2);
            Triangles.Add(Start+1);
            Triangles.Add(Start+3);
            Triangles.Add(Start+2);
            ++WaterCells;
        }
        if(Vertices.IsEmpty()) continue;
        const int32 SectionId=Sections++;
        R1RiverSurface->CreateMeshSection_LinearColor(
            SectionId,Vertices,Triangles,Normals,UVs,Colors,Tangents,false);
        // ProceduralMesh materials are indexed by section, not globally.
        // Otherwise 63 of 64 real river sections render with an unwanted
        // fallback material even though the first section looks correct.
        if(Water)R1RiverSurface->SetMaterial(SectionId,Water);
    }
    UE_LOG(LogTemp,Display,TEXT("GATE02_R1 native water: %d original mask cells %d independent render sections, no collision or fabricated water levels"),
           WaterCells,Sections);
}

bool AOGStartingRegionGenerator::QueryGate02R1SourceWaterSurface(
    const FVector& WorldPosition,float& OutWaterSurfaceWorldZCm) const
{
    OutWaterSurfaceWorldZCm=0.f;
    constexpr int32 Side=513;
    constexpr float HalfCm=160000.f;
    constexpr float StepCm=625.f;
    if(!bGate02R1RegionSource || !bR1DatasetLoaded ||
       R1RiverMask.Num()!=Side*Side ||
       R1HeightMeters.Num()!=Side*Side)return false;

    const FVector Local=GetActorTransform().InverseTransformPosition(WorldPosition);
    if(!FMath::IsFinite(Local.X) || !FMath::IsFinite(Local.Y) ||
       Local.X<-HalfCm || Local.Y<-HalfCm ||
       Local.X>HalfCm || Local.Y>HalfCm)return false;

    const float U=(Local.X+HalfCm)/StepCm;
    const float V=(Local.Y+HalfCm)/StepCm;
    const int32 X=FMath::Clamp(FMath::FloorToInt(U),0,Side-2);
    const int32 Y=FMath::Clamp(FMath::FloorToInt(V),0,Side-2);
    const float Tx=FMath::Clamp(U-static_cast<float>(X),0.f,1.f);
    const float Ty=FMath::Clamp(V-static_cast<float>(Y),0.f,1.f);
    const int32 A=Y*Side+X,B=A+1,C=A+Side,D=C+1;
    const float SourceCoverage=(
        R1RiverMask[A]+R1RiverMask[B]+R1RiverMask[C]+R1RiverMask[D])*.25f;
    if(SourceCoverage<.5f)return false;
    const auto SourceSurfaceZ=[this](int32 Xi,int32 Yi)
    {
        // Same relief-derived vertex formula as BuildR1SourceRiver.
        return FMath::Max(-17.85f,R1HeightAt(Xi,Yi)+.12f)*100.f;
    };
    const float Near=FMath::Lerp(SourceSurfaceZ(X,Y),
                                 SourceSurfaceZ(X+1,Y),Tx);
    const float Far=FMath::Lerp(SourceSurfaceZ(X,Y+1),
                                SourceSurfaceZ(X+1,Y+1),Tx);
    const float WaterLocalZ=FMath::Lerp(Near,Far,Ty);
    OutWaterSurfaceWorldZCm=GetActorTransform().TransformPosition(
        FVector(Local.X,Local.Y,WaterLocalZ)).Z;
    return FMath::IsFinite(OutWaterSurfaceWorldZCm);
}
