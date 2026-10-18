#pragma once

#include "CoreMinimal.h"

struct FOGFoundationCharacterProjection;

/**
 * Read-only, purely visual adapter. No new combat mechanics, item classes,
 * identity, economy, ownership or equipment schemas. The optional key
 * "combat_motion_profile" is an authored animation binding hint in an
 * existing equipped item's EvolutionStateJson; absent means no inference.
 * Examples of authored values: "sling", "arcane", "longbow", "fist",
 * "energy_cannon", "two_handed_blade". The vocabulary is OPEN, not an enum.
 */
class OFFLINEGAME_API FOGCombatMotionPresentation
{
public:
    /** Returns NAME_None for missing, malformed, broken or conflicting equipment. */
    static FName ResolveAuthoredProfile(const FOGFoundationCharacterProjection& Projection);

    /** Converts an explicit profile into a lookup key; never silently substitutes rifle. */
    static FName ActionClipId(FName Profile, int32 Chain, bool bBasic);

    /** Isolated pure parser, useful to validate authored source without touching a save. */
    static FName ParseProfile(const FString& ExistingEvolutionStateJson);
};
