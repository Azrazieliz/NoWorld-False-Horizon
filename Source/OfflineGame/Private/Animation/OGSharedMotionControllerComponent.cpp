#include "Animation/OGSharedMotionControllerComponent.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/OGSharedReferenceAnimInstance.h"

UOGSharedMotionControllerComponent::UOGSharedMotionControllerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UOGSharedMotionControllerComponent::HaveIdenticalReferencePose(
    const USkeletalMeshComponent* A, const USkeletalMeshComponent* B)
{
    const USkeletalMesh* AMesh = A ? A->GetSkeletalMeshAsset() : nullptr;
    const USkeletalMesh* BMesh = B ? B->GetSkeletalMeshAsset() : nullptr;
    if (!AMesh || !BMesh) return false;
    const FReferenceSkeleton& ARig = AMesh->GetRefSkeleton();
    const FReferenceSkeleton& BRig = BMesh->GetRefSkeleton();
    if (ARig.GetNum() == 0 || ARig.GetNum() != BRig.GetNum()) return false;
    for (int32 Index = 0; Index < ARig.GetNum(); ++Index)
    {
        if (ARig.GetBoneName(Index) != BRig.GetBoneName(Index) ||
            ARig.GetParentIndex(Index) != BRig.GetParentIndex(Index) ||
            !ARig.GetRefBonePose()[Index].Equals(BRig.GetRefBonePose()[Index], 0.0001f))
            return false;
    }
    return true;
}

bool UOGSharedMotionControllerComponent::RegisterNativeParts(
    const TArray<USkeletalMeshComponent*>& Parts)
{
    if (Parts.IsEmpty() || RigProfile.IsNone()) return false;
    TArray<TObjectPtr<USkeletalMeshComponent>> Verified;
    TSet<USkeletalMeshComponent*> Unique;
    for (USkeletalMeshComponent* Part : Parts)
    {
        if (!Part || Part->GetOwner() != GetOwner() || Unique.Contains(Part) ||
            !Part->GetSkeletalMeshAsset() || !Part->GetSkeletalMeshAsset()->GetSkeleton())
            return false;
        Unique.Add(Part);
        Verified.Add(Part);
    }
    TArray<FOGReferenceRigAdapter> Adapters;
    for (USkeletalMeshComponent* Part : Parts)
    {
        FOGReferenceRigAdapter Adapter; FString Error;
        if (!Adapter.Bind(Part, Error)) return false;
        Adapters.Add(MoveTemp(Adapter));
    }
    StopAction();
    for (USkeletalMeshComponent* Previous : RegisteredParts)
        if (Previous) Previous->RemoveTickPrerequisiteComponent(this);
    RegisteredParts = MoveTemp(Verified);
    ReferenceRigs = MoveTemp(Adapters);
    for (USkeletalMeshComponent* Part : RegisteredParts) Part->AddTickPrerequisiteComponent(this);
    return true;
}

void UOGSharedMotionControllerComponent::SetCurrentOutfit(FName InOutfitProfile)
{
    if (InOutfitProfile != OutfitProfile)
    {
        StopAction();
        OutfitProfile = InOutfitProfile;
    }
}

bool UOGSharedMotionControllerComponent::ResolveForPart(
    const UOGSharedMotionLibrary* Library, USkeletalMeshComponent* Part,
    FName Action, FName Context, bool bAuthorizedAdult,
    int32 ParticipantCount, FName ParticipantRole,
    UAnimSequence*& OutSequence, const FOGSharedMotionEntry*& OutEntry) const
{
    OutSequence = nullptr;
    OutEntry = nullptr;
    if (!Part) return false;
    TArray<TObjectPtr<USkeletalMeshComponent>> SearchParts;
    if (bUseIdenticalTopologyLeaderPose)
        SearchParts = RegisteredParts;
    else SearchParts.Add(Part);
    OutSequence = FOGSharedMotionResolver::Resolve(
        Library, Action, RigProfile, AnatomyProfile, OutfitProfile,
        Context, Capabilities, SearchParts, bAuthorizedAdult,
        ParticipantCount, &OutEntry, ParticipantRole);
    return OutSequence && OutEntry && OutSequence->GetSkeleton() &&
        (!OutEntry->bUsesRootMotion ||
            (bPermitAuthoritativeRootMotion && GetOwner() &&
             GetOwner()->HasAuthority() && Cast<ACharacter>(GetOwner())));
}

bool UOGSharedMotionControllerComponent::CanPlayAction(
    FName GameplayAction, FName Context, bool bAdultContextAuthorized,
    int32 ParticipantCount, FName ParticipantRole) const
{
    if (RigProfile.IsNone() || GameplayAction.IsNone() || Context.IsNone() ||
        RegisteredParts.IsEmpty() || ParticipantCount < 1)
        return false;
    FOGSharedMotionEntry Shared;
    TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> Source;
    TArray<TArray<int32>> Mapping; FString SharedError;
    if (FindReference(GameplayAction, Context, ParticipantCount, ParticipantRole,
        Shared, Source, Mapping, SharedError)) return true;
    const UOGSharedMotionLibrary* Library = SharedLibrary.IsNull()
        ? nullptr : SharedLibrary.LoadSynchronous();
    if (!Library) Library = FOGSharedMotionResolver::GetInstalledLibrary();
    FName SourceId = NAME_None;
    bool bInitialLoop = false;
    USkeletalMeshComponent* Leader = nullptr;
    int32 ValidCount = 0;
    for (USkeletalMeshComponent* Part : RegisteredParts)
    {
        UAnimSequence* Clip = nullptr;
        const FOGSharedMotionEntry* Entry = nullptr;
        if (!ResolveForPart(Library, Part, GameplayAction, Context,
            bAdultContextAuthorized, ParticipantCount, ParticipantRole, Clip, Entry))
            return false;
        if (!SourceId.IsNone() &&
            (SourceId != Entry->SourceClipId || bInitialLoop != Entry->bLoop))
            return false;
        SourceId = Entry->SourceClipId;
        bInitialLoop = Entry->bLoop;
        if (bUseIdenticalTopologyLeaderPose)
        {
            for (USkeletalMeshComponent* Candidate : RegisteredParts)
                if (Candidate && Candidate->GetSkeletalMeshAsset() &&
                    Candidate->GetSkeletalMeshAsset()->GetSkeleton() == Clip->GetSkeleton())
                { Leader = Candidate; break; }
            if (!Leader) return false;
            for (USkeletalMeshComponent* Candidate : RegisteredParts)
                if (!HaveIdenticalReferencePose(Leader, Candidate)) return false;
            return true;
        }
        ++ValidCount;
    }
    return ValidCount == RegisteredParts.Num();
}

bool UOGSharedMotionControllerComponent::PreviewSourceClipId(
    FName GameplayAction, FName Context, bool bAdultContextAuthorized,
    int32 ParticipantCount, FName ParticipantRole, FName& OutSourceClipId) const
{
    OutSourceClipId = NAME_None;
    if (!CanPlayAction(GameplayAction, Context, bAdultContextAuthorized,
        ParticipantCount, ParticipantRole)) return false;
    FOGSharedMotionEntry Shared;
    TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> Source;
    TArray<TArray<int32>> Mapping; FString SharedError;
    if (FindReference(GameplayAction, Context, ParticipantCount, ParticipantRole,
        Shared, Source, Mapping, SharedError))
    {
        OutSourceClipId = Shared.SourceClipId;
        return !OutSourceClipId.IsNone();
    }
    const UOGSharedMotionLibrary* Library = SharedLibrary.IsNull()
        ? nullptr : SharedLibrary.LoadSynchronous();
    if (!Library) Library = FOGSharedMotionResolver::GetInstalledLibrary();
    UAnimSequence* Sequence = nullptr;
    const FOGSharedMotionEntry* Entry = nullptr;
    if (!ResolveForPart(Library, RegisteredParts[0], GameplayAction, Context,
        bAdultContextAuthorized, ParticipantCount, ParticipantRole, Sequence, Entry))
        return false;
    OutSourceClipId = Entry->SourceClipId;
    return !OutSourceClipId.IsNone();
}

bool UOGSharedMotionControllerComponent::PreviewAuthoredSceneId(
    FName GameplayAction, FName Context, bool bAdultContextAuthorized,
    int32 ParticipantCount, FName ParticipantRole, FName& OutSceneId) const
{
    OutSceneId = NAME_None;
    if (ParticipantCount < 2 ||
        !CanPlayAction(GameplayAction, Context, bAdultContextAuthorized,
            ParticipantCount, ParticipantRole)) return false;
    FOGSharedMotionEntry Shared;
    TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> Source;
    TArray<TArray<int32>> Mapping; FString SharedError;
    if (FindReference(GameplayAction, Context, ParticipantCount, ParticipantRole,
        Shared, Source, Mapping, SharedError))
    {
        OutSceneId = Shared.AuthoredSceneId;
        return !OutSceneId.IsNone();
    }
    const UOGSharedMotionLibrary* Library = SharedLibrary.IsNull()
        ? nullptr : SharedLibrary.LoadSynchronous();
    if (!Library) Library = FOGSharedMotionResolver::GetInstalledLibrary();
    UAnimSequence* Sequence = nullptr;
    const FOGSharedMotionEntry* Entry = nullptr;
    if (!ResolveForPart(Library, RegisteredParts[0], GameplayAction, Context,
            bAdultContextAuthorized, ParticipantCount, ParticipantRole,
            Sequence, Entry) || !Entry) return false;
    OutSceneId = Entry->AuthoredSceneId;
    return !OutSceneId.IsNone();
}

void UOGSharedMotionControllerComponent::SynchronizeToWorldTime(float SceneStartTime)
{
    if (!bPlaying || !GetWorld()) return;
    bSceneTimeLocked = true;
    SceneWorldStartTime = SceneStartTime;
    SceneAnchorElapsed = 0.f;
    ElapsedSeconds = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - SceneWorldStartTime) * ExternalPacing;
}

bool UOGSharedMotionControllerComponent::SetExternalPacing(float Rate)
{
    if (!FMath::IsFinite(Rate) || Rate < 0.f) return false;
    if (Rate == ExternalPacing) return true;
    if (bSceneTimeLocked && GetWorld())
    {
        const float Now = GetWorld()->GetTimeSeconds();
        // Preserve phase at the speed boundary, including a paused interval.
        SceneAnchorElapsed += FMath::Max(0.f, Now - SceneWorldStartTime) * ExternalPacing;
        ElapsedSeconds = SceneAnchorElapsed;
        SceneWorldStartTime = Now;
    }
    ExternalPacing = Rate;
    for (int32 I = 0; I < PlayingParts.Num(); ++I)
        if (PlayingParts[I])
        {
            const float LengthScale = CurrentLeader ? 1.f :
                (PlayingSequenceLengths.IsValidIndex(I) && SequenceSeconds > 0.f
                    ? PlayingSequenceLengths[I] / SequenceSeconds : 1.f);
            PlayingParts[I]->SetPlayRate(PlaybackRate * ExternalPacing * LengthScale);
        }
    return true;
}

bool UOGSharedMotionControllerComponent::SetMorphLayer(
    FName Layer, FName Morph, float Weight)
{
    if (Layer.IsNone() || Morph.IsNone() || RegisteredParts.IsEmpty()) return false;
    bool bHasNativeMorph = false;
    for (USkeletalMeshComponent* Part : RegisteredParts)
        if (Part && Part->GetSkeletalMeshAsset() &&
            Part->GetSkeletalMeshAsset()->FindMorphTarget(Morph))
            { bHasNativeMorph = true; break; }
    if (!bHasNativeMorph) return false;
    for (FMorphOverlay& Overlay : MorphOverlays)
    {
        if (Overlay.Layer == Layer && Overlay.Morph == Morph)
        {
            Overlay.Weight = FMath::Clamp(Weight, -1.f, 1.f);
            SetComponentTickEnabled(true);
            ApplyMorphOverlays();
            return true;
        }
    }
    FMorphOverlay& New = MorphOverlays.AddDefaulted_GetRef();
    New.Layer = Layer;
    New.Morph = Morph;
    New.Weight = FMath::Clamp(Weight, -1.f, 1.f);
    SetComponentTickEnabled(true);
    ApplyMorphOverlays();
    return true;
}

void UOGSharedMotionControllerComponent::ClearMorphLayer(FName Layer)
{
    MorphOverlays.RemoveAll([Layer](const FMorphOverlay& M)
        { return M.Layer == Layer; });
    ApplyMorphOverlays();
    if (!bPlaying && MorphOverlays.IsEmpty()) SetComponentTickEnabled(false);
}

void UOGSharedMotionControllerComponent::ApplyMorphOverlays()
{
    // A facial/secondary blendshape channel leaves full-body native playback
    // intact. Absent morph targets never trigger guessed skeletal retargeting.
    TMap<FName, float> Combined;
    for (const FMorphOverlay& Overlay : MorphOverlays)
        Combined.FindOrAdd(Overlay.Morph) += Overlay.Weight;
    const float RawTime = ElapsedSeconds * PlaybackRate;
    const float AuthoredTime = bPlaying && bLooping && SequenceSeconds > SMALL_NUMBER
        ? FMath::Fmod(RawTime, SequenceSeconds) : RawTime;
    if (bPlaying)
    for (const FOGSharedMorphTrack& Track : AuthoredMorphTracks)
    {
        if (Track.Morph.IsNone() || Track.Keys.IsEmpty()) continue;
        float Weight = Track.Keys[0].Weight;
        if (AuthoredTime >= Track.Keys.Last().TimeSeconds)
            Weight = Track.Keys.Last().Weight;
        else for (int32 K = 1; K < Track.Keys.Num(); ++K)
        {
            if (AuthoredTime <= Track.Keys[K].TimeSeconds)
            {
                const float Span = FMath::Max(Track.Keys[K].TimeSeconds -
                    Track.Keys[K-1].TimeSeconds, KINDA_SMALL_NUMBER);
                const float T = FMath::Clamp((AuthoredTime -
                    Track.Keys[K-1].TimeSeconds) / Span, 0.f, 1.f);
                Weight = FMath::Lerp(Track.Keys[K-1].Weight,
                    Track.Keys[K].Weight, T);
                break;
            }
        }
        Combined.FindOrAdd(Track.Morph) += Weight;
    }
    for (USkeletalMeshComponent* Part : RegisteredParts)
    {
        if (!Part || !Part->GetSkeletalMeshAsset()) continue;
        for (FName PreviouslyApplied : AppliedMorphs)
            if (!Combined.Contains(PreviouslyApplied) &&
                Part->GetSkeletalMeshAsset()->FindMorphTarget(PreviouslyApplied))
                Part->SetMorphTarget(PreviouslyApplied, 0.f);
        for (const auto& Pair : Combined)
            if (Part->GetSkeletalMeshAsset()->FindMorphTarget(Pair.Key))
                Part->SetMorphTarget(Pair.Key, FMath::Clamp(Pair.Value, -1.f, 1.f));
    }
    AppliedMorphs.Reset();
    for (const auto& Pair : Combined) AppliedMorphs.Add(Pair.Key);
}

void UOGSharedMotionControllerComponent::ApplyAuthoredRootMotion(
    float PreviousSourceTime, float SourceTime)
{
    if (!bApplyingRootMotion || !CurrentSequence || SourceTime <= PreviousSourceTime)
        return;
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !Character->HasAuthority() ||
        !Character->GetCharacterMovement()) return;

    const float Length = SequenceSeconds;
    const FAnimExtractContext Extraction(0., true);
    auto ApplySegment = [Character, this, &Extraction](float Start, float End)
    {
        if (End <= Start) return;
        const FTransform Delta =
            CurrentSequence->ExtractRootMotionFromRange(Start, End, Extraction);
        if (Delta.Equals(FTransform::Identity, KINDA_SMALL_NUMBER)) return;
        // Original Koikatsu meshes use a measured visual-axis correction.
        // Root displacement follows the selected actual skeletal component
        // relative orientation rather than blindly following the capsule X.
        const USkeletalMeshComponent* Driver = CurrentLeader
            ? CurrentLeader.Get() : (PlayingParts.IsEmpty() ? nullptr : PlayingParts[0].Get());
        const FQuat VisualToActor = Driver ? Driver->GetRelativeRotation().Quaternion()
            : FQuat::Identity;
        const FQuat ActorQuat = Character->GetActorQuat();
        const FVector WorldDelta = ActorQuat.RotateVector(
            VisualToActor.RotateVector(Delta.GetTranslation()));
        const FQuat WorldRotation = ActorQuat *
            (VisualToActor * Delta.GetRotation() * VisualToActor.Inverse());
        FHitResult Hit;
        Character->GetCharacterMovement()->MoveUpdatedComponent(
            WorldDelta, WorldRotation, true, &Hit);
        if (Hit.IsValidBlockingHit())
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("Shared root motion collision blocked action=%s"), *CurrentAction.ToString());
        }
    };
    if (!bLooping)
    {
        ApplySegment(FMath::Min(PreviousSourceTime, Length),
            FMath::Min(SourceTime, Length));
        return;
    }
    float Cursor = PreviousSourceTime;
    const float Stop = FMath::Min(SourceTime, Cursor + Length * 4.f);
    while (Cursor < Stop)
    {
        const float CycleStart = FMath::FloorToFloat(Cursor / Length) * Length;
        const float End = FMath::Min(Stop, CycleStart + Length);
        ApplySegment(Cursor - CycleStart, End - CycleStart);
        if (End <= Cursor + SMALL_NUMBER) break;
        Cursor = End;
    }
}

bool UOGSharedMotionControllerComponent::PlayAction(
    FName GameplayAction, FName Context, bool bAdultContextAuthorized,
    int32 ParticipantCount, FName ParticipantRole)
{
    FOGSharedMotionEntry Shared;
    TSharedPtr<const FOGReferenceMotionClip, ESPMode::ThreadSafe> Source;
    TArray<TArray<int32>> Mapping; FString SharedError;
    if (FindReference(GameplayAction, Context, ParticipantCount, ParticipantRole,
        Shared, Source, Mapping, SharedError))
        return StartReference(Shared, MoveTemp(Source), MoveTemp(Mapping));
    if (!CanPlayAction(GameplayAction, Context, bAdultContextAuthorized,
        ParticipantCount, ParticipantRole)) return false;

    const UOGSharedMotionLibrary* Library =
        SharedLibrary.IsNull() ? nullptr : SharedLibrary.LoadSynchronous();
    if (!Library) Library = FOGSharedMotionResolver::GetInstalledLibrary();

    TArray<UAnimSequence*> Sequences;
    TArray<const FOGSharedMotionEntry*> Entries;
    USkeletalMeshComponent* Leader = nullptr;

    // Independent rigs each get their own native derivative of the SAME
    // source action. A matching clip for only the first part is insufficient.
    for (USkeletalMeshComponent* Part : RegisteredParts)
    {
        if (!Part || !Part->GetSkeletalMeshAsset()) return false;
        TArray<TObjectPtr<USkeletalMeshComponent>> SearchParts;
        if (bUseIdenticalTopologyLeaderPose)
            SearchParts = RegisteredParts;
        else
            SearchParts.Add(Part);

        const FOGSharedMotionEntry* Entry = nullptr;
        UAnimSequence* Sequence = FOGSharedMotionResolver::Resolve(
            Library, GameplayAction, RigProfile, AnatomyProfile, OutfitProfile,
            Context, Capabilities, SearchParts, bAdultContextAuthorized,
            ParticipantCount, &Entry, ParticipantRole);
        if (!Sequence || !Entry || !Sequence->GetSkeleton()) return false;

        if (!Sequences.IsEmpty() &&
            (Entry->SourceClipId != Entries[0]->SourceClipId ||
             Entry->bLoop != Entries[0]->bLoop))
            return false; // Prevent mismatched source timing or identity.

        Sequences.Add(Sequence);
        Entries.Add(Entry);
        if (bUseIdenticalTopologyLeaderPose) break;
    }

    if (bUseIdenticalTopologyLeaderPose)
    {
        for (USkeletalMeshComponent* Part : RegisteredParts)
        {
            if (Part->GetSkeletalMeshAsset()->GetSkeleton() == Sequences[0]->GetSkeleton())
            {
                Leader = Part;
                break;
            }
        }
        if (!Leader) return false;
        for (USkeletalMeshComponent* Part : RegisteredParts)
            if (!HaveIdenticalReferencePose(Leader, Part)) return false;
    }
    else if (Sequences.Num() != RegisteredParts.Num()) return false;

    // All checks finish before interrupting an existing action.
    StopAction();
    const FOGSharedMotionEntry* PrimaryEntry = Entries[0];
    CurrentSequence = Sequences[0];
    CurrentLeader = Leader;
    PlayingParts = RegisteredParts;
    bPlaying = true;
    bApplyingRootMotion = PrimaryEntry->bUsesRootMotion;
    AuthoredMorphTracks = PrimaryEntry->MorphTracks;
    bSceneTimeLocked = false;
    SceneAnchorElapsed = 0.f;
    ExternalPacing = 1.f;
    bLooping = PrimaryEntry->bLoop;
    bHasContact = PrimaryEntry->ContactSeconds >= 0.f;
    SourceContactSeconds = PrimaryEntry->ContactSeconds;
    if (!bHasContact && !PrimaryEntry->ContactNotify.IsNone())
    {
        // Native authored animation notifies take precedence over guessed
        // generic contact timing. No notify means no contact event.
        for (const FAnimNotifyEvent& Notify : Sequences[0]->Notifies)
        {
            if (Notify.NotifyName == PrimaryEntry->ContactNotify)
            {
                SourceContactSeconds = Notify.GetTriggerTime();
                bHasContact = true;
                break;
            }
        }
    }
    SequenceSeconds = FMath::Max(Sequences[0]->GetPlayLength(), 0.001f);
    PlaybackRate = FMath::Clamp(PrimaryEntry->PlayRate, 0.01f, 8.f);
    ElapsedSeconds = 0.f;
    LastContactCycle = -1;
    CurrentAction = GameplayAction;
    CurrentMotionId = PrimaryEntry->MotionId;
    CurrentSourceClipId = PrimaryEntry->SourceClipId;

    if (bUseIdenticalTopologyLeaderPose)
    {
        Leader->SetLeaderPoseComponent(nullptr);
        Leader->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Leader->PlayAnimation(Sequences[0], bLooping);
        Leader->SetPlayRate(PlaybackRate);
        PlayingSequences.Add(Sequences[0]);
        PlayingSequenceLengths.Add(SequenceSeconds);
        for (USkeletalMeshComponent* Part : PlayingParts)
            if (Part != Leader) Part->SetLeaderPoseComponent(Leader);
    }
    else
    {
        for (int32 I = 0; I < PlayingParts.Num(); ++I)
        {
            USkeletalMeshComponent* Part = PlayingParts[I];
            const float Length = FMath::Max(Sequences[I]->GetPlayLength(), 0.001f);
            Part->SetLeaderPoseComponent(nullptr);
            Part->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            Part->PlayAnimation(Sequences[I], bLooping);
            Part->SetPlayRate(PlaybackRate * Length / SequenceSeconds);
            PlayingSequences.Add(Sequences[I]);
            PlayingSequenceLengths.Add(Length);
        }
    }
    SetComponentTickEnabled(true);
    return true;
}


void UOGSharedMotionControllerComponent::StopAction()
{
    RestoreReferencePose();
    SetComponentTickEnabled(!MorphOverlays.IsEmpty());
    for (USkeletalMeshComponent* Part : PlayingParts)
    {
        if (Part) Part->SetLeaderPoseComponent(nullptr);
    }
    for (USkeletalMeshComponent* Part : PlayingParts)
    {
        if (Part) Part->Stop();
    }
    PlayingParts.Reset();
    PlayingSequences.Reset();
    PlayingSequenceLengths.Reset();
    CurrentLeader = nullptr;
    CurrentSequence = nullptr;
    CurrentAction = NAME_None;
    CurrentMotionId = NAME_None;
    CurrentSourceClipId = NAME_None;
    AuthoredMorphTracks.Reset();
    bPlaying = false;
    bApplyingRootMotion = false;
    bSceneTimeLocked = false;
    bLooping = false;
    bHasContact = false;
    SourceContactSeconds = -1.f;
    ElapsedSeconds = 0.f;
    SequenceSeconds = 0.f;
    SceneAnchorElapsed = 0.f;
    ExternalPacing = 1.f;
    LastContactCycle = -1;
    ApplyMorphOverlays();
}

void UOGSharedMotionControllerComponent::TickComponent(
    float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    if (DeltaSeconds <= 0.f) return;
    if (!bPlaying)
    {
        ApplyMorphOverlays();
        return;
    }
    const float Before = ElapsedSeconds * PlaybackRate;
    ElapsedSeconds = bSceneTimeLocked && GetWorld()
        ? SceneAnchorElapsed + FMath::Max(0.f, GetWorld()->GetTimeSeconds() - SceneWorldStartTime) * ExternalPacing
        : ElapsedSeconds + DeltaSeconds * ExternalPacing;
    const float SourceTime = ElapsedSeconds * PlaybackRate;
    if (ReferenceClip) UpdateReferencePose();
    if (!bPlaying) return;
    ApplyAuthoredRootMotion(Before, SourceTime);
    ApplyMorphOverlays();
    if (!ReferenceClip && !CurrentLeader && bPhaseLockIndependentParts)
    {
        // Synchronize independently imported part skeletons by normalized
        // source phase, not by assuming their native durations are identical.
        const float Normalized = bLooping
            ? FMath::Fmod(SourceTime, SequenceSeconds) / SequenceSeconds
            : FMath::Clamp(SourceTime / SequenceSeconds, 0.f, 1.f);
        for (int32 I = 0; I < PlayingParts.Num(); ++I)
            if (PlayingParts[I] && PlayingSequenceLengths.IsValidIndex(I))
                PlayingParts[I]->SetPosition(
                    FMath::Min(Normalized * PlayingSequenceLengths[I],
                        PlayingSequenceLengths[I] - 0.0001f), false);
    }
    // Crossing authored source contact must survive a low-FPS frame that
    // jumps straight past the event (or even across a full loop boundary).
    if (bHasContact && SourceContactSeconds <= SequenceSeconds &&
        SourceTime >= SourceContactSeconds)
    {
        const int32 ContactCycle = bLooping
            ? FMath::FloorToInt((SourceTime - SourceContactSeconds) / SequenceSeconds)
            : 0;
        if (ContactCycle > LastContactCycle)
        {
            LastContactCycle = ContactCycle;
            OnAuthoredContact.Broadcast(CurrentAction, CurrentMotionId);
        }
    }
    if (!bLooping && SourceTime >= SequenceSeconds)
    {
        const FName FinishedAction = CurrentAction;
        const FName FinishedMotion = CurrentMotionId;
        StopAction();
        OnMotionFinished.Broadcast(FinishedAction, FinishedMotion);
    }
}

void UOGSharedMotionControllerComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    StopAction();
    MorphOverlays.Reset();
    ApplyMorphOverlays();
    for (USkeletalMeshComponent* Part : RegisteredParts)
        if (Part) Part->RemoveTickPrerequisiteComponent(this);
    ReferenceRigs.Reset(); RegisteredParts.Reset();
    Super::EndPlay(EndPlayReason);
}

bool UOGSharedMotionControllerComponent::PrepareReference(const FOGReferenceMotionClip& Clip,
    TArray<TArray<int32>>& Out, FString& Error) const
{
    Out.Reset();
    if (RegisteredParts.IsEmpty() || ReferenceRigs.Num()!=RegisteredParts.Num())
    { Error=TEXT("Register the character's actual native rigs first."); return false; }
    for (int32 I=0; I<RegisteredParts.Num(); ++I)
    {
        const auto* Part=RegisteredParts[I].Get();
        if (!Part || ReferenceRigs[I].Mesh.Get()!=Part->GetSkeletalMeshAsset())
        { Out.Reset(); Error=TEXT("Character rig changed; refresh its shared-library connection."); return false; }
        TArray<int32> Indices;
        if (!ReferenceRigs[I].Map(Clip,SharedSourceBoneAliases,Indices,Error)) { Out.Reset(); return false; }
        Out.Add(MoveTemp(Indices));
    }
    return true;
}
bool UOGSharedMotionControllerComponent::FindReference(FName Action,FName Context,int32 Count,FName Role,
    FOGSharedMotionEntry& OutEntry,TSharedPtr<const FOGReferenceMotionClip,ESPMode::ThreadSafe>& OutClip,
    TArray<TArray<int32>>& OutMapping,FString& Error) const
{
    OutClip.Reset(); OutMapping.Reset();
    for (const auto& Entry:FOGSharedReferenceLibrary::Find(Action,Context,RigProfile,AnatomyProfile,Capabilities,Count,Role))
    {
        if (Entry.bUsesRootMotion || (!Entry.ContactNotify.IsNone() && Entry.ContactSeconds<0)) continue; // Existing collision-authorized native root path remains authoritative.
        if (!SharedLibrary.IsNull())
        {
            const auto* Explicit=SharedLibrary.LoadSynchronous();
            if (!Explicit) continue;
            bool Declared=false;
            for (const auto& Choice:Explicit->Motions)
                if (Choice.bApprovedForGameplay && Choice.GameplayAction==Action &&
                    Choice.CompatibleRigProfile==RigProfile &&
                    (Choice.GameplayContext.IsNone() || Choice.GameplayContext==Context) &&
                    Choice.RequiredParticipants==Count &&
                    (Count==1 ? Choice.SceneRole.IsNone() : Choice.SceneRole==Role) &&
                    FOGSharedReferenceLibrary::CanonicalId(Choice.SourceClipId)==Entry.SourceClipId)
                { Declared=true; break; }
            if (!Declared) continue;
        }
        auto Clip=FOGSharedReferenceLibrary::Load(Entry.SourceClipId,Error);
        if (Clip && PrepareReference(*Clip,OutMapping,Error))
        { OutEntry=Entry; OutClip=MoveTemp(Clip); return true; }
    }
    return false;
}
TArray<FName> UOGSharedMotionControllerComponent::GetSharedSourceMotionIds() const
{
    return FOGSharedReferenceLibrary::GetClipIds();
}
bool UOGSharedMotionControllerComponent::PlaySharedSourceMotion(FName Id,bool Loop)
{
    FString Error;
    auto Clip=FOGSharedReferenceLibrary::Load(Id,Error);
    TArray<TArray<int32>> Mapping;
    if (!Clip || !PrepareReference(*Clip,Mapping,Error)) return false;
    FOGSharedMotionEntry Entry; Entry.SourceClipId=Id; Entry.MotionId=Id;
    Entry.GameplayAction=FName(*(TEXT("Library.Source.")+Id.ToString()));
    Entry.bLoop=Loop;
    return StartReference(Entry,MoveTemp(Clip),MoveTemp(Mapping));
}
bool UOGSharedMotionControllerComponent::StartReference(const FOGSharedMotionEntry& Entry,
    TSharedPtr<const FOGReferenceMotionClip,ESPMode::ThreadSafe> Clip,TArray<TArray<int32>>&& Mapping)
{
    if (!Clip || Mapping.Num()!=RegisteredParts.Num()) return false;
    // Every native part is checked before interrupting the previous action.
    StopAction();
    ReferenceClip=MoveTemp(Clip); ReferenceMappings=MoveTemp(Mapping);
    for (USkeletalMeshComponent* Part:RegisteredParts)
    {
        FReferencePartRestore Saved; Saved.Part=Part; Saved.Mesh=Part->GetSkeletalMeshAsset();
        Saved.Mode=static_cast<uint8>(Part->GetAnimationMode());
        Saved.PreviousClass=TStrongObjectPtr<UClass>(Part->GetAnimClass());
        Saved.Leader=Part->GetLeaderPoseComponent();
        if (auto* Node=Part->GetSingleNodeInstance())
        {
            Saved.PreviousAsset=TStrongObjectPtr<UAnimationAsset>(Node->GetCurrentAsset());
            Saved.Rate=Node->GetPlayRate(); Saved.Position=Node->GetCurrentTime();
            Saved.bLoop=Node->IsLooping(); Saved.bWasPlaying=Node->IsPlaying();
        }
        ReferenceRestores.Add(MoveTemp(Saved));
        Part->SetLeaderPoseComponent(nullptr);
        Part->SetAnimInstanceClass(UOGSharedReferenceAnimInstance::StaticClass());
        auto* Instance=Cast<UOGSharedReferenceAnimInstance>(Part->GetAnimInstance());
        ReferenceRestores.Last().Instance=Instance;
        if (!Instance) { StopAction(); return false; }
    }
    bPlaying=true; bLooping=Entry.bLoop; PlaybackRate=Entry.PlayRate;
    SequenceSeconds=FMath::Max(ReferenceClip->Duration,1.f/ReferenceClip->Rate);
    CurrentAction=Entry.GameplayAction; CurrentMotionId=Entry.MotionId; CurrentSourceClipId=ReferenceClip->Id;
    AuthoredMorphTracks=Entry.MorphTracks;
    bHasContact=Entry.ContactSeconds>=0; SourceContactSeconds=Entry.ContactSeconds;
    ElapsedSeconds=0; LastContactCycle=-1; bSceneTimeLocked=false;
    UpdateReferencePose(); SetComponentTickEnabled(true);
    return true;
}
void UOGSharedMotionControllerComponent::UpdateReferencePose()
{
    if (!ReferenceClip) return;
    const double Raw=static_cast<double>(ElapsedSeconds)*PlaybackRate;
    const double Time=bLooping && SequenceSeconds>0 ? FMath::Fmod(Raw,static_cast<double>(SequenceSeconds)) : Raw;
    for (int32 I=0; I<ReferenceRestores.Num(); ++I)
    {
        auto* Part=ReferenceRestores[I].Part.Get();
        auto* Instance=ReferenceRestores[I].Instance.Get();
        if (!Part || !Instance || Part->GetAnimInstance()!=Instance ||
            ReferenceRigs[I].Mesh.Get()!=Part->GetSkeletalMeshAsset())
        { StopAction(); return; }
        TArray<FTransform> Pose;
        if (!ReferenceRigs[I].Evaluate(*ReferenceClip,ReferenceMappings[I],Time,Pose))
        { StopAction(); return; }
        Instance->SetSharedPose(Pose);
    }
}
void UOGSharedMotionControllerComponent::RestoreReferencePose()
{
    for (auto& Saved:ReferenceRestores)
    {
        auto* Part=Saved.Part.Get();
        if (!Part || (Saved.Instance.IsValid() ? Part->GetAnimInstance()!=Saved.Instance.Get() :
            Part->GetAnimClass()!=UOGSharedReferenceAnimInstance::StaticClass())) continue;
        if (Part->GetSkeletalMeshAsset()!=Saved.Mesh.Get())
        {
            // A real form/wardrobe replacement owns the new rig; never replay an old-rig asset on it.
            Part->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            Part->Stop();
            continue;
        }
        if (static_cast<EAnimationMode::Type>(Saved.Mode)==EAnimationMode::AnimationSingleNode)
        {
            Part->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            if (Saved.PreviousAsset.IsValid())
            {
                Part->PlayAnimation(Saved.PreviousAsset.Get(),Saved.bLoop);
                Part->SetPosition(Saved.Position,false); Part->SetPlayRate(Saved.Rate);
                if (!Saved.bWasPlaying) Part->Stop();
            }
        }
        else
        {
            Part->SetAnimInstanceClass(Saved.PreviousClass.Get());
            Part->SetAnimationMode(static_cast<EAnimationMode::Type>(Saved.Mode));
        }
        Part->SetLeaderPoseComponent(Saved.Leader.Get());
    }
    ReferenceRestores.Reset(); ReferenceMappings.Reset(); ReferenceClip.Reset();
}
