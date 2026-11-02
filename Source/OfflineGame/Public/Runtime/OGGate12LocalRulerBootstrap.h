#pragma once

#include "CoreMinimal.h"
#include "Core/OGEntityId.h"

class IOGWorldStore;

/**
 * Persist an explicitly created local Ruler and its world-specific active
 * binding in the SAME authoritative SQLite store. This does not create an
 * owned Manifestation, a Character Registry Identity, currency, territory,
 * gacha banners, or a diagnostic/test Ruler.
 *
 * The administrative binding has a reserved internal world-record GUID.
 * It is not a Content ID, Registry ID or owned Manifestation UUID.
 */
struct OFFLINEGAME_API FOGGate12LocalRulerBootstrap
{
    static FOGEntityId LocalBindingRecordId();

    /** Read/validate a previous real local Ruler without ever writing. */
    static bool TryLoad(
        IOGWorldStore& Store,
        bool& bOutFound,
        FOGEntityId& OutRulerId,
        FString& OutError);

    /**
     * Explicit new-game operation. Entire world-store write is transactional,
     * including binding and real Ruler shell; collision/reentry fails closed.
     */
    static bool Create(
        IOGWorldStore& Store,
        int64 CanonicalWorldTick,
        FOGEntityId& OutRulerId,
        FString& OutError);
};
