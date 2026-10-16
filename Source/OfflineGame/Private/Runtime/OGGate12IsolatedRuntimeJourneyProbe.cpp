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
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "UI/OGGate12NativeRulerWidget.h"
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
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            // Screenshot takes the NEXT actual game frame, not a synthetic
            // image or offline projection. Captured paths are verified after
            // this sandboxed process exits.
            PC->ConsoleCommand(TEXT("HighResShot 1"), true);
            UE_LOG(LogTemp, Display,
                TEXT("GATE12_JOURNEY screenshot requested for %s"), *Label);
        }
    }
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
        Record(TEXT("ruler_ui_presented"), bRulerMode &&
            HasNativeRulerWidgetInViewport(World),
            FString::Printf(TEXT("RulerMode=%d UMGVisible=%d"),
                bRulerMode,
                HasNativeRulerWidgetInViewport(World)));
        if (!bRulerMode) {SaveReceipt(true);return;}

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

    if (Phase == 2)
    {
        // Dispatch precisely the same legacy Canvas handler fixed to call
        // native owned-pawn binding/rollback. No owned character exists in
        // this empty test world, so it must preserve the real prototype Pawn.
        HUD->NotifyHitBoxClick(FName(TEXT("OG.Ruler.World")));
        const bool bWorld = Mode->GetMode() == EOGPresentationMode::World;
        Record(TEXT("ruler_to_world_validated_native_transition"),
            bWorld && InitialPawn.IsValid() &&
            InitialPawn.Get() == PC->GetPawn(),
            FString::Printf(TEXT("WorldMode=%d, PawnUnchanged=%d, owner=%s"),
                bWorld, InitialPawn.Get() == PC->GetPawn(),
                *RulerIdAtCreation));
        Phase = 3;
        ScheduleNext(3.f);
        return;
    }

    if (Phase == 3)
    {
        const bool bWorld = Mode->GetMode() == EOGPresentationMode::World;
        Record(TEXT("live_world_still_active"), bWorld &&
            PC->GetPawn() == InitialPawn.Get(),
            TEXT("Same real World Pawn after three seconds; no cloned ownership"));
        RequestActualGameScreenshot(TEXT("world"));
        HUD->NotifyHitBoxClick(FName(TEXT("OG.World.Ruler")));
        Record(TEXT("world_to_ruler_via_real_canvas_action"),
            Mode->GetMode() == EOGPresentationMode::Ruler,
            TEXT("Called existing OG.World.Ruler click handler"));
        Phase = 4;
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
