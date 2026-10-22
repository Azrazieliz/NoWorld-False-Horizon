#include "Animation/OGSharedMotionControllerComponent.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

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
    StopAction();
    RegisteredParts = MoveTemp(Verified);
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

bool UOGSharedMotionControllerComponent::PlayAction(
    FName GameplayAction, FName Context, bool bAdultContextAuthorized,
    int32 ParticipantCount)
{
    if (RigProfile.IsNone() || GameplayAction.IsNone() ||
        Context.IsNone() || RegisteredParts.IsEmpty()) return false;

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
            ParticipantCount, &Entry);
        if (!Sequence || !Entry || !Sequence->GetSkeleton() ||
            Entry->bUsesRootMotion) return false;

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
    bLooping = PrimaryEntry->bLoop;
    bHasContact = PrimaryEntry->ContactSeconds >= 0.f;
    SourceContactSeconds = PrimaryEntry->ContactSeconds;
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
    SetComponentTickEnabled(false);
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
    bPlaying = false;
    bLooping = false;
    bHasContact = false;
    SourceContactSeconds = -1.f;
    ElapsedSeconds = 0.f;
    SequenceSeconds = 0.f;
    LastContactCycle = -1;
}

void UOGSharedMotionControllerComponent::TickComponent(
    float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    if (!bPlaying || DeltaSeconds <= 0.f) return;
    ElapsedSeconds += DeltaSeconds;
    const float SourceTime = ElapsedSeconds * PlaybackRate;
    const int32 Cycle = bLooping
        ? FMath::FloorToInt(SourceTime / SequenceSeconds) : 0;
    if (!CurrentLeader && bPhaseLockIndependentParts)
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
    RegisteredParts.Reset();
    Super::EndPlay(EndPlayReason);
}
