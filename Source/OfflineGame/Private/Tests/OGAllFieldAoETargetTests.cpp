#include "Combat/OGTurnBattle.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGAllFieldTargetsTest,
    "OfflineGame.Combat.Luminous.AoEAlwaysAllFieldParticipants",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)

bool FOGAllFieldTargetsTest::RunTest(const FString&)
{
    FOGTurnBattleState State;
    State.BattleId=FOGEntityId::NewId();
    FOGTurnTeamState Team0;Team0.TeamIndex=0;
    FOGTurnTeamState Team1;Team1.TeamIndex=1;
    TArray<FOGEntityId> Allies,Enemies;
    auto Add=[&](int32 Team,bool bActive)->FOGEntityId
    {
        FOGCombatUnitState U;
        U.UnitEntityId=FOGEntityId::NewId();
        U.IdentityId=FOGContentId(
            FString::Printf(TEXT("test:aoe.%s"),*U.UnitEntityId.ToString()));
        U.TeamIndex=Team;
        U.Presence=bActive?EOGCombatPresence::Active:EOGCombatPresence::Reserve;
        U.CurrentHp=FOGLargeNumber::FromInt64(100);
        U.Stats.MaxHp=FOGLargeNumber::FromInt64(100);
        U.DefaultActionDelay=100;
        State.Units.Add(U);
        FOGTurnTeamState& T=Team==0?Team0:Team1;
        if (!bActive && T.Lanes.Num()>0)
        {
            // True OFF-FIELD reserve must follow a living active owner of
            // its succession lane; a reserve alone is promoted at start.
            T.Lanes[0].OrderedUnitIds.Add(U.UnitEntityId);
        }
        else
        {
            FOGTurnSuccessionLane L;
            L.LaneIndex=T.Lanes.Num();
            L.OrderedUnitIds.Add(U.UnitEntityId);
            T.Lanes.Add(L);
        }
        return U.UnitEntityId;
    };
    for(int32 I=0;I<3;++I)Allies.Add(Add(0,true));
    for(int32 I=0;I<5;++I)Enemies.Add(Add(1,true));
    // A real off-field reserve is NOT hit by area action.
    const FOGEntityId Reserve=Add(1,false);
    State.Teams={Team0,Team1};
    FString Error;
    auto Run=[&](bool bAlter,const FOGContentId& Skill,
                 const FString& Context,bool bAlly,int32 Expected)
    {
        FOGTurnBattleState Setup=State;
        FOGCombatUnitState& Source=Setup.Units[0];
        Source.IdentityId=FOGContentId(TEXT("character:luminous_valentine.identity"));
        Source.SkillSet.ActiveSkills=bAlter
            ? TArray<FOGContentId>({
                FOGContentId(TEXT("combat:luminous_valentine.alter.active_1")),
                FOGContentId(TEXT("combat:luminous_valentine.alter.active_2")),
                FOGContentId(TEXT("combat:luminous_valentine.alter.active_3"))})
            : TArray<FOGContentId>({
                FOGContentId(TEXT("combat:luminous_valentine.base.skill.scarlet_cadence")),
                FOGContentId(TEXT("combat:luminous_valentine.base.skill.blood_throne")),
                FOGContentId(TEXT("combat:luminous_valentine.base.skill.benediction"))});
        Source.SkillSet.UltimateSkill=bAlter
            ?FOGContentId(TEXT("combat:luminous_valentine.alter.ultimate.black_mass"))
            :FOGContentId(TEXT("combat:luminous_valentine.base.ultimate.twin_principles"));
        FOGTurnBattle Battle;
        FString E;
        if(!TestTrue(TEXT("Native battle initializes"),Battle.Initialize(Setup,E)))
        { AddError(E);return; }
        FOGResolvedCombatAction In;
        In.SourceUnitId=Source.UnitEntityId;
        In.SkillId=Skill;
        In.ResolutionJson=Context;
        In.TargetUnitIds.Add(bAlly?Allies[1]:Enemies[2]); // primary
        FOGResolvedCombatAction Out;
        bool bArea=false;
        if(!TestTrue(TEXT("Canonical field expansion"),Battle.ExpandApprovedFieldTargets(
            In,Out,bArea,E)))
        { AddError(E);return; }
        TestTrue(TEXT("Correct AoE semantic branch"),bArea);
        TestEqual(TEXT("All and only living field participants"),Out.TargetUnitIds.Num(),Expected);
        TestTrue(TEXT("Preserved selected primary"),Out.TargetUnitIds[0]==
            (bAlly?Allies[1]:Enemies[2]));
        TestFalse(TEXT("Off-field reserve remains untargeted"),Out.TargetUnitIds.Contains(Reserve));
    };
    Run(false,FOGContentId(TEXT("combat:luminous_valentine.base.skill.blood_throne")),
        TEXT("{}"),false,5);
    Run(false,FOGContentId(TEXT("combat:luminous_valentine.base.ultimate.twin_principles")),
        TEXT("{\"branch\":\"night\"}"),false,5);
    Run(false,FOGContentId(TEXT("combat:luminous_valentine.base.ultimate.twin_principles")),
        TEXT("{\"branch\":\"day\"}"),true,3);
    Run(true,FOGContentId(TEXT("combat:luminous_valentine.alter.active_2")),
        TEXT("{}"),false,5);
    Run(true,FOGContentId(TEXT("combat:luminous_valentine.alter.ultimate.black_mass")),
        TEXT("{\"branch\":\"ruin\"}"),false,5);
    return true;
}
#endif
