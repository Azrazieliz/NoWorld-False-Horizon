#include "Combat/OGNativeCombatPassive.h"
#include "Combat/OGNativeCombatAbility.h"

bool OGNativePassivePredicates::IsSupported(FName Name)
{
    return Name==TEXT("owner_is_source") || Name==TEXT("owner_is_target") ||
        Name==TEXT("source_is_enemy") || Name==TEXT("target_is_enemy") ||
        Name==TEXT("owner_hp_at_most_bps") || Name==TEXT("owner_hp_at_least_bps") ||
        Name==TEXT("skill_is") || Name==TEXT("owner_in_reserve") ||
        Name==TEXT("owner_has_effect") || Name==TEXT("owner_has_effect_family") ||
        Name==TEXT("source_has_effect") || Name==TEXT("target_has_effect");
}
bool FOGNativePassiveSpecification::MatchesSource(const FOGCombatUnitState& Unit) const
{
    return (IdentityId.IsEmpty() || IdentityId==Unit.IdentityId) &&
        (VersionId.IsEmpty() || VersionId==Unit.SkillSet.CurrentForm) &&
        Unit.SkillSet.PassiveSkills.Contains(PassiveSkillId) && !Unit.SkillSet.HasDuplicateSkills();
}
bool FOGNativePassiveSpecification::Validate(FString& Error) const
{
    Error.Reset();
    if (!PassiveSkillId.IsValid() || (!IdentityId.IsEmpty() && !IdentityId.IsValid()) ||
        (!VersionId.IsEmpty() && !VersionId.IsValid()) || OriginalSourceReference.TrimStartAndEnd().IsEmpty() ||
        Triggers.IsEmpty() || PassiveSkillId.Value.StartsWith(TEXT("diagnostic:")) ||
        IdentityId.Value.StartsWith(TEXT("diagnostic:")) || VersionId.Value.StartsWith(TEXT("diagnostic:")) || IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity")))
    { Error=TEXT("Passive requires sourced mechanics and a valid shared skill/scope; specialized Luminous passives retain their executor."); return false; }
    TSet<FOGContentId> Ids;
    for (const auto& Rule:Triggers)
    {
        const auto& T=Rule.Trigger;
        const bool Event=T.EventType==TEXT("OnBattleStart") || T.EventType==TEXT("OnDefeat") ||
            T.EventType==TEXT("OnEntry") || T.EventType==TEXT("OnEnterReserve") ||
            T.EventType==TEXT("OnDamageDealt") || T.EventType==TEXT("OnDamageReceived") ||
            T.EventType==TEXT("OnHealingApplied") || T.EventType==TEXT("OnActionResolved") || T.EventType==TEXT("OnOwnerActionEnd") ||
            T.EventType==TEXT("OnEffectApplied") || T.EventType==TEXT("OnEffectRemoved") ||
            T.EventType==TEXT("OnEffectTicked") || T.EventType==TEXT("OnEffectResisted");
        if (!T.TriggerId.IsValid() || T.TriggerId.Value.StartsWith(TEXT("diagnostic:")) || Ids.Contains(T.TriggerId) || !Event || T.ActionIds.IsEmpty() ||
            Rule.MaximumActivationsPerRoot<1 || Rule.MaximumActivationsPerOwnerAction<0 || Rule.Priority<1 ||
            static_cast<uint8>(Rule.Target)>static_cast<uint8>(EOGNativeReactionTarget::LowestHealthAlly) ||
            Rule.Unavailable==EOGNativeReactionUnavailable::Unspecified ||
            static_cast<uint8>(Rule.Unavailable)>static_cast<uint8>(EOGNativeReactionUnavailable::RejectRootAction))
        { Error=TEXT("Passive trigger requires a supported event, distinct ID, actions, explicit finite activation limit and unavailable policy."); return false; }
        Ids.Add(T.TriggerId); TSet<FOGContentId> Actions;
        for (const auto& Id:T.ActionIds)
        {
            if (!Id.IsValid() || Id.Value.StartsWith(TEXT("diagnostic:")) || Actions.Contains(Id)) { Error=TEXT("Passive actions must have distinct registered ability IDs."); return false; }
            Actions.Add(Id);
        }
        for (const auto& C:T.Conditions)
        {
            const bool Hp=C.Predicate==TEXT("owner_hp_at_most_bps") || C.Predicate==TEXT("owner_hp_at_least_bps");
            const bool Effect=C.Predicate==TEXT("owner_has_effect") || C.Predicate==TEXT("owner_has_effect_family") ||
                C.Predicate==TEXT("source_has_effect") || C.Predicate==TEXT("target_has_effect");
            const bool Content=Effect || C.Predicate==TEXT("skill_is");
            const bool OwnerOnly=C.Predicate==TEXT("owner_has_effect") || C.Predicate==TEXT("owner_has_effect_family");
            if (!OGNativePassivePredicates::IsSupported(C.Predicate) || !C.NameOperand.IsNone() ||
                (!Hp && C.IntegerOperand!=0) || (!Content && !C.ContentOperand.IsEmpty()) ||
                (Hp && (C.IntegerOperand<0 || C.IntegerOperand>10000)) ||
                (Content && !C.ContentOperand.IsValid()) ||
                (T.EventType==TEXT("OnBattleStart") && !Hp && !OwnerOnly && C.Predicate!=TEXT("owner_in_reserve")))
            { Error=TEXT("Passive condition is unsupported, invalid or requires context absent from BattleStart."); return false; }
        }
    }
    return true;
}
UOGNativeCombatPassive::UOGNativeCombatPassive() { System=TEXT("passive"); }
bool UOGNativeCombatPassive::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("passive") || DefinitionId!=Specification.PassiveSkillId)
    { Error=TEXT("Passive definition key mismatch."); return false; }
    return Specification.Validate(Error);
}
void UOGNativeCombatPassive::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{
    Super::CollectRequiredDefinitions(Out);
    for (const auto& Rule:Specification.Triggers) for (const auto& Action:Rule.Trigger.ActionIds)
    { FOGDefinitionReference Ref; Ref.System=TEXT("ability"); Ref.Id=Action; Out.Add(Ref); }
}
bool UOGNativeCombatPassive::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    for (const auto& Rule:Specification.Triggers) for (const auto& Id:Rule.Trigger.ActionIds)
        if (!Cast<UOGNativeCombatAbility>(Resolve(TEXT("ability"),Id)))
        { Error=TEXT("Every passive action must resolve to an installed native ability definition."); return false; }
    return true;
}
