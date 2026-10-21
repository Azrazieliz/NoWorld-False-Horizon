// Persist the EXISTING authored first-World player trail and physically entered
// Gate02 R1 source tiles. No new canonical Location records or gameplay awards.
#include "World/OGStartingRegionPresentation.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Persistence/OGWorldStore.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
const FName TrailKey(TEXT("world.player_exploration.gate02_r1"));
constexpr int32 TrailCapacity=96;
constexpr int32 SourceTiles=25;
constexpr double SourceHalfExtentCm=160000.0;
constexpr double SourceTileSizeCm=12800.0;
constexpr double SaveInterval=8.0;

AOGStartingRegionGenerator* FindOriginalR1(AOGWorldPrototypeCharacter* Pawn)
{
    UWorld* World=Pawn?Pawn->GetWorld():nullptr;
    if(!World || !World->IsGameWorld())return nullptr;
    for(TActorIterator<AOGStartingRegionGenerator> It(World);It;++It)
        if(It->bGate02R1RegionSource)return *It;
    return nullptr;
}
bool ReadRealOwner(AOGWorldPrototypeCharacter* Pawn,
                   FOGEntityId& OutOwner,IOGWorldStore*& OutStore,
                   int64& OutTick)
{
    OutStore=nullptr;OutTick=-1;OutOwner=FOGEntityId();
    UGameInstance* GI=Pawn?Pawn->GetGameInstance():nullptr;
    if(!GI)return false;
    UOGGate12RulerSessionSubsystem* Session=
        GI->GetSubsystem<UOGGate12RulerSessionSubsystem>();
    UOGGameCoreSubsystem* Core=GI->GetSubsystem<UOGGameCoreSubsystem>();
    if(!Session || !Session->IsBoundToStoredRuler() || !Core ||
       !Core->IsCoreReady() || !Session->BoundRulerId.IsValid())return false;
    OutOwner=Session->BoundRulerId;
    OutStore=Core->GetWorldStore();
    OutTick=Core->GetCanonicalWorldTick();
    return OutStore && OutStore->IsOpen() && OutTick>=0;
}
bool DecodePair(const TSharedPtr<FJsonValue>& Value,double& X,double& Y)
{
    if(!Value.IsValid() || Value->Type!=EJson::Array)return false;
    const TArray<TSharedPtr<FJsonValue>>& Row=Value->AsArray();
    if(Row.Num()!=2 || !Row[0].IsValid() || !Row[1].IsValid() ||
       Row[0]->Type!=EJson::Number || Row[1]->Type!=EJson::Number)return false;
    X=Row[0]->AsNumber(); Y=Row[1]->AsNumber();
    return FMath::IsFinite(X) && FMath::IsFinite(Y);
}
TSharedPtr<FJsonValue> Pair(double X,double Y)
{
    TArray<TSharedPtr<FJsonValue>> Row;
    Row.Add(MakeShared<FJsonValueNumber>(X));
    Row.Add(MakeShared<FJsonValueNumber>(Y));
    return MakeShared<FJsonValueArray>(MoveTemp(Row));
}
}

void AOGWorldPrototypeCharacter::RestoreR1ExplorationFromWorldStore()
{
    if(bR1ExplorationLoaded || bR1ExplorationRestoreRejected || !FindOriginalR1(this))
        return;
    FOGEntityId RulerOwner;
    IOGWorldStore* Store=nullptr;
    int64 Tick=0;
    if(!ReadRealOwner(this,RulerOwner,Store,Tick))return;
    bool bFound=false;
    FOGKnowledgeFactRecord Fact;
    FString Error;
    if(!Store->TryReadKnowledgeFact(RulerOwner,TrailKey,RulerOwner,bFound,Fact,Error))
    {
        UE_LOG(LogTemp,Warning,TEXT("GATE02_R1 exploration read failed: %s"),*Error);
        return;
    }
    TArray<FVector2D> RestoredTrail;
    TSet<FIntPoint> RestoredTiles;
    if(bFound)
    {
        TSharedPtr<FJsonObject> Json;
        if(!FJsonSerializer::Deserialize(
               TJsonReaderFactory<>::Create(Fact.ValueJson),Json) ||
           !Json.IsValid())
        { bR1ExplorationRestoreRejected=true;return; }
        FString Schema;
        const TArray<TSharedPtr<FJsonValue>>* Trail=nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Tiles=nullptr;
        if(!Json->TryGetStringField(TEXT("schema"),Schema) ||
           Schema!=TEXT("gate02_r1_player_exploration_v1") ||
           !Json->TryGetArrayField(TEXT("trail_cm"),Trail) ||
           !Json->TryGetArrayField(TEXT("visited_source_tiles"),Tiles) ||
           Trail->Num()>TrailCapacity || Tiles->Num()>SourceTiles*SourceTiles)
        { bR1ExplorationRestoreRejected=true;return; }
        for(const TSharedPtr<FJsonValue>& Value:*Trail)
        {
            double X=0,Y=0;
            if(!DecodePair(Value,X,Y))
            { bR1ExplorationRestoreRejected=true;return; }
            RestoredTrail.Add(FVector2D(X,Y));
        }
        for(const TSharedPtr<FJsonValue>& Value:*Tiles)
        {
            double X=0,Y=0;
            if(!DecodePair(Value,X,Y) || X!=FMath::Floor(X) ||
               Y!=FMath::Floor(Y) || X<0 || X>=SourceTiles ||
               Y<0 || Y>=SourceTiles)
            { bR1ExplorationRestoreRejected=true;return; }
            RestoredTiles.Add(FIntPoint(static_cast<int32>(X),static_cast<int32>(Y)));
        }
    }
    VisitedMapTrail=MoveTemp(RestoredTrail);
    R1ExploredSourceTiles=MoveTemp(RestoredTiles);
    bHasMapTrailSample=!VisitedMapTrail.IsEmpty();
    if(bHasMapTrailSample)LastMapTrailSample=VisitedMapTrail.Last();
    R1ExplorationRuler=RulerOwner;
    bR1ExplorationLoaded=true;
    bR1ExplorationDirty=false;
    R1NextExplorationSaveSeconds=GetWorld()->GetTimeSeconds()+SaveInterval;
}

bool AOGWorldPrototypeCharacter::SaveR1ExplorationToWorldStore(FString& OutError)
{
    OutError.Reset();
    if(!bR1ExplorationLoaded || !bR1ExplorationDirty ||
       bR1ExplorationRestoreRejected)return false;
    FOGEntityId RulerOwner;
    IOGWorldStore* Store=nullptr;
    int64 Tick=0;
    if(!ReadRealOwner(this,RulerOwner,Store,Tick) || RulerOwner!=R1ExplorationRuler)
    {
        OutError=TEXT("Exploration owner changed or WorldStore is unavailable.");
        return false;
    }
    bool bExisting=false;
    FOGKnowledgeFactRecord Existing;
    if(!Store->TryReadKnowledgeFact(RulerOwner,TrailKey,RulerOwner,
        bExisting,Existing,OutError))return false;

    TSharedRef<FJsonObject> Json=MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("schema"),TEXT("gate02_r1_player_exploration_v1"));
    TArray<TSharedPtr<FJsonValue>> Trail;
    for(const FVector2D& P:VisitedMapTrail)
        if(FMath::IsFinite(P.X) && FMath::IsFinite(P.Y))
            Trail.Add(Pair(P.X,P.Y));
    Json->SetArrayField(TEXT("trail_cm"),MoveTemp(Trail));
    TArray<FIntPoint> Sorted=R1ExploredSourceTiles.Array();
    Sorted.Sort([](const FIntPoint& A,const FIntPoint& B)
        {return A.X==B.X?A.Y<B.Y:A.X<B.X;});
    TArray<TSharedPtr<FJsonValue>> Tiles;
    for(const FIntPoint& Tile:Sorted)Tiles.Add(Pair(Tile.X,Tile.Y));
    Json->SetArrayField(TEXT("visited_source_tiles"),MoveTemp(Tiles));
    FString Payload;
    if(!FJsonSerializer::Serialize(Json,
        TJsonWriterFactory<>::Create(&Payload)))
    {
        OutError=TEXT("Failed serializing first World exploration.");
        return false;
    }
    FOGKnowledgeFactRecord Fact;
    Fact.OwnerEntityId=RulerOwner;
    Fact.SubjectEntityId=RulerOwner; // Self-owned map journal, NOT a new Location.
    Fact.FactKey=TrailKey;
    Fact.ValueJson=MoveTemp(Payload);
    Fact.LearnedWorldTick=bExisting?Existing.LearnedWorldTick:Tick;
    Fact.UpdatedWorldTick=Tick;
    Fact.BeliefState=FName(TEXT("observed"));
    if(!Store->UpsertKnowledgeFact(Fact,OutError))return false;
    bR1ExplorationDirty=false;
    R1NextExplorationSaveSeconds=GetWorld()->GetTimeSeconds()+SaveInterval;
    return true;
}

void AOGWorldPrototypeCharacter::RecordR1ExplorationAndSaveIfDue(
    const FVector& WorldLocation)
{
    AOGStartingRegionGenerator* R1=FindOriginalR1(this);
    if(!R1 || !bR1ExplorationLoaded || bR1ExplorationRestoreRejected ||
       !IsFoundationWorldInputAllowed())return;
    FOGEntityId CurrentOwner;
    IOGWorldStore* Store=nullptr;
    int64 Tick=0;
    if(!ReadRealOwner(this,CurrentOwner,Store,Tick) ||
       CurrentOwner!=R1ExplorationRuler)return;

    const FVector Local=R1->GetActorTransform().InverseTransformPosition(
        WorldLocation);
    if(FMath::Abs(Local.X)<=SourceHalfExtentCm &&
       FMath::Abs(Local.Y)<=SourceHalfExtentCm &&
       IOGPlayableBoundaryProvider::Execute_IsInsideStableGroundRegion(
           R1,WorldLocation))
    {
        const FIntPoint Tile(
            FMath::Clamp(FMath::FloorToInt(
                (Local.X+SourceHalfExtentCm)/SourceTileSizeCm),0,SourceTiles-1),
            FMath::Clamp(FMath::FloorToInt(
                (Local.Y+SourceHalfExtentCm)/SourceTileSizeCm),0,SourceTiles-1));
        if(!R1ExploredSourceTiles.Contains(Tile))
        {
            R1ExploredSourceTiles.Add(Tile);
            bR1ExplorationDirty=true;
        }
    }
    if(bR1ExplorationDirty &&
       GetWorld()->GetTimeSeconds()>=R1NextExplorationSaveSeconds)
    {
        FString Error;
        if(!SaveR1ExplorationToWorldStore(Error) && !Error.IsEmpty())
        {
            UE_LOG(LogTemp,Warning,TEXT("GATE02_R1 exploration save deferred: %s"),
                *Error);
            R1NextExplorationSaveSeconds=GetWorld()->GetTimeSeconds()+SaveInterval;
        }
    }
}
