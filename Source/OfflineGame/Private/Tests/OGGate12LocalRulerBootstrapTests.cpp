#include "Runtime/OGGate12LocalRulerBootstrap.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Persistence/OGSQLiteWorldStore.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12LocalPlayerCreateRestoreTest,
    "OfflineGame.Gate12.Ruler.LocalPlayerCreateRestoreNoFabricatedOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12LocalPlayerCreateRestoreTest::RunTest(const FString&)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(),
        TEXT("Automation"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Database = FPaths::Combine(Directory, TEXT("ruler.db"));

    bool bPassed = true;
    {
        FOGSQLiteWorldStore Store;
        FString Error;
        if (!TestTrue(TEXT("Isolated SQLite fixture opens"), Store.Open(Database, Error)))
        {
            AddError(Error);
            return false;
        }

        bool bFound = false;
        FOGEntityId Existing;
        bPassed &= TestTrue(TEXT("Empty world binding reads without writing"),
            FOGGate12LocalRulerBootstrap::TryLoad(Store, bFound, Existing, Error));
        bPassed &= TestFalse(TEXT("No synthetic Ruler appeared"), bFound);
        bPassed &= TestFalse(TEXT("Empty binding has no owner"), Existing.IsValid());

        FOGEntityId Created;
        bPassed &= TestTrue(TEXT("Explicit new local Ruler transaction succeeds"),
            FOGGate12LocalRulerBootstrap::Create(Store, 17, Created, Error));
        bPassed &= TestTrue(TEXT("New Ruler ID is a genuine generated UUID"), Created.IsValid());
        if (!Error.IsEmpty()) AddError(Error);

        bFound = false;
        Existing = FOGEntityId();
        bPassed &= TestTrue(TEXT("Real local Ruler is recovered from same world"),
            FOGGate12LocalRulerBootstrap::TryLoad(Store, bFound, Existing, Error));
        bPassed &= TestTrue(TEXT("Existing owner is recognized"), bFound);
        bPassed &= TestTrue(TEXT("Recovered owner exactly matches created owner"),
            Existing == Created);

        FOGEntityId Duplicate;
        bPassed &= TestFalse(TEXT("Repeated new-game call must fail closed"),
            FOGGate12LocalRulerBootstrap::Create(Store, 18, Duplicate, Error));
        bPassed &= TestFalse(TEXT("Failed duplicate creates no Ruler ID"), Duplicate.IsValid());

        TArray<FOGCharacterManifestationRecord> Owned;
        bPassed &= TestTrue(TEXT("Owned roster remains queryable"),
            Store.ListCharacterManifestationsByOwner(Created, Owned, Error));
        bPassed &= TestEqual(TEXT("No fabricated Manifestations"), Owned.Num(), 0);
        TArray<FOGTerritoryClaimRecord> Claims;
        bPassed &= TestTrue(TEXT("Canonical territory claims remain queryable"),
            Store.ListTerritoryClaimsByRuler(Created, Claims, Error));
        bPassed &= TestEqual(TEXT("No fabricated controlled territories"), Claims.Num(), 0);
        bool bCurrencyFound = false;
        FOGGachaStateRecord Gacha;
        // There is deliberately no fake pull grant or banner state.
        // The Ruler service's Gacha empty state is independently projected.
        Store.Close();
    }
    {
        FOGSQLiteWorldStore Reopened;
        FString Error;
        bPassed &= TestTrue(TEXT("Persisted world reopened without app-local sidecar"),
            Reopened.Open(Database, Error));
        bool bFound = false;
        FOGEntityId Recovered;
        bPassed &= TestTrue(TEXT("Reload resolves existing Ruler after process/world restart"),
            FOGGate12LocalRulerBootstrap::TryLoad(Reopened, bFound, Recovered, Error));
        bPassed &= TestTrue(TEXT("Reload found owner"), bFound);
        bPassed &= TestTrue(TEXT("Reload preserved owner UUID"), Recovered.IsValid());
        Reopened.Close();
    }

    IFileManager::Get().DeleteDirectory(*Directory, false, true);
    return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12LocalPlayerBindingCollisionTest,
    "OfflineGame.Gate12.Ruler.ReservedBindingCollisionFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12LocalPlayerBindingCollisionTest::RunTest(const FString&)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(),
        TEXT("Automation"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FOGSQLiteWorldStore Store;
    FString Error;
    if (!TestTrue(TEXT("Collision fixture opened"),
            Store.Open(FPaths::Combine(Directory, TEXT("collision.db")), Error)))
        return false;

    const bool bReserved = Store.UpsertEntity(
        FOGGate12LocalRulerBootstrap::LocalBindingRecordId(),
        FName(TEXT("unrelated_existing_data")), 0, TEXT("{}"), Error);
    TestTrue(TEXT("Existing unrelated record staged without alteration"), bReserved);

    bool bFound = false;
    FOGEntityId Owner;
    const bool bRead = FOGGate12LocalRulerBootstrap::TryLoad(
        Store, bFound, Owner, Error);
    TestFalse(TEXT("Foreign owner-binding collision rejected"), bRead);
    TestFalse(TEXT("No stolen owner"), Owner.IsValid());

    FOGEntityId Created;
    TestFalse(TEXT("No overwrite of another world's reserved record"),
        FOGGate12LocalRulerBootstrap::Create(Store, 0, Created, Error));
    TestFalse(TEXT("Failure does not invent an ID"), Created.IsValid());

    bool bSame = false;
    FName Kind;
    FString Payload;
    int64 Revision = -1;
    TestTrue(TEXT("Original record can still be read"), Store.TryReadEntity(
        FOGGate12LocalRulerBootstrap::LocalBindingRecordId(),
        bSame, Kind, Payload, Revision, Error));
    TestTrue(TEXT("Original reserved slot still exists"), bSame);
    TestEqual(TEXT("Foreign record type remains unchanged"),
        Kind, FName(TEXT("unrelated_existing_data")));

    Store.Close();
    IFileManager::Get().DeleteDirectory(*Directory, false, true);
    return true;
}
#endif
