#include "Interaction/OGInteractionParticipantComponent.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Animation/Skeleton.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Runtime/OGCanonicalCharacterPresentation.h"
#include "Runtime/OGGameCoreSubsystem.h"

namespace
{
bool Fail(FString& Error, const TCHAR* Reason)
{
    Error = Reason;
    return false;
}
bool HasNativeGeometry(const USkeletalMeshComponent* Part, const AActor* Actor)
{
    return IsValid(Part) && Part->GetOwner() == Actor && Part->IsRegistered() &&
        Part->GetSkeletalMeshAsset() && Part->GetSkeletalMeshAsset()->GetSkeleton();
}
bool ReadRegisteredParts(AActor* Actor, const UOGSharedMotionControllerComponent* Motion,
    TArray<USkeletalMeshComponent*>& Out, FString& Error)
{
    Out.Reset();
    if (!IsValid(Motion) || Motion->GetOwner() != Actor || !Motion->IsRegistered() ||
        Motion->RigProfile.IsNone() || Motion->GetNativeParts().IsEmpty())
        return Fail(Error, TEXT("Configured native motion controller has no actual registered source part set."));
    TArray<USkeletalMeshComponent*> Verified;
    for (USkeletalMeshComponent* Part : Motion->GetNativeParts())
    {
        if (!HasNativeGeometry(Part, Actor) || Verified.Contains(Part))
            return Fail(Error, TEXT("Registered native source part is missing, duplicated, detached, or owned by another actor."));
        Verified.Add(Part);
    }
    Out = MoveTemp(Verified);
    return true;
}
bool SameSelection(const FOGFoundationCharacterProjection& A, const FOGFoundationCharacterProjection& B)
{
    return A.Context.EntityId == B.Context.EntityId &&
        A.Context.ManifestationId == B.Context.ManifestationId &&
        A.Context.OwnerEntityId == B.Context.OwnerEntityId &&
        A.bHasManifestation == B.bHasManifestation;
}
bool SameVersion(const FOGFoundationCharacterProjection& A, const FOGFoundationCharacterProjection& B)
{
    return !A.bHasManifestation || (A.Manifestation.IdentityId == B.Manifestation.IdentityId &&
        A.Manifestation.ActiveVersionId == B.Manifestation.ActiveVersionId &&
        A.Manifestation.LifecycleState == B.Manifestation.LifecycleState);
}
/** Validate existing evidence before a factory may configure ordinary presentation. */
bool CheckActorSelection(AActor* Actor, const FOGFoundationCharacterProjection& P,
    bool bRequireCurrentVersion, bool bRequireEvidence, FString& Error)
{
    if (!IsInGameThread() || !IsValid(Actor) || Actor->IsActorBeingDestroyed() || !Actor->GetWorld())
        return Fail(Error, TEXT("Canonical participant requires a live native world actor."));
    bool bEvidence = false;
    TArray<UOGCanonicalCharacterPresentation*> Canonicals;
    Actor->GetComponents<UOGCanonicalCharacterPresentation>(Canonicals);
    if (Canonicals.Num() > 1)
        return Fail(Error, TEXT("Actor has ambiguous canonical presentation components."));
    for (const auto* Canonical : Canonicals)
    {
        if (!Canonical->HasProjection()) continue;
        const auto& Existing = Canonical->GetProjection();
        if (!SameSelection(Existing, P))
            return Fail(Error, TEXT("Actor is actually projected to a different canonical entity or owner."));
        if (bRequireCurrentVersion && !SameVersion(Existing, P))
            return Fail(Error, TEXT("Actor's Version projection changed; refresh ordinary presentation before binding playback."));
        bEvidence = true;
    }
    TArray<UOGCharacterAppearanceComponent*> Appearances;
    Actor->GetComponents<UOGCharacterAppearanceComponent>(Appearances);
    int32 ActiveAppearances = 0;
    for (const auto* Appearance : Appearances)
    {
        if (!Appearance->GetActiveDefinition() && !Appearance->IsOwnedManifestationBound()) continue;
        if (++ActiveAppearances > 1 || !P.bHasManifestation ||
            !Appearance->IsOwnedManifestationBound() ||
            (bRequireCurrentVersion &&
                !Appearance->MatchesOwnedManifestation(P.Context.OwnerEntityId, P.Context.ManifestationId)))
            return Fail(Error, TEXT("Actual appearance is preview-only, incomplete, or belongs to another canonical Manifestation."));
        const auto* Definition = Appearance->GetActiveDefinition();
        if (!Definition || Definition->IdentityId != P.Manifestation.IdentityId ||
            Definition->VersionId != P.Manifestation.ActiveVersionId)
            return Fail(Error, TEXT("Actual appearance does not match the current canonical Identity/Version."));
        bEvidence = true;
    }
    return !bRequireEvidence || bEvidence ||
        Fail(Error, TEXT("Actor no longer has its canonical presentation/appearance binding."));
}
FOGContentId NativeRigFamily(const TArray<USkeletalMeshComponent*>& Parts,
    const UOGSharedMotionControllerComponent* Motion)
{
    if (Motion && !Motion->RigProfile.IsNone())
    {
        const FString Profile = Motion->RigProfile.ToString().ToLower();
        const FOGContentId Exact(Profile);
        return Exact.IsValid() ? Exact : FOGContentId(FString(TEXT("rig:")) + Profile);
    }
    if (!Parts.IsEmpty() && Parts[0]->GetSkeletalMeshAsset())
        return FOGContentId(FString(TEXT("rig:")) +
            Parts[0]->GetSkeletalMeshAsset()->GetSkeleton()->GetPathName().ToLower());
    return FOGContentId();
}
}

bool FOGInteractionParticipantSourceProfile::Matches(const FOGFoundationCharacterProjection& P) const
{
    const bool bKnownEnum = CanonicalMaturity == EOGCanonicalMaturity::Unknown ||
        CanonicalMaturity == EOGCanonicalMaturity::NonAdult || CanonicalMaturity == EOGCanonicalMaturity::Adult;
    if (!EntityId.IsValid() || EntityId != P.Context.EntityId || !SourceProfileId.IsValid() ||
        SourceRevision.TrimStartAndEnd().IsEmpty() || !bKnownEnum) return false;
    if (P.bHasManifestation)
        return IdentityId.IsValid() && VersionId.IsValid() &&
            IdentityId == P.Manifestation.IdentityId && VersionId == P.Manifestation.ActiveVersionId;
    return IdentityId.IsEmpty() && VersionId.IsEmpty();
}

bool FOGInteractionParticipantSourceProfile::FromDefinitions(const FOGFoundationCharacterProjection& P,
    const FOGCharacterIdentityDefinition& Identity, const FOGCharacterVersionDefinition& Version,
    const FOGContentId& SourceId, const FString& Revision, const FString& Age,
    FOGInteractionParticipantSourceProfile& Out, FString& Error)
{
    Out = FOGInteractionParticipantSourceProfile();
    Error.Reset();
    if (!P.bHasManifestation || Identity.IdentityId != P.Manifestation.IdentityId ||
        Version.IdentityId != Identity.IdentityId || Version.VersionId != P.Manifestation.ActiveVersionId)
        return Fail(Error, TEXT("Original Identity/Version definitions must exactly match the current canonical Manifestation."));
    FOGInteractionParticipantSourceProfile Candidate;
    Candidate.EntityId = P.Context.EntityId;
    Candidate.IdentityId = Identity.IdentityId;
    Candidate.VersionId = Version.VersionId;
    Candidate.SourceProfileId = SourceId;
    Candidate.SourceRevision = Revision;
    Candidate.AuthoredAge = Age;
    Candidate.CanonicalMaturity = Identity.CanonicalMaturity;
    if (!Candidate.Matches(P))
        return Fail(Error, TEXT("Original source profile requires exact selectors and authored source ID/revision."));
    Out = MoveTemp(Candidate);
    return true;
}

UOGInteractionParticipantComponent::UOGInteractionParticipantComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UOGInteractionParticipantComponent::DiscoverNativeBinding(AActor* Actor,
    TArray<USkeletalMeshComponent*>& Out, UOGSharedMotionControllerComponent*& Motion, FString& Error)
{
    Out.Reset();
    Motion = nullptr;
    Error.Reset();
    if (!IsInGameThread() || !IsValid(Actor) || Actor->IsActorBeingDestroyed())
        return Fail(Error, TEXT("Native participant actor is unavailable."));
    TArray<UOGCharacterAppearanceComponent*> Appearances;
    Actor->GetComponents<UOGCharacterAppearanceComponent>(Appearances);
    UOGCharacterAppearanceComponent* ActiveAppearance = nullptr;
    for (auto* Appearance : Appearances)
        if (Appearance->GetActiveDefinition())
        {
            if (ActiveAppearance) return Fail(Error, TEXT("Native actor has multiple active appearances."));
            ActiveAppearance = Appearance;
        }
    TArray<USkeletalMeshComponent*> Parts;
    UOGSharedMotionControllerComponent* CandidateMotion = nullptr;
    if (ActiveAppearance)
    {
        const auto* Definition = ActiveAppearance->GetActiveDefinition();
        const auto* Wardrobe = Definition->FindWardrobe(ActiveAppearance->RenderedWardrobe);
        CandidateMotion = ActiveAppearance->GetMotionController();
        if (!Wardrobe || Wardrobe->Parts.IsEmpty() || Wardrobe->ExpectedPartCount != Wardrobe->Parts.Num())
            return Fail(Error, TEXT("Active multipart appearance has an incomplete native part/controller contract."));
        if (!ReadRegisteredParts(Actor, CandidateMotion, Parts, Error)) return false;
        if (Parts.Num() != Wardrobe->Parts.Num())
            return Fail(Error, TEXT("Actual registered appearance parts differ from the authored native wardrobe."));
        for (int32 Index = 0; Index < Wardrobe->Parts.Num(); ++Index)
        {
            const auto& Authored = Wardrobe->Parts[Index];
            const auto* Part = Parts[Index];
            if (ActiveAppearance->BodyComponentNamesToReplace.Contains(Part->GetFName()) ||
                Part->GetSkeletalMeshAsset()->GetPathName() != Authored.Mesh.ToSoftObjectPath().ToString() ||
                Part->GetSkeletalMeshAsset()->GetSkeleton()->GetPathName() != Authored.NativeSkeleton.ToSoftObjectPath().ToString() ||
                !Part->GetRelativeTransform().Equals(Authored.ComponentTransform))
                return Fail(Error, TEXT("Actual registered appearance part differs from its exact authored native source."));
        }
    }
    else
    {
        TArray<UOGSharedMotionControllerComponent*> Controllers;
        Actor->GetComponents<UOGSharedMotionControllerComponent>(Controllers);
        for (auto* Controller : Controllers)
        {
            // An unconfigured controller does not remove a physical participant
            // from ordinary interaction. It simply offers no shared playback.
            if (!IsValid(Controller) || !Controller->IsRegistered() ||
                Controller->RigProfile.IsNone() || Controller->GetNativeParts().IsEmpty()) continue;
            if (CandidateMotion) return Fail(Error, TEXT("Original native actor has ambiguous motion controllers."));
            CandidateMotion = Controller;
        }
        if (CandidateMotion)
        {
            // This exact source-owned list excludes inherited tutorial meshes,
            // obsolete bodies and unrelated accessories, even when hidden.
            if (!ReadRegisteredParts(Actor, CandidateMotion, Parts, Error)) return false;
        }
        else
        {
            // Ordinary original-mesh actors need no semantic motion controller.
            TArray<USkeletalMeshComponent*> OriginalParts;
            Actor->GetComponents<USkeletalMeshComponent>(OriginalParts);
            for (auto* Part : OriginalParts)
                if (HasNativeGeometry(Part, Actor)) Parts.Add(Part);
            Parts.Sort([](const USkeletalMeshComponent& A, const USkeletalMeshComponent& B)
            { return A.GetPathName() < B.GetPathName(); });
        }
    }
    if (Parts.IsEmpty()) return Fail(Error, TEXT("Participant has no actual registered native skeletal geometry."));
    Out = MoveTemp(Parts);
    Motion = CandidateMotion;
    return true;
}

bool UOGInteractionParticipantComponent::BindActualNativeParticipant(const FOGEntityId& Entity,
    const FOGContentId& Rig, FString& Error)
{
    UWorld* World = GetWorld();
    UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
    auto* Core = Instance ? Instance->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    IOGWorldStore* Store = Core && Core->IsCoreReady() ? Core->GetWorldStore() : nullptr;
    if (!Store) return Fail(Error, TEXT("Live canonical GameCore Store is required for participant binding."));
    return BindFromCanonicalStore(Entity, Rig, *Store, Core->GetOptionalPackageHost(), Error, Core);
}

bool UOGInteractionParticipantComponent::BindFromCanonicalStore(const FOGEntityId& Entity,
    const FOGContentId& Rig, IOGWorldStore& Store, FOGOptionalPackageHost* Host,
    FString& Error, UOGGameCoreSubsystem* Core)
{
    Error.Reset();
    if (bUnbinding || !IsInGameThread() || !Entity.IsValid() || !Rig.IsValid() || !Store.IsOpen())
        return Fail(Error, TEXT("An existing canonical entity, authored rig family and open Store are required."));
    UGameInstance* Instance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const auto* WorldCore = Instance ? Instance->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    if (WorldCore && WorldCore->IsCoreReady() && WorldCore->GetWorldStore() != &Store)
        return Fail(Error, TEXT("Native actor cannot bind an alternate Store in a world with a live canonical Core."));
    if (Core && (!Core->IsCoreReady() || Core->GetWorldStore() != &Store || Core->GetWorld() != GetWorld()))
        return Fail(Error, TEXT("Participant lifecycle Core must own this exact Store and world."));
    FOGFoundationCharacterContext Context;
    Context.EntityId = Entity;
    FOGFoundationCharacterProjection P;
    if (!FOGFoundationCharacterRuntime(Store).Project(Context, P, Error)) return false;
    if (BoundStore)
    {
        if (BoundStore != &Store || BoundEntityId != Entity || BoundRigFamilyId != Rig)
            return Fail(Error, TEXT("Unbind this participant before changing its canonical entity, Store or rig family."));
        return RefreshFromCanonical(Error);
    }
    if (!CheckActorSelection(GetOwner(), P, true, false, Error)) return false;
    TArray<USkeletalMeshComponent*> Parts;
    UOGSharedMotionControllerComponent* Motion = nullptr;
    if (!DiscoverNativeBinding(GetOwner(), Parts, Motion, Error)) return false;
    // Reject a second physical embodiment rather than selecting the first actor.
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Other = *It;
        if (Other == GetOwner() || !IsValid(Other) || Other->IsActorBeingDestroyed()) continue;
        TArray<UOGInteractionParticipantComponent*> Bindings;
        Other->GetComponents<UOGInteractionParticipantComponent>(Bindings);
        for (auto* Binding : Bindings)
            if (Binding->BoundStore == &Store && Binding->BoundEntityId == Entity)
                return Fail(Error, TEXT("Canonical entity is already bound to another real native actor."));
        TArray<UOGCanonicalCharacterPresentation*> Presenters;
        Other->GetComponents<UOGCanonicalCharacterPresentation>(Presenters);
        for (auto* Presenter : Presenters)
            if (Presenter->HasProjection() && Presenter->GetProjection().Context.EntityId == Entity)
                return Fail(Error, TEXT("Canonical entity already has another projected physical body."));
    }
    auto* Canonical = GetOwner()->FindComponentByClass<UOGCanonicalCharacterPresentation>();
    auto* Appearance = GetOwner()->FindComponentByClass<UOGCharacterAppearanceComponent>();
    // Owned appearance is already the complete ordinary canonical renderer.
    // The inherited, unbound canonical component must not capture a transient
    // wardrobe part which that renderer destroys on the next replacement.
    const bool bAppearanceOwned = Appearance && Appearance->IsOwnedManifestationBound();
    if (bAppearanceOwned) Canonical = nullptr;
    bool bCreated = false;
    if (!Canonical && !bAppearanceOwned)
    {
        Canonical = NewObject<UOGCanonicalCharacterPresentation>(GetOwner());
        GetOwner()->AddInstanceComponent(Canonical);
        Canonical->RegisterComponent();
        bCreated = true;
    }
    // An established ordinary adapter already knows its real body/rig. Refresh
    // that binding without resetting its baseline or choosing another part.
    const bool bConfigureRig = Canonical && !Canonical->HasProjection();
    const bool bCanonicalReady = !Canonical || (Canonical->HasProjection()
        ? Canonical->RefreshFromCanonicalStore(P.Context, Store, Host, Error)
        : Canonical->ConfigureFromCanonicalStore(P.Context, Parts[0], FName(*Rig.ToString()), Store, Host, Error, Core));
    if (!bCanonicalReady)
    {
        if (bCreated) Canonical->DestroyComponent();
        return false;
    }
    // A visual resolver may have changed the physical part set. Verify, don't
    // re-register a running motion controller or manufacture a replacement body.
    if ((Core && (!Core->IsCoreReady() || Core->GetWorldStore() != &Store)) ||
        !CheckActorSelection(GetOwner(), P, true, true, Error) ||
        !DiscoverNativeBinding(GetOwner(), Parts, Motion, Error))
    {
        if (bCreated) Canonical->DestroyComponent();
        if (Error.IsEmpty()) Error = TEXT("Canonical runtime changed while preparing the native binding.");
        return false;
    }
    BoundEntityId = Entity;
    BoundRigFamilyId = Rig;
    BoundStore = &Store;
    BoundHost = Host;
    CanonicalPresentation = Canonical;
    bConfiguredCanonicalRig = bConfigureRig;
    CanonicalRigPart = bConfigureRig ? Parts[0] : nullptr;
    bOwnsCanonicalPresentation = bCreated;
    BoundCore = Core;
    bUsesCoreLifetime = Core != nullptr;
    if (Core) ReleaseHandle = Core->OnCanonicalRuntimeReleasing.AddUObject(
        this, &UOGInteractionParticipantComponent::Unbind);
    return true;
}

bool UOGInteractionParticipantComponent::ValidateEmbodiment(
    const FOGFoundationCharacterProjection& P, FString& Error) const
{
    if (!CheckActorSelection(GetOwner(), P, true, true, Error)) return false;
    TArray<USkeletalMeshComponent*> Parts;
    UOGSharedMotionControllerComponent* Motion = nullptr;
    return DiscoverNativeBinding(GetOwner(), Parts, Motion, Error);
}

bool UOGInteractionParticipantComponent::GetCurrentProjection(
    FOGFoundationCharacterProjection& Out, FString& Error) const
{
    Out = FOGFoundationCharacterProjection();
    Error.Reset();
    if (!IsInGameThread() || bUnbinding || !BoundEntityId.IsValid() || !BoundStore ||
        (bUsesCoreLifetime && (!BoundCore.IsValid() || !BoundCore->IsCoreReady() ||
            BoundCore->GetWorldStore() != BoundStore)))
        return Fail(Error, TEXT("Participant has no live canonical Store binding."));
    FOGFoundationCharacterContext Context;
    Context.EntityId = BoundEntityId;
    FOGFoundationCharacterProjection P;
    if (!FOGFoundationCharacterRuntime(*BoundStore).Project(Context, P, Error) ||
        !ValidateEmbodiment(P, Error)) return false;
    Out = MoveTemp(P);
    return true;
}

bool UOGInteractionParticipantComponent::BuildParticipant(FName Role,
    FOGInteractionParticipant& Out, FString& Error) const
{
    Out = FOGInteractionParticipant();
    Error.Reset();
    if (Role.IsNone()) return Fail(Error, TEXT("Participant requires an explicit supplied interaction role."));
    FOGFoundationCharacterProjection P;
    if (!GetCurrentProjection(P, Error)) return false;
    Out.Character = P.Context;
    Out.Role = Role;
    Out.RigFamilyId = BoundRigFamilyId;
    Out.Actor = GetOwner();
    return true;
}

bool UOGInteractionParticipantComponent::RefreshFromCanonical(FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bUnbinding || bRefreshing)
        return Fail(Error, TEXT("Participant canonical refresh requires a live non-reentrant game thread binding."));
    TGuardValue<bool> RefreshGuard(bRefreshing, true);
    if (!BoundStore || !BoundEntityId.IsValid() ||
        (bUsesCoreLifetime && (!BoundCore.IsValid() || !BoundCore->IsCoreReady() ||
            BoundCore->GetWorldStore() != BoundStore)))
        return Fail(Error, TEXT("Participant canonical Store binding is unavailable."));
    FOGFoundationCharacterContext Context;
    Context.EntityId = BoundEntityId;
    FOGFoundationCharacterProjection P;
    if (!FOGFoundationCharacterRuntime(*BoundStore).Project(Context, P, Error)) return false;
    // Never refresh a body that an external possession/factory path rebound.
    if (!CheckActorSelection(GetOwner(), P, false, true, Error)) return false;
    IOGWorldStore* Store = BoundStore;
    auto* Appearance = GetOwner()->FindComponentByClass<UOGCharacterAppearanceComponent>();
    const bool bAppearanceOwned = Appearance && Appearance->IsOwnedManifestationBound();
    if (bAppearanceOwned)
        if (!Appearance->RefreshCanonicalAppearance(Error)) return false;
    if (!bAppearanceOwned)
        if (auto* Canonical = CanonicalPresentation.Get())
        {
            TArray<USkeletalMeshComponent*> CurrentParts;
            UOGSharedMotionControllerComponent* Motion = nullptr;
            if (!DiscoverNativeBinding(GetOwner(), CurrentParts, Motion, Error)) return false;
            if (bConfiguredCanonicalRig && !CurrentParts.Contains(CanonicalRigPart.Get()))
            {
                // Original multipart factories may also replace their source
                // components. Rebind only this same canonical entity to the
                // exact replacement source list, never a generic/base mesh.
                if (!Canonical->ConfigureFromCanonicalStore(P.Context, CurrentParts[0],
                    FName(*BoundRigFamilyId.ToString()), *Store, BoundHost, Error, BoundCore.Get())) return false;
                CanonicalRigPart = CurrentParts[0];
            }
            else if (!Canonical->RefreshFromCanonicalStore(P.Context, *Store, BoundHost, Error)) return false;
        }
    // Refresh can invoke engine/delegate callbacks, including lifetime release.
    if (BoundStore != Store || BoundEntityId != P.Context.EntityId)
        return Fail(Error, TEXT("Canonical participant was released during presentation refresh."));
    FOGFoundationCharacterProjection Verified;
    if (!GetCurrentProjection(Verified, Error)) return false;
    if (bHasSignificance) SetInteractionSignificance(bActionSignificant, bCameraSignificant);
    return true;
}

bool UOGInteractionParticipantComponent::GetNativeParts(TArray<USkeletalMeshComponent*>& Out, FString& Error) const
{
    Out.Reset();
    FOGFoundationCharacterProjection P;
    if (!GetCurrentProjection(P, Error)) return false;
    UOGSharedMotionControllerComponent* Motion = nullptr;
    return DiscoverNativeBinding(GetOwner(), Out, Motion, Error);
}

UOGSharedMotionControllerComponent* UOGInteractionParticipantComponent::GetMotionController() const
{
    FOGFoundationCharacterProjection P;
    FString Error;
    if (!GetCurrentProjection(P, Error)) return nullptr;
    TArray<USkeletalMeshComponent*> Parts;
    UOGSharedMotionControllerComponent* Motion = nullptr;
    return DiscoverNativeBinding(GetOwner(), Parts, Motion, Error) ? Motion : nullptr;
}

void UOGInteractionParticipantComponent::SetSourceProfileResolver(FOGInteractionParticipantSourceResolver Resolver)
{
    SourceResolver = MoveTemp(Resolver);
}

bool UOGInteractionParticipantComponent::ResolveSourceProfile(
    FOGInteractionParticipantSourceProfile& Out, FString& Error) const
{
    Out = FOGInteractionParticipantSourceProfile();
    FOGFoundationCharacterProjection P;
    if (!GetCurrentProjection(P, Error)) return false;
    if (!SourceResolver) return Fail(Error, TEXT("No actual original definition/lore source resolver is installed."));
    FOGInteractionParticipantSourceProfile Candidate;
    if (!SourceResolver(P, Candidate, Error)) return false;
    if (!Candidate.Matches(P))
        return Fail(Error, TEXT("Authored source metadata is missing provenance or differs from the exact current entity/Identity/Version."));
    // A native callback may trigger canonical changes; no stale receipt may escape.
    FOGFoundationCharacterProjection Current;
    if (!GetCurrentProjection(Current, Error)) return false;
    if (!Candidate.Matches(Current))
        return Fail(Error, TEXT("Canonical source selection changed during metadata resolution."));
    Out = MoveTemp(Candidate);
    Error.Reset();
    return true;
}

bool UOGInteractionParticipantComponent::HasCanonicalAdultSource(FString& Error) const
{
    FOGInteractionParticipantSourceProfile Source;
    if (!ResolveSourceProfile(Source, Error)) return false;
    return Source.CanonicalMaturity == EOGCanonicalMaturity::Adult ||
        Fail(Error, TEXT("Actual canonical source does not classify this participant as Adult."));
}

void UOGInteractionParticipantComponent::SetInteractionSignificance(bool bAction, bool bCamera)
{
    TArray<USkeletalMeshComponent*> Parts;
    FString Error;
    if (!GetNativeParts(Parts, Error)) { RestoreSignificance(); return; }
    // Restore any old part before forgetting it, including streamed/replaced parts.
    for (int32 Index = SignificanceRestore.Num() - 1; Index >= 0; --Index)
    {
        const auto& Saved = SignificanceRestore[Index];
        if (Parts.Contains(Saved.Part.Get())) continue;
        if (auto* Part = Saved.Part.Get())
        {
            Part->bEnableUpdateRateOptimizations = Saved.bUpdateRateOptimizations;
            Part->VisibilityBasedAnimTickOption = static_cast<EVisibilityBasedAnimTickOption>(Saved.VisibilityTickOption);
        }
        SignificanceRestore.RemoveAt(Index);
    }
    for (auto* Part : Parts)
    {
        if (!SignificanceRestore.ContainsByPredicate([Part](const auto& Saved) { return Saved.Part.Get() == Part; }))
        {
            FPartSignificanceRestore Saved;
            Saved.Part = Part;
            Saved.bUpdateRateOptimizations = Part->bEnableUpdateRateOptimizations;
            Saved.VisibilityTickOption = static_cast<uint8>(Part->VisibilityBasedAnimTickOption);
            SignificanceRestore.Add(Saved);
        }
        Part->bEnableUpdateRateOptimizations = !(bAction || bCamera);
        Part->VisibilityBasedAnimTickOption = bAction || bCamera
            ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones
            : EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    }
    const bool bChanged = !bHasSignificance || bActionSignificant != bAction || bCameraSignificant != bCamera;
    bHasSignificance = true;
    bActionSignificant = bAction;
    bCameraSignificant = bCamera;
    if (bChanged) OnSignificanceChanged.Broadcast(bAction, bCamera);
}

void UOGInteractionParticipantComponent::RestoreSignificance()
{
    for (const auto& Saved : SignificanceRestore)
        if (auto* Part = Saved.Part.Get())
        {
            Part->bEnableUpdateRateOptimizations = Saved.bUpdateRateOptimizations;
            Part->VisibilityBasedAnimTickOption = static_cast<EVisibilityBasedAnimTickOption>(Saved.VisibilityTickOption);
        }
    SignificanceRestore.Reset();
    const bool bWasSet = bHasSignificance;
    bHasSignificance = bActionSignificant = bCameraSignificant = false;
    if (bWasSet) OnSignificanceChanged.Broadcast(false, false);
}

bool UOGInteractionParticipantComponent::ResolveActualRulerParticipant(UWorld* World,
    IOGWorldStore& Store, const FOGEntityId& Ruler, FOGInteractionParticipant& Out, FString& Error)
{
    Out = FOGInteractionParticipant();
    Error.Reset();
    if (!IsInGameThread() || !IsValid(World) || !Ruler.IsValid() || !Store.IsOpen())
        return Fail(Error, TEXT("Ruler resolution requires a real world and existing canonical Store Ruler."));
    UGameInstance* Instance = World->GetGameInstance();
    const auto* Core = Instance ? Instance->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    if (Core && Core->IsCoreReady() && Core->GetWorldStore() != &Store)
        return Fail(Error, TEXT("Ruler body resolution requires this world's actual canonical Store."));
    FOGFoundationCharacterContext Context;
    Context.EntityId = Ruler;
    FOGFoundationCharacterProjection P;
    if (!FOGFoundationCharacterRuntime(Store).Project(Context, P, Error)) return false;
    if (P.EntityKind != FName(TEXT("ruler")) || P.bHasManifestation || P.Context.OwnerEntityId != Ruler)
        return Fail(Error, TEXT("Player participant must be the actual canonical Ruler, never an owned companion."));
    FOGInteractionParticipant Candidate;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed()) continue;
        bool bProjectedToRuler = false;
        TArray<UOGCanonicalCharacterPresentation*> Canonicals;
        Actor->GetComponents<UOGCanonicalCharacterPresentation>(Canonicals);
        for (auto* Canonical : Canonicals)
            if (Canonical->HasProjection() && SameSelection(Canonical->GetProjection(), P)) bProjectedToRuler = true;
        TArray<UOGInteractionParticipantComponent*> Bindings;
        Actor->GetComponents<UOGInteractionParticipantComponent>(Bindings);
        for (auto* Binding : Bindings)
            if (Binding->BoundStore == &Store && Binding->BoundEntityId == Ruler) bProjectedToRuler = true;
        if (!bProjectedToRuler) continue;
        // Conflicting evidence on a candidate body is an error, not permission
        // to skip it and select some other body by iteration order.
        if (!CheckActorSelection(Actor, P, true, true, Error)) return false;
        TArray<USkeletalMeshComponent*> Parts;
        UOGSharedMotionControllerComponent* Motion = nullptr;
        if (!DiscoverNativeBinding(Actor, Parts, Motion, Error)) return false;
        if (Candidate.Actor.IsValid())
            return Fail(Error, TEXT("Multiple physical actors are projected to the Ruler; no unique real body can be selected."));
        Candidate.Character = P.Context;
        Candidate.Actor = Actor;
        Candidate.Role = TEXT("player");
        Candidate.RigFamilyId = NativeRigFamily(Parts, Motion);
        for (auto* Binding : Bindings)
            if (Binding->BoundStore == &Store && Binding->BoundEntityId == Ruler)
                Candidate.RigFamilyId = Binding->BoundRigFamilyId;
        if (!Candidate.RigFamilyId.IsValid())
            return Fail(Error, TEXT("Real Ruler body has no valid native rig family."));
    }
    if (!Candidate.Actor.IsValid())
        return Fail(Error, TEXT("No actual physical body is canonically projected to the Ruler in this world."));
    Out = MoveTemp(Candidate);
    return true;
}

void UOGInteractionParticipantComponent::Unbind()
{
    if (bUnbinding) return;
    TGuardValue<bool> UnbindGuard(bUnbinding, true);
    // Detach authority/lifetime references before delegates can re-enter.
    if (BoundCore.IsValid()) BoundCore->OnCanonicalRuntimeReleasing.Remove(ReleaseHandle);
    ReleaseHandle.Reset();
    BoundCore.Reset();
    BoundStore = nullptr;
    BoundHost = nullptr;
    BoundEntityId = FOGEntityId();
    BoundRigFamilyId = FOGContentId();
    bUsesCoreLifetime = false;
    SourceResolver = FOGInteractionParticipantSourceResolver();
    auto* OwnedPresentation = bOwnsCanonicalPresentation ? CanonicalPresentation.Get() : nullptr;
    auto* ConfiguredPresentation = bConfiguredCanonicalRig ? CanonicalPresentation.Get() : nullptr;
    CanonicalPresentation.Reset();
    CanonicalRigPart.Reset();
    bConfiguredCanonicalRig = false;
    bOwnsCanonicalPresentation = false;
    RestoreSignificance();
    if (OwnedPresentation)
    {
        OwnedPresentation->ClearBinding();
        OwnedPresentation->DestroyComponent();
    }
    else if (ConfiguredPresentation) ConfiguredPresentation->ClearBinding();
}

void UOGInteractionParticipantComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Unbind();
    Super::EndPlay(Reason);
}

void UOGInteractionParticipantComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Unbind();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}
