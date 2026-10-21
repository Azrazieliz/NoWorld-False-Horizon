// Gate02 R1 genuine source integration into the EXISTING native starting
// region generator. Uses source metric heights, stable authored scatter IDs,
// and independently streamed 128m ecology tiles; NO fake NPCs or settlements.
#include "World/OGStartingRegionPresentation.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Persistence/OGWorldStore.h"
#include "Engine/GameInstance.h"

namespace
{
constexpr int32 R1Count=513;
constexpr int32 R1TileGrid=25;
constexpr float R1PointSpacingCm=625.f; // 3200m / 512 cells
constexpr float R1HalfExtentCm=160000.f;
constexpr float R1EcologyTileCm=12800.f;
FString R1SourcePath(const TCHAR* Name)
{
    return FPaths::Combine(FPaths::ProjectContentDir(),
        TEXT("NoWorld/FirstWorld/Source"),Name);
}
bool LoadR1FloatGrid(const TCHAR* Name,TArray<float>& Out)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes,*R1SourcePath(Name)) ||
        Bytes.Num()!=R1Count*R1Count*static_cast<int32>(sizeof(float)))
        return false;
    Out.SetNumUninitialized(R1Count*R1Count);
    FMemory::Memcpy(Out.GetData(),Bytes.GetData(),Bytes.Num());
    for (const float Value:Out)
        if (!FMath::IsFinite(Value)) {Out.Reset();return false;}
    return true;
}
int32 SourceType(const FString& Prototype)
{
    if (Prototype==TEXT("modules/moss_rock.glb")) return 0;
    if (Prototype==TEXT("modules/fir_sapling.glb")) return 1;
    if (Prototype==TEXT("modules/oak_tree.glb")) return 2;
    if (Prototype==TEXT("modules/shrub.glb")) return 3;
    return INDEX_NONE;
}
UStaticMesh* R1ActualModule(int32 Type)
{
    static const TCHAR* Paths[]={
        // Existing verified original Gate02 moss_rock GLB imported mesh.
        TEXT("/Game/Experimental/FirstWorldR1/MossRock/moss_rock/StaticMeshes/rock_moss_set_02_rock07_001.rock_moss_set_02_rock07_001"),
        // Joined, real GLB source meshes can be imported by the independent
        // source-grade module pipeline. Never use engine cubes/cones instead.
        TEXT("/Game/NoWorld/FirstWorld/R1/Modules/SM_R1_FirSource.SM_R1_FirSource"),
        TEXT("/Game/NoWorld/FirstWorld/R1/Modules/SM_R1_OakSource.SM_R1_OakSource"),
        TEXT("/Game/NoWorld/FirstWorld/R1/Modules/SM_R1_ShrubSource.SM_R1_ShrubSource")
    };
    if (Type<0 || Type>=4) return nullptr;
    return LoadObject<UStaticMesh>(nullptr,Paths[Type]);
}
FOGEntityId ResolveR1PersistentIdFromOriginalSource(const FString& SourceId)
{
    // Namespaced deterministic 128-bit UUID; never share the historical
    // Registry's IDs or create a random World entity on every stream cycle.
    const FString Stable=TEXT("gate02.r1.scatter:")+SourceId;
    const FString Hex=FMD5::HashAnsiString(*Stable);
    FGuid Parsed;
    if (FGuid::ParseExact(Hex,EGuidFormats::Digits,Parsed))
        return FOGEntityId(Parsed);
    return FOGEntityId();
}
}

FOGEntityId AOGStartingRegionGenerator::R1PersistentStateId(
    const FString& SourceId)
{
    return ResolveR1PersistentIdFromOriginalSource(SourceId);
}

float AOGStartingRegionGenerator::R1HeightAt(int32 X,int32 Y) const
{
    if (R1HeightMeters.Num()!=R1Count*R1Count) return 0.f;
    return R1HeightMeters[FMath::Clamp(Y,0,512)*513+FMath::Clamp(X,0,512)];
}

bool AOGStartingRegionGenerator::LoadR1MetricData()
{
    bR1DatasetLoaded=false;
    R1HeightMeters.Reset();R1SoilMask.Reset();R1RockMask.Reset();R1RiverMask.Reset();
    if (!LoadR1FloatGrid(TEXT("height_513_float32.raw"),R1HeightMeters) ||
        !LoadR1FloatGrid(TEXT("soil_mask_513_float32.raw"),R1SoilMask) ||
        !LoadR1FloatGrid(TEXT("rock_mask_513_float32.raw"),R1RockMask) ||
        !LoadR1FloatGrid(TEXT("river_mask_513_float32.raw"),R1RiverMask))
    {
        UE_LOG(LogTemp,Error,TEXT("Gate02 source metric RAW missing/invalid; refuses fixture substitute"));
        return false;
    }
    for (const float H:R1HeightMeters)
        if (H<-21.f || H>751.f)
        {
            UE_LOG(LogTemp,Error,TEXT("Gate02 source height out of authored -20..750m range"));
            R1HeightMeters.Reset();
            return false;
        }
    bR1DatasetLoaded=true;
    return true;
}

void AOGStartingRegionGenerator::BuildR1SourceTerrain()
{
    if (!bR1DatasetLoaded || !Terrain) return;
    // 64 independently collidable 400x400m terrain mesh sections. No huge
    // singular static mesh, and exactly the source's 6.25m grid and relief.
    int32 SectionIndex=0;
    for (int32 TileY=0;TileY<8;++TileY)
    for (int32 TileX=0;TileX<8;++TileX)
    {
        TArray<FVector> V,N;
        TArray<FVector2D> UV;
        TArray<FLinearColor> C;
        TArray<FProcMeshTangent> Tan;
        TArray<int32> Indices;
        V.Reserve(65*65);N.Reserve(65*65);UV.Reserve(65*65);
        C.Reserve(65*65);Tan.Reserve(65*65);Indices.Reserve(64*64*6);
        for(int32 Y=0;Y<=64;++Y)
        for(int32 X=0;X<=64;++X)
        {
            const int32 GX=TileX*64+X, GY=TileY*64+Y;
            const float Height=R1HeightAt(GX,GY);
            const float DX=(R1HeightAt(GX+1,GY)-R1HeightAt(GX-1,GY))*100.f/(2.f*R1PointSpacingCm);
            const float DY=(R1HeightAt(GX,GY+1)-R1HeightAt(GX,GY-1))*100.f/(2.f*R1PointSpacingCm);
            const float Soil=FMath::Clamp(R1SoilMask[GY*513+GX],0.f,1.f);
            const float Rock=FMath::Clamp(R1RockMask[GY*513+GX],0.f,1.f);
            V.Add(FVector(-R1HalfExtentCm+GX*R1PointSpacingCm,
                -R1HalfExtentCm+GY*R1PointSpacingCm,Height*100.f));
            N.Add(FVector(-DX,-DY,1.f).GetSafeNormal());
            UV.Add(FVector2D(GX/512.f,GY/512.f));
            // Source biome mask drives vertex tint; real artist materials
            // may override this via approved layer material, not grey boxes.
            C.Add(FLinearColor(.19f+.16f*Soil+.17f*Rock,
                .24f+.20f*Soil-.10f*Rock,
                .15f+.12f*Soil+.11f*Rock,1.f));
            Tan.Add(FProcMeshTangent(FVector(1.f,0.f,DX),false));
        }
        for(int32 Y=0;Y<64;++Y)
        for(int32 X=0;X<64;++X)
        {
            const int32 A=Y*65+X,B=A+1,C0=A+65,D=C0+1;
            Indices.Add(A);Indices.Add(B);Indices.Add(C0);
            Indices.Add(B);Indices.Add(D);Indices.Add(C0);
        }
        Terrain->CreateMeshSection_LinearColor(SectionIndex++,
            V,Indices,N,UV,C,Tan,true);
    }
    UE_LOG(LogTemp,Display,TEXT("GATE02_R1 source terrain: 513 grid, 64 collision sections, 3200m"));
}

void AOGStartingRegionGenerator::LoadR1SourceScatter()
{
    R1TilePlacements.Reset();
    FString Source;
    if (!FFileHelper::LoadFileToString(Source,
        *R1SourcePath(TEXT("baked_seed1412_scatter.jsonl"))))
    {
        UE_LOG(LogTemp,Error,TEXT("Original Gate02 seed1412 source scatter missing"));
        bR1DatasetLoaded=false;
        return;
    }
    TArray<FString> Rows;
    Source.ParseIntoArrayLines(Rows,true);
    for(const FString& Row:Rows)
    {
        TSharedPtr<FJsonObject> J;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Row),J) ||
            !J.IsValid()) { bR1DatasetLoaded=false;return; }
        FR1Placement P;
        P.PersistentId=J->GetStringField(TEXT("persistent_id"));
        P.TypeIndex=SourceType(J->GetStringField(TEXT("prototype")));
        const TArray<TSharedPtr<FJsonValue>>& XYZ=J->GetArrayField(TEXT("position_m"));
        if(P.PersistentId.IsEmpty() || P.TypeIndex==INDEX_NONE || XYZ.Num()!=3)
        { bR1DatasetLoaded=false;return; }
        P.PositionCm=FVector(
            XYZ[0]->AsNumber()*100.,XYZ[1]->AsNumber()*100.,
            XYZ[2]->AsNumber()*100.);
        P.YawDegrees=static_cast<float>(J->GetNumberField(TEXT("yaw_degrees")));
        P.UniformScale=static_cast<float>(J->GetNumberField(TEXT("uniform_scale")));
        if(!FMath::IsFinite(P.UniformScale)||P.UniformScale<=0.f)
        {bR1DatasetLoaded=false;return;}
        P.TileIndex=FIntPoint(
            FMath::Clamp(FMath::FloorToInt((P.PositionCm.X+R1HalfExtentCm)/R1EcologyTileCm),0,24),
            FMath::Clamp(FMath::FloorToInt((P.PositionCm.Y+R1HalfExtentCm)/R1EcologyTileCm),0,24));
        R1TilePlacements.FindOrAdd(P.TileIndex).Add(MoveTemp(P));
    }
    if(Rows.Num()!=10454 || R1TilePlacements.Num()>R1TileGrid*R1TileGrid)
    {
        UE_LOG(LogTemp,Error,TEXT("Gate02 R1 scatter signature count wrong: %d"),Rows.Num());
        bR1DatasetLoaded=false;
        return;
    }
    // Original 10,454 IDs remain on source placements, not rerolled from
    // player coordinates, tick or global game RNG.
    UE_LOG(LogTemp,Display,TEXT("GATE02_R1 ecology: %d authored stable placements, %d tiles"),
        Rows.Num(),R1TilePlacements.Num());
}

void AOGStartingRegionGenerator::RefreshR1StreamedTiles(bool bForce)
{
    if (!bR1DatasetLoaded || !GetWorld()) return;
    FVector Pos=FVector::ZeroVector;
    if (const APlayerController* PC=GetWorld()->GetFirstPlayerController())
        if (const APawn* Pawn=PC->GetPawn()) Pos=Pawn->GetActorLocation();
    const FVector Local=GetActorTransform().InverseTransformPosition(Pos);
    const FIntPoint Center(
        FMath::Clamp(FMath::FloorToInt((Local.X+R1HalfExtentCm)/R1EcologyTileCm),0,24),
        FMath::Clamp(FMath::FloorToInt((Local.Y+R1HalfExtentCm)/R1EcologyTileCm),0,24));
    if (!bForce && Center==LastR1StreamTile) return;
    LastR1StreamTile=Center;
    UHierarchicalInstancedStaticMeshComponent* Targets[]={
        RockInstances,TrunkInstances,CanopyInstances,AccentInstances};
    R1VisibleScatterSourceIdByType.Reset();
    R1VisibleScatterSourceIdByType.SetNum(4);
    for(int32 T=0;T<4;++T)
    {
        if (!Targets[T])continue;
        Targets[T]->ClearInstances();
        Targets[T]->SetStaticMesh(R1ActualModule(T));
    }
    IOGWorldStore* Store=nullptr;
    if(UGameInstance* GI=GetGameInstance())
        if(UOGGameCoreSubsystem* Core=GI->GetSubsystem<UOGGameCoreSubsystem>())
            Store=Core->GetWorldStore();
    int32 Count=0;
    for(int32 Y=FMath::Max(0,Center.Y-2);Y<=FMath::Min(24,Center.Y+2);++Y)
    for(int32 X=FMath::Max(0,Center.X-2);X<=FMath::Min(24,Center.X+2);++X)
    {
        const TArray<FR1Placement>* Tile=R1TilePlacements.Find(FIntPoint(X,Y));
        if(!Tile)continue;
        for(const FR1Placement& P:*Tile)
        {
            if(!Targets[P.TypeIndex] || !Targets[P.TypeIndex]->GetStaticMesh())continue;
            bool bRemoved=false;
            if(Store && Store->IsOpen())
            {
                const FOGEntityId PersistentId=R1PersistentStateId(P.PersistentId);
                bool bFound=false;FName Kind;FString State;int64 Revision=0;FString Error;
                if(PersistentId.IsValid() &&
                    Store->TryReadEntity(PersistentId,bFound,Kind,State,Revision,Error) &&
                    bFound && Kind==FName(TEXT("gate02_r1_ecology_v1")))
                {
                    // Persistence content is JSON, not a whitespace-dependent
                    // substring: {"removed": true} and {"removed":true}
                    // must have identical state after offline reload.
                    // A corrupt persisted ecology record must not quietly
                    // respawn a tree or rock whose removal may be canonical.
                    bRemoved=true;
                    TSharedPtr<FJsonObject> PersistentState;
                    bool bCanonicalRemoved=false;
                    if(FJsonSerializer::Deserialize(
                           TJsonReaderFactory<>::Create(State),PersistentState) &&
                       PersistentState.IsValid() &&
                       PersistentState->TryGetBoolField(
                           TEXT("removed"),bCanonicalRemoved))
                        bRemoved=bCanonicalRemoved;
                }
            }
            if(bRemoved)continue;
            const FTransform LocalTransform(
                FRotator(0.f,P.YawDegrees,0.f),P.PositionCm,
                FVector(P.UniformScale));
            const int32 PlacedIndex=Targets[P.TypeIndex]->AddInstance(LocalTransform);
            if(PlacedIndex!=INDEX_NONE)
            {
                TArray<FString>& IndexedSource=R1VisibleScatterSourceIdByType[P.TypeIndex];
                if(IndexedSource.Num()<=PlacedIndex)
                    IndexedSource.SetNum(PlacedIndex+1);
                IndexedSource[PlacedIndex]=P.PersistentId;
                ++Count;
            }
        }
    }
    UE_LOG(LogTemp,Verbose,TEXT("Gate02 R1 streamed tile %d,%d instances=%d"),
        Center.X,Center.Y,Count);
}

void AOGStartingRegionGenerator::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(bGate02R1RegionSource)
    {
        RefreshR1StreamedTiles(false);
        UpdateR1MarketStreaming();
    }
}
