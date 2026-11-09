#include "World/OGInventoryStorageService.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Persistence/OGWorldStore.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const FName StorageMoveEvent(TEXT("inventory.storage_moved"));

const UOGInventoryStorageDefinition* ResolveStorage(const FOGInventoryContainerRecord& Row)
{
    const auto* Library=UOGDefinitionLibrary::Get();
    const auto* Definition=Library?Cast<UOGInventoryStorageDefinition>(
        Library->Resolve(TEXT("inventory_storage"),Row.ContainerTypeId)):nullptr;
    FString Error;
    return Definition && Definition->ValidateDefinition(Error)?Definition:nullptr;
}

bool ReadOwnedItem(IOGWorldStore& Store,const FOGEntityId& Ruler,const FOGEntityId& ItemId,
    FOGItemInstanceRecord& Item,FString& Error)
{
    bool Found=false; FName Kind; FString Json; int64 Revision=0;
    if (!Ruler.IsValid() || !ItemId.IsValid() || !Store.TryReadEntity(Ruler,Found,Kind,Json,Revision,Error) ||
        !Found || Kind!=TEXT("ruler") || Json.Contains(TEXT("\"diagnosticFixture\"")) ||
        !Store.TryReadItemInstance(ItemId,Found,Item,Error) || !Found || !Item.OwnerEntityId.IsValid())
    { if (Error.IsEmpty()) Error=TEXT("Storage requires a stored Ruler and owned item."); return false; }
    if (Item.OwnerEntityId==Ruler) return true;
    FOGCharacterManifestationRecord Owner;
    if (!Store.TryReadCharacterManifestation(Item.OwnerEntityId,Found,Owner,Error)) return false;
    if (Found && Owner.OwningRulerId==Ruler && Owner.LifecycleState==TEXT("active")) return true;
    Error=TEXT("Only the Ruler's inventory and active owned characters may use this storage."); return false;
}

bool IsEquipped(IOGWorldStore& Store,const FOGEntityId& Ruler,const FOGItemInstanceRecord& Item,
    bool& Equipped,FString& Error)
{
    TArray<FOGEntityId> Wearers={Item.OwnerEntityId};
    if (Item.OwnerEntityId==Ruler)
    {
        TArray<FOGCharacterManifestationRecord> Copies;
        if (!Store.ListCharacterManifestationsByOwner(Ruler,Copies,Error)) return false;
        for (const auto& Copy:Copies) Wearers.AddUnique(Copy.ManifestationId);
    }
    Equipped=false;
    for (const auto& Wearer:Wearers)
    {
        TArray<FOGEquipmentBindingRecord> Bindings;
        if (!Store.ListEquipmentBindings(Wearer,Bindings,Error)) return false;
        if (Bindings.ContainsByPredicate([&](const auto& Binding){return Binding.ItemId==Item.ItemId;}))
        { Equipped=true; return true; }
    }
    return true;
}

bool AddBurden(const FOGStorageItemFootprint& Footprint,int64 Amount,TMap<FName,int64>& Used)
{
    for (const auto& Measure:Footprint.Burden)
    {
        auto& Total=Used.FindOrAdd(Measure.Key);
        if (Measure.Value>(MAX_int64-Total)/Amount) return false;
        Total+=Measure.Value*Amount;
    }
    return true;
}

FString StorageToken(const FOGInventoryStorageMove& View)
{
    FString Quote=View.RulerId.ToString()+TEXT("|")+View.ItemId.ToString()+TEXT("|")+
        View.OwnerId.ToString()+TEXT("|")+View.From.ToString()+TEXT("|")+View.Destination.ToString()+TEXT("|")+
        LexToString(View.Amount);
    TArray<FName> Dimensions; View.Capacity.GetKeys(Dimensions);
    Dimensions.Sort([](FName A,FName B){return A.LexicalLess(B);});
    for (FName Dimension:Dimensions)
    {
        const auto Name=Dimension.ToString();
        Quote+=TEXT("|")+LexToString(Name.Len())+TEXT(":")+Name+TEXT("|")+
            LexToString(View.Before.FindRef(Dimension))+TEXT("|")+LexToString(View.After.FindRef(Dimension))+
            TEXT("|")+LexToString(View.Capacity.FindChecked(Dimension));
    }
    return FMD5::HashAnsiString(*Quote);
}
}

bool UOGInventoryStorageDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("inventory_storage") || DisplayName.IsEmpty() || OriginalSourceReference.TrimStartAndEnd().IsEmpty() ||
        Capacity.IsEmpty() || ItemFootprints.IsEmpty())
    { Error=TEXT("Storage needs a source, named dimensions and admitted item footprints."); return false; }
    for (const auto& Limit:Capacity)
        if (Limit.Key.IsNone() || Limit.Value < -1)
        { Error=TEXT("Storage dimensions need exact nonnegative or unlimited capacities."); return false; }
    TSet<FOGContentId> Seen;
    for (const auto& Item:ItemFootprints)
    {
        if (!Item.ItemDefinitionId.IsValid() || Seen.Contains(Item.ItemDefinitionId) || Item.Burden.Num()!=Capacity.Num())
        { Error=TEXT("A storage footprint must cover every authored dimension exactly once."); return false; }
        for (const auto& Measure:Item.Burden)
            if (!Capacity.Contains(Measure.Key) || Measure.Value<0)
            { Error=TEXT("Storage burden is outside its authored dimensions."); return false; }
        Seen.Add(Item.ItemDefinitionId);
    }
    return true;
}

const FOGStorageItemFootprint* UOGInventoryStorageDefinition::FindFootprint(const FOGContentId& Item) const
{
    return ItemFootprints.FindByPredicate([&](const auto& Row){return Row.ItemDefinitionId==Item;});
}

bool FOGInventoryStorageService::ReadDestinations(const FOGEntityId& Ruler,const FOGEntityId& Item,
    TArray<FOGInventoryStorageMove>& Out,FString& Error) const
{
    Error.Reset(); Out.Reset();
    FOGItemInstanceRecord Owned;
    if (!ReadOwnedItem(Store,Ruler,Item,Owned,Error)) return false;
    TArray<FOGInventoryContainerRecord> Containers;
    if (!Store.ListInventoryContainersByOwner(Owned.OwnerEntityId,Containers,Error)) return false;
    TArray<FOGInventoryStorageMove> Options;
    for (const auto& Container:Containers)
    {
        FOGInventoryStorageMove Option;
        if (!Review(Ruler,Item,Container.ContainerId,Option,Error)) return false;
        Options.Add(MoveTemp(Option));
    }
    Out=MoveTemp(Options);
    return true;
}

bool FOGInventoryStorageService::Review(const FOGEntityId& Ruler,const FOGEntityId& ItemId,
    const FOGEntityId& Destination,FOGInventoryStorageMove& Out,FString& Error) const
{
    Error.Reset(); Out=FOGInventoryStorageMove();
    FOGItemInstanceRecord Item;
    if (!ReadOwnedItem(Store,Ruler,ItemId,Item,Error)) return false;
    FOGInventoryContainerRecord Container; bool Found=false;
    if (!Store.TryReadInventoryContainer(Destination,Found,Container,Error)) return false;
    if (!Found || Container.OwnerEntityId!=Item.OwnerEntityId)
    { Error=TEXT("The item and destination must belong to the same stored owner."); return false; }
    Out.RulerId=Ruler; Out.ItemId=ItemId; Out.OwnerId=Item.OwnerEntityId; Out.Destination=Destination;
    const auto* Definition=ResolveStorage(Container);
    Out.DestinationName=Definition?Definition->DisplayName.ToString():Container.ContainerTypeId.ToString();

    TArray<FOGContainerContentRecord> Locations;
    if (!Store.ListContainerPlacementsForItem(ItemId,Locations,Error)) return false;
    if (Locations.Num()>1)
    { Error=TEXT("This item has conflicting stored locations."); return false; }
    if (!Locations.IsEmpty())
    {
        FOGInventoryContainerRecord Current;
        if (!Store.TryReadInventoryContainer(Locations[0].ContainerId,Found,Current,Error)) return false;
        if (!Found || Current.OwnerEntityId!=Item.OwnerEntityId)
        { Error=TEXT("This item's stored location has a different owner."); return false; }
        Out.From=Current.ContainerId; Out.Amount=Locations[0].Amount;
        const auto* CurrentDefinition=ResolveStorage(Current);
        Out.FromName=CurrentDefinition?CurrentDefinition->DisplayName.ToString():Current.ContainerTypeId.ToString();
    }
    if (Out.From==Destination) { Out.Reason=TEXT("Current location"); return true; }
    if (!Definition)
    { Out.Reason=TEXT("This storage type's capacity rules are unavailable."); return true; }
    const auto* ItemFootprint=Definition->FindFootprint(Item.DefinitionId);
    if (!ItemFootprint)
    { Out.Reason=TEXT("This storage type does not admit the selected item."); return true; }
    bool Equipped=false;
    if (!IsEquipped(Store,Ruler,Item,Equipped,Error)) return false;
    if (Equipped) { Out.Reason=TEXT("Unequip this item before moving it."); return true; }

    Out.Capacity=Definition->Capacity;
    TSharedPtr<FJsonObject> CapacityState;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Container.CapacityStateJson),CapacityState) || !CapacityState)
    { Out.Reason=TEXT("This compartment's saved capacity is invalid."); return true; }
    if (CapacityState->HasField(TEXT("limits_exact")))
    {
        const TSharedPtr<FJsonObject>* Limits=nullptr;
        if (!CapacityState->TryGetObjectField(TEXT("limits_exact"),Limits) || !Limits || !Limits->IsValid())
        { Out.Reason=TEXT("This compartment's saved capacity limits are invalid."); return true; }
        for (const auto& Limit:(*Limits)->Values)
        {
            auto* Value=Out.Capacity.Find(FName(*Limit.Key));
            FString Encoded; int64 Parsed=0;
            if (!Value || !Limit.Value || !Limit.Value->TryGetString(Encoded) ||
                !LexTryParseString(Parsed,*Encoded) || Encoded!=LexToString(Parsed) || Parsed < -1)
            { Out.Reason=TEXT("A saved capacity limit does not match the installed storage type."); return true; }
            *Value=Parsed;
        }
    }
    TArray<FOGContainerContentRecord> Contents;
    if (!Store.ListContainerContents(Destination,Contents,Error)) return false;
    for (const auto& Row:Contents)
    {
        FOGItemInstanceRecord Stored;
        if (!Store.TryReadItemInstance(Row.ItemId,Found,Stored,Error)) return false;
        const auto* Footprint=Found && Stored.OwnerEntityId==Item.OwnerEntityId
            ? Definition->FindFootprint(Stored.DefinitionId) : nullptr;
        if (!Footprint || !AddBurden(*Footprint,Row.Amount,Out.Before))
        { Out.Reason=TEXT("Stored contents cannot be measured with the installed capacity rules."); return true; }
    }
    Out.After=Out.Before;
    if (!AddBurden(*ItemFootprint,Out.Amount,Out.After))
    { Out.Reason=TEXT("This stack exceeds measurable storage capacity."); return true; }
    for (const auto& Limit:Out.Capacity)
        if (Limit.Value>=0 && Out.After.FindRef(Limit.Key)>Limit.Value)
        { Out.Reason=TEXT("Insufficient capacity for ")+Limit.Key.ToString()+TEXT("."); return true; }
    Out.bCanMove=true; Out.ConfirmationToken=StorageToken(Out);
    return true;
}

bool FOGInventoryStorageService::Move(const FOGInventoryStorageMove& Reviewed,
    const FOGEntityId& Request,int64 Tick,FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || !Request.IsValid() || Tick<0 || Reviewed.ConfirmationToken.IsEmpty())
    { Error=TEXT("Storage move requires a current review and request."); return false; }
    if (!Store.BeginTransaction(Error)) return false;
    auto Rollback=[&](){FString Ignored; Store.RollbackTransaction(Ignored); return false;};
    FOGWorldEvent Previous; bool Found=false;
    if (!Store.TryReadWorldEvent(Request,Found,Previous,Error)) return Rollback();
    if (Found)
    {
        TSharedPtr<FJsonObject> Receipt; FString Token,Item,Destination;
        if (Previous.EventType!=StorageMoveEvent || Previous.PrimaryEntity!=Reviewed.RulerId ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Previous.PayloadJson),Receipt) || !Receipt ||
            !Receipt->TryGetStringField(TEXT("confirmation_token"),Token) || Token!=Reviewed.ConfirmationToken ||
            !Receipt->TryGetStringField(TEXT("item_id"),Item) || Item!=Reviewed.ItemId.ToString() ||
            !Receipt->TryGetStringField(TEXT("destination_id"),Destination) || Destination!=Reviewed.Destination.ToString())
        { Error=TEXT("Request ID belongs to a different storage move."); return Rollback(); }
        if (!Store.CommitTransaction(Error)) return Rollback();
        return true;
    }
    FOGInventoryStorageMove Fresh;
    if (!Review(Reviewed.RulerId,Reviewed.ItemId,Reviewed.Destination,Fresh,Error)) return Rollback();
    if (!Fresh.bCanMove || Fresh.ConfirmationToken!=Reviewed.ConfirmationToken)
    { Error=Fresh.Reason.IsEmpty()?TEXT("Storage or the item changed. Review it again."):Fresh.Reason; return Rollback(); }
    FOGContainerContentRecord Placement;
    Placement.ContainerId=Fresh.Destination; Placement.ItemId=Fresh.ItemId; Placement.Amount=Fresh.Amount;
    if (!Store.DeleteContainerContentsForItem(Fresh.ItemId,Error) || !Store.UpsertContainerContent(Placement,Error)) return Rollback();

    auto Receipt=MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("item_id"),Fresh.ItemId.ToString());
    Receipt->SetStringField(TEXT("owner_id"),Fresh.OwnerId.ToString());
    Receipt->SetStringField(TEXT("from_id"),Fresh.From.IsValid()?Fresh.From.ToString():FString());
    Receipt->SetStringField(TEXT("destination_id"),Fresh.Destination.ToString());
    Receipt->SetStringField(TEXT("amount_exact"),LexToString(Fresh.Amount));
    Receipt->SetStringField(TEXT("confirmation_token"),Fresh.ConfirmationToken);
    FOGWorldEvent Event; Event.EventId=Request; Event.EventType=StorageMoveEvent; Event.WorldTick=Tick;
    Event.PrimaryEntity=Fresh.RulerId; Event.RelatedEntities={Fresh.ItemId,Fresh.Destination};
    if (Fresh.From.IsValid()) Event.RelatedEntities.AddUnique(Fresh.From);
    if (!FJsonSerializer::Serialize(Receipt,TJsonWriterFactory<>::Create(&Event.PayloadJson)) ||
        !Store.AppendWorldEvent(Event,Error) || !Store.CommitTransaction(Error)) return Rollback();
    return true;
}
