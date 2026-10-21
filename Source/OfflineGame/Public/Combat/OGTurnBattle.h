#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatIdentityRules.h"
#include "Combat/OGCombatLog.h"
#include "Combat/OGCombatRankHooks.h"
#include "Combat/OGCombatTriggerRuntime.h"
#include "Combat/OGCombatTypes.h"
#include "Rules/OGRulePriority.h"
#include "OGTurnBattle.generated.h"

UENUM(BlueprintType)
enum class EOGTurnBattleStatus : uint8
{
    NotStarted,
    Running,
    Completed
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGTurnSuccessionLane
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 LaneIndex = 0;

    /** Preferred order: opening unit -> second-wave -> third-wave. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGEntityId> OrderedUnitIds;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGTurnTeamState
{
    GENERATED_BODY()

    static constexpr int32 MaxRosterSize = 18;
    static constexpr int32 MaxActiveSize = 6;
    static constexpr int32 MaxLaneSize = 3;
    static constexpr int32 MaxLaneCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 TeamIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGTurnSuccessionLane> Lanes;

    int32 GetRosterSize() const
    {
        int32 Count = 0;
        for (const FOGTurnSuccessionLane& Lane : Lanes)
        {
            Count += Lane.OrderedUnitIds.Num();
        }
        return Count;
    }
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGTurnBattleState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FOGEntityId BattleId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EOGTurnBattleStatus Status = EOGTurnBattleStatus::NotStarted;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGTurnTeamState> Teams;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGCombatUnitState> Units;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int64 CurrentActionValue = 0;
};

/**
 * An effect instance already validated by the registered mechanic engine.
 * Source-provenance, authoritative potency and spatial checks are performed
 * by the engine supplying this event, NEVER by a user-controlled JSON flag.
 */
enum class EOGLuminousFieldRule : uint8
{
    BaseNocturneSeverance, BaseRekindlingEdict, BaseLastRose,
    BaseBoundaryCollision, AlterBloodReversal, AlterThornUnbinding,
    AlterMirrorDebt, AlterBoundaryDecree
};
struct FOGLuminousRegisteredEffectContest
{
    EOGLuminousFieldRule Rule = EOGLuminousFieldRule::BaseNocturneSeverance;
    FOGEntityId OwnerManifestationId;
    FOGEntityId TargetUnitId;
    FOGContentId RegisteredEffectId;
    FOGEntityId EffectSourceId;
    FOGRulePriority AttackingPriority;
    FOGRulePriority DefendingPriority;
    // These statements must originate from registered typed content/state;
    // UI JSON must never be accepted as a substitute.
    bool bRegisteredTemporaryEffect = false;
    bool bSpecificEffectFamilyEligible = false;
    bool bPerceptuallyObserved = false;
    bool bInAuthorizedFieldFootprint = false;
    bool bProjectionCopyNotOriginal = false;
    bool bPreviouslyRecordedSameContentEffect = false;
    bool bCooldownElapsed = false; // actual World ms deadline (not inferred)
    bool bBoundaryOverlapSegment = false;
};
using FOGLuminousAcceptedFieldEligibility = TFunction<
    bool(const FOGCombatUnitState&,const FOGContentId&,FString&)>;

// Registered native source of truth only. Never parse client-provided claims
// of blood anatomy, susceptibility, soul continuity or superior Authority.
using FOGLuminousNativeBloodEligibility = TFunction<bool(
    const FOGCombatUnitState& Attacker,
    const FOGCombatUnitState& Target,
    const FOGContentId& RegisteredStatusFamily)>;
using FOGLuminousNativeRevivalEligibility = TFunction<bool(
    const FOGCombatUnitState& RestoringManifestation,
    const FOGCombatUnitState& IncapacitatedTarget,
    bool bAllyCovenant)>;

using FOGLuminousRegisteredEffectRemoval = TFunction<
    bool(const FOGLuminousRegisteredEffectContest&,FString&)>;

class OFFLINEGAME_API FOGTurnBattle
{
public:
    bool Initialize(
        const FOGTurnBattleState& InitialState,
        FString& OutError);

    bool Initialize(
        const FOGTurnBattleState& InitialState,
        TArray<FOGCombatTriggerBinding> TriggerBindings,
        FOGCombatConditionEvaluator ConditionEvaluator,
        FString& OutError);

    bool Initialize(
        const FOGTurnBattleState& InitialState,
        TArray<FOGCombatTriggerBinding> TriggerBindings,
        FOGCombatConditionEvaluator ConditionEvaluator,
        FOGIdentityExclusivityContext IdentityContext,
        FOGRankSuppressionResolver RankResolver,
        FString& OutError);

    const FOGTurnBattleState& GetState() const { return State; }
    const FOGCombatLog& GetLog() const { return Log; }

    // Explicit bridge from the already-authoritative, per-Manifestation
    // ultimate-charge resolver. Never infer a free global team charge.
    bool SetResolvedLuminousUltimateCharge(
        const FOGEntityId& ManifestationId,
        int32 ChargePercent,
        FString& OutError);
    int32 GetResolvedLuminousUltimateCharge(
        const FOGEntityId& ManifestationId) const;

    void BindLuminousAuthoritativeStatusGates(
        FOGLuminousNativeBloodEligibility InBlood,
        FOGLuminousNativeRevivalEligibility InRevival,
        FOGContentId InAcceptedBleedFamilyId)
    {
        LuminousBloodEligibility=MoveTemp(InBlood);
        LuminousRevivalEligibility=MoveTemp(InRevival);
        AcceptedBleedFamilyId=MoveTemp(InAcceptedBleedFamilyId);
    }

    // Gate 00 Registry/current MR / effect and field ownership gate, provided
    // by authoritative native runtime. Absent gate means activation DENIED.
    void BindApprovedLuminousFieldEligibility(
        FOGLuminousAcceptedFieldEligibility InGate)
    {
        LuminousFieldEligibility=MoveTemp(InGate);
    }

    // Registered effect engine supplies one concrete effect instance and a
    // lawful on-success removal callback; no global immunity suppression.
    bool TryResolveLuminousRegisteredFieldContest(
        const FOGLuminousRegisteredEffectContest& Contest,
        const FOGLuminousRegisteredEffectRemoval& RemoveOneEligibleEffect,
        bool& bOutWonSpecificContest,
        FString& OutError);

    int32 SelectNextActingUnitIndex() const;

    bool ApplyResolvedAction(
        const FOGResolvedCombatAction& Action,
        FString& OutError);

    bool ApplyDamage(
        const FOGEntityId& SourceUnitId,
        const FOGEntityId& TargetUnitId,
        const FOGLargeNumber& Amount,
        const FOGContentId& SkillId,
        FString& OutError);

    /** Exactly the HP lost by the most recently resolved damage event,
     * after barriers and overkill but before any defeat-triggered revival.
     * Never infer actual HP loss by inspecting HP after an OnDefeat reaction. */
    FOGLargeNumber GetLastEffectiveHpDamage(
        const FOGEntityId& TargetId) const
    {
        return LastEffectiveHpDamage.FindRef(TargetId);
    }

    bool ApplyHealing(
        const FOGEntityId& SourceUnitId,
        const FOGEntityId& TargetUnitId,
        const FOGLargeNumber& Amount,
        const FOGContentId& SkillId,
        FString& OutError);

    bool ChangePresence(
        const FOGEntityId& UnitId,
        EOGCombatPresence NewPresence,
        FString& OutError);

    /**
     * Finalizes a completed action after all OnDefeat/revival actions have been
     * resolved. Direct successors are preferred. Any still-empty battlefield
     * lane is then filled from other surviving reserves when possible.
     */
    bool ResolvePendingReplacements(FString& OutError);

    /** Triggered actions are resolved by the normal effect/action layer. */
    TArray<FOGQueuedTriggeredAction> DrainTriggeredActions();

    bool ResolveRankSuppressionMultiplier(
        const FOGEntityId& SourceUnitId,
        const FOGEntityId& TargetUnitId,
        FName ChannelId,
        int32& OutMultiplierBps,
        FString& OutError) const;

    /**
     * Call after a queued triggered action has fully applied its effects.
     * The last completed defeat-trigger action automatically closes the revival
     * window and performs succession/rebalancing.
     */
    bool CompleteTriggeredAction(
        int64 QueueSequence,
        FString& OutError);

private:
    // Atomic action transaction: the currently committed Battle must never
    // retain partial HP, resource, trigger or log changes from a rejected
    // authored multi-hit or revival action.
    bool ApplyResolvedActionMutating(
        const FOGResolvedCombatAction& Action,
        FString& OutError);

    struct FLuminousFieldState
    {
        FOGContentId OwnerFantasmId;
        int32 ActivatedOwnerAction = 0;
        int32 ExpiresBeforeOwnerAction = INDEX_NONE;
        int32 BaseNocturneSuccessfulTargetCount = 0;
        TSet<FOGEntityId> NocturneConsumedTargets;
        TSet<FOGEntityId> RekindledAllies;
        bool bLastRoseUsed = false;
        int32 AlterBloodReversalSuccessCount = 0;
        TSet<FOGEntityId> AlterThornConsumedTargets;
        bool bAlterMirrorDebtUsed = false;
        TSet<uint8> AlterNegationRefundedCategories;
    };
    TMap<FOGEntityId,FLuminousFieldState> LuminousFields;
    FOGLuminousAcceptedFieldEligibility LuminousFieldEligibility;
    FOGLuminousNativeBloodEligibility LuminousBloodEligibility;
    FOGLuminousNativeRevivalEligibility LuminousRevivalEligibility;
    FOGContentId AcceptedBleedFamilyId;
    bool IsLuminousBloodStatusEligible(
        const FOGEntityId& AttackerId,
        const FOGEntityId& TargetId,
        const FOGContentId& StatusFamily) const
    {
        // Bleed family ID must be provided from installed Registry; a
        // hand-made draft token is not permission to create blood statuses.
        if(!LuminousBloodEligibility || !AcceptedBleedFamilyId.IsValid() ||
           StatusFamily!=AcceptedBleedFamilyId)return false;
        const FOGCombatUnitState* Attacker=FindUnit(AttackerId);
        const FOGCombatUnitState* Target=FindUnit(TargetId);
        return Attacker && Target &&
            LuminousBloodEligibility(*Attacker,*Target,StatusFamily);
    }
    bool IsLuminousRevivalEligible(
        const FOGEntityId& RestoringId,
        const FOGEntityId& TargetId,
        bool bAlly) const
    {
        if(!LuminousRevivalEligibility)return false;
        const FOGCombatUnitState* Restoring=FindUnit(RestoringId);
        const FOGCombatUnitState* Target=FindUnit(TargetId);
        return Restoring && Target &&
            LuminousRevivalEligibility(*Restoring,*Target,bAlly);
    }
    bool TryActivateLuminousAuthoredField(
        const FOGCombatUnitState& Unit,
        const FOGContentId& ExactFantasmId,
        FString& OutError);

    struct FLuminousBaseBattleRuntime
    {
        int32 ScarletReserve = 35;
        int32 UltimateChargePercent = 0;
        bool bRevivalUsed = false;
        bool bDayAttunement = false;
        bool bAttunementActive = true; // Initial Night from approved Base v6.
        int32 AttunementExpiryAction = 2;
        int32 NextA2ReadyOwnerAction = 0;
        int32 NextA3ReadyOwnerAction = 0;
        int32 DiscountNextA2Reserve = 0;
        bool bNextScarletCadenceDelayReduction = false;
        int32 LastBleedReserveOwnerAction = INDEX_NONE;
        TSet<FOGEntityId> PendingFourMarkTargetBonus;
        TSet<FOGEntityId> EclipseClaimTargets;
        TMap<FOGEntityId,int32> EclipseClaimExpiryOwnerAction;
        bool bAllyRevivalSpent = false;
        FOGEntityId PreparedCovenantTarget;
        int32 CovenantExpiryOwnerAction = -1;
        int32 OwnerActions = 0;
        TMap<FOGEntityId,int32> HemalMarks;
        TMap<FOGEntityId,int32> HemalMarkExpiryOwnerAction;
        TMap<FOGEntityId,int32> HemorrhageStacks;
        TMap<FOGEntityId,int32> HemorrhageEndOwnerAction;
        TMap<FOGEntityId,int32> BlessingExpiry;
        // 1=A3 Benediction/Covenant; 2=Day Ultimate; 2 owner turns.
        TMap<FOGEntityId,uint8> BlessingCategory;
        int32 DawnOathExpiry = 0;
        TMap<FOGEntityId,int32> DawnOathByBeneficiary;
        int32 NextFantasmReadyOwnerAction = 0;
    };

    // Per-Manifestation, per-battle state. Never a shared identity/global meter.
    TMap<FOGEntityId,FLuminousBaseBattleRuntime> LuminousBaseStates;

    struct FLuminousDawnBarrier
    {
        FOGEntityId GrantingManifestation;
        FOGEntityId Beneficiary;
        FOGLargeNumber Remaining;
        int32 ExpiresAfterOwnerAction = INDEX_NONE;
    };
    TArray<FLuminousDawnBarrier> LuminousDawnBarriers;
    TMap<FOGEntityId,FOGLargeNumber> LastEffectiveHpDamage;
    void GetApprovedDawnBlessingBps(
        const FOGEntityId& BeneficiaryId,
        int32& OutAttackBonusBps,
        int32& OutSpeedBonusBps,
        int32& OutHealingReceivedBonusBps,
        int32& OutDamageReductionBps) const;

    void GrantAuthoredDawnOverhealBarrier(
        const FOGEntityId& OwnerId,
        const FOGEntityId& BeneficiaryId,
        const FOGLargeNumber& ExcessHealing);

    struct FLuminousAlterBattleRuntime
    {
        int32 RedTithe = 20;
        int32 UltimateChargePercent = 0;
        int32 OwnerActions = 0;
        int32 MartyrMomentum = 0;
        int32 MomentumExpiresOwnerAction = -1;
        int32 MartyrStanceExpires = -1;
        bool bMartyrNextFinisherPending = false;
        int32 DeathPriceExpiresOwnerAction = -1;
        int32 EucharistNextAvailableAction = 0;
        int32 CommunionNextAvailableAction = 0;
        bool bSelfRevived = false;
        TMap<FOGEntityId,int32> RuptureSeals;
        TMap<FOGEntityId,int32> SealExpiryOwnerAction;
        TMap<FOGEntityId,int32> SeveringWounds;
        TMap<FOGEntityId,int32> WoundExpiryOwnerAction;
    };
    TMap<FOGEntityId,FLuminousAlterBattleRuntime> LuminousAlterStates;
    bool ExecuteApprovedLuminousAlterAction(
        const FOGResolvedCombatAction& Action,
        FOGCombatUnitState& Source,
        FString& OutError);
    void TryResolveApprovedLuminousAlterRevival(
        const FOGEntityId& UnitId);

    // Dry legal-action check before writing any timeline, battle log or HP.
    bool PrevalidateLuminousActionWithoutSideEffects(
        const FOGResolvedCombatAction& Action,
        const FOGCombatUnitState& Source,
        FString& OutError) const;

    bool ExecuteApprovedLuminousBaseAction(
        const FOGResolvedCombatAction& Action,
        FOGCombatUnitState& Source,
        FString& OutError);
    void TryResolveApprovedLuminousBaseRevival(
        const FOGEntityId& UnitId);
    void TryResolveApprovedLuminousAllyCovenant(
        const FOGEntityId& TargetUnitId);

    FOGCombatUnitState* FindMutableUnit(const FOGEntityId& UnitId);
    const FOGCombatUnitState* FindUnit(const FOGEntityId& UnitId) const;
    const FOGTurnTeamState* FindTeam(int32 TeamIndex) const;
    const FOGTurnSuccessionLane* FindOriginLane(
        int32 TeamIndex,
        const FOGEntityId& UnitId,
        int32& OutUnitDepth) const;

    bool NormalizeOpeningLanes(FString& OutError);
    FOGCombatUnitState* FindPreferredDirectSuccessor(
        const FOGCombatUnitState& Defeated);
    FOGCombatUnitState* FindFallbackReserve(
        int32 TeamIndex);
    bool IsBattlefieldLaneOccupied(
        int32 TeamIndex,
        int32 BattlefieldLane) const;
    void PromoteIntoLane(
        FOGCombatUnitState& Unit,
        int32 BattlefieldLane,
        const TCHAR* Reason);

    void QueueTriggerEvent(
        FName EventType,
        const FOGEntityId& SourceUnitId,
        const FOGEntityId& TargetUnitId,
        const FOGContentId& SkillId,
        int64 SourceSequence);

    void FlushTriggerRuntimeQueueToBattleQueue();
    void EvaluateBattleCompletion();
    bool ValidateInitialState(FString& OutError) const;

    FOGTurnBattleState State;
    FOGCombatLog Log;
    FOGCombatTriggerRuntime TriggerRuntime;
    FOGIdentityExclusivityContext IdentityContext;
    FOGRankSuppressionResolver RankSuppressionResolver;
    TArray<FOGQueuedTriggeredAction> PendingTriggeredActions;
    TSet<int64> OutstandingTriggeredActionSequences;
    TArray<FOGEntityId> PendingDefeatedUnitIds;
};
