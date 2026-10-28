#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/OGNativeCombatAbility.h"
#include "Combat/OGTurnBattle.h"
#include "Misc/AutomationTest.h"

namespace
{
FOGCombatUnitState NativeTestUnit(int32 Team,const TCHAR* Identity,int64 Hp=1000)
{
    FOGCombatUnitState U; U.UnitEntityId=FOGEntityId::NewId(); U.IdentityId=FOGContentId(Identity);
    U.SkillSet.CurrentForm=FOGContentId(TEXT("test:native.version")); U.TeamIndex=Team;
    U.Presence=EOGCombatPresence::Active; U.CurrentHp=FOGLargeNumber::FromInt64(Hp);
    U.Stats.MaxHp=FOGLargeNumber::FromInt64(1000); U.Stats.Attack=FOGLargeNumber::FromInt64(100);
    U.Stats.HitBps=10000; U.Stats.CritRateBps=0; U.Stats.Defense=FOGLargeNumber();
    U.NextActionValue=Team?10000:0; U.DefaultActionDelay=100;
    return U;
}
FOGTurnTeamState NativeTestTeam(int32 Index,const TArray<FOGEntityId>& Ids)
{
    FOGTurnTeamState T; T.TeamIndex=Index;
    for (int32 I=0;I<Ids.Num();++I) { FOGTurnSuccessionLane L; L.LaneIndex=I; L.OrderedUnitIds.Add(Ids[I]); T.Lanes.Add(L); }
    return T;
}
FOGNativeAbilityDefinition NativeStrike(const FOGCombatUnitState& Source,EOGNativeAbilityTargets Targets=EOGNativeAbilityTargets::SingleEnemy)
{
    FOGNativeAbilityDefinition D; D.IdentityId=Source.IdentityId; D.VersionId=Source.SkillSet.CurrentForm;
    D.SkillId=Source.SkillSet.ActiveSkills[0]; D.OriginalSourceReference=TEXT("test-only exact authored fixture"); D.Targets=Targets;
    FOGNativeAbilityPacket P; P.DefenseReference=EOGNativeDefenseReference::Attack; P.Repeats=2;
    FOGNativeAbilityScale S; S.Stat=EOGNativeScaleStat::Attack; S.MultiplierBps=10000; P.Scaling.Add(S); D.Packets.Add(P);
    return D;
}
FOGTurnBattleState NativeInitial(const FOGCombatUnitState& Source,const TArray<FOGCombatUnitState>& Enemies)
{
    FOGTurnBattleState S; S.BattleId=FOGEntityId::NewId(); S.Units.Add(Source); S.Units.Append(Enemies);
    TArray<FOGEntityId> Ids; for (const auto& U:Enemies) Ids.Add(U.UnitEntityId);
    S.Teams={NativeTestTeam(0,{Source.UnitEntityId}),NativeTestTeam(1,Ids)}; return S;
}
FOGResolvedCombatAction NativeAction(const FOGCombatUnitState& Source,const FOGEntityId& Target)
{
    FOGResolvedCombatAction A; A.ActionId=FOGEntityId::NewId(); A.SourceUnitId=Source.UnitEntityId;
    A.SkillId=Source.SkillSet.ActiveSkills[0]; A.TargetUnitIds={Target}; A.ActionDelay=Source.DefaultActionDelay; return A;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAbilityExecutionTest,
    "OfflineGame.Combat.NativeAbility.RealDamageAndMissingExecutor",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeAbilityExecutionTest::RunTest(const FString& Parameters)
{
    auto Source=NativeTestUnit(0,TEXT("test:native.actor"));
    Source.SkillSet.ActiveSkills.Add(FOGContentId(TEXT("test:native.strike")));
    const auto Enemy=NativeTestUnit(1,TEXT("test:native.enemy"));
    FOGTurnBattle Battle; FString Error;
    if (!TestTrue(TEXT("Initializes actual combat state"),Battle.Initialize(NativeInitial(Source,{Enemy}),Error))) return false;
    Battle.RequireNativeActionExecutors(true);
    const auto Sequence=Battle.GetLog().GetNextSequence();
    const auto Action=NativeAction(Source,Enemy.UnitEntityId);
    TestFalse(TEXT("Unimplemented ability is never accepted as an empty action"),Battle.ApplyResolvedAction(Action,Error));
    TestEqual(TEXT("Missing executor consumes no log or turn"),Battle.GetLog().GetNextSequence(),Sequence);
    TestEqual(TEXT("Missing executor preserves timeline"),Battle.GetState().CurrentActionValue,int64(0));
    auto Definition=NativeStrike(Source);
    if (!TestTrue(TEXT("Registers exact complete typed ability"),Battle.RegisterNativeAbility(Definition,Error))) return false;
    const auto Revision=Battle.GetNativeAbilityRevision();
    TestTrue(TEXT("Identical admission is idempotent"),Battle.RegisterNativeAbility(Definition,Error));
    TestEqual(TEXT("Idempotent admission preserves revision"),Battle.GetNativeAbilityRevision(),Revision);
    auto Conflict=Definition; Conflict.Packets[0].Scaling[0].MultiplierBps=20000;
    TestFalse(TEXT("Mid-encounter content cannot overwrite an admitted ability"),Battle.RegisterNativeAbility(Conflict,Error));
    if (!TestTrue(TEXT("Accepted attack resolves actual HP"),Battle.ApplyResolvedAction(Action,Error))) return false;
    TestTrue(TEXT("Two independently resolved 100-ATK hits remove 200 HP"),
        FOGLargeNumber::Compare(Battle.GetState().Units[1].CurrentHp,FOGLargeNumber::FromInt64(800))==0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAbilityAreaRollbackTest,
    "OfflineGame.Combat.NativeAbility.AllTargetsAndAtomicRollback",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeAbilityAreaRollbackTest::RunTest(const FString& Parameters)
{
    auto Source=NativeTestUnit(0,TEXT("test:native.actor")); Source.SkillSet.ActiveSkills.Add(FOGContentId(TEXT("test:native.area")));
    const auto First=NativeTestUnit(1,TEXT("test:native.enemy_a")),Second=NativeTestUnit(1,TEXT("test:native.enemy_b"));
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{First,Second}),Error)) { AddError(Error); return false; }
    Battle.RequireNativeActionExecutors(true);
    if (!Battle.RegisterNativeAbility(NativeStrike(Source,EOGNativeAbilityTargets::AllEnemies),Error)) return false;
    auto Action=NativeAction(Source,First.UnitEntityId); FOGResolvedCombatAction Expanded; bool Area=false;
    if (!TestTrue(TEXT("Field expands"),Battle.ExpandApprovedFieldTargets(Action,Expanded,Area,Error))) return false;
    TestTrue(TEXT("Manual subset cannot omit the second enemy"),Area && Expanded.TargetUnitIds.Num()==2 && Expanded.TargetUnitIds.Contains(Second.UnitEntityId));
    if (!TestTrue(TEXT("Field damages the full roster"),Battle.ApplyResolvedAction(Action,Error))) return false;
    for (int32 I=1;I<3;++I) TestTrue(TEXT("Every eligible enemy loses genuine HP"),
        FOGLargeNumber::Compare(Battle.GetState().Units[I].CurrentHp,FOGLargeNumber::FromInt64(800))==0);

    Source.RankProjection.RankId=FOGContentId(TEXT("test:rank.source"));
    auto Ranked=Second; Ranked.RankProjection.RankId=FOGContentId(TEXT("test:rank.target"));
    FOGTurnBattle Rollback; if (!Rollback.Initialize(NativeInitial(Source,{First,Ranked}),Error)) return false;
    Rollback.RequireNativeActionExecutors(true);
    if (!Rollback.RegisterNativeAbility(NativeStrike(Source,EOGNativeAbilityTargets::AllEnemies),Error)) return false;
    const auto BeforeSequence=Rollback.GetLog().GetNextSequence();
    TestFalse(TEXT("Missing real Rank resolver rejects the later packet target"),Rollback.ApplyResolvedAction(NativeAction(Source,First.UnitEntityId),Error));
    TestEqual(TEXT("Rejected multi-target action leaves no events"),Rollback.GetLog().GetNextSequence(),BeforeSequence);
    for (int32 I=1;I<3;++I) TestTrue(TEXT("Earlier target damage rolls back with the failed action"),
        FOGLargeNumber::Compare(Rollback.GetState().Units[I].CurrentHp,FOGLargeNumber::FromInt64(1000))==0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAbilityHealingTest,
    "OfflineGame.Combat.NativeAbility.HealingAndDefinitionBoundaries",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeAbilityHealingTest::RunTest(const FString& Parameters)
{
    auto Source=NativeTestUnit(0,TEXT("test:native.healer"),100);
    Source.SkillSet.ActiveSkills.Add(FOGContentId(TEXT("test:native.restore")));
    const auto Enemy=NativeTestUnit(1,TEXT("test:native.enemy"));
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(NativeInitial(Source,{Enemy}),Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    auto D=NativeStrike(Source,EOGNativeAbilityTargets::Self);
    D.Packets[0].Effect=EOGNativeAbilityEffect::Healing; D.Packets[0].DefenseReference=EOGNativeDefenseReference::Unspecified;
    D.Packets[0].Repeats=1; D.Packets[0].Scaling[0].MultiplierBps=5000;
    if (!TestTrue(TEXT("Self healing admits its actual packet"),Battle.RegisterNativeAbility(D,Error))) return false;
    auto Action=NativeAction(Source,Enemy.UnitEntityId); Action.ResolutionJson=TEXT("{\"amount\":999999}");
    TestFalse(TEXT("Command JSON cannot replace authored numbers"),Battle.ApplyResolvedAction(Action,Error));
    Action.ResolutionJson=TEXT("{}");
    if (!TestTrue(TEXT("Self target expansion ignores stale opponent focus"),Battle.ApplyResolvedAction(Action,Error))) return false;
    TestTrue(TEXT("Healing changes genuine caster HP"),
        FOGLargeNumber::Compare(Battle.GetState().Units[0].CurrentHp,FOGLargeNumber::FromInt64(150))==0);
    auto Invalid=NativeStrike(Source); Invalid.Packets[0].DefenseReference=EOGNativeDefenseReference::Unspecified;
    TestFalse(TEXT("Physical damage never gets an invented default DEF reference"),Invalid.Validate(Error));
    Invalid=D; Invalid.IdentityId=FOGContentId(TEXT("character:luminous_valentine.identity"));
    TestFalse(TEXT("Ordinary definitions cannot masquerade as complete Luminous kits"),Invalid.Validate(Error));
    auto* Transient=NewObject<UOGNativeCombatAbility>(); Transient->Definition=D;
    TestFalse(TEXT("Transient command objects cannot enter production admission"),Transient->ValidateInstalled(Error));
    return true;
}
#endif
