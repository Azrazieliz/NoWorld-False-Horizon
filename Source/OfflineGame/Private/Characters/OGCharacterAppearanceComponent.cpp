#include "Characters/OGCharacterAppearanceComponent.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Animation/Skeleton.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"
#include "Persistence/OGWorldStore.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
constexpr float CanonicalAppearanceRefreshIntervalSeconds = 0.5f;

bool AppearanceError(FString& Error, const TCHAR* Message)
{
    Error = Message;
    return false;
}

void DestroyAppearanceComponent(UActorComponent* Component)
{
    if (!IsValid(Component)) return;
    if (AActor* Owner = Component->GetOwner()) Owner->RemoveInstanceComponent(Component);
    Component->DestroyComponent();
}

bool MatchesRig(const FReferenceSkeleton& Rig, const FOGCharacterAppearancePart& Part)
{
    if (Rig.GetNum() != Part.BoneNames.Num() || Rig.GetRefBonePose().Num() != Rig.GetNum()) return false;
    for (int32 Index = 0; Index < Rig.GetNum(); ++Index)
    {
        if (Rig.GetBoneName(Index) != Part.BoneNames[Index] ||
            Rig.GetParentIndex(Index) != Part.BoneParentIndices[Index] ||
            (!Part.ReferenceLocalTransforms.IsEmpty() &&
             !Rig.GetRefBonePose()[Index].Equals(Part.ReferenceLocalTransforms[Index], 0.0001f)))
            return false;
    }
    return true;
}

bool IdenticalRig(const USkeletalMesh* A, const USkeletalMesh* B)
{
    const FReferenceSkeleton& Left = A->GetRefSkeleton();
    const FReferenceSkeleton& Right = B->GetRefSkeleton();
    if (Left.GetNum() != Right.GetNum()) return false;
    for (int32 Index = 0; Index < Left.GetNum(); ++Index)
    {
        if (Left.GetBoneName(Index) != Right.GetBoneName(Index) ||
            Left.GetParentIndex(Index) != Right.GetParentIndex(Index) ||
            !Left.GetRefBonePose()[Index].Equals(Right.GetRefBonePose()[Index], 0.0001f)) return false;
    }
    return true;
}
}

UOGCharacterAppearanceComponent::UOGCharacterAppearanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

UOGCharacterAppearanceDefinition* UOGCharacterAppearanceComponent::FindDefinition(
    const FOGContentId& Identity, const FOGContentId& Version, FString& Error) const
{
    TStrongObjectPtr<UOGCharacterAppearanceDefinition> Match;
    for (const TSoftObjectPtr<UOGCharacterAppearanceDefinition>& Reference : Definitions)
    {
        TStrongObjectPtr<UOGCharacterAppearanceDefinition> Definition(Reference.LoadSynchronous());
        if (!Definition.IsValid() || Definition->GetPathName() != Reference.ToSoftObjectPath().ToString())
        {
            Error = TEXT("An authored appearance definition is missing, redirected or has the wrong native class.");
            return nullptr;
        }
        if (Definition->IdentityId != Identity || Definition->VersionId != Version) continue;
        if (Match.IsValid())
        {
            Error = TEXT("Duplicate exact Identity/Version appearance definitions.");
            return nullptr;
        }
        Match = MoveTemp(Definition);
    }
    if (!Match.IsValid()) Error = TEXT("No authored native appearance for this exact Identity/Version.");
    return Match.Get();
}

bool UOGCharacterAppearanceComponent::ReadCanonicalWardrobe(
    const FOwnedBinding& Binding, const UOGCharacterAppearanceDefinition* Definition,
    FName& Wardrobe, UOGGameCoreSubsystem*& Core, FString& Error) const
{
    Wardrobe = NAME_None;
    Core = nullptr;
    if (!Definition || !Binding.Ruler.IsValid() || !Binding.Manifestation.IsValid() ||
        !Binding.Identity.IsValid() || !Binding.Version.IsValid() ||
        Definition->IdentityId != Binding.Identity || Definition->VersionId != Binding.Version)
        return AppearanceError(Error, TEXT("Appearance binding requires exact valid canonical identifiers."));
    UGameInstance* Game = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    Core = Game ? Game->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen())
        return AppearanceError(Error, TEXT("Live canonical Core Store is unavailable."));

    bool bFound = false;
    FName Kind;
    FString State;
    int64 Revision = 0;
    if (!Store->TryReadEntity(Binding.Ruler, bFound, Kind, State, Revision, Error)) return false;
    if (!bFound || Kind != FName(TEXT("ruler")) || State.Contains(TEXT("\"diagnosticFixture\"")))
        return AppearanceError(Error, TEXT("Appearance owner must be an existing canonical Ruler, not a fixture."));
    FOGCharacterManifestationRecord Record;
    if (!Store->TryReadCharacterManifestation(Binding.Manifestation, bFound, Record, Error)) return false;
    if (!bFound || Record.ManifestationId != Binding.Manifestation || Record.OwningRulerId != Binding.Ruler ||
        Record.IdentityId != Binding.Identity || Record.ActiveVersionId != Binding.Version ||
        Record.LifecycleState != FName(TEXT("active")))
        return AppearanceError(Error, TEXT("Exact owner/Identity/Version/active lifecycle binding no longer exists."));

    FOGManifestationPresentationStateRecord Presentation;
    if (!Store->TryReadManifestationPresentationState(Binding.Manifestation, bFound, Presentation, Error)) return false;
    Wardrobe = Definition->SafeDefaultWardrobe;
    if (!bFound) return true;
    if (Presentation.ManifestationId != Binding.Manifestation)
        return AppearanceError(Error, TEXT("Presentation record does not belong to the exact Manifestation."));
    TSharedPtr<FJsonObject> Outfit;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Presentation.OutfitStateJson), Outfit) || !Outfit.IsValid())
        return AppearanceError(Error, TEXT("Stored OutfitStateJson is malformed; appearance was not changed."));
    const FString Key = Definition->WardrobeStateJsonField.ToString();
    if (!Outfit->HasField(Key)) return true;
    FString Saved;
    if (!Outfit->TryGetStringField(Key, Saved) || Saved.IsEmpty())
        return AppearanceError(Error, TEXT("Stored authored wardrobe field must be a nonempty string."));
    const FOGCharacterAppearanceWardrobe* Row = Definition->FindWardrobe(FName(*Saved));
    if (!Row || Row->WardrobeId.ToString() != Saved)
        return AppearanceError(Error, TEXT("Stored wardrobe is not an exact authored choice for this Version."));
    Wardrobe = Row->WardrobeId;
    return true;
}

bool UOGCharacterAppearanceComponent::BindOwnedManifestation(
    const FOGEntityId& Ruler, const FOGContentId& Identity, const FOGContentId& Version,
    const FOGEntityId& Manifestation, bool bSfw, FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bInstalling)
        return AppearanceError(Error, TEXT("Appearance binding must run without reentry on the game thread."));
    TStrongObjectPtr<UOGCharacterAppearanceDefinition> Definition(FindDefinition(Identity, Version, Error));
    if (!Definition.IsValid() || !Definition->ValidateContract(Error)) return false;
    FOwnedBinding Binding;
    Binding.Ruler = Ruler;
    Binding.Identity = Identity;
    Binding.Version = Version;
    Binding.Manifestation = Manifestation;
    FName Wardrobe;
    UOGGameCoreSubsystem* Core = nullptr;
    if (!ReadCanonicalWardrobe(Binding, Definition.Get(), Wardrobe, Core, Error)) return false;
    return InstallAppearance(Definition.Get(), Wardrobe, bSfw, &Binding, Error);
}

bool UOGCharacterAppearanceComponent::InitializePreviewDefinition(
    UOGCharacterAppearanceDefinition* Definition, FName Wardrobe, bool bSfw, FString& Error)
{
    Error.Reset();
    if (bOwnedBound)
        return AppearanceError(Error, TEXT("A preview cannot replace an owned Manifestation binding."));
    return InstallAppearance(Definition, Wardrobe, bSfw, nullptr, Error);
}

bool UOGCharacterAppearanceComponent::ApplyWardrobe(FName Wardrobe, bool bSfw, FString& Error)
{
    Error.Reset();
    if (!ActiveDefinition || (!bOwnedBound && !bPreviewInitialized))
        return AppearanceError(Error, TEXT("Bind a canonical Manifestation or explicitly initialize a real asset preview first."));
    return InstallAppearance(ActiveDefinition.Get(), Wardrobe, bSfw, bOwnedBound ? &OwnedBinding : nullptr, Error);
}

bool UOGCharacterAppearanceComponent::RefreshCanonicalAppearance(FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bInstalling || !bOwnedBound || !ActiveDefinition)
        return AppearanceError(Error, TEXT("Refresh requires a live owned appearance binding on the game thread."));
    if (!ActiveDefinition->ValidateContract(Error)) return false;
    FName Saved;
    UOGGameCoreSubsystem* Core = nullptr;
    if (!ReadCanonicalWardrobe(OwnedBinding, ActiveDefinition.Get(), Saved, Core, Error)) return false;
    if (Core != BoundCore.Get())
        return AppearanceError(Error, TEXT("Canonical Core lifetime changed; bind the owned Manifestation again."));
    if (Saved == RequestedWardrobe && HasCompleteNativeAppearance()) return true;
    return InstallAppearance(ActiveDefinition.Get(), Saved, bSfwPresentation, &OwnedBinding, Error);
}

bool UOGCharacterAppearanceComponent::MatchesOwnedManifestation(
    const FOGEntityId& Ruler, const FOGEntityId& Manifestation) const
{
    if (!IsInGameThread() || bInstalling || !bOwnedBound || !ActiveDefinition ||
        OwnedBinding.Ruler != Ruler || OwnedBinding.Manifestation != Manifestation ||
        !HasCompleteNativeAppearance()) return false;
    FName Saved;
    UOGGameCoreSubsystem* Core = nullptr;
    FString Error;
    return ReadCanonicalWardrobe(OwnedBinding, ActiveDefinition.Get(), Saved, Core, Error) &&
        Core == BoundCore.Get() && Saved == RequestedWardrobe;
}

bool UOGCharacterAppearanceComponent::InstallAppearance(
    UOGCharacterAppearanceDefinition* Definition, FName Wardrobe, bool bSfw,
    const FOwnedBinding* Binding, FString& Error)
{
    if (!IsInGameThread() || bInstalling || !IsRegistered() || !IsValid(GetOwner()) ||
        GetOwner()->IsActorBeingDestroyed() || !GetWorld() || !IsValid(Definition))
        return AppearanceError(Error, TEXT("Appearance installation requires a live registered owner, definition and non-reentrant game thread."));
    TGuardValue<bool> Installing(bInstalling, true);
    TStrongObjectPtr<UOGCharacterAppearanceDefinition> PinnedDefinition(Definition);
    const FOGCharacterAppearanceWardrobe* Resolved = Definition->ResolveRenderedWardrobe(Wardrobe, bSfw, Error);
    if (!Resolved) return false;
    const FOGCharacterAppearanceWardrobe RenderedSnapshot = *Resolved;
    const FOGCharacterAppearanceWardrobe* Rendered = &RenderedSnapshot;
    UOGGameCoreSubsystem* Core = nullptr;
    if (Binding)
    {
        FName Saved;
        if (!ReadCanonicalWardrobe(*Binding, Definition, Saved, Core, Error)) return false;
        if (Saved != Wardrobe)
            return AppearanceError(Error, TEXT("UI must persist the authored wardrobe before read-only appearance application."));
    }
    TStrongObjectPtr<UOGSharedMotionLibrary> Library(Definition->SharedMotionLibrary.LoadSynchronous());
    if (!Library.IsValid() || Library->GetPathName() != Definition->SharedMotionLibrary.ToSoftObjectPath().ToString())
        return AppearanceError(Error, TEXT("The exact authored shared motion library is missing or redirected."));

    // Validate every source mesh before allocating anything or touching the previous body/controller.
    TArray<TStrongObjectPtr<USkeletalMesh>> Meshes;
    for (const FOGCharacterAppearancePart& Part : Rendered->Parts)
    {
        TStrongObjectPtr<USkeletalMesh> Mesh(Part.Mesh.LoadSynchronous());
        TStrongObjectPtr<USkeleton> Skeleton(Part.NativeSkeleton.LoadSynchronous());
        if (!Mesh.IsValid() || !Skeleton.IsValid() || Mesh->GetSkeleton() != Skeleton.Get() ||
            Mesh->GetPathName() != Part.Mesh.ToSoftObjectPath().ToString() ||
            Skeleton->GetPathName() != Part.NativeSkeleton.ToSoftObjectPath().ToString() ||
            !MatchesRig(Mesh->GetRefSkeleton(), Part) || !MatchesRig(Skeleton->GetReferenceSkeleton(), Part) ||
            Mesh->GetMorphTargets().Num() != Part.MorphTargets.Num())
        {
            Error = FString::Printf(TEXT("Part %s failed native mesh/class/skeleton/count/hierarchy/rest-pose contract."), *Part.PartId.ToString());
            return false;
        }
        for (FName Morph : Part.MorphTargets)
            if (!Mesh->FindMorphTarget(Morph))
            {
                Error = FString::Printf(TEXT("Part %s is missing native morph %s."), *Part.PartId.ToString(), *Morph.ToString());
                return false;
            }
        if (Rendered->bUseIdenticalTopologyLeaderPose && !Meshes.IsEmpty() && !IdenticalRig(Meshes[0].Get(), Mesh.Get()))
            return AppearanceError(Error, TEXT("Leader-pose sharing requires identical actual topology and local rest poses for every part."));
        Meshes.Add(MoveTemp(Mesh));
    }

    AActor* Owner = GetOwner();
    TArray<USceneComponent*> Scenes;
    Owner->GetComponents<USceneComponent>(Scenes);
    USceneComponent* Parent = AttachmentComponentName.IsNone() ? Owner->GetRootComponent() : nullptr;
    if (!AttachmentComponentName.IsNone())
    {
        for (USceneComponent* Scene : Scenes)
        {
            if (Scene->GetFName() != AttachmentComponentName) continue;
            if (Parent) return AppearanceError(Error, TEXT("Authored attachment component name is ambiguous."));
            Parent = Scene;
        }
    }
    if (!IsValid(Parent) || !Parent->IsRegistered() || Parent->GetOwner() != Owner)
        return AppearanceError(Error, TEXT("Authored attachment root is unavailable."));
    TArray<USkeletalMeshComponent*> Bodies;
    TSet<FName> BodyNames;
    for (FName Name : BodyComponentNamesToReplace)
    {
        USkeletalMeshComponent* Match = nullptr;
        if (Name.IsNone() || BodyNames.Contains(Name))
            return AppearanceError(Error, TEXT("Authored replaced body component names must be unique and nonempty."));
        BodyNames.Add(Name);
        for (USceneComponent* Scene : Scenes)
        {
            if (Scene->GetFName() != Name) continue;
            if (Match || !Cast<USkeletalMeshComponent>(Scene))
                return AppearanceError(Error, TEXT("Authored replaced body component is ambiguous or not a skeletal mesh component."));
            Match = Cast<USkeletalMeshComponent>(Scene);
        }
        if (!IsValid(Match) || !Match->IsRegistered() || Match == Parent || ActiveParts.Contains(Match))
            return AppearanceError(Error, TEXT("Authored body replacement must name an existing independent skeletal component."));
        Bodies.Add(Match);
    }

    TArray<USkeletalMeshComponent*> Candidates;
    UOGSharedMotionControllerComponent* CandidateController = nullptr;
    bool bPublished = false;
    ON_SCOPE_EXIT
    {
        if (!bPublished)
        {
            DestroyAppearanceComponent(CandidateController);
            for (USkeletalMeshComponent* Candidate : Candidates) DestroyAppearanceComponent(Candidate);
        }
    };
    for (int32 Index = 0; Index < Meshes.Num(); ++Index)
    {
        USkeletalMeshComponent* Candidate = NewObject<USkeletalMeshComponent>(Owner, NAME_None, RF_Transient);
        if (!Candidate) return AppearanceError(Error, TEXT("Could not allocate hidden native part."));
        Candidates.Add(Candidate);
        Candidate->SetVisibility(false, false);
        Candidate->SetHiddenInGame(true, false);
        Candidate->SetupAttachment(Parent);
        Candidate->SetRelativeTransform(Rendered->Parts[Index].ComponentTransform);
        Candidate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Candidate->SetGenerateOverlapEvents(false);
        Candidate->SetCanEverAffectNavigation(false);
        Candidate->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Candidate->SetSkeletalMeshAsset(Meshes[Index].Get());
        Owner->AddInstanceComponent(Candidate);
        Candidate->RegisterComponent();
        if (!Candidate->IsRegistered() || Candidate->GetSkeletalMeshAsset() != Meshes[Index].Get())
            return AppearanceError(Error, TEXT("Hidden candidate part registration failed."));
    }
    CandidateController = NewObject<UOGSharedMotionControllerComponent>(Owner, NAME_None, RF_Transient);
    if (!CandidateController) return AppearanceError(Error, TEXT("Could not allocate candidate shared motion controller."));
    CandidateController->SharedLibrary = Definition->SharedMotionLibrary;
    CandidateController->RigProfile = Rendered->RigProfile;
    CandidateController->AnatomyProfile = Rendered->AnatomyProfile;
    CandidateController->OutfitProfile = Rendered->OutfitProfile;
    CandidateController->Capabilities = Rendered->Capabilities;
    CandidateController->bUseIdenticalTopologyLeaderPose = Rendered->bUseIdenticalTopologyLeaderPose;
    CandidateController->bPhaseLockIndependentParts = true;
    CandidateController->bPermitAuthoritativeRootMotion = false;
    Owner->AddInstanceComponent(CandidateController);
    CandidateController->RegisterComponent();
    if (!CandidateController->IsRegistered() || !CandidateController->RegisterNativeParts(Candidates))
        return AppearanceError(Error, TEXT("Shared motion registration rejected the hidden native candidate parts."));

    // Loading/registering can invoke engine callbacks. Recheck canonical truth before publishing.
    if (!IsValid(Owner) || Owner->IsActorBeingDestroyed() || !IsValid(Parent) || !Parent->IsRegistered() ||
        !IsValid(CandidateController) || !CandidateController->IsRegistered())
        return AppearanceError(Error, TEXT("Appearance owner/attachment disappeared during preparation."));
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
        if (!IsValid(Candidates[Index]) || !Candidates[Index]->IsRegistered() ||
            Candidates[Index]->GetSkeletalMeshAsset() != Meshes[Index].Get())
            return AppearanceError(Error, TEXT("A prepared native part disappeared before publication."));
    for (USkeletalMeshComponent* Body : Bodies)
        if (!IsValid(Body) || !Body->IsRegistered())
            return AppearanceError(Error, TEXT("An authored replaced body disappeared before publication."));
    if (Binding)
    {
        FName Saved;
        if (!ReadCanonicalWardrobe(*Binding, Definition, Saved, Core, Error)) return false;
        if (Saved != Wardrobe)
            return AppearanceError(Error, TEXT("Canonical wardrobe changed during native candidate preparation."));
    }

    // All fallible checks are complete. Existing geometry/playback is untouched until this boundary.
    if (UOGGameCoreSubsystem* PreviousCore = BoundCore.Get())
        PreviousCore->OnCanonicalRuntimeReleasing.RemoveAll(this);
    if (IsValid(ActiveMotionController.Get()))
    {
        RemoveTickPrerequisiteComponent(ActiveMotionController.Get());
        ActiveMotionController->StopAction();
    }
    TArray<TObjectPtr<USkeletalMeshComponent>> PreviousParts = MoveTemp(ActiveParts);
    UOGSharedMotionControllerComponent* PreviousController = ActiveMotionController.Get();
    for (USkeletalMeshComponent* Previous : PreviousParts)
    {
        if (!IsValid(Previous)) continue;
        Previous->SetVisibility(false, false);
        Previous->SetHiddenInGame(true, false);
    }
    for (USkeletalMeshComponent* Body : Bodies)
    {
        if (!ReplacedBodyVisibility.ContainsByPredicate(
            [Body](const FBodyVisibility& Entry) { return Entry.Component.Get() == Body; }))
        {
            FBodyVisibility Saved;
            Saved.Component = Body;
            Saved.bVisible = Body->GetVisibleFlag();
            Saved.bHiddenInGame = Body->bHiddenInGame;
            ReplacedBodyVisibility.Add(Saved);
        }
        Body->SetVisibility(false, false);
        Body->SetHiddenInGame(true, false);
    }
    ActiveDefinition = Definition;
    ActiveWardrobeContract = *Rendered;
    for (USkeletalMeshComponent* Candidate : Candidates) ActiveParts.Add(Candidate);
    ActiveMotionController = CandidateController;
    RequestedWardrobe = Wardrobe;
    RenderedWardrobe = Rendered->WardrobeId;
    bSfwPresentation = bSfw;
    OwnedBinding = Binding ? *Binding : FOwnedBinding();
    bOwnedBound = Binding != nullptr;
    bPreviewInitialized = !bOwnedBound;
    CanonicalRefreshElapsedSeconds = 0.f;
    BoundCore = Core;
    if (Core) Core->OnCanonicalRuntimeReleasing.AddUObject(this, &UOGCharacterAppearanceComponent::HandleCanonicalRuntimeReleasing);
    // Preserve only semantics actually authored on the new exact rendered wardrobe.
    for (auto It = SemanticWeights.CreateIterator(); It; ++It)
        if (!Rendered->Expressions.ContainsByPredicate(
            [&It](const FOGCharacterSemanticExpression& Entry) { return Entry.Semantic == It.Key(); })) It.RemoveCurrent();
    AddTickPrerequisiteComponent(CandidateController);
    for (USkeletalMeshComponent* Candidate : Candidates) Candidate->AddTickPrerequisiteComponent(this);
    ApplyExpressionWeights();
    SetComponentTickEnabled(bOwnedBound || !SemanticWeights.IsEmpty());
    for (USkeletalMeshComponent* Candidate : Candidates)
    {
        Candidate->SetHiddenInGame(false, false);
        Candidate->SetVisibility(true, false);
    }
    bPublished = true;
    DestroyAppearanceComponent(PreviousController);
    for (USkeletalMeshComponent* Previous : PreviousParts) DestroyAppearanceComponent(Previous);
    return true;
}

bool UOGCharacterAppearanceComponent::HasCompleteNativeAppearance() const
{
    if (!IsValid(ActiveDefinition.Get()) || !IsValid(ActiveMotionController.Get()) ||
        !ActiveMotionController->IsRegistered() || ActiveWardrobeContract.ExpectedPartCount <= 0 ||
        ActiveParts.Num() != ActiveWardrobeContract.ExpectedPartCount ||
        ActiveParts.Num() != ActiveWardrobeContract.Parts.Num()) return false;
    for (int32 Index = 0; Index < ActiveParts.Num(); ++Index)
    {
        const USkeletalMeshComponent* Component = ActiveParts[Index].Get();
        const USkeletalMesh* Mesh = IsValid(Component) ? Component->GetSkeletalMeshAsset() : nullptr;
        const FOGCharacterAppearancePart& Part = ActiveWardrobeContract.Parts[Index];
        if (!Mesh || !Component->IsRegistered() || Mesh->GetPathName() != Part.Mesh.ToSoftObjectPath().ToString() ||
            !Mesh->GetSkeleton() || Mesh->GetSkeleton()->GetPathName() != Part.NativeSkeleton.ToSoftObjectPath().ToString() ||
            !MatchesRig(Mesh->GetRefSkeleton(), Part) || !MatchesRig(Mesh->GetSkeleton()->GetReferenceSkeleton(), Part) ||
            Mesh->GetMorphTargets().Num() != Part.MorphTargets.Num()) return false;
        for (FName Morph : Part.MorphTargets)
            if (!Mesh->FindMorphTarget(Morph)) return false;
    }
    return true;
}

bool UOGCharacterAppearanceComponent::ValidateCurrentBinding(FString& Error) const
{
    if (!HasCompleteNativeAppearance())
        return AppearanceError(Error, TEXT("No complete active native appearance."));
    if (!bOwnedBound) return bPreviewInitialized;
    FName Saved;
    UOGGameCoreSubsystem* Core = nullptr;
    if (!ReadCanonicalWardrobe(OwnedBinding, ActiveDefinition.Get(), Saved, Core, Error)) return false;
    if (Saved != RequestedWardrobe || Core != BoundCore.Get())
        return AppearanceError(Error, TEXT("Canonical presentation changed; rebind/apply its saved wardrobe first."));
    return true;
}

bool UOGCharacterAppearanceComponent::SetSemanticExpression(FName Semantic, float Weight, FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bInstalling || !FMath::IsFinite(Weight) || Weight < 0.f || Weight > 1.f)
        return AppearanceError(Error, TEXT("Expression requires a finite normalized weight and non-reentrant game thread."));
    if (!ValidateCurrentBinding(Error)) return false;
    const FOGCharacterSemanticExpression* Expression = ActiveWardrobeContract.Expressions.FindByPredicate(
        [Semantic](const FOGCharacterSemanticExpression& Entry) { return Entry.Semantic == Semantic; });
    if (!Expression) return AppearanceError(Error, TEXT("Semantic expression was not authored for the rendered wardrobe."));
    for (const FOGCharacterExpressionMorph& Morph : Expression->Morphs)
    {
        const int32 Index = ActiveWardrobeContract.Parts.IndexOfByPredicate(
            [&Morph](const FOGCharacterAppearancePart& Part) { return Part.PartId == Morph.PartId; });
        if (!ActiveParts.IsValidIndex(Index) || !IsValid(ActiveParts[Index].Get()) ||
            !ActiveParts[Index]->GetSkeletalMeshAsset() || !ActiveParts[Index]->GetSkeletalMeshAsset()->FindMorphTarget(Morph.Morph))
            return AppearanceError(Error, TEXT("An exact rendered part/morph binding disappeared; expression was not changed."));
    }
    SemanticWeights.Add(Semantic, Weight);
    ApplyExpressionWeights();
    SetComponentTickEnabled(true);
    return true;
}

void UOGCharacterAppearanceComponent::ApplyExpressionWeights()
{
    for (int32 Index = 0; Index < ActiveParts.Num(); ++Index)
    {
        if (!IsValid(ActiveParts[Index].Get()) || !ActiveWardrobeContract.Parts.IsValidIndex(Index)) continue;
        TMap<FName, float> Mapped;
        for (const FOGCharacterSemanticExpression& Expression : ActiveWardrobeContract.Expressions)
        {
            const float* Weight = SemanticWeights.Find(Expression.Semantic);
            if (!Weight) continue;
            for (const FOGCharacterExpressionMorph& Morph : Expression.Morphs)
                if (Morph.PartId == ActiveWardrobeContract.Parts[Index].PartId)
                    Mapped.FindOrAdd(Morph.Morph) += *Weight * Morph.Scale;
        }
        // Never ClearMorphTargets or broadcast same-named morphs to unrelated parts.
        for (const auto& Pair : Mapped) ActiveParts[Index]->SetMorphTarget(Pair.Key, FMath::Clamp(Pair.Value, -1.f, 1.f));
    }
}

bool UOGCharacterAppearanceComponent::PlaySemanticAction(FName Action, FName Context,
    bool bAdultContextAuthorized, int32 ParticipantCount, FName ParticipantRole, FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bInstalling)
        return AppearanceError(Error, TEXT("Motion playback requires a non-reentrant game thread."));
    if (!ValidateCurrentBinding(Error)) return false;
    if (!ActiveMotionController->PlayAction(Action, Context, bAdultContextAuthorized, ParticipantCount, ParticipantRole))
        return AppearanceError(Error, TEXT("Shared motion rejected the semantic action for the exact parts/rig/anatomy/capabilities/context."));
    return true;
}

void UOGCharacterAppearanceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bInstalling) return;
    if (bOwnedBound && FMath::IsFinite(DeltaTime) && DeltaTime > 0.f)
    {
        CanonicalRefreshElapsedSeconds += FMath::Min(DeltaTime, CanonicalAppearanceRefreshIntervalSeconds);
        if (CanonicalRefreshElapsedSeconds >= CanonicalAppearanceRefreshIntervalSeconds)
        {
            // One read-only refresh per interval, including on failure; never catch up in a loop.
            CanonicalRefreshElapsedSeconds = 0.f;
            FString Error;
            RefreshCanonicalAppearance(Error);
        }
    }
    // Expression overlays retain their per-frame ordering after the shared motion controller.
    if (!SemanticWeights.IsEmpty()) ApplyExpressionWeights();
}

void UOGCharacterAppearanceComponent::HandleCanonicalRuntimeReleasing()
{
    // Never retain a stale owned projection after clear/import/recovery. No Store pointers are cached.
    ReleaseAppearance(false);
}

void UOGCharacterAppearanceComponent::ReleaseAppearance(bool bRestoreAuthoredBody)
{
    if (UOGGameCoreSubsystem* Core = BoundCore.Get()) Core->OnCanonicalRuntimeReleasing.RemoveAll(this);
    BoundCore.Reset();
    SetComponentTickEnabled(false);
    CanonicalRefreshElapsedSeconds = 0.f;
    if (IsValid(ActiveMotionController.Get()))
    {
        RemoveTickPrerequisiteComponent(ActiveMotionController.Get());
        ActiveMotionController->StopAction();
        DestroyAppearanceComponent(ActiveMotionController.Get());
    }
    ActiveMotionController = nullptr;
    for (USkeletalMeshComponent* Part : ActiveParts) DestroyAppearanceComponent(Part);
    ActiveParts.Reset();
    ActiveDefinition = nullptr;
    ActiveWardrobeContract = FOGCharacterAppearanceWardrobe();
    SemanticWeights.Reset();
    RequestedWardrobe = RenderedWardrobe = NAME_None;
    bSfwPresentation = true;
    bOwnedBound = bPreviewInitialized = false;
    OwnedBinding = FOwnedBinding();
    if (bRestoreAuthoredBody)
    {
        for (const FBodyVisibility& Saved : ReplacedBodyVisibility)
            if (USceneComponent* Body = Saved.Component.Get())
            {
                Body->SetVisibility(Saved.bVisible, false);
                Body->SetHiddenInGame(Saved.bHiddenInGame, false);
            }
        ReplacedBodyVisibility.Reset();
    }
}

void UOGCharacterAppearanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseAppearance(true);
    Super::EndPlay(EndPlayReason);
}

void UOGCharacterAppearanceComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    ReleaseAppearance(true);
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}
