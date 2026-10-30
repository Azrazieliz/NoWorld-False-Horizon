#include "Runtime/OGFoundationWorldSimulationRuntime.h"
#include "World/OGWorldSimulationReceiver.h"
#include "Runtime/OGGate12LocalRulerBootstrap.h"
#include "World/OGSharedWorldStateService.h"
#include "World/OGWorldDirectorService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
bool FailFoundationWorldSimulation(FString& Error, const TCHAR* Message) { Error = Message; return false; }
bool ParseFoundationWorldSimulation(const FString& Text, TSharedPtr<FJsonObject>& Out, FString& Error)
{
    if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Out) && Out) return true;
    return FailFoundationWorldSimulation(Error, TEXT("World simulation requires canonical JSON objects."));
}
bool Matches(const FJsonObject& Json, const TMap<FName, FString>& Selector)
{
    for (const auto& Pair : Selector)
    {
        FString Value;
        if (!Json.TryGetStringField(Pair.Key.ToString(), Value) || Value != Pair.Value) return false;
    }
    return true;
}
bool ReadId(const FJsonObject& Json, const TCHAR* Field, FOGEntityId& Out)
{
    FString Text; FGuid Guid;
    if (!Json.TryGetStringField(Field, Text) || !FGuid::ParseFoundationWorldSimulation(Text, Guid) || !Guid.IsValid()) return false;
    Out = FOGEntityId(Guid); return true;
}
bool SamePresence(const FOGWorldPresenceRecord& A, const FOGWorldPresenceRecord& B)
{
    return A.EntityId == B.EntityId && A.LocationId == B.LocationId &&
        A.LocalPosition == B.LocalPosition && A.MovementContext == B.MovementContext &&
        A.UpdatedWorldTick == B.UpdatedWorldTick;
}
class FBodyTransaction
{
public:
    explicit FBodyTransaction(IOGWorldStore& InStore) : Store(InStore) {}
    ~FBodyTransaction() { if (bOpen) { FString Ignored; Store.RollbackTransaction(Ignored); } }
    bool Begin(FString& Error)
    {
        // Body effects must be durable before accepting a new physical read fence.
        if (Store.IsWriteTransactionActive()) return FailFoundationWorldSimulation(Error, TEXT("World body checkpoint requires its own root transaction."));
        bOpen = Store.BeginTransaction(Error); return bOpen;
    }
    bool Commit(FString& Error)
    {
        if (!Store.CommitTransaction(Error)) return false;
        bOpen = false; return true;
    }
private:
    IOGWorldStore& Store;
    bool bOpen = false;
};
}

FName FOGFoundationWorldSimulationRuntime::ResolverKey() { return TEXT("foundation.world_body.navigation"); }
FName FOGFoundationWorldSimulationRuntime::ArrivalEventType() { return TEXT("world.simulation.arrived"); }
FName FOGFoundationWorldSimulationRuntime::ControlledCheckpointEventType() { return TEXT("world.presence.controlled_checkpointed"); }

bool FOGFoundationWorldSimulationRuntime::SameAuthority(
    const FOGFoundationWorldBodyState& A, const FOGFoundationWorldBodyState& B)
{
    return A.EntityId == B.EntityId && A.EntityKind == B.EntityKind &&
        A.EntityRevision == B.EntityRevision && A.EntityJson == B.EntityJson &&
        A.LocalRulerId == B.LocalRulerId && A.bHasControlledManifestation == B.bHasControlledManifestation &&
        (!A.bHasControlledManifestation ||
        (A.ControlledManifestation.ManifestationId == B.ControlledManifestation.ManifestationId &&
        A.ControlledManifestation.OwningRulerId == B.ControlledManifestation.OwningRulerId &&
        A.ControlledManifestation.IdentityId == B.ControlledManifestation.IdentityId &&
        A.ControlledManifestation.ActiveVersionId == B.ControlledManifestation.ActiveVersionId &&
        A.ControlledManifestation.LifecycleState == B.ControlledManifestation.LifecycleState)) &&
        SamePresence(A.Presence, B.Presence) && A.bAvailable == B.bAvailable &&
        A.bAbsent == B.bAbsent && A.bHasSchedule == B.bHasSchedule &&
        (!A.bHasSchedule || (A.Schedule.ScheduleId == B.Schedule.ScheduleId &&
        A.Schedule.Status == B.Schedule.Status && A.Schedule.ContentId == B.Schedule.ContentId &&
        A.Schedule.TemplateId == B.Schedule.TemplateId &&
        A.Schedule.ScheduledStartWorldTick == B.Schedule.ScheduledStartWorldTick &&
        A.Schedule.bHasScheduledStartWorldTick == B.Schedule.bHasScheduledStartWorldTick &&
        A.Schedule.DecisionProvenanceJson == B.Schedule.DecisionProvenanceJson));
}

bool FOGFoundationWorldSimulationRuntime::Read(const UOGWorldSimulationDefinition& D,
    const FOGEntityId& Entity, int64 Tick, FOGFoundationWorldBodyState& Out, FString& Error) const
{
    Out = FOGFoundationWorldBodyState(); Error.Reset();
    if (!Store.IsOpen() || !Entity.IsValid() || Tick < 0 || !D.ValidateDefinition(Error))
        return FailFoundationWorldSimulation(Error, TEXT("World body needs a ready canonical Store, entity, tick and authored definition."));
    FOGFoundationWorldBodyState State; State.EntityId = Entity;
    bool Found = false;
    if (!Store.TryReadEntity(Entity, Found, State.EntityKind, State.EntityJson, State.EntityRevision, Error)) return false;
    TSharedPtr<FJsonObject> Json;
    if (!Found || State.EntityKind != D.EntityKind || !ParseFoundationWorldSimulation(State.EntityJson, Json, Error) ||
        Json->HasField(TEXT("diagnosticFixture")))
        return FailFoundationWorldSimulation(Error, TEXT("World body must bind an existing entity of the definition's exact kind."));
    if (!FOGSharedWorldStateService(Store).TryReadPhysicalPresence(Entity, Found, State.Presence, Error)) return false;
    if (!Found || State.Presence.LocalPosition.ContainsNaN() ||
        State.Presence.UpdatedWorldTick < 0 || State.Presence.UpdatedWorldTick > Tick)
        return FailFoundationWorldSimulation(Error, TEXT("World body is missing a valid canonical physical presence."));
    FOGLocationRecord Location;
    if (!Store.TryReadLocation(State.Presence.LocationId, Found, Location, Error)) return false;
    if (!Found) return FailFoundationWorldSimulation(Error, TEXT("Canonical presence refers to a missing Location."));
    State.bAbsent = !D.AbsentStateValues.IsEmpty() && Matches(*Json, D.AbsentStateValues);
    State.bAvailable = !State.bAbsent && Matches(*Json, D.RequiredStateValues) &&
        Location.bPhysicallyAccessible && State.Presence.MovementContext == D.MovementContext;
    if (D.bControlledPresence)
    {
        bool HasLocalRuler = false;
        if (!FOGGate12LocalRulerBootstrap::TryLoad(Store, HasLocalRuler, State.LocalRulerId, Error)) return false;
        if (!HasLocalRuler)
            return FailFoundationWorldSimulation(Error, TEXT("Controlled presence requires the existing world-embedded local Ruler binding."));
        if (Entity != State.LocalRulerId)
        {
            if (!Store.TryReadCharacterManifestation(Entity, State.bHasControlledManifestation,
                State.ControlledManifestation, Error)) return false;
            const auto& M = State.ControlledManifestation;
            if (!State.bHasControlledManifestation || M.ManifestationId != Entity ||
                M.OwningRulerId != State.LocalRulerId || M.LifecycleState != TEXT("active") ||
                !M.IdentityId.IsValid() || !M.ActiveVersionId.IsValid())
                return FailFoundationWorldSimulation(Error, TEXT("Controlled presence needs an exact active Manifestation owned by the stored local Ruler."));
        }
        // No Director read or route inference for genuine player-controlled presence.
        Out = MoveTemp(State); return true;
    }
    const FString Field = D.ScheduleIdField.ToString();
    if (!Json->HasField(Field)) { Out = MoveTemp(State); return true; }
    FString ScheduleText;
    if (!Json->TryGetStringField(Field, ScheduleText))
        return FailFoundationWorldSimulation(Error, TEXT("Canonical schedule selector must be an exact persisted schedule ID string."));
    if (ScheduleText.IsEmpty()) { Out = MoveTemp(State); return true; }
    FGuid Guid;
    if (!FGuid::ParseFoundationWorldSimulation(ScheduleText, Guid) || !Guid.IsValid())
        return FailFoundationWorldSimulation(Error, TEXT("Canonical schedule selector contains an invalid ID."));
    if (!Store.TryReadWorldDirectorSchedule(FOGEntityId(Guid), Found, State.Schedule, Error)) return false;
    if (!Found) return FailFoundationWorldSimulation(Error, TEXT("Canonical NPC schedule is not installed in the shared Director."));
    State.bHasSchedule = true;
    TSharedPtr<FJsonObject> Provenance;
    if (!ParseFoundationWorldSimulation(State.Schedule.DecisionProvenanceJson, Provenance, Error)) return false;
    FString Resolver, Definition, Context; FOGEntityId Subject;
    if (!Provenance->TryGetStringField(TEXT("resolver_key"), Resolver) || Resolver != ResolverKey().ToString() ||
        !Provenance->TryGetStringField(TEXT("definition_id"), Definition) || Definition != D.DefinitionId.ToString() ||
        !ReadId(*Provenance, TEXT("subject_id"), Subject) || Subject != Entity ||
        !ReadId(*Provenance, TEXT("location_id"), State.Destination.LocationId) ||
        !Provenance->TryGetStringField(TEXT("movement_context"), Context) || Context != D.MovementContext.ToString())
        return FailFoundationWorldSimulation(Error, TEXT("Director movement provenance must bind this exact subject, definition, location and movement context."));
    const TArray<TSharedPtr<FJsonValue>>* Position = nullptr;
    double XYZ[3] = {};
    if (!Provenance->TryGetArrayField(TEXT("local_position"), Position) || Position->Num() != 3)
        return FailFoundationWorldSimulation(Error, TEXT("Director destination needs three finite location-local coordinates."));
    for (int32 I = 0; I < 3; ++I)
        if (!(*Position)[I].IsValid() || !(*Position)[I]->TryGetNumber(XYZ[I]) || !FMath::IsFinite(XYZ[I]))
            return FailFoundationWorldSimulation(Error, TEXT("Director destination coordinates must be finite numbers."));
    State.Destination.EntityId = Entity;
    State.Destination.LocalPosition = FVector(XYZ[0], XYZ[1], XYZ[2]);
    State.Destination.MovementContext = D.MovementContext;
    if (State.Destination.LocationId != State.Presence.LocationId)
        return FailFoundationWorldSimulation(Error, TEXT("Cross-Location travel requires an authored transition; this receiver never teleports a route."));
    if (!State.Schedule.bHasScheduledStartWorldTick || State.Schedule.ScheduledStartWorldTick < 0 ||
        (State.Schedule.Status != TEXT("scheduled") && State.Schedule.Status != TEXT("active") &&
        State.Schedule.Status != TEXT("completed")))
        return FailFoundationWorldSimulation(Error, TEXT("World body requires a bounded scheduled, active or completed Director record."));
    Out = MoveTemp(State); return true;
}

bool FOGFoundationWorldSimulationRuntime::ActivateDue(const UOGWorldSimulationDefinition& D,
    const FOGFoundationWorldBodyState& Expected, int64 Tick, FString& Error)
{
    Error.Reset();
    if (D.bControlledPresence) return FailFoundationWorldSimulation(Error, TEXT("Controlled presence cannot activate AI schedules."));
    FBodyTransaction Transaction(Store);
    if (!Transaction.Begin(Error)) return false;
    FOGFoundationWorldBodyState Current;
    if (!Read(D, Expected.EntityId, Tick, Current, Error)) return false;
    if (!SameAuthority(Current, Expected) || !Current.bAvailable || !Current.bHasSchedule ||
        Tick < Current.Schedule.ScheduledStartWorldTick)
        return FailFoundationWorldSimulation(Error, TEXT("Canonical state changed, is unavailable, or the NPC schedule is not due."));
    if (Current.Schedule.Status == TEXT("scheduled") &&
        !FOGWorldDirectorService(Store).ActivateDueContent(Current.Schedule.ScheduleId, Tick, Error)) return false;
    return Transaction.Commit(Error);
}

bool FOGFoundationWorldSimulationRuntime::Checkpoint(const UOGWorldSimulationDefinition& D,
    const FOGFoundationWorldBodyState& Expected, const FVector& Actual, int64 Tick,
    bool bArrived, bool& bChanged, FString& Error)
{
    bChanged = false; Error.Reset();
    if (D.bControlledPresence) return FailFoundationWorldSimulation(Error, TEXT("Controlled presence cannot checkpoint AI movement or arrival."));
    if (Actual.ContainsNaN()) return FailFoundationWorldSimulation(Error, TEXT("Actual body position must be finite."));
    FBodyTransaction Transaction(Store);
    if (!Transaction.Begin(Error)) return false;
    FOGFoundationWorldBodyState Current;
    if (!Read(D, Expected.EntityId, Tick, Current, Error)) return false;
    // Status is the canonical replay fence, not the lifetime of a component or event callback.
    if (Current.bHasSchedule && Expected.bHasSchedule &&
        Current.Schedule.ScheduleId == Expected.Schedule.ScheduleId && Current.Schedule.Status == TEXT("completed"))
        return Transaction.Commit(Error);
    if (!SameAuthority(Current, Expected) || !Current.bAvailable || !Current.bHasSchedule ||
        Current.Schedule.Status != TEXT("active") || Tick < Current.Schedule.ScheduledStartWorldTick)
        return FailFoundationWorldSimulation(Error, TEXT("Obsolete or unavailable World body cannot overwrite canonical presence or activity."));
    if (bArrived && FVector::Dist(Actual, Current.Destination.LocalPosition) > D.AcceptanceRadiusCm)
        return FailFoundationWorldSimulation(Error, TEXT("Path result did not physically reach the authored destination."));
    if (!bArrived && Actual == Current.Presence.LocalPosition) return Transaction.Commit(Error);
    auto Presence = Current.Presence; Presence.LocalPosition = Actual; Presence.UpdatedWorldTick = Tick;
    if (!FOGSharedWorldStateService(Store).UpdatePhysicalPresence(Presence, Error)) return false;
    if (bArrived && !D.ArrivalStateField.IsNone())
    {
        TSharedPtr<FJsonObject> State;
        if (!ParseFoundationWorldSimulation(Current.EntityJson, State, Error)) return false;
        State->SetStringField(D.ArrivalStateField.ToString(), D.ArrivalStateValue);
        FString Text; FJsonSerializer::Serialize(State.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
        if (!Store.UpsertEntity(Current.EntityId, Current.EntityKind, Tick, Text, Error)) return false;
    }
    auto Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("schedule_id"), Current.Schedule.ScheduleId.ToString());
    Payload->SetStringField(TEXT("definition_id"), D.DefinitionId.ToString());
    Payload->SetStringField(TEXT("location_id"), Presence.LocationId.ToString());
    Payload->SetNumberField(TEXT("local_x"), Actual.X); Payload->SetNumberField(TEXT("local_y"), Actual.Y);
    Payload->SetNumberField(TEXT("local_z"), Actual.Z);
    FString Text; FJsonSerializer::Serialize(Payload, TJsonWriterFactory<>::Create(&Text));
    if (bArrived && !FOGWorldDirectorService(Store).CompleteSchedule(Current.Schedule.ScheduleId, Tick, Text, Error)) return false;
    FOGWorldEvent Event; Event.EventId = FOGEntityId::NewId(); Event.WorldTick = Tick;
    Event.EventType = bArrived ? ArrivalEventType() : FName(TEXT("world.simulation.presence_checkpointed"));
    Event.PrimaryEntity = Current.EntityId; Event.RelatedEntities.Add(Current.Schedule.ScheduleId);
    Event.PayloadJson = Text; Event.bChronicleEligible = bArrived;
    if (!Store.AppendWorldEvent(Event, Error) || !Transaction.Commit(Error)) return false;
    bChanged = true; return true;
}

bool FOGFoundationWorldSimulationRuntime::CheckpointControlled(const UOGWorldSimulationDefinition& D,
    const FOGFoundationWorldBodyState& Expected, const FVector& Actual, int64 Tick,
    bool& bChanged, FString& Error)
{
    bChanged = false; Error.Reset();
    if (!D.bControlledPresence || Actual.ContainsNaN())
        return FailFoundationWorldSimulation(Error, TEXT("Controlled checkpoint requires its explicit source policy and finite measured position."));
    FBodyTransaction Transaction(Store);
    if (!Transaction.Begin(Error)) return false;
    FOGFoundationWorldBodyState Current;
    if (!Read(D, Expected.EntityId, Tick, Current, Error)) return false;
    if (!SameAuthority(Current, Expected) || !Current.bAvailable || Current.bHasSchedule ||
        !Current.LocalRulerId.IsValid())
        return FailFoundationWorldSimulation(Error, TEXT("Changed canonical position, availability, local owner or active version invalidates this physical writer."));
    if (Actual == Current.Presence.LocalPosition) return Transaction.Commit(Error);
    auto Presence = Current.Presence; Presence.LocalPosition = Actual; Presence.UpdatedWorldTick = Tick;
    if (!FOGSharedWorldStateService(Store).UpdatePhysicalPresence(Presence, Error)) return false;
    auto Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("definition_id"), D.DefinitionId.ToString());
    Payload->SetStringField(TEXT("location_id"), Presence.LocationId.ToString());
    Payload->SetNumberField(TEXT("local_x"), Actual.X); Payload->SetNumberField(TEXT("local_y"), Actual.Y);
    Payload->SetNumberField(TEXT("local_z"), Actual.Z);
    FString Text; FJsonSerializer::Serialize(Payload, TJsonWriterFactory<>::Create(&Text));
    FOGWorldEvent Event; Event.EventId = FOGEntityId::NewId(); Event.WorldTick = Tick;
    Event.EventType = ControlledCheckpointEventType(); Event.PrimaryEntity = Current.EntityId;
    Event.PayloadJson = Text; Event.bChronicleEligible = false;
    if (!Store.AppendWorldEvent(Event, Error) || !Transaction.Commit(Error)) return false;
    bChanged = true; return true;
}
