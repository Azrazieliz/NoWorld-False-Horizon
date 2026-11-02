#pragma once
#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "OGCombatResourceDefinition.generated.h"

/** Battle-scoped authored resource; never a second inventory/currency wallet. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGCombatResourceSpecification
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ResourceId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 Maximum=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 OpeningValue=0;
    bool Validate(FString& Error) const;
    bool operator==(const FOGCombatResourceSpecification& Other) const;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGCombatResourceValue
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGContentId ResourceId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) int64 Maximum=0;
    UPROPERTY(BlueprintReadOnly) int64 Value=0;
    bool operator==(const FOGCombatResourceValue& Other) const;
};
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGCombatResourceDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGCombatResourceDefinition();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGCombatResourceSpecification Specification;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};
