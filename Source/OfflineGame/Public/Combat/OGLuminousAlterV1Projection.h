#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"

/** Approved design projection only. Gate11 still owns final Registry intake;
 * neither this data nor its draft ContentIds creates a Manifestation. */
struct OFFLINEGAME_API FOGLuminousAlterV1Projection
{
    FOGCombatStats HeroLevelOne;
    FOGResolvedSkillSet Skills;
    FOGContentId DraftVersionId;
    int32 InitialRedTithe=20;
    int32 RedTitheMaximum=100;

    static FOGLuminousAlterV1Projection FromApprovedDesign();
};
