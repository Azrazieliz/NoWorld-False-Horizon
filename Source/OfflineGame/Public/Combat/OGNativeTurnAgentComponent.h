#pragma once

#include "CoreMinimal.h"
#include "Combat/OGTurnBattle.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "OGNativeTurnAgentComponent.generated.h"

class UOGGameCoreSubsystem;
class UOGCanonicalCombatActorBindingComponent;
class UOGNativeCombatAbility;

UENUM(BlueprintType)
enum class EOGNativeTurnTarget : uint8
{
    Self, FirstEnemy, LowestHealthEnemy, HighestAttackEnemy,
    LowestHealthAlly, AllActiveEnemies, AllActiveAllies, DefeatedAlly
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeTurnRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RuleId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId SkillId;
    /** Exact installed ordinary effect definition; special kit executors may already be installed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UOGNativeCombatAbility> NativeAbility;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOGNativeTurnTarget Targets = EOGNativeTurnTarget::FirstEnemy;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bIncludeSelf = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RequiredPhase;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bUseOwnHealthWindow = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10000")) int32 MinimumOwnHealthBps = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="10000")) int32 MaximumOwnHealthBps = 10000;
    /** Authored command context, e.g. an exact Ultimate branch; never damage calculated by AI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ResolutionJson = TEXT("{}");
};

/** Source-authored decisions for one exact NPC Identity/kit. No default attacks or damage. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGNativeTurnPolicy : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId IdentityId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId VersionId;
    /** These exact string fields already exist in the canonical NPC entity JSON. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CanonicalIdentityField;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CanonicalVersionField;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CanonicalLifeField;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString RequiredLifeValue;
    /** Optional canonical string state used by authored boss/encounter phases. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName CanonicalPhaseField;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float DecisionDelaySeconds = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGNativeTurnRule> Rules;

    bool Validate(FString& Error) const;
    /** Pure selection. Caller verifies the canonical NPC and native physical body separately. */
    bool BuildDecision(const FOGTurnBattleState& State, const FOGEntityId& SourceId,
        FName CanonicalPhase, FOGResolvedCombatAction& Out, FName& OutRule, FString& Error,
        const TSet<FName>* ExcludedRules = nullptr) const;
};

/** Drives only a configured real opponent's own turns in the GameCore-owned battle. */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGNativeTurnAgentComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGNativeTurnAgentComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|AI") TSoftObjectPtr<UOGNativeTurnPolicy> Policy;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|AI") FString Status;
    /** Called by the real encounter factory after it registers the existing world NPC/unit. */
    UFUNCTION(BlueprintCallable, Category="Combat|AI")
    bool ConfigureBattleUnit(const FOGEntityId& BattleId, const FOGEntityId& UnitId, FString& Error);
    UFUNCTION(BlueprintCallable, Category="Combat|AI") void ReleaseBattleUnit();
    /** Explicit retry after the actual content/authority dependency was repaired. */
    UFUNCTION(BlueprintCallable, Category="Combat|AI") void RetryDecision();
    UFUNCTION(BlueprintCallable, Category="Combat|AI") bool StepOwnTurn(FString& Error);
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<UOGNativeTurnPolicy> ActivePolicy;
    TWeakObjectPtr<UOGGameCoreSubsystem> Core;
    TWeakObjectPtr<UOGCanonicalCombatActorBindingComponent> Body;
    FOGEntityId BoundBattleId;
    FOGEntityId BoundUnitId;
    FOGEntityId LastAutoBattleId;
    FOGEntityId LastAutoUnitId;
    int64 LastAutoAbilityRevision=-1;
    int64 FailedSequence = -1;
    int64 FailedAbilityRevision = -1;
    double ReadyAtSeconds = -1.0;
    int64 ObservedSequence = -1;
    bool bSubmitting = false;
    bool bAutoBindingEnabled = true;
    bool ReadCanonicalPhase(const FOGCombatUnitState& Unit, FName& OutPhase, FString& Error) const;
    void HandleRuntimeReleasing();
};
