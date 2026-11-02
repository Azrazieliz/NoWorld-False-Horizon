#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/OGWorldActionRuntime.h"
#include "Combat/OGTurnBattle.h"
#include "Misc/AutomationTest.h"
namespace
{
FOGCombatUnitState CommittedPartyUnit()
{
    FOGCombatUnitState U; U.UnitEntityId=FOGEntityId::NewId(); U.IdentityId=FOGContentId(TEXT("test:party.identity"));
    U.SkillSet.CurrentForm=FOGContentId(TEXT("test:party.version")); U.Stats.MaxHp=FOGLargeNumber::FromInt64(100);
    U.CurrentHp=U.Stats.MaxHp; U.Presence=EOGCombatPresence::Active; return U;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCommittedPartyHpTest,
    "OfflineGame.World.Party.AcceptedBattleHpDefeatAndFallback",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCommittedPartyHpTest::RunTest(const FString&)
{
    auto* Party=NewObject<UOGWorldPartyRuntimeComponent>();
    const auto A=CommittedPartyUnit(); auto B=CommittedPartyUnit(); B.IdentityId=FOGContentId(TEXT("test:party.other"));
    FString Error; if (!Party->ConfigureParty({A,B},Error)) return false;
    FOGTurnBattleState State; State.BattleId=FOGEntityId::NewId(); State.Status=EOGTurnBattleStatus::Running;
    State.Units={A,B}; State.Units[0].CurrentHp=FOGLargeNumber::FromInt64(35);
    const auto First=FOGEntityId::NewId();
    TestTrue(TEXT("Accepted damage projects into the real party"),Party->ApplyCommittedBattleState(State,First,Error));
    TestTrue(TEXT("Party retains actual battle HP"),Party->GetSlots()[0].Unit.CurrentHp==FOGLargeNumber::FromInt64(35));
    State.Units[0].CurrentHp=FOGLargeNumber(); State.Units[0].Presence=EOGCombatPresence::Defeated;
    TestTrue(TEXT("Same receipt is idempotent"),Party->ApplyCommittedBattleState(State,First,Error));
    TestFalse(TEXT("Repeated receipt cannot invent later damage"),Party->GetSlots()[0].bDefeated);
    TestTrue(TEXT("New committed defeat projects"),Party->ApplyCommittedBattleState(State,FOGEntityId::NewId(),Error));
    TestTrue(TEXT("Defeated slot becomes unavailable"),Party->GetSlots()[0].bDefeated && !Party->GetSlots()[0].bAvailable);
    TestEqual(TEXT("Existing living fallback is selected"),Party->GetControlledSlot(),1);
    State.Status=EOGTurnBattleStatus::Completed;
    TestTrue(TEXT("Completed encounter retains its actual World party result"),Party->ApplyCommittedBattleState(State,FOGEntityId::NewId(),Error));
    TestTrue(TEXT("Leaving the turn panel does not restore HP"),Party->GetSlots()[0].Unit.CurrentHp.IsZero());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCommittedPartyIdentityTest,
    "OfflineGame.World.Party.RejectMismatchedSnapshotsAtomically",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCommittedPartyIdentityTest::RunTest(const FString&)
{
    auto* Party=NewObject<UOGWorldPartyRuntimeComponent>();
    const auto A=CommittedPartyUnit(); auto B=CommittedPartyUnit(); B.IdentityId=FOGContentId(TEXT("test:party.other"));
    FString Error; if (!Party->ConfigureParty({A,B},Error)) return false;
    FOGTurnBattleState State; State.BattleId=FOGEntityId::NewId(); State.Status=EOGTurnBattleStatus::Running;
    State.Units={A,B}; State.Units[0].CurrentHp=FOGLargeNumber::FromInt64(1);
    State.Units[1].SkillSet.CurrentForm=FOGContentId(TEXT("test:wrong.version"));
    TestFalse(TEXT("Mismatched Version rejects the snapshot"),Party->ApplyCommittedBattleState(State,FOGEntityId::NewId(),Error));
    TestTrue(TEXT("Earlier matching slot was not partially changed"),Party->GetSlots()[0].Unit.CurrentHp==A.CurrentHp);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCommittedPartyRevivalTest,
    "OfflineGame.World.Party.MirrorRealRevivalWithoutGrantingCrossEncounterHealing",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCommittedPartyRevivalTest::RunTest(const FString&)
{
    auto* Party=NewObject<UOGWorldPartyRuntimeComponent>(); const auto A=CommittedPartyUnit(); FString Error;
    if (!Party->ConfigureParty({A},Error)) return false;
    FOGTurnBattleState State; State.BattleId=FOGEntityId::NewId(); State.Status=EOGTurnBattleStatus::Running; State.Units={A};
    State.Units[0].CurrentHp=FOGLargeNumber(); State.Units[0].Presence=EOGCombatPresence::Defeated;
    if (!Party->ApplyCommittedBattleState(State,FOGEntityId::NewId(),Error)) return false;
    const auto OriginalBattle=State.BattleId; State.BattleId=FOGEntityId::NewId();
    State.Units[0].CurrentHp=FOGLargeNumber::FromInt64(40); State.Units[0].Presence=EOGCombatPresence::Active;
    TestFalse(TEXT("A different encounter cannot grant revival"),Party->ApplyCommittedBattleState(State,FOGEntityId::NewId(),Error));
    State.BattleId=OriginalBattle;
    TestTrue(TEXT("Already accepted native revival in the same encounter is mirrored"),Party->ApplyCommittedBattleState(State,FOGEntityId::NewId(),Error));
    TestTrue(TEXT("Real revived unit becomes available"),Party->GetSlots()[0].bAvailable && !Party->GetSlots()[0].bDefeated);
    return true;
}
#endif
