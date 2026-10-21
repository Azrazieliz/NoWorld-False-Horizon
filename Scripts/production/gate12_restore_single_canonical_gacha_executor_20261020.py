from pathlib import Path
R=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
base=Path("Source/OfflineGame")
def exact(p, replacements):
 f=R/base/p
 src=f.read_text(encoding="utf-8-sig")
 for a,b in replacements:
  n=src.count(a)
  if n!=1: raise ValueError(f"{p}: {n} matches: {a[:120]!r}")
  src=src.replace(a,b,1)
 f.write_text(src,encoding="utf-8",newline="")
 print("MODIFIED",p)
exact(Path("Public/World/OGStartingRegionPresentation.h"),[
('''    void ClearActiveGachaPresentation();

#if WITH_DEV_AUTOMATION_TESTS''','''    void ClearActiveGachaPresentation();

    // Reuse the established canonical gacha service for native Gate10 UI.
    // No second wallet, draw executor, or fictional banner.
    bool HasActiveGachaPresentation() const { return bHasActiveGachaPresentation; }
    const FOGGachaBannerDefinition& GetActiveGachaBanner() const { return ActiveGachaBanner; }
    const TArray<FOGGachaPullResult>& GetLastGachaResults() const { return LastGachaResults; }
    bool ExecuteActiveGachaDraw(int32 Count, FString& OutError);

#if WITH_DEV_AUTOMATION_TESTS'''),
])
cpp=R/base/"Private/World/OGStartingRegionPresentation.cpp"
s=cpp.read_text(encoding="utf-8-sig")
old='''    if (BoxName == FName(TEXT("OG.Gacha.Pull")) || BoxName == FName(TEXT("OG.Gacha.Pull10")))
    {'''
start=s.index(old)
end=s.index('''    // Revalidate on activation:''',start)
s=s[:start]+'''    if (BoxName == FName(TEXT("OG.Gacha.Pull")) ||
        BoxName == FName(TEXT("OG.Gacha.Pull10")))
    {
        FString Error;
        ExecuteActiveGachaDraw(
            BoxName == FName(TEXT("OG.Gacha.Pull10")) ? 10 : 1, Error);
        return;
    }

'''+s[end:]
insertion='''void AOGWorldPresentationHud::ClearActiveGachaPresentation()
{'''
if s.count(insertion)!=1:raise RuntimeError("ClearActive hook missing")
method='''bool AOGWorldPresentationHud::ExecuteActiveGachaDraw(
    int32 Count, FString& OutError)
{
    OutError.Reset();
    if ((Count != 1 && Count != 10) || !bHasActiveGachaPresentation)
    {
        OutError = TEXT("No eligible active authored banner or draw count.");
        FoundationStatusMessage = OutError;
        return false;
    }
    UGameInstance* GI = GetGameInstance();
    UOGGameCoreSubsystem* Core =
        GI ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    UOGGate12RulerSessionSubsystem* Session =
        GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (!Store || !Store->IsOpen() || !Core || !Core->IsCoreReady() ||
        !Session || !Session->IsBoundToStoredRuler())
    {
        OutError = TEXT("Summoning requires the current stored Ruler and canonical world.");
        FoundationStatusMessage = OutError;
        return false;
    }
    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < 0 || !FOGGachaService::ValidateBanner(ActiveGachaBanner, OutError))
    {
        if (OutError.IsEmpty()) OutError = TEXT("No valid World time for acquisition.");
        FoundationStatusMessage = OutError;
        return false;
    }
    FOGGachaService Gacha(*Store);
    TArray<FOGGachaPullResult> Results;
    if (!Gacha.PullBatch(ActiveGachaBanner, Session->BoundRulerId,
            Tick, GachaCommandSeed, Count, Results, OutError))
    {
        FoundationStatusMessage = OutError;
        return false;
    }
    LastGachaResults = MoveTemp(Results);
    // Preserve the RNG stream across committed batches (including SR floor).
    for (const FOGGachaPullResult& Result : LastGachaResults)
        GachaCommandSeed = static_cast<int64>(
            static_cast<uint64>(GachaCommandSeed) +
            static_cast<uint64>(FMath::Max<int64>(1, Result.RngDrawCount + 1)));
    GachaOverlay = TEXT("results");
    GachaDetailsPage = 0;
    FoundationStatusMessage = FString::Printf(
        TEXT("Summoned %d Manifestation%s."), Count,
        Count == 1 ? TEXT("") : TEXT("s"));
    // The transaction already committed. Refresh is presentation-only,
    // and a projection error must NOT imply the acquisition was undone.
    FString ProjectionError;
    if (!Session->RefreshCanonicalProjections(ProjectionError))
        UE_LOG(LogTemp, Warning, TEXT("Acquisition committed; roster refresh: %s"),
            *ProjectionError);
    return true;
}

'''
s=s.replace(insertion,method+insertion,1)
cpp.write_text(s,encoding="utf-8",newline="")
print("MODIFIED",cpp.relative_to(R))
exact(Path("Public/UI/OGGate12NativeRulerWidget.h"),[
('''    bool bGachaHistoryExpanded = false;''','''    bool bGachaHistoryExpanded = false;
    bool bGachaRatesExpanded = false;
    bool bGachaResultsExpanded = false;'''),
])
print("NO_UNREAL_BUILD")
