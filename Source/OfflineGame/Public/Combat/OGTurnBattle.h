#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatIdentityRules.h"
#include "Combat/OGCombatLog.h"
#include "Combat/OGCombatRankHooks.h"
#include "Combat/OGCombatTriggerRuntime.h"
#include "Combat/OGCombatTypes.h"
#include "Combat/OGNativeCombatAbility.h"
#include "Combat/OGNativeCombatPassive.h"
#include "Combat/OGWorldAbilityDefinition.h"
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

    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGCombatExecutionMode ExecutionMode=EOGCombatExecutionMode::Turn;
    /** Local gameplay milliseconds; menu/offline calendar time cannot silently advance an active cast. */
    UPROPERTY(BlueprintReadOnly) int64 WorldElapsedMilliseconds=0;

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
    // Exact runtime instance; a content definition alone is not a removable effect.
    FOGEntityId RegisteredEffectInstanceId;
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
// Read-only native effect/provenance/spatial/priority proof, not caller flags.
using FOGLuminousRegisteredContestEvidence = TFunction<
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

    bool Initialize(const FOGTurnBattleState& InitialState,TArray<FOGCombatTriggerBinding> TriggerBindings,
        FOGCombatConditionEvaluator Conditions,FOGIdentityExclusivityContext Identity,
        FOGRankSuppressionResolver Rank,TArray<FOGNativePassiveBinding> Passives,FString& Error);
    /** Complete opening reactions after native ability/resource admission. Atomic on failure. */
    bool ResolveNativeOpeningReactions(FString& Error);
    const TArray<FOGNativeReactionRecord>& GetLastNativeReactions() const { return LastNativeReactions; }
    bool CanAdmitNativeAbility(const FOGCombatUnitState& Source,const FOGContentId& Skill) const;
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

    void BindLuminousRegisteredContestEvidence(
        FOGLuminousRegisteredContestEvidence InValidator)
    {
        LuminousRegisteredContestEvidence=MoveTemp(InValidator);
    }

    // Registered effect engine supplies one concrete effect instance and a
    // lawful on-success removal callback; no global immunity suppression.
    bool TryResolveLuminousRegisteredFieldContest(
        const FOGLuminousRegisteredEffectContest& Contest,
        const FOGLuminousRegisteredEffectRemoval& RemoveOneEligibleEffect,
        bool& bOutWonSpecificContest,
        FString& OutError);

    int32 SelectNextActingUnitIndex() const;

    /** Expands authored AoE abilities to every eligible living active field
     * ally/enemy. Caller-selected primary keeps any primary-specific bonuses.
     * Applied AGAIN inside the authoritative executor: UI cannot omit units. */
    bool ExpandApprovedFieldTargets(const FOGResolvedCombatAction& Original,
        FOGResolvedCombatAction& Expanded, bool& bOutArea,
        FString& OutError) const;

    /** Read-only command readiness: same expansion, actor/timeline and approved preflight used on submission. */
    bool ValidateResolvedAction(const FOGResolvedCombatAction& Action,
        FOGResolvedCombatAction& Expanded, FString& OutError) const;
    bool ReadLuminousCommandResources(const FOGEntityId& UnitId,
        int32& Resource, int32& Charge, int32& OwnerActions) const;

    /** Production sessions require an actual executor; isolated timeline/replay adapters opt in explicitly. */
    void RequireNativeActionExecutors(bool Required) { bRequireNativeExecutors=Required; }
    bool ConfigureNativeResource(const FOGEntityId& UnitId,const FOGCombatResourceSpecification& Specification,FString& Error);
    bool RegisterNativeAbility(const FOGNativeAbilityDefinition& Definition,FString& Error);
    bool HasNativeActionExecutor(const FOGCombatUnitState& Source,const FOGContentId& Skill,FString& Error) const;
    FName ResolveNativeAbilityMotion(const FOGCombatUnitState& Source,const FOGContentId& Skill) const;
    bool RegisterWorldAbility(const FOGWorldAbilitySpecification& Specification,FString& Error);
    const FOGWorldAbilitySpecification* FindWorldAbility(const FOGCombatUnitState& Unit,const FOGContentId& Skill) const;
    bool IsWorldDamagingStep(const FOGCombatUnitState& Unit,const FOGContentId& Skill,int32 Step) const;
    bool FilterWorldContacts(const FOGResolvedCombatAction& Root,const TArray<FOGEntityId>& Raw,TArray<FOGEntityId>& Out,FString& Error) const;
    bool SetExecutionMode(EOGCombatExecutionMode Mode,FString& Error);
    bool BeginWorldAbility(const FOGResolvedCombatAction& Root,FString& Error);
    bool ApplyWorldAbilityStep(const FOGEntityId& CastId,int32 StepIndex,const TArray<FOGEntityId>& Contacts,FString& Error);
    bool CancelWorldAbility(const FOGEntityId& CastId,FString& Error);
    bool AdvanceWorldCombatClock(int64 TargetMilliseconds,FString& Error);
    bool ReadWorldCast(const FOGEntityId& CastId,FOGWorldAbilityCastView& Out) const;
    FOGEntityId GetUnitWorldCast(const FOGEntityId& UnitId) const;
    int64 GetWorldAbilityReadyAt(const FOGEntityId& UnitId,const FOGContentId& Skill) const;
    void GetWorldCasts(TArray<FOGWorldAbilityCastView>& Out) const
    {
        Out.Reset(); for (const auto& Pair:WorldCasts) { FOGWorldAbilityCastView View; if (ReadWorldCast(Pair.Key,View)) Out.Add(View); }
        Out.Sort([](const auto& A,const auto& B){return A.CastId.ToString()<B.CastId.ToString();});
    }
    bool HasPendingWorldCasts() const { return !WorldCasts.IsEmpty(); }
    bool RegisterNativeEffect(const FOGNativeEffectDefinition& Definition,FString& Error);
    FOGCombatStats GetNativeEffectiveStats(const FOGEntityId& UnitId) const;
    bool HasNativeEffect(const FOGEntityId& UnitId,const FOGContentId& Id,bool Family=false) const;
    FString DescribeNativeEffects(const FOGEntityId& UnitId) const;
    int64 GetNativeAbilityRevision() const { return NativeAbilityRevision; }

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
    bool SetExecutionModeMutating(EOGCombatExecutionMode Mode,FString& Error);
    struct FNativePassiveRule { FOGEntityId OwnerUnitId; FOGContentId PassiveSkillId; FOGNativePassiveTrigger Rule; };
    TMap<FString,FNativePassiveRule> NativePassiveRules;
    struct FNativeOwnerActivation { int64 Epoch=-1; int32 Count=0; };
    TMap<FOGEntityId,int64> NativeOwnerActionEpochs;
    TMap<FString,FNativeOwnerActivation> NativeOwnerActivationCounts;
    bool BeginNativeOwnerAction(const FOGEntityId& Unit,FString& Error);
    TSet<FString> NativePassiveActionOwners;
    FOGCombatConditionEvaluator NativeFallbackConditions;
    TOptional<FOGQueuedTriggeredAction> NativeReactionProof;
    TArray<FOGNativeReactionRecord> LastNativeReactions;
    bool InitializeNativePassives(const TArray<FOGNativePassiveBinding>& Passives,
        TArray<FOGCombatTriggerBinding>& Bindings,FString& Error);
    bool EvaluateNativePassiveCondition(const FOGCombatTriggerBinding& Binding,
        const FOGConditionDefinition& Condition,const FOGCombatTriggerContext& Context) const;
    bool IsNativeAbilityAuthorized(const FOGCombatUnitState& Source,const FOGContentId& Skill) const;
    bool HasNativeReactionProof(const FOGCombatUnitState& Source,const FOGContentId& Skill) const;
    bool AllowsNativeReactionSource(const FOGCombatUnitState& Source,const FOGResolvedCombatAction& Action) const;
    bool ResolveNativeReactions(FString& Error,TMap<FString,int32>* RootActivations=nullptr);
    bool SelectNativeReactionTarget(const FOGQueuedTriggeredAction& Queued,const FNativePassiveRule& Rule,
        FOGResolvedCombatAction& Action,FString& Error) const;
    struct FWorldCast
    {
        FOGResolvedCombatAction Root;
        FOGWorldAbilitySpecification Specification;
        int64 StartedAt=0;
        uint64 Seed=0;
        TSet<int32> CompletedSteps;
        TMap<FString,int32> Activations;
        bool bCostsPaid=false;
        bool bHadContact=false;
        TSet<FOGEntityId> BeforeEffectTargets;
        TArray<FOGEntityId> ContactTargets;
    };
    TMap<FOGContentId,FOGWorldAbilitySpecification> WorldAbilities;
    TMap<FOGEntityId,FWorldCast> WorldCasts;
    TMap<FString,int64> WorldAbilityReadyAt;
    TOptional<FOGResolvedCombatAction> WorldExecutionProof;
    bool SpendNativeAbilityCosts(const FOGResolvedCombatAction&,FOGCombatUnitState&,FString&);
    bool GrantNativeAbilityResources(const FOGResolvedCombatAction&,FOGCombatUnitState&,FString&);
    bool ExecuteNativePacketSlices(const FOGResolvedCombatAction&,FOGCombatUnitState&,const TArray<FOGNativeAbilityPacketSlice>&,FOGDeterministicRng&,FString&);
    bool FinishWorldAbility(const FOGEntityId& CastId,FString& Error);
    bool CancelWorldAbilityMutating(const FOGEntityId& CastId,FString& Error);
    bool CompleteWorldOwnerAction(const FOGResolvedCombatAction& Root,FString& Error,TMap<FString,int32>* RootActivations=nullptr);
    TMap<FOGContentId,FOGNativeEffectDefinition> NativeEffectDefinitions;
    bool ExecuteNativeEffects(const FOGResolvedCombatAction& Action,bool BeforePackets,FOGDeterministicRng& Rng,FString& Error,uint8 Scope=0);
    bool ApplyNativeEffect(const FOGNativeEffectDefinition& Definition,const FOGNativeAbilityEffectOperation& Operation,
        const FOGResolvedCombatAction& Action,const FOGEntityId& TargetId,int32 OperationIndex,FString& Error);
    bool ProcessNativeEffectOwnerAction(const FOGEntityId& Owner,FString& Error);
    int64 GetNextNativeWorldEffectTime() const;
    bool ProcessNativeWorldEffectClock(int64 Target,FString& Error);
    bool ValidateNativeWorldEffectTiming(const FOGNativeEffectDefinition& Definition,FString& Error) const;
    void RemoveNativeEffectsOnDefeat(const FOGEntityId& Target);
    void ClearNativeEncounterEffects();
    void RecordNativeEffect(EOGCombatLogEventType Type,const FOGEntityId& Source,const FOGEntityId& Target,
        const FOGContentId& Skill,const FOGNativeEffectInstance& Instance,const TCHAR* Reason);
    FOGLargeNumber AbsorbNativeBarriers(const FOGEntityId& Target,FOGLargeNumber& Amount);
    int64 NativeEffectActionDelay(const FOGCombatUnitState& Source,int64 Delay) const;
    bool bRequireNativeExecutors=false;
    int64 NativeAbilityRevision=0;
    TMap<FString,FOGNativeAbilityDefinition> NativeAbilities;
    TMap<FString,FOGCombatResourceSpecification> NativeResourceSpecifications;
    TMap<FOGEntityId,int64> NativeOwnerActions;
    TMap<FString,int64> NativeAbilityReadyActions;
    const FOGNativeAbilityDefinition* FindNativeAbility(const FOGCombatUnitState& Source,const FOGContentId& Skill) const;
    bool ValidateNativeAbility(const FOGResolvedCombatAction& Action,const FOGCombatUnitState& Source,FString& Error) const;
    bool ExecuteNativeAbility(const FOGResolvedCombatAction& Action,FOGCombatUnitState& Source,FString& Error);
    bool ExpandNativeAbilityTargets(const FOGResolvedCombatAction& Original,FOGResolvedCombatAction& Expanded,
        bool& Handled,bool& Area,FString& Error) const;
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
    FOGLuminousRegisteredContestEvidence LuminousRegisteredContestEvidence;
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
        int32 RevivalAfterglowExpiresOwnerAction = INDEX_NONE;
        int32 MercifulEdgeExpiresOwnerAction = INDEX_NONE;
        int32 LastNightExecutionChargeOwnerAction = INDEX_NONE;
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
        // Approved Day Ultimate 35 Reserve/30% ally self-continuity rescue:
        // a prepared reaction, NOT free post-succession active resurrection.
        FOGEntityId PreparedDayUltimateRevivalTarget;
        int32 DayUltimateRevivalExpiryOwnerAction = -1;
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
        // P6 Twilight Lawkeeper: source ContentIds from actually resolved
        // enemy actions targeted at this living Manifestation. Observing
        // is NOT acquiring permission to copy or automatically negate.
        TArray<FOGContentId> ObservedEnemyContentIds;
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
        // Eclipse Harrier P4 Pursuit: once per owner turn after a genuinely
        // defeated, source-owned bleeding target. Applies to next legal
        // native action-value scheduling, never generic death events.
        int32 LastPursuitOwnerAction = -1;
        bool bPursuitDelayPending = false;
        // P6 Witness Ledger: last three verified enemy effect ContentIds.
        TArray<FOGContentId> ObservedEnemyContentIds;
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

    // Normal healing cannot revive defeated units; only already-authorized
    // Base or Alter resurrection handlers enter this narrow restoration scope.
    bool bApprovedRevivalHealInProgress = false;
    bool ApplyApprovedLuminousRevivalHealing(
        const FOGEntityId& SourceId,
        const FOGEntityId& DefeatedTargetId,
        const FOGLargeNumber& Amount,
        const FOGContentId& ResurrectionSkill,
        FString& OutError);

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
