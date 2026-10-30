#pragma once

#include "CoreMinimal.h"
#include "Content/OGSystemBindingComponent.h"
#include "Components/SceneComponent.h"
#include "Runtime/OGFoundationWorldSimulationRuntime.h"
#include "Subsystems/WorldSubsystem.h"
#include "OGWorldSimulationReceiver.generated.h"

class AAIController;
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
};

/** Enforces one physical writer per entity across streamed actors in this World. */
UCLASS()
class OFFLINEGAME_API UOGWorldSimulationBodyRegistry : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    bool CanClaim(const FOGEntityId&, const UOGWorldSimulationReceiver*) const;
    bool Claim(const FOGEntityId&, UOGWorldSimulationReceiver*);
    bool Owns(const FOGEntityId&, const UOGWorldSimulationReceiver*) const;
    void Release(const FOGEntityId&, const UOGWorldSimulationReceiver*);
private:
    TMap<FOGEntityId, TWeakObjectPtr<UOGWorldSimulationReceiver>> Bodies;
};

/** Drives an existing AI pawn through actual path following. Committed events
 * invalidate canonical reads; they are never replayed as consequence commands.
 * Each operation borrows the current Store only for that call. */
UCLASS(ClassGroup=(OfflineGame))
class OFFLINEGAME_API UOGWorldSimulationReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGWorldSimulationReceiver();
    virtual bool PrepareDefinition_Implementation(UDataAsset*, const FOGDefinitionBindingContext&, FString&) override;
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
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) FString Status;
protected:
    virtual void TickComponent(float, ELevelTick, FActorComponentTickFunction*) override;
    virtual void EndPlay(const EEndPlayReason::Type) override;
private:
    UPROPERTY(Transient) TObjectPtr<UOGWorldSimulationDefinition> Prepared;
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    TWeakObjectPtr<AAIController> MoveController;
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
    bool FindLocationFrame(const FOGEntityId&, FString&);
    void WatchSchedule(const FOGEntityId&);
    void HandleCommitted(const TArray<FOGWorldEvent>&);
    void HandleMoveCompleted(FAIRequestID, const FPathFollowingResult&);
    void StopOwnedMove();
    void SetBodySuppressed(bool);
    bool SavePhysicalCheckpoint(bool, FString&);
    void HandleRuntimeReleasing();
};
