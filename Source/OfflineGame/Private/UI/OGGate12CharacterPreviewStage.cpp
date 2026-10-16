#include "UI/OGGate12CharacterPreviewStage.h"
#include "UI/OGGate12CharacterPreviewConfiguration.h"

#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AOGGate12CharacterPreviewStage::AOGGate12CharacterPreviewStage()
{
    PrimaryActorTick.bCanEverTick = false;
    StageRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
    RootComponent = StageRoot;
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("LiveCharacterCapture"));
    Capture->SetupAttachment(StageRoot);
    Capture->SetRelativeLocation(FVector(-360.f, 0.f, 145.f));
    Capture->SetRelativeRotation(FRotator(-5.f, 0.f, 0.f));
    Capture->FOVAngle = 31.f;
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->bCaptureEveryFrame = true;
    Capture->bCaptureOnMovement = false;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    SetActorEnableCollision(false);
}

bool AOGGate12CharacterPreviewStage::InitializeApprovedPreview(
    TSubclassOf<AActor> PreviewClass,
    const FOGEntityId& RulerId,
    const FOGContentId& IdentityId,
    const FOGContentId& VersionId,
    const FOGEntityId& ManifestationId)
{
    ReleasePreview();
    UWorld* World = GetWorld();
    // Never spawn a second playable Pawn, controller, or production gameplay actor.
    if (!World || !PreviewClass || PreviewClass->IsChildOf(APawn::StaticClass()) ||
        !RulerId.IsValid() || !IdentityId.IsValid() ||
        !VersionId.IsValid() || !ManifestationId.IsValid() ||
        !PreviewClass->ImplementsInterface(
            UOGGate12CharacterPreviewConfiguration::StaticClass()))
        return false;

    RenderTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
    if (!RenderTarget) return false;
    RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
    RenderTarget->InitAutoFormat(512, 768);
    RenderTarget->UpdateResourceImmediate(true);
    Capture->TextureTarget = RenderTarget;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.Owner = this;
    PreviewActor = World->SpawnActor<AActor>(
        PreviewClass, GetActorLocation(), FRotator(0.f, 180.f, 0.f), Params);
    if (!PreviewActor)
    {
        ReleasePreview();
        return false;
    }
    PreviewActor->SetActorEnableCollision(false);
    // A blueprint must bind actual owned appearance and Privacy before
    // anything is captured, or we refuse to render an incorrect outfit.
    if (!IOGGate12CharacterPreviewConfiguration::Execute_ConfigureOwnedPreview(
            PreviewActor, RulerId, IdentityId, VersionId, ManifestationId))
    {
        ReleasePreview();
        return false;
    }
    Capture->ShowOnlyActorComponents(PreviewActor, true);
    Capture->CaptureScene();
    return true;
}

void AOGGate12CharacterPreviewStage::SetLiveCapture(bool bLive)
{
    if (!Capture || !RenderTarget) return;
    Capture->bCaptureEveryFrame = bLive;
    if (!bLive) Capture->CaptureScene();
}

void AOGGate12CharacterPreviewStage::ReleasePreview()
{
    if (Capture)
    {
        Capture->ShowOnlyActors.Empty();
        Capture->ShowOnlyComponents.Empty();
        Capture->TextureTarget = nullptr;
    }
    if (IsValid(PreviewActor)) PreviewActor->Destroy();
    PreviewActor = nullptr;
    RenderTarget = nullptr;
}

void AOGGate12CharacterPreviewStage::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ReleasePreview();
    Super::EndPlay(EndPlayReason);
}
