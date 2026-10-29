#pragma once
#include "CoreMinimal.h"
#include "Animation/OGSharedMotionLibrary.h"
#include "Content/OGSystemBindingComponent.h"
#include "Core/OGEntityId.h"
#include "OGSharedAnimationRoutes.generated.h"

class UOGSharedMotionControllerComponent;
UENUM(BlueprintType)
enum class EOGSharedAnimationKind : uint8 { Body, Expression };
UENUM(BlueprintType)
enum class EOGSharedMovementCondition : uint8
{ Any, Grounded, Stationary, Moving, Rising, Falling, Swimming, Diving, Flying, Climbing, Mounted, Vehicle };
/** Read from the actual movement/traversal components. No character-name or Version selection. */
struct OFFLINEGAME_API FOGSharedMotionEnvironment
{
    EOGSharedMovementCondition Mode=EOGSharedMovementCondition::Grounded;
    bool bFalling=false;
    float Speed=0,VerticalSpeed=0;
    TArray<FName> Capabilities;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGSharedSystemAnimationRoute
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName SourceSystem;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Event;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGSharedAnimationKind Kind=EOGSharedAnimationKind::Body;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Action;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Context;
    /** Expression-only source curves can coexist with movement, combat and a synchronized body pose. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGSharedMorphTrack> MorphTracks;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float DurationSeconds=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float ContactSeconds=-1;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGSharedAutomaticAnimationRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName RuleId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName SourceSystem;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Event;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGSharedMovementCondition Condition=EOGSharedMovementCondition::Any;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float MinimumSpeed=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float MaximumSpeed=-1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FName> RequiredCapabilities;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Priority=0;
    bool Matches(const FOGSharedMotionEnvironment& Environment) const;
};
/** One reusable subsystem/event map; any number of actual rigs connect to it through a system profile. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGSharedAnimationRoutes : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGSharedAnimationRoutes() { System=TEXT("animation_routes"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGSharedSystemAnimationRoute> Routes;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGSharedAutomaticAnimationRule> AutomaticRules;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    const FOGSharedSystemAnimationRoute* FindRoute(FName SourceSystem,FName Event) const;
    bool SelectAutomatic(const FOGSharedMotionEnvironment& Environment,int32& RouteIndex,FString& Error) const;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOGSharedSystemAnimationSignal,
    FName,SourceSystem,FName,Event,FOGEntityId,RequestId,FName,Signal);
/** Presentation receiver only. Gameplay systems keep ownership, damage, state and consequence authority. */
UCLASS(ClassGroup=(OfflineGame),meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGSharedAnimationReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGSharedAnimationReceiver();
    UPROPERTY(BlueprintReadOnly) FString Status;
    UPROPERTY(BlueprintAssignable) FOGSharedSystemAnimationSignal OnSystemAnimation;
    virtual bool PrepareDefinition_Implementation(UDataAsset*,const FOGDefinitionBindingContext&,FString&) override;
    virtual void ActivateDefinition_Implementation() override;
    virtual void ReleaseDefinition_Implementation() override;
    UFUNCTION(BlueprintCallable,Category="Animation|Systems")
    bool PlaySystemEvent(FName SourceSystem,FName Event,const FOGEntityId& RequestId,FString& Error);
    UFUNCTION(BlueprintCallable,Category="Animation|Systems") void StopSystemEvent(const FOGEntityId& RequestId);
    static bool DrivesAutomaticMotion(const AActor* Actor);
protected:
    virtual void TickComponent(float,ELevelTick,FActorComponentTickFunction*) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TWeakObjectPtr<UOGSharedMotionControllerComponent> Motion;
    TMap<FOGEntityId,FString> Receipts;
    struct FExpression { int32 Route=INDEX_NONE; FOGEntityId Request; double Elapsed=0; bool ContactSent=false; };
    TArray<FExpression> Expressions;
    int32 BodyRoute=INDEX_NONE;
    FOGEntityId BodyRequest;
    uint64 BodyGeneration=0;
    bool bAutomaticBody=false;
    FString LastAutomaticFailure;
    bool ResolveMotion(FString& Error);
    bool BodyIsOwned() const;
    bool IsGameplayBodyBusy() const;
    void StopOwnedBody();
    void ClearExpression(const FExpression&);
    FName ExpressionLayer(const FExpression&,FName OriginalLayer) const;
    bool ApplyExpression(const FExpression&,double Time,FString& Error);
    UFUNCTION() void HandleContact(FName Action,FName MotionId);
    UFUNCTION() void HandleFinished(FName Action,FName MotionId);
};
