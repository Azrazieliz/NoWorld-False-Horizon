#include "World/OGStartingRegionPresentation.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

namespace
{
TArray<FSoftObjectPath> GetR1OriginalRoadPaths()
{
    // The exact regional_roads.fbx imports used by the Gate12 source-level
    // authoring tool. Their vertices already contain their World coordinates.
    static const TCHAR* const Names[] = {
        TEXT("ROAD___Eastern_caravan_road"),
        TEXT("ROAD___Market_north_street"),
        TEXT("ROAD___Royal_pilgrimage___east_spine"),
        TEXT("ROAD___South_rural_return"),
        TEXT("ROAD___Southern_mill_and_agriculture_road"),
        TEXT("ROAD___Village_woodward_pathway")
    };
    TArray<FSoftObjectPath> Paths;
    Paths.Reserve(UE_ARRAY_COUNT(Names));
    for (const TCHAR* Name : Names)
    {
        Paths.Emplace(FString::Printf(
            TEXT("/Game/Experimental/FirstWorldR1/Roads/%s.%s"), Name, Name));
    }
    return Paths;
}
}

void AOGStartingRegionGenerator::RequestR1SourceRoads()
{
    // Authored assembly maps already place these exact meshes. Only the
    // playable carrier needs this binding.
    if (!bR1DatasetLoaded || !GetWorld() || !GetWorld()->IsGameWorld() ||
        !GetWorld()->GetMapName().EndsWith(TEXT("StartingWorld")))
    {
        return;
    }

    const int32 Generation = ++R1RoadLoadGeneration;
    TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        GetR1OriginalRoadPaths(),
        FStreamableDelegate::CreateUObject(this, &AOGStartingRegionGenerator::AttachR1SourceRoads, Generation));
    if (R1RoadLoadGeneration == Generation)
    {
        R1RoadStreamHandle = MoveTemp(Handle);
    }
}

void AOGStartingRegionGenerator::AttachR1SourceRoads(int32 Generation)
{
    if (Generation != R1RoadLoadGeneration || !bR1DatasetLoaded || !SceneRoot)
    {
        return;
    }

    // Also invalidates the caller's pending handle assignment if the stream
    // delegate ran inline against assets already resident in memory.
    ++R1RoadLoadGeneration;
    for (const FSoftObjectPath& Path : GetR1OriginalRoadPaths())
    {
        UStaticMesh* Mesh = Cast<UStaticMesh>(Path.ResolveObject());
        if (!Mesh)
        {
            UE_LOG(LogTemp, Error, TEXT("Missing original Gate02 R1 road: %s"), *Path.ToString());
            continue;
        }

        UStaticMeshComponent* Road = NewObject<UStaticMeshComponent>(this);
        Road->SetMobility(EComponentMobility::Static);
        Road->SetupAttachment(SceneRoot);
        Road->SetRelativeTransform(FTransform::Identity);
        Road->SetStaticMesh(Mesh);
        Road->SetCollisionProfileName(TEXT("BlockAll"));
        AddInstanceComponent(Road);
        Road->RegisterComponent();
        R1SourceRoadComponents.Add(Road);
    }
    R1RoadStreamHandle.Reset();
}

void AOGStartingRegionGenerator::UnloadR1SourceRoads()
{
    ++R1RoadLoadGeneration;
    if (R1RoadStreamHandle.IsValid())
    {
        R1RoadStreamHandle->CancelHandle();
        R1RoadStreamHandle.Reset();
    }
    for (UStaticMeshComponent* Road : R1SourceRoadComponents)
    {
        if (IsValid(Road))
        {
            Road->DestroyComponent();
        }
    }
    R1SourceRoadComponents.Reset();
}
