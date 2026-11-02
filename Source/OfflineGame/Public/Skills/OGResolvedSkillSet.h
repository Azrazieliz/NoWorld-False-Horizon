#pragma once

#include "CoreMinimal.h"
#include "Content/OGContentId.h"
#include "OGResolvedSkillSet.generated.h"

/**
 * Current resolved kit used by combat executors.
 *
 * Learned/evolved skills are integrated into this character-specific kit.
 * The runtime does not expose an endlessly growing bag of duplicate skills.
 */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGResolvedSkillSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGContentId> ActiveSkills;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGContentId> PassiveSkills;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FOGContentId UltimateSkill;

    /** Ordered authored Ultimates. The legacy field may alias only the first entry. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly)
    TArray<FOGContentId> UltimateSkills;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FOGContentId CurrentForm;

    bool HasDuplicateSkills() const;
    TArray<FOGContentId> GetUltimateSkills() const;
    FOGContentId GetPrimaryUltimateSkill() const;
    bool IsUltimate(const FOGContentId& Skill) const;
    bool HasSameUltimates(const FOGResolvedSkillSet& Other) const;
};
