#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/OGEntityId.h"
#include "Content/OGContentId.h"
#include "OGGate12CharacterPreviewConfiguration.generated.h"

/** A live 3D preview actor MUST bind the same persisted Manifestation as
 * the World pawn. Implementers read canonical appearance/wardrobe state
 * through the existing read-only presentation services. They may never
 * modify physical wardrobe, gameplay, or the owner.
 */
UINTERFACE(BlueprintType)
class OFFLINEGAME_API UOGGate12CharacterPreviewConfiguration : public UInterface
{
    GENERATED_BODY()
};

class OFFLINEGAME_API IOGGate12CharacterPreviewConfiguration
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable,
        Category="OfflineGame|Character|Preview")
    bool ConfigureOwnedPreview(
        const FOGEntityId& RulerId,
        const FOGContentId& IdentityId,
        const FOGContentId& VersionId,
        const FOGEntityId& ManifestationId);
};
