#pragma once

#include "CoreMinimal.h"

struct FOGFoundationCharacterProjection;

/**
 * Read-only, purely visual adapter. No new combat mechanics, item classes,
 * identity, economy, ownership or equipment schemas. The optional key
 * "combat_motion_profile" is an authored animation binding hint in an
 * existing equipped item's EvolutionStateJson. The SAME optional key may
 * also exist on the canonical character's EntityStateJson for explicitly
 * authored natural/unarmed, magic or anatomy-specific actions. Equipment
 * takes priority; conflicting equipped profiles fail closed. Neither source
 * is guessed from a character/item name; absent means no inference.
 * Examples of authored values: "sling", "arcane", "longbow", "fist",
 * "energy_cannon", "two_handed_blade". The vocabulary is OPEN, not an enum.
 */
class OFFLINEGAME_API FOGCombatMotionPresentation
{
public:
    /** Equipment first; optional canonical innate style when no item style is bound.
     * Conflicting usable item profiles return None even if an innate style exists. */
    static FName ResolveAuthoredProfile(const FOGFoundationCharacterProjection& Projection);

    /** Converts an explicit profile into a lookup key; never silently substitutes rifle. */
    static FName ActionClipId(FName Profile, int32 Chain, bool bBasic,
        bool bUltimate = false);

    /** Exact authored semantic SFX key; no global weapon/gun sound guess. */
    static FName SemanticActionSoundId(FName Profile, bool bBasic,
        bool bUltimate = false);

    /** Isolated pure parser, useful to validate authored source without touching a save. */
    static FName ParseProfile(const FString& ExistingEvolutionStateJson);
};
