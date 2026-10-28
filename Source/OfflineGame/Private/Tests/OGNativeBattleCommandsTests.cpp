#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/OGNativeBattleCommands.h"
#include "Combat/OGApprovedLuminousSourceKit.h"
#include "Combat/OGLuminousAlterV1Projection.h"
#include "Combat/OGTurnBattle.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/AutomationTest.h"

namespace
{
FOGTurnTeamState CommandTeam(int32 Index,const FOGEntityId& Unit)
{
    FOGTurnTeamState Team; Team.TeamIndex=Index;
    FOGTurnSuccessionLane Lane; Lane.LaneIndex=0; Lane.OrderedUnitIds.Add(Unit);
    Team.Lanes.Add(Lane); return Team;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeBattleCommandModesTest,
    "OfflineGame.Combat.NativeCommands.AuthoredBranchesAndPreparation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeBattleCommandModesTest::RunTest(const FString& Parameters)
{
    FOGApprovedLuminousSourceKitView Base; FString Error;
    if (!TestTrue(TEXT("Verified Base source loads"),FOGApprovedLuminousSourceKit::LoadBaseV6(Base,Error))) return false;
    FOGCombatUnitState Source; Source.UnitEntityId=FOGEntityId::NewId();
    Source.IdentityId=Base.IdentityId; Source.SkillSet=Base.CoreSkills; Source.DefaultActionDelay=173;
    FOGNativeBattleCommandSet Set;
    if (!TestTrue(TEXT("Exact source kit exposes native commands"),FOGNativeBattleCommands::BuildFor(Source,Set,Error))) return false;
    TestEqual(TEXT("Base has three actives, covenant, three ultimate choices and Fantasm"),Set.Commands.Num(),8);
    const FOGEntityId Ally=FOGEntityId::NewId();
    const auto* Preparation=Set.Commands.FindByPredicate([](const FOGNativeBattleCommand& C){return C.Id==TEXT("revival_prepare");});
    if (!TestNotNull(TEXT("Day preparation is reachable"),Preparation)) return false;
    FOGResolvedCombatAction Action; Preparation->BuildAction(Source,{Ally},Action);
    TSharedPtr<FJsonObject> Context;
    if (!TestTrue(TEXT("Command generates valid JSON"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Action.ResolutionJson),Context))) return false;
    TestEqual(TEXT("Prepared revival uses Day"),Context->GetStringField(TEXT("branch")),FString(TEXT("day")));
    TestEqual(TEXT("Exact preparation mode reaches existing executor"),Context->GetStringField(TEXT("mode")),FString(TEXT("revival_prepare")));
    TestTrue(TEXT("Preview does not mint a journal action identity"),!Action.ActionId.IsValid());
    TestEqual(TEXT("Actual source action delay is retained"),Action.ActionDelay,int64(173));
    const auto* Covenant=Set.Commands.FindByPredicate([](const FOGNativeBattleCommand& C){return C.Id==TEXT("covenant");});
    if (!TestNotNull(TEXT("Covenant is reachable"),Covenant)) return false;
    Covenant->BuildAction(Source,{Ally},Action);
    TestEqual(TEXT("Covenant uses the genuine A3 ID"),Action.SkillId,Base.CoreSkills.ActiveSkills[2]);
    TestTrue(TEXT("Covenant retains the selected living ally"),Action.TargetUnitIds.Num()==1 && Action.TargetUnitIds[0]==Ally);
    const auto* Field=Set.Commands.FindByPredicate([](const FOGNativeBattleCommand& C){return C.bFantasm;});
    if (!TestNotNull(TEXT("Genuine World Fantasm is reachable"),Field)) return false;
    Field->BuildAction(Source,{Ally},Action);
    TestEqual(TEXT("Fantasm uses its separate source ID"),Action.SkillId,Base.WorldFantasmId);
    TestTrue(TEXT("Field activation does not borrow a stale target"),Action.TargetUnitIds.Num()==1 && Action.TargetUnitIds[0]==Source.UnitEntityId);
    Source.SkillSet.PassiveSkills.Pop();
    TestFalse(TEXT("An incomplete kit cannot borrow these commands"),FOGNativeBattleCommands::BuildFor(Source,Set,Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeBattleAlterSelfTargetTest,
    "OfflineGame.Combat.NativeCommands.AlterSelfTargetAndBranch",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeBattleAlterSelfTargetTest::RunTest(const FString& Parameters)
{
    FOGLuminousAlterV1Projection Kit; FString Error;
    if (!TestTrue(TEXT("Verified Alter source loads"),FOGLuminousAlterV1Projection::LoadApprovedV1(Kit,Error))) return false;
    FOGCombatUnitState Source; Source.UnitEntityId=FOGEntityId::NewId();
    Source.IdentityId=FOGContentId(TEXT("character:luminous_valentine.identity")); Source.SkillSet=Kit.Skills;
    FOGNativeBattleCommandSet Set;
    if (!TestTrue(TEXT("Exact Alter kit exposes native commands"),FOGNativeBattleCommands::BuildFor(Source,Set,Error))) return false;
    TestEqual(TEXT("Alter has three actives, two branches and Fantasm"),Set.Commands.Num(),6);
    const auto* Self=Set.Commands.FindByPredicate([](const FOGNativeBattleCommand& C){return C.Id==TEXT("active_3");});
    if (!TestNotNull(TEXT("Actual self skill exists"),Self)) return false;
    FOGResolvedCombatAction Action; Self->BuildAction(Source,{FOGEntityId::NewId()},Action);
    TestTrue(TEXT("Self skill cannot accidentally target the selected opponent"),
        Action.TargetUnitIds.Num()==1 && Action.TargetUnitIds[0]==Source.UnitEntityId);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeBattleReadinessTest,
    "OfflineGame.Combat.NativeCommands.ReadinessDoesNotConsumeTurnOrResources",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeBattleReadinessTest::RunTest(const FString& Parameters)
{
    FOGApprovedLuminousSourceKitView Kit; FString Error;
    if (!FOGApprovedLuminousSourceKit::LoadBaseV6(Kit,Error)) { AddError(Error); return false; }
    FOGCombatUnitState Source; Source.UnitEntityId=FOGEntityId::NewId(); Source.IdentityId=Kit.IdentityId;
    Source.SkillSet=Kit.CoreSkills; Source.Stats=Kit.RankOneHeroStats; Source.CurrentHp=Source.Stats.MaxHp;
    Source.TeamIndex=0; Source.Presence=EOGCombatPresence::Active; Source.NextActionValue=0;
    FOGCombatUnitState Enemy; Enemy.UnitEntityId=FOGEntityId::NewId(); Enemy.IdentityId=FOGContentId(TEXT("test:commands.enemy"));
    Enemy.Stats.MaxHp=FOGLargeNumber::FromInt64(100000); Enemy.CurrentHp=Enemy.Stats.MaxHp;
    Enemy.TeamIndex=1; Enemy.Presence=EOGCombatPresence::Active; Enemy.NextActionValue=100;
    FOGTurnBattleState Initial; Initial.BattleId=FOGEntityId::NewId();
    Initial.Units={Source,Enemy}; Initial.Teams={CommandTeam(0,Source.UnitEntityId),CommandTeam(1,Enemy.UnitEntityId)};
    FOGTurnBattle Battle;
    if (!TestTrue(TEXT("Native battle initializes"),Battle.Initialize(Initial,Error))) return false;
    FOGNativeBattleCommandSet Set; if (!FOGNativeBattleCommands::BuildFor(Source,Set,Error)) return false;
    const auto* Basic=Set.Commands.FindByPredicate([](const FOGNativeBattleCommand& C){return C.Id==TEXT("active_1");});
    FOGResolvedCombatAction Proposal,Expanded; Basic->BuildAction(Source,{Enemy.UnitEntityId},Proposal);
    const int64 Sequence=Battle.GetLog().GetNextSequence(),Timeline=Battle.GetState().CurrentActionValue;
    int32 Reserve=0,Charge=0,Actions=0; Battle.ReadLuminousCommandResources(Source.UnitEntityId,Reserve,Charge,Actions);
    for (int32 I=0;I<3;++I) TestTrue(TEXT("Readiness accepts the legal next action"),Battle.ValidateResolvedAction(Proposal,Expanded,Error));
    int32 After=0,AfterCharge=0,AfterActions=0; Battle.ReadLuminousCommandResources(Source.UnitEntityId,After,AfterCharge,AfterActions);
    TestEqual(TEXT("Readiness does not append combat events"),Battle.GetLog().GetNextSequence(),Sequence);
    TestEqual(TEXT("Readiness does not advance timeline"),Battle.GetState().CurrentActionValue,Timeline);
    TestEqual(TEXT("Readiness does not spend Reserve"),After,Reserve);
    TestEqual(TEXT("Readiness does not advance owner actions"),AfterActions,Actions);
    Proposal.SourceUnitId=Enemy.UnitEntityId;
    TestFalse(TEXT("Readiness rejects out-of-turn commands"),Battle.ValidateResolvedAction(Proposal,Expanded,Error));
    return true;
}
#endif
