#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"
#include "Combat/OGNativeTurnAgentComponent.h"

namespace
{
FOGCombatUnitState PolicyUnit(const FOGEntityId& Id, int32 Team, int64 Hp, int64 MaxHp)
{
    FOGCombatUnitState U;
    U.UnitEntityId = Id; U.TeamIndex = Team; U.CurrentHp = FOGLargeNumber::FromInt64(Hp);
    U.Stats.MaxHp = FOGLargeNumber::FromInt64(MaxHp); U.Presence = EOGCombatPresence::Active;
    U.DefaultActionDelay = 53;
    return U;
}
UOGNativeTurnPolicy* FixturePolicy()
{
    UOGNativeTurnPolicy* P = NewObject<UOGNativeTurnPolicy>();
    P->IdentityId = FOGContentId(TEXT("test:npc.identity"));
    P->VersionId = FOGContentId(TEXT("test:npc.kit"));
    P->CanonicalIdentityField = TEXT("identity"); P->CanonicalVersionField = TEXT("version");
    P->CanonicalLifeField = TEXT("life"); P->RequiredLifeValue = TEXT("live"); P->CanonicalPhaseField = TEXT("phase");
    return P;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeTurnPolicyPriorityTest,
    "OfflineGame.Combat.NativeTurnPolicy.ExactKitHealthPhaseAndFallback",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeTurnPolicyPriorityTest::RunTest(const FString& Parameters)
{
    UOGNativeTurnPolicy* P = FixturePolicy();
    const FOGContentId Attack(TEXT("test:attack")), Heal(TEXT("test:heal")), Ultimate(TEXT("test:ultimate"));
    const FOGEntityId Source = FOGEntityId::NewId(), A = FOGEntityId::NewId(), B = FOGEntityId::NewId();
    FOGTurnBattleState State; State.BattleId = FOGEntityId::NewId(); State.Status = EOGTurnBattleStatus::Running;
    State.Units = { PolicyUnit(Source, 0, 60, 100), PolicyUnit(A, 1, 30, 100), PolicyUnit(B, 1, 200, 1000) };
    State.Units[0].IdentityId = P->IdentityId; State.Units[0].SkillSet.CurrentForm = P->VersionId;
    State.Units[0].SkillSet.ActiveSkills = { Attack, Heal }; State.Units[0].SkillSet.UltimateSkill = Ultimate;
    FOGNativeTurnRule Strike; Strike.RuleId = TEXT("strike"); Strike.SkillId = Attack; Strike.Targets = EOGNativeTurnTarget::LowestHealthEnemy;
    FOGNativeTurnRule Restore; Restore.RuleId = TEXT("restore"); Restore.SkillId = Heal; Restore.Targets = EOGNativeTurnTarget::Self;
    Restore.Priority = 100; Restore.bUseOwnHealthWindow = true; Restore.MaximumOwnHealthBps = 2500;
    FOGNativeTurnRule Phase; Phase.RuleId = TEXT("phase"); Phase.SkillId = Ultimate; Phase.Priority = 200;
    Phase.RequiredPhase = TEXT("test.phase2"); Phase.ResolutionJson = TEXT("{\"branch\":\"fixture\"}");
    P->Rules = { Strike, Restore, Phase };
    FString Error; FName Rule; FOGResolvedCombatAction Action;
    TestTrue(TEXT("Exact authored policy validates"), P->Validate(Error));
    TestTrue(TEXT("Build attack from real snapshot"), P->BuildDecision(State, Source, NAME_None, Action, Rule, Error));
    TestTrue(TEXT("Lowest health is a ratio, not smallest absolute HP"), Action.TargetUnitIds.Num() == 1 && Action.TargetUnitIds[0] == B);
    TestEqual(TEXT("Source timing is retained without an AI balance multiplier"), Action.ActionDelay, int64(53));
    TestTrue(TEXT("Action has a new identity and unchanged source skill"), Action.ActionId.IsValid() && Action.SkillId == Attack);
    State.Units[0].CurrentHp = FOGLargeNumber::FromInt64(20);
    TestTrue(TEXT("Authored low-health self action wins"), P->BuildDecision(State, Source, NAME_None, Action, Rule, Error));
    TestTrue(TEXT("Healing choice targets self, not a fabricated ally"), Action.SkillId == Heal && Action.TargetUnitIds[0] == Source);
    TestTrue(TEXT("Actual canonical phase enables its rule"), P->BuildDecision(State, Source, TEXT("test.phase2"), Action, Rule, Error));
    TestTrue(TEXT("Approved branch context is retained for the executor"), Action.SkillId == Ultimate && Action.ResolutionJson == Phase.ResolutionJson);
    TSet<FName> Rejected = { Phase.RuleId, Restore.RuleId };
    TestTrue(TEXT("Rejected rules can yield only a separately authored fallback"), P->BuildDecision(State, Source, TEXT("test.phase2"), Action, Rule, Error, &Rejected));
    TestTrue(TEXT("Fallback is the actual owned attack"), Action.SkillId == Attack);
    Rejected.Add(Strike.RuleId);
    TestFalse(TEXT("No implicit basic attack exists after authored rules are exhausted"), P->BuildDecision(State, Source, TEXT("test.phase2"), Action, Rule, Error, &Rejected));
    State.Units[0].SkillSet.CurrentForm = FOGContentId(TEXT("test:other.kit"));
    TestFalse(TEXT("Policy cannot borrow another Version's skills"), P->BuildDecision(State, Source, NAME_None, Action, Rule, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeTurnPolicyFieldTest,
    "OfflineGame.Combat.NativeTurnPolicy.FullFieldAndDeterministicTies",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeTurnPolicyFieldTest::RunTest(const FString& Parameters)
{
    UOGNativeTurnPolicy* P = FixturePolicy();
    const FOGEntityId Source = FOGEntityId::NewId(); const FOGContentId Skill(TEXT("test:field.skill"));
    FOGTurnBattleState State; State.BattleId = FOGEntityId::NewId(); State.Status = EOGTurnBattleStatus::Running;
    State.Units.Add(PolicyUnit(Source, 0, 100, 100));
    State.Units[0].IdentityId = P->IdentityId; State.Units[0].SkillSet.CurrentForm = P->VersionId; State.Units[0].SkillSet.ActiveSkills.Add(Skill);
    for (int32 I = 0; I < 17; ++I) State.Units.Add(PolicyUnit(FOGEntityId::NewId(), 1, 10, 10));
    FOGCombatUnitState Reserve = PolicyUnit(FOGEntityId::NewId(), 1, 10, 10); Reserve.Presence = EOGCombatPresence::Reserve; State.Units.Add(Reserve);
    FOGNativeTurnRule Field; Field.RuleId = TEXT("a.field"); Field.SkillId = Skill; Field.Targets = EOGNativeTurnTarget::AllActiveEnemies;
    FOGNativeTurnRule Other = Field; Other.RuleId = TEXT("z.field");
    P->Rules = { Other, Field };
    FString Error; FName Rule; FOGResolvedCombatAction Action;
    TestTrue(TEXT("Build full-field decision"), P->BuildDecision(State, Source, NAME_None, Action, Rule, Error));
    TestEqual(TEXT("All 17 real active opponents remain addressable"), Action.TargetUnitIds.Num(), 17);
    TestFalse(TEXT("Reserve is never admitted as active AoE target"), Action.TargetUnitIds.Contains(Reserve.UnitEntityId));
    TestTrue(TEXT("Rule ties use stable authored key"), Rule == Field.RuleId);
    const auto Targets = Action.TargetUnitIds;
    Algo::Reverse(P->Rules); Algo::Reverse(State.Units);
    TestTrue(TEXT("Reordered arrays preserve decisions"), P->BuildDecision(State, Source, NAME_None, Action, Rule, Error));
    TestTrue(TEXT("Stable native target order and rule tie-break"), Rule == Field.RuleId && Action.TargetUnitIds == Targets);
    const FOGEntityId Strongest = Targets.Last();
    for (FOGCombatUnitState& Unit : State.Units)
        Unit.Stats.Attack = FOGLargeNumber::FromInt64(Unit.UnitEntityId == Strongest ? 9999 : 1);
    FOGNativeTurnRule Highest = Field; Highest.Targets = EOGNativeTurnTarget::HighestAttackEnemy;
    P->Rules = { Highest };
    TestTrue(TEXT("Highest-attack policy is selectable"), P->BuildDecision(State, Source, NAME_None, Action, Rule, Error));
    TestTrue(TEXT("Highest attack wins even when its UUID sorts last"), Action.TargetUnitIds.Num() == 1 && Action.TargetUnitIds[0] == Strongest);
    P->Rules[0].MaximumOwnHealthBps = -1;
    TestFalse(TEXT("Invalid condition bounds rejected"), P->Validate(Error));
    return true;
}

#endif
