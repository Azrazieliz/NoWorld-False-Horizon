#include "Runtime/OGGameCoreSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/GameInstance.h"
#include "Combat/OGNativeCombatAbility.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FOGCombatUnitState ProvenanceUnit(const FOGEntityId& Id, int32 Team, int64 ActionValue)
{
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = Id;
    Unit.IdentityId = FOGContentId(Team == 0
        ? TEXT("test:identity.native_event_source") : TEXT("test:identity.native_event_target"));
    Unit.SkillSet.CurrentForm=FOGContentId(TEXT("test:version.native_event"));
    if (Team==0) Unit.SkillSet.ActiveSkills={FOGContentId(TEXT("test:skill.basic")),FOGContentId(TEXT("test:skill.restore"))};
    Unit.Stats.Attack=FOGLargeNumber::FromInt64(100); Unit.Stats.HitBps=10000;
    Unit.TeamIndex = Team;
    Unit.Presence = EOGCombatPresence::Active;
    Unit.CurrentHp = Unit.Stats.MaxHp = FOGLargeNumber::FromInt64(100);
    Unit.NextActionValue = ActionValue;
    Unit.DefaultActionDelay = 100;
    return Unit;
}
FOGTurnTeamState ProvenanceTeam(int32 Team, const FOGEntityId& Unit)
{
    FOGTurnTeamState Out;
    Out.TeamIndex = Team;
    FOGTurnSuccessionLane Lane;
    Lane.LaneIndex = 0;
    Lane.OrderedUnitIds.Add(Unit);
    Out.Lanes.Add(Lane);
    return Out;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeCombatEventProvenanceTest,
    "OfflineGame.Combat.Persistence.NativeEventProvenanceCommitFailureAndLifetime",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNativeCombatEventProvenanceTest::RunTest(const FString& Parameters)
{
    // No subsystem initialization: never open or alter the player's real save.
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Directory, true);
    TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
    TStrongObjectPtr<UOGGameCoreSubsystem> Core(NewObject<UOGGameCoreSubsystem>(Instance.Get()));
    ON_SCOPE_EXIT
    {
        Core->OnNativeCombatActionResolved.Clear();
        Core->OnCanonicalRuntimeReleasing.Clear();
        Core->ReleaseCanonicalRuntime();
        if (Core->WorldStore) Core->WorldStore->Close();
        Core->WorldStore.Reset();
        Core->bCoreReady = false;
        IFileManager::Get().DeleteDirectory(*Directory, false, true);
    };
    TestFalse(TEXT("Fresh Core has no invented combat event"), Core->GetLastNativeCombatEventId().IsValid());
    auto Store = MakeUnique<FOGSQLiteWorldStore>();
    FOGSQLiteWorldStore* SQLite = Store.Get();
    FString Error;
    if (!TestTrue(TEXT("Open isolated real canonical ledger"), Store->Open(FPaths::Combine(Directory, TEXT("combat_event.db")), Error))) return false;
    Core->WorldStore = MoveTemp(Store);
    const FOGEntityId Ruler = FOGEntityId::NewId();
    const FOGEntityId Source = FOGEntityId::NewId();
    const FOGEntityId Target = FOGEntityId::NewId();
    if (!TestTrue(TEXT("Persist canonical fixture Ruler"), SQLite->UpsertEntity(Ruler, TEXT("ruler"), 0, TEXT("{}"), Error)) ||
        !TestTrue(TEXT("Persist real fixture source"), SQLite->UpsertEntity(Source, TEXT("character"), 0, TEXT("{}"), Error)) ||
        !TestTrue(TEXT("Persist real fixture target"), SQLite->UpsertEntity(Target, TEXT("character"), 0, TEXT("{}"), Error))) return false;
    FOGCanonicalClockPolicy Policy;
    Policy.PolicyId = TEXT("test.native_event"); Policy.Version = 1;
    Policy.RateDenominator = 1000; Policy.WorldRateNumerator = Policy.RulerRateNumerator = Policy.OfflineRateNumerator = 1;
    Policy.TurnLocalRateDivisor = 2; Policy.MaxDueActionsPerPump = 4;
    Policy.CalendarId = FOGContentId(TEXT("test:calendar.native_event"));
    Policy.LocalTicksPerDay = 1000; Policy.DaysPerMonth = 30;
    Core->CanonicalClock = MakeUnique<FOGCanonicalClockRuntime>(*SQLite);
    if (!TestTrue(TEXT("Load actual canonical clock"), Core->CanonicalClock->LoadOrCreate(Policy, 1000, Error))) return false;
    FOGTurnBattleState State;
    State.BattleId = FOGEntityId::NewId();
    State.Units = {ProvenanceUnit(Source, 0, 0), ProvenanceUnit(Target, 1, 1000)};
    State.Teams = {ProvenanceTeam(0, Source), ProvenanceTeam(1, Target)};
    Core->OwnedLuminousTurnBattle = MakeUnique<FOGTurnBattle>();
    if (!TestTrue(TEXT("Use the existing native executor"), Core->OwnedLuminousTurnBattle->Initialize(State, Error))) return false;
    auto InstallFixtureExecutors=[&]()
    {
        Core->OwnedLuminousTurnBattle->RequireNativeActionExecutors(true);
        FOGNativeAbilityDefinition Damage; Damage.SkillId=FOGContentId(TEXT("test:skill.basic"));
        Damage.OriginalSourceReference=TEXT("native ledger integration fixture"); Damage.bNoResourceCost=true;
        FOGNativeAbilityPacket Packet; Packet.FlatAmount=FOGLargeNumber::FromInt64(1);
        Packet.DefenseReference=EOGNativeDefenseReference::Attack; Damage.Packets={Packet};
        if (!Core->OwnedLuminousTurnBattle->RegisterNativeAbility(Damage,Error)) return false;
        auto Restore=Damage; Restore.SkillId=FOGContentId(TEXT("test:skill.restore")); Restore.Targets=EOGNativeAbilityTargets::Self;
        Restore.Packets[0].Effect=EOGNativeAbilityEffect::Healing;
        Restore.Packets[0].DefenseReference=EOGNativeDefenseReference::Unspecified;
        return Core->OwnedLuminousTurnBattle->RegisterNativeAbility(Restore,Error);
    };
    if (!InstallFixtureExecutors()) return false;
    Core->ActiveCombatRulerId = Ruler;
    Core->bCoreReady = true;
    auto Action = [Source, Target]()
    {
        FOGResolvedCombatAction Out;
        Out.ActionId = FOGEntityId::NewId();
        Out.SourceUnitId = Source;
        Out.TargetUnitIds.Add(Target);
        // Exercise an actual installed native packet executor, not an empty timeline action.
        Out.SkillId = FOGContentId(TEXT("test:skill.basic"));
        Out.ActionDelay = 100;
        return Out;
    };
    int32 Notifications = 0;
    Core->OnNativeCombatActionResolved.AddLambda([&](const FOGResolvedCombatAction& Accepted)
    {
        ++Notifications;
        const auto Context = Core->GetLastNativeCombatEventProvenance();
        TestTrue(TEXT("Event provenance is available inside the existing notification"), Context.EventId.IsValid() && Context.ActionId == Accepted.ActionId);
        bool Found = false;
        FOGWorldEvent Stored;
        TestTrue(TEXT("Notified ID already exists in the actual ledger"), SQLite->TryReadWorldEvent(Context.EventId, Found, Stored, Error) && Found);
        TestTrue(TEXT("Recorded event matches exact Ruler and canonical tick"), Stored.PrimaryEntity == Context.OwningRulerId && Stored.WorldTick == Context.WorldTick);
        TestEqual(TEXT("Context is the existing native action journal category"), Stored.EventType, FName(TEXT("combat.turn_action_resolved")));
    });
    const auto First = Action();
    Core->CanonicalClock->SetMode(EOGCanonicalClockMode::World);
    if (!TestTrue(TEXT("World-facing entry uses real journal append"), Core->ApplyApprovedLuminousResolvedAction(First, Error))) return false;
    const auto Recorded = Core->GetLastNativeCombatEventProvenance();
    TestTrue(TEXT("Getter returns the actual journal ID, not action/battle identity"),
        Recorded.EventId == Core->GetLastNativeCombatEventId() && Recorded.EventId != First.ActionId && Recorded.EventId != State.BattleId);
    TestTrue(TEXT("Exact battle, Ruler, source, targets and skill preserved"), Recorded.BattleId == State.BattleId &&
        Recorded.OwningRulerId == Ruler && Recorded.SourceUnitId == Source && Recorded.TargetUnitIds == First.TargetUnitIds && Recorded.SkillId == First.SkillId);
    TestEqual(TEXT("Actual canonical tick retained"), Recorded.WorldTick, Core->GetCanonicalWorldTick());
    TestTrue(TEXT("Ongoing action is not mislabeled post-combat"), Recorded.BattleStatus == EOGTurnBattleStatus::Running);
    const auto BeforeDuplicate=Core->GetApprovedLuminousEncounterState();
    TestFalse(TEXT("Retrying an accepted request cannot execute twice"),Core->ApplyOwnedLuminousTurnAction(First,Error));
    TestEqual(TEXT("Duplicate request emits no extra notification"),Notifications,1);
    TestTrue(TEXT("Duplicate request changes no target HP"),Core->GetApprovedLuminousEncounterState().Units[1].CurrentHp==BeforeDuplicate.Units[1].CurrentHp);
    auto Rejected = Action();
    Rejected.SourceUnitId = FOGEntityId::NewId();
    TestFalse(TEXT("Rejected action cannot replace context"), Core->ApplyOwnedLuminousTurnAction(Rejected, Error));
    TestTrue(TEXT("Last accepted ID survives executor rejection"), Core->GetLastNativeCombatEventId() == Recorded.EventId);
    if (!TestTrue(TEXT("Force an actual SQLite append rejection"), SQLite->ExecuteSql(TEXT(
        "CREATE TRIGGER reject_native_event BEFORE INSERT ON world_events "
        "BEGIN SELECT RAISE(ABORT, 'test_native_event_append_failure'); END;"), Error))) return false;
    const auto Retry = Action();
    const auto BeforeFailure = Core->GetApprovedLuminousEncounterState();
    TestFalse(TEXT("Failed ledger append rejects the action publication"), Core->ApplyOwnedLuminousTurnAction(Retry, Error));
    TestTrue(TEXT("Failed append cannot publish a fresh event ID"), Core->GetLastNativeCombatEventId() == Recorded.EventId);
    TestEqual(TEXT("Failed append emits no resolved notification"), Notifications, 1);
    TestEqual(TEXT("Failed append retains the executor's committed timeline"), Core->GetApprovedLuminousEncounterState().CurrentActionValue, BeforeFailure.CurrentActionValue);
    if (!TestTrue(TEXT("Release the real SQLite failure"), SQLite->ExecuteSql(TEXT("DROP TRIGGER reject_native_event;"), Error))) return false;
    Core->CanonicalClock->SetMode(EOGCanonicalClockMode::Ruler);
    if (!TestTrue(TEXT("Ruler-facing entry uses the same native journal path"), Core->ApplyOwnedLuminousTurnAction(Retry, Error))) return false;
    const auto Latest = Core->GetLastNativeCombatEventProvenance();
    TestTrue(TEXT("Successful later action replaces provenance with its actual event"), Latest.EventId != Recorded.EventId && Latest.ActionId == Retry.ActionId);
    TestEqual(TEXT("Exactly the two committed actions notify consumers"), Notifications, 2);
    // Exercise the existing executor's completion window, as in OGTurnBattleTests.
    // This uses its existing damage primitive, not a second resolver or new skill.
    if (!TestTrue(TEXT("Existing native executor defeats the fixture opponent"), Core->OwnedLuminousTurnBattle->ApplyDamage(
        Source, Target, FOGLargeNumber::FromInt64(100), First.SkillId, Error))) return false;
    auto Completed = Action();
    Completed.TargetUnitIds = {Source};
    Completed.SkillId=FOGContentId(TEXT("test:skill.restore"));
    if (!TestTrue(TEXT("Only the accepted action window publishes post-combat context"), Core->ApplyOwnedLuminousTurnAction(Completed, Error))) return false;
    TestTrue(TEXT("Post-combat classification mirrors actual executor completion"),
        Core->GetLastNativeCombatEventProvenance().BattleStatus == EOGTurnBattleStatus::Completed &&
        Core->GetApprovedLuminousEncounterState().Status == EOGTurnBattleStatus::Completed);
    TestEqual(TEXT("Completed action also notifies exactly once"), Notifications, 3);
    FOGTurnBattleState NextState = State;
    NextState.BattleId = FOGEntityId::NewId();
    if (!TestTrue(TEXT("Actual production encounter start accepts the next fixture battle"), Core->StartApprovedLuminousEncounter(NextState, Ruler, Error))) return false;
    TestFalse(TEXT("New encounter cannot reuse a previous battle's post-combat event"), Core->GetLastNativeCombatEventId().IsValid());
    TestTrue(TEXT("Both public starts establish a durable opening receipt"),Core->GetLastNativeBattleReceipt().IsValid());
    TestFalse(TEXT("Public World startup rejects journal-only commands before admission"),Core->ApplyApprovedLuminousResolvedAction(Action(),Error));
    if (!InstallFixtureExecutors()) return false;
    if (!TestTrue(TEXT("New battle publishes only its own committed action"), Core->ApplyApprovedLuminousResolvedAction(Action(), Error))) return false;
    TestTrue(TEXT("Exact new battle provenance replaces the previous battle"), Core->GetLastNativeCombatEventProvenance().BattleId == NextState.BattleId);
    bool ClearedBeforeReleaseNotification = false;
    Core->OnCanonicalRuntimeReleasing.AddLambda([&]()
    { ClearedBeforeReleaseNotification = !Core->GetLastNativeCombatEventId().IsValid(); });
    TestTrue(TEXT("Canonical runtime releases cleanly"), Core->ReleaseCanonicalRuntime());
    TestTrue(TEXT("Consumers cannot observe previous-world context during release"), ClearedBeforeReleaseNotification);
    const auto Empty = Core->GetLastNativeCombatEventProvenance();
    TestFalse(TEXT("Event ID cleared for a new/imported world"), Empty.EventId.IsValid());
    TestFalse(TEXT("Battle and source provenance also cleared"), Empty.BattleId.IsValid() || Empty.SourceUnitId.IsValid() || Empty.OwningRulerId.IsValid());
    TestEqual(TEXT("Unavailable canonical tick resets explicitly"), Empty.WorldTick, int64(-1));
    return true;
}
#endif
