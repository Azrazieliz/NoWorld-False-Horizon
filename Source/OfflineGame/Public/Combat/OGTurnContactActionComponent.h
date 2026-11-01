#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/OGCombatTypes.h"
#include "OGTurnContactActionComponent.generated.h"

class UOGSharedMotionControllerComponent;
class UOGGameCoreSubsystem;

/**
 * Real turn-mode action staging on the actual source character.
 *
 * An action has NO HP, charge, cost or turn effect until the authored source
 * clip reaches its contact notify. There is no fallback timer, fabricated
 * hit trace, visual-only HP meter or generic damage packet.
 *
 * World (real-time) combat continues to use UOGWorldAbilityExecutionComponent.
 * The staged action is transient; interrupted/unloaded actions never settle.
 */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGTurnContactActionComponent final : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGTurnContactActionComponent();

    bool StageActualTurnAction(const FOGResolvedCombatAction& Requested,
        FName ApprovedMotionSemantic, FString& OutError);

    /** Cancel without committing gameplay effects or spending resources. */
    void CancelUncommittedTurnAction();

    UFUNCTION(BlueprintPure, Category="Combat|Turn")
    bool IsAwaitingAuthoredContact() const { return bStaged; }

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleSourceContact(FName GameplayAction, FName MotionId);
    void HandleSourceMotionFinished(
        UOGSharedMotionControllerComponent* Source, FName GameplayAction,
        FName MotionId);
    bool IsSameLiveTurn(FString& Error) const;
    void DropStagedAction(bool bStopSourcePlayback);

    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    TWeakObjectPtr<UOGSharedMotionControllerComponent> BoundMotion;
    FOGResolvedCombatAction StagedAction;
    FOGEntityId BattleId;
    FName SourceSemantic = NAME_None;
    bool bStaged = false;
    bool bResolving = false;
};
