#include "Persistence/OGOpeningResumeValidation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "sqlite/sqlite3.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12OpeningResumeIntegrityTest,
    "OfflineGame.Gate12.Opening.ContinueRequiresReadableCanonicalWorld",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12OpeningResumeIntegrityTest::RunTest(const FString&)
{
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString ValidDb = FPaths::Combine(Directory, TEXT("valid_world.db"));
    const FString CorruptDb = FPaths::Combine(Directory, TEXT("corrupt_world.db"));
    const FString EmptySchemaDb = FPaths::Combine(Directory, TEXT("empty_schema.db"));
    bool bPassed = true;
    bPassed &= TestFalse(TEXT("No world file cannot enable Continue"),
        FOGOpeningResumeValidation::HasReadableWorld(ValidDb));

    {
        FOGSQLiteWorldStore Store;
        FString Error;
        if (!TestTrue(TEXT("Real isolated Foundation world opens"),
            Store.Open(ValidDb, Error)))
        {
            AddError(Error);
            return false;
        }
        bPassed &= TestTrue(TEXT("Live WAL world passes canonical store integrity"),
            FOGOpeningResumeValidation::HasLiveReadableWorld(&Store, ValidDb, true));
        bPassed &= TestFalse(TEXT("Live store cannot validate unrelated save path"),
            FOGOpeningResumeValidation::HasLiveReadableWorld(
                &Store, FPaths::Combine(Directory, TEXT("not_this_world.db")), true));
        Store.Close();
        bPassed &= TestFalse(TEXT("Closed store cannot enable opening Continue"),
            FOGOpeningResumeValidation::HasLiveReadableWorld(&Store, ValidDb, true));
        const int64 BytesBefore = IFileManager::Get().FileSize(*ValidDb);
        const FDateTime StampBefore = IFileManager::Get().GetTimeStamp(*ValidDb);
        bPassed &= TestTrue(TEXT("Read-only validator accepts real schema"),
            FOGOpeningResumeValidation::HasReadableWorld(ValidDb));
        bPassed &= TestEqual(TEXT("Validator never changes database size"),
            IFileManager::Get().FileSize(*ValidDb), BytesBefore);
        bPassed &= TestTrue(TEXT("Validator never changes database timestamp"),
            IFileManager::Get().GetTimeStamp(*ValidDb) == StampBefore);
    }
    {
        const FString Garbage = FString::ChrN(1024, TEXT('x'));
        bPassed &= TestTrue(TEXT("Create isolated invalid database fixture"),
            FFileHelper::SaveStringToFile(Garbage, *CorruptDb));
        bPassed &= TestFalse(TEXT("Corrupt non-SQLite bytes cannot enable Continue"),
            FOGOpeningResumeValidation::HasReadableWorld(CorruptDb));
    }
    {
        // A structurally valid SQLite database without Foundation tables
        // must not enable Continue or cause any migration.
        sqlite3* Db = nullptr;
        FTCHARToUTF8 P(*EmptySchemaDb);
        bPassed &= TestEqual(TEXT("Create independent empty schema fixture"),
            sqlite3_open_v2(P.Get(), &Db,
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr), SQLITE_OK);
        if (Db)
        {
            sqlite3_exec(Db, "CREATE TABLE unrelated (id INTEGER);", nullptr,
                         nullptr, nullptr);
            sqlite3_close_v2(Db);
        }
        bPassed &= TestFalse(TEXT("Unrelated SQLite schema cannot enable Continue"),
            FOGOpeningResumeValidation::HasReadableWorld(EmptySchemaDb));
    }

    // Entry is legal on first install or a healthy unowned world; it must
    // never create/replace an existing invalid save or bypass a failed core.
    bPassed &= TestTrue(TEXT("Fresh install may enter Ruler setup"),
        FOGOpeningResumeValidation::CanEnterRuler(true, false, false));
    bPassed &= TestTrue(TEXT("Healthy world without Ruler may enter setup"),
        FOGOpeningResumeValidation::CanEnterRuler(true, true, true));
    bPassed &= TestFalse(TEXT("Corrupt existing world blocks Ruler setup"),
        FOGOpeningResumeValidation::CanEnterRuler(true, true, false));
    bPassed &= TestFalse(TEXT("Unready core blocks Ruler setup"),
        FOGOpeningResumeValidation::CanEnterRuler(false, false, false));

    // A click must bypass the presentation cache even if a replacement has
    // identical size and timestamp. Never trust a recent successful paint.
    const int64 ValidSize = IFileManager::Get().FileSize(*ValidDb);
    const FDateTime ValidStamp = IFileManager::Get().GetTimeStamp(*ValidDb);
    FOGOpeningResumeValidation::HasReadableWorld(ValidDb, true);
    TArray<uint8> Replacement;
    Replacement.Init(static_cast<uint8>('x'), static_cast<int32>(ValidSize));
    bPassed &= TestTrue(TEXT("Replace isolated fixture with same-sized corruption"),
        FFileHelper::SaveArrayToFile(Replacement, *ValidDb));
    IFileManager::Get().SetTimeStamp(*ValidDb, ValidStamp);
    bPassed &= TestFalse(TEXT("Action preflight bypasses cached valid result"),
        FOGOpeningResumeValidation::HasReadableWorld(ValidDb, true));
    return bPassed;
}
#endif
