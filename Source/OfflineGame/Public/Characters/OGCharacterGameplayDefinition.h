#pragma once
#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "Skills/OGResolvedSkillSet.h"
#include "OGCharacterGameplayDefinition.generated.h"

/** Shared kit definition. Actual copies, attained progression and inventory remain canonical Store records. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGCharacterGameplayDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGCharacterGameplayDefinition();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId IdentityId;
    /** DefinitionId is this Version's stable key; it must equal Skills.CurrentForm. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGResolvedSkillSet Skills;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGDefinitionReference> BattleResources;
    /** Optional shared decision policy for an actual non-player body. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGDefinitionReference TurnPolicy;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
    static const UOGCharacterGameplayDefinition* Find(const FOGContentId& Identity,
        const FOGContentId& Version,FString& Error);
    bool MatchesKit(const FOGResolvedSkillSet& Kit) const;
};
