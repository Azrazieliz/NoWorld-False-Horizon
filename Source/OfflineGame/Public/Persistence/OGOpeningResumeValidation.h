#pragma once
#include "CoreMinimal.h"

/**
 * Read-only validation for the Continue action. A filename on disk is not
 * evidence of a resumable world. Never repairs, migrates, or writes to a save.
 */
namespace FOGOpeningResumeValidation
{
    OFFLINEGAME_API bool HasReadableWorld(const FString& DatabasePath, bool bForceRefresh = false);

    // A first launch may enter Ruler setup without a world file. An existing
    // unreadable world must instead go through recovery, never be replaced.
    OFFLINEGAME_API bool CanEnterRuler(bool bCoreReady, bool bWorldFilePresent, bool bReadableWorld);
}
