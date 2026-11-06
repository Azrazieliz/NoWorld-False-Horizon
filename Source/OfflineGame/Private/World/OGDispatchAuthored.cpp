#include "World/OGDispatchAuthored.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/OGSharedWorldStateService.h"

namespace
{
bool DispatchError(FString& Error,const TCHAR* Message)
{ Error=Message; return false; }

bool ReadDispatchObject(const FString& Text,TSharedPtr<FJsonObject>& Out)
{
    return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out) && Out.IsValid();
}

FString WriteDispatchObject(const TSharedRef<FJsonObject>& Object)
{
    FString Out;
    FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Out));
    return Out;
}

bool ReadDispatchId(const FJsonObject& Json,const TCHAR* Key,FOGEntityId& Out)
{
    FString Text;
    FGuid Guid;
    if (!Json.TryGetStringField(Key,Text) || !FGuid::Parse(Text,Guid) || !Guid.IsValid()) return false;
    Out=FOGEntityId(Guid);
    return true;
}

bool HasDispatchSkill(IOGWorldStore& Store,const FOGEntityId& Participant,const FOGContentId& Skill,
    FName State,bool& Qualified,FString& Error)
{
    Qualified=true;
    if (Skill.IsEmpty()) return true;
    TArray<FOGEntitySkillRecord> Skills;
    if (!Store.ListEntitySkills(Participant,Skills,Error)) return false;
    Qualified=Skills.ContainsByPredicate([&](const FOGEntitySkillRecord& Row)
    { return Row.OwnerEntityId==Participant && Row.SkillId==Skill && Row.CurrentState==State; });
    return true;
}

bool FindDispatchEvidence(IOGWorldStore& Store,const FOGDispatchRecord& Order,FName Type,bool bPrior,
    FOGEntityId& Evidence,FString& Error)
{
    Evidence=FOGEntityId();
    if (Type.IsNone()) return true;
    FOGWorldEventCursor Cursor;
    while (true)
    {
        TArray<FOGWorldEvent> Events;
        FOGWorldEventCursor Next;
        if (!Store.ListWorldEventsPage(Order.TargetEntityId,Type,false,64,Cursor,Events,Next,Error)) return false;
        for (const auto& Event:Events)
        {
            if (Event.WorldTick>Order.ResolveWorldTick) continue;
            if (!bPrior && Event.WorldTick<=Order.StartWorldTick) return true;
            if (Event.EventType==Type &&
                (Event.PrimaryEntity==Order.TargetEntityId || Event.RelatedEntities.Contains(Order.TargetEntityId)))
            {
                Evidence=Event.EventId;
                return true;
            }
        }
        if (!Next.EventId.IsValid()) return true;
        if (Cursor.EventId.IsValid() && (Next.WorldTick>Cursor.WorldTick ||
            (Next.WorldTick==Cursor.WorldTick && Next.EventId.ToString()>=Cursor.EventId.ToString())))
            return DispatchError(Error,TEXT("Dispatch evidence history did not advance."));
        Cursor=Next;
    }
}

bool EvaluateDispatchObjectives(IOGWorldStore& Store,const FOGDispatchRecord& Order,
    const TArray<FOGDispatchObjectiveRecord>& Objectives,bool bAccessible,bool& AllMet,
    TArray<TSharedPtr<FJsonValue>>& Outcomes,FString& Error)
{
    AllMet=true;
    for (const auto& Objective:Objectives)
    {
        TSharedPtr<FJsonObject> Spec;
        FString Name,EventType;
        bool bNeedsAccess=false,bPrior=false;
        if (Objective.TargetEntityId!=Order.TargetEntityId || !ReadDispatchObject(Objective.StateJson,Spec) ||
            !Spec->TryGetStringField(TEXT("name"),Name) ||
            !Spec->TryGetStringField(TEXT("target_event"),EventType) ||
            !Spec->TryGetBoolField(TEXT("requires_access"),bNeedsAccess) ||
            !Spec->TryGetBoolField(TEXT("allow_prior"),bPrior))
            return DispatchError(Error,TEXT("The dispatch has an unreadable authored objective."));
        FOGEntityId Evidence;
        const FName RequiredEvent(*EventType);
        if (!FindDispatchEvidence(Store,Order,RequiredEvent,bPrior,Evidence,Error)) return false;
        const bool bMet=(!bNeedsAccess || bAccessible) && (RequiredEvent.IsNone() || Evidence.IsValid());
        AllMet &= bMet;
        auto Result=MakeShared<FJsonObject>();
        Result->SetStringField(TEXT("objective_id"),Objective.ObjectiveId.Value);
        Result->SetStringField(TEXT("name"),Name);
        Result->SetBoolField(TEXT("mandatory"),Objective.bMandatory);
        Result->SetBoolField(TEXT("met"),bMet);
        if (Evidence.IsValid()) Result->SetStringField(TEXT("evidence_event_id"),Evidence.ToString());
        Outcomes.Add(MakeShared<FJsonValueObject>(Result));
    }
    return true;
}
}

bool OGDispatchAuthored::Validate(const UOGGatheringDispatchDefinition& D,FString& Error)
{
    const bool bSupported=D.DispatchType==EOGDispatchType::Exploration ||
        D.DispatchType==EOGDispatchType::Reconnaissance || D.DispatchType==EOGDispatchType::Espionage ||
        D.DispatchType==EOGDispatchType::Support;
    if (!bSupported || !D.TargetLocationId.IsValid() || D.Briefing.IsEmpty() ||
        D.RiskDescription.IsEmpty() || D.SuccessSummary.IsEmpty() || D.RiskBps<0 || D.RiskBps>10000 ||
        D.RiskToleranceBps<0 || D.RiskToleranceBps>10000 ||
        (!D.RequiredParticipantSkillId.IsEmpty() && !D.RequiredParticipantSkillId.IsValid()) ||
        (D.RequiredParticipantSkillId.IsEmpty()!=D.RequiredParticipantSkillState.IsNone()) ||
        (D.bTransferHoldingStock && (D.DispatchType!=EOGDispatchType::Support || D.TargetLocationId==D.LocationId ||
            !D.ResourceId.IsValid() || D.ResourceName.IsEmpty() || D.Amount<=0)))
        return DispatchError(Error,TEXT("Dispatch requires an original target, briefing, risk policy and valid success effects."));
    if (D.bRevealLocation && (D.RevealedLocationLevel<EOGLocationKnowledgeLevel::Rumored ||
        D.RevealedLocationLevel>EOGLocationKnowledgeLevel::Explored))
        return DispatchError(Error,TEXT("Dispatch has an invalid authored location knowledge level."));
    if (!D.KnowledgeFactKey.IsNone() && (D.KnowledgeSummary.IsEmpty() ||
        (D.KnowledgeBeliefState!=TEXT("confirmed") && D.KnowledgeBeliefState!=TEXT("estimated") &&
            D.KnowledgeBeliefState!=TEXT("rumor")) ||
        D.KnowledgeConfidenceBps<0 || D.KnowledgeConfidenceBps>10000 ||
        D.KnowledgeFactKey==FOGSharedWorldStateService::LocationKnowledgeFactKey()))
        return DispatchError(Error,TEXT("Dispatch knowledge needs its own fact key, authored summary, belief and confidence."));

    TSet<FOGContentId> Ids;
    bool bMandatory=false;
    for (const auto& Objective:D.Objectives)
    {
        if (!Objective.ObjectiveId.IsValid() || Objective.DisplayName.IsEmpty() || Ids.Contains(Objective.ObjectiveId) ||
            (!Objective.bRequireAccessibleTarget && Objective.TargetEventType.IsNone()))
            return DispatchError(Error,TEXT("Dispatch objectives need distinct IDs and a canonical target predicate."));
        Ids.Add(Objective.ObjectiveId);
        bMandatory |= Objective.bMandatory;
    }
    return bMandatory || DispatchError(Error,TEXT("Dispatch needs an authored mandatory objective."));
}

bool OGDispatchAuthored::Prepare(IOGWorldStore& Store,const UOGGatheringDispatchDefinition& D,
    FOGGatheringDispatchQuote& Quote,FString& Error)
{
    bool bRulerFound=false;
    FName RulerKind;
    FString RulerState;
    int64 RulerRevision=0;
    if (!Store.TryReadEntity(Quote.RulerId,bRulerFound,RulerKind,RulerState,RulerRevision,Error)) return false;
    if (!bRulerFound || RulerKind!=TEXT("ruler") || RulerState.Contains(TEXT("\"diagnosticFixture\"")))
        return DispatchError(Error,TEXT("Production dispatch requires a canonical Ruler."));
    Quote.TargetLocationId=D.TargetLocationId;
    Quote.Risk=D.RiskBps;
    Quote.RiskTolerance=D.RiskToleranceBps;
    Quote.Briefing=D.Briefing.ToString();
    Quote.RiskDescription=D.RiskDescription.ToString();
    if (D.bRequireKnownTarget)
    {
        bool bKnown=false;
        EOGLocationKnowledgeLevel Level=EOGLocationKnowledgeLevel::Rumored;
        if (!FOGSharedWorldStateService(Store).TryReadLocationKnowledge(Quote.RulerId,D.TargetLocationId,bKnown,Level,Error))
            return false;
        if (!bKnown || Level==EOGLocationKnowledgeLevel::Unknown)
        {
            Quote.bVisible=false;
            Quote.Name.Reset();
            Quote.Reason=TEXT("This dispatch target is unknown.");
            return true;
        }
    }
    FOGLocationRecord Target;
    bool bFound=false,bQualified=false;
    if (!Store.TryReadLocation(D.TargetLocationId,bFound,Target,Error) ||
        !HasDispatchSkill(Store,Quote.ParticipantId,D.RequiredParticipantSkillId,D.RequiredParticipantSkillState,bQualified,Error))
        return false;
    if (!bFound) Quote.Reason=TEXT("The original dispatch target is unavailable.");
    else if (!bQualified) Quote.Reason=TEXT("The assigned character lacks the authored capability.");
    if (!Quote.Reason.IsEmpty()) return true;

    auto Plan=MakeShared<FJsonObject>();
    Plan->SetStringField(TEXT("origin"),D.LocationId.ToString());
    Plan->SetStringField(TEXT("target"),D.TargetLocationId.ToString());
    Plan->SetNumberField(TEXT("type"),static_cast<int32>(D.DispatchType));
    Plan->SetStringField(TEXT("briefing"),Quote.Briefing);
    Plan->SetStringField(TEXT("risk_description"),Quote.RiskDescription);
    Plan->SetStringField(TEXT("source_reference"),D.OriginalSourceReference);
    Plan->SetStringField(TEXT("skill_id"),D.RequiredParticipantSkillId.Value);
    Plan->SetStringField(TEXT("skill_state"),D.RequiredParticipantSkillState.ToString());
    Plan->SetStringField(TEXT("summary"),D.SuccessSummary.ToString());
    Plan->SetBoolField(TEXT("transfer"),D.bTransferHoldingStock);
    Plan->SetStringField(TEXT("resource"),D.ResourceId.Value);
    Plan->SetStringField(TEXT("resource_name"),D.ResourceName.ToString());
    Plan->SetStringField(TEXT("amount"),LexToString(D.Amount));
    Plan->SetBoolField(TEXT("reveal_location"),D.bRevealLocation);
    Plan->SetNumberField(TEXT("knowledge_level"),static_cast<int32>(D.RevealedLocationLevel));
    Plan->SetStringField(TEXT("knowledge_fact"),D.KnowledgeFactKey.ToString());
    Plan->SetStringField(TEXT("knowledge_summary"),D.KnowledgeSummary.ToString());
    Plan->SetStringField(TEXT("knowledge_belief"),D.KnowledgeBeliefState.ToString());
    Plan->SetNumberField(TEXT("knowledge_confidence"),D.KnowledgeConfidenceBps);
    Quote.AuthoredPlanJson=WriteDispatchObject(Plan);

    for (const auto& Objective:D.Objectives)
    {
        FOGDispatchObjectiveRecord& Row=Quote.Objectives.AddDefaulted_GetRef();
        Row.ObjectiveId=Objective.ObjectiveId;
        Row.bMandatory=Objective.bMandatory;
        Row.TargetEntityId=D.TargetLocationId;
        auto Spec=MakeShared<FJsonObject>();
        Spec->SetStringField(TEXT("name"),Objective.DisplayName.ToString());
        Spec->SetStringField(TEXT("target_event"),Objective.TargetEventType.ToString());
        Spec->SetBoolField(TEXT("requires_access"),Objective.bRequireAccessibleTarget);
        Spec->SetBoolField(TEXT("allow_prior"),Objective.bAllowPriorEvidence);
        Row.StateJson=WriteDispatchObject(Spec);
        Quote.ObjectiveNames.Add(Objective.DisplayName.ToString()+
            (Objective.bMandatory?TEXT(" (required)"):TEXT(" (secondary)")));
    }
    FOGDispatchConstraintRecord& Constraint=Quote.Constraints.AddDefaulted_GetRef();
    Constraint.ConstraintId=D.DefinitionId;
    Constraint.StateJson=Quote.AuthoredPlanJson;
    return true;
}

bool OGDispatchAuthored::Evaluate(IOGWorldStore& Store,const FOGDispatchRecord& Order,
    const FOGDispatchResolutionContext& Policy,const FOGEntityId& ReceiptId,int64 Tick,bool bOffline,
    bool bHoldingAccess,bool bParticipantAvailable,FOGDispatchResolutionResult& Result,FString& Error)
{
    TSharedPtr<FJsonObject> Envelope,Plan;
    FString PlanText,DefinitionId,Skill,SkillState,Summary;
    FOGEntityId Origin,Target;
    int32 Type=0;
    if (!ReadDispatchObject(Policy.AbortPolicyJson,Envelope) ||
        !Envelope->TryGetStringField(TEXT("definition_id"),DefinitionId) ||
        !Envelope->TryGetStringField(TEXT("authored_plan"),PlanText) ||
        Policy.HardConstraints.Num()!=1 || Policy.HardConstraints[0].ConstraintId!=FOGContentId(DefinitionId) ||
        Policy.HardConstraints[0].StateJson!=PlanText || Policy.MandatoryObjectives.IsEmpty() ||
        !ReadDispatchObject(PlanText,Plan) || !ReadDispatchId(*Plan,TEXT("origin"),Origin) ||
        !ReadDispatchId(*Plan,TEXT("target"),Target) || Target!=Order.TargetEntityId ||
        !Plan->TryGetNumberField(TEXT("type"),Type) || Type!=static_cast<int32>(Order.Type) ||
        !Plan->TryGetStringField(TEXT("skill_id"),Skill) ||
        !Plan->TryGetStringField(TEXT("skill_state"),SkillState) ||
        !Plan->TryGetStringField(TEXT("summary"),Summary))
        return DispatchError(Error,TEXT("The authored dispatch no longer matches its saved policy."));

    FOGLocationRecord Location;
    bool bTarget=false,bQualified=false;
    if (!Store.TryReadLocation(Target,bTarget,Location,Error) ||
        !HasDispatchSkill(Store,Order.ParticipantEntityIds[0],FOGContentId(Skill),FName(*SkillState),bQualified,Error))
        return false;
    auto Outcome=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ObjectiveResults;
    bool bMandatory=false,bSecondary=true;
    if (!EvaluateDispatchObjectives(Store,Order,Policy.MandatoryObjectives,bTarget && Location.bPhysicallyAccessible,
        bMandatory,ObjectiveResults,Error)) return false;
    Result.bMandatoryObjectivesEvaluated=true;
    const bool bHardConstraints=bHoldingAccess && bTarget && bQualified;
    Result.bHardConstraintsEvaluated=true;
    const bool bWithinTolerance=Order.RiskBps<=Policy.RiskToleranceBps;
    Result.bSurvivalAbortPolicyEvaluated=true;

    bool bSuccess=bMandatory && bHardConstraints && bParticipantAvailable && bWithinTolerance;
    const TCHAR* Failure=!bParticipantAvailable?TEXT("The assigned character is no longer available."):
        !bHardConstraints?TEXT("A target, capability or holding requirement changed."):
        !bWithinTolerance?TEXT("The order returned under its authored risk policy."):
        TEXT("The required objectives were not met by the deadline.");
    const bool bSecondaryEvaluated=bSuccess;
    if (bSecondaryEvaluated && !EvaluateDispatchObjectives(Store,Order,Policy.SecondaryObjectives,
        bTarget && Location.bPhysicallyAccessible,bSecondary,ObjectiveResults,Error)) return false;

    bool bTransfer=false,bReveal=false;
    FString Resource,ResourceName,AmountText,FactKey,KnowledgeSummary,Belief;
    int32 KnowledgeLevel=0,Confidence=0;
    if (!Plan->TryGetBoolField(TEXT("transfer"),bTransfer) ||
        !Plan->TryGetBoolField(TEXT("reveal_location"),bReveal) ||
        !Plan->TryGetStringField(TEXT("resource"),Resource) ||
        !Plan->TryGetStringField(TEXT("resource_name"),ResourceName) ||
        !Plan->TryGetStringField(TEXT("amount"),AmountText) ||
        !Plan->TryGetNumberField(TEXT("knowledge_level"),KnowledgeLevel) ||
        !Plan->TryGetStringField(TEXT("knowledge_fact"),FactKey) ||
        !Plan->TryGetStringField(TEXT("knowledge_summary"),KnowledgeSummary) ||
        !Plan->TryGetStringField(TEXT("knowledge_belief"),Belief) ||
        !Plan->TryGetNumberField(TEXT("knowledge_confidence"),Confidence))
        return DispatchError(Error,TEXT("Dispatch success effects are unreadable."));

    int64 Delivered=0;
    if (bSuccess && bTransfer)
    {
        int64 Amount=0,Stock=0,Destination=0;
        bool bSourceFound=false,bDestinationFound=false;
        const FOGContentId ResourceId(Resource);
        if (Order.Type!=EOGDispatchType::Support || Origin==Target || !ResourceId.IsValid() ||
            !LexTryParseString(Amount,*AmountText) || AmountText!=LexToString(Amount) || Amount<=0)
            return DispatchError(Error,TEXT("Invalid saved support transfer."));
        if (!Store.TryReadResourceBalance(Origin,ResourceId,bSourceFound,Stock,Error) ||
            !Store.TryReadResourceBalance(Target,ResourceId,bDestinationFound,Destination,Error)) return false;
        if (!bDestinationFound) Destination=0;
        if (!bSourceFound || Stock<Amount || Destination<0 || Destination>MAX_int64-Amount)
        {
            bSuccess=false;
            Failure=TEXT("The source stock or receiving capacity changed before delivery.");
        }
        else
        {
            if (!Store.SetResourceBalance(Origin,ResourceId,Stock-Amount,Error) ||
                !Store.SetResourceBalance(Target,ResourceId,Destination+Amount,Error)) return false;
            Delivered=Amount;
            Summary+=TEXT(" ")+FText::AsNumber(Delivered).ToString()+TEXT(" ")+ResourceName+TEXT(" delivered.");
            Outcome->SetStringField(TEXT("resource_id"),Resource);
            Outcome->SetStringField(TEXT("source_remaining_exact"),LexToString(Stock-Amount));
            Outcome->SetStringField(TEXT("target_balance_exact"),LexToString(Destination+Amount));
        }
    }
    if (bSuccess && bReveal)
    {
        if (KnowledgeLevel<static_cast<int32>(EOGLocationKnowledgeLevel::Rumored) ||
            KnowledgeLevel>static_cast<int32>(EOGLocationKnowledgeLevel::Explored))
            return DispatchError(Error,TEXT("Invalid saved location knowledge level."));
        FOGSharedWorldStateService Knowledge(Store);
        auto Level=static_cast<EOGLocationKnowledgeLevel>(KnowledgeLevel);
        bool bKnown=false;
        if (!Knowledge.RecordLocationDiscovery(Order.OwnerEntityId,Target,Level,Tick,Error) ||
            !Knowledge.TryReadLocationKnowledge(Order.OwnerEntityId,Target,bKnown,Level,Error)) return false;
        if (bKnown) Outcome->SetNumberField(TEXT("location_knowledge_level"),static_cast<int32>(Level));
    }
    const FName KnowledgeKey(*FactKey);
    if (bSuccess && !KnowledgeKey.IsNone())
    {
        bool bFound=false;
        FOGKnowledgeFactRecord Fact;
        if (!Store.TryReadKnowledgeFact(Order.OwnerEntityId,KnowledgeKey,Target,bFound,Fact,Error)) return false;
        if (!bFound)
        {
            Fact=FOGKnowledgeFactRecord();
            Fact.OwnerEntityId=Order.OwnerEntityId;
            Fact.SubjectEntityId=Target;
            Fact.FactKey=KnowledgeKey;
            auto Value=MakeShared<FJsonObject>();
            Value->SetStringField(TEXT("summary"),KnowledgeSummary);
            Fact.ValueJson=WriteDispatchObject(Value);
            Fact.BeliefState=FName(*Belief);
            Fact.ConfidenceBps=Confidence;
            Fact.LearnedWorldTick=Tick;
            Fact.UpdatedWorldTick=Tick;
            Fact.SourceEntityId=Order.ParticipantEntityIds[0];
            Fact.SourceEventId=ReceiptId;
            Fact.bHasEvidenceWorldTick=true;
            Fact.EvidenceWorldTick=Tick;
            if (!Store.UpsertKnowledgeFact(Fact,Error)) return false;
            Outcome->SetStringField(TEXT("recorded_fact_key"),FactKey);
            Summary+=TEXT(" ")+KnowledgeSummary;
        }
    }

    Result.bSucceeded=bSuccess;
    Result.OutcomeState=bSuccess?(bSecondary?TEXT("completed"):TEXT("completed_with_secondary_unmet")):TEXT("returned_without_completion");
    Outcome->SetStringField(TEXT("definition_id"),DefinitionId);
    Outcome->SetStringField(TEXT("outcome"),Result.OutcomeState.ToString());
    Outcome->SetNumberField(TEXT("dispatch_type"),static_cast<int32>(Order.Type));
    Outcome->SetStringField(TEXT("summary"),bSuccess?Summary:FString(Failure));
    Outcome->SetStringField(TEXT("delivered_exact"),LexToString(Delivered));
    Outcome->SetBoolField(TEXT("offline"),bOffline);
    Outcome->SetBoolField(TEXT("hard_constraints_met"),bHardConstraints);
    Outcome->SetBoolField(TEXT("participant_available"),bParticipantAvailable);
    Outcome->SetBoolField(TEXT("within_risk_policy"),bWithinTolerance);
    Outcome->SetBoolField(TEXT("secondary_evaluated"),bSecondaryEvaluated);
    Outcome->SetArrayField(TEXT("objectives"),ObjectiveResults);
    Result.ResultJson=WriteDispatchObject(Outcome);
    return true;
}
