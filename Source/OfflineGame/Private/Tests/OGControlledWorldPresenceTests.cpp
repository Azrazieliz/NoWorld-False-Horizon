#if WITH_DEV_AUTOMATION_TESTS
#include "Runtime/OGFoundationWorldSimulationRuntime.h"
#include "Runtime/OGGate12LocalRulerBootstrap.h"
#include "World/OGWorldSimulationReceiver.h"
#include "World/OGSharedWorldStateService.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FControlledPresenceFixture
{
    FOGSQLiteWorldStore Store;
    FString Path, Error;
    FOGEntityId Ruler, Location = FOGEntityId::NewId();
    TStrongObjectPtr<UOGWorldSimulationDefinition> Definition{NewObject<UOGWorldSimulationDefinition>()};
    ~FControlledPresenceFixture() { Store.Close(); if (!Path.IsEmpty()) IFileManager::Get().Delete(*Path); }
    bool Open()
    {
        const auto Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"));
        IFileManager::Get().MakeDirectory(*Folder, true);
        Path = FPaths::Combine(Folder, FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".controlledpresence.db"));
        Definition->DefinitionId = FOGContentId(TEXT("test:controlled_presence"));
        Definition->bControlledPresence = true;
        Definition->EntityKind = TEXT("ruler");
        Definition->MovementContext = TEXT("test_context");
        Definition->RequiredStateValues.Add(TEXT("origin"), TEXT("explicit_local_creation"));
        Definition->RefreshIntervalSeconds = 1.f;
        Definition->CheckpointIntervalWorldTicks = 5;
        if (!Store.Open(Path, Error) || !FOGGate12LocalRulerBootstrap::Create(Store, 0, Ruler, Error) ||
            !Store.UpsertEntity(Location, TEXT("test_location"), 0, TEXT("{}"), Error)) return false;
        FOGLocationRecord L; L.LocationId = Location; L.Kind = TEXT("test_location");
        if (!Store.UpsertLocation(L, 0, Error)) return false;
        return AddPresence(Ruler);
    }
    bool AddPresence(const FOGEntityId& Entity)
    {
        FOGWorldPresenceRecord P; P.EntityId = Entity; P.LocationId = Location;
        P.MovementContext = Definition->MovementContext;
        return FOGSharedWorldStateService(Store).UpdatePhysicalPresence(P, Error);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGControlledPresenceReopenTest,
    "OfflineGame.Foundation.WorldBody.ControlledPresence.ReopenAndStationaryBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGControlledPresenceReopenTest::RunTest(const FString&)
{
    FControlledPresenceFixture F; if (!TestTrue(TEXT("Open real local-owner fixture"), F.Open())) return false;
    FOGFoundationWorldSimulationRuntime Runtime(F.Store); FOGFoundationWorldBodyState Before, After;
    if (!TestTrue(TEXT("Read controlled body without an AI schedule"), Runtime.Read(*F.Definition, F.Ruler, 0, Before, F.Error))) return false;
    TestFalse(TEXT("No synthetic schedule"), Before.bHasSchedule);
    bool Changed = false;
    TestFalse(TEXT("Controlled body cannot activate a Director schedule"), Runtime.ActivateDue(*F.Definition, Before, 5, F.Error));
    TestFalse(TEXT("Controlled body cannot manufacture arrival"), Runtime.Checkpoint(*F.Definition, Before, FVector(7,8,9), 5, true, Changed, F.Error));
    if (!TestTrue(TEXT("Measured boundary checkpoint"), Runtime.CheckpointControlled(*F.Definition, Before, FVector(7,8,9), 5, Changed, F.Error))) return false;
    TestTrue(TEXT("First measured motion commits"), Changed);
    F.Store.Close();
    if (!F.Store.Open(F.Path, F.Error) || !Runtime.Read(*F.Definition, F.Ruler, 5, After, F.Error)) return false;
    TestEqual(TEXT("Same canonical database retains local position"), After.Presence.LocalPosition, FVector(7,8,9));
    TestEqual(TEXT("Source tick retained"), After.Presence.UpdatedWorldTick, int64(5));
    TestTrue(TEXT("Stationary save is inert"), Runtime.CheckpointControlled(*F.Definition, After, FVector(7,8,9), 6, Changed, F.Error));
    TestFalse(TEXT("Stationary save adds no receipt"), Changed);
    TArray<FOGWorldEvent> Events;
    if (!F.Store.ListWorldEvents(F.Ruler, FOGFoundationWorldSimulationRuntime::ControlledCheckpointEventType(), false, 32, Events, F.Error)) return false;
    TestEqual(TEXT("One measured checkpoint receipt"), Events.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGControlledPresenceAuthorityTest,
    "OfflineGame.Foundation.WorldBody.ControlledPresence.StaleAuthorityAndTransactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGControlledPresenceAuthorityTest::RunTest(const FString&)
{
    FControlledPresenceFixture F; if (!F.Open()) return false;
    FOGFoundationWorldSimulationRuntime Runtime(F.Store); FOGFoundationWorldBodyState Before, Current;
    if (!Runtime.Read(*F.Definition, F.Ruler, 0, Before, F.Error)) return false;
    auto P = Before.Presence; P.LocalPosition = FVector(11,0,0);
    if (!FOGSharedWorldStateService(F.Store).UpdatePhysicalPresence(P, F.Error)) return false;
    bool Changed = false;
    TestFalse(TEXT("Same-tick external position wins over stale body"), Runtime.CheckpointControlled(*F.Definition, Before, FVector(22,0,0), 1, Changed, F.Error));
    if (!Runtime.Read(*F.Definition, F.Ruler, 1, Current, F.Error) || !F.Store.BeginTransaction(F.Error)) return false;
    TestFalse(TEXT("Foreign root transaction cannot absorb a physical checkpoint"), Runtime.CheckpointControlled(*F.Definition, Current, FVector(22,0,0), 1, Changed, F.Error));
    if (!F.Store.RollbackTransaction(F.Error)) return false;
    TestFalse(TEXT("Time cannot move behind the accepted presence"), Runtime.CheckpointControlled(*F.Definition, Current, FVector(22,0,0), -1, Changed, F.Error));
    F.Definition->AbsentStateValues = F.Definition->RequiredStateValues;
    if (!Runtime.Read(*F.Definition, F.Ruler, 1, Current, F.Error)) return false;
    TestTrue(TEXT("Canonical absence visible"), Current.bAbsent);
    TestFalse(TEXT("Absent body cannot save"), Runtime.CheckpointControlled(*F.Definition, Current, FVector(22,0,0), 2, Changed, F.Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGControlledPresenceOwnershipTest,
    "OfflineGame.Foundation.WorldBody.ControlledPresence.OwnedManifestationFence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGControlledPresenceOwnershipTest::RunTest(const FString&)
{
    FControlledPresenceFixture F; if (!F.Open()) return false;
    F.Definition->EntityKind = TEXT("manifestation");
    F.Definition->RequiredStateValues.Reset();
    F.Definition->RequiredStateValues.Add(TEXT("state"), TEXT("ready"));
    FOGCharacterManifestationRecord M; M.ManifestationId = FOGEntityId::NewId();
    M.OwningRulerId = F.Ruler; M.IdentityId = FOGContentId(TEXT("test:presence_identity"));
    M.ActiveVersionId = FOGContentId(TEXT("test:presence_version"));
    if (!F.Store.UpsertCharacterManifestation(M, 0, F.Error) ||
        !F.Store.UpsertEntity(M.ManifestationId, F.Definition->EntityKind, 0, TEXT("{\"state\":\"ready\"}"), F.Error) ||
        !F.AddPresence(M.ManifestationId)) return false;
    FOGFoundationWorldSimulationRuntime Runtime(F.Store); FOGFoundationWorldBodyState Before;
    if (!TestTrue(TEXT("Exact active locally owned Manifestation reads"), Runtime.Read(*F.Definition, M.ManifestationId, 0, Before, F.Error))) return false;
    M.ActiveVersionId = FOGContentId(TEXT("test:presence_other_version"));
    if (!F.Store.UpsertCharacterManifestation(M, 1, F.Error)) return false;
    bool Changed = false;
    TestFalse(TEXT("Version replacement invalidates an old physical writer"), Runtime.CheckpointControlled(*F.Definition, Before, FVector(1,0,0), 1, Changed, F.Error));
    M.OwningRulerId = FOGEntityId::NewId();
    if (!F.Store.UpsertEntity(M.OwningRulerId, TEXT("ruler"), 1, TEXT("{}"), F.Error) ||
        !F.Store.UpsertCharacterManifestation(M, 1, F.Error)) return false;
    TestFalse(TEXT("Foreign ownership is never inferred from possession"), Runtime.Read(*F.Definition, M.ManifestationId, 1, Before, F.Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGControlledPresencePolicyTest,
    "OfflineGame.Foundation.WorldBody.ControlledPresence.SourcePolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGControlledPresencePolicyTest::RunTest(const FString&)
{
    FControlledPresenceFixture F; if (!F.Open()) return false;
    TestTrue(TEXT("Controlled policy needs no invented AI tolerances"), F.Definition->ValidateDefinition(F.Error));
    F.Definition->ScheduleIdField = TEXT("active_schedule");
    TestFalse(TEXT("Controlled policy cannot also drive schedules"), F.Definition->ValidateDefinition(F.Error));
    F.Definition->ScheduleIdField = NAME_None;
    F.Definition->CheckpointIntervalWorldTicks = 0;
    TestFalse(TEXT("Missing source cadence cannot silently pick a default"), F.Definition->ValidateDefinition(F.Error));
    return true;
}
#endif
