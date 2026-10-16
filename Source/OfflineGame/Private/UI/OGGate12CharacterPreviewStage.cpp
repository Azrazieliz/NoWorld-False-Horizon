#include "UI/OGGate12CharacterPreviewStage.h"
#include "UI/OGGate12CharacterPreviewConfiguration.h"

#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkeletalMesh.h"
#include "World/OGStartingRegionPresentation.h"

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
    const FOGEntityId& ManifestationId,
    bool bSfwPresentation)
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
            PreviewActor, RulerId, IdentityId, VersionId, ManifestationId,
            bSfwPresentation))
    {
        ReleasePreview();
        return false;
    }
    Capture->ShowOnlyActorComponents(PreviewActor, true);
    Capture->CaptureScene();
    return true;
}

bool AOGGate12CharacterPreviewStage::IsDevelopmentQaClassAllowed(
    const UClass* SourceQaClass)
{
#if UE_BUILD_SHIPPING
    return false;
#else
    // Explicit fixture allowlist: no arbitrary Pawn, no fake canonical IDs,
    // and no unaccepted Alter identity/appearance substitution.
    return SourceQaClass &&
        SourceQaClass->IsChildOf(AOGWorldPrototypeCharacter::StaticClass()) &&
        SourceQaClass->GetPathName() ==
        TEXT("/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/"
             "BP_LuminousValentineV3_CLOTHED_RuntimeQA."
             "BP_LuminousValentineV3_CLOTHED_RuntimeQA_C");
#endif
}

bool AOGGate12CharacterPreviewStage::InitializeDevelopmentQaPreview(
    UClass* SourceQaClass)
{
#if UE_BUILD_SHIPPING
    return false;
#else
    ReleasePreview();
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() ||
        !IsDevelopmentQaClassAllowed(SourceQaClass) ||
        !Capture)
        return false;

    RenderTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
    if (!RenderTarget) return false;
    RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
    RenderTarget->InitAutoFormat(512, 768);
    RenderTarget->UpdateResourceImmediate(true);
    Capture->TextureTarget = RenderTarget;

    // Assemble only the exact 45 authentic source skeletal mesh components
    // used by the verified experimental QA Blueprint. DO NOT spawn that Pawn:
    // its BeginPlay would register World runtime, create input and touch UI.
    struct FSourcePart { int32 Mesh; int32 Primitive; };
    static constexpr FSourcePart SourceParts[] =
    {
        {0,0},{0,1},{0,2},{1,0},{2,0},
        {3,0},{3,1},{3,2},{3,3},{3,4},{3,5},
        {3,6},{3,7},{3,8},{3,9},{3,10},{3,11},
        {4,0},{4,1},{4,2},{4,3},{4,4},{4,5},
        {4,6},{4,7},{4,8},{4,9},{4,10},{4,11},
        {5,0},{5,1},{5,2},{5,3},{5,4},{5,5},
        {5,6},{5,7},{5,8},{5,9},{5,10},{5,11},
        {5,12},{5,13},{5,14},{5,15}
    };

    TArray<USkeletalMesh*> SourceMeshes;
    SourceMeshes.Reserve(UE_ARRAY_COUNT(SourceParts));
    for (const FSourcePart& Part : SourceParts)
    {
        const FString MeshName = FString::Printf(
            TEXT("Mesh_%02d_Prim_%02d"), Part.Mesh, Part.Primitive);
        const FString Package = FString::Printf(
            TEXT("/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/"
                 "Mesh%03dPrim%03d/%s/SkeletalMeshes/%s"),
            Part.Mesh, Part.Primitive, *MeshName, *MeshName);
        USkeletalMesh* Skeletal = LoadObject<USkeletalMesh>(nullptr, *Package);
        if (!Skeletal || !Skeletal->GetSkeleton() ||
            Skeletal->GetRefSkeleton().GetNum() != 975)
        {
            ReleasePreview();
            return false;
        }
        SourceMeshes.Add(Skeletal);
    }
    if (SourceMeshes.Num() != 45)
    {
        ReleasePreview();
        return false;
    }

    for (USkeletalMesh* Mesh : SourceMeshes)
    {
        USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(
            this, NAME_None, RF_Transient);
        if (!Part)
        {
            ReleasePreview();
            return false;
        }
        DevelopmentQaMeshParts.Add(Part);
        Part->SetupAttachment(StageRoot);
        Part->SetSkeletalMeshAsset(Mesh);
        // Exact original 45 QA Blueprint part placement, not an inferred resize.
        Part->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->RegisterComponent();
        Part->SetComponentTickEnabled(false);
    }

    // Low-intensity test lighting previously measured in GPU studio sweep.
    // QA-only stage lights never edit the actual kingdom's map lighting.
    for (const TPair<FVector, float>& LightSpec :
         { TPair<FVector, float>(FVector(-185.f,-110.f,150.f),300.f),
           TPair<FVector, float>(FVector(-170.f,200.f,120.f),145.f) })
    {
        UPointLightComponent* Light = NewObject<UPointLightComponent>(
            this, NAME_None, RF_Transient);
        if (!Light)
        {
            ReleasePreview();
            return false;
        }
        DevelopmentQaLights.Add(Light);
        Light->SetupAttachment(StageRoot);
        Light->SetRelativeLocation(LightSpec.Key);
        Light->SetIntensity(LightSpec.Value);
        Light->SetAttenuationRadius(900.f);
        Light->RegisterComponent();
    }

    // Only the diagnostic camera moves closer. Production capture defaults,
    // the character mesh, original materials and World camera remain frozen.
    Capture->SetRelativeLocation(FVector(-260.f, 0.f, -18.f));
    Capture->SetRelativeRotation(FRotator::ZeroRotator);
    Capture->ShowOnlyActorComponents(this, true);
    Capture->CaptureScene();
    return true;
#endif
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
    for (USkeletalMeshComponent* Part : DevelopmentQaMeshParts)
        if (IsValid(Part)) Part->DestroyComponent();
    DevelopmentQaMeshParts.Reset();
    for (UPointLightComponent* Light : DevelopmentQaLights)
        if (IsValid(Light)) Light->DestroyComponent();
    DevelopmentQaLights.Reset();

    // Restore the accepted preview defaults for any later approved presentation.
    if (Capture)
    {
        Capture->SetRelativeLocation(FVector(-360.f, 0.f, 145.f));
        Capture->SetRelativeRotation(FRotator(-5.f, 0.f, 0.f));
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
