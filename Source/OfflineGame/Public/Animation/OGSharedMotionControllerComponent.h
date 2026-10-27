#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/OGSharedMotionLibrary.h"
#include "OGSharedMotionControllerComponent.generated.h"

class USkeletalMeshComponent;
class UAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOGSharedMotionSignal, FName, GameplayAction, FName, MotionId);

/**
 * Attach once to a character Actor, not once per Character Version.
 * Register the actual visual skeletal components after character/wardrobe installation.
 * All source selection is delegated to the exact-rig shared animation catalog.
 * Does not change gameplay authority, saves, inventory or ownership.
 */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGSharedMotionControllerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UOGSharedMotionControllerComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Shared")
    TSoftObjectPtr<UOGSharedMotionLibrary> SharedLibrary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    FName RigProfile = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    FName AnatomyProfile = FName(TEXT("humanoid"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    FName OutfitProfile = FName(TEXT("CLOTHED"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    TArray<FName> Capabilities;

    /** Identical full-bone hierarchy and rest poses permit one driver with leader-pose followers.
     * Otherwise ALL registered parts must accept the exact same clip Skeleton. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    bool bUseIdenticalTopologyLeaderPose = false;

    /** Keep same source-action phase across independently rigged visual parts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    bool bPhaseLockIndependentParts = true;

    /** Required to apply authored root movement to the owning Character's
     * movement component using collision sweeps; never implicitly authorized. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Root")
    bool bPermitAuthoritativeRootMotion = false;

    /** A presentation-only contact signal; the combat executor remains authoritative. */
    UPROPERTY(BlueprintAssignable, Category="Motion|Events")
    FOGSharedMotionSignal OnAuthoredContact;

    UPROPERTY(BlueprintAssignable, Category="Motion|Events")
    FOGSharedMotionSignal OnMotionFinished;

    UPROPERTY(BlueprintReadOnly, Category="Motion|State")
    FName CurrentAction = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Motion|State")
    FName CurrentMotionId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Motion|State")
    FName CurrentSourceClipId = NAME_None;

    /** Configures existing components only; never constructs or replaces mesh assets. */
    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    bool RegisterNativeParts(const TArray<USkeletalMeshComponent*>& Parts);

    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    void SetCurrentOutfit(FName InOutfitProfile);

    /** Caller must have completed gameplay authority checks and adult context authorization. */
    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    bool PlayAction(FName GameplayAction, FName Context, bool bAdultContextAuthorized = false,
        int32 ParticipantCount = 1, FName ParticipantRole = NAME_None);

    /** Pure preflight for synchronized scene start. No actor or animation state changes. */
    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    bool CanPlayAction(FName GameplayAction, FName Context,
        bool bAdultContextAuthorized = false, int32 ParticipantCount = 1,
        FName ParticipantRole = NAME_None) const;

    /** Scene preflight: exposes original source identity, never a guessed role match. */
    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    bool PreviewSourceClipId(FName GameplayAction, FName Context,
        bool bAdultContextAuthorized, int32 ParticipantCount,
        FName ParticipantRole, FName& OutSourceClipId) const;

    /** Authored scene group, not the individual source clip ID. Required to
     * prove synchronized roles are from the SAME genuine source scene. */
    UFUNCTION(BlueprintCallable, Category="Motion|Scene")
    bool PreviewAuthoredSceneId(FName GameplayAction, FName Context,
        bool bAdultContextAuthorized, int32 ParticipantCount,
        FName ParticipantRole, FName& OutSceneId) const;

    /** One global time origin for every participant. Each actor remains independently animated. */
    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    void SynchronizeToWorldTime(float SceneStartTime);

    /** Changes presentation speed continuously without changing authored timing.
     * Zero pauses only this action; canonical World time continues. */
    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    bool SetExternalPacing(float Rate);

    UFUNCTION(BlueprintPure, Category="Motion|Shared")
    float GetExternalPacing() const { return ExternalPacing; }

    /** An authored facial/secondary morph layer that does not replace body animation. */
    UFUNCTION(BlueprintCallable, Category="Motion|Layers")
    bool SetMorphLayer(FName Layer, FName Morph, float Weight);

    UFUNCTION(BlueprintCallable, Category="Motion|Layers")
    void ClearMorphLayer(FName Layer);

    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    void StopAction();

    UFUNCTION(BlueprintPure, Category="Motion|Shared")
    bool IsPlayingAction() const { return bPlaying; }

    UFUNCTION(BlueprintPure, Category="Motion|Shared")
    int32 GetNativePartCount() const { return RegisteredParts.Num(); }
    const TArray<TObjectPtr<USkeletalMeshComponent>>& GetNativeParts() const { return RegisteredParts; }

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> RegisteredParts;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> PlayingParts;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> CurrentLeader;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> CurrentSequence;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UAnimSequence>> PlayingSequences;

    TArray<float> PlayingSequenceLengths;

    bool bPlaying = false;
    bool bLooping = false;
    bool bHasContact = false;
    float SourceContactSeconds = -1.f;
    float ElapsedSeconds = 0.f;
    float SequenceSeconds = 0.f;
    float PlaybackRate = 1.f;
    int32 LastContactCycle = -1;
    bool bApplyingRootMotion = false;
    bool bSceneTimeLocked = false;
    float SceneWorldStartTime = 0.f;
    float SceneAnchorElapsed = 0.f;
    float ExternalPacing = 1.f;

    struct FMorphOverlay
    {
        FName Layer = NAME_None;
        FName Morph = NAME_None;
        float Weight = 0.f;
    };
    TArray<FMorphOverlay> MorphOverlays;
    TSet<FName> AppliedMorphs;
    TArray<FOGSharedMorphTrack> AuthoredMorphTracks;

    void ApplyMorphOverlays();
    bool ResolveForPart(const UOGSharedMotionLibrary* Library,
        USkeletalMeshComponent* Part, FName Action, FName Context,
        bool bAuthorizedAdult, int32 ParticipantCount, FName ParticipantRole,
        UAnimSequence*& OutSequence, const FOGSharedMotionEntry*& OutEntry) const;
    void ApplyAuthoredRootMotion(float PreviousSourceTime, float SourceTime);

    static bool HaveIdenticalReferencePose(const USkeletalMeshComponent* A,
        const USkeletalMeshComponent* B);
};
