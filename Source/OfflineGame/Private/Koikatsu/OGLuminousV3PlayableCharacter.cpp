#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Koikatsu/OGLuminousV3WardrobeQAWidget.h"

#include "Animation/AnimSequence.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/OGDiagnosticCombatComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/OGFoundationCharacterRuntime.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const TCHAR* WardrobeName(EOGLuminousV3Wardrobe State)
{
    switch (State)
    {
    case EOGLuminousV3Wardrobe::Clothed: return TEXT("CLOTHED");
    case EOGLuminousV3Wardrobe::Underwear: return TEXT("UNDERWEAR");
    case EOGLuminousV3Wardrobe::Nude: return TEXT("NUDE");
    }
    return TEXT("INVALID");
}
bool ParseWardrobe(const FString& Value, EOGLuminousV3Wardrobe& Out)
{
    if (Value == TEXT("CLOTHED")) { Out = EOGLuminousV3Wardrobe::Clothed; return true; }
    if (Value == TEXT("UNDERWEAR")) { Out = EOGLuminousV3Wardrobe::Underwear; return true; }
    if (Value == TEXT("NUDE")) { Out = EOGLuminousV3Wardrobe::Nude; return true; }
    return false;
}
bool SameBoneTopologyAndRest(const USkeletalMesh* A, const USkeletalMesh* B)
{
    if (!A || !B) return false;
    const FReferenceSkeleton& Left = A->GetRefSkeleton();
    const FReferenceSkeleton& Right = B->GetRefSkeleton();
    if (Left.GetNum() != 975 || Right.GetNum() != 975) return false;
    for (int32 Index = 0; Index < Left.GetNum(); ++Index)
    {
        if (Left.GetBoneName(Index) != Right.GetBoneName(Index) ||
            Left.GetParentIndex(Index) != Right.GetParentIndex(Index) ||
            !Left.GetRefBonePose()[Index].Equals(Right.GetRefBonePose()[Index], 0.0001f))
            return false;
    }
    return true;
}
}

FOGLuminousV3AppearanceProjection FOGLuminousV3AppearancePolicy::Resolve(
    EOGLuminousV3Wardrobe Actual,
    bool bPrivacySfw,
    bool bExplicitContextAuthorized,
    bool bSafeClothedAssetAvailable)
{
    FOGLuminousV3AppearanceProjection Result;
    Result.ActualWardrobe = Actual;
    Result.bPrivacySfwEnabled = bPrivacySfw;
    Result.bDefaultBaseAppearance = Actual == EOGLuminousV3Wardrobe::Clothed;
    // This is a pure read-only function: no store, equipment or world writes.
    switch (Actual)
    {
    case EOGLuminousV3Wardrobe::Clothed:
        Result.EffectiveRenderedWardrobe = EOGLuminousV3Wardrobe::Clothed;
        Result.bRenderable = bSafeClothedAssetAvailable;
        return Result;
    case EOGLuminousV3Wardrobe::Underwear:
    case EOGLuminousV3Wardrobe::Nude:
        if (bPrivacySfw || !bExplicitContextAuthorized)
        {
            Result.EffectiveRenderedWardrobe = EOGLuminousV3Wardrobe::Clothed;
            Result.bMaskedForPrivacyOrUnavailableAdultContext = true;
            Result.bRenderable = bSafeClothedAssetAvailable;
            return Result;
        }
        Result.EffectiveRenderedWardrobe = Actual;
        Result.bRenderable = true;
        return Result;
    default:
        Result.bRenderable = false; // Unsupported asset: never display an old explicit pose.
        return Result;
    }
}

AOGLuminousV3PlayableCharacter::AOGLuminousV3PlayableCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}

const TArray<FSoftObjectPath>& AOGLuminousV3PlayableCharacter::PartsFor(EOGLuminousV3Wardrobe State) const
{
    switch (State)
    {
    case EOGLuminousV3Wardrobe::Underwear: return UnderwearNativeParts;
    case EOGLuminousV3Wardrobe::Nude: return NudeNativeParts;
    default: return ClothedNativeParts;
    }
}

int32 AOGLuminousV3PlayableCharacter::ExpectedPartsFor(EOGLuminousV3Wardrobe State)
{
    switch (State)
    {
    case EOGLuminousV3Wardrobe::Clothed: return 45;
    case EOGLuminousV3Wardrobe::Underwear: return 39;
    case EOGLuminousV3Wardrobe::Nude: return 37;
    }
    return 0;
}

bool AOGLuminousV3PlayableCharacter::ValidateWardrobeAssets(
    EOGLuminousV3Wardrobe Desired, FString& Error) const
{
    const TArray<FSoftObjectPath>& Paths = PartsFor(Desired);
    if (Paths.Num() != ExpectedPartsFor(Desired))
    {
        Error = FString::Printf(TEXT("Luminous V3: wardrobe %s requires %d exact parts, got %d"),
            WardrobeName(Desired), ExpectedPartsFor(Desired), Paths.Num());
        return false;
    }
    TSet<FString> UniquePaths;
    for (int32 Index = 0; Index < Paths.Num(); ++Index)
    {
        const FString AssetPath = Paths[Index].ToString();
        if (AssetPath.IsEmpty() || UniquePaths.Contains(AssetPath))
        {
            Error = FString::Printf(TEXT("Luminous V3: invalid/duplicate source index %d"), Index);
            return false;
        }
        UniquePaths.Add(AssetPath);
        if (!AssetPath.StartsWith(TEXT("/Game/Experimental/LuminousValentineV3/")))
        {
            Error = TEXT("Luminous V3: refusing an unexpected or cross-character asset namespace");
            return false;
        }
    }
    return true;
}

bool AOGLuminousV3PlayableCharacter::ApplyVisualWardrobe(
    EOGLuminousV3Wardrobe Desired, FString& Error)
{
    if (!ValidateWardrobeAssets(Desired, Error)) return false;
    const auto& Paths = PartsFor(Desired);
    // Retain the existing pawn, capsule, camera, input and combat components.
    // Prepare the exact full state first; do not publish a half-loaded wardrobe.
    TArray<USkeletalMesh*> Prepared;
    Prepared.Reserve(Paths.Num());
    for (int32 Index = 0; Index < Paths.Num(); ++Index)
    {
        USkeletalMesh* NativeMesh = Cast<USkeletalMesh>(Paths[Index].TryLoad());
        if (!NativeMesh || !NativeMesh->GetSkeleton() || NativeMesh->GetRefSkeleton().GetNum() != 975)
        {
            Error = FString::Printf(TEXT("Luminous V3: missing/invalid 975-bone mesh at index %d state %s"),
                Index, WardrobeName(Desired));
            return false;
        }
        Prepared.Add(NativeMesh);
    }
    // One component pool reused for all three states. Never instantiate 121
    // simultaneous 975-bone character parts or touch imported source UAssets.
    while (RuntimeParts.Num() < 45)
    {
        const int32 Index = RuntimeParts.Num();
        const FName Name(*FString::Printf(TEXT("LuminousV3NativePart_%02d"), Index));
        USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(this, Name);
        if (!Component) { Error = TEXT("Luminous V3: skeletal component allocation failed"); return false; }
        Component->SetupAttachment(GetRootComponent());
        Component->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCastShadow(true);
        Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        AddInstanceComponent(Component);
        Component->RegisterComponent();
        RuntimeParts.Add(Component);
    }
    for (int32 Index = 0; Index < RuntimeParts.Num(); ++Index)
    {
        USkeletalMeshComponent* Component = RuntimeParts[Index];
        Component->SetLeaderPoseComponent(nullptr);
        Component->Stop();
        Component->SetSkeletalMeshAsset(Prepared.IsValidIndex(Index) ? Prepared[Index] : nullptr);
        Component->SetVisibility(Prepared.IsValidIndex(Index), true);
    }
    ActivePartCount = Prepared.Num();
    // Assign only the rendered geometry. Actual selected clothing is owned by
    // CurrentWardrobe, never by the Privacy/SFW presentation resolver.
    EffectiveAppearance.EffectiveRenderedWardrobe = Desired;
    CurrentNativeClip = nullptr;
    bCompatibleNativeMotion = false;
    CurrentMotionPhase = EOGLuminousV3MotionPhase::Idle;
    bLuminousReady = true;
    if (GetMesh()) GetMesh()->SetHiddenInGame(true);
    UE_LOG(LogTemp, Display, TEXT("Gate12 Luminous: native wardrobe=%s parts=%d/45 skeletons=975"),
        WardrobeName(Desired), ActivePartCount);
    return true;
}

bool AOGLuminousV3PlayableCharacter::IsIsolatedWardrobeQAContext() const
{
    return bRunNonPersistentWardrobeQACycle || bRunNativeMotionQACycle ||
        bEnableNativeWardrobeQATouchPanel || bEnterWorldModeOnIsolatedQAStartup;
}

void AOGLuminousV3PlayableCharacter::ConcealUnsafeVisualParts()
{
    for (USkeletalMeshComponent* Part : RuntimeParts)
    {
        if (Part) Part->SetVisibility(false, true);
    }
}

bool AOGLuminousV3PlayableCharacter::RefreshPrivacyRenderedAppearance()
{
    // Only the rendered representation is changed by Privacy. The actual
    // wardrobe, equipment, identity, Manifestation and canonical DB remain
    // completely untouched by this method.
    const bool bPrivacy = GetSfwPresentation();
    const bool bQAAuthorized = IsIsolatedWardrobeQAContext();
    const bool bHasSafeBase = ClothedNativeParts.Num() ==
        ExpectedPartsFor(EOGLuminousV3Wardrobe::Clothed);
    const FOGLuminousV3AppearanceProjection Decision =
        FOGLuminousV3AppearancePolicy::Resolve(
            CurrentWardrobe, bPrivacy, bQAAuthorized, bHasSafeBase);

    if (!Decision.bRenderable)
    {
        ConcealUnsafeVisualParts();
        EffectiveAppearance = Decision;
        LastLuminousError =
            TEXT("No safe/authorized Luminous presentation can be rendered.");
        return false;
    }

    if (!bLuminousReady ||
        Decision.EffectiveRenderedWardrobe != EffectiveAppearance.EffectiveRenderedWardrobe ||
        ActivePartCount != ExpectedPartsFor(Decision.EffectiveRenderedWardrobe))
    {
        FString Error;
        if (!ApplyVisualWardrobe(Decision.EffectiveRenderedWardrobe, Error))
        {
            ConcealUnsafeVisualParts();
            LastLuminousError = Error;
            return false;
        }
    }

    EffectiveAppearance = Decision;
    bLastAppliedPrivacy = bPrivacy;
    bLastAppliedExplicitContext = bQAAuthorized;
    UE_LOG(LogTemp, Display,
        TEXT("G12_LUMINOUS_PRIVACY_RENDER_ONLY actual=%s effective=%s sfw=%d masked=%d"),
        WardrobeName(CurrentWardrobe),
        WardrobeName(EffectiveAppearance.EffectiveRenderedWardrobe),
        bPrivacy ? 1 : 0,
        Decision.bMaskedForPrivacyOrUnavailableAdultContext ? 1 : 0);
    return true;
}

bool AOGLuminousV3PlayableCharacter::ResolveCanonicalControlledContext(
    FOGFoundationCharacterContext& OutContext, FString& Error) const
{
    OutContext = FOGFoundationCharacterContext();
    if (CanonicalLuminousIdentityId.IsEmpty() || CanonicalLuminousVersionId.IsEmpty())
    {
        Error = TEXT("Gate11 Luminous canonical IdentityId/VersionId not yet assigned; refusing persistent wardrobe modification");
        return false;
    }
    const UGameInstance* GI = GetGameInstance();
    const UOGGameCoreSubsystem* Core = GI ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Core->GetWorldStore() || Core->GetCanonicalWorldTick() < 0)
    {
        Error = TEXT("Canonical world store is not available");
        return false;
    }
    const UOGDiagnosticCombatComponent* Combat = FindComponentByClass<UOGDiagnosticCombatComponent>();
    if (!Combat || !Combat->IsActive())
    {
        Error = TEXT("No active selected party context; wardrobe selection not persisted");
        return false;
    }
    FOGEntityId Entity;
    FOGContentId Identity;
    bool bHasManifestation = false;
    FOGCharacterManifestationRecord Manifestation;
    if (!Combat->TryGetControlledContext(Entity, Identity, bHasManifestation, Manifestation, Error)) return false;
    if (Identity.ToString() != CanonicalLuminousIdentityId ||
        !bHasManifestation || Manifestation.ActiveVersionId.ToString() != CanonicalLuminousVersionId)
    {
        Error = TEXT("Current selected canonical character is not Luminous Valentine V3; refusing cross-identity save");
        return false;
    }
    OutContext.EntityId = Entity;
    OutContext.ManifestationId = Entity;
    OutContext.OwnerEntityId = Manifestation.OwningRulerId;
    return true;
}

bool AOGLuminousV3PlayableCharacter::PersistCanonicalWardrobe(
    EOGLuminousV3Wardrobe Desired, FString& Error)
{
    FOGFoundationCharacterContext Context;
    if (!ResolveCanonicalControlledContext(Context, Error)) return false;
    UOGGameCoreSubsystem* Core = GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    FOGFoundationCharacterRuntime Runtime(*Core->GetWorldStore());
    FOGFoundationCharacterProjection Projection;
    if (!Runtime.Project(Context, Projection, Error)) return false;
    const FString Json = Projection.bHasPresentation ? Projection.Presentation.OutfitStateJson : TEXT("{}");
    TSharedPtr<FJsonObject> Outfit;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json.IsEmpty() ? TEXT("{}") : Json), Outfit) ||
        !Outfit.IsValid())
    {
        Error = TEXT("Existing canonical outfit JSON invalid; preserving it without overwrite");
        return false;
    }
    Outfit->SetStringField(TEXT("luminous_v3_wardrobe"), WardrobeName(Desired));
    FString Updated;
    FJsonSerializer::Serialize(Outfit.ToSharedRef(), TJsonWriterFactory<>::Create(&Updated));
    FOGManifestationPresentationStateRecord Presentation =
        Projection.bHasPresentation ? Projection.Presentation : FOGManifestationPresentationStateRecord();
    Presentation.OutfitStateJson = Updated;
    if (Presentation.PresentationVariantStateJson.IsEmpty()) Presentation.PresentationVariantStateJson = TEXT("{}");
    return Runtime.SetPresentation(Context, Presentation, Core->GetCanonicalWorldTick(), Error);
}

bool AOGLuminousV3PlayableCharacter::RestoreCanonicalWardrobe()
{
    FString Error;
    FOGFoundationCharacterContext Context;
    if (!ResolveCanonicalControlledContext(Context, Error)) return false;
    UOGGameCoreSubsystem* Core = GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    FOGFoundationCharacterRuntime Runtime(*Core->GetWorldStore());
    FOGFoundationCharacterProjection Projection;
    if (!Runtime.Project(Context, Projection, Error) || !Projection.bHasPresentation) return false;
    TSharedPtr<FJsonObject> Outfit;
    if (!FJsonSerializer::Deserialize(
        TJsonReaderFactory<>::Create(Projection.Presentation.OutfitStateJson), Outfit) || !Outfit.IsValid())
        return false;
    FString Value;
    EOGLuminousV3Wardrobe Saved;
    if (!Outfit->TryGetStringField(TEXT("luminous_v3_wardrobe"), Value) || !ParseWardrobe(Value, Saved))
        return false;
    const EOGLuminousV3Wardrobe Before = CurrentWardrobe;
    CurrentWardrobe = Saved;
    if (!RefreshPrivacyRenderedAppearance())
    {
        CurrentWardrobe = Before;
        return false;
    }
    return true;
}

bool AOGLuminousV3PlayableCharacter::SetLuminousWardrobe(
    EOGLuminousV3Wardrobe Desired, bool bPersist)
{
    LastLuminousError.Reset();
    if (ExpectedPartsFor(Desired) == 0) { LastLuminousError = TEXT("Unknown wardrobe"); return false; }
    // Persistence preflight before visual switch: do not display successful
    // canonical selection when registry and selected party are not authorized.
    if (bPersist)
    {
        FOGFoundationCharacterContext Context;
        if (!ResolveCanonicalControlledContext(Context, LastLuminousError)) return false;
    }
    // Production explicit-asset eligibility must come from accepted Gate11
    // character lore/asset contracts. No writable Blueprint override or guessed
    // maturity flag is trusted. Only isolated engineering QA may preview them.
    if (Desired != EOGLuminousV3Wardrobe::Clothed &&
        !IsIsolatedWardrobeQAContext())
    {
        LastLuminousError =
            TEXT("Explicit outfit selection requires approved canonical adult context; only isolated QA can preview it.");
        return false;
    }
    if (!ValidateWardrobeAssets(Desired, LastLuminousError)) return false;

    const EOGLuminousV3Wardrobe Before = CurrentWardrobe;
    CurrentWardrobe = Desired;
    if (!RefreshPrivacyRenderedAppearance())
    {
        const FString FailedRenderError = LastLuminousError;
        CurrentWardrobe = Before;
        RefreshPrivacyRenderedAppearance();
        LastLuminousError = FailedRenderError;
        return false;
    }
    if (bPersist && !PersistCanonicalWardrobe(Desired, LastLuminousError))
    {
        const FString FailedSaveError = LastLuminousError;
        CurrentWardrobe = Before;
        RefreshPrivacyRenderedAppearance();
        LastLuminousError = FailedSaveError;
        return false;
    }
    return true;
}

bool AOGLuminousV3PlayableCharacter::SetLuminousNativeMotionClip(UAnimSequence* Clip, bool bLoop)
{
    LastLuminousError.Reset();
    if (!Clip || !bLuminousReady || ActivePartCount < 4 || !RuntimeParts.IsValidIndex(3))
    {
        LastLuminousError = TEXT("Motion V3 requires a native Luminous character and animation asset");
        return false;
    }
    USkeletalMeshComponent* Leader = RuntimeParts[3];
    if (!Leader || !Leader->GetSkeletalMeshAsset() ||
        Clip->GetSkeleton() != Leader->GetSkeletalMeshAsset()->GetSkeleton())
    {
        LastLuminousError = TEXT("Refused foreign/fixture animation: native skeleton asset mismatch");
        return false;
    }
    for (int32 Index = 0; Index < ActivePartCount; ++Index)
    {
        const USkeletalMeshComponent* Component = RuntimeParts[Index];
        if (!Component || !SameBoneTopologyAndRest(
                Leader->GetSkeletalMeshAsset(), Component->GetSkeletalMeshAsset()))
        {
            LastLuminousError = FString::Printf(
                TEXT("Cannot share Motion V3 leader pose with differing native rest rig part=%d"), Index);
            return false;
        }
    }
    for (int32 Index = 0; Index < ActivePartCount; ++Index)
        if (Index != 3) RuntimeParts[Index]->SetLeaderPoseComponent(Leader);
    Leader->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Leader->PlayAnimation(Clip, bLoop);
    CurrentNativeClip = Clip;
    bCompatibleNativeMotion = true;
    UE_LOG(LogTemp, Display, TEXT("G12_LUMINOUS_975_NATIVE_MOTION_STARTED=%s skeleton=%s source_clips=22_bone_tracks"),
        *Clip->GetName(), *Clip->GetSkeleton()->GetPathName());
    return true;
}

UAnimSequence* AOGLuminousV3PlayableCharacter::ResolveAuthoredMotionClip(
    EOGLuminousV3MotionPhase Phase) const
{
    const TArray<FSoftObjectPath>* Clips = nullptr;
    switch (Phase)
    {
    case EOGLuminousV3MotionPhase::Walk: Clips = &NativeWalkByWardrobe; break;
    case EOGLuminousV3MotionPhase::Run: Clips = &NativeRunByWardrobe; break;
    case EOGLuminousV3MotionPhase::Jump: Clips = &NativeJumpByWardrobe; break;
    default: return nullptr;
    }
    // Animation must follow the ACTUALLY RENDERED 975-bone skeleton, which
    // can differ from the real clothing state under Privacy/SFW masking.
    const int32 State = static_cast<int32>(EffectiveAppearance.EffectiveRenderedWardrobe);
    if (!Clips || Clips->Num() != 3 || !Clips->IsValidIndex(State)) return nullptr;
    if (!(*Clips)[State].ToString().StartsWith(
        TEXT("/Game/Experimental/LuminousValentineV3/NativeMocapV3/"))) return nullptr;
    return Cast<UAnimSequence>((*Clips)[State].TryLoad());
}

void AOGLuminousV3PlayableCharacter::UpdateNativeMotionFromTraversal()
{
    if (!bLuminousReady) return;
    const UCharacterMovementComponent* Movement = GetCharacterMovement();
    EOGLuminousV3MotionPhase Desired = EOGLuminousV3MotionPhase::Idle;
    if (Movement && Movement->IsFlying())
        Desired = EOGLuminousV3MotionPhase::Flying;
    else if (Movement && Movement->IsSwimming())
        Desired = EOGLuminousV3MotionPhase::Swimming;
    else if (Movement && Movement->IsFalling())
        Desired = GetVelocity().Z > 0.f ?
            EOGLuminousV3MotionPhase::Jump : EOGLuminousV3MotionPhase::Fall;
    else
    {
        const FVector Horizontal(GetVelocity().X, GetVelocity().Y, 0.0f);
        if (Horizontal.SizeSquared2D() > FMath::Square(12.0f))
        {
            const FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(Horizontal);
            // Only source-authorised forward CMU gait is available today;
            // never replay forward gait while travelling backwards/sideways.
            if (LocalVelocity.X > 0.0f &&
                FMath::Abs(LocalVelocity.Y) <= LocalVelocity.X * 0.65f)
                Desired = IsSprinting() || Horizontal.Size2D() > 430.0f ?
                    EOGLuminousV3MotionPhase::Run : EOGLuminousV3MotionPhase::Walk;
        }
    }
    if (Desired == CurrentMotionPhase) return;
    CurrentMotionPhase = Desired;
    if (Desired == EOGLuminousV3MotionPhase::Idle ||
        Desired == EOGLuminousV3MotionPhase::Fall ||
        Desired == EOGLuminousV3MotionPhase::Flying ||
        Desired == EOGLuminousV3MotionPhase::Swimming)
    {
        if (RuntimeParts.IsValidIndex(3) && RuntimeParts[3])
            RuntimeParts[3]->Stop();
        UE_LOG(LogTemp, Display,
            TEXT("G12_LUMINOUS_MOTION_STATE_NEUTRAL=%d no_wrong_ground_gait_in_flight_or_swim"),
            static_cast<int32>(Desired));
        return;
    }
    UAnimSequence* Sequence = ResolveAuthoredMotionClip(Desired);
    if (!Sequence || !SetLuminousNativeMotionClip(
        Sequence, Desired != EOGLuminousV3MotionPhase::Jump))
    {
        UE_LOG(LogTemp, Error, TEXT("G12_LUMINOUS_MOTION_NATIVE_INCOMPATIBLE state=%d: %s"),
            static_cast<int32>(Desired), *LastLuminousError);
    }
}

void AOGLuminousV3PlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent) return;
    PlayerInputComponent->BindKey(EKeys::Gamepad_DPad_Right, IE_Pressed,
        this, &AOGLuminousV3PlayableCharacter::CycleLuminousWardrobeFromGamepad);
    PlayerInputComponent->BindKey(EKeys::T, IE_Pressed,
        this, &AOGLuminousV3PlayableCharacter::CycleLuminousWardrobeFromGamepad);
}

void AOGLuminousV3PlayableCharacter::CycleLuminousWardrobeFromGamepad()
{
    if (!bEnableNativeWardrobeQATouchPanel) return;
    const int32 NextIndex = (static_cast<int32>(CurrentWardrobe) + 1) % 3;
    const bool bSuccess = SetLuminousWardrobe(
        static_cast<EOGLuminousV3Wardrobe>(NextIndex), false);
    UE_LOG(LogTemp, Display,
        TEXT("G12_LUMINOUS_GAMEPAD_WARDROBE_CYCLE=%d result=%s"),
        NextIndex, bSuccess ? TEXT("PASS") : TEXT("FAIL"));
}

void AOGLuminousV3PlayableCharacter::BeginPlay()
{
    Super::BeginPlay();
    FString Error;
    CurrentWardrobe = InitialWardrobe;
    if (!RefreshPrivacyRenderedAppearance())
    {
        const FString Error = LastLuminousError;
        UE_LOG(LogTemp, Error, TEXT("Gate12 Luminous native startup failed: %s"), *Error);
        return;
    }
    // Only applies if the selected saved canonical character is *actually*
    // this Gate11 Luminous identity; it cannot overwrite a Ruler/another copy.
    RestoreCanonicalWardrobe();
    if ((bRunNonPersistentWardrobeQACycle || bEnterWorldModeOnIsolatedQAStartup) &&
        bLuminousReady && GetWorld())
        GetWorldTimerManager().SetTimer(QAWorldModeTimer, this,
            &AOGLuminousV3PlayableCharacter::ActivateNativeQAWorldMode, 1.25f, false);
}

void AOGLuminousV3PlayableCharacter::ActivateNativeQAWorldMode()
{
    if ((!bRunNonPersistentWardrobeQACycle && !bEnterWorldModeOnIsolatedQAStartup) ||
        !bLuminousReady || !GetWorld()) return;
    UGameInstance* GI = GetGameInstance();
    UOGPresentationModeSubsystem* Presentation = GI ? GI->GetSubsystem<UOGPresentationModeSubsystem>() : nullptr;
    if (!Presentation)
    {
        UE_LOG(LogTemp, Error, TEXT("G12_LUMINOUS_NATIVE_QA_WORLD_MODE=FAIL_NO_SUBSYSTEM"));
        return;
    }
    Presentation->SetMode(EOGPresentationMode::World);
    UE_LOG(LogTemp, Display, TEXT("G12_LUMINOUS_NATIVE_QA_WORLD_MODE=%s"),
        Presentation->GetMode() == EOGPresentationMode::World ? TEXT("PASS") : TEXT("FAIL"));
    QANextCycleSeconds = GetWorld()->GetTimeSeconds() + 3.0f;
}

void AOGLuminousV3PlayableCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bLuminousReady || !GetWorld()) return;
    if (GetSfwPresentation() != bLastAppliedPrivacy ||
        IsIsolatedWardrobeQAContext() != bLastAppliedExplicitContext)
    {
        // Device Privacy is non-authoritative, so toggling it cannot call
        // SetLuminousWardrobe(bPersist=true) or modify the canonical store.
        RefreshPrivacyRenderedAppearance();
    }
    if (bEnableNativeWardrobeQATouchPanel && !WardrobeQAWidget &&
        IsLocallyControlled() && GetController())
    {
        WardrobeQAWidget = CreateWidget<UOGLuminousV3WardrobeQAWidget>(
            GetWorld(), UOGLuminousV3WardrobeQAWidget::StaticClass());
        if (WardrobeQAWidget)
        {
            WardrobeQAWidget->SetTargetCharacter(this);
            WardrobeQAWidget->AddToViewport(70);
            UE_LOG(LogTemp, Display, TEXT("G12_LUMINOUS_TOUCH_WARDROBE_PANEL=CREATED"));
        }
    }
    const float Now = GetWorld()->GetTimeSeconds();
    if (bRunNonPersistentWardrobeQACycle && QACycleStep < 3 &&
        Now >= QANextCycleSeconds)
    {
        const EOGLuminousV3Wardrobe Target = QACycleStep == 0 ? EOGLuminousV3Wardrobe::Underwear :
            QACycleStep == 1 ? EOGLuminousV3Wardrobe::Nude : EOGLuminousV3Wardrobe::Clothed;
        const bool bSuccess = SetLuminousWardrobe(Target, false);
        UE_LOG(LogTemp, Display, TEXT("G12_LUMINOUS_NATIVE_QA_WARDROBE_STEP=%d target=%s result=%s native_parts=%d error=%s"),
            QACycleStep, WardrobeName(Target), bSuccess ? TEXT("PASS") : TEXT("FAIL"),
            ActivePartCount, *LastLuminousError);
        if (!bSuccess) { QACycleStep = 3; return; }
        ++QACycleStep;
        QANextCycleSeconds = Now + 3.0f;
    }
    // Only QA map: after the three wardrobe transitions, prove three genuine
    // Luminous native 975-bone clips can actually play on the real character.
    if (bRunNativeMotionQACycle)
    {
        if (bRunNonPersistentWardrobeQACycle && QACycleStep < 3) return;
        if (QANextMotionSeconds <= 0.f) QANextMotionSeconds = Now + 2.0f;
        if (QAMotionCycleStep < 3 && Now >= QANextMotionSeconds)
        {
            const EOGLuminousV3MotionPhase Phase = QAMotionCycleStep == 0 ?
                EOGLuminousV3MotionPhase::Walk : QAMotionCycleStep == 1 ?
                EOGLuminousV3MotionPhase::Run : EOGLuminousV3MotionPhase::Jump;
            UAnimSequence* Clip = ResolveAuthoredMotionClip(Phase);
            const bool bValid = Clip && SetLuminousNativeMotionClip(
                Clip, Phase != EOGLuminousV3MotionPhase::Jump);
            CurrentMotionPhase = Phase;
            UE_LOG(LogTemp, Display,
                TEXT("G12_LUMINOUS_NATIVE_MOTION_QA_STEP=%d motion=%d result=%s active_native_part_count=%d clip=%s error=%s"),
                QAMotionCycleStep, static_cast<int32>(Phase),
                bValid ? TEXT("PASS") : TEXT("FAIL"), ActivePartCount,
                Clip ? *Clip->GetName() : TEXT("MISSING"), *LastLuminousError);
            if (!bValid) { QAMotionCycleStep = 3; return; }
            ++QAMotionCycleStep;
            QANextMotionSeconds = Now + 3.0f;
        }
        return;
    }
    UpdateNativeMotionFromTraversal();
}

void AOGLuminousV3PlayableCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if (WardrobeQAWidget)
    {
        WardrobeQAWidget->RemoveFromParent();
        WardrobeQAWidget = nullptr;
    }
    GetWorldTimerManager().ClearTimer(QAWorldModeTimer);
    CurrentNativeClip = nullptr;
    RuntimeParts.Reset();
    bCompatibleNativeMotion = false;
    bLuminousReady = false;
    Super::EndPlay(Reason);
}
