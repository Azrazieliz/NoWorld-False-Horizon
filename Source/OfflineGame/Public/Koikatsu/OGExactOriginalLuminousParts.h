#pragma once
#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

// Exact source-card imported and verified from Koikatsu -> Blender -> UE 5.8.
// No placeholder, no substitution, no fifth ordinary lineage.
// These entries affect rendered parts, NOT the shared animation library.
struct OFFLINEGAME_API FOGExactOriginalLuminousParts
{
    static bool GetClothed(const FString& VersionId,
        TArray<FSoftObjectPath>& OutPaths, int32& OutSourceBones);
    static bool IsInstalled(const FString& VersionId);
};
