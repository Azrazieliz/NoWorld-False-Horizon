from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
h=root/"Source/OfflineGame/Public/World/OGStartingRegionPresentation.h"
c=root/"Source/OfflineGame/Private/World/OGStartingRegionPresentation.cpp"
hi=h.read_text(encoding="utf-8-sig"); ci=c.read_text(encoding="utf-8-sig")
def one(s,a,b):
 n=s.count(a)
 if n!=1:raise RuntimeError(f"expected one match, found {n}: {a[:85]}")
 return s.replace(a,b,1)
hi=one(hi,'''    float GetPlayableHalfExtent() const
    {
        return FMath::Clamp(TerrainCellsPerSide, 24, 128) *''','''    float GetPlayableHalfExtent() const
    {
        if (bGate02R1RegionSource) return 160000.0f;
        return FMath::Clamp(TerrainCellsPerSide, 24, 128) *''')
hi=one(hi,'''    int32 EditorPreviewSeed = 731942;''','''    int32 EditorPreviewSeed = 731942;

    /** Reuses THIS generator's ProceduralMesh/HISM pipeline, replacing the
     * disposable fixture geometry with actual Gate02 R1 513-height source.
     * Never author legacy random settlement/faction/dungeon on this path. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="OfflineGame|World|Gate02")
    bool bGate02R1RegionSource = false;''')
hi=one(hi,'''    static constexpr int32 GeneratorVersion = 1;''','''    struct FR1Placement
    {
        FString PersistentId;
        FVector PositionCm = FVector::ZeroVector;
        float YawDegrees = 0.f;
        float UniformScale = 1.f;
        int32 TypeIndex = INDEX_NONE;
        FIntPoint TileIndex = FIntPoint::ZeroValue;
    };

    // Actual Gate02 R1 authored data; no invented runtime layouts.
    TArray<float> R1HeightMeters;
    TArray<float> R1SoilMask;
    TArray<float> R1RockMask;
    TMap<FIntPoint,TArray<FR1Placement>> R1TilePlacements;
    FIntPoint LastR1StreamTile = FIntPoint(MIN_int32,MIN_int32);
    bool bR1DatasetLoaded = false;
    bool LoadR1MetricData();
    void BuildR1SourceTerrain();
    void LoadR1SourceScatter();
    void RefreshR1StreamedTiles(bool bForce);
    float R1HeightAt(int32 X,int32 Y) const;

    static constexpr int32 GeneratorVersion = 1;''')
hi=one(hi,'''    virtual void BeginPlay() override;

    virtual bool IsInsidePlayableBoundary_Implementation(''','''    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    virtual bool IsInsidePlayableBoundary_Implementation(''')
ci=one(ci,'''#include "World/OGStartingRegionPresentation.h"''','''#include "World/OGStartingRegionPresentation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"''')
ci=one(ci,'''    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>''','''    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>''')
ci=one(ci,'''    ResolveCanonicalSeed(Seed);
    ActiveSeed = Seed;

    GenerateRegion(Seed);
    PersistCanonicalLocations(Seed);''','''    // R1 is a protected candidate region source, NOT yet accepted as canonical
    // locations. Do not write candidate IDs, locations or random settlements.
    if (!bGate02R1RegionSource) ResolveCanonicalSeed(Seed);
    else Seed = 1412; // Original authored Gate02 ecology seed.
    ActiveSeed = Seed;
    GenerateRegion(Seed);
    if (!bGate02R1RegionSource)
        PersistCanonicalLocations(Seed);
    SetActorTickEnabled(bGate02R1RegionSource && bR1DatasetLoaded);''')
ci=one(ci,'''void AOGStartingRegionGenerator::GenerateRegion(int32 Seed)
{
    ClearGeneratedContent();
    BuildLayout(Seed);''','''void AOGStartingRegionGenerator::GenerateRegion(int32 Seed)
{
    ClearGeneratedContent();
    if (bGate02R1RegionSource)
    {
        // Do not silently replace a missing/invalid Gate02 source with a
        // grey primitive playground; preserve the failure and real project.
        if (!LoadR1MetricData())
        {
            UE_LOG(LogOfflineGame,Error,TEXT("Gate02 R1 source region unavailable: refusing dummy terrain"));
            return;
        }
        ApplyPresentationMaterials();
        BuildR1SourceTerrain();
        LoadR1SourceScatter();
        RefreshR1StreamedTiles(true);
        return;
    }
    BuildLayout(Seed);''')
ci=one(ci,'''    const float Phase = static_cast<float>(Seed % 4093) * 0.0037f;''','''    if (bGate02R1RegionSource && R1HeightMeters.Num() == 513*513)
    {
        const float SampleX = FMath::Clamp((X+160000.f)/625.f,0.f,512.f);
        const float SampleY = FMath::Clamp((Y+160000.f)/625.f,0.f,512.f);
        const int32 X0=FMath::FloorToInt(SampleX), Y0=FMath::FloorToInt(SampleY);
        const int32 X1=FMath::Min(512,X0+1), Y1=FMath::Min(512,Y0+1);
        const float TX=SampleX-X0,TY=SampleY-Y0;
        const float A=FMath::Lerp(R1HeightAt(X0,Y0),R1HeightAt(X1,Y0),TX);
        const float B=FMath::Lerp(R1HeightAt(X0,Y1),R1HeightAt(X1,Y1),TX);
        return FMath::Lerp(A,B,TY)*100.f;
    }
    const float Phase = static_cast<float>(Seed % 4093) * 0.0037f;''')
c.write_text(ci,encoding="utf-8",newline="")
h.write_text(hi,encoding="utf-8",newline="")
print("Extended EXISTING region generator with real Gate02 R1 streaming source contract.")
