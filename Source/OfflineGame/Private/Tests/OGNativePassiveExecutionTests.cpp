#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/OGTurnBattle.h"
#include "Combat/OGCombatActionJournal.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
namespace
{
FOGCombatUnitState PassiveUnit(int32 Team)
{
    FOGCombatUnitState U; U.UnitEntityId=FOGEntityId::NewId();
    U.IdentityId=FOGContentId(Team?TEXT("test:passive.enemy"):TEXT("test:passive.player"));
    U.SkillSet.CurrentForm=FOGContentId(TEXT("test:passive.version")); U.TeamIndex=Team;
    U.Presence=EOGCombatPresence::Active; U.Stats.MaxHp=FOGLargeNumber::FromInt64(1000);
    U.CurrentHp=U.Stats.MaxHp; U.Stats.Attack=FOGLargeNumber::FromInt64(100); U.Stats.HitBps=10000;
    U.DefaultActionDelay=100; U.NextActionValue=Team?10000:0; return U;
}
FOGTurnBattleState PassiveState(const FOGCombatUnitState& A,const FOGCombatUnitState& B)
{
    FOGTurnBattleState S; S.BattleId=FOGEntityId::NewId(); S.Units={A,B};
    for (const auto& U:S.Units)
    { FOGTurnTeamState T; T.TeamIndex=U.TeamIndex; FOGTurnSuccessionLane L; L.LaneIndex=0; L.OrderedUnitIds={U.UnitEntityId}; T.Lanes={L}; S.Teams.Add(T); }
    return S;
}
FOGNativeAbilityDefinition PassiveDamage(const FOGContentId& Id,int64 Amount)
{
    FOGNativeAbilityDefinition D; D.SkillId=Id; D.OriginalSourceReference=TEXT("native passive integration test fixture");
    D.bNoResourceCost=true; FOGNativeAbilityPacket P; P.FlatAmount=FOGLargeNumber::FromInt64(Amount);
    P.DefenseReference=EOGNativeDefenseReference::Attack; D.Packets={P}; return D;
}
FOGNativePassiveBinding Counter(const FOGCombatUnitState& Owner,const FOGContentId& Action)
{
    FOGNativePassiveBinding B; B.OwnerUnitId=Owner.UnitEntityId; auto& S=B.Specification;
    S.PassiveSkillId=Owner.SkillSet.PassiveSkills[0]; S.OriginalSourceReference=TEXT("native passive integration test fixture");
    FOGNativePassiveTrigger R; R.Trigger.TriggerId=FOGContentId(S.PassiveSkillId.Value+TEXT(".trigger"));
    R.Trigger.EventType=TEXT("OnDamageReceived"); R.Trigger.ActionIds={Action}; R.MaximumActivationsPerRoot=1;
    R.Target=EOGNativeReactionTarget::EventSource; R.Unavailable=EOGNativeReactionUnavailable::Skip;
    FOGConditionDefinition C; C.Predicate=TEXT("owner_is_target"); R.Trigger.Conditions={C}; S.Triggers={R}; return B;
}
FOGResolvedCombatAction Strike(const FOGCombatUnitState& A,const FOGCombatUnitState& B)
{
    FOGResolvedCombatAction R; R.ActionId=FOGEntityId::NewId(); R.SourceUnitId=A.UnitEntityId;
    R.TargetUnitIds={B.UnitEntityId}; R.SkillId=A.SkillSet.ActiveSkills[0]; R.ActionDelay=A.DefaultActionDelay; return R;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativePassiveCounterTest,
    "OfflineGame.Combat.Passives.CounterExecutesWithOwnedQueueProof",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativePassiveCounterTest::RunTest(const FString&)
{
    auto A=PassiveUnit(0),B=PassiveUnit(1); A.SkillSet.ActiveSkills={FOGContentId(TEXT("test:root.strike"))};
    B.SkillSet.PassiveSkills={FOGContentId(TEXT("test:enemy.counter"))};
    const FOGContentId Child(TEXT("test:enemy.counter_action")); FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(PassiveState(A,B),{}, {},{}, {},{Counter(B,Child)},Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    if (!Battle.RegisterNativeAbility(PassiveDamage(A.SkillSet.ActiveSkills[0],100),Error) ||
        !Battle.RegisterNativeAbility(PassiveDamage(Child,50),Error)) return false;
    auto Forged=Strike(A,B); Forged.SourceUnitId=B.UnitEntityId; Forged.TargetUnitIds={A.UnitEntityId};
    Forged.SkillId=Child; Forged.InterruptPriority=999;
    TestFalse(TEXT("A command cannot forge permission to cast a passive action"),Battle.ApplyResolvedAction(Forged,Error));
    TestTrue(TEXT("Real root executes its automatic counter"),Battle.ApplyResolvedAction(Strike(A,B),Error));
    TestTrue(TEXT("Root applies actual enemy damage"),Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(900));
    TestTrue(TEXT("Counter applies actual source damage"),Battle.GetState().Units[0].CurrentHp==FOGLargeNumber::FromInt64(950));
    TestEqual(TEXT("Counter does not spend its owner's ordinary turn"),Battle.GetState().Units[1].NextActionValue,int64(10000));
    TestEqual(TEXT("Counter has one accepted causal receipt"),Battle.GetLastNativeReactions().Num(),1);
    TestTrue(TEXT("Receipt is an executed reaction"),Battle.GetLastNativeReactions()[0].bExecuted);
    TestEqual(TEXT("Actual damaged subject remains attached"),Battle.GetLastNativeReactions()[0].EventTargetUnitId,B.UnitEntityId);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativePassiveChainTest,
    "OfflineGame.Combat.Passives.MutualCountersRespectAuthoredRootLimits",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativePassiveChainTest::RunTest(const FString&)
{
    auto A=PassiveUnit(0),B=PassiveUnit(1); A.SkillSet.ActiveSkills={FOGContentId(TEXT("test:root.strike"))};
    A.SkillSet.PassiveSkills={FOGContentId(TEXT("test:a.counter"))}; B.SkillSet.PassiveSkills={FOGContentId(TEXT("test:b.counter"))};
    const FOGContentId ChildA(TEXT("test:a.counter_action")),ChildB(TEXT("test:b.counter_action"));
    FOGTurnBattle Battle; FString Error;
    auto Left=Counter(A,ChildA),Right=Counter(B,ChildB);
    Left.Specification.Triggers[0].bAllowReactionOrigin=Right.Specification.Triggers[0].bAllowReactionOrigin=true;
    if (!Battle.Initialize(PassiveState(A,B),{}, {},{}, {},{Left,Right},Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    if (!Battle.RegisterNativeAbility(PassiveDamage(A.SkillSet.ActiveSkills[0],100),Error) ||
        !Battle.RegisterNativeAbility(PassiveDamage(ChildA,30),Error) || !Battle.RegisterNativeAbility(PassiveDamage(ChildB,50),Error)) return false;
    TestTrue(TEXT("Mutual counter chain terminates and commits"),Battle.ApplyResolvedAction(Strike(A,B),Error));
    TestTrue(TEXT("Player receives the one allowed enemy counter"),Battle.GetState().Units[0].CurrentHp==FOGLargeNumber::FromInt64(950));
    TestTrue(TEXT("Enemy receives the root and one return counter"),Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(870));
    TestEqual(TEXT("Accepted and capped outcomes remain factual"),Battle.GetLastNativeReactions().Num(),3);
    TestFalse(TEXT("Excess activation is recorded as skipped"),Battle.GetLastNativeReactions().Last().bExecuted);
    TestEqual(TEXT("Skip reason is the source-authored root limit"),Battle.GetLastNativeReactions().Last().Outcome,FString(TEXT("authored_root_activation_limit")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativePassiveRollbackTest,
    "OfflineGame.Combat.Passives.RequiredReactionFailureRollsBackEntireRoot",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativePassiveRollbackTest::RunTest(const FString&)
{
    auto A=PassiveUnit(0),B=PassiveUnit(1); A.SkillSet.ActiveSkills={FOGContentId(TEXT("test:root.strike"))};
    B.SkillSet.PassiveSkills={FOGContentId(TEXT("test:required.counter"))};
    const FOGContentId Child(TEXT("test:required.counter_action")); auto Passive=Counter(B,Child);
    Passive.Specification.Triggers[0].Unavailable=EOGNativeReactionUnavailable::RejectRootAction;
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(PassiveState(A,B),{}, {},{}, {},{Passive},Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    FOGCombatResourceSpecification Meter; Meter.ResourceId=FOGContentId(TEXT("test:empty.meter"));
    Meter.DisplayName=FText::FromString(TEXT("Test meter")); Meter.Maximum=1; Meter.OpeningValue=0;
    if (!Battle.ConfigureNativeResource(B.UnitEntityId,Meter,Error)) return false;
    auto Ability=PassiveDamage(Child,50); Ability.bNoResourceCost=false;
    FOGNativeAbilityResourceAmount Cost; Cost.ResourceId=Meter.ResourceId; Cost.Amount=1; Ability.ResourceCosts={Cost};
    if (!Battle.RegisterNativeAbility(PassiveDamage(A.SkillSet.ActiveSkills[0],100),Error) ||
        !Battle.RegisterNativeAbility(Ability,Error)) return false;
    const auto Sequence=Battle.GetLog().GetNextSequence();
    TestFalse(TEXT("Required unaffordable reaction rejects the root"),Battle.ApplyResolvedAction(Strike(A,B),Error));
    TestTrue(TEXT("Enemy HP rolls back"),Battle.GetState().Units[1].CurrentHp==B.CurrentHp);
    TestTrue(TEXT("Source HP rolls back"),Battle.GetState().Units[0].CurrentHp==A.CurrentHp);
    TestEqual(TEXT("Root timeline rolls back"),Battle.GetState().Units[0].NextActionValue,A.NextActionValue);
    TestEqual(TEXT("Root and reaction log rolls back"),Battle.GetLog().GetNextSequence(),Sequence);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativePassiveOpeningTest,
    "OfflineGame.Combat.Passives.OpeningReactionUsesAdmittedResourcesAndJournal",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativePassiveOpeningTest::RunTest(const FString&)
{
    auto A=PassiveUnit(0),B=PassiveUnit(1); A.SkillSet.PassiveSkills={FOGContentId(TEXT("test:opening.passive"))};
    const FOGContentId Child(TEXT("test:opening.resource_action")); auto Passive=Counter(A,Child);
    auto& R=Passive.Specification.Triggers[0]; R.Trigger.EventType=TEXT("OnBattleStart"); R.Trigger.Conditions.Reset();
    R.Target=EOGNativeReactionTarget::Owner; R.Unavailable=EOGNativeReactionUnavailable::RejectRootAction;
    const auto Initial=PassiveState(A,B); FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(Initial,{}, {},{}, {},{Passive},Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    FOGCombatResourceSpecification Meter; Meter.ResourceId=FOGContentId(TEXT("test:opening.meter"));
    Meter.DisplayName=FText::FromString(TEXT("Opening meter")); Meter.Maximum=10; Meter.OpeningValue=0;
    if (!Battle.ConfigureNativeResource(A.UnitEntityId,Meter,Error)) return false;
    auto Ability=PassiveDamage(Child,1); Ability.Packets.Reset(); Ability.Targets=EOGNativeAbilityTargets::Self;
    FOGNativeAbilityResourceAmount Gain; Gain.ResourceId=Meter.ResourceId; Gain.Amount=2; Ability.ResourceChanges={Gain};
    if (!Battle.RegisterNativeAbility(Ability,Error)) return false;
    TestTrue(TEXT("Opening reaction resolves after library admission"),Battle.ResolveNativeOpeningReactions(Error));
    TestEqual(TEXT("Opening resource gain is real"),Battle.GetState().Units[0].BattleResources[0].Value,int64(2));
    FOGWorldEvent Event; const auto Ruler=FOGEntityId::NewId();
    TestTrue(TEXT("Opening receipt includes real reaction and state"),FOGCombatActionJournal::BuildEncounterStartedEvent(
        Initial,Battle.GetState(),Battle.GetLastNativeReactions(),Ruler,10,Event,Error));
    TestTrue(TEXT("Opening has an independent valid causal receipt ID"),Event.EventId.IsValid() && Event.EventId!=Initial.BattleId);
    TestTrue(TEXT("Receipt records the actual resource result"),Event.PayloadJson.Contains(TEXT("value_exact")) && Event.PayloadJson.Contains(Child.ToString()));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeNoRecursiveProcsTest,
    "OfflineGame.Combat.Passives.DefaultDoesNotProcFromPassiveFollowups",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeNoRecursiveProcsTest::RunTest(const FString&)
{
    auto A=PassiveUnit(0),B=PassiveUnit(1); A.SkillSet.ActiveSkills={FOGContentId(TEXT("test:root.strike"))};
    A.SkillSet.PassiveSkills={FOGContentId(TEXT("test:a.counter"))}; B.SkillSet.PassiveSkills={FOGContentId(TEXT("test:b.counter"))};
    const FOGContentId ChildA(TEXT("test:a.counter_action")),ChildB(TEXT("test:b.counter_action"));
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(PassiveState(A,B),{}, {},{}, {},{Counter(A,ChildA),Counter(B,ChildB)},Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    if (!Battle.RegisterNativeAbility(PassiveDamage(A.SkillSet.ActiveSkills[0],100),Error) ||
        !Battle.RegisterNativeAbility(PassiveDamage(ChildA,30),Error) || !Battle.RegisterNativeAbility(PassiveDamage(ChildB,50),Error) ||
        !Battle.ApplyResolvedAction(Strike(A,B),Error)) return false;
    TestTrue(TEXT("Root and first counter apply real damage"),Battle.GetState().Units[0].CurrentHp==FOGLargeNumber::FromInt64(950));
    TestTrue(TEXT("A passive hit cannot activate the opposing passive by default"),Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(900));
    TestEqual(TEXT("Only the real first reaction is emitted"),Battle.GetLastNativeReactions().Num(),1); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeOwnerBoundTest,
    "OfflineGame.Combat.Passives.OwnerBoundSurvivesOtherActorsAndResetsOnOwnerAction",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeOwnerBoundTest::RunTest(const FString&)
{
    auto A=PassiveUnit(0),B=PassiveUnit(1);
    A.SkillSet.ActiveSkills={FOGContentId(TEXT("test:root.strike"))};
    B.SkillSet.ActiveSkills={FOGContentId(TEXT("test:enemy.strike"))};
    B.SkillSet.PassiveSkills={FOGContentId(TEXT("test:enemy.counter"))}; B.NextActionValue=150;
    const FOGContentId Child(TEXT("test:enemy.counter_action")); auto Passive=Counter(B,Child);
    Passive.Specification.Triggers[0].MaximumActivationsPerOwnerAction=1;
    FOGTurnBattle Battle; FString Error;
    if (!Battle.Initialize(PassiveState(A,B),{}, {},{}, {},{Passive},Error)) return false;
    Battle.RequireNativeActionExecutors(true);
    if (!Battle.RegisterNativeAbility(PassiveDamage(A.SkillSet.ActiveSkills[0],100),Error) ||
        !Battle.RegisterNativeAbility(PassiveDamage(B.SkillSet.ActiveSkills[0],1),Error) ||
        !Battle.RegisterNativeAbility(PassiveDamage(Child,50),Error) ||
        !Battle.ApplyResolvedAction(Strike(A,B),Error) || !Battle.ApplyResolvedAction(Strike(A,B),Error)) return false;
    TestTrue(TEXT("Other actors' root actions do not reset this owner's bound"),
        Battle.GetState().Units[0].CurrentHp==FOGLargeNumber::FromInt64(950));
    if (!Battle.ApplyResolvedAction(Strike(B,A),Error) || !Battle.ApplyResolvedAction(Strike(A,B),Error)) return false;
    TestTrue(TEXT("The next real owner action renews the source-authored bound"),
        Battle.GetState().Units[0].CurrentHp==FOGLargeNumber::FromInt64(899));
    return true;
}
#endif
