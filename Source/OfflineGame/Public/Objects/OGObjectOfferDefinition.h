#pragma once
#include "CoreMinimal.h"
#include "Content/OGSystemBindingComponent.h"
#include "OGObjectOfferDefinition.generated.h"

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectOfferSpecification
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ActionId;
    /** Explicit allocation identity. Rebinding/streaming never replenishes this allocation. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId PublicationId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEntityId AllowedRulerId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 RemainingUses=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 OpensTick=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 ClosesTick=-1;
};
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGObjectOfferDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGObjectOfferDefinition() { System=TEXT("object_offers"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGObjectOfferSpecification> Offers;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
};
/** Shared NPC/world profile adapter. Activation performs no canonical writes;
 * the subsequent native producer tick publishes sourced allocations once. */
UCLASS(ClassGroup=(OfflineGame))
class OFFLINEGAME_API UOGObjectOfferDefinitionReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGObjectOfferDefinitionReceiver();
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) FString Status;
    virtual bool PrepareDefinition_Implementation(UDataAsset*,const FOGDefinitionBindingContext&,FString&) override;
    virtual void ActivateDefinition_Implementation() override;
    virtual void ReleaseDefinition_Implementation() override;
protected:
    virtual void TickComponent(float,ELevelTick,FActorComponentTickFunction*) override;
private:
    UPROPERTY(Transient) TObjectPtr<UOGObjectOfferDefinition> Prepared;
};
