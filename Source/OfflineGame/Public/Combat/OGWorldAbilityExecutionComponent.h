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
private:
    UPROPERTY() TObjectPtr<USphereComponent> Collision;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<UProjectileMovementComponent> Movement;
    TWeakObjectPtr<UOGWorldAbilityExecutionComponent> Source;
    FOGEntityId CastId;
    int32 StepIndex=INDEX_NONE;
    FVector LaunchOrigin=FVector::ZeroVector;
    float MaximumTravelCentimeters=0;
    UFUNCTION() void OnPhysicalHit(UPrimitiveComponent*,AActor*,UPrimitiveComponent*,FVector,const FHitResult&);
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
    bool ResolveRuntime(FString& Error);
    USceneComponent* TracePart(const FOGWorldAbilityStep& Step,FName& Start,FName& End,FString& Error) const;
    void QueryContacts(const FOGWorldAbilityStep&,int32 StepIndex,TArray<FOGEntityId>& Out);
    bool CommitContacts(int32 Step,const TArray<FOGEntityId>& Contacts,FString& Error);
    void EndLocalCast();
};
