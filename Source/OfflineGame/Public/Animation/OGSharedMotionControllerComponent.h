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
    FName AnatomyProfile = TEXT("humanoid");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    FName OutfitProfile = TEXT("CLOTHED");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    TArray<FName> Capabilities;

    /** Identical full-bone hierarchy and rest poses permit one driver with leader-pose followers.
     * Otherwise ALL registered parts must accept the exact same clip Skeleton. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    bool bUseIdenticalTopologyLeaderPose = false;

    /** Keep same source-action phase across independently rigged visual parts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Character")
    bool bPhaseLockIndependentParts = true;

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
        int32 ParticipantCount = 1);

    UFUNCTION(BlueprintCallable, Category="Motion|Shared")
    void StopAction();

    UFUNCTION(BlueprintPure, Category="Motion|Shared")
    bool IsPlayingAction() const { return bPlaying; }

    UFUNCTION(BlueprintPure, Category="Motion|Shared")
    int32 GetNativePartCount() const { return RegisteredParts.Num(); }

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

    static bool HaveIdenticalReferencePose(const USkeletalMeshComponent* A,
        const USkeletalMeshComponent* B);
};
