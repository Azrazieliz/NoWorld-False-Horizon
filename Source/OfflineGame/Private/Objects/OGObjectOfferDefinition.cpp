#include "Objects/OGObjectOfferDefinition.h"
#include "Objects/OGObjectActionService.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Combat/OGTurnBattle.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "World/OGStartingRegionPresentation.h"
#include "World/OGWorldModeInterfaces.h"

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
    PrimaryComponentTick.TickInterval = .25f;
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
{
    SetComponentTickEnabled(false);
    Prepared = nullptr;
    bOffersPublished = false;
    WorldInteractionError.Reset();
    WorldFeedbackUntil = 0.0;
}
void UOGObjectOfferDefinitionReceiver::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Tick,Function);
    if (!Prepared)
    {
        SetComponentTickEnabled(false);
        return;
    }
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    auto* Store=Core?Core->GetWorldStore():nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() || Store->IsWriteTransactionActive())
    {
        Status = TEXT("Waiting for canonical offer publication.");
        return;
    }

    SetComponentTickEnabled(false);
    FString Error;
    if (!FOGObjectActionService(*Store).PublishOffers(*Prepared, BoundContext.EntityId,
            Core->GetCanonicalWorldTick(), Error))
    {
        Status = Error.IsEmpty() ? TEXT("Installed offer profile is unavailable.") : Error;
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Status);
        return;
    }
    bOffersPublished = true;
    Status.Reset();
}

UOGObjectOfferDefinitionReceiver* UOGObjectOfferDefinitionReceiver::FindWorldInteraction(
    AActor* Target, const AOGWorldPrototypeCharacter* Interactor)
{
    if (!IsValid(Target) || !IsValid(Interactor) || Target == Interactor ||
        Target->GetWorld() != Interactor->GetWorld() || Target->IsActorBeingDestroyed() ||
        Target->IsHidden()) return nullptr;

    auto* Receiver = Target->FindComponentByClass<UOGObjectOfferDefinitionReceiver>();
    if (!Receiver || !Receiver->bOffersPublished || !Receiver->Prepared ||
        !Receiver->BoundContext.EntityId.IsValid() || !Interactor->IsLocallyControlled() ||
        !Interactor->IsFoundationWorldInputAllowed()) return nullptr;

    if (Target->GetClass()->ImplementsInterface(UOGWorldInteractable::StaticClass()) &&
        (Receiver->Prepared->WorldInteractionPrompt.IsEmpty() ||
         !IOGWorldInteractable::Execute_CanInteract(Target, const_cast<AOGWorldPrototypeCharacter*>(Interactor))))
        return nullptr;

    const auto* PC = Cast<APlayerController>(Interactor->GetController());
    const auto* Hud = PC ? Cast<AOGWorldPresentationHud>(PC->GetHUD()) : nullptr;
    const auto* Game = Target->GetGameInstance();
    const auto* Core = Game ? Game->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const auto* Session = Game ? Game->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    const auto* Battle = Core ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!Hud || !Hud->IsGate12NativeRulerUMGEnabled() || !Core || !Core->IsCoreReady() ||
        !Session || !Session->IsBoundToStoredRuler() ||
        (Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running))
        return nullptr;
    return Receiver;
}

FText UOGObjectOfferDefinitionReceiver::GetWorldInteractionPrompt() const
{
    return Prepared && !Prepared->WorldInteractionPrompt.IsEmpty()
        ? Prepared->WorldInteractionPrompt
        : NSLOCTEXT("NoWorldObjectOffers", "BrowseOffers", "Browse offers");
}

FText UOGObjectOfferDefinitionReceiver::GetWorldInteractionFeedback() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() < WorldFeedbackUntil
        ? FText::FromString(WorldInteractionError) : FText::GetEmpty();
}

bool UOGObjectOfferDefinitionReceiver::OpenFromWorldInteraction(
    AOGWorldPrototypeCharacter& Interactor, FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || FindWorldInteraction(GetOwner(), &Interactor) != this ||
        Interactor.GetContextInteractableForHud() != GetOwner())
    {
        Error = TEXT("This merchant or station is outside the current World interaction.");
        return false;
    }

    auto* PC = Cast<APlayerController>(Interactor.GetController());
    auto* Hud = PC ? Cast<AOGWorldPresentationHud>(PC->GetHUD()) : nullptr;
    if (!Hud || !Hud->ShowNativeObjectOffers(BoundContext.EntityId, Error))
    {
        if (Error.IsEmpty()) Error = TEXT("The native offers screen is unavailable.");
        WorldInteractionError = Error;
        WorldFeedbackUntil = GetWorld()->GetTimeSeconds() + 5.0;
        return false;
    }
    WorldInteractionError.Reset();
    WorldFeedbackUntil = 0.0;
    return true;
}
