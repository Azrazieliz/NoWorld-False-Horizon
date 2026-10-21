#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"

/** Exact user-approved Alter v1 design, still pending production Registry.
 * No Manifestation, currency or rarity progression is invented by loading it.
 */
struct OFFLINEGAME_API FOGLuminousAlterV1Projection
{
    FOGCombatStats HeroLevelOne;
    FOGResolvedSkillSet Skills;
    FOGContentId DraftVersionId;
    // Stable executable ID for the independently approved Alter v1.
    // Its exact source JSON still retains the historical draft source key.
    FOGContentId NativeVersionId;
    FOGContentId WorldFantasmId;
    int32 InitialRedTithe=0;
    int32 RedTitheMaximum=0;
    int32 UltimateChargeCapPercent=0;

    /** Fail closed on any changed source bytes or incomplete 3A+6P schema. */
    static bool LoadApprovedV1(
        FOGLuminousAlterV1Projection& Out,
        FString& OutError);
};
