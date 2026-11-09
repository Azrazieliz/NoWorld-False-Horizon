#include "World/OGStartingRegionPresentation.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Persistence/OGWorldStore.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Native Gate02 R1 one-instance causal ecology mutation.
// Caller owns tool/skill/physical-authored-source verification. This method
// persists only the exact source and transactional outcomes, never guessed loot.
bool AOGStartingRegionGenerator::PersistGate02SourceScatterRemoval(
    const FString& Gate02StableScatterId,
    FString& OutError)
{
    return PersistGate02ScatterMutation(Gate02StableScatterId,
        FOGEntityId(),FOGContentId(),0,FOGEntityId(),OutError);
}

bool AOGStartingRegionGenerator::CommitAuthorizedGate02SourceHarvest(
    const FString& Gate02StableScatterId,
    const FOGEntityId& RulerId,
    const FOGContentId& AuthoredResourceId,
    int64 AuthoredAmount,
    const FOGEntityId& RequestId,
    FString& OutError)
{
    OutError.Reset();
    if(!RulerId.IsValid() || !AuthoredResourceId.IsValid() ||
       AuthoredAmount<=0 || !RequestId.IsValid())
    {
        OutError=TEXT("An authenticated ruler, authored resource amount and unique harvest request are required.");
        return false;
    }
    UGameInstance* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    const UOGGate12RulerSessionSubsystem* Session=
        GI?GI->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    if(!Session || !Session->IsBoundToStoredRuler() ||
       Session->BoundRulerId!=RulerId)
    {
        OutError=TEXT("Harvest rewards must belong to the current stored Ruler.");
        return false;
    }
    return PersistGate02ScatterMutation(Gate02StableScatterId,
        RulerId,AuthoredResourceId,AuthoredAmount,RequestId,OutError);
}

bool AOGStartingRegionGenerator::PersistGate02ScatterMutation(
    const FString& Gate02StableScatterId,
    const FOGEntityId& RulerId,
    const FOGContentId& ResourceId,
    int64 Amount,
    const FOGEntityId& RequestId,
    FString& OutError)
{
    OutError.Reset();
    if(!IsInGameThread() || !GetWorld() || !GetWorld()->IsGameWorld())
    {
        OutError=TEXT("Ecology changes require the active authoritative game world thread.");
        return false;
    }
    const bool bWithReward=RulerId.IsValid();
    if(!bGate02R1RegionSource || !bR1DatasetLoaded ||
       Gate02StableScatterId.IsEmpty() || R1TilePlacements.IsEmpty())
    {
        OutError=TEXT("Gate02 R1 source and authored scatter are not initialized.");
        return false;
    }
    // Never permit a caller to create a made-up world entity or remove
    // another content namespace by guessing a UUID. Source IDs are literal
    // Gate02 authored population records and stable across all seeds.
    if(!R1SourceTypeById.Contains(Gate02StableScatterId))
    {
        OutError=TEXT("Scatter record is absent from original Gate02 R1 authored population.");
        return false;
    }
    UGameInstance* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    UOGGameCoreSubsystem* Core=GI?GI->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    IOGWorldStore* Store=Core?Core->GetWorldStore():nullptr;
    const int64 CanonicalTick=Core?Core->GetCanonicalWorldTick():-1;
    if(!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() ||
       Store->IsWriteTransactionActive() || CanonicalTick<0)
    {
        OutError=TEXT("Offline canonical store/clock unavailable or another write transaction is active; refusing destructive World mutation.");
        return false;
    }
    const FOGEntityId StableEntityId=R1PersistentStateId(Gate02StableScatterId);
    if(!StableEntityId.IsValid())
    {
        OutError=TEXT("Gate02 source record has an invalid deterministic persistent entity ID.");
        return false;
    }
    // An exact replay of a committed harvest is a successful no-op. This
    // check precedes the consumed-source check and never grants stock twice.
    if(bWithReward)
    {
        bool bReceiptFound=false;
        FOGWorldEvent Previous;
        if(!Store->TryReadWorldEvent(RequestId,bReceiptFound,Previous,OutError))
            return false;
        if(bReceiptFound)
        {
            TSharedPtr<FJsonObject> Receipt;
            FString Source,Resource,AmountExact;
            if(Previous.EventType!=FName(TEXT("ecology.source_harvested")) ||
               Previous.PrimaryEntity!=RulerId ||
               !FJsonSerializer::Deserialize(
                    TJsonReaderFactory<>::Create(Previous.PayloadJson),Receipt) ||
               !Receipt.IsValid() ||
               !Receipt->TryGetStringField(TEXT("source_id"),Source) ||
               !Receipt->TryGetStringField(TEXT("resource_id"),Resource) ||
               !Receipt->TryGetStringField(TEXT("amount_exact"),AmountExact) ||
               Source!=Gate02StableScatterId ||
               Resource!=ResourceId.ToString() ||
               AmountExact!=LexToString(Amount))
            {
                OutError=TEXT("Harvest request ID is already committed to another action.");
                return false;
            }
            RefreshR1StreamedTiles(true);
            return true;
        }
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
            // A successful result can trigger a second loot grant upstream.
            // A consumed source is not a successful new removal.
            OutError=TEXT("Original ecology source was already consumed.");
            RefreshR1StreamedTiles(true);
            return false;
        }
    }
    // The resource tombstone must own its transaction. Never nest it inside
    // another gameplay operation: an inner commit could prematurely commit
    // unrelated inventory/reward writes or break their rollback contract.
    if(!Store->BeginTransaction(OutError))
        return false;
    // Re-read under the write transaction. The first read above is only a
    // fast preflight; another action may consume the resource before our
    // transaction begins. Never overwrite its removal or grant duplicate
    // harvest success after such a race.
    bool bCurrentFound=false;
    FName CurrentKind;
    FString CurrentJson;
    int64 CurrentCreatedTick=0;
    if(!Store->TryReadEntity(StableEntityId,bCurrentFound,CurrentKind,
                             CurrentJson,CurrentCreatedTick,OutError))
    {
        FString Ignored;
        Store->RollbackTransaction(Ignored);
        return false;
    }
    if(bCurrentFound)
    {
        TSharedPtr<FJsonObject> CurrentState;
        bool bCurrentRemoved=false;
        if(CurrentKind!=ExpectedKind ||
           !FJsonSerializer::Deserialize(
               TJsonReaderFactory<>::Create(CurrentJson),CurrentState) ||
           !CurrentState.IsValid() ||
           !CurrentState->TryGetBoolField(TEXT("removed"),bCurrentRemoved) ||
           bCurrentRemoved)
        {
            OutError=TEXT("Ecology source changed or was consumed before mutation.");
            FString Ignored;
            Store->RollbackTransaction(Ignored);
            RefreshR1StreamedTiles(true);
            return false;
        }
    }
    // Preserve all previously authored/persisted ecology fields. Replacing the
    // record with a one-field tombstone would silently discard future resource
    // depletion, regeneration and provenance state when removing a source.
    TSharedPtr<FJsonObject> RemovedState=MakeShared<FJsonObject>();
    if(bCurrentFound)
    {
        if(!FJsonSerializer::Deserialize(
               TJsonReaderFactory<>::Create(CurrentJson),RemovedState) ||
           !RemovedState.IsValid())
        {
            OutError=TEXT("Ecology state could not be preserved during removal.");
            FString Ignored;
            Store->RollbackTransaction(Ignored);
            return false;
        }
    }
    RemovedState->SetBoolField(TEXT("removed"),true);
    FString RemovedJson;
    const TSharedRef<TJsonWriter<>> Writer=TJsonWriterFactory<>::Create(&RemovedJson);
    if(!FJsonSerializer::Serialize(RemovedState.ToSharedRef(),Writer))
    {
        OutError=TEXT("Ecology removal state could not be serialized.");
        FString Ignored;
        Store->RollbackTransaction(Ignored);
        return false;
    }
    const int64 OriginalCreatedTick=bCurrentFound?CurrentCreatedTick:CanonicalTick;
    const bool bWritten=Store->UpsertEntity(
        StableEntityId,ExpectedKind,OriginalCreatedTick,RemovedJson,OutError);
    if(!bWritten)
    {
        FString Ignored;
        Store->RollbackTransaction(Ignored);
        return false;
    }
    if(bWithReward)
    {
        // Reuse the same canonical balance path as existing Gathering Dispatch.
        // The required resource identity and quantity come from the caller's
        // authored tool/action definition, never the generic mesh prototype.
        bool bBalanceFound=false;
        int64 Balance=0;
        if(!Store->TryReadResourceBalance(
                RulerId,ResourceId,bBalanceFound,Balance,OutError))
        {
            FString Ignored; Store->RollbackTransaction(Ignored);
            return false;
        }
        if(Balance<0 || Balance>MAX_int64-Amount)
        {
            OutError=TEXT("Harvest resource balance is invalid or would overflow.");
            FString Ignored; Store->RollbackTransaction(Ignored);
            return false;
        }
        if(!Store->SetResourceBalance(RulerId,ResourceId,Balance+Amount,OutError))
        {
            FString Ignored; Store->RollbackTransaction(Ignored);
            return false;
        }
        TSharedPtr<FJsonObject> Receipt=MakeShared<FJsonObject>();
        Receipt->SetStringField(TEXT("source_id"),Gate02StableScatterId);
        Receipt->SetStringField(TEXT("resource_id"),ResourceId.ToString());
        Receipt->SetStringField(TEXT("amount_exact"),LexToString(Amount));
        Receipt->SetStringField(TEXT("source_entity_id"),StableEntityId.ToString());
        FOGWorldEvent Event;
        Event.EventId=RequestId;
        Event.EventType=TEXT("ecology.source_harvested");
        Event.WorldTick=CanonicalTick;
        Event.PrimaryEntity=RulerId;
        Event.RelatedEntities.Add(StableEntityId);
        Event.bChronicleEligible=true;
        if(!FJsonSerializer::Serialize(
               Receipt.ToSharedRef(),
               TJsonWriterFactory<>::Create(&Event.PayloadJson)) ||
           !Store->AppendWorldEvent(Event,OutError))
        {
            FString Ignored; Store->RollbackTransaction(Ignored);
            return false;
        }
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
       !Hit.bBlockingHit || Hit.Item<0 || R1VisibleScatterSourceIdByType.Num()!=4)
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
        // A tile refresh can recycle the same HISM index for a different
        // authored scatter. Never resolve a cached hit solely by its index:
        // require the live component instance and its actual world placement
        // to agree with the physics contact being processed.
        if (!Components[Type]->IsRegistered() ||
            !Components[Type]->IsValidInstance(Hit.Item))
        {
            OutError=TEXT("The struck ecology instance is no longer streamed.");
            return false;
        }
        FTransform InstanceWorld;
        if (!Components[Type]->GetInstanceTransform(Hit.Item,InstanceWorld,true) ||
            !InstanceWorld.IsValid() || !Hit.ImpactPoint.IsFinite())
        {
            OutError=TEXT("The streamed ecology contact has no valid world placement.");
            return false;
        }
        const UStaticMesh* SourceMesh=Components[Type]->GetStaticMesh();
        if (!SourceMesh)
        {
            OutError=TEXT("The original ecology mesh is no longer bound.");
            return false;
        }
        // Validate the contact against the CURRENT instance's rotated and
        // scaled mesh bounds, not a generous world-space sphere. Adjacent
        // streamed resources may overlap that sphere and must not inherit
        // one another's stale contact indices.
        const FVector Scale=InstanceWorld.GetScale3D();
        if (!Scale.IsFinite() ||
            FMath::Abs(Scale.X)<KINDA_SMALL_NUMBER ||
            FMath::Abs(Scale.Y)<KINDA_SMALL_NUMBER ||
            FMath::Abs(Scale.Z)<KINDA_SMALL_NUMBER)
        {
            OutError=TEXT("The ecology instance has a degenerate world transform.");
            return false;
        }
        const FBox LocalBounds=SourceMesh->GetBounds().GetBox();
        const FVector LocalImpact=InstanceWorld.InverseTransformPosition(Hit.ImpactPoint);
        const FVector Tolerance(25.0f/FMath::Abs(Scale.X),
                                25.0f/FMath::Abs(Scale.Y),
                                25.0f/FMath::Abs(Scale.Z));
        if (!LocalImpact.IsFinite() ||
            !LocalBounds.ExpandBy(Tolerance).IsInsideOrOn(LocalImpact))
        {
            OutError=TEXT("The ecology contact belongs to a previous streamed placement.");
            return false;
        }
        const FString& Stable=Records[Hit.Item];
        // Source IDs are indexed only after the original 10,454-record
        // manifest passes duplicate/prototype integrity checking.
        // Never accept a stale HISM index after a different tile was streamed.
        const int32* SourceType=R1SourceTypeById.Find(Stable);
        if(!SourceType || *SourceType!=Type)
        {
            OutError=TEXT("Streamed hit identity is not present in original Gate02 ecology.");
            return false;
        }
        // The streamed projection is not the persistence authority: a contact
        // may have been cached before a different action removed this source.
        // Never return an already-consumed record to a harvesting caller.
        UGameInstance* GI=GetWorld()->GetGameInstance();
        UOGGameCoreSubsystem* Core=GI?GI->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
        IOGWorldStore* Store=Core?Core->GetWorldStore():nullptr;
        if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() ||
            Store->IsWriteTransactionActive())
        {
            OutError=TEXT("Canonical ecology persistence is unavailable during hit resolution.");
            return false;
        }
        const FOGEntityId PersistentId=R1PersistentStateId(Stable);
        if (!PersistentId.IsValid())
        {
            OutError=TEXT("Original ecology source has no valid persistent identity.");
            return false;
        }
        bool bFound=false;
        FName SavedKind;
        FString SavedJson;
        int64 CreatedTick=0;
        if (!Store->TryReadEntity(PersistentId,bFound,SavedKind,SavedJson,CreatedTick,OutError))
            return false;
        if (bFound)
        {
            if (SavedKind!=FName(TEXT("gate02_r1_ecology_v1")))
            {
                OutError=TEXT("Ecology source identity conflicts with persisted entity kind.");
                return false;
            }
            TSharedPtr<FJsonObject> SavedState;
            bool bRemoved=false;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(SavedJson),SavedState) ||
                !SavedState.IsValid() || !SavedState->TryGetBoolField(TEXT("removed"),bRemoved))
            {
                OutError=TEXT("Invalid persisted ecology state; refusing interaction.");
                return false;
            }
            if (bRemoved)
            {
                OutError=TEXT("This original ecology source has already been removed.");
                return false;
            }
        }
        OutStableSourceId=Stable;
        return true;
    }
    OutError=TEXT("Hit component is not one of the four real Gate02 ecology components.");
    return false;
}
