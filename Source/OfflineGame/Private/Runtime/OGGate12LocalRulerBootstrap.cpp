#include "Runtime/OGGate12LocalRulerBootstrap.h"

#include "Dom/JsonObject.h"
#include "Persistence/OGWorldStore.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FOGEntityId FOGGate12LocalRulerBootstrap::LocalBindingRecordId()
{
    // Internal administrative slot ONLY. Not a Character, Ruler, Registry or
    // owned-Manifestation ID. Persisted with the world so backup/restore works.
    return FOGEntityId(FGuid(
        0x4E4F574F, 0x524C4452, 0x4C4F4341, 0x4C534C54));
}

bool FOGGate12LocalRulerBootstrap::TryLoad(
    IOGWorldStore& Store, bool& bOutFound,
    FOGEntityId& OutRulerId, FString& OutError)
{
    bOutFound = false;
    OutRulerId = FOGEntityId();
    OutError.Reset();

    if (!Store.IsOpen())
    {
        OutError = TEXT("Canonical world store is not open.");
        return false;
    }
    bool bBindingFound = false;
    FName BindingKind;
    FString BindingState;
    int64 BindingRevision = 0;
    if (!Store.TryReadEntity(LocalBindingRecordId(),
            bBindingFound, BindingKind, BindingState,
            BindingRevision, OutError))
        return false;
    if (!bBindingFound)
        return true; // New world: nothing silently created.

    if (BindingKind != FName(TEXT("local_ruler_binding")))
    {
        OutError = TEXT("Reserved local player binding ID collision; refusing overwrite.");
        return false;
    }

    TSharedPtr<FJsonObject> Json;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(BindingState);
    FString RawOwner, Contract;
    if (!FJsonSerializer::Deserialize(Reader, Json) ||
        !Json.IsValid() ||
        !Json->TryGetStringField(TEXT("rulerId"), RawOwner) ||
        !Json->TryGetStringField(TEXT("contract"), Contract) ||
        Contract != TEXT("gate12_local_ruler_v1"))
    {
        OutError = TEXT("Local Ruler binding is invalid or from an unknown contract.");
        return false;
    }

    FGuid Parsed;
    if (!FGuid::Parse(RawOwner, Parsed) || !Parsed.IsValid())
    {
        OutError = TEXT("Local player binding contains an invalid Ruler GUID.");
        return false;
    }
    const FOGEntityId Candidate(Parsed);
    bool bOwnerFound = false;
    FName OwnerKind;
    FString OwnerState;
    int64 OwnerRevision = 0;
    if (!Store.TryReadEntity(Candidate, bOwnerFound, OwnerKind,
            OwnerState, OwnerRevision, OutError))
        return false;

    if (!bOwnerFound || OwnerKind != FName(TEXT("ruler")) ||
        OwnerState.Contains(TEXT("\"diagnosticFixture\"")))
    {
        OutError = TEXT("Bound local player is not an existing canonical Ruler.");
        return false;
    }
    bOutFound = true;
    OutRulerId = Candidate;
    return true;
}

bool FOGGate12LocalRulerBootstrap::Create(
    IOGWorldStore& Store, int64 CanonicalWorldTick,
    FOGEntityId& OutRulerId, FString& OutError)
{
    OutRulerId = FOGEntityId();
    OutError.Reset();
    if (!Store.IsOpen() || CanonicalWorldTick < 0)
    {
        OutError = TEXT("Cannot create a Ruler without an open canonical world and valid tick.");
        return false;
    }
    bool bExisting = false;
    FOGEntityId ExistingOwner;
    if (!TryLoad(Store, bExisting, ExistingOwner, OutError))
        return false;
    if (bExisting)
    {
        OutError = TEXT("A local Ruler already exists; use Continue instead of creating duplicates.");
        return false;
    }

    const FOGEntityId NewRuler = FOGEntityId::NewId();
    if (!NewRuler.IsValid() || NewRuler == LocalBindingRecordId())
    {
        OutError = TEXT("Generated Ruler ID is invalid or reserved.");
        return false;
    }
    bool bCollision = false;
    FName ExistingKind;
    FString ExistingState;
    int64 ExistingRevision = 0;
    if (!Store.TryReadEntity(NewRuler, bCollision, ExistingKind,
            ExistingState, ExistingRevision, OutError))
        return false;
    if (bCollision)
    {
        OutError = TEXT("New Ruler GUID collides with an existing world entity.");
        return false;
    }

    if (!Store.BeginTransaction(OutError))
        return false;

    auto RollBack = [&Store, &OutError]()
    {
        FString RollbackError;
        if (!Store.RollbackTransaction(RollbackError) && OutError.IsEmpty())
            OutError = RollbackError;
        return false;
    };

    // A real locally-created Ruler, not test fixture ownership.
    // Deliberately NO territories, currency, characters or banner records.
    if (!Store.UpsertEntity(NewRuler, FName(TEXT("ruler")),
            CanonicalWorldTick,
            TEXT("{\"origin\":\"explicit_local_creation\",\"schema\":1}"),
            OutError))
        return RollBack();

    const FString BindingJson = FString::Printf(
        TEXT("{\"contract\":\"gate12_local_ruler_v1\",\"rulerId\":\"%s\"}"),
        *NewRuler.ToString());
    if (!Store.UpsertEntity(LocalBindingRecordId(),
            FName(TEXT("local_ruler_binding")),
            CanonicalWorldTick, BindingJson, OutError))
        return RollBack();

    if (!Store.CommitTransaction(OutError))
        return RollBack();

    bool bLoaded = false;
    FOGEntityId Verified;
    if (!TryLoad(Store, bLoaded, Verified, OutError) ||
        !bLoaded || Verified != NewRuler)
    {
        if (OutError.IsEmpty())
            OutError = TEXT("Persisted Ruler could not be verified after commit.");
        return false;
    }
    OutRulerId = NewRuler;
    return true;
}
