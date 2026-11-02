#pragma once
#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "Effects/OGNativeEffectTypes.h"
#include "OGNativeCombatEffect.generated.h"

/** Register once in the shared effect library; abilities reference its stable definition ID. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGNativeCombatEffect : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGNativeCombatEffect() { System=TEXT("effect"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGNativeEffectDefinition Specification;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};
