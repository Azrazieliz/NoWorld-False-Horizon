#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OGSharedMotionSceneComponent.generated.h"

class UOGSharedMotionControllerComponent;

/** Roles are defined by the source animation, not by actor identity or Version. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGSharedMotionParticipant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Scene")
    TObjectPtr<UOGSharedMotionControllerComponent> Controller = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Scene")
    FName SourceRole = NAME_None;
};

/**
 * Synchronized real native AnimSequence playback across authored role tracks.
 * Authoritative scene/gameplay eligibility is verified by the caller; this
 * component independently requires compatible approved native source clips.
 */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGSharedMotionSceneComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGSharedMotionSceneComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion|Scene")
    TArray<FOGSharedMotionParticipant> Participants;

    UPROPERTY(BlueprintReadOnly, Category="Motion|Scene")
    FName ActiveSceneAction = NAME_None;

    /** No partially animated scene: each role/rig and original source is checked first. */
    UFUNCTION(BlueprintCallable, Category="Motion|Scene")
    bool StartSynchronizedScene(FName Action, FName Context,
        bool bAuthorizedAdultContext = false);

    UFUNCTION(BlueprintCallable, Category="Motion|Scene")
    void StopSynchronizedScene();

    UFUNCTION(BlueprintPure, Category="Motion|Scene")
    bool IsScenePlaying() const { return bScenePlaying; }

protected:
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool bScenePlaying = false;
    TArray<TWeakObjectPtr<UOGSharedMotionControllerComponent>> ActiveControllers;
};
