#include "Narrative/OGNarrativeService.h"
#include "Narrative/OGNarrativeJson.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
using namespace OGNarrative;
bool Child(const FObject& O, const TCHAR* Key, FObject& Out, const FString& At, FString& Error)
{
    const auto* V = O->Values.Find(Key);
    return V && Object(*V, Out, At + TEXT(".") + Key, Error);
}
bool ReadSubject(const FObject& O, FOGNarrativeSubjectRule& Out, const FString& At, FString& Error)
{
    if (!Fields(O, {TEXT("entity_kind"),TEXT("actor_id"),TEXT("identity_id"),TEXT("lifecycle_state"),TEXT("state_equals")}, At, Error) ||
        !Name(O,TEXT("entity_kind"),Out.EntityKind,At,Error) ||
        !Id(O,TEXT("actor_id"),Out.ActorId,At,Error,false) || !Id(O,TEXT("identity_id"),Out.IdentityId,At,Error,false) ||
        !Name(O,TEXT("lifecycle_state"),Out.LifecycleState,At,Error,false)) return false;
    if (O->HasField(TEXT("state_equals")))
    {
        FObject M; if (!Child(O,TEXT("state_equals"),M,At,Error) || !StringMap(M,Out.StateEquals,At,Error)) return false;
    }
    return true;
}
bool ReadEvent(const FObject& O, FOGNarrativeEventRule& Out, const FString& At, FString& Error)
{
    FString Subject;
    if (!Name(O,TEXT("event_type"),Out.EventType,At,Error) || !String(O,TEXT("subject"),Subject,At,Error)) return false;
    if (!Subject.Equals(TEXT("ruler"),ESearchCase::CaseSensitive) && !Subject.Equals(TEXT("subject"),ESearchCase::CaseSensitive))
        return Fail(Error,At,TEXT("subject must be ruler or subject"));
    Out.bSubjectIsRuler = Subject.Equals(TEXT("ruler"),ESearchCase::CaseSensitive); return true;
}
bool ReadBranches(const FObject& O, const TCHAR* Key, TArray<FOGNarrativeBranch>& Out, const FString& At, FString& Error)
{
    const FArray* A = nullptr; if (!Array(O,Key,A,At,Error,false,64)) return false;
    for (const auto& V : *A)
    {
        FObject B; FOGNarrativeBranch R;
        if (!Object(V,B,At,Error) || !Fields(B,{TEXT("branch_id"),TEXT("value_id")},At,Error) ||
            !Id(B,TEXT("branch_id"),R.BranchId,At,Error) || !Id(B,TEXT("value_id"),R.ValueId,At,Error)) return false;
        Out.Add(MoveTemp(R));
    }
    return true;
}
bool ReadConditions(const FObject& O, FOGNarrativeConditions& Out, const FString& At, FString& Error)
{
    if (!Fields(O,{TEXT("completed_quests"),TEXT("required_events"),TEXT("excluded_events"),TEXT("required_branches")},At,Error)) return false;
    const FArray* A = nullptr; if (!Array(O,TEXT("completed_quests"),A,At,Error,false,64)) return false;
    for (const auto& V : *A)
    {
        FString S; if (!V.IsValid() || V->Type != EJson::String || !V->TryGetString(S) || !FOGContentId(S).IsValid())
            return Fail(Error,At,TEXT("completed_quests requires content ID strings"));
        Out.CompletedQuests.Emplace(S);
    }
    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (!Array(O,Index == 0 ? TEXT("required_events") : TEXT("excluded_events"),A,At,Error,false,64)) return false;
        for (const auto& V : *A)
        {
            FObject E; FOGNarrativeEventRule R;
            if (!Object(V,E,At,Error) || !Fields(E,{TEXT("event_type"),TEXT("subject")},At,Error) || !ReadEvent(E,R,At,Error)) return false;
            (Index == 0 ? Out.RequiredEvents : Out.ExcludedEvents).Add(MoveTemp(R));
        }
    }
    return ReadBranches(O,TEXT("required_branches"),Out.RequiredBranches,At,Error);
}
bool ReadEffects(const FObject& O, FOGNarrativeEffects& Out, const FString& At, FString& Error)
{
    if (!Fields(O,{TEXT("costs"),TEXT("rewards"),TEXT("branch_locks")},At,Error)) return false;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FArray* A = nullptr;
        if (!Array(O,Index == 0 ? TEXT("costs") : TEXT("rewards"),A,At,Error,false,64)) return false;
        for (const auto& V : *A)
        {
            FObject E; FOGNarrativeResource R; FString Amount;
            if (!Object(V,E,At,Error) || !Fields(E,{TEXT("resource_id"),TEXT("display_name"),TEXT("amount")},At,Error) ||
                !Id(E,TEXT("resource_id"),R.ResourceId,At,Error) || !String(E,TEXT("display_name"),R.DisplayName,At,Error) ||
                !String(E,TEXT("amount"),Amount,At,Error)) return false;
            if (!Decimal(Amount,R.Amount,true)) return Fail(Error,At,TEXT("amount must be a positive exact int64 decimal string"));
            (Index == 0 ? Out.Costs : Out.Rewards).Add(MoveTemp(R));
        }
    }
    return ReadBranches(O,TEXT("branch_locks"),Out.BranchLocks,At,Error);
}
template<typename T, typename Reader>
bool Optional(const FObject& O, const TCHAR* Key, T& Out, Reader Read, const FString& At, FString& Error)
{
    if (!O->HasField(Key)) return true;
    FObject C; return Child(O,Key,C,At,Error) && Read(C,Out,At + TEXT(".") + Key,Error);
}
bool ValidSubject(const FOGNarrativeSubjectRule& R, bool Required, const FString& At, FString& Error)
{
    const bool HasActor = !R.ActorId.IsEmpty(), HasIdentity = !R.IdentityId.IsEmpty();
    if (!Required && !HasActor && !HasIdentity && R.EntityKind.IsNone() && R.LifecycleState.IsNone() && R.StateEquals.IsEmpty()) return true;
    if (!Token(R.EntityKind.ToString()) || HasActor == HasIdentity ||
        (HasActor && (!R.ActorId.IsValid() || R.ActorId.Value.StartsWith(TEXT("diagnostic:")))) ||
        (HasIdentity && (!R.IdentityId.IsValid() || R.IdentityId.Value.StartsWith(TEXT("diagnostic:")))) || R.StateEquals.Num() > 64)
        return Fail(Error,At,TEXT("subject requires an exact entity_kind and exactly one valid actor_id or identity_id"));
    if (HasActor && (!R.LifecycleState.IsNone() ||
        (!R.StateEquals.Contains(TEXT("life_state")) && !R.StateEquals.Contains(TEXT("lifecycle_state")))))
        return Fail(Error,At,TEXT("actor subjects require canonical life_state/lifecycle_state in state_equals"));
    if (HasIdentity && !Token(R.LifecycleState.ToString()))
        return Fail(Error,At,TEXT("manifestation subjects require an exact lifecycle_state"));
    for (const auto& P : R.StateEquals)
        if (!Token(P.Key) || P.Value.IsEmpty()) return Fail(Error,At,TEXT("invalid state_equals string predicate"));
    return true;
}
bool ValidBranches(const TArray<FOGNarrativeBranch>& Values, const FString& At, FString& Error)
{
    if (Values.Num() > 64) return Fail(Error,At,TEXT("too many branches"));
    TSet<FOGContentId> Seen;
    for (const auto& V : Values)
    {
        if (!V.BranchId.IsValid() || !V.ValueId.IsValid() || Seen.Contains(V.BranchId)) return Fail(Error,At,TEXT("invalid or duplicate branch ID"));
        Seen.Add(V.BranchId);
    }
    return true;
}
bool ValidConditions(const FOGNarrativeConditions& C, bool HasSubject, const FString& At, FString& Error)
{
    if (C.CompletedQuests.Num() > 64 || C.RequiredEvents.Num() > 64 || C.ExcludedEvents.Num() > 64 ||
        !ValidBranches(C.RequiredBranches,At,Error)) return Fail(Error,At,TEXT("invalid or oversized conditions"));
    TSet<FOGContentId> Quests;
    for (const auto& Q : C.CompletedQuests)
    {
        if (!Q.IsValid() || Quests.Contains(Q)) return Fail(Error,At,TEXT("invalid or duplicate completed quest"));
        Quests.Add(Q);
    }
    TSet<FString> Required;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        TSet<FString> Seen;
        for (const auto& E : Index == 0 ? C.RequiredEvents : C.ExcludedEvents)
        {
            const FString Key = FString(E.bSubjectIsRuler ? TEXT("ruler:") : TEXT("subject:")) + E.EventType.ToString().ToLower();
            if (!Token(E.EventType.ToString()) || (!E.bSubjectIsRuler && !HasSubject) || Seen.Contains(Key) || (Index == 1 && Required.Contains(Key)))
                return Fail(Error,At,TEXT("invalid, duplicate, unbound or contradictory event condition"));
            Seen.Add(Key);
        }
        if (Index == 0) Required = MoveTemp(Seen);
    }
    return true;
}
bool ValidEffects(const FOGNarrativeEffects& E, const FOGNarrativeConditions& C, const FString& At, FString& Error)
{
    if (!ValidBranches(E.BranchLocks,At,Error)) return false;
    for (const auto* List : {&E.Costs,&E.Rewards})
    {
        if (List->Num() > 64) return Fail(Error,At,TEXT("too many resources"));
        TSet<FOGContentId> Seen;
        for (const auto& R : *List)
        {
            if (!R.ResourceId.IsValid() || R.DisplayName.TrimStartAndEnd().IsEmpty() || R.Amount <= 0 || Seen.Contains(R.ResourceId)) return Fail(Error,At,TEXT("invalid, unlabeled, nonpositive or duplicate canonical resource"));
            Seen.Add(R.ResourceId);
        }
    }
    for (const auto& Cost : E.Costs)
        for (const auto& Reward : E.Rewards)
            if (Cost.ResourceId == Reward.ResourceId && !Cost.DisplayName.Equals(Reward.DisplayName,ESearchCase::CaseSensitive))
                return Fail(Error,At,TEXT("cost/reward labels for the same resource must agree"));
    for (const auto& Lock : E.BranchLocks)
        for (const auto& Need : C.RequiredBranches)
            if (Lock.BranchId == Need.BranchId && Lock.ValueId != Need.ValueId) return Fail(Error,At,TEXT("effects contradict required immutable branch"));
    return true;
}
}

bool FOGNarrativeCatalog::LoadInstalled(FOGNarrativeCatalog& Out, FString& Error)
{
    Out = FOGNarrativeCatalog(); Error.Reset(); FString Json;
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(),TEXT("NoWorld/Narrative/NarrativeCatalog.json"));
    if (!FFileHelper::LoadFileToString(Json,*Path)) return OGNarrative::Fail(Error,Path,TEXT("cannot read approved catalog; restore file and stage NoWorld/Narrative as UFS"));
    if (!Parse(Json,Out,Error)) { Error = Path + TEXT(": ") + Error; return false; }
    return true;
}
bool FOGNarrativeCatalog::Parse(const FString& Json, FOGNarrativeCatalog& Out, FString& Error)
{
    using namespace OGNarrative;
    Out = FOGNarrativeCatalog(); Error.Reset(); FObject Root; FString Schema;
    if (!ReadObject(Json,Root,Error) || !Fields(Root,{TEXT("schema"),TEXT("quests"),TEXT("dialogues")},TEXT("catalog"),Error) ||
        !String(Root,TEXT("schema"),Schema,TEXT("catalog"),Error)) return false;
    if (!Schema.Equals(TEXT("noworld.narrative.v1"),ESearchCase::CaseSensitive)) return Fail(Error,TEXT("schema"),TEXT("expected noworld.narrative.v1"));
    FOGNarrativeCatalog Parsed; const FArray* A = nullptr;
    if (!Array(Root,TEXT("quests"),A,TEXT("catalog"),Error,true)) return false;
    for (const auto& V : *A)
    {
        FObject O; FOGNarrativeQuestDefinition Q;
        const FString At = TEXT("quest");
        if (!Object(V,O,At,Error) || !Fields(O,{TEXT("quest_id"),TEXT("display_name"),TEXT("description"),TEXT("subject"),TEXT("allow_remote_completion"),TEXT("conditions"),TEXT("accept_effects"),TEXT("completion_effects"),TEXT("objectives")},At,Error) ||
            !Id(O,TEXT("quest_id"),Q.QuestId,At,Error) || !String(O,TEXT("display_name"),Q.DisplayName,At,Error) ||
            !String(O,TEXT("description"),Q.Description,At,Error,false,true) ||
            !Boolean(O,TEXT("allow_remote_completion"),Q.bAllowRemoteCompletion,At,Error) ||
            !Optional(O,TEXT("subject"),Q.Subject,ReadSubject,At,Error) || !Optional(O,TEXT("conditions"),Q.Conditions,ReadConditions,At,Error) ||
            !Optional(O,TEXT("accept_effects"),Q.AcceptEffects,ReadEffects,At,Error) || !Optional(O,TEXT("completion_effects"),Q.CompletionEffects,ReadEffects,At,Error)) return false;
        const FArray* Objectives = nullptr; if (!Array(O,TEXT("objectives"),Objectives,At,Error,true,64)) return false;
        for (const auto& Item : *Objectives)
        {
            FObject R; FOGNarrativeObjective Objective;
            if (!Object(Item,R,At,Error) || !Fields(R,{TEXT("objective_id"),TEXT("display_name"),TEXT("event_type"),TEXT("subject"),TEXT("required_count"),TEXT("allow_historical_events")},At,Error) ||
                !Id(R,TEXT("objective_id"),Objective.ObjectiveId,At,Error) || !String(R,TEXT("display_name"),Objective.DisplayName,At,Error) ||
                !ReadEvent(R,Objective.Event,At,Error) || !Integer(R,TEXT("required_count"),Objective.RequiredCount,At,Error,1,1000) ||
                !Boolean(R,TEXT("allow_historical_events"),Objective.bAllowHistoricalEvents,At,Error)) return false;
            Q.Objectives.Add(MoveTemp(Objective));
        }
        Parsed.Quests.Add(MoveTemp(Q));
    }
    if (!Array(Root,TEXT("dialogues"),A,TEXT("catalog"),Error,true)) return false;
    for (const auto& V : *A)
    {
        FObject O; FOGNarrativeDialogueDefinition D; const FString At = TEXT("dialogue");
        if (!Object(V,O,At,Error) || !Fields(O,{TEXT("dialogue_id"),TEXT("display_name"),TEXT("subject"),TEXT("conditions"),TEXT("entry_node_id"),TEXT("nodes")},At,Error) ||
            !Id(O,TEXT("dialogue_id"),D.DialogueId,At,Error) || !String(O,TEXT("display_name"),D.DisplayName,At,Error) ||
            !Optional(O,TEXT("subject"),D.Subject,ReadSubject,At,Error) || !Optional(O,TEXT("conditions"),D.Conditions,ReadConditions,At,Error) ||
            !Id(O,TEXT("entry_node_id"),D.EntryNodeId,At,Error)) return false;
        const FArray* Nodes = nullptr; if (!Array(O,TEXT("nodes"),Nodes,At,Error,true,128)) return false;
        for (const auto& Item : *Nodes)
        {
            FObject R; FOGNarrativeNodeDefinition N;
            if (!Object(Item,R,At,Error) || !Fields(R,{TEXT("node_id"),TEXT("text"),TEXT("terminal"),TEXT("choices")},At,Error) ||
                !Id(R,TEXT("node_id"),N.NodeId,At,Error) || !String(R,TEXT("text"),N.Text,At,Error) || !Boolean(R,TEXT("terminal"),N.bTerminal,At,Error)) return false;
            const FArray* Choices = nullptr; if (!Array(R,TEXT("choices"),Choices,At,Error,true,64)) return false;
            for (const auto& ItemChoice : *Choices)
            {
                FObject C; FOGNarrativeChoiceDefinition Choice;
                if (!Object(ItemChoice,C,At,Error) || !Fields(C,{TEXT("choice_id"),TEXT("text"),TEXT("next_node_id"),TEXT("conditions"),TEXT("effects")},At,Error) ||
                    !Id(C,TEXT("choice_id"),Choice.ChoiceId,At,Error) || !String(C,TEXT("text"),Choice.Text,At,Error) ||
                    !Id(C,TEXT("next_node_id"),Choice.NextNodeId,At,Error) || !Optional(C,TEXT("conditions"),Choice.Conditions,ReadConditions,At,Error) ||
                    !Optional(C,TEXT("effects"),Choice.Effects,ReadEffects,At,Error)) return false;
                N.Choices.Add(MoveTemp(Choice));
            }
            D.Nodes.Add(MoveTemp(N));
        }
        Parsed.Dialogues.Add(MoveTemp(D));
    }
    if (!Validate(Parsed,Error)) return false;
    Out = MoveTemp(Parsed); return true;
}

bool FOGNarrativeCatalog::Validate(const FOGNarrativeCatalog& Catalog, FString& Error)
{
    using namespace OGNarrative;
    Error.Reset();
    if (Catalog.Quests.Num() > 1000 || Catalog.Dialogues.Num() > 1000) return Fail(Error,TEXT("catalog"),TEXT("too many definitions"));
    TMap<FOGContentId,const FOGNarrativeQuestDefinition*> Quests; TSet<FOGContentId> ContentIds;
    for (const auto& Q : Catalog.Quests)
    {
        const FString At = Q.QuestId.Value;
        const bool HasSubject = !Q.Subject.ActorId.IsEmpty() || !Q.Subject.IdentityId.IsEmpty();
        if (!Q.QuestId.IsValid() || Q.DisplayName.TrimStartAndEnd().IsEmpty() || ContentIds.Contains(Q.QuestId) ||
            Q.Objectives.IsEmpty() || Q.Objectives.Num() > 64) return Fail(Error,At,TEXT("quest needs a unique ID, name and 1..64 objectives"));
        if (!ValidSubject(Q.Subject,false,At,Error) || !ValidConditions(Q.Conditions,HasSubject,At,Error) ||
            !ValidEffects(Q.AcceptEffects,Q.Conditions,At,Error) || !ValidEffects(Q.CompletionEffects,Q.Conditions,At,Error)) return false;
        for (const auto& A : Q.AcceptEffects.BranchLocks)
            for (const auto& B : Q.CompletionEffects.BranchLocks)
                if (A.BranchId == B.BranchId && A.ValueId != B.ValueId) return Fail(Error,At,TEXT("acceptance and completion lock incompatible branches"));
        TSet<FOGContentId> Objectives;
        for (const auto& O : Q.Objectives)
        {
            if (!O.ObjectiveId.IsValid() || O.DisplayName.TrimStartAndEnd().IsEmpty() || !Token(O.Event.EventType.ToString()) ||
                (!O.Event.bSubjectIsRuler && !HasSubject) || O.RequiredCount < 1 || O.RequiredCount > 1000 || Objectives.Contains(O.ObjectiveId))
                return Fail(Error,At,TEXT("invalid or duplicate objective; event subject must be bound"));
            Objectives.Add(O.ObjectiveId);
        }
        ContentIds.Add(Q.QuestId); Quests.Add(Q.QuestId,&Q);
    }
    auto Refs = [&](const FOGNarrativeConditions& C, const FString& At)
    {
        for (const auto& Id : C.CompletedQuests)
            if (!Quests.Contains(Id)) return Fail(Error,At,TEXT("unknown completed quest reference: ") + Id.Value);
        return true;
    };
    TMap<FOGContentId,int32> Pending; TMap<FOGContentId,TArray<FOGContentId>> Children; TArray<FOGContentId> Ready;
    for (const auto& Q : Catalog.Quests)
    {
        if (!Refs(Q.Conditions,Q.QuestId.Value)) return false;
        Pending.Add(Q.QuestId,Q.Conditions.CompletedQuests.Num());
        if (Q.Conditions.CompletedQuests.IsEmpty()) Ready.Add(Q.QuestId);
        for (const auto& Id : Q.Conditions.CompletedQuests) Children.FindOrAdd(Id).Add(Q.QuestId);
    }
    for (int32 I = 0; I < Ready.Num(); ++I)
        if (const auto* List = Children.Find(Ready[I]))
            for (const auto& Id : *List) if (--Pending.FindChecked(Id) == 0) Ready.Add(Id);
    if (Ready.Num() != Catalog.Quests.Num()) return Fail(Error,TEXT("quests"),TEXT("completed_quests prerequisite cycle"));
    for (const auto& D : Catalog.Dialogues)
    {
        const FString At = D.DialogueId.Value;
        if (!D.DialogueId.IsValid() || ContentIds.Contains(D.DialogueId) || D.DisplayName.TrimStartAndEnd().IsEmpty() ||
            D.Nodes.IsEmpty() || D.Nodes.Num() > 128 || !ValidSubject(D.Subject,true,At,Error) ||
            !ValidConditions(D.Conditions,true,At,Error) || !Refs(D.Conditions,At)) return Fail(Error,At,TEXT("invalid dialogue definition or subject/conditions"));
        ContentIds.Add(D.DialogueId);
        TMap<FOGContentId,const FOGNarrativeNodeDefinition*> Nodes; TSet<FOGContentId> Choices;
        for (const auto& N : D.Nodes)
        {
            if (!N.NodeId.IsValid() || Nodes.Contains(N.NodeId) || N.Text.TrimStartAndEnd().IsEmpty() ||
                N.Choices.Num() > 64 || (N.bTerminal ? !N.Choices.IsEmpty() : N.Choices.IsEmpty())) return Fail(Error,At,TEXT("invalid node, duplicate node or terminal/choices mismatch"));
            Nodes.Add(N.NodeId,&N);
            for (const auto& C : N.Choices)
            {
                if (!C.ChoiceId.IsValid() || Choices.Contains(C.ChoiceId) || C.Text.TrimStartAndEnd().IsEmpty() ||
                    !C.NextNodeId.IsValid() || !ValidConditions(C.Conditions,true,At,Error) || !ValidEffects(C.Effects,C.Conditions,At,Error) || !Refs(C.Conditions,At))
                    return Fail(Error,At,TEXT("invalid or duplicate dialogue choice"));
                Choices.Add(C.ChoiceId);
                for (const auto& Required : D.Conditions.RequiredBranches)
                    for (const auto& Lock : C.Effects.BranchLocks)
                        if (Required.BranchId == Lock.BranchId && Required.ValueId != Lock.ValueId) return Fail(Error,At,TEXT("choice contradicts dialogue branch condition"));
            }
        }
        if (!Nodes.Contains(D.EntryNodeId)) return Fail(Error,At,TEXT("missing entry node"));
        TArray<FOGContentId> Reachable{D.EntryNodeId}; TSet<FOGContentId> Seen; Seen.Add(D.EntryNodeId);
        TMap<FOGContentId,int32> InDegree;
        for (const auto& N : D.Nodes) InDegree.Add(N.NodeId,0);
        for (const auto& N : D.Nodes)
            for (const auto& C : N.Choices)
            {
                int32* Degree = InDegree.Find(C.NextNodeId);
                if (!Degree) return Fail(Error,At,TEXT("choice references an unknown next node"));
                ++*Degree;
            }
        for (int32 I = 0; I < Reachable.Num(); ++I)
            for (const auto& C : Nodes.FindChecked(Reachable[I])->Choices)
                if (!Seen.Contains(C.NextNodeId)) { Seen.Add(C.NextNodeId); Reachable.Add(C.NextNodeId); }
        if (Seen.Num() != Nodes.Num()) return Fail(Error,At,TEXT("unreachable dialogue node"));
        TArray<FOGContentId> Roots;
        for (const auto& P : InDegree) if (P.Value == 0) Roots.Add(P.Key);
        for (int32 I = 0; I < Roots.Num(); ++I)
            for (const auto& C : Nodes.FindChecked(Roots[I])->Choices)
                if (--InDegree.FindChecked(C.NextNodeId) == 0) Roots.Add(C.NextNodeId);
        if (Roots.Num() != Nodes.Num()) return Fail(Error,At,TEXT("dialogue cycles are unsupported; single-run branches cannot farm rewards"));
    }
    return true;
}
