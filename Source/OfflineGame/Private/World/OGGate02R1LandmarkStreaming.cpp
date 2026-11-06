#include "World/OGStartingRegionPresentation.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
struct FR1AuthoredLandmark
{
    const TCHAR* AssetName;
    FVector PlacementCm;
    FVector StreamCenterCm;
};

// Exact saved SourceAssembly transforms, with proximity measured at the
// source registry's local reference centers. The Citadel already contains
// world-space vertices. The farmland and hamlet use their original GLB origins.
const FR1AuthoredLandmark OriginalR1Landmarks[] = {
    { TEXT("SM_R1_RoyalCitadel_SmoothedTriangulated"), FVector::ZeroVector, FVector(-1300, 7000, 0) },
    { TEXT("SM_R1_SouthernFarmland"), FVector(21100, -34700, 0), FVector(21100, -34700, 0) },
    { TEXT("SM_R1_WatchwoodHamlet"), FVector(53000, 26500, 0), FVector(53000, 26500, 0) }
};
constexpr double R1LandmarkLoadRangeCm = 28000;
constexpr double R1LandmarkUnloadRangeCm = 38000;

FSoftObjectPath OriginalR1LandmarkPath(int32 Index)
{
    const TCHAR* Name = OriginalR1Landmarks[Index].AssetName;
    return FSoftObjectPath(FString::Printf(
        TEXT("/Game/NoWorld/FirstWorld/R1/Anchors/%s.%s"), Name, Name));
}
}

void AOGStartingRegionGenerator::UpdateR1LandmarkStreaming()
{
    UWorld* World = GetWorld();
    if (!bR1DatasetLoaded || !World || !World->IsGameWorld() ||
        !World->GetMapName().EndsWith(TEXT("StartingWorld")))
    {
        return; // Authored assembly maps place their own source actors.
    }

    const APlayerController* Player = World->GetFirstPlayerController();
    const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    if (!Pawn)
    {
        return;
    }

    R1Landmarks.SetNum(UE_ARRAY_COUNT(OriginalR1Landmarks));
    const FVector LocalPosition = GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
    int32 NearestIndex = INDEX_NONE;
    double NearestDistanceSq = FMath::Square(R1LandmarkLoadRangeCm);
    bool bLoadInProgress = false;
    for (int32 Index = 0; Index < R1Landmarks.Num(); ++Index)
    {
        FR1LandmarkStreamState& State = R1Landmarks[Index];
        const double DistanceSq = FVector::DistSquared2D(
            LocalPosition, OriginalR1Landmarks[Index].StreamCenterCm);
        if (DistanceSq > FMath::Square(R1LandmarkUnloadRangeCm))
        {
            if (State.bRequested)
            {
                UnloadR1Landmark(Index);
            }
            continue;
        }
        bLoadInProgress |= State.Handle.IsValid();
        if (!State.bRequested && DistanceSq <= NearestDistanceSq)
        {
            NearestDistanceSq = DistanceSq;
            NearestIndex = Index;
        }
    }
    if (bLoadInProgress || NearestIndex == INDEX_NONE)
    {
        return;
    }

    FR1LandmarkStreamState& State = R1Landmarks[NearestIndex];
    State.bRequested = true;
    const int64 Generation = ++R1LandmarkGeneration;
    State.Generation = Generation;
    TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        OriginalR1LandmarkPath(NearestIndex), FStreamableDelegate::CreateUObject(
            this, &AOGStartingRegionGenerator::OnR1LandmarkLoaded, NearestIndex, Generation));
    // A resident asset can complete inside RequestAsyncLoad.
    if (R1Landmarks.IsValidIndex(NearestIndex) && R1Landmarks[NearestIndex].Generation == Generation)
    {
        R1Landmarks[NearestIndex].Handle = MoveTemp(Handle);
        if (!R1Landmarks[NearestIndex].Handle)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not request original R1 landmark %s"),
                OriginalR1Landmarks[NearestIndex].AssetName);
        }
    }
}

void AOGStartingRegionGenerator::OnR1LandmarkLoaded(int32 Index, int64 Generation)
{
    if (!R1Landmarks.IsValidIndex(Index) || R1Landmarks[Index].Generation != Generation ||
        !bR1DatasetLoaded || !SceneRoot)
    {
        return;
    }

    FR1LandmarkStreamState& State = R1Landmarks[Index];
    State.Generation = ++R1LandmarkGeneration;
    const APlayerController* Player = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    if (!Pawn || FVector::DistSquared2D(
            GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation()),
            OriginalR1Landmarks[Index].StreamCenterCm) > FMath::Square(R1LandmarkUnloadRangeCm))
    {
        UnloadR1Landmark(Index);
        return;
    }

    UStaticMesh* Mesh = Cast<UStaticMesh>(OriginalR1LandmarkPath(Index).ResolveObject());
    if (!Mesh)
    {
        State.Handle.Reset();
        UE_LOG(LogTemp, Error, TEXT("Original R1 landmark is missing: %s"),
            OriginalR1Landmarks[Index].AssetName);
        return;
    }

    UStaticMeshComponent* Landmark = NewObject<UStaticMeshComponent>(this);
    Landmark->SetMobility(EComponentMobility::Static);
    Landmark->SetupAttachment(SceneRoot);
    Landmark->SetRelativeLocation(OriginalR1Landmarks[Index].PlacementCm);
    Landmark->SetStaticMesh(Mesh);
    Landmark->SetCollisionProfileName(TEXT("BlockAll"));
    AddInstanceComponent(Landmark);
    Landmark->RegisterComponent();
    State.Component = Landmark;
    State.Handle.Reset();
}

void AOGStartingRegionGenerator::UnloadR1Landmark(int32 Index)
{
    FR1LandmarkStreamState& State = R1Landmarks[Index];
    State.Generation = ++R1LandmarkGeneration;
    if (State.Handle.IsValid())
    {
        State.Handle->CancelHandle();
        State.Handle.Reset();
    }
    if (UStaticMeshComponent* Component = State.Component.Get())
    {
        Component->DestroyComponent();
    }
    State.Component.Reset();
    State.bRequested = false;
}

void AOGStartingRegionGenerator::UnloadR1Landmarks()
{
    for (int32 Index = 0; Index < R1Landmarks.Num(); ++Index)
    {
        UnloadR1Landmark(Index);
    }
    R1Landmarks.Reset();
}
