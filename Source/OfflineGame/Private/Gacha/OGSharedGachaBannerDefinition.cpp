#include "Gacha/OGSharedGachaBannerDefinition.h"
#include "Gacha/OGGachaService.h"
UOGSharedGachaBannerDefinition::UOGSharedGachaBannerDefinition() { System=TEXT("gacha"); }
bool UOGSharedGachaBannerDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("gacha") || DefinitionId!=Schedule.Banner.BannerId || DisplayName.IsEmpty() ||
        Schedule.BeginsAtWorldTick<0 || Schedule.EndsBeforeWorldTick<=Schedule.BeginsAtWorldTick)
    { Error=TEXT("Shared banner requires an exact key, display name and valid canonical World schedule."); return false; }
    return FOGGachaService::ValidateBanner(Schedule.Banner,Error);
}
bool FOGInstalledGachaBannerLibrary::Collect(int64 Tick,TArray<FOGGachaBannerDefinition>& Out,FString& Error)
{
    Out.Reset(); Error.Reset();
    if (Tick<0) { Error=TEXT("Banner discovery requires actual canonical World time."); return false; }
    TArray<FOGInstalledScheduledGachaBanner> Schedules;
    TArray<UDataAsset*> Shared;
    if (auto* Library=UOGDefinitionLibrary::Get())
        Library->List(TEXT("gacha"),UOGSharedGachaBannerDefinition::StaticClass(),Shared);
    for (const auto* Asset:Shared)
    {
        const auto* Definition=CastChecked<UOGSharedGachaBannerDefinition>(Asset);
        if (!Definition->ValidateDefinition(Error)) return false;
        Schedules.Add(Definition->Schedule);
    }
    if (const auto* Legacy=LoadObject<UOGApprovedGachaBannerCatalog>(nullptr,
        TEXT("/Game/NoWorld/Gacha/DA_NoWorld_ApprovedBanners.DA_NoWorld_ApprovedBanners")))
        Schedules.Append(Legacy->ActiveSchedules);
    return FilterSchedules(Schedules,Tick,Out,Error);
}
bool FOGInstalledGachaBannerLibrary::FilterSchedules(const TArray<FOGInstalledScheduledGachaBanner>& Schedules,
    int64 Tick,TArray<FOGGachaBannerDefinition>& Out,FString& Error)
{
    Out.Reset(); Error.Reset();
    if (Tick<0) { Error=TEXT("Banner selection requires nonnegative World time."); return false; }
    TSet<FOGContentId> Ids;
    for (const auto& S:Schedules)
    {
        if (!S.bAcceptedInstalledAndActivated) continue;
        if (S.BeginsAtWorldTick<0 || S.EndsBeforeWorldTick<=S.BeginsAtWorldTick)
        { Error=TEXT("An activated banner has an invalid World schedule."); Out.Reset(); return false; }
        if (Tick<S.BeginsAtWorldTick || Tick>=S.EndsBeforeWorldTick) continue;
        if (!FOGGachaService::ValidateBanner(S.Banner,Error) || Ids.Contains(S.Banner.BannerId))
        { if (Error.IsEmpty()) Error=TEXT("Duplicate active installed banner identity."); Out.Reset(); return false; }
        Ids.Add(S.Banner.BannerId); Out.Add(S.Banner);
    }
    Out.Sort([](const auto& A,const auto& B){return A.BannerId.Value<B.BannerId.Value;});
    return true;
}
FString FOGInstalledGachaBannerLibrary::DisplayName(const FOGContentId& Id)
{
    auto* Library=UOGDefinitionLibrary::Get();
    const auto* Definition=Library?Cast<UOGSharedGachaBannerDefinition>(Library->Resolve(TEXT("gacha"),Id)):nullptr;
    return Definition && !Definition->DisplayName.IsEmpty()?Definition->DisplayName.ToString():Id.ToString();
}
