#pragma once
#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "Combat/OGCombatTypes.h"
#include "Effects/OGTriggerDefinition.h"
#include "OGNativeCombatPassive.generated.h"

UENUM(BlueprintType)
enum class EOGNativeReactionTarget : uint8
{ Owner, EventSource, EventTarget, FirstEnemy, LowestHealthEnemy, LowestHealthAlly };
UENUM(BlueprintType)
enum class EOGNativeReactionUnavailable : uint8 { Unspecified, Skip, RejectRootAction };

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativePassiveTrigger
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGTriggerDefinition Trigger;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeReactionTarget Target=EOGNativeReactionTarget::Owner;
    /** Source-authored finite activation limit for one root action and all its reactions. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumActivationsPerRoot=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Priority=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bAllowReserveSource=false;
    /** Explicit behavior for missing live targets, insufficient resources or a cooling-down reaction. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeReactionUnavailable Unavailable=EOGNativeReactionUnavailable::Unspecified;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativePassiveSpecification
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId PassiveSkillId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId IdentityId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId VersionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativePassiveTrigger> Triggers;
    bool Validate(FString& Error) const;
    bool MatchesSource(const FOGCombatUnitState& Unit) const;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativePassiveBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEntityId OwnerUnitId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGNativePassiveSpecification Specification;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeReactionRecord
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int64 QueueSequence=-1;
    UPROPERTY(BlueprintReadOnly) int64 SourceSequence=-1;
    UPROPERTY(BlueprintReadOnly) FOGContentId PassiveSkillId;
    UPROPERTY(BlueprintReadOnly) FOGContentId TriggerId;
    UPROPERTY(BlueprintReadOnly) FName EventType;
    UPROPERTY(BlueprintReadOnly) FOGEntityId EventSourceUnitId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId EventTargetUnitId;
    UPROPERTY(BlueprintReadOnly) FOGContentId EventSkillId;
    UPROPERTY(BlueprintReadOnly) FOGResolvedCombatAction Action;
    UPROPERTY(BlueprintReadOnly) bool bExecuted=false;
    UPROPERTY(BlueprintReadOnly) FString Outcome;
};
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGNativeCombatPassive : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGNativeCombatPassive();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGNativePassiveSpecification Specification;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
};
namespace OGNativePassivePredicates
{
    OFFLINEGAME_API bool IsSupported(FName Predicate);
}
