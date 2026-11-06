#pragma once
#include "CoreMinimal.h"
#include "Content/OGSystemBindingComponent.h"
#include "Math/OGLargeNumber.h"
#include "OGObjectOfferDefinition.generated.h"

class AOGWorldPrototypeCharacter;

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
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGMerchantRestockRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId RestockId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId OfferActionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEntityId SupplierId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName SupplierKind;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEntityId RouteId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ObjectId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId RankId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId QualityId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumOnHand=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumPerDelivery=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 DeliveryIntervalTicks=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGLargeNumber RouteLoadPerItem;
};

UCLASS(BlueprintType)
class OFFLINEGAME_API UOGObjectOfferDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGObjectOfferDefinition() { System=TEXT("object_offers"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    /** Explicit opt-in for actors that already own a separate World interaction. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText WorldInteractionPrompt;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGObjectOfferSpecification> Offers;
    /** Move existing supplier stock through an authored live route when a market is visited. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGMerchantRestockRule> RestockRules;
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
    static UOGObjectOfferDefinitionReceiver* FindWorldInteraction(AActor* Target, const AOGWorldPrototypeCharacter* Interactor);
    FText GetWorldInteractionPrompt() const;
    FText GetWorldInteractionFeedback() const;
    bool OpenFromWorldInteraction(AOGWorldPrototypeCharacter& Interactor, FString& Error);
protected:
    virtual void TickComponent(float,ELevelTick,FActorComponentTickFunction*) override;
private:
    UPROPERTY(Transient) TObjectPtr<UOGObjectOfferDefinition> Prepared;
    bool bOffersPublished = false;
    FString WorldInteractionError;
    double WorldFeedbackUntil = 0.0;
};
