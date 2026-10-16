#include "UI/OGOpeningResumeValidation.h"

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
        Store.Close();
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
    return bPassed;
}
#endif
