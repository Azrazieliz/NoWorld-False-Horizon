#include "Persistence/OGOpeningResumeValidation.h"
#include "Persistence/OGWorldStore.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "sqlite/sqlite3.h"

namespace FOGOpeningResumeValidation
{
bool HasLiveReadableWorld(const IOGWorldStore* Store,
                          const FString& DatabasePath,
                          bool bForceIntegrityCheck)
{
    // Never confuse an initialized game instance with a readable world. Match
    // the exact active canonical file and reject a closed/missing store.
    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(DatabasePath);
    if (!Store || !Store->IsOpen() ||
        IFileManager::Get().FileSize(*AbsolutePath) < 512 ||
        !FPaths::IsSamePath(
            FPaths::ConvertRelativePathToFull(Store->GetDatabasePath()),
            AbsolutePath))
        return false;

    FString Error;
    if (Store->GetSchemaVersion(Error) <= 0 || !Error.IsEmpty())
        return false;
    if (!bForceIntegrityCheck)
        return true;

    // Read-only PRAGMA executed on the existing canonical connection. No
    // backup, migration, write transaction or Ruler ownership mutation.
    FString Report;
    return Store->RunIntegrityCheck(Report, Error) && Error.IsEmpty() &&
        Report.TrimStartAndEnd().Equals(TEXT("ok"), ESearchCase::IgnoreCase);
}

bool CanEnterRuler(bool bCoreReady, bool bWorldFilePresent, bool bReadableWorld)
{
    return bCoreReady && (!bWorldFilePresent || bReadableWorld);
}

bool HasReadableWorld(const FString& DatabasePath, bool bForceRefresh)
{
    // This is only a read-only resume preflight, not a replacement for the
    // canonical store's migration/version/restore protocol.
    const FString AbsolutePath = FPaths::ConvertRelativePathToFull(DatabasePath);
    const int64 Bytes = IFileManager::Get().FileSize(*AbsolutePath);
    if (Bytes < 512)
    {
        return false;
    }
    // Opening is refreshed while visible: avoid a database integrity scan
    // on every Slate tick. Recheck promptly after save size/mtime changes.
    const FDateTime Modified = IFileManager::Get().GetTimeStamp(*AbsolutePath);
    const double Now = FPlatformTime::Seconds();
    struct FReadinessCache
    {
        FString Path;
        int64 Size = -1;
        FDateTime Stamp;
        double CheckedAt = -1.0;
        bool bPassed = false;
    };
    static FReadinessCache Cached;
    if (!bForceRefresh && Cached.Path == AbsolutePath && Cached.Size == Bytes &&
        Cached.Stamp == Modified && Now - Cached.CheckedAt < 2.0)
    {
        return Cached.bPassed;
    }
    auto StoreResult = [&](bool bPassed)
    {
        Cached.Path = AbsolutePath;
        Cached.Size = Bytes;
        Cached.Stamp = Modified;
        Cached.CheckedAt = Now;
        Cached.bPassed = bPassed;
        return bPassed;
    };

    sqlite3* Database = nullptr;
    FTCHARToUTF8 Utf8Path(*AbsolutePath);
    const int OpenResult = sqlite3_open_v2(
        Utf8Path.Get(), &Database,
        SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);
    if (OpenResult != SQLITE_OK || !Database)
    {
        if (Database) sqlite3_close_v2(Database);
        return StoreResult(false);
    }

    // Fail closed if another writer holds an incompatible lock. Do not create
    // journals, touch original saves, or block the game thread for seconds.
    sqlite3_busy_timeout(Database, 75);
    auto Query = [Database](const char* Sql,
                            TFunctionRef<bool(sqlite3_stmt*)> Check) -> bool
    {
        sqlite3_stmt* Statement = nullptr;
        if (sqlite3_prepare_v2(Database, Sql, -1, &Statement, nullptr) != SQLITE_OK)
        {
            if (Statement) sqlite3_finalize(Statement);
            return false;
        }
        const bool bGood = sqlite3_step(Statement) == SQLITE_ROW &&
            Check(Statement) &&
            sqlite3_step(Statement) == SQLITE_DONE;
        sqlite3_finalize(Statement);
        return bGood;
    };
    const bool bHasFoundationTables = Query(
        "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' "
        "AND name IN ('entities', 'world_events')",
        [](sqlite3_stmt* Statement)
        {
            return sqlite3_column_int(Statement, 0) == 2;
        });
    const bool bHealthy = bHasFoundationTables && Query(
        "PRAGMA quick_check(1)",
        [](sqlite3_stmt* Statement)
        {
            const unsigned char* Value = sqlite3_column_text(Statement, 0);
            return Value && FCStringAnsi::Stricmp(
                reinterpret_cast<const ANSICHAR*>(Value), "ok") == 0;
        });
    sqlite3_close_v2(Database);
    return StoreResult(bHealthy);
}
}
