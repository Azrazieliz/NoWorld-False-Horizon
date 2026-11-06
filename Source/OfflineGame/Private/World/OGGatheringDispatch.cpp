#include "World/OGGatheringDispatch.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Runtime/OGCanonicalClockRuntime.h"
#include "Runtime/OGReportService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/OGDispatchService.h"
#include "World/OGDispatchAuthored.h"
#include "World/OGTerritoryControlService.h"

namespace
{
const FName ResolverKey(TEXT("noworld.gathering_dispatch.v1"));
const FName StartedEvent(TEXT("dispatch.gathering_started"));
const FName ResolvedEvent(TEXT("dispatch.gathering_resolved"));
const FName AuthoredStartedEvent(TEXT("dispatch.authored_started"));
const FName AuthoredResolvedEvent(TEXT("dispatch.authored_resolved"));

struct FGatheringPlan
{
    FOGContentId Definition,Resource;
    FOGEntityId Territory,Origin;
    FString Name,ResourceName;
    int64 Amount=0;
};

bool Parse(const FString& Json,TSharedPtr<FJsonObject>& Out)
{ return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Out) && Out.IsValid(); }
FString Serialize(const TSharedRef<FJsonObject>& Object)
{
    FString Result; FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Result)); return Result;
}
bool GuidField(const FJsonObject& Json,const TCHAR* Field,FOGEntityId& Out)
{
    FString Text; FGuid Guid;
    if (!Json.TryGetStringField(Field,Text) || !FGuid::Parse(Text,Guid) || !Guid.IsValid()) return false;
    Out=FOGEntityId(Guid); return true;
}
bool DecodePlan(const FOGDispatchRecord& Order,FGatheringPlan& Out)
{
    TSharedPtr<FJsonObject> Json; FString Schema,Definition,Resource,Amount;
    if (!Parse(Order.AbortPolicyJson,Json) ||
        !Json->TryGetStringField(TEXT("schema"),Schema) || Schema!=ResolverKey.ToString() ||
        !Json->TryGetStringField(TEXT("definition_id"),Definition) ||
        !Json->TryGetStringField(TEXT("name"),Out.Name) ||
        !GuidField(*Json,TEXT("territory_id"),Out.Territory)) return false;
    Out.Definition=FOGContentId(Definition);
    if (!Out.Definition.IsValid()) return false;

    if (Order.Type!=EOGDispatchType::Gathering)
    {
        FString AuthoredPlan;
        return GuidField(*Json,TEXT("origin_location_id"),Out.Origin) &&
            Json->TryGetStringField(TEXT("authored_plan"),AuthoredPlan) && !AuthoredPlan.IsEmpty();
    }

    Out.Origin=Order.TargetEntityId;
    if (!Json->TryGetStringField(TEXT("resource_id"),Resource) ||
        !Json->TryGetStringField(TEXT("resource_name"),Out.ResourceName) ||
        !Json->TryGetStringField(TEXT("amount_exact"),Amount) || !LexTryParseString(Out.Amount,*Amount) ||
        Amount!=LexToString(Out.Amount) || Out.Amount<=0) return false;
    Out.Resource=FOGContentId(Resource);
    return Out.Resource.IsValid();
}
TArray<TSharedPtr<FJsonValue>> DispatchPartyJson(const TArray<FOGEntityId>& Participants)
{
    TArray<TSharedPtr<FJsonValue>> Ids;
    for (const auto& Id:Participants) Ids.Add(MakeShared<FJsonValueString>(Id.ToString()));
    return Ids;
}

bool IsDistinctDispatchParty(const TArray<FOGEntityId>& Participants)
{
    TSet<FOGEntityId> Seen;
    for (const auto& Id:Participants)
    {
        if (!Id.IsValid() || Seen.Contains(Id)) return false;
        Seen.Add(Id);
    }
    return true;
}

FOGContentId DueContent(const FOGEntityId& Dispatch)
{ return FOGContentId(TEXT("gathering_dispatch:due/")+Dispatch.ToString()); }

bool HasHoldingAccess(IOGWorldStore& Store,const FOGEntityId& Ruler,const FOGEntityId& Territory,
    const FOGEntityId& Location,int64 Tick,bool& Allowed,FString& Error)
{
    TArray<FOGTerritoryClaimRecord> Claims;
    if (!Store.ListActiveClaimsForLocation(Location,Claims,Error)) return false;
    Allowed=false;
    if (!Claims.ContainsByPredicate([&](const auto& Claim){return Claim.RulerId==Ruler && Claim.TerritoryId==Territory;})) return true;
    FOGTerritoryEffectiveControlResult Control;
    if (!FOGTerritoryControlService(Store).EvaluateEffectiveControl(Territory,Tick,Control,Error)) return false;
    Allowed=!Control.bContested && Control.EffectiveRulerIds.Contains(Ruler);
    return true;
}
bool ReadStock(IOGWorldStore& Store,const FOGEntityId& Owner,const FOGContentId& Resource,int64& Out,FString& Error)
{
    bool Found=false; Out=0;
    if (!Store.TryReadResourceBalance(Owner,Resource,Found,Out,Error)) return false;
    if (!Found) Out=0;
    if (Out>=0) return true;
    Error=TEXT("The saved resource balance is invalid."); return false;
}
bool IsActiveOwned(IOGWorldStore& Store,const FOGEntityId& Ruler,const FOGEntityId& Participant,bool& Owned,FString& Error)
{
    FOGCharacterManifestationRecord Copy; bool Found=false;
    if (!Store.TryReadCharacterManifestation(Participant,Found,Copy,Error)) return false;
    Owned=Found && Copy.ManifestationId==Participant && Copy.OwningRulerId==Ruler &&
        Copy.LifecycleState==TEXT("active");
    return true;
}

bool RegisterOrder(IOGWorldStore& Store,FOGCanonicalClockRuntime& Clock,const FOGDispatchRecord& Order,FString& Error)
{
    TArray<FOGWorldDirectorScheduleRecord> Rows;
    if (!Store.ListWorldDirectorSchedulesByContent(DueContent(Order.DispatchId),Rows,Error)) return false;
    if (Rows.Num()!=1)
    { Error=TEXT("A gathering party is missing its unique saved deadline."); return false; }
    return Clock.RegisterDueAction(Rows[0].ScheduleId,ResolverKey,Order.OwnerEntityId,0,0,false,false,Error);
}
}

bool UOGGatheringDispatchDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("gathering_dispatch") || OriginalSourceReference.TrimStartAndEnd().IsEmpty() ||
        DisplayName.IsEmpty() || !LocationId.IsValid() || DurationTicks<=0)
    { Error=TEXT("Dispatch requires an original source, issuing holding and duration."); return false; }
    if (DispatchType!=EOGDispatchType::Gathering) return OGDispatchAuthored::Validate(*this,Error);
    if (ResourceId.IsValid() && !ResourceName.IsEmpty() && Amount>0 &&
        MinimumPartySize==1 && MaximumPartySize==1) return true;
    Error=TEXT("Gathering dispatch requires original stock, quantity and one participant.");
    return false;
}

const UOGGatheringDispatchDefinition* FOGGatheringDispatch::FindDefinition(const FOGContentId& Id)
{
    auto* Library=UOGDefinitionLibrary::Get();
    return Library?Cast<UOGGatheringDispatchDefinition>(Library->Resolve(TEXT("gathering_dispatch"),Id)):nullptr;
}

bool FOGGatheringDispatch::ReadAssignedParticipants(IOGWorldStore& Store,TSet<FOGEntityId>& Out,FString& Error)
{
    Out.Reset();
    TArray<FOGEntityId> Ids;
    // An existing reservation follows a copy even if its owner changes.
    if (!Store.ListDispatchIds(FOGEntityId(),true,Ids,Error)) return false;
    TSet<FOGEntityId> Assigned;
    for (const auto& Id:Ids)
    {
        bool Found=false;
        FOGDispatchRecord Order;
        if (!Store.TryReadDispatch(Id,Found,Order,Error)) return false;
        if (Found && Order.Status==EOGDispatchStatus::Active)
            for (const auto& Participant:Order.ParticipantEntityIds) Assigned.Add(Participant);
    }
    Out=MoveTemp(Assigned);
    return true;
}

bool FOGGatheringDispatch::IsAssigned(IOGWorldStore& Store,const FOGEntityId&,const FOGEntityId& Participant,
    bool& Assigned,FString& Error)
{
    Assigned=false;
    if (!Participant.IsValid()) return true;
    TSet<FOGEntityId> Reservations;
    if (!ReadAssignedParticipants(Store,Reservations,Error)) return false;
    Assigned=Reservations.Contains(Participant);
    return true;
}

bool FOGGatheringDispatch::Preview(const UOGGatheringDispatchDefinition& Definition,const FOGEntityId& Ruler,
    const FOGEntityId& Territory,const FOGEntityId& Participant,int64 Tick,FOGGatheringDispatchQuote& Out,FString& Error) const
{
    return PreviewParty(Definition,Ruler,Territory,TArray<FOGEntityId>{Participant},Tick,Out,Error);
}

bool FOGGatheringDispatch::PreviewParty(const UOGGatheringDispatchDefinition& Definition,const FOGEntityId& Ruler,
    const FOGEntityId& Territory,const TArray<FOGEntityId>& Participants,int64 Tick,
    FOGGatheringDispatchQuote& Out,FString& Error) const
{
    Out=FOGGatheringDispatchQuote(); Error.Reset();
    if (!Ruler.IsValid() || !Territory.IsValid() || !IsDistinctDispatchParty(Participants) || Tick<0 ||
        !Definition.ValidateDefinition(Error) || Definition.DurationTicks>MAX_int64-Tick)
    { if (Error.IsEmpty()) Error=TEXT("Select an actual holding and distinct owned copies."); return false; }
    Out.DefinitionId=Definition.DefinitionId; Out.ResourceId=Definition.ResourceId; Out.RulerId=Ruler;
    Out.ParticipantIds=Participants;
    if (Participants.Num()==1) Out.ParticipantId=Participants[0];
    Out.TerritoryId=Territory; Out.LocationId=Definition.LocationId;
    Out.MinimumPartySize=Definition.MinimumPartySize;
    Out.MaximumPartySize=Definition.MaximumPartySize;
    Out.Name=Definition.DisplayName.ToString(); Out.ResourceName=Definition.ResourceName.ToString();
    Out.Type=Definition.DispatchType;
    Out.TargetLocationId=Definition.LocationId;
    const bool bGathering=Out.Type==EOGDispatchType::Gathering;
    Out.Amount=(bGathering || Definition.bTransferHoldingStock)?Definition.Amount:0;
    Out.Duration=Definition.DurationTicks;
    if (!bGathering && !OGDispatchAuthored::Prepare(Store,Definition,Out,Error)) return false;
    if (!Out.bVisible) return true;
    if (Participants.Num()<Out.MinimumPartySize || Participants.Num()>Out.MaximumPartySize)
    {
        Out.Reason=Out.MinimumPartySize==Out.MaximumPartySize
            ? FString::Printf(TEXT("Select exactly %d participants."),Out.MinimumPartySize)
            : FString::Printf(TEXT("Select %d to %d participants."),Out.MinimumPartySize,Out.MaximumPartySize);
        return true;
    }
    if (!Out.Reason.IsEmpty()) return true;

    bool Access=false;
    TSet<FOGEntityId> Assigned;
    if (!HasHoldingAccess(Store,Ruler,Territory,Definition.LocationId,Tick,Access,Error) ||
        !ReadAssignedParticipants(Store,Assigned,Error) ||
        (Out.Amount>0 && !ReadStock(Store,Definition.LocationId,Definition.ResourceId,Out.Available,Error))) return false;
    if (!Access) Out.Reason=TEXT("This holding no longer controls the issuing location.");
    for (const auto& Participant:Participants)
    {
        if (!Out.Reason.IsEmpty()) break;
        bool Owned=false;
        if (!IsActiveOwned(Store,Ruler,Participant,Owned,Error)) return false;
        if (!Owned) Out.Reason=TEXT("A selected participant is no longer an active owned copy.");
        else if (Assigned.Contains(Participant)) Out.Reason=TEXT("A selected participant already has an active dispatch.");
        else
        {
            bool bFound=false;
            FOGManifestationManagementMetadataRecord Metadata;
            if (!Store.TryReadManifestationManagementMetadata(Participant,bFound,Metadata,Error)) return false;
            if (bFound && Metadata.bLocked) Out.Reason=TEXT("A selected participant is locked.");
        }
    }
    if (Out.Reason.IsEmpty() && Out.Available<Out.Amount)
        Out.Reason=TEXT("The issuing location does not have enough stock.");
    if (!Out.Reason.IsEmpty()) return true;
    if (!bGathering)
    {
        bool Qualified=false;
        if (!OGDispatchAuthored::CheckQualification(Store,Definition,Participants,Qualified,Error)) return false;
        if (!Qualified)
        {
            Out.Reason=FString::Printf(TEXT("This order requires %d participants with the authored capability."),
                Definition.MinimumQualifiedParticipants);
            return true;
        }
    }
    FString Key=Out.DefinitionId.ToString()+TEXT("|")+Out.RulerId.ToString()+TEXT("|")+Out.TerritoryId.ToString()+
        TEXT("|")+Out.LocationId.ToString()+TEXT("|")+Out.ParticipantId.ToString()+TEXT("|")+Out.ResourceId.ToString()+
        TEXT("|")+LexToString(Out.Amount)+TEXT("|")+LexToString(Out.Duration)+TEXT("|")+LexToString(Out.Available);
    if (!bGathering)
    {
        auto Contract=MakeShared<FJsonObject>();
        Contract->SetStringField(TEXT("gathering_key"),Key);
        Contract->SetStringField(TEXT("name"),Out.Name);
        Contract->SetNumberField(TEXT("risk"),Out.Risk);
        Contract->SetNumberField(TEXT("tolerance"),Out.RiskTolerance);
        Contract->SetStringField(TEXT("plan"),Out.AuthoredPlanJson);
        Contract->SetArrayField(TEXT("participants"),DispatchPartyJson(Participants));
        TArray<TSharedPtr<FJsonValue>> Objectives;
        for (const auto& Objective:Out.Objectives)
        {
            auto Row=MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("id"),Objective.ObjectiveId.Value);
            Row->SetBoolField(TEXT("mandatory"),Objective.bMandatory);
            Row->SetStringField(TEXT("rule"),Objective.StateJson);
            Objectives.Add(MakeShared<FJsonValueObject>(Row));
        }
        Contract->SetArrayField(TEXT("objectives"),Objectives);
        Key=Serialize(Contract);
    }
    FTCHARToUTF8 Text(*Key);
    FMD5 Hash;
    Hash.Update(reinterpret_cast<const uint8*>(Text.Get()),Text.Length());
    uint8 Digest[16];
    Hash.Final(Digest);
    Out.Token=bGathering?FMD5::HashAnsiString(*Key):BytesToHex(Digest,16);
    Out.bCanStart=true;
    return true;
}

bool FOGGatheringDispatch::Start(const FOGGatheringDispatchQuote& Quote,const FOGEntityId& Request,
    FOGCanonicalClockRuntime& Clock,FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || !Request.IsValid() || Quote.Token.IsEmpty())
    { Error=TEXT("Review a gathering dispatch before starting it."); return false; }
    const int64 Tick=Clock.GetCanonicalWorldTick();
    if (!Store.BeginTransaction(Error)) return false;
    auto Rollback=[&](){FString Ignored; Store.RollbackTransaction(Ignored); return false;};

    bool Found=false; FOGWorldEvent Previous;
    if (!Store.TryReadWorldEvent(Request,Found,Previous,Error)) return Rollback();
    if (Found)
    {
        TSharedPtr<FJsonObject> Saved; FString Token; FOGEntityId Id;
        FOGDispatchRecord Order;
        const FName Expected=Quote.Type==EOGDispatchType::Gathering?StartedEvent:AuthoredStartedEvent;
        if (Previous.EventType!=Expected || Previous.PrimaryEntity!=Quote.RulerId ||
            !Parse(Previous.PayloadJson,Saved) || !Saved->TryGetStringField(TEXT("confirmation_token"),Token) ||
            Token!=Quote.Token || !GuidField(*Saved,TEXT("dispatch_id"),Id) ||
            !Store.TryReadDispatch(Id,Found,Order,Error) || !Found || Order.OwnerEntityId!=Quote.RulerId ||
            Order.ParticipantEntityIds!=Quote.ParticipantIds)
        { if (Error.IsEmpty()) Error=TEXT("Request belongs to a different dispatch."); return Rollback(); }
        if (!Store.CommitTransaction(Error)) return Rollback();
        return RegisterOrder(Store,Clock,Order,Error);
    }
    const auto* Definition=FindDefinition(Quote.DefinitionId);
    FOGGatheringDispatchQuote Fresh;
    if (!Definition || !PreviewParty(*Definition,Quote.RulerId,Quote.TerritoryId,Quote.ParticipantIds,Tick,Fresh,Error) ||
        !Fresh.bCanStart || Fresh.Token!=Quote.Token)
    { if (Error.IsEmpty()) Error=Fresh.Reason.IsEmpty()?TEXT("The gathering order changed. Review it again."):Fresh.Reason; return Rollback(); }

    auto Plan=MakeShared<FJsonObject>();
    Plan->SetStringField(TEXT("schema"),ResolverKey.ToString()); Plan->SetStringField(TEXT("definition_id"),Fresh.DefinitionId.ToString());
    Plan->SetStringField(TEXT("resource_id"),Fresh.ResourceId.ToString()); Plan->SetStringField(TEXT("amount_exact"),LexToString(Fresh.Amount));
    Plan->SetStringField(TEXT("territory_id"),Fresh.TerritoryId.ToString()); Plan->SetStringField(TEXT("name"),Fresh.Name);
    Plan->SetStringField(TEXT("resource_name"),Fresh.ResourceName);
    const bool bGathering=Fresh.Type==EOGDispatchType::Gathering;
    if (bGathering)
    {
        FOGDispatchObjectiveRecord& Objective=Fresh.Objectives.AddDefaulted_GetRef();
        Objective.ObjectiveId=Fresh.DefinitionId;
        Objective.bMandatory=true;
        Objective.TargetEntityId=Fresh.LocationId;
    }
    else
    {
        Plan->SetStringField(TEXT("origin_location_id"),Fresh.LocationId.ToString());
        Plan->SetStringField(TEXT("authored_plan"),Fresh.AuthoredPlanJson);
    }
    FOGEntityId DispatchId;
    const int64 Due=Tick+Fresh.Duration;
    const int64 Seed=static_cast<int64>(GetTypeHash(Request));
    if (!FOGDispatchService(Store).StartDispatch(Fresh.RulerId,Fresh.TargetLocationId,Fresh.Type,
        Fresh.ParticipantIds,Tick,Due,Fresh.Risk,Fresh.RiskTolerance,Serialize(Plan),Fresh.Objectives,
        Fresh.Constraints,Seed,DispatchId,Error)) return Rollback();

    auto Provenance=MakeShared<FJsonObject>();
    Provenance->SetStringField(TEXT("audit_reason"),TEXT("Resolve the authored dispatch at its saved deadline."));
    Provenance->SetObjectField(TEXT("world_state_inputs"),MakeShared<FJsonObject>());
    Provenance->SetObjectField(TEXT("package_versions"),MakeShared<FJsonObject>());
    Provenance->SetStringField(TEXT("dispatch_id"),DispatchId.ToString());
    FOGWorldDirectorScheduleRequest Schedule;
    Schedule.ContentId=DueContent(DispatchId); Schedule.EligibleSinceWorldTick=Tick;
    Schedule.EarliestStartWorldTick=Due; Schedule.LatestStartWorldTick=Due; Schedule.ResolutionSeed=Seed;
    Schedule.DecisionProvenanceJson=Serialize(Provenance);
    FOGWorldDirectorScheduleRecord Deadline;
    if (!FOGWorldDirectorService(Store).ScheduleEligibleContent(Schedule,Deadline,Error)) return Rollback();

    auto Receipt=MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("dispatch_id"),DispatchId.ToString());
    Receipt->SetStringField(TEXT("confirmation_token"),Fresh.Token);
    Receipt->SetArrayField(TEXT("participant_ids"),DispatchPartyJson(Fresh.ParticipantIds));
    FOGWorldEvent Event; Event.EventId=Request; Event.EventType=bGathering?StartedEvent:AuthoredStartedEvent; Event.WorldTick=Tick;
    Event.PrimaryEntity=Fresh.RulerId; Event.RelatedEntities={DispatchId,Fresh.LocationId};
    for (const auto& Participant:Fresh.ParticipantIds) Event.RelatedEntities.AddUnique(Participant);
    // The target can be unknown until a successful discovery.
    if (bGathering) Event.RelatedEntities.AddUnique(Fresh.TargetLocationId);
    Event.PayloadJson=Serialize(Receipt); Event.bChronicleEligible=true;
    if (!Store.AppendWorldEvent(Event,Error) || !Store.CommitTransaction(Error)) return Rollback();
    // The saved Dispatch and Director schedule survive interruption before clock registration.
    FString RegistrationError;
    if (!Clock.RegisterDueAction(Deadline.ScheduleId,ResolverKey,Fresh.RulerId,0,0,false,false,RegistrationError))
        Error=TEXT("The dispatch was saved. Its deadline will reconnect when the world reloads. ")+RegistrationError;
    return true;
}

bool FOGGatheringDispatch::AttachClock(IOGWorldStore& Store,FOGCanonicalClockRuntime& Clock,FString& Error)
{
    Clock.RegisterResolver(ResolverKey,&FOGGatheringDispatch::Resolve);
    TArray<FOGEntityId> Ids;
    if (!Store.ListDispatchIds(FOGEntityId(),true,Ids,Error)) return false;
    for (const auto& Id:Ids)
    {
        bool Found=false; FOGDispatchRecord Order; FGatheringPlan Plan;
        if (!Store.TryReadDispatch(Id,Found,Order,Error)) return false;
        if (Found && DecodePlan(Order,Plan) && !RegisterOrder(Store,Clock,Order,Error)) return false;
    }
    return true;
}

bool FOGGatheringDispatch::Resolve(IOGWorldStore& Store,const FOGWorldDirectorScheduleRecord& Schedule,
    int64 Tick,bool bOffline,FString& Outcome,FString& Error)
{
    TSharedPtr<FJsonObject> Provenance; FOGEntityId Id;
    if (!Parse(Schedule.DecisionProvenanceJson,Provenance) || !GuidField(*Provenance,TEXT("dispatch_id"),Id) ||
        Schedule.ContentId!=DueContent(Id) || Schedule.Status!=TEXT("active") ||
        !Schedule.bHasScheduledStartWorldTick || Tick<Schedule.ScheduledStartWorldTick)
    { Error=TEXT("The gathering schedule does not prove its saved dispatch."); return false; }
    if (!Store.BeginTransaction(Error)) return false;
    auto Rollback=[&](){FString Ignored; Store.RollbackTransaction(Ignored); return false;};
    bool Found=false; FOGDispatchRecord Order; FGatheringPlan Plan;
    if (!Store.TryReadDispatch(Id,Found,Order,Error) || !Found || !DecodePlan(Order,Plan) ||
        Order.ResolveWorldTick!=Schedule.ScheduledStartWorldTick || Order.ResolutionSeed!=Schedule.ResolutionSeed ||
        Order.ParticipantEntityIds.IsEmpty() || !IsDistinctDispatchParty(Order.ParticipantEntityIds) ||
        (Order.Type==EOGDispatchType::Gathering && Order.ParticipantEntityIds.Num()!=1))
    { if (Error.IsEmpty()) Error=TEXT("The authored gathering dispatch is no longer readable."); return Rollback(); }
    if (Order.Status!=EOGDispatchStatus::Active)
    {
        Outcome=Order.ResultJson;
        if (!Store.CommitTransaction(Error)) return Rollback();
        return true;
    }

    FOGDispatchRecord Settled;
    const bool bGathering=Order.Type==EOGDispatchType::Gathering;
    const auto Resolver=[&](const FOGDispatchResolutionContext& Context,FOGDispatchResolutionResult& Result,FString& Failure)
    {
        if (!bGathering)
        {
            bool Access=false;
            if (!HasHoldingAccess(Store,Order.OwnerEntityId,Plan.Territory,Plan.Origin,Tick,Access,Failure)) return false;
            TArray<FOGEntityId> Unavailable;
            for (const auto& Participant:Order.ParticipantEntityIds)
            {
                bool Owned=false;
                if (!IsActiveOwned(Store,Order.OwnerEntityId,Participant,Owned,Failure)) return false;
                if (!Owned) Unavailable.Add(Participant);
            }
            return OGDispatchAuthored::Evaluate(Store,Order,Context,Schedule.ScheduleId,Tick,bOffline,Access,Unavailable,Result,Failure);
        }
        if (Context.MandatoryObjectives.Num()!=1 || !Context.SecondaryObjectives.IsEmpty() || !Context.HardConstraints.IsEmpty() ||
            Context.MandatoryObjectives[0].ObjectiveId!=Plan.Definition ||
            Context.MandatoryObjectives[0].TargetEntityId!=Order.TargetEntityId)
        { Failure=TEXT("The dispatch objective no longer matches its authored gathering plan."); return false; }

        bool Access=false,Owned=false;
        int64 Source=0,Wallet=0;
        if (!HasHoldingAccess(Store,Order.OwnerEntityId,Plan.Territory,Order.TargetEntityId,Tick,Access,Failure) ||
            !IsActiveOwned(Store,Order.OwnerEntityId,Order.ParticipantEntityIds[0],Owned,Failure) ||
            !ReadStock(Store,Order.TargetEntityId,Plan.Resource,Source,Failure) ||
            !ReadStock(Store,Order.OwnerEntityId,Plan.Resource,Wallet,Failure)) return false;
        Result.bMandatoryObjectivesEvaluated=true; Result.bHardConstraintsEvaluated=true;
        Result.bSurvivalAbortPolicyEvaluated=true;
        const bool Success=Access && Owned && Source>=Plan.Amount && Wallet<=MAX_int64-Plan.Amount;
        if (Success && (!Store.SetResourceBalance(Order.TargetEntityId,Plan.Resource,Source-Plan.Amount,Failure) ||
            !Store.SetResourceBalance(Order.OwnerEntityId,Plan.Resource,Wallet+Plan.Amount,Failure))) return false;
        Result.bSucceeded=Success;
        Result.OutcomeState=Success?TEXT("resources_delivered"):TEXT("returned_without_stock");
        auto Json=MakeShared<FJsonObject>();
        Json->SetStringField(TEXT("resource_id"),Plan.Resource.ToString());
        Json->SetStringField(TEXT("delivered_exact"),LexToString(Success?Plan.Amount:0));
        Json->SetStringField(TEXT("summary"),Success?FText::AsNumber(Plan.Amount).ToString()+TEXT(" ")+Plan.ResourceName+TEXT(" delivered."):
            !Access?TEXT("The holding lost access before collection."):!Owned?TEXT("The assigned copy is no longer available."):
            Source<Plan.Amount?TEXT("The site stock changed before collection."):TEXT("The receiving resource balance is full."));
        Json->SetBoolField(TEXT("offline"),bOffline);
        Result.ResultJson=Serialize(Json);
        return true;
    };
    if (!FOGDispatchService(Store).ResolveDispatchWithPolicy(Id,Tick,Resolver,Settled,Error)) return Rollback();
    FOGWorldEvent Receipt; Receipt.EventId=Schedule.ScheduleId; Receipt.EventType=bGathering?ResolvedEvent:AuthoredResolvedEvent;
    Receipt.WorldTick=Tick; Receipt.PrimaryEntity=Order.OwnerEntityId; Receipt.RelatedEntities={Id};
    for (const auto& Participant:Order.ParticipantEntityIds) Receipt.RelatedEntities.AddUnique(Participant);
    if (bGathering) Receipt.RelatedEntities.AddUnique(Order.TargetEntityId);
    Receipt.PayloadJson=Settled.ResultJson; Receipt.bChronicleEligible=true;
    FOGEntityId Report;
    if (!Store.AppendWorldEvent(Receipt,Error) ||
        !FOGReportService(Store).CreateReport(Order.OwnerEntityId,Receipt.EventId,TEXT("dispatch"),
            0,Tick,Receipt.PayloadJson,Report,Error) || !Store.CommitTransaction(Error)) return Rollback();
    Outcome=MoveTemp(Settled.ResultJson);
    return true;
}

bool FOGGatheringDispatch::ReadOrders(const FOGEntityId& Ruler,TArray<FOGDispatchRecord>& Out,FString& Error) const
{
    Out.Reset();
    if (!Ruler.IsValid())
    { Error=TEXT("Dispatch history requires a selected Ruler."); return false; }
    TArray<FOGEntityId> Ids;
    if (!Store.ListDispatchIds(Ruler,false,Ids,Error)) return false;
    for (const auto& Id:Ids)
    {
        bool Found=false; FOGDispatchRecord Order; FGatheringPlan Plan;
        if (!Store.TryReadDispatch(Id,Found,Order,Error)) return false;
        if (Found && DecodePlan(Order,Plan)) Out.Add(MoveTemp(Order));
    }
    return true;
}

bool FOGGatheringDispatch::ReadSummary(const FOGDispatchRecord& Order,FString& Name,FString& Outcome)
{
    FGatheringPlan Plan;
    if (!DecodePlan(Order,Plan)) return false;
    Name=Plan.Name;
    TSharedPtr<FJsonObject> Result;
    if (Order.Status==EOGDispatchStatus::Active)
        Outcome=TEXT("Due at world tick ")+FText::AsNumber(Order.ResolveWorldTick).ToString();
    else if (!Parse(Order.ResultJson,Result) || !Result->TryGetStringField(TEXT("summary"),Outcome))
        Outcome=Order.OutcomeState.ToString();
    else if (Order.Type!=EOGDispatchType::Gathering)
    {
        const TArray<TSharedPtr<FJsonValue>>* Objectives=nullptr;
        if (Result->TryGetArrayField(TEXT("objectives"),Objectives) && Objectives)
            for (const auto& Value:*Objectives)
            {
                const TSharedPtr<FJsonObject>* Objective=nullptr;
                FString Label;
                bool bMet=false;
                if (Value.IsValid() && Value->TryGetObject(Objective) && Objective && Objective->IsValid() &&
                    (*Objective)->TryGetStringField(TEXT("name"),Label) &&
                    (*Objective)->TryGetBoolField(TEXT("met"),bMet))
                    Outcome+=TEXT("\n")+Label+(bMet?TEXT(" / met"):TEXT(" / unmet"));
            }
    }
    return true;
}
