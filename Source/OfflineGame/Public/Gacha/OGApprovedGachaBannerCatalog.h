#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gacha/OGGachaDefinitions.h"
#include "OGApprovedGachaBannerCatalog.generated.h"

/** Eligibility is granted only after Gate00/Registry intake and Gate12 asset
 * verification. A design-stage ID or an approved rate table alone is not a
 * playable, installed Gacha banner. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGInstalledScheduledGachaBanner
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gacha|Canonical")
    FOGGachaBannerDefinition Banner;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gacha|Canonical")
    bool bAcceptedInstalledAndActivated = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gacha|Canonical")
    int64 BeginsAtWorldTick = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gacha|Canonical")
    int64 EndsBeforeWorldTick = MAX_int64;
};

/** Source-authored banner inventory, not a second game wallet or Registry.
 * Empty by default. Runtime still validates every exact playable Version,
 * Ruler access, currency payment and persistent Gacha transaction. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGApprovedGachaBannerCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gacha|Canonical")
    TArray<FOGInstalledScheduledGachaBanner> ActiveSchedules;
};
