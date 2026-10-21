// Gate 02 R1 original Eastern Market. Native Unreal async source mesh streaming.
// The source map keeps all 12 original StaticMeshActors. The shipping/runtime
// duplicate map keeps the authored terrain, roads, other anchors and player
// start, and streams these SAME twelve source meshes without low-res proxies.
#include "World/OGStartingRegionPresentation.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"

namespace
{
constexpr int32 MarketSourceSectionCount=12;
const FVector MarketSourceOriginCm(33000.f,-11500.f,0.f);
constexpr float StartLoadingRangeCm=28000.f;
constexpr float StopLoadingRangeCm=38000.f;

FSoftObjectPath ExactMarketSourcePath(const int32 Section)
{
    const FString Id=FString::Printf(
        TEXT("SM_R1_EasternMarket_SourceSection_%02d"),Section);
    const FString Object=FString::Printf(
        TEXT("/Game/NoWorld/FirstWorld/R1/Anchors/Market/%s.%s"),*Id,*Id);
    return FSoftObjectPath(Object);
}
}

void AOGStartingRegionGenerator::UpdateR1MarketStreaming()
{
    if(!bGate02R1RegionSource || !bR1DatasetLoaded ||
       bR1MarketSourceAssetUnavailable || !GetWorld() ||
       !GetWorld()->IsGameWorld())return;
    const APlayerController* PC=GetWorld()->GetFirstPlayerController();
    const APawn* Pawn=PC?PC->GetPawn():nullptr;
    if(!Pawn)return;

    const FVector Local=GetActorTransform().InverseTransformPosition(
        Pawn->GetActorLocation());
    const float DistanceSq=FVector::DistSquared2D(
        Local,MarketSourceOriginCm);

    if(DistanceSq>FMath::Square(StopLoadingRangeCm))
    {
        if(R1MarketNextSection>0 || R1MarketStreamHandle.IsValid() ||
           !R1MarketSectionComponents.IsEmpty())
            UnloadR1MarketSections();
        return;
    }
    if(DistanceSq>FMath::Square(StartLoadingRangeCm) ||
       R1MarketNextSection>=MarketSourceSectionCount ||
       R1MarketStreamHandle.IsValid())
        return;

    // Serial asynchronous mesh load: never build 12 huge Gate02 meshes at
    // once on an 8 GB workstation or an Android phone. Meshes are the
    // original source exports and preserve their original materials.
    const FSoftObjectPath Source=ExactMarketSourcePath(R1MarketNextSection);
    R1MarketStreamHandle=UAssetManager::GetStreamableManager().
        RequestAsyncLoad(Source,
            FStreamableDelegate::CreateUObject(
                this,&AOGStartingRegionGenerator::OnR1MarketSectionLoaded));
    if(!R1MarketStreamHandle.IsValid())
    {
        bR1MarketSourceAssetUnavailable=true;
        UE_LOG(LogTemp,Error,
            TEXT("Gate02 authentic market section %d async request failed, no fabricated replacement"),
            R1MarketNextSection);
    }
}

void AOGStartingRegionGenerator::OnR1MarketSectionLoaded()
{
    if(!bGate02R1RegionSource || !GetWorld() ||
       !GetWorld()->IsGameWorld() ||
       R1MarketNextSection>=MarketSourceSectionCount)
        return;

    const int32 Section=R1MarketNextSection;
    UStaticMesh* Mesh=Cast<UStaticMesh>(
        ExactMarketSourcePath(Section).ResolveObject());
    if(!Mesh)
    {
        bR1MarketSourceAssetUnavailable=true;
        UE_LOG(LogTemp,Error,
            TEXT("Gate02 original Eastern Market section %d missing or wrong class; original source preserved"),
            Section);
        R1MarketStreamHandle.Reset();
        return;
    }
    UStaticMeshComponent* Component=NewObject<UStaticMeshComponent>(this);
    if(!Component)
    {
        bR1MarketSourceAssetUnavailable=true;
        UE_LOG(LogTemp,Error,TEXT("Cannot create Gate02 real market mesh component"));
        R1MarketStreamHandle.Reset();
        return;
    }
    Component->SetMobility(EComponentMobility::Static);
    Component->SetupAttachment(SceneRoot);
    Component->SetRelativeLocation(MarketSourceOriginCm);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Component->SetCastShadow(true);
    AddInstanceComponent(Component);
    Component->RegisterComponent();
    R1MarketSectionComponents.Add(Component);
    ++R1MarketNextSection;
    R1MarketStreamHandle.Reset();

    UE_LOG(LogTemp,Display,
        TEXT("GATE02_R1 streamed original Eastern Market section=%d/12 mesh=%s world-origin-m=(330,-115,0)"),
        R1MarketNextSection,*Mesh->GetPathName());
}

void AOGStartingRegionGenerator::UnloadR1MarketSections()
{
    if(R1MarketStreamHandle.IsValid())
    {
        R1MarketStreamHandle->CancelHandle();
        R1MarketStreamHandle.Reset();
    }
    for(UStaticMeshComponent* Component:R1MarketSectionComponents)
        if(Component)
            Component->DestroyComponent();
    R1MarketSectionComponents.Reset();
    R1MarketNextSection=0;
    bR1MarketSourceAssetUnavailable=false;
}
