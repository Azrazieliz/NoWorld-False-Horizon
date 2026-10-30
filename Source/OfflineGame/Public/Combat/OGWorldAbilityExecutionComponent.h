#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Combat/OGWorldAbilityDefinition.h"
#include "OGWorldAbilityExecutionComponent.generated.h"

class UOGGameCoreSubsystem;
class UOGCanonicalCombatActorBindingComponent;
class UOGWorldAbilityExecutionComponent;
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class USceneComponent;
class UOGSharedMotionControllerComponent;
class USkeletalMeshComponent;
class USkeletalMesh;

USTRUCT()
struct OFFLINEGAME_API FOGNativeProjectileCheckpoint
{
    GENERATED_BODY()
    UPROPERTY() int32 StepIndex=INDEX_NONE;
    UPROPERTY() FTransform Transform;
    UPROPERTY() FVector Velocity=FVector::ZeroVector;
    UPROPERTY() FVector LaunchOrigin=FVector::ZeroVector;
    UPROPERTY() float RemainingLifeSeconds=0;
};
USTRUCT()
struct OFFLINEGAME_API FOGWorldCastPresentationCheckpoint
{
    GENERATED_BODY()
    UPROPERTY() FOGEntityId CastId;
    UPROPERTY() TSet<int32> SpawnedSteps;
    UPROPERTY() TArray<FOGNativeProjectileCheckpoint> Projectiles;
    UPROPERTY() TMap<int32,FVector> PreviousStarts;
    UPROPERTY() TMap<int32,FVector> PreviousEnds;
};
USTRUCT()
struct OFFLINEGAME_API FOGCombatBodyCheckpoint
{
    GENERATED_BODY()
    UPROPERTY() FOGEntityId UnitId;
    UPROPERTY() FTransform Transform;
    UPROPERTY() bool bCharacterMovement=false;
    UPROPERTY() FVector Velocity=FVector::ZeroVector;
    UPROPERTY() uint8 MovementMode=0;
    UPROPERTY() uint8 CustomMovementMode=0;
};
USTRUCT()
struct OFFLINEGAME_API FOGWorldEncounterPresentationCheckpoint
{
    GENERATED_BODY()
    UPROPERTY() TArray<FOGCombatBodyCheckpoint> Bodies;
    UPROPERTY() TArray<FOGWorldCastPresentationCheckpoint> Casts;
};

/** Physical projectile for an already accepted source cast. No damage numbers or ownership live here. */
UCLASS(NotBlueprintable)
class OFFLINEGAME_API AOGNativeWorldProjectile : public AActor
{
    GENERATED_BODY()
public:
    AOGNativeWorldProjectile();
    bool InitializeAcceptedStep(UOGWorldAbilityExecutionComponent* Source,const FOGEntityId& Cast,int32 Step,
        const FOGWorldAbilityStep& Specification,const FVector& Direction,FString& Error);
    virtual void Tick(float DeltaSeconds) override;
    bool CaptureCheckpoint(FOGNativeProjectileCheckpoint& Out) const;
    bool RestoreCheckpoint(const FOGNativeProjectileCheckpoint& State,FString& Error);
private:
    UPROPERTY() TObjectPtr<USphereComponent> Collision;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<UProjectileMovementComponent> Movement;
    TWeakObjectPtr<UOGWorldAbilityExecutionComponent> Source;
    FOGEntityId CastId;
    int32 StepIndex=INDEX_NONE;
    FVector LaunchOrigin=FVector::ZeroVector;
    float MaximumTravelCentimeters=0;
    float RecoveryRemainingLife=0;
    bool bRecoveryPaused=false;
    UFUNCTION() void OnPhysicalHit(UPrimitiveComponent* HitComponent,AActor* Other,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
};

/** One reusable actor receiver for every admitted character/NPC. Mechanics remain in the shared executor. */
UCLASS(ClassGroup=(OfflineGame),meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGWorldAbilityExecutionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGWorldAbilityExecutionComponent();
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) FString Status;
    bool BeginNativeAction(const FOGResolvedCombatAction& Root,bool FromCanonicalAI,FString& Error);
    bool CommitProjectileContact(const FOGEntityId& Cast,int32 Step,AActor* Contact,FString& Error);
    bool HasActiveCast() const { return ActiveCast.IsValid(); }
    FOGEntityId GetActiveCastId() const { return ActiveCast; }
    void CancelActiveCast();
    bool IsRecoveryPaused() const;
    void CapturePresentationCheckpoint(FOGWorldCastPresentationCheckpoint& Out) const;
    bool CanRecoverAcceptedCast(const FOGWorldCastPresentationCheckpoint& State,FString& Error);
    bool RecoverAcceptedCast(const FOGWorldCastPresentationCheckpoint& State,FString& Error);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float,ELevelTick,FActorComponentTickFunction*) override;
private:
    TWeakObjectPtr<UOGGameCoreSubsystem> Core;
    FOGEntityId ActiveCast;
    FOGEntityId BattleId;
    FOGResolvedCombatAction Root;
    FOGWorldAbilitySpecification Specification;
    int64 StartedAt=0;
    TSet<int32> Completed;
    TSet<int32> SpawnedProjectiles;
    TArray<TWeakObjectPtr<AOGNativeWorldProjectile>> Projectiles;
    TMap<int32,FVector> PreviousStarts,PreviousEnds;
    bool bEndingLocal=false;
    bool bSourceMotionFinished=false;
    bool bTracksSourceMotion=false;
    TWeakObjectPtr<UOGSharedMotionControllerComponent> SourceMotion;
    struct FAcceptedNativePart
    {
        TWeakObjectPtr<USkeletalMeshComponent> Part;
        TWeakObjectPtr<USkeletalMesh> Mesh;
    };
    TArray<FAcceptedNativePart> SourceParts;
    friend struct FOGNativeCombatExecutionTestAccess;
    bool IsAcceptedBodyCurrent() const;
    bool TrackAcceptedMotion(FString& Error);
    bool CaptureSourceRig(UOGSharedMotionControllerComponent& Motion,FString& Error);
    void ClearSourceMotionTracking();
    UFUNCTION() void HandleSourceMotionFinished(FName Action,FName MotionId);
    bool ResolveRuntime(FString& Error);
    USceneComponent* TracePart(const FOGWorldAbilityStep& Step,FName& Start,FName& End,FString& Error) const;
    void QueryContacts(const FOGWorldAbilityStep&,int32 StepIndex,TArray<FOGEntityId>& Out);
    bool CommitContacts(int32 Step,const TArray<FOGEntityId>& Contacts,FString& Error);
    void EndLocalCast(bool CompletedNormally=false);
};
