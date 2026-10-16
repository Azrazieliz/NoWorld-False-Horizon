#include "Runtime/OGGate12IsolatedRuntimeJourneyProbe.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "UnrealClient.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "UI/OGGate12NativeRulerWidget.h"
#include "UI/OGGate12CharacterPreviewStage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "World/OGStartingRegionPresentation.h"
#include "UObject/UObjectIterator.h"

namespace
{
    constexpr int32 MaxReadyRetries = 35;

    bool HasNativeRulerWidgetInViewport(const UWorld* World)
    {
        for (TObjectIterator<UOGGate12NativeRulerWidget> It; It; ++It)
        {
            if (It->GetWorld() == World && It->IsInViewport())
                return true;
        }
        return false;
    }
}

bool UOGGate12IsolatedRuntimeJourneyProbe::ShouldCreateSubsystem(
    UObject* Outer) const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    UWorld* World = Cast<UWorld>(Outer);
    return Super::ShouldCreateSubsystem(Outer) &&
        World && World->WorldType == EWorldType::Game &&
        FParse::Param(FCommandLine::Get(), TEXT("Gate12RuntimeJourney")) &&
        // Absolute safety boundary: never create a Ruler in production saves
        // or normal PIE, even if someone accidentally adds the CLI flag.
        FPaths::ProjectDir().Contains(
            TEXT("Isolated_NoCook_Win64_Game"), ESearchCase::IgnoreCase);
#endif
}

void UOGGate12IsolatedRuntimeJourneyProbe::OnWorldBeginPlay(
    UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Gate12Readiness"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    ReceiptPath = FPaths::Combine(Directory,
        TEXT("runtime_ruler_world_journey.json"));

    Record(TEXT("world_begin_play"), true,
        FString::Printf(TEXT("World=%s, project_isolated=1"),
            *InWorld.GetMapName()));
    ScheduleNext(4.f);
}

void UOGGate12IsolatedRuntimeJourneyProbe::Deinitialize()
{
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(NextStepTimer);
    Super::Deinitialize();
}

void UOGGate12IsolatedRuntimeJourneyProbe::ScheduleNext(
    float Seconds)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(NextStepTimer, this,
            &UOGGate12IsolatedRuntimeJourneyProbe::RunNextStep,
            Seconds, false);
    }
}

void UOGGate12IsolatedRuntimeJourneyProbe::SaveReceipt(
    bool bComplete)
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("schema"),
        TEXT("GATE12_ISOLATED_NATIVE_WIN64_GAME_JOURNEY_V1"));
    Root->SetStringField(TEXT("original_project_saves"),
        TEXT("NO_ACCESS_ISOLATED_PROJECT_ONLY"));
    Root->SetBoolField(TEXT("android_cooked_or_installed"), false);
    Root->SetBoolField(TEXT("production_assets_modified"), false);
    Root->SetBoolField(TEXT("programmatic_native_ui_actions"), true);
    Root->SetBoolField(TEXT("physical_touch_inputs_verified"), false);
    Root->SetBoolField(TEXT("complete"), bComplete);
    Root->SetBoolField(TEXT("passed"), bComplete && !bFailed);
    Root->SetStringField(TEXT("status"),
        !bComplete ? TEXT("RUNNING")
        : bFailed ? TEXT("FAIL_NATIVE_GAME_JOURNEY")
                  : TEXT("PASS_NATIVE_GAME_JOURNEY_NOT_ANDROID"));
    TArray<TSharedPtr<FJsonValue>> JsonSteps;
    for (const FJourneyStep& Entry : Events)
    {
        TSharedRef<FJsonObject> Step = MakeShared<FJsonObject>();
        Step->SetStringField(TEXT("name"), Entry.Name);
        Step->SetBoolField(TEXT("passed"), Entry.bPassed);
        Step->SetStringField(TEXT("detail"), Entry.Detail);
        JsonSteps.Add(MakeShared<FJsonValueObject>(Step));
    }
    Root->SetArrayField(TEXT("steps"), MoveTemp(JsonSteps));
    FString Payload;
    TSharedRef<TJsonWriter<>> Writer =
        TJsonWriterFactory<>::Create(&Payload);
    FJsonSerializer::Serialize(Root, Writer);
    if (!FFileHelper::SaveStringToFile(Payload, *ReceiptPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("GATE12_JOURNEY receipt write failed"));
    }
}

void UOGGate12IsolatedRuntimeJourneyProbe::Record(
    const FString& Name, bool bSuccess, const FString& Detail)
{
    Events.Add({Name, Detail, bSuccess});
    bFailed |= !bSuccess;
    UE_LOG(LogTemp, Display, TEXT("GATE12_JOURNEY %s %s: %s"),
        bSuccess ? TEXT("PASS") : TEXT("FAIL"),
        *Name, *Detail);
    SaveReceipt(false);
}

void UOGGate12IsolatedRuntimeJourneyProbe::RequestActualGameScreenshot(
    const FString& Label)
{
    // HighResShot excludes Slate/UMG by default, so it created false
    // negative black Ruler images despite a live full-screen native widget.
    // Request a genuine composed frame WITH Slate from the real viewport.
    if (!GetWorld() || !GetWorld()->GetFirstPlayerController())
        return;
    const FString ScreenshotDirectory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Gate12Readiness"),
        TEXT("JourneyScreenshots"));
    IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
    const FString Screenshot = FPaths::Combine(
        ScreenshotDirectory, Label + TEXT(".png"));
    FScreenshotRequest::RequestScreenshot(
        Screenshot, /*bInShowUI=*/true,
        /*bAddFilenameSuffix=*/false,
        /*bHdrScreenshot=*/false, FIntRect(),
        /*bInRestrictToGameViewport=*/true);
    UE_LOG(LogTemp, Display,
        TEXT("GATE12_JOURNEY Slate-inclusive screenshot requested: %s"),
        *Screenshot);
}

void UOGGate12IsolatedRuntimeJourneyProbe::RunNextStep()
{
    UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    AOGWorldPresentationHud* HUD = PC
        ? Cast<AOGWorldPresentationHud>(PC->GetHUD()) : nullptr;
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    UOGPresentationModeSubsystem* Mode = GI
        ? GI->GetSubsystem<UOGPresentationModeSubsystem>() : nullptr;
    UOGGameCoreSubsystem* Core = GI
        ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;

    if (!World || !GI || !PC || !PC->IsLocalController() ||
        !PC->GetLocalPlayer() || !HUD || !PC->GetPawn() ||
        !Session || !Mode || !Core || !Core->IsCoreReady())
    {
        if (++ReadinessRetries < MaxReadyRetries)
        {
            ScheduleNext(2.f);
            return;
        }
        Record(TEXT("local_gameplay_ready"), false,
            TEXT("No genuine local Pawn/HUD/GameInstance/core after timeout"));
        SaveReceipt(true);
        return;
    }

    if (Phase == 0)
    {
        const bool bReady = World->GetMapName().Contains(
            TEXT("StartingWorld")) &&
            IsValid(PC->GetPawn()) &&
            HUD->IsGate12NativeRulerUMGEnabled();
        InitialPawn = PC->GetPawn();
        Record(TEXT("starting_world_real_local_player_hud"), bReady,
            FString::Printf(
                TEXT("Map=%s, Pawn=%s, Hud=%s, CoreReady=%d"),
                *World->GetMapName(),
                *PC->GetPawn()->GetClass()->GetName(),
                *HUD->GetClass()->GetName(), Core->IsCoreReady()));
        if (!bReady) {SaveReceipt(true);return;}
        HUD->NotifyHitBoxClick(FName(TEXT("OG.Opening.Ruler")));
        Record(TEXT("opening_to_ruler_via_real_canvas_action"),
            Mode->GetMode() == EOGPresentationMode::Ruler,
            TEXT("Called existing OG.Opening.Ruler click handler"));
        RequestActualGameScreenshot(TEXT("ruler_before_owner"));
        Phase = 1;
        ScheduleNext(3.f);
        return;
    }

    if (Phase == 1)
    {
        const bool bRulerMode =
            Mode->GetMode() == EOGPresentationMode::Ruler;
        UOGGate12NativeRulerWidget* LiveWidget = nullptr;
        for (TObjectIterator<UOGGate12NativeRulerWidget> It; It; ++It)
        {
            if (It->GetWorld() == World && It->IsInViewport())
            {
                LiveWidget = *It;
                break;
            }
        }
        const bool bRoot = LiveWidget &&
            LiveWidget->GetRootWidget() != nullptr;
        const bool bTitle = LiveWidget &&
            LiveWidget->GetWidgetFromName(TEXT("GameTitle")) != nullptr;
        const bool bPage = LiveWidget &&
            LiveWidget->GetWidgetFromName(TEXT("CanonicalPageBody")) != nullptr;
        const bool bNavigation = LiveWidget &&
            LiveWidget->GetWidgetFromName(TEXT("RulerFiveDestinations")) != nullptr;
        // SelfHitTestInvisible renders its own surface and permits child
        // buttons to receive input. It is not the same as Hidden, Collapsed
        // or HitTestInvisible (which blocks every child hit target).
        const ESlateVisibility Visibility = LiveWidget
            ? LiveWidget->GetVisibility() : ESlateVisibility::Collapsed;
        const bool bChildInteractive =
            Visibility == ESlateVisibility::Visible ||
            Visibility == ESlateVisibility::SelfHitTestInvisible;
        const bool bValidWidget = bRoot && bTitle && bPage &&
            bNavigation && bChildInteractive;
        Record(TEXT("ruler_ui_presented"), bRulerMode && bValidWidget,
            FString::Printf(
                TEXT("RulerMode=%d Root=%d Title=%d Page=%d Nav=%d Interactive=%d VisibilityEnum=%d Blueprint=%s"),
                bRulerMode, bRoot, bTitle, bPage, bNavigation,
                bChildInteractive, static_cast<int32>(Visibility),
                LiveWidget ? *LiveWidget->GetClass()->GetName() : TEXT("None")));
        if (!bRulerMode || !bValidWidget)
        {
            SaveReceipt(true);
            return;
        }

        // This legitimate new-local-Ruler user action writes ONLY to the
        // sandbox project's independent SQLite world, not user saves.
        if (!Session->IsBoundToStoredRuler())
            HUD->NotifyHitBoxClick(FName(TEXT("OG.Ruler.Create")));
        if (!Session->IsBoundToStoredRuler())
            HUD->NotifyHitBoxClick(FName(TEXT("OG.Ruler.Restore")));

        const bool bBound = Session->IsBoundToStoredRuler() &&
            Session->BoundRulerId.IsValid();
        if (bBound)
            RulerIdAtCreation = Session->BoundRulerId.ToString();
        Record(TEXT("canonical_ruler_create_or_restore"),
            bBound && Session->OwnedRoster.IsEmpty() &&
            Session->TerritoryProjection.Territories.IsEmpty(),
            FString::Printf(
                TEXT("ActualOwner=%s, owned=%d, claimed=%d"),
                *RulerIdAtCreation, Session->OwnedRoster.Num(),
                Session->TerritoryProjection.Territories.Num()));
        RequestActualGameScreenshot(TEXT("ruler_after_owner"));
        if (!bBound) {SaveReceipt(true);return;}
        Phase = 2;
        ScheduleNext(3.f);
        return;
    }

    if (Phase >= 2 && Phase <= 6)
    {
        static const TCHAR* DestinationNames[] =
        {
            TEXT("home"), TEXT("characters"), TEXT("gacha"),
            TEXT("territory"), TEXT("records")
        };
        const int32 Index = Phase - 2;
        UOGGate12NativeRulerWidget* LiveWidget = nullptr;
        for (TObjectIterator<UOGGate12NativeRulerWidget> It; It; ++It)
        {
            if (It->GetWorld() == World && It->IsInViewport())
            {
                LiveWidget = *It;
                break;
            }
        }
        if (LiveWidget)
            LiveWidget->InvokeAction(
                FString::Printf(TEXT("nav/%d"), Index));
        const bool bNavigated = LiveWidget &&
            Session->CurrentDestination ==
                static_cast<EOGRulerPrimaryDestination>(Index) &&
            LiveWidget->GetWidgetFromName(TEXT("CanonicalPageBody")) != nullptr &&
            Session->OwnedRoster.IsEmpty();
        Record(FString::Printf(TEXT("native_ruler_navigation_%s"),
                   DestinationNames[Index]),
            bNavigated, FString::Printf(
                TEXT("ActualPage=%d Selected=%d, AuthenticOwner=%s"),
                static_cast<int32>(Session->CurrentDestination), Index,
                *RulerIdAtCreation));
        if (!bNavigated) {SaveReceipt(true);return;}
        RequestActualGameScreenshot(
            FString::Printf(TEXT("ruler_nav_%s"), DestinationNames[Index]));
        if (Index == 1 && LiveWidget)
        {
            // The visible QA control uses only genuine imported 45-part
            // Luminous meshes. It is noncanonical and does not own content.
            LiveWidget->InvokeAction(TEXT("qa/open"));
            int32 SourceMeshCount = 0;
            bool bValidTarget = false;
            for (TActorIterator<AOGGate12CharacterPreviewStage> It(World);
                 It; ++It)
            {
                if (It->IsActorBeingDestroyed() ||
                    !It->GetLiveRenderTarget())
                    continue;
                const int32 FoundMeshes =
                    It->GetDevelopmentQaSourcePartCount();
                if (FoundMeshes == 45)
                {
                    SourceMeshCount = FoundMeshes;
                    bValidTarget = true;
                    break;
                }
            }
            const bool bValidQa = bValidTarget &&
                Session->OwnedRoster.IsEmpty() &&
                Session->SelectedManifestationId.IsValid() == false;
            Record(TEXT("development_45part_preview_noncanonical"),
                bValidQa,
                FString::Printf(
                    TEXT("RealMeshParts=%d RenderTarget=%d FakeOwnership=%d"),
                    SourceMeshCount, bValidTarget,
                    Session->OwnedRoster.Num()));
            RequestActualGameScreenshot(
                TEXT("ruler_characters_real_source_qa"));
            if (!bValidQa) {SaveReceipt(true);return;}
            // The source material first-load shader compilation must not be
            // mistaken for a character with invisible meshes. Hold the real
            // development viewer open long enough for GPU texture/shader
            // streaming before capturing its genuine display again.
            Phase = 11;
            ScheduleNext(25.f);
            return;
        }
        ++Phase;
        ScheduleNext(3.f);
        return;
    }

    if (Phase == 11)
    {
        int32 SourceMeshes = 0;
        bool bHasTexture = false;
        for (TActorIterator<AOGGate12CharacterPreviewStage> It(World);
             It; ++It)
        {
            if (It->IsActorBeingDestroyed())
                continue;
            SourceMeshes = It->GetDevelopmentQaSourcePartCount();
            bHasTexture = IsValid(It->GetLiveRenderTarget());
        }
        const bool bRemainsValid = SourceMeshes == 45 && bHasTexture &&
            Session->OwnedRoster.IsEmpty() &&
            Mode->GetMode() == EOGPresentationMode::Ruler;
        Record(TEXT("development_45part_preview_survives_shader_warmup"),
            bRemainsValid,
            FString::Printf(TEXT("Meshes=%d RT=%d OwnerChanged=%d"),
                SourceMeshes, bHasTexture,
                !Session->OwnedRoster.IsEmpty()));
        RequestActualGameScreenshot(
            TEXT("ruler_characters_real_source_qa_warm"));
        if (!bRemainsValid) {SaveReceipt(true);return;}
        Phase = 4;
        ScheduleNext(3.f);
        return;
    }

    if (Phase == 7)
    {
        // Exercise the actual native UMG ENTER WORLD action, not only the
        // legacy Canvas fallback. Empty-roster worlds legitimately explore
        // with the existing prototype Pawn, with no made-up Manifestation.
        UOGGate12NativeRulerWidget* LiveWidget = nullptr;
        for (TObjectIterator<UOGGate12NativeRulerWidget> It; It; ++It)
        {
            if (It->GetWorld() == World && It->IsInViewport())
            {
                LiveWidget = *It;
                break;
            }
        }
        if (LiveWidget)
        {
            LiveWidget->InvokeAction(TEXT("nav/0"));
            LiveWidget->InvokeAction(TEXT("world"));
        }
        const bool bWorld = Mode->GetMode() == EOGPresentationMode::World;
        Record(TEXT("ruler_to_world_validated_native_transition"),
            bWorld && InitialPawn.IsValid() &&
            InitialPawn.Get() == PC->GetPawn(),
            FString::Printf(TEXT("WorldMode=%d, PawnUnchanged=%d, owner=%s"),
                bWorld, InitialPawn.Get() == PC->GetPawn(),
                *RulerIdAtCreation));
        if (!bWorld) {SaveReceipt(true);return;}
        Phase = 8;
        ScheduleNext(3.f);
        return;
    }

    if (Phase == 8)
    {
        const bool bWorld = Mode->GetMode() == EOGPresentationMode::World;
        Record(TEXT("live_world_still_active"), bWorld &&
            PC->GetPawn() == InitialPawn.Get(),
            TEXT("Same real World Pawn after three seconds; no cloned ownership"));
        // Scene screenshot must actually complete BEFORE returning to
        // Ruler. Previously, immediate same-tick return captured Ruler Home
        // and falsely looked like a bad World camera/material.
        RequestActualGameScreenshot(TEXT("world"));
        Phase = 9;
        ScheduleNext(3.f);
        return;
    }

    if (Phase == 9)
    {
        HUD->NotifyHitBoxClick(FName(TEXT("OG.World.Ruler")));
        const bool bRuler = Mode->GetMode() == EOGPresentationMode::Ruler;
        Record(TEXT("world_to_ruler_via_real_canvas_action"),
            bRuler, TEXT("Called existing OG.World.Ruler click handler"));
        if (!bRuler) {SaveReceipt(true);return;}
        Phase = 10;
        ScheduleNext(3.f);
        return;
    }

    const bool bSameOwner = Session->IsBoundToStoredRuler() &&
        Session->BoundRulerId.ToString() == RulerIdAtCreation;
    const bool bRealReturn = Mode->GetMode() == EOGPresentationMode::Ruler &&
        HasNativeRulerWidgetInViewport(World) &&
        PC->GetPawn() == InitialPawn.Get();
    const bool bOwnershipSafe = Session->OwnedRoster.IsEmpty() &&
        Session->TerritoryProjection.Territories.IsEmpty();

    Record(TEXT("ruler_world_ruler_end_to_end"),
        bSameOwner && bRealReturn && bOwnershipSafe,
        FString::Printf(
            TEXT("OwnerPreserved=%d UMG=%d PawnPreserved=%d NoFakeAssets=%d"),
            bSameOwner, HasNativeRulerWidgetInViewport(World),
            PC->GetPawn() == InitialPawn.Get(), bOwnershipSafe));
    RequestActualGameScreenshot(TEXT("ruler_after_world"));
    SaveReceipt(true);
}
