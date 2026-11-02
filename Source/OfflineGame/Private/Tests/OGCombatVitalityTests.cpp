#if WITH_DEV_AUTOMATION_TESTS
#include "Combat/OGCombatVitalityService.h"
#include "Combat/OGCombatActionJournal.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
namespace
{
struct FVitalityFixture
{
    FOGSQLiteWorldStore Store;
    FOGCombatVitalityService Service{Store};
    FOGTurnBattleState State;
    FOGEntityId Ruler=FOGEntityId::NewId(),Enemy=FOGEntityId::NewId();
    FString Path,Error;
    FString Original=TEXT("{\"approved_name\":\"Preserved fixture\",\"unrelated\":{\"value\":71}}");
    int64 Tick=1;
    ~FVitalityFixture() { Store.Close(); if (!Path.IsEmpty()) IFileManager::Get().Delete(*Path); }
    bool Open()
    {
        const auto Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation"));
        IFileManager::Get().MakeDirectory(*Directory,true);
        Path=FPaths::Combine(Directory,FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".vitality.db"));
        if (!Store.Open(Path,Error) || !Store.UpsertEntity(Ruler,TEXT("ruler"),0,TEXT("{}"),Error) ||
            !Store.UpsertEntity(Enemy,TEXT("test_npc"),0,Original,Error)) return false;
        State.BattleId=FOGEntityId::NewId(); State.Status=EOGTurnBattleStatus::Running;
        for (int32 I=0;I<2;++I)
        {
            FOGCombatUnitState U; U.UnitEntityId=I?Enemy:Ruler;
            U.IdentityId=FOGContentId(I?TEXT("test:vitality.enemy"):TEXT("test:vitality.ruler"));
            U.SkillSet.CurrentForm=FOGContentId(TEXT("test:vitality.version"));
            U.Stats.MaxHp=U.CurrentHp=FOGLargeNumber::FromInt64(100);
            U.Presence=EOGCombatPresence::Active; U.TeamIndex=I; State.Units.Add(U);
        }
        FOGWorldEvent Opening;
        if (!FOGCombatActionJournal::BuildEncounterStartedEvent(State,State,{},Ruler,Tick++,Opening,Error) ||
            !Store.BeginTransaction(Error)) return false;
        if (!Store.AppendWorldEvent(Opening,Error) || !Service.Commit(State,State,Ruler,Opening,Error))
        { FString Ignored; Store.RollbackTransaction(Ignored); return false; }
        return Store.CommitTransaction(Error);
    }
    bool ApplyHp(int64 Hp,bool Rollback=false,bool Stale=false)
    {
        auto Before=State;
        if (Stale) Before.Units[1].CurrentHp=FOGLargeNumber::FromInt64(100);
        auto After=Before; After.Units[1].CurrentHp=FOGLargeNumber::FromInt64(Hp);
        After.Units[1].Presence=Hp?EOGCombatPresence::Active:EOGCombatPresence::Defeated;
        FOGResolvedCombatAction Action; Action.ActionId=FOGEntityId::NewId(); Action.SourceUnitId=Ruler;
        Action.SkillId=FOGContentId(TEXT("test:vitality.damage")); Action.TargetUnitIds={Enemy}; Action.ActionDelay=100;
        FOGWorldEvent Receipt;
        if (!FOGCombatActionJournal::BuildResolvedActionEvent(Before,After,Action,Ruler,Tick++,Receipt,Error) ||
            !Store.BeginTransaction(Error)) return false;
        if (!Store.AppendWorldEvent(Receipt,Error) || !Service.Commit(Before,After,Ruler,Receipt,Error))
        { FString Ignored; Store.RollbackTransaction(Ignored); return false; }
        if (Rollback) return Store.RollbackTransaction(Error);
        if (!Store.CommitTransaction(Error)) return false;
        State=After; return true;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCombatVitalityRestartTest,
    "OfflineGame.Combat.Persistence.ActualHealthSurvivesRestartAndVersionProjection",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCombatVitalityRestartTest::RunTest(const FString&)
{
    FVitalityFixture F; if (!TestTrue(TEXT("Seed real isolated canonical entities and opening"),F.Open()) ||
        !TestTrue(TEXT("Commit actual HP with the same receipt transaction"),F.ApplyHp(25))) return false;
    F.Store.Close(); if (!TestTrue(TEXT("Reopen the actual SQLite save"),F.Store.Open(F.Path,F.Error))) return false;
    auto Unit=F.State.Units[1]; Unit.CurrentHp=FOGLargeNumber::FromInt64(100);
    Unit.SkillSet.CurrentForm=FOGContentId(TEXT("test:vitality.other_version"));
    if (!TestTrue(TEXT("Project the same actual entity through another Version"),F.Service.Project(Unit,F.Error))) return false;
    TestTrue(TEXT("A Version switch does not create a fresh HP pool"),Unit.CurrentHp==FOGLargeNumber::FromInt64(25));
    bool Found=false; FName Kind; FString Metadata; int64 Revision=0;
    if (!F.Store.TryReadEntity(F.Enemy,Found,Kind,Metadata,Revision,F.Error)) return false;
    TestTrue(TEXT("Approved entity metadata remains byte-for-byte unchanged"),Found && Metadata==F.Original && Revision==0);
    FOGStoredCombatVitality Saved;
    if (!F.Service.Read(F.Enemy,Found,Saved,F.Error)) return false;
    TestTrue(TEXT("Health retains actual entity, encounter and receipt provenance"),
        Found && Saved.EntityId==F.Enemy && Saved.BattleId==F.State.BattleId && Saved.ReceiptId.IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCombatVitalityAtomicTest,
    "OfflineGame.Combat.Persistence.HealthRollbackStaleWritesAndPersistentDefeat",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCombatVitalityAtomicTest::RunTest(const FString&)
{
    FVitalityFixture F; if (!F.Open() || !F.ApplyHp(25) || !F.ApplyHp(0,true)) return false;
    bool Found=false; FOGStoredCombatVitality Saved;
    if (!F.Service.Read(F.Enemy,Found,Saved,F.Error)) return false;
    TestTrue(TEXT("Action rollback rolls health back with its receipt"),Found && Saved.CurrentHp==FOGLargeNumber::FromInt64(25));
    TestFalse(TEXT("A stale encounter cannot overwrite newer canonical health"),F.ApplyHp(0,false,true));
    if (!F.ApplyHp(0)) return false;
    auto Restored=F.State.Units[1]; Restored.CurrentHp=Restored.Stats.MaxHp; Restored.Presence=EOGCombatPresence::Active;
    if (!F.Service.Project(Restored,F.Error)) return false;
    TestTrue(TEXT("Re-entry does not silently revive a defeated entity"),
        !Restored.IsAlive() && Restored.Presence==EOGCombatPresence::Defeated);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCombatVitalityRespawnTest,
    "OfflineGame.Combat.Persistence.OnlyCanonicalLifeEpochAuthorizesRespawn",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCombatVitalityRespawnTest::RunTest(const FString&)
{
    FVitalityFixture F; if (!F.Open() || !F.ApplyHp(0)) return false;
    const auto Life=FOGEntityId::NewId();
    const FString NewState=FString::Printf(TEXT("{\"combat_reward_life_id\":\"%s\",\"approved_name\":\"Preserved fixture\"}"),*Life.ToString());
    if (!F.Store.UpsertEntity(F.Enemy,TEXT("test_npc"),10,NewState,F.Error)) return false;
    auto Unit=F.State.Units[1]; Unit.CurrentHp=Unit.Stats.MaxHp; Unit.Presence=EOGCombatPresence::Active;
    if (!F.Service.Project(Unit,F.Error)) return false;
    TestTrue(TEXT("An actual new canonical life can use its authored opening health"),Unit.IsAlive());
    return true;
}
#endif
