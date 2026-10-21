#include "World/OGStartingRegionPresentation.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Persistence/OGWorldStore.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Native Gate02 R1 one-instance causal ecology mutation.
// Called only after higher-level authorized harvesting/destruction resolution.
// Uses the existing canonical SQLite store, not an Actor's ephemeral lifetime.
bool AOGStartingRegionGenerator::PersistGate02SourceScatterRemoval(
    const FString& Gate02StableScatterId,
    FString& OutError)
{
    OutError.Reset();
    if(!bGate02R1RegionSource || !bR1DatasetLoaded ||
       Gate02StableScatterId.IsEmpty() || R1TilePlacements.IsEmpty())
    {
        OutError=TEXT("Gate02 R1 source and authored scatter are not initialized.");
        return false;
    }
    // Never permit a caller to create a made-up world entity or remove
    // another content namespace by guessing a UUID. Source IDs are literal
    // Gate02 authored population records and stable across all seeds.
    bool bKnown=false;
    for(const auto& Tile:R1TilePlacements)
    {
        for(const FR1Placement& P:Tile.Value)
            if(P.PersistentId==Gate02StableScatterId)
            {
                bKnown=true;
                break;
            }
        if(bKnown)break;
    }
    if(!bKnown)
    {
        OutError=TEXT("Scatter record is absent from original Gate02 R1 authored population.");
        return false;
    }
    UGameInstance* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    UOGGameCoreSubsystem* Core=GI?GI->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    IOGWorldStore* Store=Core?Core->GetWorldStore():nullptr;
    const int64 CanonicalTick=Core?Core->GetCanonicalWorldTick():-1;
    if(!Store || !Store->IsOpen() || CanonicalTick<0)
    {
        OutError=TEXT("Offline canonical store/clock unavailable; refusing unpersisted destructive World mutation.");
        return false;
    }
    const FOGEntityId StableEntityId=R1PersistentStateId(Gate02StableScatterId);
    if(!StableEntityId.IsValid())
    {
        OutError=TEXT("Gate02 source record has an invalid deterministic persistent entity ID.");
        return false;
    }
    bool bFound=false;
    FName Kind;
    FString ExistingJson;
    int64 ExistingCreatedTick=0;
    if(!Store->TryReadEntity(StableEntityId,bFound,Kind,ExistingJson,
                            ExistingCreatedTick,OutError))
        return false;
    const FName ExpectedKind(TEXT("gate02_r1_ecology_v1"));
    if(bFound && Kind!=ExpectedKind)
    {
        OutError=TEXT("Conflicting canonical entity kind: cannot reinterpret or overwrite established save.");
        return false;
    }
    if(bFound)
    {
        TSharedPtr<FJsonObject> StateJson;
        bool bAlreadyRemoved=false;
        if(!FJsonSerializer::Deserialize(
                TJsonReaderFactory<>::Create(ExistingJson),StateJson) ||
           !StateJson.IsValid() ||
           !StateJson->TryGetBoolField(TEXT("removed"),bAlreadyRemoved))
        {
            OutError=TEXT("Damaged ecology save record preserved; no silent repair or overwrite.");
            return false;
        }
        if(bAlreadyRemoved)
        {
            // Idempotent. Do not repeat rewards or rewrite canonical time.
            RefreshR1StreamedTiles(true);
            return true;
        }
    }
    if(!Store->BeginTransaction(OutError))
        return false;
    const FString RemovedJson=TEXT("{\"removed\":true}");
    const int64 OriginalCreatedTick=bFound?ExistingCreatedTick:CanonicalTick;
    const bool bWritten=Store->UpsertEntity(
        StableEntityId,ExpectedKind,OriginalCreatedTick,RemovedJson,OutError);
    if(!bWritten)
    {
        FString Ignored;
        Store->RollbackTransaction(Ignored);
        return false;
    }
    if(!Store->CommitTransaction(OutError))
    {
        FString Ignored;
        Store->RollbackTransaction(Ignored);
        return false;
    }
    // Re-stream ONLY the current tile window, preserving all other world
    // actors, the player's transform, original terrain, and unmodified assets.
    RefreshR1StreamedTiles(true);
    return true;
}

// Translate a REAL physics hit on one visible HISM ecology instance into its
// original Gate02 stable authored population record. This is a read-only
// identity resolver: permissions, tool requirements and loot are owned by
// higher-level action rules. An instance index without the matching current
// streaming component cannot mutate the canonical WorldStore.
bool AOGStartingRegionGenerator::TryResolveGate02SourceScatterHit(
    const FHitResult& Hit,FString& OutStableSourceId,
    FString& OutError) const
{
    OutStableSourceId.Reset();
    OutError.Reset();
    if(!bGate02R1RegionSource || !bR1DatasetLoaded || !GetWorld() ||
       !GetWorld()->IsGameWorld() || Hit.GetActor()!=this ||
       Hit.Item<0 || R1VisibleScatterSourceIdByType.Num()!=4)
    {
        OutError=TEXT("Interaction did not hit an active Gate02 source ecology instance.");
        return false;
    }
    const UPrimitiveComponent* Component=Hit.GetComponent();
    const UHierarchicalInstancedStaticMeshComponent* Components[]={
        RockInstances,TrunkInstances,CanopyInstances,AccentInstances};
    for(int32 Type=0;Type<4;++Type)
    {
        if(Components[Type]!=Component)continue;
        const TArray<FString>& Records=R1VisibleScatterSourceIdByType[Type];
        if(!Records.IsValidIndex(Hit.Item) || Records[Hit.Item].IsEmpty())
        {
            OutError=TEXT("The visible source instance has no stable Gate02 record.");
            return false;
        }
        const FString& Stable=Records[Hit.Item];
        // A stale HISM index cannot refer to a newly invented record:
        // verify the exact original source population, not a random GUID.
        bool bOriginal=false;
        for(const auto& Tile:R1TilePlacements)
        {
            for(const FR1Placement& Placement:Tile.Value)
                if(Placement.PersistentId==Stable && Placement.TypeIndex==Type)
                {bOriginal=true;break;}
            if(bOriginal)break;
        }
        if(!bOriginal)
        {
            OutError=TEXT("Streamed hit identity is not present in original Gate02 ecology.");
            return false;
        }
        OutStableSourceId=Stable;
        return true;
    }
    OutError=TEXT("Hit component is not one of the four real Gate02 ecology components.");
    return false;
}
