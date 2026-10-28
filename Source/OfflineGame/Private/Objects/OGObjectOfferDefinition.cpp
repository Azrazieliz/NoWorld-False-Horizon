#include "Objects/OGObjectOfferDefinition.h"
#include "Objects/OGObjectActionService.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/SecureHash.h"

bool UOGObjectOfferDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("object_offers") || OriginalSourceReference.TrimStartAndEnd().IsEmpty() || Offers.IsEmpty())
    { Error=TEXT("An object offer set requires sourced allocations."); return false; }
    TSet<FOGContentId> Actions,Publications;
    for (const auto& Offer:Offers)
    {
        if (!Offer.ActionId.IsValid() || !Offer.PublicationId.IsValid() || Actions.Contains(Offer.ActionId) ||
            Publications.Contains(Offer.PublicationId) || Offer.RemainingUses < -1 || Offer.OpensTick<0 ||
            (Offer.ClosesTick!=-1 && Offer.ClosesTick<=Offer.OpensTick))
        { Error=TEXT("Offer allocations need distinct real action/publication IDs and explicit count/time bounds."); return false; }
        Actions.Add(Offer.ActionId); Publications.Add(Offer.PublicationId);
    }
    return true;
}
void UOGObjectOfferDefinition::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{
    Super::CollectRequiredDefinitions(Out);
    for (const auto& Offer:Offers)
    { FOGDefinitionReference Ref; Ref.System=TEXT("object_action"); Ref.Id=Offer.ActionId; Out.Add(Ref); }
}
bool UOGObjectOfferDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    for (const auto& Offer:Offers)
    {
        const auto* Action=Cast<UOGObjectActionDefinition>(Resolve(TEXT("object_action"),Offer.ActionId));
        if (!Action || !Action->UsesCounterparty())
        { Error=TEXT("Offer allocation must reference an installed counterparty action."); return false; }
    }
    return true;
}
UOGObjectOfferDefinitionReceiver::UOGObjectOfferDefinitionReceiver()
{
    AcceptedSystem=TEXT("object_offers"); ExclusiveChannels.Add(TEXT("economy.object_offers"));
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.bStartWithTickEnabled=false;
}
bool UOGObjectOfferDefinitionReceiver::PrepareDefinition_Implementation(
    UDataAsset* Asset,const FOGDefinitionBindingContext& Context,FString& Error)
{
    auto* Definition=Cast<UOGObjectOfferDefinition>(Asset); auto* Library=UOGDefinitionLibrary::Get();
    if (Prepared || !Definition || !Definition->ValidateDefinition(Error) || !Library) return false;
    for (const auto& Offer:Definition->Offers)
    {
        const auto* Action=Cast<UOGObjectActionDefinition>(Library->Resolve(TEXT("object_action"),Offer.ActionId));
        if (!Action || Action->CounterpartyEntityKind!=Context.EntityKind)
        { Error=TEXT("Object offer set is incompatible with this real canonical entity kind."); return false; }
    }
    Prepared=Definition; return true;
}
void UOGObjectOfferDefinitionReceiver::ActivateDefinition_Implementation()
{ if (Prepared) SetComponentTickEnabled(true); }
void UOGObjectOfferDefinitionReceiver::ReleaseDefinition_Implementation()
{ SetComponentTickEnabled(false); Prepared=nullptr; }
void UOGObjectOfferDefinitionReceiver::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Tick,Function); SetComponentTickEnabled(false);
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    auto* Store=Core?Core->GetWorldStore():nullptr; auto* Library=UOGDefinitionLibrary::Get();
    if (!Prepared || !Core || !Core->IsCoreReady() || !Store || !Library)
    { Status=TEXT("Canonical offer producer is unavailable."); return; }
    for (const auto& Offer:Prepared->Offers)
    {
        const auto* Action=Cast<UOGObjectActionDefinition>(Library->Resolve(TEXT("object_action"),Offer.ActionId));
        const FString Key=TEXT("object_offer_allocation|")+BoundContext.EntityId.ToString()+TEXT("|")+
            Offer.PublicationId.ToString()+TEXT("|")+Offer.ActionId.ToString();
        FTCHARToUTF8 Bytes(*Key); FMD5 Hash; Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
        uint8 Digest[16]; Hash.Final(Digest); FGuid Guid; FGuid::ParseExact(BytesToHex(Digest,16),EGuidFormats::Digits,Guid);
        FString Error;
        if (!Action || !FOGObjectActionService(*Store).PublishOffer(*Action,BoundContext.EntityId,Offer.AllowedRulerId,
            Offer.RemainingUses,Offer.OpensTick,Offer.ClosesTick,FOGEntityId(Guid),Core->GetCanonicalWorldTick(),Error))
        { Status=Error.IsEmpty()?TEXT("Installed offer allocation is unavailable."):Error; UE_LOG(LogTemp,Warning,TEXT("%s"),*Status); return; }
    }
    Status.Reset();
}
