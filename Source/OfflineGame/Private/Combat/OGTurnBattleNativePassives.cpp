#include "Combat/OGTurnBattle.h"
#include "Misc/SecureHash.h"

namespace
{
FString PassiveKey(const FOGEntityId& Owner,const FOGContentId& Id)
{ return Owner.ToString()+TEXT("|")+Id.ToString(); }
bool OrdinaryOwns(const FOGCombatUnitState& Unit,const FOGContentId& Skill)
{ return !Unit.SkillSet.HasDuplicateSkills() && (Unit.SkillSet.ActiveSkills.Contains(Skill) || Unit.SkillSet.IsUltimate(Skill)); }
}
bool FOGTurnBattle::InitializeNativePassives(const TArray<FOGNativePassiveBinding>& Passives,
    TArray<FOGCombatTriggerBinding>& Bindings,FString& Error)
{
    TSet<FString> Keys;
    for (const auto& B:Bindings) Keys.Add(PassiveKey(B.OwnerUnitId,B.Trigger.TriggerId));
    for (const auto& B:Passives)
    {
        const auto* Owner=FindUnit(B.OwnerUnitId);
        if (!Owner || !B.Specification.Validate(Error) || !B.Specification.MatchesSource(*Owner) ||
            Owner->IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity")))
        { if (Error.IsEmpty()) Error=TEXT("Native passive requires its actual participant and exact owned passive kit."); return false; }
        for (const auto& Rule:B.Specification.Triggers)
        {
            const FString Key=PassiveKey(B.OwnerUnitId,Rule.Trigger.TriggerId);
            if (Keys.Contains(Key)) { Error=TEXT("Duplicate owner/trigger registration."); return false; }
            Keys.Add(Key);
            FNativePassiveRule Native; Native.OwnerUnitId=B.OwnerUnitId; Native.PassiveSkillId=B.Specification.PassiveSkillId; Native.Rule=Rule;
            NativePassiveRules.Add(Key,Native);
            FOGCombatTriggerBinding Trigger; Trigger.OwnerUnitId=B.OwnerUnitId; Trigger.Trigger=Rule.Trigger; Bindings.Add(Trigger);
            for (const auto& Action:Rule.Trigger.ActionIds) NativePassiveActionOwners.Add(PassiveKey(B.OwnerUnitId,Action));
        }
    }
    return true;
}
bool FOGTurnBattle::CanAdmitNativeAbility(const FOGCombatUnitState& Unit,const FOGContentId& Skill) const
{ return OrdinaryOwns(Unit,Skill) || NativePassiveActionOwners.Contains(PassiveKey(Unit.UnitEntityId,Skill)); }
bool FOGTurnBattle::HasNativeReactionProof(const FOGCombatUnitState& Unit,const FOGContentId& Skill) const
{
    if (!NativeReactionProof.IsSet()) return false;
    const auto& Q=NativeReactionProof.GetValue();
    const auto* Rule=NativePassiveRules.Find(PassiveKey(Unit.UnitEntityId,Q.TriggerId));
    return Q.OwnerUnitId==Unit.UnitEntityId && Q.ActionId==Skill && Rule &&
        Rule->Rule.Trigger.ActionIds.Contains(Skill) && OutstandingTriggeredActionSequences.Contains(Q.QueueSequence);
}
bool FOGTurnBattle::IsNativeAbilityAuthorized(const FOGCombatUnitState& Unit,const FOGContentId& Skill) const
{ return OrdinaryOwns(Unit,Skill) || HasNativeReactionProof(Unit,Skill); }
bool FOGTurnBattle::AllowsNativeReactionSource(const FOGCombatUnitState& Unit,const FOGResolvedCombatAction& Action) const
{
    if (Action.InterruptPriority<=0 || !HasNativeReactionProof(Unit,Action.SkillId)) return false;
    const auto& Q=NativeReactionProof.GetValue();
    const auto* Rule=NativePassiveRules.Find(PassiveKey(Unit.UnitEntityId,Q.TriggerId));
    return Unit.IsAlive() && (Unit.CanAct() || (Unit.Presence==EOGCombatPresence::Reserve && Rule->Rule.bAllowReserveSource));
}
bool FOGTurnBattle::EvaluateNativePassiveCondition(const FOGCombatTriggerBinding& B,
    const FOGConditionDefinition& C,const FOGCombatTriggerContext& E) const
{
    if (!NativePassiveRules.Contains(PassiveKey(B.OwnerUnitId,B.Trigger.TriggerId)))
        return NativeFallbackConditions && NativeFallbackConditions(B,C,E);
    const auto* Owner=FindUnit(B.OwnerUnitId); if (!Owner) return false;
    const auto* Source=FindUnit(E.SourceUnitId); const auto* Target=FindUnit(E.TargetUnitId);
    if (C.Predicate==TEXT("owner_is_source")) return E.SourceUnitId==B.OwnerUnitId;
    if (C.Predicate==TEXT("owner_is_target")) return E.TargetUnitId==B.OwnerUnitId;
    if (C.Predicate==TEXT("source_is_enemy")) return Source && Source->TeamIndex!=Owner->TeamIndex;
    if (C.Predicate==TEXT("target_is_enemy")) return Target && Target->TeamIndex!=Owner->TeamIndex;
    if (C.Predicate==TEXT("owner_in_reserve")) return Owner->Presence==EOGCombatPresence::Reserve;
    if (C.Predicate==TEXT("skill_is")) return E.SkillId==C.ContentOperand;
    if (C.Predicate==TEXT("owner_has_effect")) return HasNativeEffect(B.OwnerUnitId,C.ContentOperand);
    if (C.Predicate==TEXT("owner_has_effect_family")) return HasNativeEffect(B.OwnerUnitId,C.ContentOperand,true);
    if (C.Predicate==TEXT("source_has_effect")) return HasNativeEffect(E.SourceUnitId,C.ContentOperand);
    if (C.Predicate==TEXT("target_has_effect")) return HasNativeEffect(E.TargetUnitId,C.ContentOperand);
    const auto Threshold=FOGLargeNumber::ScaleByBasisPoints(Owner->Stats.MaxHp,static_cast<int32>(C.IntegerOperand));
    if (C.Predicate==TEXT("owner_hp_at_most_bps")) return FOGLargeNumber::Compare(Owner->CurrentHp,Threshold)<=0;
    if (C.Predicate==TEXT("owner_hp_at_least_bps")) return FOGLargeNumber::Compare(Owner->CurrentHp,Threshold)>=0;
    return false;
}
bool FOGTurnBattle::SelectNativeReactionTarget(const FOGQueuedTriggeredAction& Q,const FNativePassiveRule& Registered,
    FOGResolvedCombatAction& Action,FString& Error) const
{
    Action=FOGResolvedCombatAction(); Action.SourceUnitId=Q.OwnerUnitId; Action.SkillId=Q.ActionId;
    Action.InterruptPriority=Registered.Rule.Priority; Action.ActionDelay=0;
    const auto* Owner=FindUnit(Q.OwnerUnitId);
    if (!Owner) { Error=TEXT("Reaction owner is absent."); return false; }
    const auto* Ability=FindNativeAbility(*Owner,Q.ActionId);
    if (!Ability) { Error=TEXT("Reaction's registered native ability is absent."); return false; }
    const FString Identity=State.BattleId.ToString()+FString::Printf(TEXT(":reaction:%lld"),static_cast<long long>(Q.QueueSequence));
    FGuid Guid; FGuid::ParseExact(FMD5::HashAnsiString(*Identity),EGuidFormats::Digits,Guid); Action.ActionId=FOGEntityId(Guid);
    const auto Selection=Registered.Rule.Target;
    if (Ability->Targets==EOGNativeAbilityTargets::Self || Selection==EOGNativeReactionTarget::Owner)
        Action.TargetUnitIds.Add(Q.OwnerUnitId);
    else if (Selection==EOGNativeReactionTarget::EventSource) Action.TargetUnitIds.Add(Q.EventSourceUnitId);
    else if (Selection==EOGNativeReactionTarget::EventTarget) Action.TargetUnitIds.Add(Q.EventTargetUnitId);
    else
    {
        const bool Allies=Selection==EOGNativeReactionTarget::LowestHealthAlly;
        TArray<const FOGCombatUnitState*> Targets;
        for (const auto& U:State.Units)
            if (U.CanAct() && ((U.TeamIndex==Owner->TeamIndex)==Allies) &&
                (U.UnitEntityId!=Owner->UnitEntityId || Ability->bIncludeSelf)) Targets.Add(&U);
        Targets.Sort([Selection](const FOGCombatUnitState& A,const FOGCombatUnitState& B)
        {
            if (Selection!=EOGNativeReactionTarget::FirstEnemy)
            {
                const int32 Health=FOGLargeNumber::Compare(
                    FOGLargeNumber::Multiply(A.CurrentHp,B.Stats.MaxHp),FOGLargeNumber::Multiply(B.CurrentHp,A.Stats.MaxHp));
                if (Health) return Health<0;
            }
            return A.UnitEntityId.ToString()<B.UnitEntityId.ToString();
        });
        if (!Targets.IsEmpty()) Action.TargetUnitIds.Add(Targets[0]->UnitEntityId);
    }
    if (Action.TargetUnitIds.IsEmpty()) { Error=TEXT("Reaction has no current eligible target."); return false; }
    return true;
}
bool FOGTurnBattle::ResolveNativeOpeningReactions(FString& Error)
{
    for (const auto& Pair:NativePassiveRules)
    {
        const auto* Owner=FindUnit(Pair.Value.OwnerUnitId);
        for (const auto& Action:Pair.Value.Rule.Trigger.ActionIds)
            if (!Owner || !FindNativeAbility(*Owner,Action))
            { Error=TEXT("Missing source-bound native passive executor: ")+Action.ToString(); return false; }
    }
    FOGTurnBattle Candidate=*this; Candidate.LastNativeReactions.Reset();
    if (!Candidate.ResolveNativeReactions(Error)) return false;
    *this=MoveTemp(Candidate); return true;
}
bool FOGTurnBattle::ResolveNativeReactions(FString& Error,TMap<FString,int32>* RootActivations)
{
    TMap<FString,int32> LocalActivations;
    auto& Activations=RootActivations?*RootActivations:LocalActivations;
    TSet<FString> AcceptedGroups,RejectedGroups;
    while (true)
    {
        int32 Index=INDEX_NONE; int32 BestPriority=MIN_int32;
        for (int32 I=0;I<PendingTriggeredActions.Num();++I)
        {
            const auto& Q=PendingTriggeredActions[I];
            const auto* Rule=NativePassiveRules.Find(PassiveKey(Q.OwnerUnitId,Q.TriggerId));
            if (Rule && (Index==INDEX_NONE || Rule->Rule.Priority>BestPriority ||
                (Rule->Rule.Priority==BestPriority && Q.QueueSequence<PendingTriggeredActions[Index].QueueSequence)))
            { Index=I; BestPriority=Rule->Rule.Priority; }
        }
        if (Index==INDEX_NONE) break; // Existing external trigger consumers retain their own pending queue.
        const auto Q=PendingTriggeredActions[Index]; PendingTriggeredActions.RemoveAt(Index);
        const auto Registered=NativePassiveRules.FindChecked(PassiveKey(Q.OwnerUnitId,Q.TriggerId));
        FOGNativeReactionRecord Record; Record.QueueSequence=Q.QueueSequence; Record.SourceSequence=Q.SourceSequence;
        Record.PassiveSkillId=Registered.PassiveSkillId; Record.TriggerId=Q.TriggerId;
        Record.EventType=Q.EventType; Record.EventSourceUnitId=Q.EventSourceUnitId;
        Record.EventTargetUnitId=Q.EventTargetUnitId; Record.EventSkillId=Q.EventSkillId;
        Record.Action.SourceUnitId=Q.OwnerUnitId; Record.Action.SkillId=Q.ActionId;
        const FString Trigger=PassiveKey(Q.OwnerUnitId,Q.TriggerId);
        const FString Group=Trigger+FString::Printf(TEXT("|%lld"),static_cast<long long>(Q.SourceSequence));
        FString Unavailable;
        const auto* Owner=FindUnit(Q.OwnerUnitId);
        if (!Owner || !FindNativeAbility(*Owner,Q.ActionId))
        { Error=TEXT("Missing admitted native reaction executor: ")+Q.ActionId.ToString(); return false; }
        if (!Owner || (!Owner->CanAct() && !(Owner->IsAlive() && Owner->Presence==EOGCombatPresence::Reserve && Registered.Rule.bAllowReserveSource)))
            Unavailable=TEXT("reaction_owner_unavailable");
        if (!AcceptedGroups.Contains(Group) && !RejectedGroups.Contains(Group))
        {
            if (Activations.FindRef(Trigger)>=Registered.Rule.MaximumActivationsPerRoot) RejectedGroups.Add(Group);
            else { AcceptedGroups.Add(Group); ++Activations.FindOrAdd(Trigger); }
        }
        if (RejectedGroups.Contains(Group)) Record.Outcome=TEXT("authored_root_activation_limit");
        else
        {
            NativeReactionProof=Q;
            FOGResolvedCombatAction Proposal,Expanded;
            if (Unavailable.IsEmpty())
            {
                if (!SelectNativeReactionTarget(Q,Registered,Proposal,Unavailable) ||
                    !ValidateResolvedAction(Proposal,Expanded,Unavailable))
                    if (Unavailable.IsEmpty()) Unavailable=TEXT("Reaction preflight rejected the current target/state.");
            }
            if (Unavailable.IsEmpty())
            {
                if (!ApplyResolvedActionMutating(Expanded,Error)) { NativeReactionProof.Reset(); return false; }
                Record.Action=Expanded; Record.bExecuted=true; Record.Outcome=TEXT("executed");
            }
            else
            {
                Record.Outcome=Unavailable;
                if (Registered.Rule.Unavailable==EOGNativeReactionUnavailable::RejectRootAction)
                { NativeReactionProof.Reset(); Error=Unavailable; return false; }
            }
            NativeReactionProof.Reset();
        }
        LastNativeReactions.Add(MoveTemp(Record));
        if (!CompleteTriggeredAction(Q.QueueSequence,Error)) return false;
    }
    if (bRequireNativeExecutors && !OutstandingTriggeredActionSequences.IsEmpty())
    { Error=TEXT("An existing trigger has no installed automatic native executor; its registration requires a concrete source action binding."); return false; }
    return true;
}
