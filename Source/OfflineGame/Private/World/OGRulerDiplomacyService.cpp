#include "World/OGRulerDiplomacyService.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Persistence/OGWorldStore.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/OGFactionWarService.h"
#include "World/OGFactionDiplomacyPolicy.h"
#include "Runtime/OGReportService.h"

namespace
{
const FName DiplomacyEvent(TEXT("faction.diplomacy_changed"));
const FName ProposalEvent(TEXT("faction.proposal_resolved"));

bool AsProposal(EOGRulerDiplomacyAction Action,EOGFactionProposal& Out)
{
    switch (Action)
    {
    case EOGRulerDiplomacyAction::RequestAlliance: Out=EOGFactionProposal::Alliance; return true;
    case EOGRulerDiplomacyAction::RequestSupport: Out=EOGFactionProposal::Support; return true;
    case EOGRulerDiplomacyAction::OfferSubordination: Out=EOGFactionProposal::Subordination; return true;
    default: return false;
    }
}

FString ReadDisplayName(const FString& Json)
{
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Object) || !Object) return {};
    FString Name;
    for (const TCHAR* Key:{TEXT("display_name"),TEXT("name"),TEXT("title")})
        if (Object->TryGetStringField(Key,Name) && !Name.TrimStartAndEnd().IsEmpty()) return Name.TrimStartAndEnd();
    return {};
}

bool IsRuler(IOGWorldStore& Store,const FOGEntityId& Id,FString& Error)
{
    bool Found=false; FName Kind; FString Json; int64 Revision=0;
    if (Id.IsValid() && Store.TryReadEntity(Id,Found,Kind,Json,Revision,Error) &&
        Found && Kind==TEXT("ruler") && !Json.Contains(TEXT("\"diagnosticFixture\""))) return true;
    if (Error.IsEmpty()) Error=TEXT("Diplomacy requires an existing production Ruler.");
    return false;
}

bool ReadProductionFaction(IOGWorldStore& Store,const FOGEntityId& Id,bool& Found,
    FOGFactionRecord& Out,FString& State,FString& Error)
{
    if (!Store.TryReadFaction(Id,Found,Out,Error)) return false;
    if (!Found) return true;
    FName Kind; int64 Revision=0;
    if (!Store.TryReadEntity(Id,Found,Kind,State,Revision,Error)) return false;
    Found=Found && Kind==TEXT("faction") && !State.Contains(TEXT("\"diagnosticFixture\""));
    return true;
}

bool LoadLinks(IOGWorldStore& Store,const FOGEntityId& Own,const FOGEntityId& Other,
    FOGRulerDiplomacyLinks& Links,FString& Error)
{
    Links=FOGRulerDiplomacyLinks();
    bool Found=false;
    auto Read=[&](const FOGEntityId& From,const FOGEntityId& To,EOGFactionLinkType Type,FOGFactionLinkRecord& Row)
    {
        if (!Store.TryReadFactionLink(From,To,Type,Found,Row,Error)) return false;
        if (!Found) Row.bActive=false;
        return true;
    };
    auto A=Own,B=Other;
    if (A.ToString().Compare(B.ToString(),ESearchCase::CaseSensitive)>0) Swap(A,B);
    return Read(A,B,EOGFactionLinkType::Alliance,Links.Alliance) &&
        Read(Own,Other,EOGFactionLinkType::Support,Links.OutgoingSupport) &&
        Read(Other,Own,EOGFactionLinkType::Support,Links.IncomingSupport) &&
        Read(Own,Other,EOGFactionLinkType::Subordination,Links.Subordination);
}

const TCHAR* ActionKey(EOGRulerDiplomacyAction Action)
{
    switch (Action)
    {
    case EOGRulerDiplomacyAction::EndAlliance: return TEXT("end_alliance");
    case EOGRulerDiplomacyAction::PledgeSupport: return TEXT("pledge_support");
    case EOGRulerDiplomacyAction::WithdrawSupport: return TEXT("withdraw_support");
    case EOGRulerDiplomacyAction::EndSubordination: return TEXT("end_subordination");
    case EOGRulerDiplomacyAction::RequestAlliance: return TEXT("request_alliance");
    case EOGRulerDiplomacyAction::RequestSupport: return TEXT("request_support");
    case EOGRulerDiplomacyAction::OfferSubordination: return TEXT("offer_subordination");
    }
    return TEXT("");
}
}

FString FOGRulerDiplomacyService::ActionLabel(EOGRulerDiplomacyAction Action)
{
    switch (Action)
    {
    case EOGRulerDiplomacyAction::EndAlliance: return TEXT("End alliance");
    case EOGRulerDiplomacyAction::PledgeSupport: return TEXT("Pledge support");
    case EOGRulerDiplomacyAction::WithdrawSupport: return TEXT("Withdraw support");
    case EOGRulerDiplomacyAction::EndSubordination: return TEXT("End subordination");
    case EOGRulerDiplomacyAction::RequestAlliance: return TEXT("Request alliance");
    case EOGRulerDiplomacyAction::RequestSupport: return TEXT("Request their support");
    case EOGRulerDiplomacyAction::OfferSubordination: return TEXT("Offer subordination");
    }
    return {};
}

bool FOGRulerDiplomacyService::ReadFactions(const FOGEntityId& Ruler,TArray<FOGRulerDiplomacyFaction>& Led,
    TArray<FOGRulerDiplomacyFaction>& Known,FString& Error) const
{
    Led.Reset(); Known.Reset(); Error.Reset();
    if (!IsRuler(Store,Ruler,Error)) return false;
    TArray<FOGEntityId> Owned;
    if (!Store.ListFactionIdsLedBy(Ruler,Owned,Error)) return false;
    TSet<FOGEntityId> Added;
    for (const auto& Id:Owned)
    {
        bool Found=false; FOGFactionRecord Faction; FString State;
        if (!ReadProductionFaction(Store,Id,Found,Faction,State,Error)) return false;
        if (!Found || Faction.LeaderRulerId!=Ruler) continue;
        FString Name=ReadDisplayName(State);
        if (Name.IsEmpty()) Name=TEXT("Faction / ")+Id.ToString().Left(8);
        Led.Add({Id,MoveTemp(Name)});
        Added.Add(Id);
    }
    TArray<FOGKnowledgeFactRecord> Facts;
    if (!Store.ListKnowledgeFactsByOwner(Ruler,Facts,Error)) return false;
    Facts.Sort([](const auto& A,const auto& B){return A.UpdatedWorldTick>B.UpdatedWorldTick;});
    TMap<FOGEntityId,FString> Contacts;
    for (const auto& Fact:Facts)
    {
        if (Fact.OwnerEntityId!=Ruler || !Fact.SubjectEntityId.IsValid() || Added.Contains(Fact.SubjectEntityId) ||
            Fact.BeliefState==TEXT("refuted") || Fact.ConfidenceBps<=0) continue;
        auto& Name=Contacts.FindOrAdd(Fact.SubjectEntityId);
        if (Name.IsEmpty()) Name=ReadDisplayName(Fact.ValueJson);
    }
    for (auto& Contact:Contacts)
    {
        bool Found=false; FOGFactionRecord Faction; FString State;
        if (!ReadProductionFaction(Store,Contact.Key,Found,Faction,State,Error)) return false;
        if (!Found) continue;
        if (Contact.Value.IsEmpty()) Contact.Value=TEXT("Known faction / ")+Contact.Key.ToString().Left(8);
        Known.Add({Contact.Key,MoveTemp(Contact.Value)});
    }
    auto ByName=[](const auto& A,const auto& B)
    { return A.Name==B.Name?A.Id.ToString()<B.Id.ToString():A.Name<B.Name; };
    Led.Sort(ByName); Known.Sort(ByName);
    return true;
}

bool FOGRulerDiplomacyService::ReadLinks(const FOGEntityId& Ruler,const FOGEntityId& Own,
    const FOGEntityId& Other,FOGRulerDiplomacyLinks& Out,FString& Error) const
{
    TArray<FOGRulerDiplomacyFaction> Led,Known;
    if (!ReadFactions(Ruler,Led,Known,Error)) return false;
    if (!Led.ContainsByPredicate([&](const auto& F){return F.Id==Own;}) ||
        !Known.ContainsByPredicate([&](const auto& F){return F.Id==Other;}))
    { Error=TEXT("Choose a led faction and a known foreign faction."); return false; }
    return LoadLinks(Store,Own,Other,Out,Error);
}

bool FOGRulerDiplomacyService::HasResponse(const FOGEntityId& Other,EOGRulerDiplomacyAction Action)
{
    EOGFactionProposal Proposal;
    FString Error;
    return AsProposal(Action,Proposal) && UOGFactionDiplomacyPolicy::Find(Other,Proposal,Error)!=nullptr;
}

bool FOGRulerDiplomacyService::ReadRecentReplies(const FOGEntityId& Ruler,const FOGEntityId& Own,
    const FOGEntityId& Other,TArray<FString>& Out,FString& Error) const
{
    Out.Reset();
    FOGRulerDiplomacyLinks Links;
    if (!ReadLinks(Ruler,Own,Other,Links,Error)) return false;
    TArray<FOGWorldEvent> Events;
    if (!Store.ListWorldEvents(Other,ProposalEvent,false,20,Events,Error)) return false;
    for (const auto& Event:Events)
    {
        if (Event.PrimaryEntity!=Ruler || !Event.RelatedEntities.Contains(Own) || !Event.RelatedEntities.Contains(Other)) continue;
        TSharedPtr<FJsonObject> Payload;
        FString From,To,Reply;
        bool Accepted=false;
        if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Event.PayloadJson),Payload) && Payload &&
            Payload->TryGetStringField(TEXT("acting_faction_id"),From) && From==Own.ToString() &&
            Payload->TryGetStringField(TEXT("foreign_faction_id"),To) && To==Other.ToString() &&
            Payload->TryGetStringField(TEXT("reply"),Reply) && Payload->TryGetBoolField(TEXT("accepted"),Accepted))
            Out.Add(FString(Accepted?TEXT("Accepted / "):TEXT("Refused / "))+Reply);
        if (Out.Num()==3) break;
    }
    return true;
}

bool FOGRulerDiplomacyService::Review(const FOGEntityId& Ruler,const FOGEntityId& Own,const FOGEntityId& Other,
    EOGRulerDiplomacyAction Action,FOGRulerDiplomacyReview& Out,FString& Error) const
{
    Out=FOGRulerDiplomacyReview();
    TArray<FOGRulerDiplomacyFaction> Led,Known;
    if (!ReadFactions(Ruler,Led,Known,Error)) return false;
    const auto* Actor=Led.FindByPredicate([&](const auto& F){return F.Id==Own;});
    const auto* Target=Known.FindByPredicate([&](const auto& F){return F.Id==Other;});
    if (!Actor || !Target || !*ActionKey(Action))
    { Error=TEXT("The faction, its leadership or the chosen command has changed."); return false; }
    FOGRulerDiplomacyLinks Links;
    if (!LoadLinks(Store,Own,Other,Links,Error)) return false;
    EOGFactionProposal Proposal;
    const bool IsProposal=AsProposal(Action,Proposal);
    const auto& Link=(Action==EOGRulerDiplomacyAction::EndAlliance || Action==EOGRulerDiplomacyAction::RequestAlliance)
        ?Links.Alliance:(Action==EOGRulerDiplomacyAction::EndSubordination || Action==EOGRulerDiplomacyAction::OfferSubordination)
        ?Links.Subordination:Action==EOGRulerDiplomacyAction::RequestSupport?Links.IncomingSupport:Links.OutgoingSupport;
    const bool ActiveAfter=IsProposal || Action==EOGRulerDiplomacyAction::PledgeSupport;
    Out.RulerId=Ruler; Out.OwnFactionId=Own; Out.OtherFactionId=Other;
    Out.OwnName=Actor->Name; Out.OtherName=Target->Name; Out.Action=Action;
    if (ActiveAfter==Link.bActive)
    { Out.Reason=ActiveAfter?TEXT("This agreement is already active."):TEXT("This agreement is no longer active."); return true; }
    FString Quote=Ruler.ToString()+TEXT("|")+Own.ToString()+TEXT("|")+Other.ToString()+TEXT("|")+
        ActionKey(Action)+TEXT("|")+LexToString(Link.UpdatedWorldTick);
    if (IsProposal)
    {
        if (Proposal==EOGFactionProposal::Subordination)
        {
            FOGFactionLinkRecord Reverse; bool Found=false;
            if (!Store.TryReadFactionLink(Other,Own,EOGFactionLinkType::Subordination,Found,Reverse,Error)) return false;
            if (Found && Reverse.bActive)
            { Out.Reason=TEXT("This faction is already subordinate to yours."); return true; }
        }
        const auto* Policy=UOGFactionDiplomacyPolicy::Find(Other,Proposal,Error);
        if (!Policy)
        {
            if (!Error.IsEmpty()) return false;
            Out.Reason=TEXT("The foreign faction has no installed response for this request.");
            return true;
        }
        Quote+=TEXT("|")+Policy->Fingerprint();
    }
    Out.Token=FMD5::HashAnsiString(*Quote);
    Out.bCanApply=true;
    return true;
}

bool FOGRulerDiplomacyService::Confirm(const FOGRulerDiplomacyReview& Reviewed,
    const FOGEntityId& Request,int64 Tick,FString& Error,FString* Reply)
{
    Error.Reset();
    if (Reply) Reply->Reset();
    EOGFactionProposal Proposal;
    const bool IsProposal=AsProposal(Reviewed.Action,Proposal);
    const FName EventType=IsProposal?ProposalEvent:DiplomacyEvent;
    if (!IsInGameThread() || !Request.IsValid() || Reviewed.Token.IsEmpty() || Tick<0)
    { Error=TEXT("A current diplomacy review is required."); return false; }
    if (!Store.BeginTransaction(Error)) return false;
    auto Rollback=[&](){FString Ignored; Store.RollbackTransaction(Ignored); return false;};
    FOGWorldEvent Previous; bool Found=false;
    if (!Store.TryReadWorldEvent(Request,Found,Previous,Error)) return Rollback();
    if (Found)
    {
        TSharedPtr<FJsonObject> Proof; FString Token;
        if (Previous.EventType!=EventType || Previous.PrimaryEntity!=Reviewed.RulerId ||
            !Previous.RelatedEntities.Contains(Reviewed.OwnFactionId) ||
            !Previous.RelatedEntities.Contains(Reviewed.OtherFactionId) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Previous.PayloadJson),Proof) ||
            !Proof || !Proof->TryGetStringField(TEXT("confirmation_token"),Token) || Token!=Reviewed.Token)
        { Error=TEXT("This request belongs to a different diplomacy command."); return Rollback(); }
        if (Reply && IsProposal)
        {
            FString Text; bool Accepted=false;
            if (!Proof->TryGetStringField(TEXT("reply"),Text) || !Proof->TryGetBoolField(TEXT("accepted"),Accepted))
            { Error=TEXT("The saved faction response is incomplete."); return Rollback(); }
            *Reply=FString(Accepted?TEXT("Accepted / "):TEXT("Refused / "))+Text;
        }
        if (!Store.CommitTransaction(Error)) return Rollback();
        return true;
    }
    FOGRulerDiplomacyReview Fresh;
    if (!Review(Reviewed.RulerId,Reviewed.OwnFactionId,Reviewed.OtherFactionId,Reviewed.Action,Fresh,Error)) return Rollback();
    if (!Fresh.bCanApply || Fresh.Token!=Reviewed.Token)
    { Error=Fresh.Reason.IsEmpty()?TEXT("Diplomacy state changed. Review the command again."):Fresh.Reason; return Rollback(); }

    FOGFactionWarService Factions(Store);
    const auto& Own=Fresh.OwnFactionId;
    const auto& Other=Fresh.OtherFactionId;
    const UOGFactionDiplomacyPolicy* Policy=nullptr;
    const FOGFactionProposalRule* Decision=nullptr;
    if (IsProposal)
    {
        Policy=UOGFactionDiplomacyPolicy::Find(Other,Proposal,Error);
        FOGFactionRecord Requester,Responder; FString State; bool Found=false;
        if (!Policy || !ReadProductionFaction(Store,Own,Found,Requester,State,Error) || !Found ||
            !ReadProductionFaction(Store,Other,Found,Responder,State,Error) || !Found ||
            !Policy->Decide(Store,Proposal,Requester,Responder,Decision,Error) || !Decision)
        { if (Error.IsEmpty()) Error=TEXT("The foreign faction can no longer respond."); return Rollback(); }
    }

    bool Applied=false;
    switch (Fresh.Action)
    {
    case EOGRulerDiplomacyAction::EndAlliance: Applied=Factions.BreakAlliance(Own,Other,Tick,Error); break;
    case EOGRulerDiplomacyAction::PledgeSupport: Applied=Factions.SetSupport(Own,Other,true,Tick,Error); break;
    case EOGRulerDiplomacyAction::WithdrawSupport: Applied=Factions.SetSupport(Own,Other,false,Tick,Error); break;
    case EOGRulerDiplomacyAction::EndSubordination: Applied=Factions.SetSubordination(Own,Other,false,Tick,Error); break;
    case EOGRulerDiplomacyAction::RequestAlliance:
        Applied=!Decision->bAccept || Factions.FormAlliance(Own,Other,Tick,Error); break;
    case EOGRulerDiplomacyAction::RequestSupport:
        Applied=!Decision->bAccept || Factions.SetSupport(Other,Own,true,Tick,Error); break;
    case EOGRulerDiplomacyAction::OfferSubordination:
        Applied=!Decision->bAccept || Factions.SetSubordination(Own,Other,true,Tick,Error); break;
    }
    if (!Applied) return Rollback();
    auto Receipt=MakeShared<FJsonObject>();
    if (Decision)
    {
        Receipt->SetBoolField(TEXT("accepted"),Decision->bAccept);
        Receipt->SetStringField(TEXT("reply"),Decision->Reply.ToString());
        Receipt->SetStringField(TEXT("summary"),Decision->Reply.ToString());
        Receipt->SetStringField(TEXT("response_rule"),Decision->RuleId.ToString());
        Receipt->SetStringField(TEXT("response_policy"),Policy->DefinitionId.ToString());
    }
    Receipt->SetStringField(TEXT("command"),ActionKey(Fresh.Action));
    Receipt->SetStringField(TEXT("acting_faction_id"),Own.ToString());
    Receipt->SetStringField(TEXT("foreign_faction_id"),Other.ToString());
    Receipt->SetStringField(TEXT("confirmation_token"),Fresh.Token);
    FOGWorldEvent Event; Event.EventId=Request; Event.EventType=EventType; Event.WorldTick=Tick;
    Event.PrimaryEntity=Fresh.RulerId; Event.RelatedEntities={Own,Other}; Event.bChronicleEligible=true;
    FOGEntityId ReportId;
    if (!FJsonSerializer::Serialize(Receipt,TJsonWriterFactory<>::Create(&Event.PayloadJson)) ||
        !Store.AppendWorldEvent(Event,Error) ||
        (Decision && !FOGReportService(Store).CreateReport(Fresh.RulerId,Event.EventId,TEXT("diplomacy"),0,
            Tick,Event.PayloadJson,ReportId,Error)) || !Store.CommitTransaction(Error)) return Rollback();
    if (Reply && Decision)
        *Reply=FString(Decision->bAccept?TEXT("Accepted / "):TEXT("Refused / "))+Decision->Reply.ToString();
    return true;
}
