#if WITH_DEV_AUTOMATION_TESTS
#include "Runtime/OGFoundationWorldSimulationRuntime.h"
#include "World/OGWorldSimulationReceiver.h"
#include "World/OGWorldDirectorService.h"
#include "World/OGSharedWorldStateService.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FWorldBodyFixture
{
    FOGSQLiteWorldStore Store;
    FString Path, Error;
    FOGEntityId Entity = FOGEntityId::NewId(), Location = FOGEntityId::NewId();
    FOGWorldDirectorScheduleRecord Schedule;
    TStrongObjectPtr<UOGWorldSimulationDefinition> Definition{NewObject<UOGWorldSimulationDefinition>()};
    ~FWorldBodyFixture() { Store.Close(); if (!Path.IsEmpty()) IFileManager::Get().Delete(*Path); }
    FString State(const FString& Availability = TEXT("ready")) const
    {
        return FString::Printf(TEXT("{\"active_schedule\":\"%s\",\"state\":\"%s\",\"retained\":\"unchanged\"}"),
            *Schedule.ScheduleId.ToString(), *Availability);
    }
    bool Open()
    {
        const auto Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        Path = FPaths::Combine(Folder, FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".worldbody.db"));
        Definition->DefinitionId = FOGContentId(TEXT("test:world_body"));
        Definition->EntityKind = TEXT("test_body");
        Definition->ScheduleIdField = TEXT("active_schedule");
        Definition->MovementContext = TEXT("test_context");
        Definition->RequiredStateValues.Add(TEXT("state"), TEXT("ready"));
        Definition->AbsentStateValues.Add(TEXT("state"), TEXT("absent"));
        Definition->ArrivalStateField = TEXT("state"); Definition->ArrivalStateValue = TEXT("arrived");
        Definition->AcceptanceRadiusCm = 2.f; Definition->RefreshIntervalSeconds = 1.f;
        Definition->CheckpointIntervalWorldTicks = 5;
        if (!Store.Open(Path, Error) ||
            !Store.UpsertEntity(Location, TEXT("test_location"), 0, TEXT("{}"), Error) ||
            !Store.UpsertEntity(Entity, Definition->EntityKind, 0, TEXT("{}"), Error)) return false;
        FOGLocationRecord L; L.LocationId = Location; L.Kind = TEXT("test_location");
        if (!Store.UpsertLocation(L, 0, Error)) return false;
        FOGWorldPresenceRecord P; P.EntityId = Entity; P.LocationId = Location;
        P.MovementContext = Definition->MovementContext;
        if (!FOGSharedWorldStateService(Store).UpdatePhysicalPresence(P, Error)) return false;
        auto J = MakeShared<FJsonObject>();
        J->SetStringField(TEXT("audit_reason"), TEXT("Isolated native regression fixture"));
        J->SetObjectField(TEXT("world_state_inputs"), MakeShared<FJsonObject>());
        J->SetObjectField(TEXT("package_versions"), MakeShared<FJsonObject>());
        J->SetStringField(TEXT("resolver_key"), FOGFoundationWorldSimulationRuntime::ResolverKey().ToString());
        J->SetStringField(TEXT("definition_id"), Definition->DefinitionId.ToString());
        J->SetStringField(TEXT("subject_id"), Entity.ToString());
        J->SetStringField(TEXT("location_id"), Location.ToString());
        J->SetStringField(TEXT("movement_context"), Definition->MovementContext.ToString());
        TArray<TSharedPtr<FJsonValue>> Position;
        Position.Add(MakeShared<FJsonValueNumber>(100.)); Position.Add(MakeShared<FJsonValueNumber>(0.));
        Position.Add(MakeShared<FJsonValueNumber>(0.)); J->SetArrayField(TEXT("local_position"), Position);
        FOGWorldDirectorScheduleRequest R; R.ContentId = FOGContentId(TEXT("test:world_body_route"));
        R.EarliestStartWorldTick = R.LatestStartWorldTick = 10;
        FJsonSerializer::Serialize(J, TJsonWriterFactory<>::Create(&R.DecisionProvenanceJson));
        return FOGWorldDirectorService(Store).ScheduleEligibleContent(R, Schedule, Error) &&
            Store.UpsertEntity(Entity, Definition->EntityKind, 0, State(), Error);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldBodyArrivalReplayTest,
    "OfflineGame.Foundation.WorldBody.ArrivalReopenAndReplayAreAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGWorldBodyArrivalReplayTest::RunTest(const FString&)
{
    FWorldBodyFixture F; if (!TestTrue(TEXT("Open isolated canonical fixture"), F.Open())) return false;
    FOGFoundationWorldSimulationRuntime Runtime(F.Store); FOGFoundationWorldBodyState Before, Active;
    if (!TestTrue(TEXT("Read existing schedule"), Runtime.Read(*F.Definition, F.Entity, 9, Before, F.Error))) return false;
    TestFalse(TEXT("A future schedule cannot activate"), Runtime.ActivateDue(*F.Definition, Before, 9, F.Error));
    if (!TestTrue(TEXT("Activate at canonical deadline"), Runtime.ActivateDue(*F.Definition, Before, 10, F.Error)) ||
        !TestTrue(TEXT("Read active authority"), Runtime.Read(*F.Definition, F.Entity, 10, Active, F.Error))) return false;
    bool Changed = false;
    TestTrue(TEXT("A stationary checkpoint is accepted without a consequence"),
        Runtime.Checkpoint(*F.Definition, Active, FVector::ZeroVector, 10, false, Changed, F.Error));
    TestFalse(TEXT("No duplicate progress receipt for a stationary body"), Changed);
    TestFalse(TEXT("Path completion far from goal cannot arrive"),
        Runtime.Checkpoint(*F.Definition, Active, FVector::ZeroVector, 10, true, Changed, F.Error));
    if (!TestTrue(TEXT("Actual intermediate motion checkpoints"),
        Runtime.Checkpoint(*F.Definition, Active, FVector(50,0,0), 11, false, Changed, F.Error)) ||
        !TestTrue(TEXT("Read checkpoint"), Runtime.Read(*F.Definition, F.Entity, 11, Active, F.Error))) return false;
    TestEqual(TEXT("Canonical physical position retained"), Active.Presence.LocalPosition, FVector(50,0,0));
    if (!TestTrue(TEXT("Measured arrival commits"), Runtime.Checkpoint(*F.Definition, Active,
        FVector(100,0,0), 12, true, Changed, F.Error))) return false;
    TestTrue(TEXT("First arrival changes state"), Changed);
    F.Store.Close();
    if (!TestTrue(TEXT("Reopen same canonical database"), F.Store.Open(F.Path, F.Error))) return false;
    FOGFoundationWorldSimulationRuntime Reopened(F.Store);
    if (!TestTrue(TEXT("Crash/rebind replay is inert"), Reopened.Checkpoint(*F.Definition, Active,
        FVector(100,0,0), 13, true, Changed, F.Error))) return false;
    TestFalse(TEXT("No duplicate state transition"), Changed);
    FOGFoundationWorldBodyState Final;
    if (!TestTrue(TEXT("Read recovered authority"), Reopened.Read(*F.Definition, F.Entity, 13, Final, F.Error))) return false;
    TestEqual(TEXT("Director remains completed"), Final.Schedule.Status, FName(TEXT("completed")));
    TestTrue(TEXT("Unrelated canonical state survives"), Final.EntityJson.Contains(TEXT("unchanged")));
    TestTrue(TEXT("Authored activity transition survives"), Final.EntityJson.Contains(TEXT("arrived")));
    TArray<FOGWorldEvent> Events;
    if (!F.Store.ListWorldEvents(F.Entity, FOGFoundationWorldSimulationRuntime::ArrivalEventType(),
        false, 32, Events, F.Error)) return false;
    TestEqual(TEXT("Exactly one arrival consequence after reopen"), Events.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldBodyAuthorityFenceTest,
    "OfflineGame.Foundation.WorldBody.ExternalStateAndPresenceInvalidateMotion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGWorldBodyAuthorityFenceTest::RunTest(const FString&)
{
    FWorldBodyFixture F; if (!F.Open()) return false;
    FOGFoundationWorldSimulationRuntime Runtime(F.Store); FOGFoundationWorldBodyState Before, Active, Absent;
    if (!Runtime.Read(*F.Definition, F.Entity, 10, Before, F.Error) ||
        !Runtime.ActivateDue(*F.Definition, Before, 10, F.Error) ||
        !Runtime.Read(*F.Definition, F.Entity, 10, Active, F.Error)) return false;
    auto External = Active.Presence; External.LocalPosition = FVector(25,0,0); External.UpdatedWorldTick = 10;
    if (!FOGSharedWorldStateService(F.Store).UpdatePhysicalPresence(External, F.Error)) return false;
    bool Changed = false;
    TestFalse(TEXT("Same-tick external presence cannot be overwritten by stale body"),
        Runtime.Checkpoint(*F.Definition, Active, FVector(50,0,0), 11, false, Changed, F.Error));
    if (!Runtime.Read(*F.Definition, F.Entity, 11, Active, F.Error) ||
        !F.Store.UpsertEntity(F.Entity, F.Definition->EntityKind, 11, F.State(TEXT("absent")), F.Error)) return false;
    TestFalse(TEXT("World state changes stop obsolete arrival"), Runtime.Checkpoint(*F.Definition,
        Active, FVector(100,0,0), 12, true, Changed, F.Error));
    if (!Runtime.Read(*F.Definition, F.Entity, 12, Absent, F.Error)) return false;
    TestTrue(TEXT("Body suppression is derived from canonical state"), Absent.bAbsent);
    TestFalse(TEXT("Absent body cannot execute schedule"), Absent.bAvailable);
    TestEqual(TEXT("Rejected writes preserve external position"), Absent.Presence.LocalPosition, FVector(25,0,0));
    TestEqual(TEXT("Arrival was not fabricated"), Absent.Schedule.Status, FName(TEXT("active")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldBodyProvenanceAndTransactionTest,
    "OfflineGame.Foundation.WorldBody.ForeignSubjectAndUncommittedEffectsAreRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGWorldBodyProvenanceAndTransactionTest::RunTest(const FString&)
{
    FWorldBodyFixture F; if (!F.Open()) return false;
    FOGFoundationWorldSimulationRuntime Runtime(F.Store); FOGFoundationWorldBodyState State;
    if (!Runtime.Read(*F.Definition, F.Entity, 10, State, F.Error) ||
        !F.Store.BeginTransaction(F.Error)) return false;
    TestFalse(TEXT("A body cannot accept effects inside another caller's uncommitted transaction"),
        Runtime.ActivateDue(*F.Definition, State, 10, F.Error));
    if (!F.Store.RollbackTransaction(F.Error)) return false;
    auto Tampered = F.Schedule;
    TSharedPtr<FJsonObject> Json;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Tampered.DecisionProvenanceJson), Json) || !Json) return false;
    Json->SetStringField(TEXT("subject_id"), FOGEntityId::NewId().ToString());
    FJsonSerializer::Serialize(Json.ToSharedRef(), TJsonWriterFactory<>::Create(&Tampered.DecisionProvenanceJson));
    if (!F.Store.UpsertWorldDirectorSchedule(Tampered, 10, F.Error)) return false;
    TestFalse(TEXT("A schedule for another subject cannot drive this body"),
        Runtime.Read(*F.Definition, F.Entity, 10, State, F.Error));
    bool Found = false; FOGWorldDirectorScheduleRecord Persisted;
    if (!F.Store.TryReadWorldDirectorSchedule(F.Schedule.ScheduleId, Found, Persisted, F.Error)) return false;
    TestTrue(TEXT("Original schedule still exists"), Found);
    TestEqual(TEXT("Rejected bindings never activate a schedule"), Persisted.Status, FName(TEXT("scheduled")));
    return true;
}
#endif
