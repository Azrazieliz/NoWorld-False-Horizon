#include "Combat/OGNativeTurnAgentComponent.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool LowerHealthRatio(const FOGCombatUnitState& A, const FOGCombatUnitState& B)
{
    const int32 Comparison = FOGLargeNumber::Compare(
        FOGLargeNumber::Multiply(A.CurrentHp, B.Stats.MaxHp),
        FOGLargeNumber::Multiply(B.CurrentHp, A.Stats.MaxHp));
    return Comparison < 0 || (Comparison == 0 && A.UnitEntityId.ToString() < B.UnitEntityId.ToString());
}

bool PickTargets(const FOGNativeTurnRule& Rule, const FOGCombatUnitState& Source,
    const FOGTurnBattleState& State, TArray<FOGEntityId>& Out)
{
    if (Rule.Targets == EOGNativeTurnTarget::Self) { Out = { Source.UnitEntityId }; return true; }
    TArray<const FOGCombatUnitState*> Eligible;
    const bool bAllies = Rule.Targets == EOGNativeTurnTarget::LowestHealthAlly ||
        Rule.Targets == EOGNativeTurnTarget::AllActiveAllies || Rule.Targets == EOGNativeTurnTarget::DefeatedAlly;
    for (const FOGCombatUnitState& Unit : State.Units)
    {
        if ((Unit.TeamIndex == Source.TeamIndex) != bAllies ||
            (Unit.UnitEntityId == Source.UnitEntityId && !Rule.bIncludeSelf)) continue;
        if (Rule.Targets == EOGNativeTurnTarget::DefeatedAlly)
        { if (Unit.Presence != EOGCombatPresence::Defeated || Unit.IsAlive()) continue; }
        else if (!Unit.CanAct()) continue;
        if (!Unit.UnitEntityId.IsValid() || Unit.Stats.MaxHp.GetSign() <= 0) continue;
        Eligible.Add(&Unit);
    }
    if (Eligible.IsEmpty()) return false;
    Eligible.Sort([](const FOGCombatUnitState& A, const FOGCombatUnitState& B)
        { return A.UnitEntityId.ToString() < B.UnitEntityId.ToString(); });
    const FOGCombatUnitState* Selected = Eligible[0];
    if (Rule.Targets == EOGNativeTurnTarget::LowestHealthAlly || Rule.Targets == EOGNativeTurnTarget::LowestHealthEnemy)
    {
        for (const FOGCombatUnitState* Unit : Eligible) if (LowerHealthRatio(*Unit, *Selected)) Selected = Unit;
    }
    else if (Rule.Targets == EOGNativeTurnTarget::HighestAttackEnemy)
    {
        for (const FOGCombatUnitState* Unit : Eligible)
            if (FOGLargeNumber::Compare(Unit->Stats.Attack, Selected->Stats.Attack) > 0) Selected = Unit;
    }
    if (Rule.Targets == EOGNativeTurnTarget::AllActiveEnemies || Rule.Targets == EOGNativeTurnTarget::AllActiveAllies)
        for (const FOGCombatUnitState* Unit : Eligible) Out.Add(Unit->UnitEntityId);
    else Out.Add(Selected->UnitEntityId);
    return true;
}
}

bool UOGNativeTurnPolicy::MatchesSource(const FOGContentId& Identity,const FOGContentId& Version) const
{ return (IdentityId.IsEmpty() || IdentityId==Identity) && (VersionId.IsEmpty() || VersionId==Version); }
bool UOGNativeTurnPolicy::Validate(FString& Error) const
{
    Error.Reset();
    if ((!IdentityId.IsEmpty() && !IdentityId.IsValid()) || (!VersionId.IsEmpty() && !VersionId.IsValid()) || IdentityId.Value.StartsWith(TEXT("diagnostic:")) ||
        VersionId.Value.StartsWith(TEXT("diagnostic:")) || CanonicalIdentityField.IsNone() ||
        CanonicalVersionField.IsNone() || CanonicalLifeField.IsNone() || RequiredLifeValue.IsEmpty() ||
        CanonicalIdentityField == CanonicalVersionField || CanonicalIdentityField == CanonicalLifeField ||
        CanonicalVersionField == CanonicalLifeField || !FMath::IsFinite(DecisionDelaySeconds) ||
        DecisionDelaySeconds < 0.f || Rules.IsEmpty() || Rules.Num() > 256)
    { Error = TEXT("Turn policy requires exact source identity/kit, canonical identity/life fields and authored rules."); return false; }
    TSet<FName> Ids;
    for (const FOGNativeTurnRule& Rule : Rules)
    {
        TSharedPtr<FJsonObject> Resolution;
        if (Rule.RuleId.IsNone() || Ids.Contains(Rule.RuleId) || !Rule.SkillId.IsValid() ||
            static_cast<uint8>(Rule.Targets) > static_cast<uint8>(EOGNativeTurnTarget::DefeatedAlly) ||
            Rule.MinimumOwnHealthBps < 0 || Rule.MaximumOwnHealthBps > 10000 ||
            Rule.MinimumOwnHealthBps > Rule.MaximumOwnHealthBps ||
            Rule.ResolutionJson.Len() > 64 * 1024 ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Rule.ResolutionJson), Resolution) || !Resolution ||
            (!Rule.RequiredPhase.IsNone() && CanonicalPhaseField.IsNone()))
        { Error = TEXT("Invalid or duplicate authored AI rule, health window, phase or command context."); return false; }
        Ids.Add(Rule.RuleId);
    }
    return true;
}

bool UOGNativeTurnPolicy::BuildDecision(const FOGTurnBattleState& State, const FOGEntityId& SourceId,
    FName Phase, FOGResolvedCombatAction& Out, FName& OutRule, FString& Error, const TSet<FName>* ExcludedRules) const
{
    Out = FOGResolvedCombatAction(); OutRule = NAME_None;
    if (!Validate(Error)) return false;
    const FOGCombatUnitState* Source = State.Units.FindByPredicate(
        [&SourceId](const FOGCombatUnitState& U) { return U.UnitEntityId == SourceId; });
    if (State.Status != EOGTurnBattleStatus::Running || !State.BattleId.IsValid() || !Source || !Source->CanAct() ||
        !MatchesSource(Source->IdentityId,Source->SkillSet.CurrentForm) || Source->DefaultActionDelay <= 0 ||
        Source->Stats.MaxHp.GetSign() <= 0)
    { Error = TEXT("No matching active real source unit/kit is available to this AI policy."); return false; }
    const FOGNativeTurnRule* Best = nullptr;
    TArray<FOGEntityId> BestTargets;
    for (const FOGNativeTurnRule& Rule : Rules)
    {
        if (ExcludedRules && ExcludedRules->Contains(Rule.RuleId)) continue;
        if ((!Rule.RequiredPhase.IsNone() && Rule.RequiredPhase != Phase) ||
            (!Source->SkillSet.ActiveSkills.Contains(Rule.SkillId) && !Source->SkillSet.IsUltimate(Rule.SkillId))) continue;
        if (Rule.bUseOwnHealthWindow &&
            (FOGLargeNumber::Compare(Source->CurrentHp, FOGLargeNumber::ScaleByBasisPoints(Source->Stats.MaxHp, Rule.MinimumOwnHealthBps)) < 0 ||
             FOGLargeNumber::Compare(Source->CurrentHp, FOGLargeNumber::ScaleByBasisPoints(Source->Stats.MaxHp, Rule.MaximumOwnHealthBps)) > 0)) continue;
        TArray<FOGEntityId> Targets;
        if (!PickTargets(Rule, *Source, State, Targets)) continue;
        if (!Best || Rule.Priority > Best->Priority ||
            (Rule.Priority == Best->Priority && Rule.RuleId.LexicalLess(Best->RuleId)))
        { Best = &Rule; BestTargets = MoveTemp(Targets); }
    }
    if (!Best) { Error = TEXT("No authored legal decision applies; no substitute attack was generated."); return false; }
    Out.ActionId = FOGEntityId::NewId();
    Out.SourceUnitId = SourceId;
    Out.SkillId = Best->SkillId;
    Out.TargetUnitIds = MoveTemp(BestTargets);
    Out.ActionDelay = Source->DefaultActionDelay;
    Out.ResolutionJson = Best->ResolutionJson;
    OutRule = Best->RuleId;
    return true;
}
