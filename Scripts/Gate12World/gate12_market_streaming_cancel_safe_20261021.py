from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=root/"Public/World/OGStartingRegionPresentation.h"
c=root/"Private/World/OGGate02R1MarketSourceStreaming.cpp"
def patch(path,pairs):
 s=path.read_text(encoding="utf-8-sig")
 for a,b in pairs:
  n=s.count(a)
  if n!=1:raise RuntimeError(f"{path.name} expected 1, found {n} {a[:100]!r}")
  s=s.replace(a,b,1)
 path.write_text(s,encoding="utf8",newline="")
 print("UPDATED",path.name)
patch(h,[
 ("""    void OnR1MarketSectionLoaded();""",
 """    void OnR1MarketSectionLoaded(int32 RequestGeneration,int32 RequestedSection);"""),
 ("""    int32 R1MarketNextSection = 0;
    bool bR1MarketSourceAssetUnavailable = false;""",
 """    int32 R1MarketNextSection = 0;
    // Increment on every new request and unload. Late async callbacks are
    // discarded rather than attaching source meshes after leaving the area.
    int32 R1MarketRequestGeneration = 0;
    bool bR1MarketSourceAssetUnavailable = false;""")
])
patch(c,[
 ("""    const FSoftObjectPath Source=ExactMarketSourcePath(R1MarketNextSection);
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
    }""",
 """    const int32 Section=R1MarketNextSection;
    const int32 Generation=++R1MarketRequestGeneration;
    const FSoftObjectPath Source=ExactMarketSourcePath(Section);
    // RequestAsyncLoad may complete inline when a mesh is already cached.
    // Never overwrite a successful inline callback's Reset() with a stale
    // completed handle that would block loading the next source section.
    TSharedPtr<FStreamableHandle> NewHandle=
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            Source,FStreamableDelegate::CreateUObject(
                this,&AOGStartingRegionGenerator::OnR1MarketSectionLoaded,
                Generation,Section));
    if(Generation==R1MarketRequestGeneration &&
       Section==R1MarketNextSection)
        R1MarketStreamHandle=MoveTemp(NewHandle);
    if(!R1MarketStreamHandle.IsValid() &&
       Generation==R1MarketRequestGeneration &&
       Section==R1MarketNextSection)
    {
        bR1MarketSourceAssetUnavailable=true;
        UE_LOG(LogTemp,Error,
            TEXT("Gate02 market original section %d async request failed; no replacement mesh"),
            Section);
    }"""),
 ("""void AOGStartingRegionGenerator::OnR1MarketSectionLoaded()
{
    if(!bGate02R1RegionSource || !GetWorld() ||
       !GetWorld()->IsGameWorld() ||
       R1MarketNextSection>=MarketSourceSectionCount)
        return;

    const int32 Section=R1MarketNextSection;""",
 """void AOGStartingRegionGenerator::OnR1MarketSectionLoaded(
    const int32 RequestGeneration,const int32 RequestedSection)
{
    if(RequestGeneration!=R1MarketRequestGeneration ||
       RequestedSection!=R1MarketNextSection ||
       !bGate02R1RegionSource || !GetWorld() ||
       !GetWorld()->IsGameWorld() ||
       R1MarketNextSection>=MarketSourceSectionCount)
        return;
    const APlayerController* PC=GetWorld()->GetFirstPlayerController();
    const APawn* Pawn=PC?PC->GetPawn():nullptr;
    if(!Pawn ||
       FVector::DistSquared2D(
           GetActorTransform().InverseTransformPosition(
               Pawn->GetActorLocation()),
           MarketSourceOriginCm)>FMath::Square(StopLoadingRangeCm))
    {
        UnloadR1MarketSections();
        return; // Player left while the large original section was loading.
    }
    const int32 Section=RequestedSection;"""),
 ("""void AOGStartingRegionGenerator::UnloadR1MarketSections()
{
    if(R1MarketStreamHandle.IsValid())""",
 """void AOGStartingRegionGenerator::UnloadR1MarketSections()
{
    ++R1MarketRequestGeneration;
    if(R1MarketStreamHandle.IsValid())""")
])
print("CHECKPOINT_3: original Market sections remain exact; async inline race, cancelled callback, out-of-range install now prevented")
