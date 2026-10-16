#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Content/OGContentId.h"
#include "OGGate12CharacterVisualCatalog.generated.h"

class UTexture2D;
class AActor;
class AOGWorldPrototypeCharacter;

/** Verified presentation assets from an installed, Gate11-approved version.
 * Records contain no invented ownership or automatic Identity aliases.
 * A 2D portrait may be absent until user artwork is delivered.
 */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGGate12CharacterVisualEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    FOGContentId IdentityId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    FOGContentId VersionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    TSoftObjectPtr<UTexture2D> OriginalArtwork2D;

    /** Optional SFW-approved artwork, no unsafe fallback when Privacy is on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    TSoftObjectPtr<UTexture2D> ApprovedSfwArtwork2D;

    /** Optional typed wardrobe choices authored per Version by Gate01/11. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    FName WardrobeStateJsonField = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    TArray<FName> ApprovedWardrobeChoices;

    /** Authored actor containing the full real skeletal character and outfit,
     * including any modular parts. Never substitute a generic mannequin. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    TSoftClassPtr<AActor> GenuineCharacterPreviewClass;

    /** Canonical World pawn for this exact Version, not a generic replacement.
     * Must be a real AOGWorldPrototypeCharacter-derived authored pawn and
     * implement IOGGate12OwnedWorldCharacterBinding. Optional until Gate11
     * accepts the installed character package. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    TSoftClassPtr<AOGWorldPrototypeCharacter> ApprovedWorldPlayableClass;
};

/** Per-version asset manifest. Edited in Unreal; never changes the save database. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGGate12CharacterVisualCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character Presentation")
    TArray<FOGGate12CharacterVisualEntry> Entries;

    /** Fails closed for missing and duplicate entries. */
    const FOGGate12CharacterVisualEntry* FindExact(
        const FOGContentId& IdentityId, const FOGContentId& VersionId) const;
};
