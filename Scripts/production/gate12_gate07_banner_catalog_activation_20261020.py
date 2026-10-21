from pathlib import Path
R=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
h=R/"Source/OfflineGame/Public/World/OGStartingRegionPresentation.h"
w=R/"Source/OfflineGame/Private/World/OGStartingRegionPresentation.cpp"
u=R/"Source/OfflineGame/Private/UI/OGGate12NativeRulerWidget.cpp"
def change(p,swaps):
 s=p.read_text(encoding="utf-8-sig")
 for old,new in swaps:
  if s.count(old)!=1:raise ValueError(f"unexpected {s.count(old)} in {p} for {old[:120]!r}")
  s=s.replace(old,new,1)
 p.write_text(s,encoding="utf-8",newline="")
 print("MODIFIED",p.name)
change(h,[('''    bool ExecuteActiveGachaDraw(int32 Count, FString& OutError);''','''    bool ExecuteActiveGachaDraw(int32 Count, FString& OutError);
    bool LoadInstalledGachaBannerForRuler(FString& OutError);''')])
change(w,[
('''#include "Gacha/OGGachaService.h"''','''#include "Gacha/OGGachaService.h"
#include "Gacha/OGApprovedGachaBannerCatalog.h"'''),
('''void AOGWorldPresentationHud::ClearActiveGachaPresentation()
{''','''bool AOGWorldPresentationHud::LoadInstalledGachaBannerForRuler(
    FString& OutError)
{
    OutError.Reset();
    // Respect an already-activated banner supplied by the authorized
    // installed-content service; never replace it with a guessed pool.
    if (bHasActiveGachaPresentation) return true;
    UGameInstance* GI = GetGameInstance();
    UOGGameCoreSubsystem* Core =
        GI ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    UOGGate12RulerSessionSubsystem* Session =
        GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Session ||
        !Session->IsBoundToStoredRuler())
    {
        OutError=TEXT("No stored player World for live banner resolution.");
        return false;
    }
    UOGApprovedGachaBannerCatalog* Catalog =
        LoadObject<UOGApprovedGachaBannerCatalog>(nullptr,
            TEXT("/Game/NoWorld/Gacha/DA_NoWorld_ApprovedBanners."
                 "DA_NoWorld_ApprovedBanners"));
    if (!Catalog)
    {
        OutError=TEXT("No installed accepted banner catalog.");
        return false;
    }
    const int64 WorldTick=Core->GetCanonicalWorldTick();
    for (const FOGInstalledScheduledGachaBanner& Scheduled:
         Catalog->ActiveSchedules)
    {
        if (!Scheduled.bAcceptedInstalledAndActivated ||
            WorldTick < Scheduled.BeginsAtWorldTick ||
            WorldTick >= Scheduled.EndsBeforeWorldTick)
            continue;
        // Every pull is still Ruler-access, payment and Registry-gated.
        // Seed is tied to the actual World and owner, not an invented
        // Ruler account or developer wallet.
        const int64 Seed=static_cast<int64>(
            HashCombine(GetTypeHash(Session->BoundRulerId.ToString()),
                        GetTypeHash(WorldTick)));
        SetActiveGachaPresentation(Scheduled.Banner,WorldTick,Seed);
        if (bHasActiveGachaPresentation) return true;
    }
    OutError=TEXT("No currently eligible installed authored banner.");
    return false;
}

void AOGWorldPresentationHud::ClearActiveGachaPresentation()
{''')
])
change(u,[('''        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);''','''        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);
        if (Error.IsEmpty() && Index == 2 && GetOwningPlayer())
        {
            if (AOGWorldPresentationHud* Hud =
                Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()))
            {
                FString BannerError;
                Hud->LoadInstalledGachaBannerForRuler(BannerError);
                // No catalog is the legitimate empty state, not a reason to
                // block Ruler navigation or invent a featured banner.
            }
        }''')])
print("AUTHORED_SCHEDULE_CATALOG_LOAD_CONNECTED; NO_BUILD")
