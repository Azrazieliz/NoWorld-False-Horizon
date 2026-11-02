#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGCombatActionJournal.h"

namespace
{
FOGContentId EffectTestId(const TCHAR* S) { return FOGContentId(FString(TEXT("test:effects."))+S); }
struct FEffectFixture
{
    FOGTurnBattle Battle;
    FOGEntityId A=FOGEntityId::NewId(),B=FOGEntityId::NewId();
    FString Error;
    bool Initialize(bool EnemyActs=false)
    {
        FOGCombatUnitState One,Two; One.UnitEntityId=A; Two.UnitEntityId=B;
        One.IdentityId=EffectTestId(TEXT("identity_a")); Two.IdentityId=EffectTestId(TEXT("identity_b"));
        One.SkillSet.CurrentForm=Two.SkillSet.CurrentForm=EffectTestId(TEXT("version"));
        for (const TCHAR* Skill:{TEXT("basic"),TEXT("effect"),TEXT("dispel")}) One.SkillSet.ActiveSkills.Add(EffectTestId(Skill));
        Two.SkillSet.ActiveSkills={EffectTestId(TEXT("basic"))};
        One.TeamIndex=0; Two.TeamIndex=1; One.Presence=Two.Presence=EOGCombatPresence::Active;
        One.Stats.MaxHp=Two.Stats.MaxHp=FOGLargeNumber::FromInt64(1000);
        One.CurrentHp=Two.CurrentHp=One.Stats.MaxHp;
        One.Stats.Attack=Two.Stats.Attack=FOGLargeNumber::FromInt64(100);
        One.Stats.Speed=Two.Stats.Speed=100; One.Stats.HitBps=Two.Stats.HitBps=10000;
        One.DefaultActionDelay=Two.DefaultActionDelay=100;
        Two.NextActionValue=EnemyActs?1:10000;
        FOGTurnBattleState State; State.BattleId=FOGEntityId::NewId(); State.Units={One,Two};
        for (const auto& Unit:State.Units)
        {
            FOGTurnTeamState Team; Team.TeamIndex=Unit.TeamIndex;
            FOGTurnSuccessionLane Lane; Lane.LaneIndex=0; Lane.OrderedUnitIds={Unit.UnitEntityId};
            Team.Lanes={Lane}; State.Teams.Add(Team);
        }
        if (!Battle.Initialize(State,Error)) return false;
        Battle.RequireNativeActionExecutors(true);
        FOGNativeAbilityDefinition Basic; Basic.SkillId=EffectTestId(TEXT("basic")); Basic.bNoResourceCost=true;
        Basic.OriginalSourceReference=TEXT("native shared-effect integration fixture");
        FOGNativeAbilityPacket Packet; Packet.DefenseReference=EOGNativeDefenseReference::Attack;
        FOGNativeAbilityScale Scale; Scale.Stat=EOGNativeScaleStat::Attack; Scale.MultiplierBps=10000;
        Packet.Scaling={Scale}; Basic.Packets={Packet};
        return Battle.RegisterNativeAbility(Basic,Error);
    }
    FOGNativeEffectDefinition Effect(EOGNativeEffectMechanic Mechanic,int64 Turns=2)
    {
        FOGNativeEffectDefinition D; D.Effect.EffectId=EffectTestId(TEXT("definition")); D.Effect.FamilyId=EffectTestId(TEXT("family"));
        D.Effect.MechanicKind=TEXT("test_native_effect"); D.Effect.RulePriority.SourceRuleId=EffectTestId(TEXT("rule"));
        D.Effect.Lifetime.Kind=EOGEffectLifetimeKind::Turns; D.Effect.Lifetime.Magnitude=Turns;
        D.OriginalSourceReference=TEXT("source-independent native effect fixture"); D.Mechanic=Mechanic;
        if (Mechanic==EOGNativeEffectMechanic::StatModifier)
        { FOGNativeEffectModifier Modifier; Modifier.Stat=EOGNativeEffectStat::Attack; Modifier.AdditiveMultiplierBps=10000; D.Modifiers={Modifier}; }
        else D.FlatMagnitude=FOGLargeNumber::FromInt64(25);
        return D;
    }
    bool Install(const FOGNativeEffectDefinition& D,bool Self=false)
    {
        if (!Battle.RegisterNativeEffect(D,Error)) return false;
        FOGNativeAbilityDefinition Ability; Ability.SkillId=EffectTestId(TEXT("effect")); Ability.bNoResourceCost=true;
        Ability.Targets=Self?EOGNativeAbilityTargets::Self:EOGNativeAbilityTargets::SingleEnemy;
        Ability.OriginalSourceReference=TEXT("native effect-only ability fixture");
        FOGNativeAbilityEffectOperation Op; Op.Id=D.Effect.EffectId; Ability.Effects={Op};
        return Battle.RegisterNativeAbility(Ability,Error);
    }
    bool Act(const FOGEntityId& Source,const FOGEntityId& Target,const TCHAR* Skill)
    {
        FOGResolvedCombatAction Action; Action.ActionId=FOGEntityId::NewId(); Action.SourceUnitId=Source;
        Action.TargetUnitIds={Target}; Action.SkillId=EffectTestId(Skill); Action.ActionDelay=100;
        return Battle.ApplyResolvedAction(Action,Error);
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeEffectModifierExpiryTest,
    "OfflineGame.Combat.SharedEffects.ModifiersExpireWithoutChangingBaseStats",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeEffectModifierExpiryTest::RunTest(const FString&)
{
    FEffectFixture F; if (!TestTrue(TEXT("Initialize"),F.Initialize())) return false;
    auto D=F.Effect(EOGNativeEffectMechanic::StatModifier); D.bBeneficial=true;
    if (!TestTrue(TEXT("Admit shared modifier and effect-only ability"),F.Install(D,true))) return false;
    if (!TestTrue(TEXT("Apply source-owned effect"),F.Act(F.A,F.A,TEXT("effect")))) return false;
    TestTrue(TEXT("Effective attack doubles"),F.Battle.GetNativeEffectiveStats(F.A).Attack==FOGLargeNumber::FromInt64(200));
    TestTrue(TEXT("Base stat stays unchanged"),F.Battle.GetState().Units[0].Stats.Attack==FOGLargeNumber::FromInt64(100));
    TestTrue(TEXT("Applying action did not consume duration"),F.Battle.GetState().Units[0].NativeEffects[0].RemainingActions==2);
    if (!TestTrue(TEXT("First beneficiary action"),F.Act(F.A,F.B,TEXT("basic")))) return false;
    if (!TestTrue(TEXT("Second beneficiary action"),F.Act(F.A,F.B,TEXT("basic")))) return false;
    TestFalse(TEXT("Expired instance removed"),F.Battle.HasNativeEffect(F.A,D.Effect.EffectId));
    if (!TestTrue(TEXT("Unmodified action"),F.Act(F.A,F.B,TEXT("basic")))) return false;
    TestTrue(TEXT("Actual damage uses modified then original stats"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(500));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeEffectBarrierTest,
    "OfflineGame.Combat.SharedEffects.BarriersAbsorbActualDamageAndReleaseTheirSlot",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeEffectBarrierTest::RunTest(const FString&)
{
    FEffectFixture F; if (!F.Initialize(true)) return false;
    auto D=F.Effect(EOGNativeEffectMechanic::Barrier); D.bBeneficial=true; D.FlatMagnitude=FOGLargeNumber::FromInt64(30);
    if (!F.Install(D,true) || !F.Act(F.A,F.A,TEXT("effect")) || !F.Act(F.B,F.A,TEXT("basic"))) return false;
    TestTrue(TEXT("Barrier absorbs 30 of 100"),F.Battle.GetState().Units[0].CurrentHp==FOGLargeNumber::FromInt64(930));
    TestTrue(TEXT("Effective HP damage excludes absorbed damage"),F.Battle.GetLastEffectiveHpDamage(F.A)==FOGLargeNumber::FromInt64(70));
    TestFalse(TEXT("Depleted barrier removes concrete instance"),F.Battle.HasNativeEffect(F.A,D.Effect.EffectId));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativePeriodicEffectTest,
    "OfflineGame.Combat.SharedEffects.PeriodicDamageTicksOnItsDeclaredClock",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativePeriodicEffectTest::RunTest(const FString&)
{
    FEffectFixture F; if (!F.Initialize(true)) return false;
    auto D=F.Effect(EOGNativeEffectMechanic::PeriodicDamage,1);
    if (!F.Install(D) || !F.Act(F.A,F.B,TEXT("effect"))) return false;
    TestTrue(TEXT("Application does not invent an initial tick"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(1000));
    if (!F.Act(F.B,F.A,TEXT("basic"))) return false;
    TestTrue(TEXT("Actual periodic HP loss"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(975));
    TestFalse(TEXT("Final tick expires effect"),F.Battle.HasNativeEffect(F.B,D.Effect.EffectId));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeEffectDispelAndReceiptTest,
    "OfflineGame.Combat.SharedEffects.DispelRespectsPriorityAndReceiptsIncludeStatusOnlyChanges",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeEffectDispelAndReceiptTest::RunTest(const FString&)
{
    FEffectFixture F; if (!F.Initialize()) return false;
    auto D=F.Effect(EOGNativeEffectMechanic::StatModifier); D.Effect.RulePriority.Authority=2;
    if (!F.Install(D)) return false;
    FOGNativeAbilityDefinition Dispel; Dispel.SkillId=EffectTestId(TEXT("dispel")); Dispel.bNoResourceCost=true;
    Dispel.OriginalSourceReference=TEXT("native priority-qualified dispel fixture");
    FOGNativeAbilityEffectOperation Op; Op.Operation=EOGNativeEffectOperation::DispelFamily; Op.Id=D.Effect.FamilyId;
    Op.RemovalPriority.SourceRuleId=EffectTestId(TEXT("remover")); Op.RemovalPriority.Authority=1;
    Dispel.Effects={Op}; if (!F.Battle.RegisterNativeAbility(Dispel,F.Error)) return false;
    const auto Before=F.Battle.GetState();
    FOGResolvedCombatAction Action; Action.ActionId=FOGEntityId::NewId(); Action.SourceUnitId=F.A;
    Action.SkillId=EffectTestId(TEXT("effect")); Action.TargetUnitIds={F.B}; Action.ActionDelay=100;
    if (!F.Battle.ApplyResolvedAction(Action,F.Error)) return false;
    FOGWorldEvent Receipt;
    if (!TestTrue(TEXT("Status-only action has a durable receipt"),FOGCombatActionJournal::BuildResolvedActionEvent(
        Before,F.Battle.GetState(),Action,FOGEntityId::NewId(),0,Receipt,F.Error))) return false;
    TestTrue(TEXT("Receipt retains exact effect provenance"),Receipt.PayloadJson.Contains(TEXT("effects_after")) && Receipt.PayloadJson.Contains(D.Effect.EffectId.ToString()));
    if (!F.Act(F.A,F.B,TEXT("dispel"))) return false;
    TestTrue(TEXT("Lower authority cannot erase effect"),F.Battle.HasNativeEffect(F.B,D.Effect.EffectId));
    D.Effect.RulePriority.Authority=0;
    TestFalse(TEXT("An admitted effect cannot change to weaken dispel protection"),F.Battle.RegisterNativeEffect(D,F.Error));
    return true;
}
#endif
