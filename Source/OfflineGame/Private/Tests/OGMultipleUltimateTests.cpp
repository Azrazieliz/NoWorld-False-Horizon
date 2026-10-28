#if WITH_DEV_AUTOMATION_TESTS
#include "Skills/OGResolvedSkillSet.h"
#include "Combat/OGTurnBattle.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGMultipleUltimateIdentityTest,
    "OfflineGame.Skills.Ultimates.LegacyCompatibilityAndDistinctOwnership",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGMultipleUltimateIdentityTest::RunTest(const FString&)
{
    const FOGContentId One(TEXT("test:ultimate.one")),Two(TEXT("test:ultimate.two")),Three(TEXT("test:ultimate.three"));
    FOGResolvedSkillSet Legacy; Legacy.UltimateSkill=One;
    FOGResolvedSkillSet Shared; Shared.UltimateSkills={One};
    TestTrue(TEXT("One-entry shared and legacy kits match"),Legacy.HasSameUltimates(Shared));
    Shared.UltimateSkills={One,Two,Three};
    TestEqual(TEXT("Three distinct Ultimates are retained"),Shared.GetUltimateSkills().Num(),3);
    TestTrue(TEXT("Third Ultimate has real kit ownership"),Shared.IsUltimate(Three));
    TestFalse(TEXT("Unowned Ultimate remains unavailable"),Shared.IsUltimate(FOGContentId(TEXT("test:ultimate.other"))));
    Shared.UltimateSkill=One;
    TestFalse(TEXT("Primary alias is backwards-compatible"),Shared.HasDuplicateSkills());
    Shared.UltimateSkill=Two;
    TestTrue(TEXT("Alias cannot silently change the declared primary"),Shared.HasDuplicateSkills());
    Shared.UltimateSkill=One; Shared.ActiveSkills={Two};
    TestTrue(TEXT("Cross-role duplicate is rejected"),Shared.HasDuplicateSkills());
    Shared.ActiveSkills.Reset(); Shared.UltimateSkills={One,One};
    TestTrue(TEXT("Duplicate Ultimate is rejected"),Shared.HasDuplicateSkills());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGMultipleUltimateExecutionTest,
    "OfflineGame.Combat.Ultimates.AllAuthoredAbilitiesExecuteIndependently",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGMultipleUltimateExecutionTest::RunTest(const FString&)
{
    FOGCombatUnitState A,B; A.UnitEntityId=FOGEntityId::NewId(); B.UnitEntityId=FOGEntityId::NewId();
    A.IdentityId=FOGContentId(TEXT("test:multi.identity")); B.IdentityId=FOGContentId(TEXT("test:multi.enemy"));
    A.SkillSet.CurrentForm=B.SkillSet.CurrentForm=FOGContentId(TEXT("test:multi.version"));
    A.SkillSet.UltimateSkills={FOGContentId(TEXT("test:multi.first")),FOGContentId(TEXT("test:multi.second")),FOGContentId(TEXT("test:multi.third"))};
    A.TeamIndex=0; B.TeamIndex=1; A.Presence=B.Presence=EOGCombatPresence::Active;
    A.Stats.MaxHp=B.Stats.MaxHp=FOGLargeNumber::FromInt64(1000); A.CurrentHp=B.CurrentHp=A.Stats.MaxHp;
    A.Stats.Attack=B.Stats.Attack=FOGLargeNumber::FromInt64(100); A.Stats.HitBps=B.Stats.HitBps=10000;
    A.DefaultActionDelay=B.DefaultActionDelay=100; B.NextActionValue=10000;
    FOGTurnBattleState Initial; Initial.BattleId=FOGEntityId::NewId(); Initial.Units={A,B};
    for (const auto& Unit:Initial.Units)
    { FOGTurnTeamState Team; Team.TeamIndex=Unit.TeamIndex; FOGTurnSuccessionLane Lane; Lane.LaneIndex=0; Lane.OrderedUnitIds={Unit.UnitEntityId}; Team.Lanes={Lane}; Initial.Teams.Add(Team); }
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(Initial,Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    for (int32 I=0;I<A.SkillSet.UltimateSkills.Num();++I)
    {
        FOGNativeAbilityDefinition D; D.SkillId=A.SkillSet.UltimateSkills[I]; D.bNoResourceCost=true;
        D.OriginalSourceReference=TEXT("multiple-Ultimate native integration fixture"); D.CooldownOwnerActions=1;
        FOGNativeAbilityPacket P; P.FlatAmount=FOGLargeNumber::FromInt64((I+1)*10);
        P.DefenseReference=EOGNativeDefenseReference::Attack; D.Packets={P};
        if (!TestTrue(TEXT("Each distinct Ultimate admits its own executor"),Battle.RegisterNativeAbility(D,Error))) return false;
    }
    for (int32 I=0;I<A.SkillSet.UltimateSkills.Num();++I)
    {
        FOGResolvedCombatAction Action; Action.ActionId=FOGEntityId::NewId(); Action.SourceUnitId=A.UnitEntityId;
        Action.SkillId=A.SkillSet.UltimateSkills[I]; Action.TargetUnitIds={B.UnitEntityId}; Action.ActionDelay=100;
        if (!TestTrue(TEXT("Each declared Ultimate executes"),Battle.ApplyResolvedAction(Action,Error))) return false;
        if (I==0) TestFalse(TEXT("First Ultimate cooldown does not disappear"),Battle.ApplyResolvedAction(Action,Error));
    }
    TestTrue(TEXT("All three actual outcomes are retained"),Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(940));
    return true;
}
#endif
