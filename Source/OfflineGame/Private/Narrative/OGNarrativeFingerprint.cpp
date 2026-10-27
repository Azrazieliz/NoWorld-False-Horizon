#include "Narrative/OGNarrativeFingerprint.h"
#include "Misc/SecureHash.h"

namespace OGNarrative
{
FString Digest(const FString& Text)
{
    FTCHARToUTF8 Bytes(*Text); FMD5 Hash; Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    uint8 Result[16]; Hash.Final(Result); return BytesToHex(Result,16).ToLower();
}
void Part(FString& Out, const FString& Value) { Out += FString::Printf(TEXT("%d:"),Value.Len()) + Value; }
namespace
{
void Branches(FString& S, const TArray<FOGNarrativeBranch>& B)
{
    Part(S,LexToString(B.Num())); for (const auto& V : B) { Part(S,V.BranchId.Value); Part(S,V.ValueId.Value); }
}
void Subject(FString& S, const FOGNarrativeSubjectRule& R)
{
    for (const FString& V : {R.EntityKind.ToString(),R.ActorId.Value,R.IdentityId.Value,R.LifecycleState.ToString()}) Part(S,V);
    TArray<FString> Keys; R.StateEquals.GetKeys(Keys); Keys.Sort(); Part(S,LexToString(Keys.Num()));
    for (const auto& Key : Keys) { Part(S,Key); Part(S,R.StateEquals.FindRef(Key)); }
}
void Conditions(FString& S, const FOGNarrativeConditions& C)
{
    Part(S,LexToString(C.CompletedQuests.Num())); for (const auto& Id : C.CompletedQuests) Part(S,Id.Value);
    for (const auto* List : {&C.RequiredEvents,&C.ExcludedEvents})
    {
        Part(S,LexToString(List->Num()));
        for (const auto& E : *List) { Part(S,E.EventType.ToString()); Part(S,E.bSubjectIsRuler ? TEXT("ruler") : TEXT("subject")); }
    }
    Branches(S,C.RequiredBranches);
}
void Effects(FString& S, const FOGNarrativeEffects& E)
{
    for (const auto* List : {&E.Costs,&E.Rewards})
    {
        Part(S,LexToString(List->Num())); for (const auto& R : *List) { Part(S,R.ResourceId.Value); Part(S,R.DisplayName); Part(S,LexToString(R.Amount)); }
    }
    Branches(S,E.BranchLocks);
}
}
FString DefinitionHash(const FOGNarrativeQuestDefinition& Q)
{
    FString S; Part(S,TEXT("quest.v1"));
    for (const auto& V : {Q.QuestId.Value,Q.DisplayName,Q.Description,LexToString(Q.bAllowRemoteCompletion)}) Part(S,V);
    Subject(S,Q.Subject); Conditions(S,Q.Conditions); Effects(S,Q.AcceptEffects); Effects(S,Q.CompletionEffects);
    Part(S,LexToString(Q.Objectives.Num()));
    for (const auto& O : Q.Objectives)
        for (const auto& V : {O.ObjectiveId.Value,O.DisplayName,O.Event.EventType.ToString(),
            LexToString(O.Event.bSubjectIsRuler),LexToString(O.RequiredCount),LexToString(O.bAllowHistoricalEvents)}) Part(S,V);
    return Digest(S);
}
FString DefinitionHash(const FOGNarrativeDialogueDefinition& D)
{
    FString S; Part(S,TEXT("dialogue.v1")); Part(S,D.DialogueId.Value); Part(S,D.DisplayName); Subject(S,D.Subject);
    Conditions(S,D.Conditions); Part(S,D.EntryNodeId.Value); Part(S,LexToString(D.Nodes.Num()));
    for (const auto& N : D.Nodes)
    {
        Part(S,N.NodeId.Value); Part(S,N.Text); Part(S,LexToString(N.bTerminal)); Part(S,LexToString(N.Choices.Num()));
        for (const auto& C : N.Choices)
        {
            Part(S,C.ChoiceId.Value); Part(S,C.Text); Part(S,C.NextNodeId.Value); Conditions(S,C.Conditions); Effects(S,C.Effects);
        }
    }
    return Digest(S);
}
FString CommandHash(const FOGNarrativeCommand& C)
{
    FString S;
    for (const auto& V : {C.Context.RulerId.ToString(),C.Context.InteractorEntityId.ToString(),C.Context.SubjectId.ToString(),
        C.Context.LocationId.ToString(),LexToString(static_cast<int32>(C.Kind)),C.ContentId.Value,C.ChoiceId.Value,LexToString(C.EvidenceEventIds.Num())}) Part(S,V);
    for (const auto& Id : C.EvidenceEventIds) Part(S,Id.ToString());
    return Digest(S);
}
}
