#include "Objects/OGObjectActionDefinition.h"
#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool JsonObject(const FString& Text)
{ TSharedPtr<FJsonObject> J; return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),J) && J.IsValid(); }
void Part(FString& S,const FString& V) { S+=LexToString(V.Len())+TEXT(":")+V; }
bool OptionalId(const FOGContentId& Id) { return Id.IsEmpty() || Id.IsValid(); }
}
bool UOGObjectDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("object") || DefinitionId.Value.StartsWith(TEXT("diagnostic:")) ||
        OriginalSourceReference.TrimStartAndEnd().IsEmpty() || DisplayName.IsEmpty() || !OptionalId(InitialRankId) || !OptionalId(InitialQualityId) ||
        !JsonObject(InitialDurabilityJson) || !JsonObject(InitialEvolutionJson))
    { Error=TEXT("Object definitions require sourced identity, valid initial rank/quality and structured initial state."); return false; }
    return true;
}
bool UOGObjectActionDefinition::UsesCounterparty() const
{
    if (Kind==EOGObjectActionKind::Exchange || !CounterpartyEntityKind.IsNone()) return true;
    for (const auto& I:Inputs) if (I.From==EOGObjectActionParty::Counterparty || I.Disposition==EOGObjectInputDisposition::Transfer) return true;
    for (const auto& O:Outputs) if (O.To==EOGObjectActionParty::Counterparty) return true;
    for (const auto& R:Resources) if (R.Account==EOGObjectActionParty::Counterparty) return true;
    return false;
}
bool UOGObjectActionDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("object_action") || DefinitionId.Value.StartsWith(TEXT("diagnostic:")) ||
        OriginalSourceReference.TrimStartAndEnd().IsEmpty() || DisplayName.IsEmpty() ||
        static_cast<uint8>(Kind)>static_cast<uint8>(EOGObjectActionKind::Exchange) ||
        Inputs.Num()>4096 || Outputs.Num()>4096 || Resources.Num()>4096 ||
        (Inputs.IsEmpty() && Outputs.IsEmpty() && Resources.IsEmpty() && Kind!=EOGObjectActionKind::Repair))
    { Error=TEXT("Object action requires a sourced definition and actual executable operations."); return false; }
    bool Cost=false; int64 OutputCount=0;
    for (const auto& I:Inputs)
    {
        if (!I.ObjectId.IsValid() || !OptionalId(I.RankId) || !OptionalId(I.QualityId) || I.InstanceCount<1 || I.InstanceCount>10000 ||
            static_cast<uint8>(I.From)>static_cast<uint8>(EOGObjectActionParty::Counterparty) ||
            static_cast<uint8>(I.Disposition)>static_cast<uint8>(EOGObjectInputDisposition::Transfer) ||
            (Kind==EOGObjectActionKind::Exchange && I.Disposition==EOGObjectInputDisposition::Consume))
        { Error=TEXT("Input rows require exact object/rank/quality, valid counts and a permitted disposition."); return false; }
        Cost|=I.Disposition!=EOGObjectInputDisposition::Retain;
    }
    for (const auto& O:Outputs)
    {
        if (!O.ObjectId.IsValid() || O.InstanceCount<1 || O.InstanceCount>10000 ||
            static_cast<uint8>(O.To)>static_cast<uint8>(EOGObjectActionParty::Counterparty))
        { Error=TEXT("Object output is invalid."); return false; }
        OutputCount+=O.InstanceCount;
    }
    if (OutputCount>10000 || (Kind==EOGObjectActionKind::Exchange && !Outputs.IsEmpty()))
    { Error=TEXT("An exchange transfers real stock; generated output counts must remain bounded."); return false; }
    TSet<FString> Accounts; TMap<FOGContentId,int64> ExchangeDebits,ExchangeCredits;
    for (const auto& R:Resources)
    {
        const FString Key=FString::FromInt(static_cast<int32>(R.Account))+TEXT("|")+R.ResourceId.ToString();
        if (!R.ResourceId.IsValid() || R.DisplayName.IsEmpty() || R.Amount==0 || R.Amount==MIN_int64 || Accounts.Contains(Key) ||
            static_cast<uint8>(R.Account)>static_cast<uint8>(EOGObjectActionParty::Counterparty))
        { Error=TEXT("Resource rows require distinct accounts, real IDs and safe explicit nonzero amounts."); return false; }
        Accounts.Add(Key); Cost|=R.Amount<0;
        if (Kind==EOGObjectActionKind::Exchange)
            (R.Amount<0?ExchangeDebits:ExchangeCredits).FindOrAdd(R.ResourceId)=R.Amount<0?-R.Amount:R.Amount;
    }
    if (Kind==EOGObjectActionKind::Exchange)
    {
        if (ExchangeDebits.Num()!=ExchangeCredits.Num())
        { Error=TEXT("Exchange wallets must conserve each currency/material."); return false; }
        for (const auto& R:ExchangeDebits)
            if (ExchangeCredits.FindRef(R.Key)!=R.Value)
            { Error=TEXT("Exchange debits and credits must balance exactly."); return false; }
    }
    if (Cost==bExplicitNoCost || (UsesCounterparty() && CounterpartyEntityKind.IsNone()) ||
        (Kind==EOGObjectActionKind::Repair && (!SubjectObjectId.IsValid() || RepairToConditionBps<1 || RepairToConditionBps>10000)) ||
        (Kind!=EOGObjectActionKind::Repair && (!SubjectObjectId.IsEmpty() || RepairToConditionBps!=0)))
    { Error=TEXT("Declare costs or explicit no-cost, an exact counterparty kind, and repair-only subject/condition rules."); return false; }
    return true;
}
FString UOGObjectActionDefinition::Fingerprint() const
{
    FString S; for (const auto& V:{DefinitionId.ToString(),OriginalSourceReference,DisplayName.ToString(),Description.ToString(),
        FString::FromInt(static_cast<int32>(Kind)),bExplicitNoCost?FString(TEXT("free")):FString(TEXT("cost")),
        SubjectObjectId.ToString(),FString::FromInt(RepairToConditionBps),CounterpartyEntityKind.ToString()}) Part(S,V);
    for (const auto& I:Inputs)
        for (const auto& V:{I.ObjectId.ToString(),I.RankId.ToString(),I.QualityId.ToString(),FString::FromInt(I.InstanceCount),
            FString::FromInt(static_cast<int32>(I.From)),FString::FromInt(static_cast<int32>(I.Disposition))}) Part(S,V);
    Part(S,TEXT("outputs"));
    for (const auto& O:Outputs) for (const auto& V:{O.ObjectId.ToString(),FString::FromInt(O.InstanceCount),FString::FromInt(static_cast<int32>(O.To))}) Part(S,V);
    Part(S,TEXT("resources"));
    for (const auto& R:Resources) for (const auto& V:{R.ResourceId.ToString(),FString::FromInt(static_cast<int32>(R.Account)),LexToString(R.Amount),R.DisplayName.ToString()}) Part(S,V);
    return FMD5::HashAnsiString(*S);
}
void UOGObjectActionDefinition::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{
    Super::CollectRequiredDefinitions(Out);
    auto Add=[&](const FOGContentId& Id){FOGDefinitionReference R; R.System=TEXT("object"); R.Id=Id; Out.Add(R);};
    for (const auto& I:Inputs) Add(I.ObjectId);
    for (const auto& O:Outputs) Add(O.ObjectId);
    if (SubjectObjectId.IsValid()) Add(SubjectObjectId);
}
bool UOGObjectActionDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    TArray<FOGDefinitionReference> References; CollectRequiredDefinitions(References);
    for (const auto& Ref:References) if (Ref.System==TEXT("object") && !Cast<UOGObjectDefinition>(Resolve(Ref.System,Ref.Id)))
    { Error=TEXT("Object action requires the shared native object definition class."); return false; }
    return true;
}
