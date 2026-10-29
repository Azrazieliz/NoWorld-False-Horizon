#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Combat/OGTurnBattle.h"

namespace
{
FOGContentId WorldTestId(const TCHAR* Name) { return FOGContentId(FString(TEXT("test:shared_world."))+Name); }
struct FSharedWorldFixture
{
    FOGTurnBattle Battle; FString Error;
    FOGEntityId A=FOGEntityId::NewId(),B=FOGEntityId::NewId();
    FOGNativeAbilityDefinition Ability;
    FOGWorldAbilitySpecification World;
    bool Open(EOGWorldAbilityCostTiming Cost=EOGWorldAbilityCostTiming::AcceptedStart,bool Refund=false)
    {
        FOGCombatUnitState Source,Target; Source.UnitEntityId=A; Target.UnitEntityId=B;
        Source.IdentityId=WorldTestId(TEXT("source")); Target.IdentityId=WorldTestId(TEXT("target"));
        Source.SkillSet.CurrentForm=Target.SkillSet.CurrentForm=WorldTestId(TEXT("version"));
        Source.SkillSet.ActiveSkills={WorldTestId(TEXT("skill"))};
        Source.TeamIndex=0; Target.TeamIndex=1; Source.Presence=Target.Presence=EOGCombatPresence::Active;
        Source.Stats.MaxHp=Target.Stats.MaxHp=FOGLargeNumber::FromInt64(100);
        Source.CurrentHp=Target.CurrentHp=Source.Stats.MaxHp;
        Source.Stats.Attack=Target.Stats.Attack=FOGLargeNumber::FromInt64(100);
        Source.Stats.HitBps=Target.Stats.HitBps=10000;
        Source.DefaultActionDelay=Target.DefaultActionDelay=100; Source.NextActionValue=10000;
        FOGTurnBattleState State; State.BattleId=FOGEntityId::NewId(); State.Units={Source,Target};
        for (const auto& U:State.Units)
        { FOGTurnTeamState Team; Team.TeamIndex=U.TeamIndex; FOGTurnSuccessionLane Lane; Lane.OrderedUnitIds={U.UnitEntityId}; Team.Lanes={Lane}; State.Teams.Add(Team); }
        if (!Battle.Initialize(State,Error)) return false;
        Battle.RequireNativeActionExecutors(true);
        FOGCombatResourceSpecification Resource; Resource.ResourceId=WorldTestId(TEXT("resource"));
        Resource.DisplayName=FText::FromString(TEXT("Fixture energy")); Resource.Maximum=100; Resource.OpeningValue=20;
        if (!Battle.ConfigureNativeResource(A,Resource,Error)) return false;
        Ability.SkillId=WorldTestId(TEXT("skill")); Ability.OriginalSourceReference=TEXT("native staged World fixture");
        FOGNativeAbilityResourceAmount CostRow; CostRow.ResourceId=Resource.ResourceId; CostRow.Amount=20; Ability.ResourceCosts={CostRow};
        auto Gain=CostRow; Gain.Amount=3; Ability.ResourceChanges={Gain};
        FOGNativeAbilityPacket Packet; Packet.FlatAmount=FOGLargeNumber::FromInt64(10);
        Packet.DefenseReference=EOGNativeDefenseReference::Attack; Packet.Repeats=3; Ability.Packets={Packet};
        if (!Battle.RegisterNativeAbility(Ability,Error)) return false;
        World.SkillId=Ability.SkillId; World.OriginalSourceReference=TEXT("native source timing fixture");
        World.CostTiming=Cost; World.DurationMilliseconds=1000; World.CooldownMilliseconds=1500; World.bRefundCostsOnCancel=Refund;
        for (int32 I=0;I<3;++I)
        {
            FOGWorldAbilityStep Step; Step.OpensAtMilliseconds=100+I*200; Step.ClosesAtMilliseconds=150+I*200;
            Step.RangeCentimeters=1000; FOGNativeAbilityPacketSlice Slice; Slice.PacketIndex=0; Slice.FirstRepeat=I;
            Step.Packets={Slice}; World.Steps.Add(Step);
        }
        return Battle.RegisterWorldAbility(World,Error) && Battle.SetExecutionMode(EOGCombatExecutionMode::World,Error);
    }
    FOGResolvedCombatAction Root()
    { FOGResolvedCombatAction R; R.ActionId=FOGEntityId::NewId(); R.SourceUnitId=A; R.SkillId=Ability.SkillId; R.TargetUnitIds={B}; return R; }
    int64 Energy() const { return Battle.GetState().Units[0].BattleResources[0].Value; }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedWorldStagingTest,
    "OfflineGame.Combat.World.SharedPacketsConsumeOnceAndResolveAtContacts",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedWorldStagingTest::RunTest(const FString&)
{
    FSharedWorldFixture F; if (!TestTrue(TEXT("Open admitted shared World executor"),F.Open())) return false;
    const auto Root=F.Root();
    if (!TestTrue(TEXT("World cast ignores turn order while preserving ownership"),F.Battle.BeginWorldAbility(Root,F.Error))) return false;
    TestEqual(TEXT("Accepted-start cost paid exactly once"),F.Energy(),static_cast<int64>(0));
    TestTrue(TEXT("Cast start does not invent a hit"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(100));
    TestFalse(TEXT("Impact cannot precede its authored interval"),F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{F.B},F.Error));
    for (int32 I=0;I<3;++I)
    {
        if (!F.Battle.AdvanceWorldCombatClock(100+I*200,F.Error) ||
            !TestTrue(TEXT("Actual contact resolves its original repeat"),F.Battle.ApplyWorldAbilityStep(Root.ActionId,I,{F.B},F.Error))) return false;
        TestFalse(TEXT("Same step cannot deal damage twice"),F.Battle.ApplyWorldAbilityStep(Root.ActionId,I,{F.B},F.Error));
    }
    TestTrue(TEXT("Original three repeats retain exact total damage"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(70));
    TestFalse(TEXT("Mode change cannot interrupt the accepted recovery interval"),F.Battle.SetExecutionMode(EOGCombatExecutionMode::Turn,F.Error));
    if (!F.Battle.AdvanceWorldCombatClock(1000,F.Error)) return false;
    TestFalse(TEXT("Completed action releases its cast"),F.Battle.GetUnitWorldCast(F.A).IsValid());
    TestEqual(TEXT("Completion grants once"),F.Energy(),static_cast<int64>(3));
    TestFalse(TEXT("Mode changes do not erase active World cooldowns"),F.Battle.SetExecutionMode(EOGCombatExecutionMode::Turn,F.Error));
    if (!F.Battle.AdvanceWorldCombatClock(1500,F.Error)) return false;
    TestTrue(TEXT("Completed clocks allow a mode change"),F.Battle.SetExecutionMode(EOGCombatExecutionMode::Turn,F.Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedWorldFirstContactTest,
    "OfflineGame.Combat.World.FirstContactCostsAndMissedRepeats",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedWorldFirstContactTest::RunTest(const FString&)
{
    FSharedWorldFixture F; if (!F.Open(EOGWorldAbilityCostTiming::FirstContact)) return false;
    const auto Root=F.Root(); if (!F.Battle.BeginWorldAbility(Root,F.Error)) return false;
    TestEqual(TEXT("First-contact contract does not pay at declaration"),F.Energy(),static_cast<int64>(20));
    if (!F.Battle.AdvanceWorldCombatClock(100,F.Error) || !F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{},F.Error)) return false;
    TestEqual(TEXT("A physical miss does not invent resource commitment"),F.Energy(),static_cast<int64>(20));
    if (!F.Battle.AdvanceWorldCombatClock(300,F.Error) || !F.Battle.ApplyWorldAbilityStep(Root.ActionId,1,{F.B},F.Error)) return false;
    TestEqual(TEXT("First real contact pays once"),F.Energy(),static_cast<int64>(0));
    if (!F.Battle.AdvanceWorldCombatClock(1001,F.Error) || !F.Battle.ApplyWorldAbilityStep(Root.ActionId,2,{},F.Error)) return false;
    TestTrue(TEXT("Missing repeats do not fabricate damage"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(90));
    TestFalse(TEXT("Late miss closes the cast without leaving a permanent busy flag"),F.Battle.GetUnitWorldCast(F.A).IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedWorldCancelAndCoverageTest,
    "OfflineGame.Combat.World.AuthoredCancellationAndExactPacketCoverage",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedWorldCancelAndCoverageTest::RunTest(const FString&)
{
    FSharedWorldFixture F; if (!F.Open(EOGWorldAbilityCostTiming::AcceptedStart,true)) return false;
    auto Bad=F.World; Bad.Steps[2].Packets[0].FirstRepeat=1;
    TestFalse(TEXT("World mappings cannot duplicate/omit original repeats"),Bad.ValidatePackets(F.Ability,F.Error));
    const auto Root=F.Root(); if (!F.Battle.BeginWorldAbility(Root,F.Error)) return false;
    TestFalse(TEXT("Another cast cannot double-claim the same source"),F.Battle.BeginWorldAbility(F.Root(),F.Error));
    if (!F.Battle.CancelWorldAbility(Root.ActionId,F.Error)) return false;
    TestEqual(TEXT("Only the authored cancellation policy refunds"),F.Energy(),static_cast<int64>(20));
    TestFalse(TEXT("Refund does not reset the accepted cooldown"),F.Battle.BeginWorldAbility(F.Root(),F.Error));
    TestTrue(TEXT("Cancellation never invents target damage"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(100));
    return true;
}
#endif
