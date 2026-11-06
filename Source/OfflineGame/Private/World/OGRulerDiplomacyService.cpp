#include "World/OGRulerDiplomacyService.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Persistence/OGWorldStore.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/OGFactionWarService.h"

namespace
{
const FName DiplomacyEvent(TEXT("faction.diplomacy_changed"));

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
    const auto& Link=Action==EOGRulerDiplomacyAction::EndAlliance?Links.Alliance:
        Action==EOGRulerDiplomacyAction::EndSubordination?Links.Subordination:Links.OutgoingSupport;
    const bool ActiveAfter=Action==EOGRulerDiplomacyAction::PledgeSupport;
    Out.RulerId=Ruler; Out.OwnFactionId=Own; Out.OtherFactionId=Other;
    Out.OwnName=Actor->Name; Out.OtherName=Target->Name; Out.Action=Action;
    if (ActiveAfter==Link.bActive)
    { Out.Reason=ActiveAfter?TEXT("Support is already pledged."):TEXT("This agreement is no longer active."); return true; }
    FString Quote=Ruler.ToString()+TEXT("|")+Own.ToString()+TEXT("|")+Other.ToString()+TEXT("|")+
        ActionKey(Action)+TEXT("|")+LexToString(Link.UpdatedWorldTick);
    Out.Token=FMD5::HashAnsiString(*Quote);
    Out.bCanApply=true;
    return true;
}

bool FOGRulerDiplomacyService::Confirm(const FOGRulerDiplomacyReview& Reviewed,
    const FOGEntityId& Request,int64 Tick,FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || !Request.IsValid() || Reviewed.Token.IsEmpty() || Tick<0)
    { Error=TEXT("A current diplomacy review is required."); return false; }
    if (!Store.BeginTransaction(Error)) return false;
    auto Rollback=[&](){FString Ignored; Store.RollbackTransaction(Ignored); return false;};
    FOGWorldEvent Previous; bool Found=false;
    if (!Store.TryReadWorldEvent(Request,Found,Previous,Error)) return Rollback();
    if (Found)
    {
        TSharedPtr<FJsonObject> Proof; FString Token;
        if (Previous.EventType!=DiplomacyEvent || Previous.PrimaryEntity!=Reviewed.RulerId ||
            !Previous.RelatedEntities.Contains(Reviewed.OwnFactionId) ||
            !Previous.RelatedEntities.Contains(Reviewed.OtherFactionId) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Previous.PayloadJson),Proof) ||
            !Proof || !Proof->TryGetStringField(TEXT("confirmation_token"),Token) || Token!=Reviewed.Token)
        { Error=TEXT("This request belongs to a different diplomacy command."); return Rollback(); }
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
    bool Changed=false;
    switch (Fresh.Action)
    {
    case EOGRulerDiplomacyAction::EndAlliance: Changed=Factions.BreakAlliance(Own,Other,Tick,Error); break;
    case EOGRulerDiplomacyAction::PledgeSupport: Changed=Factions.SetSupport(Own,Other,true,Tick,Error); break;
    case EOGRulerDiplomacyAction::WithdrawSupport: Changed=Factions.SetSupport(Own,Other,false,Tick,Error); break;
    case EOGRulerDiplomacyAction::EndSubordination: Changed=Factions.SetSubordination(Own,Other,false,Tick,Error); break;
    }
    if (!Changed) return Rollback();
    auto Receipt=MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("command"),ActionKey(Fresh.Action));
    Receipt->SetStringField(TEXT("acting_faction_id"),Own.ToString());
    Receipt->SetStringField(TEXT("foreign_faction_id"),Other.ToString());
    Receipt->SetStringField(TEXT("confirmation_token"),Fresh.Token);
    FOGWorldEvent Event; Event.EventId=Request; Event.EventType=DiplomacyEvent; Event.WorldTick=Tick;
    Event.PrimaryEntity=Fresh.RulerId; Event.RelatedEntities={Own,Other}; Event.bChronicleEligible=true;
    if (!FJsonSerializer::Serialize(Receipt,TJsonWriterFactory<>::Create(&Event.PayloadJson)) ||
        !Store.AppendWorldEvent(Event,Error) || !Store.CommitTransaction(Error)) return Rollback();
    return true;
}
