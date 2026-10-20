#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"
#include "OGApprovedLuminousSourceKit.generated.h"

/** Verified user-approved Base v6 source data with independently approved
 * Registry design identifiers. These data alone never install a package,
 * grant a Manifestation, change Ruler ownership or activate combat effects. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGApprovedLuminousSourceKitView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FOGContentId IdentityId;

    UPROPERTY(BlueprintReadOnly)
    FOGContentId VersionId;

    UPROPERTY(BlueprintReadOnly)
    FOGCombatStats RankOneHeroStats;

    UPROPERTY(BlueprintReadOnly)
    FOGResolvedSkillSet CoreSkills;

    UPROPERTY(BlueprintReadOnly)
    FOGContentId WorldFantasmId;

    UPROPERTY(BlueprintReadOnly)
    int32 InitialScarletReserve = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ScarletReserveMax = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 UltimateChargeCapPercent = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DayUltimateCostPercent = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 NightUltimateCostPercent = 0;

    /** Exact user-approved source is retained for deterministic action,
     * passive, restoration and World Negation effect dispatch. */
    FString AuthoritativeSourceJson;
};

class OFFLINEGAME_API FOGApprovedLuminousSourceKit
{
public:
    /** Explicit design-source read. NOT a Registry or gameplay grant.
     * Fails closed if original bytes, hash, stats or 3A/6P IDs differ. */
    static bool LoadBaseV6(
        FOGApprovedLuminousSourceKitView& Out,
        FString& OutError);
};
