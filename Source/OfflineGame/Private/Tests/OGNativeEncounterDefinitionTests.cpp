#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/OGNativeEncounterDefinition.h"
#include "Misc/AutomationTest.h"

namespace
{
UOGNativeEncounterDefinition* EncounterFixture(EOGCombatExecutionMode Mode)
{
    auto* D=NewObject<UOGNativeEncounterDefinition>(); D->DefinitionId=FOGContentId(TEXT("test:encounter.plan"));
    D->OriginalSourceReference=TEXT("isolated encounter fixture"); D->Mode=Mode;
    FOGEncounterRolePlan A,B; A.Role=TEXT("allies"); A.TeamIndex=2; B.Role=TEXT("opponents"); B.TeamIndex=7;
    D->Roles={A,B}; return D;
}
FOGResolvedEncounterParticipant EncounterUnit(FName Role,int32 Index)
{
    FOGResolvedEncounterParticipant P; P.Role=Role; P.FormationIndex=Index; P.Unit.UnitEntityId=FOGEntityId::NewId();
    P.Unit.IdentityId=FOGContentId(FString(TEXT("test:entity."))+FGuid::NewGuid().ToString(EGuidFormats::Digits));
    P.Unit.SkillSet.CurrentForm=FOGContentId(TEXT("test:encounter.version"));
    P.Unit.Stats.MaxHp=P.Unit.CurrentHp=FOGLargeNumber::FromInt64(100); P.Unit.DefaultActionDelay=100;
    return P;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGReusableEncounterFormationTest,
    "OfflineGame.Combat.Encounters.ReusableSourceFormationWithoutCharacterDependencies",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGReusableEncounterFormationTest::RunTest(const FString&)
{
    auto* D=EncounterFixture(EOGCombatExecutionMode::Turn); TArray<FOGResolvedEncounterParticipant> Inputs;
    for (int32 I=0;I<18;++I) Inputs.Add(EncounterUnit(TEXT("allies"),I));
    Inputs.Add(EncounterUnit(TEXT("opponents"),0));
    FOGTurnBattleState State; FString Error; const auto Id=FOGEntityId::NewId();
    if (!TestTrue(TEXT("Assemble actual supplied participants"),D->Assemble(Inputs,Id,State,Error))) return false;
    TestEqual(TEXT("Exact source plan is retained"),State.EncounterDefinitionId,D->DefinitionId);
    TestEqual(TEXT("Six real succession lanes"),State.Teams[0].Lanes.Num(),6);
    TestEqual(TEXT("Third wave remains the actual supplied entity"),State.Teams[0].Lanes[0].OrderedUnitIds[2],Inputs[12].Unit.UnitEntityId);
    TestEqual(TEXT("Reserve is not a generated duplicate"),State.Units[6].Presence,EOGCombatPresence::Reserve);
    FOGTurnBattle Battle;
    TestTrue(TEXT("Shared turn executor admits the generic assembled formation"),Battle.Initialize(State,Error));
    Inputs.Last().Unit.UnitEntityId=Inputs[0].Unit.UnitEntityId;
    TestFalse(TEXT("The same physical unit cannot fill two encounter roles"),D->Assemble(Inputs,Id,State,Error));
    TestEqual(TEXT("Rejected assembly publishes no partial units"),State.Units.Num(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGReusableWorldEncounterTest,
    "OfflineGame.Combat.Encounters.WorldPresencePreservesActualHealth",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGReusableWorldEncounterTest::RunTest(const FString&)
{
    auto* D=EncounterFixture(EOGCombatExecutionMode::World);
    auto A=EncounterUnit(TEXT("allies"),0),B=EncounterUnit(TEXT("opponents"),0),Reserve=EncounterUnit(TEXT("opponents"),0);
    A.Unit.CurrentHp=FOGLargeNumber::FromInt64(25); Reserve.bWorldActive=false;
    FOGTurnBattleState State; FString Error;
    if (!D->Assemble({A,B,Reserve},FOGEntityId::NewId(),State,Error)) return false;
    TestTrue(TEXT("Assembly never heals the actual supplied character"),State.Units[0].CurrentHp==A.Unit.CurrentHp);
    TestEqual(TEXT("World reserves remain off-field"),State.Units[2].Presence,EOGCombatPresence::Reserve);
    TestEqual(TEXT("World mode does not fabricate turn lanes"),State.Teams[0].Lanes.Num(),0);
    FOGTurnBattle Battle; return TestTrue(TEXT("Shared World executor admits the source plan"),Battle.Initialize(State,Error));
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGEncounterRankSourceTest,
    "OfflineGame.Combat.Encounters.RankChannelsUseSourceOrderingAndIntervals",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGEncounterRankSourceTest::RunTest(const FString&)
{
    FOGEncounterRankRules R; R.OrderedRanks={FOGContentId(TEXT("test:z_lower")),FOGContentId(TEXT("test:a_higher"))};
    FOGEncounterRankBand B; B.Channel=OGRankSuppressionChannels::Damage(); B.MinimumRankGap=B.MaximumRankGap=1; B.MultiplierBps=5000;
    R.Bands={B}; FString Error; if (!R.Validate(Error)) return false;
    FOGRankSuppressionQuery Q; Q.SourceRankId=R.OrderedRanks[0]; Q.TargetRankId=R.OrderedRanks[1]; Q.ChannelId=B.Channel;
    int32 Multiplier=0;
    TestTrue(TEXT("Source order is used instead of lexical rank names"),R.Resolve(Q,Multiplier,Error));
    TestEqual(TEXT("Exact authored multiplier"),Multiplier,5000);
    Q.ChannelId=OGRankSuppressionChannels::Control();
    TestFalse(TEXT("An absent channel does not invent a multiplier"),R.Resolve(Q,Multiplier,Error));
    auto Conflict=B; Conflict.MultiplierBps=6000; R.Bands.Add(Conflict);
    TestFalse(TEXT("Contradictory original bands fail before publication"),R.Validate(Error));
    return true;
}
#endif
