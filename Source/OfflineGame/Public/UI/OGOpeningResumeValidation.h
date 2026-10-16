#pragma once
#include "CoreMinimal.h"

/**
 * Read-only validation for the Continue action. A filename on disk is not
 * evidence of a resumable world. Never repairs, migrates, or writes to a save.
 */
namespace FOGOpeningResumeValidation
{
    OFFLINEGAME_API bool HasReadableWorld(const FString& DatabasePath);
}
