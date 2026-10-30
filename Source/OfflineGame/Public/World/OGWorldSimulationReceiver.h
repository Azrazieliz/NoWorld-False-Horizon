#pragma once

#include "CoreMinimal.h"
#include "Content/OGSystemBindingComponent.h"
#include "Components/SceneComponent.h"
#include "Runtime/OGFoundationWorldSimulationRuntime.h"
#include "Subsystems/WorldSubsystem.h"
#include "OGWorldSimulationReceiver.generated.h"

class AAIController;
class AController;
class APawn;
class UOGInteractionParticipantComponent;
class UPathFollowingComponent;
struct FOGTurnBattleState;
class UOGWorldSimulationReceiver;
struct FAIRequestID;
struct FPathFollowingResult;

/** Shared policy: exact state selectors and tolerances come from source content.
 * Schedule identity is read from an existing canonical entity JSON field. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGWorldSimulationDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGWorldSimulationDefinition();
    /** Opt-in for measured local-player presence. AI remains the default.
     * Controlled mode must omit schedule/arrival selectors and AI acceptance radius. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bControlledPresence = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName EntityKind;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ScheduleIdField;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName MovementContext;
    /** All entries must match to execute a schedule. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FString> RequiredStateValues;
    /** A nonempty, fully matching selector suppresses the real body's collision/visibility. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FString> AbsentStateValues;
    /** Optional authored state transition at arrival; unrelated JSON is retained. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ArrivalStateField;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ArrivalStateValue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float AcceptanceRadiusCm = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float RefreshIntervalSeconds = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int64 CheckpointIntervalWorldTicks = 0;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};

/** Attach to a real streamed location actor. No location or actor is created.
 * Transform defines the persisted location-local coordinate frame. */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGWorldSimulationLocationComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly) FOGEntityId LocationId;
protected:
    virtual void OnUnregister() override;
};

/** Enforces one physical writer per entity across streamed actors in this World. */
UCLASS()
class OFFLINEGAME_API UOGWorldSimulationBodyRegistry : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase&) override;
    virtual void Deinitialize() override;
    bool CanClaim(const FOGEntityId&, const UOGWorldSimulationReceiver*) const;
    bool Claim(const FOGEntityId&, UOGWorldSimulationReceiver*);
    bool Owns(const FOGEntityId&, const UOGWorldSimulationReceiver*) const;
    void Release(const FOGEntityId&, const UOGWorldSimulationReceiver*);
    /** Explicit save/backup boundary. False means at least one controlled body
     * could not safely contribute; the caller must report/handle that result. */
    bool CheckpointControlledBodies(FString& Error);
    void HandleLocationUnregistering(UOGWorldSimulationLocationComponent* Frame);
private:
    TMap<FOGEntityId, TWeakObjectPtr<UOGWorldSimulationReceiver>> Bodies;
    FDelegateHandle DeactivateHandle, BackgroundHandle, TearDownHandle;
    void HandleSaveBoundary();
    void HandleWorldTearDown(UWorld*);
};

/** Drives an existing AI pawn through actual path following, or observes an
 * explicitly configured genuine locally controlled body using the same Store. Committed events
 * invalidate canonical reads; they are never replayed as consequence commands.
 * Each operation borrows the current Store only for that call. */
UCLASS(ClassGroup=(OfflineGame))
class OFFLINEGAME_API UOGWorldSimulationReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGWorldSimulationReceiver();
    virtual bool PrepareDefinition_Implementation(UDataAsset*, const FOGDefinitionBindingContext&, FString&) override;
    virtual bool IsWaitingForNativePrerequisites() const override { return bWaitingForNativePrerequisites; }
    virtual void ActivateDefinition_Implementation() override;
    virtual void ReleaseDefinition_Implementation() override;
    /** Binding host retains only these actor flags while canonical state is unavailable. */
    bool GetOriginalBodyFlags(bool& Hidden, bool& Collision) const
    { Hidden = bOriginalHidden; Collision = bOriginalCollision; return bCapturedBodyFlags; }
    /** Transfer retained flags without revealing an unrehydrated body. */
    void AdoptOriginalBodyFlags(bool Hidden,bool Collision)
    { if (!bActive) { bOriginalHidden=Hidden; bOriginalCollision=Collision; bCapturedBodyFlags=true; } }
    void HoldSuppressedOnRelease() { bSuppressOnRelease=true; }
    bool IsBodyActive() const { return bActive; }
    bool IsControlledPresence() const { return Prepared && Prepared->bControlledPresence; }
    /** Save before changing possession or staging a body. This never restores,
     * activates an AI schedule, or adopts a newer canonical read fence. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|World|Presence")
    bool CheckpointControlledPresence(FString& Error);
    /** Checkpoint while authority is still held, then block subsequent writes.
     * Suspension still takes effect if checkpointing fails. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|World|Presence")
    bool SuspendControlledPresence(FString& Error);
    UFUNCTION(BlueprintCallable, Category="OfflineGame|World|Presence")
    void ResumeControlledPresence();
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) FString Status;
protected:
    virtual void TickComponent(float, ELevelTick, FActorComponentTickFunction*) override;
    virtual void EndPlay(const EEndPlayReason::Type) override;
private:
    /** Native residency only; never persisted as canonical state. */
    UPROPERTY(Transient) bool bWaitingForNativePrerequisites = false;
    UPROPERTY(Transient) TObjectPtr<UOGWorldSimulationDefinition> Prepared;
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    TWeakObjectPtr<AAIController> MoveController;
    TWeakObjectPtr<AController> ControlledController;
    TWeakObjectPtr<UPathFollowingComponent> ControlledPath;
    bool bObservedControlledPath = false;
    TWeakObjectPtr<UOGInteractionParticipantComponent> ControlledParticipant;
    FDelegateHandle InteractionHandle, BattleHandle;
    FTransform AcceptedFrameTransform = FTransform::Identity;
    bool bControlledSuspended = false;
    bool bControlledCheckpointing = false;
    friend class UOGWorldSimulationBodyRegistry;
    friend struct FOGControlledPresenceTestAccess;
    TWeakObjectPtr<UOGWorldSimulationLocationComponent> LocationFrame;
    TSharedPtr<FOGWorldStoreCommittedEvents> EntityFeed;
    TSharedPtr<FOGWorldStoreCommittedEvents> ScheduleFeed;
    FDelegateHandle EntityHandle, ScheduleHandle, ReleaseHandle;
    FOGEntityId ObservedSchedule;
    FOGFoundationWorldBodyState Accepted;
    uint32 MoveRequestId = 0;
    uint32 CompletedMoveRequestId = 0;
    int64 LastCheckpointTick = -1;
    friend struct FOGWorldSimulationBodyTestAccess;
    bool bRestoreRequired = true;
    bool bSuppressOnRelease = false;
    static bool OwnsCheckpointRequest(uint32 Request,bool Arrival,uint32 Completed,uint32 Current,bool CurrentValid,bool Idle);
    bool bActive = false;
    bool bHasAccepted = false;
    bool bArrivalPending = false;
    bool bCapturedBodyFlags = false;
    bool bOriginalHidden = false;
    bool bOriginalCollision = false;
    void Refresh();
    void RefreshControlled();
    bool ValidateControlledPawn(FString&, AController* DepartingController = nullptr) const;
    bool SaveControlledCheckpoint(AController* DepartingController, FString&);
    UFUNCTION()
    void HandleControllerChanged(APawn*, AController* OldController, AController* NewController);
    bool HasControlledPreemption() const;
    void InvalidateControlledPresence();
    void HandleControlledBoundary();
    void HandleInteractionSignificance(bool, bool);
    void HandleBattleCommitted(const FOGTurnBattleState&, const FOGEntityId&);
    void WatchControlledPath();
    void HandleControlledMoveCompleted(FAIRequestID, const FPathFollowingResult&);
    bool FindLocationFrame(const FOGEntityId&, FString&);
    void WatchSchedule(const FOGEntityId&);
    void HandleCommitted(const TArray<FOGWorldEvent>&);
    void HandleMoveCompleted(FAIRequestID, const FPathFollowingResult&);
    void StopOwnedMove();
    void SetBodySuppressed(bool);
    bool SavePhysicalCheckpoint(bool, FString&);
    void HandleRuntimeReleasing();
};
