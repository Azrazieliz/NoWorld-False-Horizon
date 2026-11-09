#include "World/OGFactionDiplomacyPolicy.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Persistence/OGWorldStore.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
void AddPart(FString& Out,const FString& Part)
{ Out+=LexToString(Part.Len())+TEXT(":")+Part; }

FOGEntityId SubjectId(EOGFactionNegotiator Subject,const FOGFactionRecord& Requester,const FOGFactionRecord& Responder)
{
    switch (Subject)
    {
    case EOGFactionNegotiator::RequestingFaction: return Requester.FactionId;
    case EOGFactionNegotiator::RespondingFaction: return Responder.FactionId;
    case EOGFactionNegotiator::RequestingLeader: return Requester.LeaderRulerId;
    case EOGFactionNegotiator::RespondingLeader: return Responder.LeaderRulerId;
    }
    return {};
}
}

bool UOGFactionDiplomacyPolicy::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("faction_diplomacy") || !RespondingFactionId.IsValid() ||
        OriginalSourceReference.TrimStartAndEnd().IsEmpty() || Rules.IsEmpty())
    { Error=TEXT("Diplomacy policy requires an original source, a real faction and authored responses."); return false; }
    TSet<FName> Seen;
    TSet<EOGFactionProposal> Terminal,Offered;
    for (const auto& Rule:Rules)
    {
        if (Rule.RuleId.IsNone() || Seen.Contains(Rule.RuleId) || Rule.Reply.IsEmpty() ||
            Rule.ExactRequester==RespondingFactionId || Terminal.Contains(Rule.Proposal) ||
            static_cast<uint8>(Rule.Proposal)>static_cast<uint8>(EOGFactionProposal::Subordination))
        { Error=TEXT("Diplomacy responses need distinct rules in reachable authored order."); return false; }
        Seen.Add(Rule.RuleId); Offered.Add(Rule.Proposal);
        for (const auto& State:Rule.State)
            if (State.Field.IsNone() || static_cast<uint8>(State.Subject)>static_cast<uint8>(EOGFactionNegotiator::RespondingLeader))
            { Error=TEXT("Diplomacy response requires a valid exact faction or leader state field."); return false; }
        for (const auto& Link:Rule.Links)
            if (static_cast<uint8>(Link.Type)>static_cast<uint8>(EOGFactionLinkType::Subordination))
            { Error=TEXT("Diplomacy response references an invalid relationship."); return false; }
        if (!Rule.ExactRequester.IsValid() && Rule.State.IsEmpty() && Rule.Links.IsEmpty())
            Terminal.Add(Rule.Proposal);
    }
    for (const auto Proposal:Offered)
        if (!Terminal.Contains(Proposal))
        { Error=TEXT("Each offered proposal requires an explicit final response from its author."); return false; }
    return true;
}

bool UOGFactionDiplomacyPolicy::HasProposal(EOGFactionProposal Proposal) const
{ return Rules.ContainsByPredicate([&](const auto& Rule){return Rule.Proposal==Proposal;}); }

bool UOGFactionDiplomacyPolicy::Decide(IOGWorldStore& Store,EOGFactionProposal Proposal,
    const FOGFactionRecord& Requester,const FOGFactionRecord& Responder,
    const FOGFactionProposalRule*& Out,FString& Error) const
{
    Out=nullptr;
    if (!Requester.FactionId.IsValid() || Responder.FactionId!=RespondingFactionId ||
        Requester.FactionId==Responder.FactionId || !ValidateDefinition(Error))
    { if (Error.IsEmpty()) Error=TEXT("The negotiation no longer has its actual factions and policy."); return false; }

    for (const auto& Rule:Rules)
    {
        if (Rule.Proposal!=Proposal || (Rule.ExactRequester.IsValid() && Rule.ExactRequester!=Requester.FactionId)) continue;
        bool Matches=true;
        for (const auto& State:Rule.State)
        {
            const auto Entity=SubjectId(State.Subject,Requester,Responder);
            if (!Entity.IsValid()) { Matches=false; break; }
            bool Found=false; FName Kind; FString Json,Value; int64 Revision=0;
            if (!Store.TryReadEntity(Entity,Found,Kind,Json,Revision,Error)) return false;
            TSharedPtr<FJsonObject> Object;
            if (!Found || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Object) || !Object ||
                !Object->TryGetStringField(State.Field.ToString(),Value) || Value!=State.ExactValue)
            { Matches=false; break; }
        }
        if (!Matches) continue;
        for (const auto& Link:Rule.Links)
        {
            auto From=Link.bFromRespondingFaction?Responder.FactionId:Requester.FactionId;
            auto To=Link.bFromRespondingFaction?Requester.FactionId:Responder.FactionId;
            if (Link.Type==EOGFactionLinkType::Alliance && From.ToString().Compare(To.ToString(),ESearchCase::CaseSensitive)>0)
                Swap(From,To);
            bool Found=false; FOGFactionLinkRecord Row;
            if (!Store.TryReadFactionLink(From,To,Link.Type,Found,Row,Error)) return false;
            if ((Found && Row.bActive)!=Link.bMustBeActive) { Matches=false; break; }
        }
        if (Matches) { Out=&Rule; return true; }
    }
    Error=TEXT("The installed faction has no response for this proposal.");
    return false;
}

FString UOGFactionDiplomacyPolicy::Fingerprint() const
{
    FString Key;
    for (const auto& Value:{DefinitionId.ToString(),RespondingFactionId.ToString(),OriginalSourceReference})
        AddPart(Key,Value);
    for (const auto& Rule:Rules)
    {
        for (const auto& Value:{Rule.RuleId.ToString(),LexToString(static_cast<uint8>(Rule.Proposal)),
            Rule.ExactRequester.ToString(),FString(Rule.bAccept?TEXT("accepted"):TEXT("refused")),Rule.Reply.ToString()})
            AddPart(Key,Value);
        AddPart(Key,LexToString(Rule.State.Num()));
        for (const auto& State:Rule.State)
        {
            AddPart(Key,LexToString(static_cast<uint8>(State.Subject)));
            AddPart(Key,State.Field.ToString()); AddPart(Key,State.ExactValue);
        }
        AddPart(Key,LexToString(Rule.Links.Num()));
        for (const auto& Link:Rule.Links)
        {
            AddPart(Key,LexToString(static_cast<uint8>(Link.Type)));
            AddPart(Key,Link.bFromRespondingFaction?TEXT("responding"):TEXT("requesting"));
            AddPart(Key,Link.bMustBeActive?TEXT("active"):TEXT("inactive"));
        }
    }
    return FMD5::HashAnsiString(*Key);
}

const UOGFactionDiplomacyPolicy* UOGFactionDiplomacyPolicy::Find(const FOGEntityId& Responder,
    EOGFactionProposal Proposal,FString& Error)
{
    Error.Reset();
    auto* Library=UOGDefinitionLibrary::Get();
    TArray<UDataAsset*> Assets;
    if (Library) Library->List(TEXT("faction_diplomacy"),StaticClass(),Assets);
    const UOGFactionDiplomacyPolicy* Found=nullptr;
    for (const auto* Asset:Assets)
    {
        const auto* Policy=CastChecked<UOGFactionDiplomacyPolicy>(Asset);
        if (Policy->RespondingFactionId!=Responder || !Policy->HasProposal(Proposal)) continue;
        if (Found || !Policy->ValidateDefinition(Error))
        {
            if (Error.IsEmpty()) Error=TEXT("The foreign faction has overlapping installed responses for this proposal.");
            return nullptr;
        }
        Found=Policy;
    }
    return Found;
}
