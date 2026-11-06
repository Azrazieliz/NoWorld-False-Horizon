#include "Narrative/OGNarrativeFingerprint.h"
#include "Misc/SecureHash.h"

namespace OGNarrative
{
FString Digest(const FString& Text)
{
    FTCHARToUTF8 Bytes(*Text); FMD5 Hash; Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    uint8 Result[16]; Hash.Final(Result); return BytesToHex(Result,16).ToLower();
}
void AppendNarrativeFingerprintPart(FString& Out, const FString& Value) { Out += FString::Printf(TEXT("%d:"),Value.Len()) + Value; }
// Preserve the existing canonical fingerprint encoding for narrative callers.
void Part(FString& Out, const FString& Value) { AppendNarrativeFingerprintPart(Out, Value); }
namespace
{
void Branches(FString& S, const TArray<FOGNarrativeBranch>& B)
{
    AppendNarrativeFingerprintPart(S,LexToString(B.Num())); for (const auto& V : B) { AppendNarrativeFingerprintPart(S,V.BranchId.Value); AppendNarrativeFingerprintPart(S,V.ValueId.Value); }
}
void Subject(FString& S, const FOGNarrativeSubjectRule& R)
{
    for (const FString& V : {R.EntityKind.ToString(),R.ActorId.Value,R.IdentityId.Value,R.LifecycleState.ToString()}) AppendNarrativeFingerprintPart(S,V);
    TArray<FString> Keys; R.StateEquals.GetKeys(Keys); Keys.Sort(); AppendNarrativeFingerprintPart(S,LexToString(Keys.Num()));
    for (const auto& Key : Keys) { AppendNarrativeFingerprintPart(S,Key); AppendNarrativeFingerprintPart(S,R.StateEquals.FindRef(Key)); }
}
void Conditions(FString& S, const FOGNarrativeConditions& C)
{
    AppendNarrativeFingerprintPart(S,LexToString(C.CompletedQuests.Num())); for (const auto& Id : C.CompletedQuests) AppendNarrativeFingerprintPart(S,Id.Value);
    for (const auto* List : {&C.RequiredEvents,&C.ExcludedEvents})
    {
        AppendNarrativeFingerprintPart(S,LexToString(List->Num()));
        for (const auto& E : *List) { AppendNarrativeFingerprintPart(S,E.EventType.ToString()); AppendNarrativeFingerprintPart(S,E.bSubjectIsRuler ? TEXT("ruler") : TEXT("subject")); }
    }
    Branches(S,C.RequiredBranches);
    if (!C.RequiredDiscoveries.IsEmpty())
    {
        Part(S,TEXT("required_discoveries.v1"));
        Part(S,LexToString(C.RequiredDiscoveries.Num()));
        for (const FOGContentId& Id : C.RequiredDiscoveries) Part(S,Id.Value);
    }
    if (!C.InteractorStateEquals.IsEmpty())
    {
        Part(S,TEXT("interactor_state_equals.v1"));
        TArray<FString> Keys;
        C.InteractorStateEquals.GetKeys(Keys);
        Keys.Sort();
        Part(S,LexToString(Keys.Num()));
        for (const FString& Key : Keys)
        {
            Part(S,Key);
            Part(S,C.InteractorStateEquals.FindChecked(Key));
        }
    }
}
void Effects(FString& S, const FOGNarrativeEffects& E)
{
    for (const auto* List : {&E.Costs,&E.Rewards})
    {
        AppendNarrativeFingerprintPart(S,LexToString(List->Num())); for (const auto& R : *List) { AppendNarrativeFingerprintPart(S,R.ResourceId.Value); AppendNarrativeFingerprintPart(S,R.DisplayName); AppendNarrativeFingerprintPart(S,LexToString(R.Amount)); }
    }
    Branches(S,E.BranchLocks);
    // Existing definitions without discoveries retain their persisted fingerprint.
    if (!E.Discoveries.IsEmpty())
    {
        Part(S,TEXT("codex_discoveries.v1"));
        Part(S,LexToString(E.Discoveries.Num()));
        for (const FOGNarrativeDiscovery& Discovery : E.Discoveries)
        {
            Part(S,Discovery.EntryId.Value);
            Part(S,Discovery.Category.ToString());
            Part(S,Discovery.DisplayName);
            Part(S,Discovery.Summary);
            Part(S,Discovery.BeliefState.ToString());
            Part(S,LexToString(Discovery.ConfidenceBps));
        }
    }
}
}
FString DefinitionHash(const FOGNarrativeQuestDefinition& Q)
{
    FString S; AppendNarrativeFingerprintPart(S,TEXT("quest.v1"));
    for (const auto& V : {Q.QuestId.Value,Q.DisplayName,Q.Description,LexToString(Q.bAllowRemoteCompletion)}) AppendNarrativeFingerprintPart(S,V);
    Subject(S,Q.Subject); Conditions(S,Q.Conditions); Effects(S,Q.AcceptEffects); Effects(S,Q.CompletionEffects);
    if (!Q.ExpirationEvent.EventType.IsNone())
    {
        Part(S,TEXT("expires_on.v1"));
        Part(S,Q.ExpirationEvent.EventType.ToString());
        Part(S,LexToString(Q.ExpirationEvent.bSubjectIsRuler));
    }
    AppendNarrativeFingerprintPart(S,LexToString(Q.Objectives.Num()));
    for (const auto& O : Q.Objectives)
        for (const auto& V : {O.ObjectiveId.Value,O.DisplayName,O.Event.EventType.ToString(),
            LexToString(O.Event.bSubjectIsRuler),LexToString(O.RequiredCount),LexToString(O.bAllowHistoricalEvents)}) AppendNarrativeFingerprintPart(S,V);
    return Digest(S);
}
FString DefinitionHash(const FOGNarrativeDialogueDefinition& D)
{
    FString S; AppendNarrativeFingerprintPart(S,TEXT("dialogue.v1")); AppendNarrativeFingerprintPart(S,D.DialogueId.Value); AppendNarrativeFingerprintPart(S,D.DisplayName); Subject(S,D.Subject);
    Conditions(S,D.Conditions); AppendNarrativeFingerprintPart(S,D.EntryNodeId.Value); AppendNarrativeFingerprintPart(S,LexToString(D.Nodes.Num()));
    for (const auto& N : D.Nodes)
    {
        AppendNarrativeFingerprintPart(S,N.NodeId.Value); AppendNarrativeFingerprintPart(S,N.Text); AppendNarrativeFingerprintPart(S,LexToString(N.bTerminal)); AppendNarrativeFingerprintPart(S,LexToString(N.Choices.Num()));
        for (const auto& C : N.Choices)
        {
            AppendNarrativeFingerprintPart(S,C.ChoiceId.Value); AppendNarrativeFingerprintPart(S,C.Text); AppendNarrativeFingerprintPart(S,C.NextNodeId.Value); Conditions(S,C.Conditions); Effects(S,C.Effects);
        }
    }
    return Digest(S);
}
FString CommandHash(const FOGNarrativeCommand& C)
{
    FString S;
    for (const auto& V : {C.Context.RulerId.ToString(),C.Context.InteractorEntityId.ToString(),C.Context.SubjectId.ToString(),
        C.Context.LocationId.ToString(),LexToString(static_cast<int32>(C.Kind)),C.ContentId.Value,C.ChoiceId.Value,LexToString(C.EvidenceEventIds.Num())}) AppendNarrativeFingerprintPart(S,V);
    for (const auto& Id : C.EvidenceEventIds) AppendNarrativeFingerprintPart(S,Id.ToString());
    return Digest(S);
}
}
