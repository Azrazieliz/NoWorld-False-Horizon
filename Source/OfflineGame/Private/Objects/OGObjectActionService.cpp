#include "Objects/OGObjectActionService.h"
#include "Objects/OGObjectOfferDefinition.h"
#include "Combat/OGCombatVitalityService.h"
#include "World/OGOwnedEquipmentService.h"
#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const FName ActionEvent(TEXT("object_action_applied")),OfferEvent(TEXT("object_action_offer_published"));
void AppendObjectServicePart(FString& S,const FString& V) { S+=LexToString(V.Len())+TEXT(":")+V; }
FString ObjectActionStableDigest(const FString& Text)
{
    FTCHARToUTF8 Bytes(*Text); FMD5 Hash; Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    uint8 Result[16]; Hash.Final(Result); return BytesToHex(Result,16);
}
FOGEntityId StableId(const FString& Text)
{ FGuid Guid; FGuid::ParseExact(ObjectActionStableDigest(Text),EGuidFormats::Digits,Guid); return FOGEntityId(Guid); }
FOGEntityId OfferId(const FOGEntityId& Counterparty,const FOGContentId& Action)
{ return StableId(TEXT("object_offer|")+Counterparty.ToString()+TEXT("|")+Action.ToString()); }
bool Object(const FString& S,TSharedPtr<FJsonObject>& J)
{ return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),J) && J.IsValid(); }
FString Write(const TSharedRef<FJsonObject>& J)
{ FString S; FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&S)); return S; }
bool Integer(const TSharedPtr<FJsonObject>& J,const TCHAR* Name,int64& Out)
{ FString S; return J && J->TryGetStringField(Name,S) && LexTryParseString(Out,*S) && LexToString(Out)==S; }
FOGEntityId Party(const FOGObjectActionContext& C,EOGObjectActionParty Which)
{ return Which==EOGObjectActionParty::Ruler?C.RulerId:C.CounterpartyId; }
void ItemQuote(FString& S,const FOGItemInstanceRecord& I)
{
    for (const auto& V:{I.ItemId.ToString(),I.OwnerEntityId.ToString(),I.DefinitionId.ToString(),I.CurrentRankId.ToString(),
        I.QualityId.ToString(),I.DurabilityStateJson,I.EvolutionStateJson,I.HistoryStateJson}) AppendObjectServicePart(S,V);
}
bool ReadParty(IOGWorldStore& Store,const FOGEntityId& Owner,FName Expected,FString& State,int64& Revision,FString& Error)
{
    bool Found=false; FName Kind;
    if (!Owner.IsValid() || !Store.TryReadEntity(Owner,Found,Kind,State,Revision,Error)) return false;
    if (!Found || Kind!=Expected || State.Contains(TEXT("\"diagnosticFixture\"")))
    { Error=TEXT("Object action party is not the required real stored entity."); return false; }
    return true;
}
bool OwnedItems(IOGWorldStore& Store,const FOGEntityId& Owner,bool IncludeManifestations,
    TArray<FOGItemInstanceRecord>& Out,TSet<FOGEntityId>& Equipped,FString& Quote,FString& Error)
{
    TArray<FOGEntityId> Owners={Owner};
    if (IncludeManifestations)
    {
        TArray<FOGCharacterManifestationRecord> Copies;
        if (!Store.ListCharacterManifestationsByOwner(Owner,Copies,Error)) return false;
        for (const auto& M:Copies) if (M.OwningRulerId==Owner) Owners.AddUnique(M.ManifestationId);
    }
    TSet<FOGEntityId> Seen;
    for (const auto& Id:Owners)
    {
        TArray<FOGItemInstanceRecord> Items; TArray<FOGEquipmentBindingRecord> Bindings;
        if (!Store.ListItemInstancesByOwner(Id,Items,Error) || !Store.ListEquipmentBindings(Id,Bindings,Error)) return false;
        for (const auto& I:Items)
        {
            if (I.OwnerEntityId!=Id || Seen.Contains(I.ItemId))
            { Error=TEXT("Inconsistent canonical object ownership."); return false; }
            Seen.Add(I.ItemId); Out.Add(I);
        }
        for (const auto& B:Bindings)
        {
            Equipped.Add(B.ItemId); AppendObjectServicePart(Quote,B.ItemId.ToString()); AppendObjectServicePart(Quote,B.WearerEntityId.ToString());
            AppendObjectServicePart(Quote,B.SlotId.ToString()); AppendObjectServicePart(Quote,B.StateJson);
        }
    }
    Out.Sort([](const auto& A,const auto& B){return A.ItemId.ToString()<B.ItemId.ToString();});
    return true;
}
const UOGObjectDefinition* ResolveObject(const FOGContentId& Id,FString& Error)
{
    auto* Library=UOGDefinitionLibrary::Get();
    const auto* D=Library?Cast<UOGObjectDefinition>(Library->Resolve(TEXT("object"),Id)):nullptr;
    if (!D || !D->ValidateDefinition(Error))
    { if (Error.IsEmpty()) Error=TEXT("Object requires an installed native definition: ")+Id.ToString(); return nullptr; }
    return D;
}
}
bool FOGObjectActionService::Preview(const UOGObjectActionDefinition& D,const FOGObjectActionContext& C,
    int64 Tick,FOGObjectActionPreview& Out,FString& Error) const
{
    Out=FOGObjectActionPreview(); Error.Reset();
    if (!IsInGameThread() || !Store.IsOpen() || Tick<0 || !D.ValidateDefinition(Error)) return false;
    Out.ActionId=D.DefinitionId; Out.DisplayName=D.DisplayName; Out.Description=D.Description; Out.Context=C; Out.Outputs=D.Outputs;
    FString Quote=D.Fingerprint(),State; int64 Revision=0;
    if (!ReadParty(Store,C.RulerId,TEXT("ruler"),State,Revision,Error)) return false;
    AppendObjectServicePart(Quote,C.RulerId.ToString()); AppendObjectServicePart(Quote,C.CounterpartyId.ToString()); AppendObjectServicePart(Quote,C.SelectedItemId.ToString());
    if (D.UsesCounterparty())
    {
        if (C.CounterpartyId==C.RulerId || !ReadParty(Store,C.CounterpartyId,D.CounterpartyEntityKind,State,Revision,Error)) return false;
        Out.OfferId=OfferId(C.CounterpartyId,D.DefinitionId);
        bool Found=false; FName Kind; FString OfferState; int64 OfferRevision=0;
        if (!Store.TryReadEntity(Out.OfferId,Found,Kind,OfferState,OfferRevision,Error)) return false;
        TSharedPtr<FJsonObject> Offer; FString Action,Counterparty,Allowed; int64 Opens=0,Closes=0,Remaining=0;
        if (!Found || Kind!=TEXT("object_action_offer") || !Object(OfferState,Offer) ||
            !Offer->TryGetStringField(TEXT("action_id"),Action) || Action!=D.DefinitionId.ToString() ||
            !Offer->TryGetStringField(TEXT("counterparty_id"),Counterparty) || Counterparty!=C.CounterpartyId.ToString() ||
            !Offer->TryGetStringField(TEXT("allowed_ruler_id"),Allowed) ||
            !Integer(Offer,TEXT("remaining_uses_exact"),Remaining) || Remaining < -1 ||
            !Integer(Offer,TEXT("opens_tick_exact"),Opens) || !Integer(Offer,TEXT("closes_tick_exact"),Closes) ||
            Opens<0 || (Closes!=-1 && Closes<=Opens))
        { Out.MissingRequirements.Add(TEXT("No valid canonical offer is available from this counterparty.")); }
        else
        {
            if ((!Allowed.IsEmpty() && Allowed!=C.RulerId.ToString()) || Remaining==0 || Tick<Opens || (Closes>=0 && Tick>=Closes))
                Out.MissingRequirements.Add(TEXT("This stored offer is unavailable, exhausted or outside its availability window."));
            Out.OfferStateJson=OfferState; Out.OfferRemaining=Remaining;
            AppendObjectServicePart(Quote,OfferState); AppendObjectServicePart(Quote,LexToString(OfferRevision));
        }
    }
    else if (C.CounterpartyId.IsValid())
    { Error=TEXT("This definition has no counterparty operation."); return false; }
    if (D.RestoreHealthBps>0)
    {
        if (!C.HealthRecipientId.IsValid())
        { Error=TEXT("Select the character who will use this consumable."); return false; }
        if (C.HealthRecipientId!=C.RulerId)
        {
            FOGCharacterManifestationRecord Copy; bool Found=false;
            if (!Store.TryReadCharacterManifestation(C.HealthRecipientId,Found,Copy,Error)) return false;
            if (!Found || Copy.OwningRulerId!=C.RulerId || Copy.LifecycleState!=TEXT("active"))
            { Error=TEXT("Consumable recovery requires the Ruler or an active owned Manifestation."); return false; }
        }
        FOGStoredCombatVitality Saved; bool Found=false;
        if (!FOGCombatVitalityService(Store).Read(C.HealthRecipientId,Found,Saved,Error)) return false;
        if (!Found || Saved.CurrentHp.GetSign()<=0)
            Out.MissingRequirements.Add(TEXT("This character has no living saved HP to restore."));
        else
        {
            Out.bRestoresHealth=true; Out.HealthBattleId=Saved.BattleId; Out.HealthBefore=Saved.CurrentHp;
            Out.HealthMaximum=Saved.MaximumHpAtCommit;
            Out.HealthAfter=FOGCombatVitalityService::ResolveRestoredHealth(Saved,D.RestoreHealthBps);
            if (FOGLargeNumber::Compare(Out.HealthAfter,Out.HealthBefore)<=0)
                Out.MissingRequirements.Add(TEXT("This character would not regain any HP."));
            for (const auto& Value:{C.HealthRecipientId.ToString(),Saved.LifeId.ToString(),Saved.ReceiptId.ToString(),
                Out.HealthBefore.ToDebugString(),Out.HealthAfter.ToDebugString(),Out.HealthMaximum.ToDebugString()})
                AppendObjectServicePart(Quote,Value);
        }
    }
    TArray<FOGItemInstanceRecord> PlayerItems,OtherItems; TSet<FOGEntityId> Equipped;
    if (!OwnedItems(Store,C.RulerId,true,PlayerItems,Equipped,Quote,Error) ||
        (D.UsesCounterparty() && !OwnedItems(Store,C.CounterpartyId,false,OtherItems,Equipped,Quote,Error))) return false;
    TSet<FOGEntityId> Used;
    if (D.Kind==EOGObjectActionKind::Repair)
    {
        const auto* Item=PlayerItems.FindByPredicate([&](const auto& I){return I.ItemId==C.SelectedItemId && I.DefinitionId==D.SubjectObjectId;});
        if (!Item) Out.MissingRequirements.Add(TEXT("Select an owned instance of this repair's exact object definition."));
        else
        {
            TSharedPtr<FJsonObject> Condition; double Before=10000; bool Broken=false;
            if (!Object(Item->DurabilityStateJson,Condition) ||
                (Condition->HasField(TEXT("condition_bps")) && !Condition->TryGetNumberField(TEXT("condition_bps"),Before)) ||
                (Condition->HasField(TEXT("broken")) && !Condition->TryGetBoolField(TEXT("broken"),Broken)) ||
                !FMath::IsFinite(Before) || Before<0 || Before>10000 || static_cast<double>(static_cast<int32>(Before))!=Before)
            { Error=TEXT("Stored repair condition is invalid; preserving the item."); return false; }
            if (!Broken && Before>=D.RepairToConditionBps) Out.MissingRequirements.Add(TEXT("This repair would not improve the selected item."));
            Condition->SetNumberField(TEXT("condition_bps"),D.RepairToConditionBps); Condition->SetBoolField(TEXT("broken"),false);
            Out.bRepairsSubject=true; Out.SubjectBefore=*Item; Out.SubjectDurabilityAfter=Write(Condition.ToSharedRef());
            Out.RepairBeforeConditionBps=static_cast<int32>(Before); Out.RepairAfterConditionBps=D.RepairToConditionBps; Out.bSubjectWasBroken=Broken;
            ItemQuote(Quote,*Item); Used.Add(Item->ItemId);
        }
    }
    if (D.Kind==EOGObjectActionKind::Enhance)
    {
        const auto* Item=PlayerItems.FindByPredicate([&](const auto& Row)
        { return Row.ItemId==C.SelectedItemId && Row.DefinitionId==D.SubjectObjectId; });
        if (!Item || Item->CurrentRankId!=D.EnhanceFromRankId || Item->QualityId!=D.EnhanceFromQualityId)
            Out.MissingRequirements.Add(TEXT("Select an owned item at this enhancement's exact starting Rank and Quality."));
        else
        {
            if (!ResolveObject(Item->DefinitionId,Error)) return false;
            Out.bEnhancesSubject=true; Out.SubjectBefore=*Item;
            Out.EnhancedRankId=D.EnhanceToRankId; Out.EnhancedQualityId=D.EnhanceToQualityId;
            ItemQuote(Quote,*Item); Used.Add(Item->ItemId);
            TArray<FOGContainerContentRecord> Storage;
            if (!Store.ListContainerPlacementsForItem(Item->ItemId,Storage,Error)) return false;
            if (Storage.Num()>1 || (!Storage.IsEmpty() && Storage[0].Amount!=1))
                Out.MissingRequirements.Add(TEXT("Enhancement requires a single item instance in one physical location."));
            TArray<FOGItemModifierRecord> Modifiers;
            if (!Store.ListItemModifiers(Item->ItemId,Modifiers,Error)) return false;
            for (const auto& Modifier:Modifiers)
            {
                AppendObjectServicePart(Quote,Modifier.ModifierId.ToString());
                AppendObjectServicePart(Quote,LexToString(Modifier.Ordinal));
                AppendObjectServicePart(Quote,Modifier.StateJson);
            }
            if (Equipped.Contains(Item->ItemId))
            {
                FOGEquipmentCatalog Equipment;
                FString RulesError;
                if (!FOGEquipmentCatalog::LoadInstalled(Equipment,RulesError))
                    Out.MissingRequirements.Add(TEXT("This equipped item's resulting slot rule is unavailable."));
                else
                {
                    auto Enhanced=*Item;
                    Enhanced.CurrentRankId=D.EnhanceToRankId; Enhanced.QualityId=D.EnhanceToQualityId;
                    TArray<FOGCharacterManifestationRecord> Copies;
                    if (!Store.ListCharacterManifestationsByOwner(C.RulerId,Copies,Error)) return false;
                    TArray<FOGEquipmentBindingRecord> RulerBindings;
                    if (!Store.ListEquipmentBindings(C.RulerId,RulerBindings,Error)) return false;
                    bool Compatible=!RulerBindings.ContainsByPredicate([&](const auto& B){return B.ItemId==Item->ItemId;});
                    for (const auto& Copy:Copies)
                    {
                        TArray<FOGEquipmentBindingRecord> Bindings;
                        if (!Store.ListEquipmentBindings(Copy.ManifestationId,Bindings,Error)) return false;
                        for (const auto& Binding:Bindings)
                            if (Binding.ItemId==Item->ItemId && !Equipment.FindRule(Copy,Binding.SlotId,Enhanced))
                                Compatible=false;
                    }
                    if (!Compatible)
                        Out.MissingRequirements.Add(TEXT("The resulting item grade has no authored rule for its current equipped slot."));
                    AppendObjectServicePart(Quote,Equipment.Fingerprint());
                }
            }
        }
    }
    bool SelectedUsed=false;
    for (const auto& Input:D.Inputs)
    {
        const auto* Definition=ResolveObject(Input.ObjectId,Error); if (!Definition) return false;
        const auto& Items=Input.From==EOGObjectActionParty::Ruler?PlayerItems:OtherItems;
        TArray<const FOGItemInstanceRecord*> Matches;
        for (const auto& Item:Items)
            if (!Used.Contains(Item.ItemId) && Item.DefinitionId==Input.ObjectId && Item.CurrentRankId==Input.RankId && Item.QualityId==Input.QualityId &&
                (Input.Disposition==EOGObjectInputDisposition::Retain || !Equipped.Contains(Item.ItemId))) Matches.Add(&Item);
        Matches.Sort([&](const FOGItemInstanceRecord& A,const FOGItemInstanceRecord& B)
        { if ((A.ItemId==C.SelectedItemId)!=(B.ItemId==C.SelectedItemId)) return A.ItemId==C.SelectedItemId; return A.ItemId.ToString()<B.ItemId.ToString(); });
        if (Matches.Num()<Input.InstanceCount)
            Out.MissingRequirements.Add(FString::Printf(TEXT("Requires %d available instances of %s with the authored rank/quality."),
                Input.InstanceCount,*Definition->DisplayName.ToString()));
        for (int32 I=0;I<FMath::Min(Input.InstanceCount,Matches.Num());++I)
        {
            const auto& Item=*Matches[I]; Used.Add(Item.ItemId);
            SelectedUsed|=Item.ItemId==C.SelectedItemId && Input.Disposition==EOGObjectInputDisposition::Consume && Input.From==EOGObjectActionParty::Ruler;
            FOGObjectActionSelectedInput Selection; Selection.Item=Item; Selection.DisplayName=Definition->DisplayName;
            Selection.Disposition=Input.Disposition;
            if (Input.Disposition==EOGObjectInputDisposition::Retain) Selection.NewOwnerId=Item.OwnerEntityId;
            else if (Input.Disposition==EOGObjectInputDisposition::Transfer)
                Selection.NewOwnerId=Input.From==EOGObjectActionParty::Ruler?C.CounterpartyId:C.RulerId;
            ItemQuote(Quote,Item);
            TArray<FOGItemModifierRecord> Modifiers;
            if (!Store.ListItemModifiers(Item.ItemId,Modifiers,Error)) return false;
            for (const auto& M:Modifiers)
            { AppendObjectServicePart(Quote,M.ModifierId.ToString()); AppendObjectServicePart(Quote,FString::FromInt(M.Ordinal)); AppendObjectServicePart(Quote,M.StateJson); }
            Out.Inputs.Add(MoveTemp(Selection));
        }
    }
    if (D.Kind==EOGObjectActionKind::Use && !SelectedUsed)
        Out.MissingRequirements.Add(TEXT("Select the owned consumable instance used by this action."));
    for (const auto& Change:D.Resources)
    {
        FOGObjectActionWalletPreview Wallet; Wallet.OwnerId=Party(C,Change.Account); Wallet.ResourceId=Change.ResourceId; Wallet.DisplayName=Change.DisplayName;
        bool Found=false; if (!Store.TryReadResourceBalance(Wallet.OwnerId,Change.ResourceId,Found,Wallet.Before,Error)) return false;
        if (!Found) Wallet.Before=0;
        if (Wallet.Before<0) { Error=TEXT("Stored resource balance is invalid."); return false; }
        if (Change.Amount<0 && Wallet.Before < -Change.Amount)
        { Out.MissingRequirements.Add(TEXT("Insufficient ")+Change.DisplayName.ToString()+(Change.Account==EOGObjectActionParty::Ruler?TEXT("."):TEXT(" in the counterparty stock."))); Wallet.After=Wallet.Before; }
        else if (Change.Amount>0 && Change.Amount>MAX_int64-Wallet.Before)
        { Out.MissingRequirements.Add(Change.DisplayName.ToString()+TEXT(" would exceed its balance limit.")); Wallet.After=Wallet.Before; }
        else Wallet.After=Wallet.Before+Change.Amount;
        AppendObjectServicePart(Quote,Wallet.OwnerId.ToString()); AppendObjectServicePart(Quote,Wallet.ResourceId.ToString()); AppendObjectServicePart(Quote,LexToString(Wallet.Before));
        Out.Wallets.Add(Wallet);
    }
    for (const auto& Output:D.Outputs)
    {
        const auto* ObjectDefinition=ResolveObject(Output.ObjectId,Error); if (!ObjectDefinition) return false;
        for (const auto& Value:{Output.ObjectId.ToString(),ObjectDefinition->InitialRankId.ToString(),ObjectDefinition->InitialQualityId.ToString(),
            ObjectDefinition->InitialDurabilityJson,ObjectDefinition->InitialEvolutionJson}) AppendObjectServicePart(Quote,Value);
    }
    Out.bCanApply=Out.MissingRequirements.IsEmpty(); Out.ConfirmationToken=ObjectActionStableDigest(Quote); return true;
}
bool FOGObjectActionService::Apply(const UOGObjectActionDefinition& Definition,const FOGObjectActionPreview& Reviewed,
    const FOGEntityId& Request,int64 Tick,bool& Already,TArray<FOGEntityId>& Created,FString& Error)
{ return ApplyImpl(Definition,Reviewed,Request,Tick,Already,Created,Error,true); }
bool FOGObjectActionService::ApplyWithinTransaction(const UOGObjectActionDefinition& Definition,const FOGObjectActionPreview& Reviewed,
    const FOGEntityId& Request,int64 Tick,bool& Already,TArray<FOGEntityId>& Created,FString& Error)
{ return ApplyImpl(Definition,Reviewed,Request,Tick,Already,Created,Error,false); }
bool FOGObjectActionService::ApplyImpl(const UOGObjectActionDefinition& D,const FOGObjectActionPreview& Reviewed,
    const FOGEntityId& RequestId,int64 Tick,bool& AlreadyApplied,TArray<FOGEntityId>& CreatedItems,FString& Error,bool OwnTransaction)
{
    AlreadyApplied=false; CreatedItems.Reset(); Error.Reset();
    if (!OwnTransaction && !Store.IsWriteTransactionActive())
    { Error=TEXT("Cross-system object execution requires the caller's active write transaction."); return false; }
    if (!IsInGameThread() || !RequestId.IsValid() || Reviewed.ConfirmationToken.IsEmpty() || Reviewed.ActionId!=D.DefinitionId || Tick<0 ||
        (OwnTransaction && !Store.BeginTransaction(Error))) return false;
    auto Fail=[&](){FString Ignored; if (OwnTransaction) Store.RollbackTransaction(Ignored); CreatedItems.Reset(); return false;};
    bool Found=false; FOGWorldEvent Previous;
    if (!Store.TryReadWorldEvent(RequestId,Found,Previous,Error)) return Fail();
    if (Found)
    {
        TSharedPtr<FJsonObject> Receipt; FString Action,Token,Owner,Other,Subject;
        if (Previous.EventType!=ActionEvent || Previous.PrimaryEntity!=Reviewed.Context.RulerId || !Object(Previous.PayloadJson,Receipt) ||
            !Receipt->TryGetStringField(TEXT("action_id"),Action) || Action!=D.DefinitionId.ToString() ||
            !Receipt->TryGetStringField(TEXT("confirmation_token"),Token) || Token!=Reviewed.ConfirmationToken ||
            !Receipt->TryGetStringField(TEXT("ruler_id"),Owner) || Owner!=Reviewed.Context.RulerId.ToString() ||
            !Receipt->TryGetStringField(TEXT("counterparty_id"),Other) || Other!=(Reviewed.Context.CounterpartyId.IsValid()?Reviewed.Context.CounterpartyId.ToString():TEXT("")) ||
            !Receipt->TryGetStringField(TEXT("selected_item_id"),Subject) || Subject!=(Reviewed.Context.SelectedItemId.IsValid()?Reviewed.Context.SelectedItemId.ToString():TEXT("")))
        { Error=TEXT("Request ID belongs to a different canonical object action."); return Fail(); }
        FString HealthRecipient;
        if ((Receipt->HasField(TEXT("health_recipient_id")) &&
             (!Receipt->TryGetStringField(TEXT("health_recipient_id"),HealthRecipient) ||
              HealthRecipient!=Reviewed.Context.HealthRecipientId.ToString())) ||
            (!Receipt->HasField(TEXT("health_recipient_id")) && Reviewed.Context.HealthRecipientId.IsValid()))
        { Error=TEXT("Request ID belongs to a different consumable recipient."); return Fail(); }
        const TArray<TSharedPtr<FJsonValue>>* Ids=nullptr;
        if (!Receipt->TryGetArrayField(TEXT("created_item_ids"),Ids)) { Error=TEXT("Stored object receipt is incomplete."); return Fail(); }
        for (const auto& Value:*Ids)
        {
            FGuid Id; FString Text;
            if (!Value->TryGetString(Text) || !FGuid::Parse(Text,Id) || !Id.IsValid()) { Error=TEXT("Stored created-item ID is invalid."); return Fail(); }
            CreatedItems.Add(FOGEntityId(Id));
        }
        if (OwnTransaction && !Store.CommitTransaction(Error)) return Fail();
        AlreadyApplied=true; return true;
    }
    FOGObjectActionPreview Fresh;
    if (!Preview(D,Reviewed.Context,Tick,Fresh,Error)) return Fail();
    if (!Fresh.bCanApply || Fresh.ConfirmationToken!=Reviewed.ConfirmationToken)
    { Error=Fresh.bCanApply?TEXT("Objects, stock, prices or installed definitions changed. Review again."):FString::Join(Fresh.MissingRequirements,TEXT("\n")); return Fail(); }
    for (const auto& Wallet:Fresh.Wallets)
        if (!Store.SetResourceBalance(Wallet.OwnerId,Wallet.ResourceId,Wallet.After,Error)) return Fail();
    TArray<TSharedPtr<FJsonValue>> Changes;
    for (const auto& Input:Fresh.Inputs)
    {
        if (Input.Disposition==EOGObjectInputDisposition::Retain) continue;
        auto Item=Input.Item; TSharedPtr<FJsonObject> History;
        if (!Object(Item.HistoryStateJson,History)) { Error=TEXT("Stored item provenance is invalid."); return Fail(); }
        History->SetStringField(TEXT("last_object_action_event"),RequestId.ToString());
        if (Input.Disposition==EOGObjectInputDisposition::Consume)
            History->SetStringField(TEXT("consumed_by_object_action_event"),RequestId.ToString());
        Item.HistoryStateJson=Write(History.ToSharedRef()); Item.OwnerEntityId=Input.NewOwnerId;
        if (!Store.DeleteContainerContentsForItem(Item.ItemId,Error) || !Store.DeleteEquipmentBindingsForItem(Item.ItemId,Error) ||
            !Store.UpsertItemInstance(Item,Tick,Error)) return Fail();
        auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("item_id"),Item.ItemId.ToString());
        Row->SetStringField(TEXT("definition_id"),Item.DefinitionId.ToString());
        Row->SetStringField(TEXT("owner_before"),Input.Item.OwnerEntityId.ToString());
        Row->SetStringField(TEXT("owner_after"),Item.OwnerEntityId.IsValid()?Item.OwnerEntityId.ToString():TEXT(""));
        Row->SetStringField(TEXT("disposition"),Input.Disposition==EOGObjectInputDisposition::Consume?TEXT("consumed"):TEXT("transferred"));
        Changes.Add(MakeShared<FJsonValueObject>(Row));
    }
    if (Fresh.bRepairsSubject)
    {
        auto Subject=Fresh.SubjectBefore; Subject.DurabilityStateJson=Fresh.SubjectDurabilityAfter;
        TSharedPtr<FJsonObject> History;
        if (!Object(Subject.HistoryStateJson,History)) { Error=TEXT("Stored repair provenance is invalid."); return Fail(); }
        History->SetStringField(TEXT("last_repair_event"),RequestId.ToString()); Subject.HistoryStateJson=Write(History.ToSharedRef());
        if (!Store.UpsertItemInstance(Subject,Tick,Error)) return Fail();
    }
    if (Fresh.bEnhancesSubject)
    {
        auto Subject=Fresh.SubjectBefore;
        Subject.CurrentRankId=Fresh.EnhancedRankId;
        Subject.QualityId=Fresh.EnhancedQualityId;
        TSharedPtr<FJsonObject> History;
        if (!Object(Subject.HistoryStateJson,History)) { Error=TEXT("Stored item provenance is invalid."); return Fail(); }
        History->SetStringField(TEXT("last_enhancement_event"),RequestId.ToString());
        Subject.HistoryStateJson=Write(History.ToSharedRef());
        if (!Store.UpsertItemInstance(Subject,Tick,Error)) return Fail();
    }
    int32 Ordinal=0;
    for (const auto& Output:D.Outputs)
    {
        const auto* Spec=ResolveObject(Output.ObjectId,Error); if (!Spec) return Fail();
        for (int32 I=0;I<Output.InstanceCount;++I)
        {
            FOGItemInstanceRecord Item; Item.ItemId=StableId(RequestId.ToString()+TEXT("|output|")+FString::FromInt(Ordinal++));
            bool Exists=false; FName Kind; FString State; int64 Revision=0;
            if (!Store.TryReadEntity(Item.ItemId,Exists,Kind,State,Revision,Error)) return Fail();
            if (Exists) { Error=TEXT("Output identity already exists without this action's receipt."); return Fail(); }
            Item.DefinitionId=Output.ObjectId; Item.OwnerEntityId=Party(Fresh.Context,Output.To);
            Item.CurrentRankId=Spec->InitialRankId; Item.QualityId=Spec->InitialQualityId;
            Item.DurabilityStateJson=Spec->InitialDurabilityJson; Item.EvolutionStateJson=Spec->InitialEvolutionJson;
            auto History=MakeShared<FJsonObject>(); History->SetStringField(TEXT("origin_object_action"),D.DefinitionId.ToString());
            History->SetStringField(TEXT("origin_event_id"),RequestId.ToString()); Item.HistoryStateJson=Write(History);
            if (!Store.UpsertItemInstance(Item,Tick,Error)) return Fail();
            CreatedItems.Add(Item.ItemId);
        }
    }
    if (Fresh.OfferId.IsValid())
    {
        TSharedPtr<FJsonObject> Offer;
        if (!Object(Fresh.OfferStateJson,Offer)) { Error=TEXT("Canonical offer snapshot is invalid."); return Fail(); }
        if (Fresh.OfferRemaining>0) Offer->SetStringField(TEXT("remaining_uses_exact"),LexToString(Fresh.OfferRemaining-1));
        if (!Store.UpsertEntity(Fresh.OfferId,TEXT("object_action_offer"),Tick,Write(Offer.ToSharedRef()),Error)) return Fail();
    }
    auto Receipt=MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("action_id"),D.DefinitionId.ToString());
    Receipt->SetStringField(TEXT("definition_fingerprint"),D.Fingerprint());
    Receipt->SetStringField(TEXT("confirmation_token"),Reviewed.ConfirmationToken);
    Receipt->SetStringField(TEXT("ruler_id"),Fresh.Context.RulerId.ToString());
    Receipt->SetStringField(TEXT("counterparty_id"),Fresh.Context.CounterpartyId.IsValid()?Fresh.Context.CounterpartyId.ToString():TEXT(""));
    Receipt->SetStringField(TEXT("selected_item_id"),Fresh.Context.SelectedItemId.IsValid()?Fresh.Context.SelectedItemId.ToString():TEXT(""));
    Receipt->SetArrayField(TEXT("item_changes"),MoveTemp(Changes));
    TArray<TSharedPtr<FJsonValue>> Outputs,Wallets;
    for (const auto& Id:CreatedItems) Outputs.Add(MakeShared<FJsonValueString>(Id.ToString()));
    Receipt->SetArrayField(TEXT("created_item_ids"),MoveTemp(Outputs));
    for (const auto& W:Fresh.Wallets)
    {
        auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("owner_id"),W.OwnerId.ToString());
        Row->SetStringField(TEXT("resource_id"),W.ResourceId.ToString()); Row->SetStringField(TEXT("before_exact"),LexToString(W.Before));
        Row->SetStringField(TEXT("after_exact"),LexToString(W.After)); Wallets.Add(MakeShared<FJsonValueObject>(Row));
    }
    Receipt->SetArrayField(TEXT("wallet_changes"),MoveTemp(Wallets));
    if (Fresh.bRepairsSubject)
    {
        Receipt->SetStringField(TEXT("repaired_item_id"),Fresh.SubjectBefore.ItemId.ToString());
        Receipt->SetStringField(TEXT("durability_before"),Fresh.SubjectBefore.DurabilityStateJson);
        Receipt->SetStringField(TEXT("durability_after"),Fresh.SubjectDurabilityAfter);
    }
    if (Fresh.bEnhancesSubject)
    {
        Receipt->SetStringField(TEXT("enhanced_item_id"),Fresh.SubjectBefore.ItemId.ToString());
        Receipt->SetStringField(TEXT("rank_before"),Fresh.SubjectBefore.CurrentRankId.ToString());
        Receipt->SetStringField(TEXT("rank_after"),Fresh.EnhancedRankId.ToString());
        Receipt->SetStringField(TEXT("quality_before"),Fresh.SubjectBefore.QualityId.ToString());
        Receipt->SetStringField(TEXT("quality_after"),Fresh.EnhancedQualityId.ToString());
    }
    if (Fresh.bRestoresHealth)
    {
        Receipt->SetStringField(TEXT("battle_id"),Fresh.HealthBattleId.ToString());
        Receipt->SetStringField(TEXT("health_recipient_id"),Fresh.Context.HealthRecipientId.ToString());
        auto Delta=MakeShared<FJsonObject>();
        Delta->SetStringField(TEXT("unit_id"),Fresh.Context.HealthRecipientId.ToString());
        Delta->SetStringField(TEXT("hp_before"),Fresh.HealthBefore.ToDebugString());
        Delta->SetStringField(TEXT("hp_after"),Fresh.HealthAfter.ToDebugString());
        TArray<TSharedPtr<FJsonValue>> Deltas;
        Deltas.Add(MakeShared<FJsonValueObject>(Delta));
        Receipt->SetArrayField(TEXT("unit_deltas"),MoveTemp(Deltas));
    }
    FOGWorldEvent Event; Event.EventId=RequestId; Event.EventType=ActionEvent; Event.WorldTick=Tick;
    Event.PrimaryEntity=Fresh.Context.RulerId; Event.bChronicleEligible=true; Event.PayloadJson=Write(Receipt);
    if (Fresh.Context.CounterpartyId.IsValid()) Event.RelatedEntities.Add(Fresh.Context.CounterpartyId);
    for (const auto& I:Fresh.Inputs) Event.RelatedEntities.AddUnique(I.Item.ItemId);
    for (const auto& Id:CreatedItems) Event.RelatedEntities.AddUnique(Id);
    if (Fresh.bRepairsSubject || Fresh.bEnhancesSubject) Event.RelatedEntities.AddUnique(Fresh.SubjectBefore.ItemId);
    if (Fresh.bRestoresHealth) Event.RelatedEntities.AddUnique(Fresh.Context.HealthRecipientId);
    if (!Store.AppendWorldEvent(Event,Error) ||
        (Fresh.bRestoresHealth && !FOGCombatVitalityService(Store).CommitConsumableRestoration(
            Fresh.Context.HealthRecipientId,D.RestoreHealthBps,Event,Error)) ||
        (OwnTransaction && !Store.CommitTransaction(Error))) return Fail();
    return true;
}
bool FOGObjectActionService::PublishOffer(const UOGObjectActionDefinition& Definition,
    const FOGEntityId& Counterparty, const FOGEntityId& AllowedRuler, int64 RemainingUses,
    int64 Opens, int64 Closes, const FOGEntityId& RequestId, int64 Tick, FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || !Store.BeginTransaction(Error)) return false;
    if (PublishOfferInTransaction(Definition, Counterparty, AllowedRuler, RemainingUses,
            Opens, Closes, RequestId, Tick, Error) && Store.CommitTransaction(Error)) return true;
    FString Ignored;
    Store.RollbackTransaction(Ignored);
    return false;
}

bool FOGObjectActionService::PublishOffers(const UOGObjectOfferDefinition& Profile,
    const FOGEntityId& Counterparty, int64 Tick, FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || !Profile.ValidateDefinition(Error)) return false;
    auto* Library = UOGDefinitionLibrary::Get();
    if (!Library)
    {
        Error = TEXT("The installed object action library is unavailable.");
        return false;
    }
    if (!Store.BeginTransaction(Error)) return false;

    for (const FOGObjectOfferSpecification& Offer : Profile.Offers)
    {
        const auto* Action = Cast<UOGObjectActionDefinition>(Library->Resolve(TEXT("object_action"), Offer.ActionId));
        // Keep the original publication identity so saved receipts still prevent stock refills.
        const FOGEntityId RequestId = StableId(TEXT("object_offer_allocation|") + Counterparty.ToString() +
            TEXT("|") + Offer.PublicationId.ToString() + TEXT("|") + Offer.ActionId.ToString());
        if (!Action || !PublishOfferInTransaction(*Action, Counterparty, Offer.AllowedRuler,
                Offer.RemainingUses, Offer.OpensTick, Offer.ClosesTick, RequestId, Tick, Error))
        {
            FString Ignored;
            Store.RollbackTransaction(Ignored);
            Error = TEXT("Could not publish offer ") + Offer.ActionId.ToString() +
                TEXT(". ") + (Error.IsEmpty() ? FString(TEXT("The installed allocation is unavailable.")) : Error);
            return false;
        }
    }
    if (Store.CommitTransaction(Error)) return true;
    FString Ignored;
    Store.RollbackTransaction(Ignored);
    return false;
}

bool FOGObjectActionService::PublishOfferInTransaction(const UOGObjectActionDefinition& D,
    const FOGEntityId& Counterparty, const FOGEntityId& AllowedRuler, int64 RemainingUses,
    int64 Opens, int64 Closes, const FOGEntityId& RequestId, int64 Tick, FString& Error)
{
    FString State; int64 Revision=0;
    if (!D.ValidateDefinition(Error) || !D.UsesCounterparty() || !RequestId.IsValid() || Tick<0 ||
        RemainingUses < -1 || Opens<0 || (Closes!=-1 && Closes<=Opens) ||
        !ReadParty(Store,Counterparty,D.CounterpartyEntityKind,State,Revision,Error)) return false;
    if (AllowedRuler.IsValid() && !ReadParty(Store,AllowedRuler,TEXT("ruler"),State,Revision,Error)) return false;
    auto Offer=MakeShared<FJsonObject>(); Offer->SetStringField(TEXT("action_id"),D.DefinitionId.ToString());
    Offer->SetStringField(TEXT("counterparty_id"),Counterparty.ToString());
    Offer->SetStringField(TEXT("allowed_ruler_id"),AllowedRuler.IsValid()?AllowedRuler.ToString():TEXT(""));
    Offer->SetStringField(TEXT("remaining_uses_exact"),LexToString(RemainingUses));
    Offer->SetStringField(TEXT("opens_tick_exact"),LexToString(Opens));
    Offer->SetStringField(TEXT("closes_tick_exact"),LexToString(Closes));
    const auto Id=OfferId(Counterparty,D.DefinitionId); const FString Json=Write(Offer);
    bool Found=false; FOGWorldEvent Prior;
    if (!Store.TryReadWorldEvent(RequestId,Found,Prior,Error)) return false;
    if (Found)
    {
        if (Prior.EventType!=OfferEvent || Prior.PrimaryEntity!=Id || Prior.PayloadJson!=Json)
        { Error=TEXT("Offer request ID belongs to a different event."); return false; }
        return true; // Never replenish already-consumed stock on publication retries.
    }
    if (!Store.UpsertEntity(Id,TEXT("object_action_offer"),Tick,Json,Error)) return false;
    FOGWorldEvent Event; Event.EventId=RequestId; Event.EventType=OfferEvent; Event.PrimaryEntity=Id;
    Event.RelatedEntities={Counterparty}; if (AllowedRuler.IsValid()) Event.RelatedEntities.Add(AllowedRuler);
    Event.WorldTick=Tick; Event.PayloadJson=Json;
    return Store.AppendWorldEvent(Event,Error);
}
