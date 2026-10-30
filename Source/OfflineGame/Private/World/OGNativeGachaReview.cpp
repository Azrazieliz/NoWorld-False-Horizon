#include "World/OGStartingRegionPresentation.h"
#include "Gacha/OGGachaService.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Engine/GameInstance.h"

bool AOGWorldPresentationHud::ExecuteReviewedGachaDraw(int32 Count,const FString& Token,FString& Error)
{
    Error.Reset();
    if (bReviewedGachaDrawInFlight || Token.IsEmpty() || (Count!=1&&Count!=10) || !bHasActiveGachaPresentation)
    { Error=TEXT("Review an available draw before confirming; another command may already be committing."); return false; }
    TGuardValue<bool> Guard(bReviewedGachaDrawInFlight,true);
    auto* Game=GetGameInstance();
    auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    auto* Session=Game?Game->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    auto* Store=Core?Core->GetWorldStore():nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() ||
        !Session || !Session->IsBoundToStoredRuler() || !Session->bRealGachaAccessUnlocked)
    { Error=TEXT("Acquisition requires the actual unlocked local Ruler and canonical World."); return false; }
    const auto BannerId=ActiveGachaBanner.BannerId;
    if (!SelectInstalledGachaBanner(BannerId,Error)) return false; // Revalidates actual current playable pool and schedule.
    const int64 Tick=Core->GetCanonicalWorldTick();
    if (Tick<0 || Core->GetWorldStore()!=Store || !Core->IsCoreReady())
    { Error=TEXT("The canonical acquisition lifetime changed."); return false; }
    TArray<FOGGachaPullResult> Results;
    if (!FOGGachaService(*Store).PullBatchReviewed(ActiveGachaBanner,Session->BoundRulerId,
        Tick,GachaCommandSeed,Count,Token,Results,Error)) return false;
    LastGachaResults=MoveTemp(Results);
    for (const auto& Result:LastGachaResults)
        GachaCommandSeed=static_cast<int64>(static_cast<uint64>(GachaCommandSeed)+
            static_cast<uint64>(FMath::Max<int64>(1,Result.RngDrawCount+1)));
    GachaSeedsByBanner.Add(ActiveGachaBanner.BannerId,GachaCommandSeed);
    GachaOverlay=TEXT("results"); GachaDetailsPage=0;
    FoundationStatusMessage=FString::Printf(TEXT("Summoned %d Manifestation%s."),Count,Count==1?TEXT(""):TEXT("s"));
    FString RefreshError;
    if (!Session->RefreshCanonicalProjections(RefreshError))
        UE_LOG(LogTemp,Warning,TEXT("Acquisition committed; projection refresh: %s"),*RefreshError);
    return true;
}
