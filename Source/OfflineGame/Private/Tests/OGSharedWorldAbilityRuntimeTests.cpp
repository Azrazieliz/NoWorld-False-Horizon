#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Combat/OGTurnBattle.h"

namespace
{
FOGContentId WorldTestId(const TCHAR* Name) { return FOGContentId(FString(TEXT("test:shared_world."))+Name); }
struct FSharedWorldFixture
{
    FOGTurnBattle Battle; FString Error;
    FOGEntityId A=FOGEntityId::NewId(),B=FOGEntityId::NewId(),C=FOGEntityId::NewId();
    FOGNativeAbilityDefinition Ability;
    FOGWorldAbilitySpecification World;
    bool Open(EOGWorldAbilityCostTiming Cost=EOGWorldAbilityCostTiming::AcceptedStart,bool Refund=false,bool WithEffect=false,bool WithExtraTarget=false)
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
        if (WithExtraTarget)
        { auto Other=Target; Other.UnitEntityId=C; Other.IdentityId=WorldTestId(TEXT("other_target")); State.Units.Add(Other); }
        for (const auto& U:State.Units)
        {
            auto* Team=State.Teams.FindByPredicate([&](const auto& T){return T.TeamIndex==U.TeamIndex;});
            if (!Team) { FOGTurnTeamState Added; Added.TeamIndex=U.TeamIndex; Team=&State.Teams.Add_GetRef(Added); }
            FOGTurnSuccessionLane Lane; Lane.OrderedUnitIds={U.UnitEntityId}; Team->Lanes.Add(Lane);
        }
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
        if (WithEffect)
        {
            FOGNativeEffectDefinition Effect; Effect.Effect.EffectId=WorldTestId(TEXT("periodic"));
            Effect.Effect.FamilyId=WorldTestId(TEXT("periodic_family")); Effect.Effect.MechanicKind=TEXT("periodic_hp");
            Effect.Effect.RulePriority.SourceRuleId=Effect.Effect.EffectId;
            Effect.Effect.Lifetime.Kind=EOGEffectLifetimeKind::RealTimeMilliseconds; Effect.Effect.Lifetime.Magnitude=600;
            Effect.Mechanic=EOGNativeEffectMechanic::PeriodicDamage; Effect.FlatMagnitude=FOGLargeNumber::FromInt64(5);
            Effect.WorldTickIntervalMilliseconds=200; Effect.OriginalSourceReference=TEXT("isolated real-time fixture");
            if (!Battle.RegisterNativeEffect(Effect,Error)) return false;
            FOGNativeAbilityEffectOperation Apply; Apply.Id=Effect.Effect.EffectId; Apply.bBeforePackets=true;
            Ability.Effects={Apply};
        }
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldPeriodicLifetimeTest,
    "OfflineGame.Combat.World.PeriodicStatusUsesGameplayMilliseconds",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGWorldPeriodicLifetimeTest::RunTest(const FString&)
{
    FSharedWorldFixture F; if (!F.Open(EOGWorldAbilityCostTiming::AcceptedStart,false,true)) return false;
    auto Root=F.Root(); if (!F.Battle.BeginWorldAbility(Root,F.Error) ||
        !F.Battle.AdvanceWorldCombatClock(100,F.Error) ||
        !F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{F.B},F.Error) ||
        !F.Battle.CancelWorldAbility(Root.ActionId,F.Error)) return false;
    TestEqual(TEXT("A finite real-time status is actually installed"),F.Battle.GetState().Units[1].NativeEffects.Num(),1);
    TestFalse(TEXT("Changing combat mode cannot erase an active status clock"),F.Battle.SetExecutionMode(EOGCombatExecutionMode::Turn,F.Error));
    if (!F.Battle.AdvanceWorldCombatClock(299,F.Error)) return false;
    TestTrue(TEXT("No early periodic damage"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(90));
    if (!F.Battle.AdvanceWorldCombatClock(300,F.Error)) return false;
    TestTrue(TEXT("The exact interval deals its sourced packet"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(85));
    if (!F.Battle.AdvanceWorldCombatClock(701,F.Error)) return false;
    TestTrue(TEXT("Catch-up preserves every endpoint-inclusive tick"),F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(75));
    TestEqual(TEXT("Finite status expires without requiring an owner action"),F.Battle.GetState().Units[1].NativeEffects.Num(),0);
    TestEqual(TEXT("Status processing does not fabricate additional skill costs"),F.Energy(),static_cast<int64>(0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedWorldCheckpointTest,
    "OfflineGame.Combat.Recovery.ActiveCastEffectsCostsAndCooldown",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedWorldCheckpointTest::RunTest(const FString&)
{
    FSharedWorldFixture F; if (!F.Open(EOGWorldAbilityCostTiming::AcceptedStart,false,true)) return false;
    const auto Root=F.Root();
    if (!F.Battle.BeginWorldAbility(Root,F.Error) || !F.Battle.AdvanceWorldCombatClock(100,F.Error) ||
        !F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{F.B},F.Error) ||
        !F.Battle.AdvanceWorldCombatClock(250,F.Error)) return false;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Exports settled real cast runtime"),F.Battle.ExportNativeCheckpoint(Bytes,F.Error))) { AddError(F.Error); return false; }
    FOGTurnBattle Restored;
    if (!TestTrue(TEXT("Restores existing cast without opening/replay"),Restored.RestoreNativeCheckpoint(Bytes,{},F.Error)))
    { AddError(F.Error); return false; }
    TestEqual(TEXT("Offline elapsed time never advances the active cast"),Restored.GetState().WorldElapsedMilliseconds,int64(250));
    TestEqual(TEXT("Accepted-start cost is not paid twice"),Restored.GetState().Units[0].BattleResources[0].Value,int64(0));
    TestEqual(TEXT("Exact World cooldown survives"),Restored.GetWorldAbilityReadyAt(F.A,F.Ability.SkillId),F.Battle.GetWorldAbilityReadyAt(F.A,F.Ability.SkillId));
    TestEqual(TEXT("No new opening/contact log events"),Restored.GetLog().GetNextSequence(),F.Battle.GetLog().GetNextSequence());
    TestFalse(TEXT("A committed contact cannot be replayed"),Restored.ApplyWorldAbilityStep(Root.ActionId,0,{F.B},F.Error));
    for (int32 Step=1;Step<3;++Step)
    {
        const int64 Time=100+Step*200;
        if (!F.Battle.AdvanceWorldCombatClock(Time,F.Error) || !Restored.AdvanceWorldCombatClock(Time,F.Error) ||
            !F.Battle.ApplyWorldAbilityStep(Root.ActionId,Step,{F.B},F.Error) ||
            !Restored.ApplyWorldAbilityStep(Root.ActionId,Step,{F.B},F.Error)) return false;
        TestTrue(TEXT("Original and restored effects/contacts resolve identical HP"),
            F.Battle.GetState().Units[1].CurrentHp==Restored.GetState().Units[1].CurrentHp);
    }
    if (!F.Battle.AdvanceWorldCombatClock(1000,F.Error) || !Restored.AdvanceWorldCombatClock(1000,F.Error)) return false;
    TestFalse(TEXT("Recovered action releases at its original deadline"),Restored.GetUnitWorldCast(F.A).IsValid());
    TestEqual(TEXT("Completion grant occurs once"),Restored.GetState().Units[0].BattleResources[0].Value,int64(3));
    TestTrue(TEXT("Periodic effect clock matches uninterrupted execution"),
        F.Battle.GetState().Units[1].CurrentHp==Restored.GetState().Units[1].CurrentHp);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedWorldCheckpointIntegrityTest,
    "OfflineGame.Combat.Recovery.CorruptionPreservesCurrentEncounter",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedWorldCheckpointIntegrityTest::RunTest(const FString&)
{
    FSharedWorldFixture F; if (!F.Open()) return false;
    TArray<uint8> Bytes; if (!F.Battle.ExportNativeCheckpoint(Bytes,F.Error)) return false;
    const auto Before=F.Battle.GetState().BattleId; const auto Sequence=F.Battle.GetLog().GetNextSequence();
    Bytes.Last()^=0x5a;
    TestFalse(TEXT("Corrupt payload is rejected before state publication"),F.Battle.RestoreNativeCheckpoint(Bytes,{},F.Error));
    TestTrue(TEXT("Current encounter survives failed restore"),F.Battle.GetState().BattleId==Before);
    TestEqual(TEXT("Failed restore does not publish log events"),F.Battle.GetLog().GetNextSequence(),Sequence);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldAcceptedTargetLockTest,
    "OfflineGame.Combat.World.AcceptedTargetLockRejectsRetargetAndDuplicateContacts",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGWorldAcceptedTargetLockTest::RunTest(const FString&)
{
    FSharedWorldFixture F;
    if (!F.Open(EOGWorldAbilityCostTiming::FirstContact,false,false,true)) { AddError(F.Error); return false; }
    const auto Root=F.Root();
    if (!F.Battle.BeginWorldAbility(Root,F.Error) || !F.Battle.AdvanceWorldCombatClock(100,F.Error)) return false;
    const int64 Before=F.Battle.GetLog().GetNextSequence();
    TestFalse(TEXT("Direct impact cannot replace the accepted selected target"),
        F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{F.C},F.Error));
    TestFalse(TEXT("Duplicate physical contacts cannot resolve the same repeat twice"),
        F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{F.B,F.B},F.Error));
    TestEqual(TEXT("Rejected impacts do not spend first-contact costs"),F.Energy(),int64(20));
    TestEqual(TEXT("Rejected impacts append no canonical combat events"),F.Battle.GetLog().GetNextSequence(),Before);
    FOGWorldAbilityCastView View;
    TestTrue(TEXT("Rejected impacts leave the accepted step open"),
        F.Battle.ReadWorldCast(Root.ActionId,View) && View.CompletedSteps==0);
    if (!TestTrue(TEXT("The genuine selected contact still resolves"),
        F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{F.B},F.Error))) return false;
    TestEqual(TEXT("First accepted contact spends the source cost once"),F.Energy(),int64(0));
    TestTrue(TEXT("Only the declared target receives the original packet"),
        F.Battle.GetState().Units[1].CurrentHp==FOGLargeNumber::FromInt64(90) &&
        F.Battle.GetState().Units[2].CurrentHp==FOGLargeNumber::FromInt64(100));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldFreeAimStableContactTest,
    "OfflineGame.Combat.World.FreeAimContactSelectionIsStable",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGWorldFreeAimStableContactTest::RunTest(const FString&)
{
    FSharedWorldFixture F;
    if (!F.Open(EOGWorldAbilityCostTiming::FirstContact,false,false,true)) { AddError(F.Error); return false; }
    auto Root=F.Root(); Root.TargetUnitIds.Reset();
    TArray<FOGEntityId> Forward,Reverse;
    if (!F.Battle.FilterWorldContacts(Root,{F.B,F.C,F.B},Forward,F.Error) ||
        !F.Battle.FilterWorldContacts(Root,{F.C,F.B},Reverse,F.Error)) return false;
    TestTrue(TEXT("Physics enumeration and repeated overlaps cannot change the chosen target"),
        Forward.Num()==1 && Forward==Reverse);
    const FOGEntityId Expected=F.B.ToString()<F.C.ToString()?F.B:F.C;
    TestTrue(TEXT("Free aim uses stable canonical contact order"),Forward.Num()==1 && Forward[0]==Expected);
    if (!F.Battle.BeginWorldAbility(Root,F.Error) || !F.Battle.AdvanceWorldCombatClock(100,F.Error)) return false;
    TestFalse(TEXT("A direct single-target impact cannot smuggle two targets"),
        F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,{F.B,F.C},F.Error));
    TestTrue(TEXT("The filtered genuine contact resolves"),F.Battle.ApplyWorldAbilityStep(Root.ActionId,0,Forward,F.Error));
    return true;
}

#endif
