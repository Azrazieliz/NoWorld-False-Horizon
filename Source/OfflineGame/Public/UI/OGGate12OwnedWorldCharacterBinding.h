#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/OGEntityId.h"
#include "Content/OGContentId.h"
#include "OGGate12OwnedWorldCharacterBinding.generated.h"

/** The authored World playable actor must bind the exact live persisted
 * Ruler -> Identity -> Version -> owned Manifestation, and restore the
 * genuine character kit, wardrobe, rig, camera and Privacy rendering.
 * No QA fixture, generic pawn, fake owner or runtime mutation of ownership.
 */
UINTERFACE(BlueprintType)
class OFFLINEGAME_API UOGGate12OwnedWorldCharacterBinding : public UInterface
{
    GENERATED_BODY()
};

class OFFLINEGAME_API IOGGate12OwnedWorldCharacterBinding
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable,
        Category="OfflineGame|Character|World")
    bool BindOwnedWorldCharacter(
        const FOGEntityId& RulerId,
        const FOGContentId& IdentityId,
        const FOGContentId& VersionId,
        const FOGEntityId& ManifestationId,
        bool bSfwPresentation);
};
