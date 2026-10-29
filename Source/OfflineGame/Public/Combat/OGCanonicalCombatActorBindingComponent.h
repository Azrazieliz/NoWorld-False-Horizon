#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/OGEntityId.h"
#include "Content/OGContentId.h"
#include "Math/OGLargeNumber.h"
#include "OGCanonicalCombatActorBindingComponent.generated.h"

class UOGGameCoreSubsystem;
struct FOGResolvedCombatAction;
struct FOGTurnBattleState;

/** Physical 3D body of an existing canonical encounter unit; no authority over
 * combat, ownership, NPC identity, saved state, damage or content approval.
 * An actual character/NPC factory installs and binds this to its source mesh.
 */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGCanonicalCombatActorBindingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGCanonicalCombatActorBindingComponent();
    /** Calls against the actual GameCore battle; rejects fake unit IDs, cross-
     * world actors, absent real geometry, duplicates and wrong source identity.
     * Returns false without changing any previous valid binding. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Combat|World")
    bool BindActualNativeUnit(const FOGEntityId& UnitId,
        const FOGContentId& ApprovedIdentity, FString& OutError);

    /** A visual unbind only; cannot remove or change a canonical battle unit. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Combat|World")
    void UnbindVisual();

    bool IsBoundTo(const FOGEntityId& BattleId, const FOGEntityId& UnitId) const;
    FOGEntityId GetBoundBattleId() const { return BoundBattleId; }
    FOGEntityId GetBoundUnitId() const { return BoundUnitId; }
    static AActor* FindActualWorldActor(UWorld* World,
        const FOGEntityId& BattleId, const FOGEntityId& UnitId);

protected:
    virtual void TickComponent(float,ELevelTick,FActorComponentTickFunction*) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    FOGEntityId LastPresentedAction;
    FOGLargeNumber LastVisualHp;
    bool bHasVisualHp=false;
    void HandleCommittedState(const FOGTurnBattleState& State,const FOGEntityId& Receipt);
    void HandleNativeAction(const FOGResolvedCombatAction& Action);
    UPROPERTY(Transient)
    FOGEntityId BoundBattleId;
    UPROPERTY(Transient)
    FOGEntityId BoundUnitId;
};
