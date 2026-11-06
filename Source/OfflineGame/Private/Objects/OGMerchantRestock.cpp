#include "Objects/OGObjectActionService.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Objects/OGObjectOfferDefinition.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const FName DeliveryStateKind(TEXT("merchant_delivery_state"));
const FName DeliveryEventKind(TEXT("economy.merchant_delivery"));

FOGEntityId DeliveryStateId(const FOGEntityId& Merchant,const FOGContentId& Rule)
{
    FGuid Guid;
    FGuid::ParseExact(FMD5::HashAnsiString(*(TEXT("merchant_delivery|")+Merchant.ToString()+TEXT("|")+Rule.ToString())),
        EGuidFormats::Digits,Guid);
    return FOGEntityId(Guid);
}

bool Parse(const FString& Json,TSharedPtr<FJsonObject>& Out)
{ return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Out) && Out.IsValid(); }

bool Matches(const FOGItemInstanceRecord& Item,const FOGMerchantRestockRule& Rule)
{ return Item.DefinitionId==Rule.ObjectId && Item.CurrentRankId==Rule.RankId && Item.QualityId==Rule.QualityId; }

bool ReadAnchoredAt(IOGWorldStore& Store,const FOGEntityId& Actor,const FOGEntityId& Location,bool& At,FString& Error)
{
    if (Actor==Location) { At=true; return true; }
    FOGWorldPresenceRecord Presence; bool Found=false;
    if (!Store.TryReadWorldPresence(Actor,Found,Presence,Error)) return false;
    At=Found && Presence.LocationId==Location;
    return true;
}

bool ReadDeliveryTick(IOGWorldStore& Store,const FOGEntityId& Id,const FOGEntityId& Merchant,
    const FOGContentId& Rule,int64& Last,FString& Error)
{
    bool Found=false; FName Kind; FString Json; int64 Revision=0;
    Last=-1;
    if (!Store.TryReadEntity(Id,Found,Kind,Json,Revision,Error)) return false;
    if (!Found) return true;
    TSharedPtr<FJsonObject> State; FString MerchantText,RuleText,TickText;
    if (Kind==DeliveryStateKind && Parse(Json,State) &&
        State->TryGetStringField(TEXT("merchant_id"),MerchantText) && MerchantText==Merchant.ToString() &&
        State->TryGetStringField(TEXT("restock_id"),RuleText) && RuleText==Rule.ToString() &&
        State->TryGetStringField(TEXT("last_delivery_tick_exact"),TickText) &&
        LexTryParseString(Last,*TickText) && Last>=0 && LexToString(Last)==TickText)
        return true;
    Error=TEXT("The saved merchant delivery record does not match this stock allocation.");
    return false;
}

bool EligibleSupplier(IOGWorldStore& Store,const FOGMerchantRestockRule& Rule,bool& Eligible,FString& Error)
{
    bool Found=false; FName Kind; FString Json; int64 Revision=0;
    if (!Store.TryReadEntity(Rule.SupplierId,Found,Kind,Json,Revision,Error)) return false;
    Eligible=false;
    if (!Found || Kind!=Rule.SupplierKind || Kind==TEXT("ruler") || Json.Contains(TEXT("\"diagnosticFixture\""))) return true;
    FOGCharacterManifestationRecord Copy;
    if (!Store.TryReadCharacterManifestation(Rule.SupplierId,Found,Copy,Error)) return false;
    Eligible=!Found;
    return true;
}
}

bool FOGObjectActionService::RestockOffers(const UOGObjectOfferDefinition& Profile,const FOGEntityId& Merchant,
    int64 Tick,FString& Error)
{
    Error.Reset();
    if (Profile.RestockRules.IsEmpty()) return true;
    if (!IsInGameThread() || !Merchant.IsValid() || Tick<0 || !Profile.ValidateDefinition(Error))
    { if (Error.IsEmpty()) Error=TEXT("Merchant restock needs a valid canonical market and source profile."); return false; }
    auto* Library=UOGDefinitionLibrary::Get();
    if (!Library || !Profile.ValidateResolvedReferences([&](FName System,const FOGContentId& Id)
        { return Library->Resolve(System,Id); },Error)) return false;
    if (!Store.BeginTransaction(Error)) return false;
    auto Rollback=[&](){FString Ignored; Store.RollbackTransaction(Ignored); return false;};

    bool Found=false; FName MerchantKind; FString MerchantState; int64 Revision=0;
    if (!Store.TryReadEntity(Merchant,Found,MerchantKind,MerchantState,Revision,Error) || !Found ||
        MerchantState.Contains(TEXT("\"diagnosticFixture\"")))
    { if (Error.IsEmpty()) Error=TEXT("The actual production merchant is unavailable."); return Rollback(); }

    TMap<FOGEntityId,FOGLargeNumber> UsedRouteLoad;
    for (const auto& Rule:Profile.RestockRules)
    {
        const auto& Offer=*Profile.Offers.FindByPredicate([&](const auto& Row){return Row.ActionId==Rule.OfferActionId;});
        const auto* Action=CastChecked<UOGObjectActionDefinition>(Library->Resolve(TEXT("object_action"),Offer.ActionId));
        if (Action->CounterpartyEntityKind!=MerchantKind || Merchant==Rule.SupplierId ||
            Tick<Offer.OpensTick || (Offer.ClosesTick>=0 && Tick>=Offer.ClosesTick)) continue;
        const auto StateId=DeliveryStateId(Merchant,Rule.RestockId);
        int64 Last=-1;
        if (!ReadDeliveryTick(Store,StateId,Merchant,Rule.RestockId,Last,Error)) return Rollback();
        if (Last>=0 && (Tick<Last || Tick-Last<Rule.DeliveryIntervalTicks)) continue;
        bool Eligible=false;
        if (!EligibleSupplier(Store,Rule,Eligible,Error)) return Rollback();
        if (!Eligible) continue;
        FOGLogisticsRouteRecord Route;
        if (!Store.TryReadLogisticsRoute(Rule.RouteId,Found,Route,Error)) return Rollback();
        if (!Found || Route.Status!=TEXT("open") || Route.Capacity.GetSign()<=0) continue;
        bool AtOrigin=false,AtDestination=false;
        if (!ReadAnchoredAt(Store,Rule.SupplierId,Route.OriginLocationId,AtOrigin,Error) ||
            !ReadAnchoredAt(Store,Merchant,Route.DestinationLocationId,AtDestination,Error)) return Rollback();
        if (!AtOrigin || !AtDestination) continue;

        TArray<FOGItemInstanceRecord> Stock;
        if (!Store.ListItemInstancesByOwner(Merchant,Stock,Error)) return Rollback();
        int32 OnHand=0;
        for (const auto& Item:Stock)
            if (Item.OwnerEntityId==Merchant && Matches(Item,Rule)) ++OnHand;
        const int32 Limit=FMath::Min(Rule.MaximumPerDelivery,Rule.MaximumOnHand-OnHand);
        if (Limit<=0) continue;

        TArray<FOGItemInstanceRecord> Supply;
        TArray<FOGEquipmentBindingRecord> Equipped;
        if (!Store.ListItemInstancesByOwner(Rule.SupplierId,Supply,Error) ||
            !Store.ListEquipmentBindings(Rule.SupplierId,Equipped,Error)) return Rollback();
        Supply.Sort([](const auto& A,const auto& B){return A.ItemId.ToString()<B.ItemId.ToString();});
        const FOGEntityId DeliveryId=FOGEntityId::NewId();
        TArray<TSharedPtr<FJsonValue>> Moved;
        TArray<FOGEntityId> Related={Rule.SupplierId,Rule.RouteId};
        auto& RouteLoad=UsedRouteLoad.FindOrAdd(Rule.RouteId);
        for (auto& Item:Supply)
        {
            if (Moved.Num()>=Limit) break;
            if (Item.OwnerEntityId!=Rule.SupplierId || !Matches(Item,Rule) ||
                Equipped.ContainsByPredicate([&](const auto& Binding){return Binding.ItemId==Item.ItemId;})) continue;
            const auto Load=FOGLargeNumber::Add(RouteLoad,Rule.RouteLoadPerItem);
            if (FOGLargeNumber::Compare(Load,Route.Capacity)>0) break;
            TArray<FOGContainerContentRecord> Placements;
            if (!Store.ListContainerPlacementsForItem(Item.ItemId,Placements,Error)) return Rollback();
            if (Placements.Num()>1 || (!Placements.IsEmpty() && Placements[0].Amount!=1)) continue;
            if (!Placements.IsEmpty())
            {
                FOGInventoryContainerRecord Container;
                if (!Store.TryReadInventoryContainer(Placements[0].ContainerId,Found,Container,Error)) return Rollback();
                if (!Found || Container.OwnerEntityId!=Rule.SupplierId) continue;
            }
            TSharedPtr<FJsonObject> History;
            if (!Parse(Item.HistoryStateJson,History))
            { Error=TEXT("Supplier item history is invalid."); return Rollback(); }
            History->SetStringField(TEXT("last_merchant_delivery"),DeliveryId.ToString());
            if (!FJsonSerializer::Serialize(History.ToSharedRef(),TJsonWriterFactory<>::Create(&Item.HistoryStateJson)))
            { Error=TEXT("Could not save merchant delivery provenance."); return Rollback(); }
            Item.OwnerEntityId=Merchant;
            if (!Store.DeleteContainerContentsForItem(Item.ItemId,Error) || !Store.UpsertItemInstance(Item,Tick,Error))
                return Rollback();
            RouteLoad=Load;
            Related.Add(Item.ItemId); Moved.Add(MakeShared<FJsonValueString>(Item.ItemId.ToString()));
        }
        if (Moved.IsEmpty()) continue;

        auto Receipt=MakeShared<FJsonObject>();
        Receipt->SetStringField(TEXT("merchant_id"),Merchant.ToString());
        Receipt->SetStringField(TEXT("supplier_id"),Rule.SupplierId.ToString());
        Receipt->SetStringField(TEXT("restock_id"),Rule.RestockId.ToString());
        Receipt->SetStringField(TEXT("offer_id"),Rule.OfferActionId.ToString());
        Receipt->SetStringField(TEXT("route_id"),Rule.RouteId.ToString());
        Receipt->SetStringField(TEXT("last_delivery_tick_exact"),LexToString(Tick));
        Receipt->SetArrayField(TEXT("item_ids"),MoveTemp(Moved));
        FOGWorldEvent Event; Event.EventId=DeliveryId; Event.EventType=DeliveryEventKind;
        Event.WorldTick=Tick; Event.PrimaryEntity=Merchant; Event.RelatedEntities=MoveTemp(Related); Event.bChronicleEligible=true;
        if (!FJsonSerializer::Serialize(Receipt,TJsonWriterFactory<>::Create(&Event.PayloadJson)) ||
            !Store.UpsertEntity(StateId,DeliveryStateKind,Tick,Event.PayloadJson,Error) ||
            !Store.AppendWorldEvent(Event,Error)) return Rollback();
    }
    if (!Store.CommitTransaction(Error)) return Rollback();
    return true;
}
