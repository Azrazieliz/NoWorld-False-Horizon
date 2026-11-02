#pragma once

#include "CoreMinimal.h"
#include "Persistence/OGWorldStore.h"

class UOGWorldSimulationDefinition;

/** Read fence only. Canonical records remain in the shared Store. */
struct OFFLINEGAME_API FOGFoundationWorldBodyState
{
    FOGEntityId EntityId;
    FName EntityKind;
    FString EntityJson;
    int64 EntityRevision = 0;
    FOGWorldPresenceRecord Presence;
    /** Controlled mode fences the existing local owner and exact active copy/version. */
    FOGEntityId LocalRulerId;
    bool bHasControlledManifestation = false;
    FOGCharacterManifestationRecord ControlledManifestation;
    bool bAvailable = false;
    bool bAbsent = false;
    bool bHasSchedule = false;
    FOGWorldDirectorScheduleRecord Schedule;
    FOGWorldPresenceRecord Destination;
};

/** Per-call adapter over existing presence and Director services.
 * Persisted schedule provenance supplies resolver_key, definition_id, subject_id,
 * location_id, local_position [x,y,z] and movement_context.
 * Does not create NPCs, schedules, locations, ownership or rewards. */
class OFFLINEGAME_API FOGFoundationWorldSimulationRuntime
{
public:
    explicit FOGFoundationWorldSimulationRuntime(IOGWorldStore& InStore) : Store(InStore) {}
    static FName ResolverKey();
    static FName ArrivalEventType();
    static FName ControlledCheckpointEventType();
    static bool SameAuthority(const FOGFoundationWorldBodyState& A,
        const FOGFoundationWorldBodyState& B);
    bool Read(const UOGWorldSimulationDefinition& Definition, const FOGEntityId& Entity,
        int64 Tick, FOGFoundationWorldBodyState& Out, FString& Error) const;
    bool ActivateDue(const UOGWorldSimulationDefinition& Definition,
        const FOGFoundationWorldBodyState& Expected, int64 Tick, FString& Error);
    /** Rechecks the read fence inside the transaction. Arrival, authored state,
     * Director completion and receipt commit together. Completed replay is inert. */
    bool Checkpoint(const UOGWorldSimulationDefinition& Definition,
        const FOGFoundationWorldBodyState& Expected, const FVector& ActualLocalPosition,
        int64 Tick, bool bArrived, bool& bChanged, FString& Error);
    /** Measured locally controlled motion only; never activates/completes a schedule.
     * The receiver owns physical/controller/preemption validation. This command
     * revalidates canonical presence, local ownership and version inside its root transaction. */
    bool CheckpointControlled(const UOGWorldSimulationDefinition& Definition,
        const FOGFoundationWorldBodyState& Expected, const FVector& ActualLocalPosition,
        int64 Tick, bool& bChanged, FString& Error);
private:
    IOGWorldStore& Store;
};
