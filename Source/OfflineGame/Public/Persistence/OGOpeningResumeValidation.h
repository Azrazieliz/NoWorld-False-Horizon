#pragma once
#include "CoreMinimal.h"

class IOGWorldStore;

/**
 * Read-only validation for the Continue action. A filename on disk is not
 * evidence of a resumable world. Never repairs, migrates, or writes to a save.
 */
namespace FOGOpeningResumeValidation
{
    OFFLINEGAME_API bool HasReadableWorld(const FString& DatabasePath, bool bForceRefresh = false);

    // The live canonical SQLite store owns a WAL connection. Validate through
    // that already-open store; a separate read-only connection can see a
    // transient busy/inaccessible view while the canonical writer is active.
    // The deep integrity query is reserved for explicit Continue/Enter clicks.
    OFFLINEGAME_API bool HasLiveReadableWorld(
        const IOGWorldStore* Store, const FString& DatabasePath,
        bool bForceIntegrityCheck = false);

    // A first launch may enter Ruler setup without a world file. An existing
    // unreadable world must instead go through recovery, never be replaced.
    OFFLINEGAME_API bool CanEnterRuler(bool bCoreReady, bool bWorldFilePresent, bool bReadableWorld);
}
