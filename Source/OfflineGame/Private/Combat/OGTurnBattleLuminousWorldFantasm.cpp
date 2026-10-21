// Gate12 source-derived World Negation fields. Registered-effect contests only;
// this does not invent a global immunity, auto-win MR, new ContentIds, NPCs,
// resurrection permission, or a persistent environmental World.
#include "Combat/OGTurnBattle.h"
#include "Combat/OGApprovedLuminousSourceKit.h"
#include "Combat/OGLuminousAlterV1Projection.h"
#include "Rules/OGRulePriority.h"

namespace
{
bool IsBaseFieldRule(EOGLuminousFieldRule R)
{
    return R==EOGLuminousFieldRule::BaseNocturneSeverance ||
           R==EOGLuminousFieldRule::BaseRekindlingEdict ||
           R==EOGLuminousFieldRule::BaseLastRose ||
           R==EOGLuminousFieldRule::BaseBoundaryCollision;
}
bool IsBoundaryRule(EOGLuminousFieldRule R)
{
    return R==EOGLuminousFieldRule::BaseBoundaryCollision ||
           R==EOGLuminousFieldRule::AlterBoundaryDecree;
}
}
bool FOGTurnBattle::TryActivateLuminousAuthoredField(
    const FOGCombatUnitState& Unit,
    const FOGContentId& ExactFantasmId,
    FString& OutError)
{
    OutError.Reset();
    if(State.Status!=EOGTurnBattleStatus::Running || !Unit.CanAct() ||
       !ExactFantasmId.IsValid() || !LuminousFieldEligibility)
    {
        OutError=TEXT("Fantasm is unavailable without live owner, legal action and accepted native MR/field eligibility.");
        return false;
    }
    if(!LuminousFieldEligibility(Unit,ExactFantasmId,OutError))
    {
        if(OutError.IsEmpty())
            OutError=TEXT("Canonical Registry/current MR/World authority rejected this Fantasm activation.");
        return false;
    }
    const bool bBase=ExactFantasmId ==
       FOGContentId(TEXT("combat:luminous_valentine.base.fantasm.sovereign_eclipse"));
    const bool bAlter=ExactFantasmId ==
       FOGContentId(TEXT("draft:luminous_valentine.alter.world_fantasm.cathedral"));
    if(!bBase && !bAlter)
    {
        OutError=TEXT("Unregistered Luminous Fantasm is not executable.");
        return false;
    }

    const FLuminousFieldState* OldField=LuminousFields.Find(Unit.UnitEntityId);
    int32 ActionNumber=0;
    if(bBase)
    {
        FLuminousBaseBattleRuntime& Owner=LuminousBaseStates.FindOrAdd(
            Unit.UnitEntityId);
        ActionNumber=Owner.OwnerActions+1;
        if(Owner.ScarletReserve<60 ||
           ActionNumber<Owner.NextFantasmReadyOwnerAction)
        {
            OutError=TEXT("Base Fantasm requires 60 Reserve and six-owner-action cooldown.");
            return false;
        }
        bool bPrepared=false;
        // A mark that already expired by the activation action does not
        // satisfy the source's actual preparation requirement.
        for(const auto& Row:Owner.HemalMarks)
            if(Row.Value>0 &&
               Owner.HemalMarkExpiryOwnerAction.FindRef(Row.Key)>=ActionNumber)
            {bPrepared=true;break;}
        if(!bPrepared)
            for(const auto& Row:Owner.DawnOathByBeneficiary)
                if(Row.Value>=ActionNumber){bPrepared=true;break;}
        if(!bPrepared)
        {
            OutError=TEXT("Base Fantasm needs a source-owned Hemal Mark or active Dawn Oath.");
            return false;
        }
        if(OldField && OldField->ExpiresBeforeOwnerAction>=ActionNumber)
        {
            OutError=TEXT("Cannot overlay the same Manifestation's own active Fantasm.");
            return false;
        }
        Owner.ScarletReserve-=60;
        Owner.NextFantasmReadyOwnerAction=ActionNumber+7;
        Owner.OwnerActions=ActionNumber;
    }
    else
    {
        FLuminousAlterBattleRuntime& Owner=LuminousAlterStates.FindOrAdd(
            Unit.UnitEntityId);
        ActionNumber=Owner.OwnerActions+1;
        if(Owner.RedTithe<60)
        {
            OutError=TEXT("Alter Cathedral requires 60 Red Tithe.");
            return false;
        }
        if(OldField && OldField->ExpiresBeforeOwnerAction>=ActionNumber)
        {
            OutError=TEXT("Cannot overlay this Manifestation's active Cathedral.");
            return false;
        }
        Owner.RedTithe-=60;
        Owner.OwnerActions=ActionNumber;
    }
    FLuminousFieldState Field;
    Field.OwnerFantasmId=ExactFantasmId;
    Field.ActivatedOwnerAction=ActionNumber;
    Field.ExpiresBeforeOwnerAction=ActionNumber+3;
    LuminousFields.Add(Unit.UnitEntityId,MoveTemp(Field));
    FOGCombatLogEvent Entry;
    Entry.Type=EOGCombatLogEventType::ActionResolved;
    Entry.SourceUnitId=Unit.UnitEntityId;
    Entry.SkillId=ExactFantasmId;
    Entry.PayloadJson=FString::Printf(
        TEXT("{\"fantasm\":\"%s\",\"duration_owner_turns\":3,\"activation_owner_action\":%d}"),
        *ExactFantasmId.ToString(),ActionNumber);
    Log.Append(MoveTemp(Entry));
    return true;
}

bool FOGTurnBattle::TryResolveLuminousRegisteredFieldContest(
    const FOGLuminousRegisteredEffectContest& Contest,
    const FOGLuminousRegisteredEffectRemoval& RemoveOneEligibleEffect,
    bool& bOutWonSpecificContest,
    FString& OutError)
{
    bOutWonSpecificContest=false;
    OutError.Reset();
    const FOGCombatUnitState* Owner=FindUnit(Contest.OwnerManifestationId);
    const FOGCombatUnitState* Target=FindUnit(Contest.TargetUnitId);
    FLuminousFieldState* Field=LuminousFields.Find(Contest.OwnerManifestationId);
    const bool bBase=IsBaseFieldRule(Contest.Rule);
    const bool bBoundary=IsBoundaryRule(Contest.Rule);
    if(State.Status!=EOGTurnBattleStatus::Running || !Owner || !Owner->IsAlive() ||
       (!bBoundary && (!Target || !Target->IsAlive())) || !Field ||
       !Contest.bInAuthorizedFieldFootprint ||
       !Contest.bPerceptuallyObserved ||
       !LuminousRegisteredContestEvidence ||
       (!bBoundary && !Contest.RegisteredEffectInstanceId.IsValid()) ||
       !Contest.AttackingPriority.SourceRuleId.IsValid() ||
       !Contest.DefendingPriority.SourceRuleId.IsValid() ||
       !RemoveOneEligibleEffect ||
       (!bBoundary && !Contest.RegisteredEffectId.IsValid()))
    {
        OutError=TEXT("Fantasm contest lacks a live field, observed typed content, lawful range or actual effect resolver.");
        return false;
    }
    if(bBase != (Field->OwnerFantasmId ==
       FOGContentId(TEXT("combat:luminous_valentine.base.fantasm.sovereign_eclipse"))))
    {
        OutError=TEXT("Luminous Base and Alter Fantasm rules cannot be mixed.");
        return false;
    }
    const int32 CurrentAction=bBase
        ? LuminousBaseStates.FindRef(Contest.OwnerManifestationId).OwnerActions
        : LuminousAlterStates.FindRef(Contest.OwnerManifestationId).OwnerActions;
    if(CurrentAction>Field->ExpiresBeforeOwnerAction ||
       CurrentAction<Field->ActivatedOwnerAction)
    {
        OutError=TEXT("Fantasm expired; never revive it from a later mode.");
        return false;
    }
    if(!bBoundary &&
       (!Contest.bRegisteredTemporaryEffect ||
        !Contest.bSpecificEffectFamilyEligible))
    {
        OutError=TEXT("Only actual registered and compatible temporary effect instances can be negated.");
        return false;
    }
    if(bBoundary && !Contest.bBoundaryOverlapSegment)
    {
        OutError=TEXT("Boundary overlap must be an observed exact spatial segment.");
        return false;
    }

    // Native registered-effect source must re-derive provenance, instance,
    // spatial footprint, observation, rule family and both priority values.
    // Caller-supplied booleans and priorities are not authority by themselves.
    if(!LuminousRegisteredContestEvidence(Contest,OutError))
    {
        if(OutError.IsEmpty())
            OutError=TEXT("No live registered instance/World priority proof for this contest.");
        return false;
    }

    int32 Cost=0;
    bool bChargeOnFailedContest=false;
    FLuminousBaseBattleRuntime* BaseState=
        bBase?LuminousBaseStates.Find(Contest.OwnerManifestationId):nullptr;
    FLuminousAlterBattleRuntime* AlterState=
        bBase?nullptr:LuminousAlterStates.Find(Contest.OwnerManifestationId);
    if(bBase && !BaseState || !bBase && !AlterState)
    {
        OutError=TEXT("Field owner has no corresponding Manifestation-specific resource state.");
        return false;
    }

    switch(Contest.Rule)
    {
    case EOGLuminousFieldRule::BaseNocturneSeverance:
        if(!BaseState->bAttunementActive ||
           BaseState->bDayAttunement ||
           Owner->TeamIndex==Target->TeamIndex ||
           BaseState->HemalMarks.FindRef(Contest.TargetUnitId)<3 ||
           BaseState->HemalMarkExpiryOwnerAction.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->NocturneConsumedTargets.Contains(Contest.TargetUnitId))
        {
            OutError=TEXT("Nocturne requires three source marks and one eligible temporary enemy effect.");
            return false;
        }
        break;
    case EOGLuminousFieldRule::BaseRekindlingEdict:
        if(!BaseState->bAttunementActive ||
           !BaseState->bDayAttunement ||
           Owner->TeamIndex!=Target->TeamIndex ||
           !BaseState->DawnOathByBeneficiary.Contains(Contest.TargetUnitId) ||
           BaseState->DawnOathByBeneficiary.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->RekindledAllies.Contains(Contest.TargetUnitId) ||
           FOGLargeNumber::Compare(
               Target->CurrentHp,
               FOGLargeNumber::ScaleByBasisPoints(Target->Stats.MaxHp,3500))>0)
        {
            OutError=TEXT("Rekindling requires a wounded allied Oath bearer and actual registered heal-block.");
            return false;
        }
        break;
    case EOGLuminousFieldRule::BaseLastRose:
        Cost=20;
        if((Target && Owner->TeamIndex==Target->TeamIndex) ||
           Field->bLastRoseUsed || !Contest.bProjectionCopyNotOriginal ||
           !Contest.bPreviouslyRecordedSameContentEffect ||
           !BaseState->ObservedEnemyContentIds.Contains(
               Contest.RegisteredEffectId))
        {
            OutError=TEXT("Last Rose applies once to a recorded projection, never an original spell.");
            return false;
        }
        break;
    case EOGLuminousFieldRule::BaseBoundaryCollision:
        // Only a physically overlapping segment, not entire rival field.
        break;
    case EOGLuminousFieldRule::AlterBloodReversal:
        Cost=15;bChargeOnFailedContest=true;
        if(Owner->TeamIndex==Target->TeamIndex ||
           AlterState->SeveringWounds.FindRef(Contest.TargetUnitId)<2 ||
           AlterState->WoundExpiryOwnerAction.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->AlterBloodReversalSuccessCount>=2 ||
           !Contest.bCooldownElapsed)
        {
            OutError=TEXT("Blood Reversal requires two owned wound stacks, eligible heal and actual 5-second cooldown.");
            return false;
        }
        break;
    case EOGLuminousFieldRule::AlterThornUnbinding:
        Cost=20;bChargeOnFailedContest=true;
        if(Owner->TeamIndex==Target->TeamIndex ||
           AlterState->RuptureSeals.FindRef(Contest.TargetUnitId)<3 ||
           AlterState->SealExpiryOwnerAction.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->AlterThornConsumedTargets.Contains(Contest.TargetUnitId))
        {
            OutError=TEXT("Thorn Unbinding requires three owned seals and one eligible tether.");
            return false;
        }
        break;
    case EOGLuminousFieldRule::AlterMirrorDebt:
        Cost=20;bChargeOnFailedContest=true;
        if(Field->bAlterMirrorDebtUsed || !Contest.bProjectionCopyNotOriginal ||
           Contest.TargetUnitId!=Contest.OwnerManifestationId ||
           !AlterState->ObservedEnemyContentIds.Contains(
               Contest.RegisteredEffectId))
        {
            OutError=TEXT("Mirror Debt only contests a reflected copy directed at Alter, never the original.");
            return false;
        }
        break;
    case EOGLuminousFieldRule::AlterBoundaryDecree:
        break;
    default:
        OutError=TEXT("Unrecognized World Negation rule.");
        return false;
    }
    if((bBase && BaseState->ScarletReserve<Cost) ||
       (!bBase && AlterState->RedTithe<Cost))
    {
        OutError=TEXT("Cannot contest with insufficient personal field resource.");
        return false;
    }
    const bool bPriorityWins=FOGRulePriority::Compare(
        Contest.AttackingPriority,Contest.DefendingPriority)>0;
    // The actual registered mechanic owns mutation. No generic, unconditional
    // removal, and a failed concrete mutation NEVER consumes a mark or
    // commits a successful contest. World boundary implementation supplies
    // a callback that changes only the winning overlapping spatial segment.
    if(bPriorityWins)
    {
        if(!RemoveOneEligibleEffect(Contest,OutError))
        {
            if(OutError.IsEmpty())
                OutError=TEXT("Winning Authority contest could not remove the specific effect instance.");
            return false;
        }
        bOutWonSpecificContest=true;
    }
    if(Cost>0 && (bOutWonSpecificContest || bChargeOnFailedContest))
    {
        if(bBase)BaseState->ScarletReserve-=Cost;
        else AlterState->RedTithe-=Cost;
    }
    if(!bOutWonSpecificContest)return true;

    switch(Contest.Rule)
    {
    case EOGLuminousFieldRule::BaseNocturneSeverance:
        // Deplete source-owned eligible marks only AFTER the callback has
        // actually removed the one authority-approved registered effect.
        BaseState->HemalMarks.FindOrAdd(Contest.TargetUnitId)-=3;
        if(BaseState->HemalMarks.FindRef(Contest.TargetUnitId)<=0)
        {
            BaseState->HemalMarks.Remove(Contest.TargetUnitId);
            BaseState->HemalMarkExpiryOwnerAction.Remove(
                Contest.TargetUnitId);
            BaseState->PendingFourMarkTargetBonus.Remove(
                Contest.TargetUnitId);
        }
        Field->NocturneConsumedTargets.Add(Contest.TargetUnitId);
        break;
    case EOGLuminousFieldRule::BaseRekindlingEdict:
        BaseState->DawnOathByBeneficiary.Remove(Contest.TargetUnitId);
        Field->RekindledAllies.Add(Contest.TargetUnitId);
        break;
    case EOGLuminousFieldRule::BaseLastRose:
        Field->bLastRoseUsed=true;
        break;
    case EOGLuminousFieldRule::AlterBloodReversal:
        ++Field->AlterBloodReversalSuccessCount;
        AlterState->RedTithe=FMath::Min(100,AlterState->RedTithe+6);
        break;
    case EOGLuminousFieldRule::AlterThornUnbinding:
        AlterState->RuptureSeals.Remove(Contest.TargetUnitId);
        AlterState->SealExpiryOwnerAction.Remove(Contest.TargetUnitId);
        Field->AlterThornConsumedTargets.Add(Contest.TargetUnitId);
        break;
    case EOGLuminousFieldRule::AlterMirrorDebt:
        Field->bAlterMirrorDebtUsed=true;
        break;
    default:
        break;
    }
    // Alter P6 Negation Account: +8 Tithe only on genuine successful
    // World Negation, once for each of the four source rule categories.
    if(!bBase)
    {
        const uint8 Category=static_cast<uint8>(Contest.Rule);
        if(!Field->AlterNegationRefundedCategories.Contains(Category))
        {
            Field->AlterNegationRefundedCategories.Add(Category);
            AlterState->RedTithe=FMath::Min(100,AlterState->RedTithe+8);
        }
    }
    FOGCombatLogEvent Won;
    Won.Type=EOGCombatLogEventType::ActionResolved;
    Won.SourceUnitId=Contest.OwnerManifestationId;
    Won.TargetUnitId=Contest.TargetUnitId;
    Won.SkillId=Field->OwnerFantasmId;
    Won.PayloadJson=FString::Printf(
        TEXT("{\"resolved_registered_fantasm_rule\":%d,\"effect\":\"%s\",\"scope\":\"one_effect_or_boundary_segment\"}"),
        static_cast<int32>(Contest.Rule),
        *Contest.RegisteredEffectId.ToString());
    Log.Append(MoveTemp(Won));
    return true;
}
