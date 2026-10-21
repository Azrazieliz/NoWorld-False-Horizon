from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=root/"Public/World/OGStartingRegionPresentation.h"
cpp=root/"Private/World/OGStartingRegionPresentation.cpp"
def apply(p,swaps):
 s=p.read_text(encoding="utf-8-sig")
 for old,new in swaps:
  count=s.count(old)
  if count!=1:raise RuntimeError(f"{p.name}: {count} occurrences for {old[:110]}")
  s=s.replace(old,new,1)
 p.write_text(s,encoding="utf-8",newline="")
 print("SOURCE_PATCHED",p.name)
apply(h,[
 ("    bool bIntegrationCourseBuilt = false;",
 """    bool bIntegrationCourseBuilt = false;
    // First World needs a real UI/interaction runtime, not fixture actors.
    // Guard against invoking the diagnostic course on every World tick.
    bool bNativeFirstWorldRuntimeInitialized = false;""")
])
apply(cpp,[
 ("""    if (!IntegrationWorld) InitializeFoundationIntegration();
    UpdateDiagnosticAnimationState();""",
 """    if (!IntegrationWorld && !bNativeFirstWorldRuntimeInitialized)
        InitializeFoundationIntegration();
    UpdateDiagnosticAnimationState();"""),
 ("""        if (!bHasGate02FirstWorld)
            if (AOGWorldPrototypeCharacter* Character = Cast<AOGWorldPrototypeCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)))
                Character->InitializeFoundationIntegration();""",
 """        // The native character initializer now distinguishes the authored
        // Gate02 World from the diagnostic fixture. Always initialize the
        // real runtime, HUD interaction host and canonical presentation;
        // never silently abandon these services in the production World.
        if (AOGWorldPrototypeCharacter* Character =
                Cast<AOGWorldPrototypeCharacter>(UGameplayStatics::GetPlayerCharacter(World,0)))
            Character->InitializeFoundationIntegration();"""),
 ("""    GetWorld()->GetWorldSettings()->bEnableWorldBoundsChecks = false;
    if (DiagnosticAnimations) DiagnosticAnimations->BindMesh(GetMesh());
    bHasDiagnosticFacingSample = false;
    if (!IntegrationWorld) IntegrationWorld = GetWorld()->SpawnActor<AOGFoundationIntegrationWorld>(FVector::ZeroVector, FRotator::ZeroRotator);""",
 """    GetWorld()->GetWorldSettings()->bEnableWorldBoundsChecks = false;
    if (DiagnosticAnimations) DiagnosticAnimations->BindMesh(GetMesh());
    bHasDiagnosticFacingSample = false;

    // Native First World: retain the same persistent Ruler, actual ground
    // snapping, movement, camera, Gate10 UI and interaction services.
    // No integration test obstacle course, artificial party/NPC, forced
    // teleport, or synthetic opening combat is permitted in this map.
    for(TActorIterator<AOGStartingRegionGenerator> It(GetWorld());It;++It)
        if(It->bGate02R1RegionSource)
        {
            if (DiagnosticMenu) DiagnosticMenu->Initialize();
            if (InteractionHost) InteractionHost->Initialize();
            bNativeFirstWorldRuntimeInitialized = true;
            UE_LOG(LogOfflineGame,Display,
                   TEXT("Gate02 R1 first World: genuine gameplay services initialized without diagnostic fixture"));
            return;
        }

    if (!IntegrationWorld) IntegrationWorld = GetWorld()->SpawnActor<AOGFoundationIntegrationWorld>(FVector::ZeroVector, FRotator::ZeroRotator);"""),
 ("""    bIntegrationCourseBuilt = false;
    bHasDiagnosticFacingSample = false;""",
 """    bIntegrationCourseBuilt = false;
    bNativeFirstWorldRuntimeInitialized = false;
    bHasDiagnosticFacingSample = false;"""),
])
print("NATIVE_FIRST_WORLD_INITIALIZES_REAL_CONTROL_WITHOUT_FIXTURE_AND_NO_TICK_RESPAWN")
