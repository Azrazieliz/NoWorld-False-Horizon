#pragma once

#include "CoreMinimal.h"
#include "Content/OGContentId.h"
#include "Core/OGEntityId.h"

/** Non-canonical per-owner/per-owned-Manifestation presentation preference.
 * Never changes the selected Manifestation, outfit, equipment or world.
 * Missing preference means 2D; missing 2D art is handled by the UI.
 */
class OFFLINEGAME_API FOGGate12CharacterPresentationPreferences
{
public:
    static FString MakeKey(
        const FOGEntityId& RulerId,
        const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId);

    /** One profile read for an entire roster, avoiding disk IO per card. */
    static bool LoadAll(
        const FString& ProfilePath,
        TMap<FString, bool>& OutPreferences,
        FString& OutError);

    static bool Load(
        const FString& ProfilePath,
        const FOGEntityId& RulerId,
        const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId,
        bool& bOutPrefer3D,
        FString& OutError);

    static bool Save(
        const FString& ProfilePath,
        const FOGEntityId& RulerId,
        const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId,
        bool bPrefer3D,
        FString& OutError);
};
