#include "Narrative/OGNarrativeService.h"
#include "Narrative/OGNarrativeFingerprint.h"
#include "Narrative/OGNarrativeJson.h"

namespace
{
using namespace OGNarrative;
const FName ReceiptType(TEXT("noworld.narrative.command_applied"));
const TCHAR* StateSchema = TEXT("noworld.narrative_state.v1");
using FLocks = TMap<FOGContentId,FOGContentId>;

FOGEntityId ProjectId(const FOGEntityId& Ruler, const TCHAR* Kind, const FOGContentId& Content,
    const FOGEntityId& Subject = FOGEntityId())
{
    FString Key; Part(Key,TEXT("noworld.narrative.project.v1")); Part(Key,Ruler.ToString());
    Part(Key,Kind); Part(Key,Content.Value); Part(Key,Subject.ToString());
    FGuid Guid; FGuid::ParseExact(Digest(Key),EGuidFormats::Digits,Guid); return FOGEntityId(Guid);
}
FOGContentId ProjectType(const TCHAR* Kind) { return FOGContentId(FString(TEXT("noworld:narrative.")) + Kind); }
void StampProject(FString& Stamp, bool Found, const FOGProjectRecord& P)
{
    Part(Stamp,LexToString(Found));
    if (Found)
        for (const auto& S : {P.ProjectId.ToString(),P.OwnerEntityId.ToString(),P.LocationId.ToString(),P.ProjectTypeId.Value,
            LexToString(static_cast<int32>(P.Status)),LexToString(P.StartWorldTick),LexToString(P.ResolveWorldTick),LexToString(P.ProgressBps),P.PayloadJson}) Part(Stamp,S);
}
bool Load(IOGWorldStore& Store, const FOGEntityId& Ruler, const TCHAR* Kind, const FOGContentId& Content,
    const FOGEntityId& Subject, bool& Found, FOGProjectRecord& P, FObject& Root, FString& Error)
{
    if (!Store.IsOpen() || !Ruler.IsValid() || !Content.IsValid()) return Fail(Error,TEXT("narrative"),TEXT("open world, ruler and content ID required"));
    const auto Id = ProjectId(Ruler,Kind,Content,Subject);
    if (!Store.TryReadProject(Id,Found,P,Error)) return false;
    if (!Found)
    {
        FName EntityKind; FString EntityState; int64 Revision = 0; bool EntityFound = false;
        if (!Store.TryReadEntity(Id,EntityFound,EntityKind,EntityState,Revision,Error)) return false;
        return !EntityFound || Fail(Error,TEXT("narrative"),TEXT("reserved project ID is already occupied by another entity"));
    }
    if (P.OwnerEntityId != Ruler || P.ProjectTypeId != ProjectType(Kind) || P.StartWorldTick < 0 ||
        P.ResolveWorldTick < P.StartWorldTick || !P.LocationId.IsValid()) return Fail(Error,TEXT("narrative"),TEXT("project owner/type/timeline mismatch"));
    if (!ReadObject(P.PayloadJson,Root,Error)) return false;
    FString Schema;
    return (String(Root,TEXT("schema"),Schema,TEXT("state"),Error) && Schema.Equals(StateSchema,ESearchCase::CaseSensitive)) ||
        Fail(Error,TEXT("state"),TEXT("unsupported narrative save schema"));
}
bool SavedIdentity(const FObject& Root, const FOGContentId& Content, FOGEntityId& Subject,
    FString& Definition, FString& Error)
{
    FOGContentId Id; FString Text;
    if (!OGNarrative::Id(Root,TEXT("content_id"),Id,TEXT("state"),Error) || Id != Content ||
        !String(Root,TEXT("definition_hash"),Definition,TEXT("state"),Error) ||
        !String(Root,TEXT("subject_id"),Text,TEXT("state"),Error,true,true)) return Fail(Error,TEXT("state"),TEXT("saved content identity/hash mismatch"));
    if (Text.IsEmpty()) { Subject = FOGEntityId(); return true; }
    FGuid Guid;
    if (!FGuid::ParseExact(Text,EGuidFormats::DigitsWithHyphens,Guid) || !Guid.IsValid()) return Fail(Error,TEXT("state.subject_id"),TEXT("invalid entity UUID"));
    Subject = FOGEntityId(Guid); return true;
}
bool LoadQuest(IOGWorldStore& Store, const FOGEntityId& Ruler, const FOGContentId& Id,
    FOGNarrativeQuestState& Out, FOGProjectRecord& P, FString& Definition, FString& Error)
{
    Out = FOGNarrativeQuestState(); FObject Root;
    if (!Load(Store,Ruler,TEXT("quest"),Id,FOGEntityId(),Out.bExists,P,Root,Error)) return false;
    if (!Out.bExists) return true;
    if (!Fields(Root,{TEXT("schema"),TEXT("content_id"),TEXT("definition_hash"),TEXT("subject_id"),TEXT("completed"),TEXT("objectives"),TEXT("acceptance_baseline")},TEXT("quest state"),Error) ||
        !SavedIdentity(Root,Id,Out.SubjectId,Definition,Error) || !Root->HasField(TEXT("completed")) ||
        !Boolean(Root,TEXT("completed"),Out.bCompleted,TEXT("quest state"),Error)) return Fail(Error,TEXT("quest state"),TEXT("invalid saved quest fields"));
    for (int32 MapIndex = 0; MapIndex < 2; ++MapIndex)
    {
      const TCHAR* Key = MapIndex == 0 ? TEXT("objectives") : TEXT("acceptance_baseline");
      const auto* Values = Root->Values.Find(Key); FObject Objectives;
      if (!Values || !Object(*Values,Objectives,Key,Error) || Objectives->Values.Num() > 64)
          return Fail(Error,TEXT("quest state"),TEXT("invalid saved objectives/baseline"));
      for (const auto& Pair : Objectives->Values)
      {
        FOGContentId Objective(Pair.Key); const FArray* Events = nullptr;
        if (!Objective.IsValid() || !Pair.Value.IsValid() || !Pair.Value->TryGetArray(Events) || !Events || Events->Num() > 1000)
            return Fail(Error,TEXT("quest state"),TEXT("invalid objective evidence array"));
        auto& Credits = (MapIndex == 0 ? Out.CreditedEvents : Out.AcceptanceBaseline).FindOrAdd(Objective); TSet<FOGEntityId> Seen;
        for (const auto& V : *Events)
        {
            FString Text; FGuid Guid;
            if (!V.IsValid() || V->Type != EJson::String || !V->TryGetString(Text) ||
                !FGuid::ParseExact(Text,EGuidFormats::DigitsWithHyphens,Guid) || !Guid.IsValid() || Seen.Contains(FOGEntityId(Guid)))
                return Fail(Error,TEXT("quest state"),TEXT("invalid or duplicate credited event UUID"));
            Seen.Add(FOGEntityId(Guid)); Credits.Emplace(Guid);
        }
      }
    }
    if (P.Status != (Out.bCompleted ? EOGProjectStatus::Completed : EOGProjectStatus::Active))
        return Fail(Error,TEXT("quest state"),TEXT("saved completion/status mismatch"));
    Out.StartedWorldTick = P.StartWorldTick; Out.UpdatedWorldTick = P.ResolveWorldTick; return true;
}
bool LoadDialogue(IOGWorldStore& Store, const FOGNarrativeContext& Context, const FOGContentId& Id,
    FOGNarrativeDialogueState& Out, FOGProjectRecord& P, FString& Definition, FString& Error)
{
    Out = FOGNarrativeDialogueState(); FObject Root; FOGEntityId Subject;
    if (!Context.SubjectId.IsValid()) return Fail(Error,TEXT("dialogue"),TEXT("canonical subject entity is required"));
    if (!Load(Store,Context.RulerId,TEXT("dialogue"),Id,Context.SubjectId,Out.bExists,P,Root,Error)) return false;
    if (!Out.bExists) return true;
    if (!Fields(Root,{TEXT("schema"),TEXT("content_id"),TEXT("definition_hash"),TEXT("subject_id"),TEXT("node_id"),TEXT("completed")},TEXT("dialogue state"),Error) ||
        !SavedIdentity(Root,Id,Subject,Definition,Error) || Subject != Context.SubjectId || !Root->HasField(TEXT("completed")) ||
        !Boolean(Root,TEXT("completed"),Out.bCompleted,TEXT("dialogue state"),Error) ||
        !OGNarrative::Id(Root,TEXT("node_id"),Out.NodeId,TEXT("dialogue state"),Error)) return Fail(Error,TEXT("dialogue state"),TEXT("invalid saved dialogue fields"));
    if (P.Status != (Out.bCompleted ? EOGProjectStatus::Completed : EOGProjectStatus::Active))
        return Fail(Error,TEXT("dialogue state"),TEXT("saved completion/status mismatch"));
    Out.UpdatedWorldTick = P.ResolveWorldTick; return true;
}
const FOGContentId BranchContent(TEXT("noworld:narrative.branches"));
bool LoadLocks(IOGWorldStore& Store, const FOGEntityId& Ruler, FLocks& Out,
    bool& Found, FOGProjectRecord& P, FString& Error)
{
    FObject Root; Out.Reset();
    if (!Load(Store,Ruler,TEXT("branches"),BranchContent,FOGEntityId(),Found,P,Root,Error)) return false;
    if (!Found) return true;
    if (!Fields(Root,{TEXT("schema"),TEXT("locks")},TEXT("branch state"),Error)) return false;
    const auto* V = Root->Values.Find(TEXT("locks")); FObject Locks;
    if (!V || !Object(*V,Locks,TEXT("locks"),Error) || Locks->Values.Num() > 4096 || P.Status != EOGProjectStatus::Active)
        return Fail(Error,TEXT("branch state"),TEXT("invalid lock map or project status"));
    for (const auto& Pair : Locks->Values)
    {
        FString Value;
        if (!FOGContentId(Pair.Key).IsValid() || !Pair.Value.IsValid() || Pair.Value->Type != EJson::String ||
            !Pair.Value->TryGetString(Value) || !FOGContentId(Value).IsValid()) return Fail(Error,TEXT("branch state"),TEXT("invalid canonical branch/value ID"));
        Out.Add(FOGContentId(Pair.Key),FOGContentId(Value));
    }
    return true;
}
bool Entity(IOGWorldStore& Store, const FOGEntityId& Id, const FString& Role,
    FName& Kind, FString& State, FString& Stamp, TArray<FString>& Missing, FString& Error)
{
    bool Found = false; int64 Revision = 0;
    if (!Id.IsValid()) { Missing.Add(Role + TEXT(" entity is required.")); return true; }
    if (!Store.TryReadEntity(Id,Found,Kind,State,Revision,Error)) return false;
    for (const auto& S : {Role,Id.ToString(),LexToString(Found),Kind.ToString(),State,LexToString(Revision)}) Part(Stamp,S);
    if (!Found) Missing.Add(Role + TEXT(" entity no longer exists."));
    else if (State.Contains(TEXT("\"diagnosticFixture\""))) Missing.Add(Role + TEXT(" is a diagnostic fixture, not a canonical narrative participant."));
    return true;
}
bool Presence(IOGWorldStore& Store, const FOGEntityId& Id, const FOGEntityId& Location,
    int64 Tick, FString& Stamp, TArray<FString>& Missing, FString& Error)
{
    bool Found = false; FOGWorldPresenceRecord P;
    if (!Id.IsValid()) { Missing.Add(TEXT("Local interaction needs a canonical entity.")); return true; }
    if (!Store.TryReadWorldPresence(Id,Found,P,Error)) return false;
    for (const auto& S : {Id.ToString(),LexToString(Found),P.LocationId.ToString(),P.LocalPosition.ToString(),P.MovementContext.ToString(),LexToString(P.UpdatedWorldTick)}) Part(Stamp,S);
    if (!Found || P.LocationId != Location) Missing.Add(TEXT("Interactor and subject must occupy this canonical location."));
    if (Found && P.UpdatedWorldTick > Tick) return Fail(Error,TEXT("presence"),TEXT("command tick precedes current canonical presence"));
    return true;
}
bool ContextCheck(IOGWorldStore& Store, const FOGNarrativeContext& C, bool Local, int64 Tick,
    FString& Stamp, TArray<FString>& Missing, FString& Error)
{
    FName Kind; FString State;
    if (!Entity(Store,C.RulerId,TEXT("Ruler"),Kind,State,Stamp,Missing,Error)) return false;
    if (Kind != FName(TEXT("ruler"))) Missing.Add(TEXT("Narrative ownership requires a canonical ruler entity."));
    if (!State.IsEmpty())
    {
        FObject RulerState;
        if (!ReadObject(State,RulerState,Error)) return false;
        if (RulerState->HasField(TEXT("diagnosticFixture"))) Missing.Add(TEXT("Diagnostic fixtures cannot own canonical narrative rewards or quests."));
    }
    const auto Interactor = C.InteractorEntityId.IsValid() ? C.InteractorEntityId : C.RulerId;
    if (Interactor != C.RulerId)
    {
        bool Found = false; FOGCharacterManifestationRecord M;
        if (!Store.TryReadCharacterManifestation(Interactor,Found,M,Error)) return false;
        for (const auto& S : {Interactor.ToString(),LexToString(Found),M.OwningRulerId.ToString(),M.IdentityId.Value,M.ActiveVersionId.Value,M.LifecycleState.ToString()}) Part(Stamp,S);
        if (!Found || M.OwningRulerId != C.RulerId || M.LifecycleState != FName(TEXT("active")) ||
            M.IdentityId.Value.StartsWith(TEXT("diagnostic:")) || M.ActiveVersionId.Value.StartsWith(TEXT("diagnostic:")))
            Missing.Add(TEXT("Interactor must be an active manifestation owned by this ruler."));
        if (!Entity(Store,Interactor,TEXT("Interactor"),Kind,State,Stamp,Missing,Error)) return false;
    }
    if (!Local) return true; // Journal reads/writes never manufacture observation.
    bool Found = false; FOGLocationRecord L;
    if (!C.LocationId.IsValid()) { Missing.Add(TEXT("Local interaction needs its canonical location.")); return true; }
    if (!Store.TryReadLocation(C.LocationId,Found,L,Error)) return false;
    for (const auto& S : {C.LocationId.ToString(),LexToString(Found),L.Kind.ToString(),L.ParentLocationId.ToString(),L.TerritoryId.ToString(),LexToString(L.bPhysicallyAccessible)}) Part(Stamp,S);
    if (!Found || !L.bPhysicallyAccessible) Missing.Add(TEXT("Interaction location is unavailable."));
    return Presence(Store,Interactor,C.LocationId,Tick,Stamp,Missing,Error);
}
bool SubjectCheck(IOGWorldStore& Store, const FOGNarrativeSubjectRule& Rule, const FOGNarrativeContext& C,
    bool Local, int64 Tick, FString& Stamp, TArray<FString>& Missing, FString& Error)
{
    if (Rule.ActorId.IsEmpty() && Rule.IdentityId.IsEmpty()) return true;
    FName Kind; FString Json;
    const int32 Before = Missing.Num();
    if (!Entity(Store,C.SubjectId,TEXT("Subject"),Kind,Json,Stamp,Missing,Error)) return false;
    if (Missing.Num() != Before) return true;
    if (Kind != Rule.EntityKind) Missing.Add(TEXT("This entity is not an authored subject for this content."));
    FObject State;
    if (!ReadObject(Json,State,Error)) return false;
    if (State->HasField(TEXT("diagnosticFixture"))) Missing.Add(TEXT("Diagnostic subjects cannot unlock canonical narrative choices."));
    if (!Rule.ActorId.IsEmpty())
    {
        FString Actor;
        const auto* V = State->Values.Find(TEXT("narrative_actor_id"));
        if (!V || !V->IsValid() || (*V)->Type != EJson::String || !(*V)->TryGetString(Actor) ||
            !Actor.Equals(Rule.ActorId.Value,ESearchCase::CaseSensitive)) Missing.Add(TEXT("Subject's canonical actor identity does not match."));
    }
    else
    {
        bool Found = false; FOGCharacterManifestationRecord M;
        if (!Store.TryReadCharacterManifestation(C.SubjectId,Found,M,Error)) return false;
        for (const auto& S : {LexToString(Found),M.IdentityId.Value,M.ActiveVersionId.Value,M.LifecycleState.ToString(),M.OwningRulerId.ToString()}) Part(Stamp,S);
        if (!Found || M.IdentityId != Rule.IdentityId || M.LifecycleState != Rule.LifecycleState)
            Missing.Add(TEXT("Subject's canonical identity or lifecycle does not meet this scene's authored rule."));
    }
    for (const auto& Predicate : Rule.StateEquals)
    {
        FString Actual; const auto* V = State->Values.Find(Predicate.Key);
        if (!V || !V->IsValid() || (*V)->Type != EJson::String || !(*V)->TryGetString(Actual) ||
            !Actual.Equals(Predicate.Value,ESearchCase::CaseSensitive))
            Missing.Add(TEXT("Subject's current canonical state does not meet the authored scene conditions."));
    }
    return !Local || Presence(Store,C.SubjectId,C.LocationId,Tick,Stamp,Missing,Error);
}
bool AppliesTo(const FOGWorldEvent& E, const FOGEntityId& Id)
{
    return Id.IsValid() && (E.PrimaryEntity == Id || E.RelatedEntities.Contains(Id));
}
void StampEvent(FString& Stamp, const FOGWorldEvent& E)
{
    for (const auto& S : {E.EventId.ToString(),E.EventType.ToString(),LexToString(E.WorldTick),E.PrimaryEntity.ToString(),E.PayloadJson,LexToString(E.bChronicleEligible)}) Part(Stamp,S);
    Part(Stamp,LexToString(E.RelatedEntities.Num())); for (const auto& Id : E.RelatedEntities) Part(Stamp,Id.ToString());
}
bool AcceptanceBaseline(IOGWorldStore& Store, const FOGNarrativeQuestDefinition& Q, const FOGNarrativeContext& C,
    int64 Tick, FOGNarrativeQuestState& State, FString& Stamp, FString& Error)
{
    for (const auto& O : Q.Objectives)
    {
        auto& Baseline = State.AcceptanceBaseline.FindOrAdd(O.ObjectiveId);
        Part(Stamp,O.ObjectiveId.Value);
        if (O.bAllowHistoricalEvents) { Part(Stamp,TEXT("historical")); continue; }
        const auto Subject = O.Event.bSubjectIsRuler ? C.RulerId : C.SubjectId;
        if (!Subject.IsValid()) return Fail(Error,TEXT("acceptance"),TEXT("objective subject must be bound before accepting"));
        TArray<FOGWorldEvent> Events;
        if (!Store.ListWorldEvents(Subject,O.Event.EventType,false,1000,Events,Error)) return false;
        if (Events.Num() == 1000 && Events.Last().WorldTick >= Tick)
            return Fail(Error,TEXT("acceptance"),TEXT("same-tick objective baseline exceeds store query window; accept at a later canonical tick"));
        for (const auto& E : Events)
        {
            if (E.WorldTick > Tick) return Fail(Error,TEXT("acceptance"),TEXT("cannot accept before latest canonical objective history"));
            if (E.WorldTick != Tick) break;
            if (E.EventType != O.Event.EventType || !AppliesTo(E,Subject)) return Fail(Error,TEXT("acceptance"),TEXT("store returned an objective event outside its exact scope"));
            Baseline.Add(E.EventId);
        }
        TArray<FString> Ids; for (const auto& Id : Baseline) Ids.Add(Id.ToString()); Ids.Sort();
        Part(Stamp,LexToString(Ids.Num())); for (const auto& Id : Ids) Part(Stamp,Id);
    }
    return true;
}
bool Conditions(IOGWorldStore& Store, const FOGNarrativeConditions& C, const FOGNarrativeContext& Context,
    const FLocks& Locks, int64 Tick, FString& Stamp, TArray<FString>& Missing, FString& Error)
{
    for (const auto& Id : C.CompletedQuests)
    {
        FOGNarrativeQuestState State; FOGProjectRecord P; FString Hash;
        if (!LoadQuest(Store,Context.RulerId,Id,State,P,Hash,Error)) return false;
        StampProject(Stamp,State.bExists,P);
        if (!State.bExists || !State.bCompleted) Missing.Add(TEXT("A required quest has not been completed."));
    }
    for (const auto& Need : C.RequiredBranches)
    {
        const auto* Value = Locks.Find(Need.BranchId);
        if (!Value || *Value != Need.ValueId) Missing.Add(TEXT("This authored branch requirement has not been met."));
    }
    for (int32 Index = 0; Index < 2; ++Index)
        for (const auto& Rule : Index == 0 ? C.RequiredEvents : C.ExcludedEvents)
        {
            const auto Subject = Rule.bSubjectIsRuler ? Context.RulerId : Context.SubjectId;
            if (!Subject.IsValid()) { Missing.Add(TEXT("Event condition requires its bound canonical subject.")); continue; }
            TArray<FOGWorldEvent> Events;
            if (!Store.ListWorldEvents(Subject,Rule.EventType,false,1,Events,Error)) return false;
            Part(Stamp,LexToString(Events.Num()));
            if (!Events.IsEmpty())
            {
                StampEvent(Stamp,Events[0]);
                if (Events[0].WorldTick > Tick) return Fail(Error,TEXT("event condition"),TEXT("command tick precedes the canonical event ledger"));
                if (Events[0].EventType != Rule.EventType || !AppliesTo(Events[0],Subject)) return Fail(Error,TEXT("event condition"),TEXT("store returned evidence outside exact subject/type"));
            }
            if (Index == 0 ? Events.IsEmpty() : !Events.IsEmpty())
                Missing.Add(Index == 0 ? TEXT("A required canonical event has not occurred.") : TEXT("A canonical event excludes this authored branch."));
        }
    return true;
}
bool Resources(IOGWorldStore& Store, const FOGEntityId& Ruler, const FOGNarrativeEffects& E,
    const FLocks& Locks, FString& Stamp, FOGNarrativePreview& Out, FString& Error)
{
    TMap<FOGContentId,FOGNarrativeResourcePreview> Rows;
    for (int32 Index = 0; Index < 2; ++Index)
        for (const auto& R : Index == 0 ? E.Costs : E.Rewards)
        {
            auto& Row = Rows.FindOrAdd(R.ResourceId); Row.ResourceId = R.ResourceId; Row.DisplayName = R.DisplayName;
            (Index == 0 ? Row.Cost : Row.Reward) = R.Amount;
        }
    TArray<FOGContentId> Keys; Rows.GetKeys(Keys); Keys.Sort([](const auto& A,const auto& B){return A.Value < B.Value;});
    for (const auto& Key : Keys)
    {
        auto Row = Rows.FindRef(Key); bool Found = false;
        if (!Store.TryReadResourceBalance(Ruler,Key,Found,Row.BalanceBefore,Error)) return false;
        if (!Found) Row.BalanceBefore = 0;
        for (const auto& S : {Key.Value,LexToString(Found),LexToString(Row.BalanceBefore)}) Part(Stamp,S);
        if (Row.BalanceBefore < 0) return Fail(Error,TEXT("resources"),TEXT("canonical balance cannot be negative"));
        if (Row.BalanceBefore < Row.Cost) Out.MissingRequirements.Add(TEXT("Insufficient ") + Row.DisplayName + TEXT("."));
        else if (Row.Reward > MAX_int64 - (Row.BalanceBefore - Row.Cost)) Out.MissingRequirements.Add(TEXT("Reward exceeds the canonical resource capacity for ") + Row.DisplayName + TEXT("."));
        else Row.BalanceAfter = Row.BalanceBefore - Row.Cost + Row.Reward;
        Out.Resources.Add(MoveTemp(Row));
    }
    for (const auto& B : E.BranchLocks)
        if (const auto* Existing = Locks.Find(B.BranchId))
            if (*Existing != B.ValueId) Out.MissingRequirements.Add(TEXT("A previous canonical choice locks the alternative branch."));
    return true;
}

bool Save(IOGWorldStore& Store, const FOGNarrativeContext& C, const TCHAR* Kind, const FOGContentId& Id,
    const FOGEntityId& BoundSubject, const FObject& Root, int64 Tick, bool Completed, int32 Progress, FString& Error)
{
    bool Found = false; FOGProjectRecord P; FObject Old;
    if (!Load(Store,C.RulerId,Kind,Id,BoundSubject,Found,P,Old,Error)) return false;
    if (Found && Tick < P.ResolveWorldTick) return Fail(Error,TEXT("narrative"),TEXT("cannot overwrite later canonical history"));
    if (!Found)
    {
        P.ProjectId = ProjectId(C.RulerId,Kind,Id,BoundSubject); P.OwnerEntityId = C.RulerId;
        P.LocationId = C.LocationId; P.ProjectTypeId = ProjectType(Kind); P.StartWorldTick = Tick;
    }
    P.ResolveWorldTick = Tick; P.Status = Completed ? EOGProjectStatus::Completed : EOGProjectStatus::Active;
    P.ProgressBps = Progress; P.PayloadJson = Write(Root);
    return Store.UpsertProject(P,P.StartWorldTick,Error);
}
FObject Base(const FOGContentId& Id, const FOGEntityId& Subject, const FString& Hash, bool Completed)
{
    auto Root = MakeShared<FJsonObject>(); Root->SetStringField(TEXT("schema"),StateSchema);
    Root->SetStringField(TEXT("content_id"),Id.Value); Root->SetStringField(TEXT("subject_id"),Subject.IsValid() ? Subject.ToString() : TEXT(""));
    Root->SetStringField(TEXT("definition_hash"),Hash); Root->SetBoolField(TEXT("completed"),Completed); return Root;
}
struct FTransaction
{
    IOGWorldStore& Store; FString& Error; bool Active = false;
    FTransaction(IOGWorldStore& S,FString& E) : Store(S),Error(E) {}
    ~FTransaction() { if (Active) { FString E; if (!Store.RollbackTransaction(E)) Error += TEXT(" Rollback failed: ") + E; } }
    bool Begin() { Active = Store.BeginTransaction(Error); return Active; }
    bool Commit() { if (!Store.CommitTransaction(Error)) return false; Active = false; return true; }
};
}

bool FOGNarrativeService::ReadQuest(const FOGEntityId& RulerId, const FOGContentId& QuestId,
    FOGNarrativeQuestState& Out, FString& Error) const
{
    Error.Reset(); FOGProjectRecord P; FString Hash; return LoadQuest(Store,RulerId,QuestId,Out,P,Hash,Error);
}
bool FOGNarrativeService::ReadDialogue(const FOGNarrativeContext& Context, const FOGContentId& DialogueId,
    FOGNarrativeDialogueState& Out, FString& Error) const
{
    Error.Reset(); FOGProjectRecord P; FString Hash; return LoadDialogue(Store,Context,DialogueId,Out,P,Hash,Error);
}

bool FOGNarrativeService::Preview(const FOGNarrativeCatalog& Catalog, const FOGNarrativeCommand& Command,
    int64 Tick, FOGNarrativePreview& Out, FString& Error) const
{
    Out = FOGNarrativePreview(); Error.Reset();
    const auto& C = Command.Context;
    const bool IsQuest = Command.Kind == EOGNarrativeCommand::AcceptQuest || Command.Kind == EOGNarrativeCommand::AdvanceQuest || Command.Kind == EOGNarrativeCommand::CompleteQuest;
    const bool IsDialogue = Command.Kind == EOGNarrativeCommand::StartDialogue || Command.Kind == EOGNarrativeCommand::ChooseDialogue;
    if (!Store.IsOpen() || !C.RulerId.IsValid() || !Command.ContentId.IsValid() || Tick < 0 || (!IsQuest && !IsDialogue) ||
        (Command.Kind == EOGNarrativeCommand::ChooseDialogue ? !Command.ChoiceId.IsValid() : !Command.ChoiceId.IsEmpty()) ||
        (Command.Kind == EOGNarrativeCommand::AdvanceQuest ? Command.EvidenceEventIds.IsEmpty() : !Command.EvidenceEventIds.IsEmpty()) ||
        Command.EvidenceEventIds.Num() > 256) return Fail(Error,TEXT("command"),TEXT("invalid narrative command, IDs, evidence batch or tick"));
    TSet<FOGEntityId> EvidenceIds;
    for (const auto& Id : Command.EvidenceEventIds)
    {
        if (!Id.IsValid() || EvidenceIds.Contains(Id)) return Fail(Error,TEXT("evidence"),TEXT("event IDs must be valid and unique"));
        EvidenceIds.Add(Id);
    }
    if (!FOGNarrativeCatalog::Validate(Catalog,Error)) return false;
    const auto* Q = IsQuest ? Catalog.Quests.FindByPredicate([&](const auto& V){return V.QuestId == Command.ContentId;}) : nullptr;
    const auto* D = IsDialogue ? Catalog.Dialogues.FindByPredicate([&](const auto& V){return V.DialogueId == Command.ContentId;}) : nullptr;
    if ((!Q && IsQuest) || (!D && IsDialogue)) return Fail(Error,TEXT("command"),TEXT("content ID is not in the approved catalog"));
    FOGProjectRecord Project, BranchProject; FString SavedHash; FLocks Locks; bool BranchFound = false;
    if (!LoadLocks(Store,C.RulerId,Locks,BranchFound,BranchProject,Error)) return false;
    if (BranchFound && BranchProject.ResolveWorldTick > Tick) return Fail(Error,TEXT("branches"),TEXT("command precedes saved history"));
    FString Stamp; Part(Stamp,CommandHash(Command)); StampProject(Stamp,BranchFound,BranchProject);
    FOGNarrativeEffects NoEffects; const FOGNarrativeEffects* Effects = &NoEffects;
    const auto& SubjectRule = Q ? Q->Subject : D->Subject;
    const bool HasSubject = !SubjectRule.ActorId.IsEmpty() || !SubjectRule.IdentityId.IsEmpty();
    const bool Local = IsDialogue || Command.Kind == EOGNarrativeCommand::AcceptQuest ||
        (Command.Kind == EOGNarrativeCommand::CompleteQuest && HasSubject && !Q->bAllowRemoteCompletion);
    if (!ContextCheck(Store,C,Local,Tick,Stamp,Out.MissingRequirements,Error) ||
        !SubjectCheck(Store,SubjectRule,C,Local && HasSubject,Tick,Stamp,Out.MissingRequirements,Error)) return false;

    if (Q)
    {
        const FString Hash = DefinitionHash(*Q); Part(Stamp,Hash); Out.DisplayName = Q->DisplayName;
        if (!LoadQuest(Store,C.RulerId,Q->QuestId,Out.Quest,Project,SavedHash,Error)) return false;
        StampProject(Stamp,Out.Quest.bExists,Project);
        if (Out.Quest.bExists)
        {
            if (SavedHash != Hash) return Fail(Error,Q->QuestId.Value,TEXT("authored quest changed after acceptance; explicit save/content migration required"));
            if (Tick < Out.Quest.UpdatedWorldTick) return Fail(Error,Q->QuestId.Value,TEXT("command precedes saved quest history"));
            if (HasSubject && Out.Quest.SubjectId != C.SubjectId) Out.MissingRequirements.Add(TEXT("Quest commands must keep the originally accepted subject."));
            if (Out.Quest.CreditedEvents.Num() != Q->Objectives.Num() || Out.Quest.AcceptanceBaseline.Num() != Q->Objectives.Num()) return Fail(Error,Q->QuestId.Value,TEXT("saved objective/baseline set differs from authored content"));
            for (const auto& O : Q->Objectives)
            {
                const auto* Credits = Out.Quest.CreditedEvents.Find(O.ObjectiveId);
                if (!Credits || Credits->Num() > O.RequiredCount || !Out.Quest.AcceptanceBaseline.Contains(O.ObjectiveId)) return Fail(Error,Q->QuestId.Value,TEXT("invalid saved objective credits/baseline"));
            }
        }
        if (Command.Kind == EOGNarrativeCommand::AcceptQuest)
        {
            if (Out.Quest.bExists) Out.MissingRequirements.Add(TEXT("This quest was already accepted and cannot be restarted."));
            else
            {
                Out.Quest.bExists = true; Out.Quest.SubjectId = HasSubject ? C.SubjectId : FOGEntityId();
                Out.Quest.StartedWorldTick = Tick;
                for (const auto& O : Q->Objectives) Out.Quest.CreditedEvents.Add(O.ObjectiveId,TArray<FOGEntityId>());
                if (!AcceptanceBaseline(Store,*Q,C,Tick,Out.Quest,Stamp,Error)) return false;
            }
            if (!Conditions(Store,Q->Conditions,C,Locks,Tick,Stamp,Out.MissingRequirements,Error)) return false;
            Effects = &Q->AcceptEffects;
        }
        else
        {
            if (!Out.Quest.bExists || Out.Quest.bCompleted) Out.MissingRequirements.Add(TEXT("An active accepted quest is required."));
            if (Command.Kind == EOGNarrativeCommand::AdvanceQuest)
            {
                int32 Added = 0;
                for (const auto& Id : Command.EvidenceEventIds)
                {
                    bool Found = false; FOGWorldEvent E;
                    if (!Store.TryReadWorldEvent(Id,Found,E,Error)) return false;
                    if (!Found || E.WorldTick < 0 || E.WorldTick > Tick) return Fail(Error,TEXT("evidence"),TEXT("objective evidence must already exist in the canonical ledger at this tick"));
                    StampEvent(Stamp,E);
                    if (!Out.Quest.bExists || Out.Quest.bCompleted) continue;
                    for (const auto& O : Q->Objectives)
                    {
                        const auto Subject = O.Event.bSubjectIsRuler ? C.RulerId : Out.Quest.SubjectId;
                        auto& Credits = Out.Quest.CreditedEvents.FindOrAdd(O.ObjectiveId);
                        if (E.EventType == O.Event.EventType && AppliesTo(E,Subject) &&
                            (O.bAllowHistoricalEvents || E.WorldTick > Out.Quest.StartedWorldTick ||
                                (E.WorldTick == Out.Quest.StartedWorldTick && !Out.Quest.AcceptanceBaseline.FindRef(O.ObjectiveId).Contains(E.EventId))) &&
                            Credits.Num() < O.RequiredCount && !Credits.Contains(E.EventId)) { Credits.Add(E.EventId); ++Added; }
                    }
                }
                if (Added == 0) Out.MissingRequirements.Add(TEXT("No new canonical objective evidence matched this accepted quest."));
            }
            else
            {
                for (const auto& O : Q->Objectives)
                {
                    const auto* Credits = Out.Quest.CreditedEvents.Find(O.ObjectiveId);
                    if (!Credits || Credits->Num() < O.RequiredCount) Out.MissingRequirements.Add(TEXT("Incomplete objective: ") + O.DisplayName);
                }
                if (!Conditions(Store,Q->Conditions,C,Locks,Tick,Stamp,Out.MissingRequirements,Error)) return false;
                Effects = &Q->CompletionEffects; Out.Quest.bCompleted = true;
            }
        }
        Out.Quest.UpdatedWorldTick = Tick;
    }
    else
    {
        const FString Hash = DefinitionHash(*D); Part(Stamp,Hash); Out.DisplayName = D->DisplayName;
        if (!LoadDialogue(Store,C,D->DialogueId,Out.Dialogue,Project,SavedHash,Error)) return false;
        StampProject(Stamp,Out.Dialogue.bExists,Project);
        if (Out.Dialogue.bExists && SavedHash != Hash) return Fail(Error,D->DialogueId.Value,TEXT("authored dialogue changed after start; explicit save/content migration required"));
        if (Out.Dialogue.bExists && Tick < Out.Dialogue.UpdatedWorldTick) return Fail(Error,D->DialogueId.Value,TEXT("command precedes saved conversation history"));
        if (!Conditions(Store,D->Conditions,C,Locks,Tick,Stamp,Out.MissingRequirements,Error)) return false;
        if (Command.Kind == EOGNarrativeCommand::StartDialogue)
        {
            if (Out.Dialogue.bExists) Out.MissingRequirements.Add(TEXT("This subject-bound conversation was already started and cannot restart."));
            else
            {
                Out.Dialogue.bExists = true; Out.Dialogue.NodeId = D->EntryNodeId;
                const auto* Entry = D->Nodes.FindByPredicate([&](const auto& N){return N.NodeId == D->EntryNodeId;});
                if (!Entry) return Fail(Error,D->DialogueId.Value,TEXT("missing entry node"));
                Out.Dialogue.bCompleted = Entry->bTerminal;
            }
        }
        else
        {
            if (!Out.Dialogue.bExists || Out.Dialogue.bCompleted) Out.MissingRequirements.Add(TEXT("An active conversation at this subject is required."));
            const auto* Node = D->Nodes.FindByPredicate([&](const auto& N){return N.NodeId == Out.Dialogue.NodeId;});
            if (Out.Dialogue.bExists && (!Node || Node->bTerminal != Out.Dialogue.bCompleted)) return Fail(Error,D->DialogueId.Value,TEXT("saved conversation node/status mismatch"));
            const auto* Choice = Node ? Node->Choices.FindByPredicate([&](const auto& V){return V.ChoiceId == Command.ChoiceId;}) : nullptr;
            if (!Choice) Out.MissingRequirements.Add(TEXT("This authored choice is not available at the current node."));
            else
            {
                if (!Conditions(Store,Choice->Conditions,C,Locks,Tick,Stamp,Out.MissingRequirements,Error)) return false;
                Effects = &Choice->Effects; Out.Dialogue.NodeId = Choice->NextNodeId;
                const auto* Next = D->Nodes.FindByPredicate([&](const auto& N){return N.NodeId == Choice->NextNodeId;});
                if (!Next) return Fail(Error,D->DialogueId.Value,TEXT("missing authored destination node"));
                Out.Dialogue.bCompleted = Next->bTerminal;
            }
        }
        Out.Dialogue.UpdatedWorldTick = Tick;
    }
    if (!Resources(Store,C.RulerId,*Effects,Locks,Stamp,Out,Error)) return false;
    Out.ConfirmationToken = Digest(Stamp); Out.bCanApply = Out.MissingRequirements.IsEmpty(); return true;
}

bool FOGNarrativeService::Apply(const FOGNarrativeCatalog& Catalog, const FOGNarrativeCommand& Command,
    const FString& Token, const FOGEntityId& RequestId, int64 Tick, bool& Already, FString& Error)
{
    Already = false; Error.Reset();
    if (!Store.IsOpen() || !RequestId.IsValid() || !Command.Context.RulerId.IsValid() || Token.IsEmpty() || Tick < 0)
        return Fail(Error,TEXT("narrative"),TEXT("reviewed request ID, confirmation token and valid ruler/tick required"));
    FTransaction Transaction(Store,Error); if (!Transaction.Begin()) return false;
    bool Found = false; FOGWorldEvent Existing;
    if (!Store.TryReadWorldEvent(RequestId,Found,Existing,Error)) return false;
    if (Found)
    {
        FObject Receipt; FString Hash, Confirmed;
        if (Existing.EventType != ReceiptType || Existing.PrimaryEntity != Command.Context.RulerId ||
            !ReadObject(Existing.PayloadJson,Receipt,Error) ||
            !String(Receipt,TEXT("command_hash"),Hash,TEXT("receipt"),Error) || Hash != CommandHash(Command) ||
            !String(Receipt,TEXT("confirmation_token"),Confirmed,TEXT("receipt"),Error) || Confirmed != Token)
            return Fail(Error,TEXT("request_id"),TEXT("already belongs to a different canonical command"));
        if (!Transaction.Commit()) return false;
        Already = true; return true;
    }
    FOGNarrativePreview Quote;
    if (!Preview(Catalog,Command,Tick,Quote,Error)) return false;
    if (!Quote.bCanApply || Quote.ConfirmationToken != Token)
        return Fail(Error,TEXT("narrative"),Quote.bCanApply ? TEXT("canonical state, resources or authored content changed; review again") : FString::Join(Quote.MissingRequirements,TEXT("\n")));
    const auto& C = Command.Context;
    const bool IsQuest = Command.Kind == EOGNarrativeCommand::AcceptQuest || Command.Kind == EOGNarrativeCommand::AdvanceQuest || Command.Kind == EOGNarrativeCommand::CompleteQuest;
    const auto* Q = IsQuest ? Catalog.Quests.FindByPredicate([&](const auto& V){return V.QuestId == Command.ContentId;}) : nullptr;
    const auto* D = IsQuest ? nullptr : Catalog.Dialogues.FindByPredicate([&](const auto& V){return V.DialogueId == Command.ContentId;});
    if ((IsQuest && !Q) || (!IsQuest && !D)) return Fail(Error,TEXT("narrative"),TEXT("validated definition disappeared"));
    FOGNarrativeEffects None; const FOGNarrativeEffects* Effects = &None;
    if (Command.Kind == EOGNarrativeCommand::AcceptQuest) Effects = &Q->AcceptEffects;
    else if (Command.Kind == EOGNarrativeCommand::CompleteQuest) Effects = &Q->CompletionEffects;
    else if (Command.Kind == EOGNarrativeCommand::ChooseDialogue)
    {
        for (const auto& N : D->Nodes)
            for (const auto& Choice : N.Choices)
                if (Choice.ChoiceId == Command.ChoiceId) Effects = &Choice.Effects;
    }
    // Validate all quoted arithmetic before writing any balance; a late error
    // (including event receipt collision) rolls back every project and resource.
    for (const auto& R : Quote.Resources)
        if (!Store.SetResourceBalance(C.RulerId,R.ResourceId,R.BalanceAfter,Error)) return false;
    if (Q)
    {
        const auto& State = Quote.Quest; auto Root = Base(Q->QuestId,State.SubjectId,DefinitionHash(*Q),State.bCompleted);
        auto Objectives = MakeShared<FJsonObject>(); auto Baseline = MakeShared<FJsonObject>(); int64 Required = 0, Completed = 0;
        for (const auto& O : Q->Objectives)
        {
            const auto* Credits = State.CreditedEvents.Find(O.ObjectiveId);
            if (!Credits) return Fail(Error,TEXT("quest"),TEXT("missing validated objective state"));
            TArray<FString> Ids; for (const auto& Id : *Credits) Ids.Add(Id.ToString()); Ids.Sort();
            FArray Values; for (const auto& Id : Ids) Values.Add(MakeShared<FJsonValueString>(Id));
            Objectives->SetArrayField(O.ObjectiveId.Value,Values); Required += O.RequiredCount; Completed += Credits->Num();
            TArray<FString> Before; for (const auto& Id : State.AcceptanceBaseline.FindRef(O.ObjectiveId)) Before.Add(Id.ToString()); Before.Sort();
            FArray Prior; for (const auto& Id : Before) Prior.Add(MakeShared<FJsonValueString>(Id));
            Baseline->SetArrayField(O.ObjectiveId.Value,Prior);
        }
        Root->SetObjectField(TEXT("objectives"),Objectives);
        Root->SetObjectField(TEXT("acceptance_baseline"),Baseline);
        const int32 Progress = State.bCompleted ? 10000 : static_cast<int32>(Completed * 10000 / Required);
        if (!Save(Store,C,TEXT("quest"),Q->QuestId,FOGEntityId(),Root,Tick,State.bCompleted,Progress,Error)) return false;
    }
    else
    {
        auto Root = Base(D->DialogueId,C.SubjectId,DefinitionHash(*D),Quote.Dialogue.bCompleted);
        Root->SetStringField(TEXT("node_id"),Quote.Dialogue.NodeId.Value);
        if (!Save(Store,C,TEXT("dialogue"),D->DialogueId,C.SubjectId,Root,Tick,Quote.Dialogue.bCompleted,Quote.Dialogue.bCompleted ? 10000 : 0,Error)) return false;
    }
    if (!Effects->BranchLocks.IsEmpty())
    {
        FLocks Locks; bool Exists = false; FOGProjectRecord P;
        if (!LoadLocks(Store,C.RulerId,Locks,Exists,P,Error)) return false;
        for (const auto& Lock : Effects->BranchLocks)
        {
            if (const auto* Previous = Locks.Find(Lock.BranchId))
                if (*Previous != Lock.ValueId) return Fail(Error,TEXT("branch"),TEXT("cannot overwrite a prior choice"));
            Locks.Add(Lock.BranchId,Lock.ValueId);
        }
        if (Locks.Num() > 4096) return Fail(Error,TEXT("branches"),TEXT("narrative lock map exceeds supported capacity"));
        auto Root = MakeShared<FJsonObject>(); Root->SetStringField(TEXT("schema"),StateSchema);
        auto Values = MakeShared<FJsonObject>(); TArray<FOGContentId> Keys; Locks.GetKeys(Keys);
        Keys.Sort([](const auto& A,const auto& B){return A.Value < B.Value;});
        for (const auto& Key : Keys) Values->SetStringField(Key.Value,Locks.FindRef(Key).Value);
        Root->SetObjectField(TEXT("locks"),Values);
        FOGNarrativeContext LedgerContext = C;
        if (!Exists && Q)
        {
            // Remote turn-in keeps the accepted quest's existing location;
            // it does not create a new canonical observation or move anyone.
            FOGProjectRecord Accepted; bool AcceptedFound = false;
            if (!Store.TryReadProject(ProjectId(C.RulerId,TEXT("quest"),Command.ContentId),AcceptedFound,Accepted,Error)) return false;
            if (!AcceptedFound) return Fail(Error,TEXT("branches"),TEXT("no accepted project location for remote completion"));
            LedgerContext.LocationId = Accepted.LocationId;
        }
        if (!Save(Store,LedgerContext,TEXT("branches"),BranchContent,FOGEntityId(),Root,Tick,false,0,Error)) return false;
    }
    auto Receipt = MakeShared<FJsonObject>(); Receipt->SetStringField(TEXT("schema"),TEXT("noworld.narrative_receipt.v1"));
    Receipt->SetStringField(TEXT("command_hash"),CommandHash(Command)); Receipt->SetStringField(TEXT("confirmation_token"),Token);
    Receipt->SetStringField(TEXT("content_id"),Command.ContentId.Value); Receipt->SetStringField(TEXT("choice_id"),Command.ChoiceId.Value);
    Receipt->SetStringField(TEXT("kind"),LexToString(static_cast<int32>(Command.Kind)));
    FArray Changes;
    for (const auto& R : Quote.Resources)
    {
        auto Change = MakeShared<FJsonObject>(); Change->SetStringField(TEXT("resource_id"),R.ResourceId.Value);
        Change->SetStringField(TEXT("cost"),LexToString(R.Cost)); Change->SetStringField(TEXT("reward"),LexToString(R.Reward));
        Change->SetStringField(TEXT("balance_before"),LexToString(R.BalanceBefore)); Change->SetStringField(TEXT("balance_after"),LexToString(R.BalanceAfter));
        Changes.Add(MakeShared<FJsonValueObject>(Change));
    }
    Receipt->SetArrayField(TEXT("resources"),Changes);
    FOGWorldEvent Event; Event.EventId = RequestId; Event.EventType = ReceiptType; Event.PrimaryEntity = C.RulerId;
    if (C.SubjectId.IsValid()) Event.RelatedEntities.Add(C.SubjectId);
    if (C.InteractorEntityId.IsValid() && C.InteractorEntityId != C.RulerId) Event.RelatedEntities.AddUnique(C.InteractorEntityId);
    Event.WorldTick = Tick; Event.PayloadJson = Write(Receipt); Event.bChronicleEligible = true;
    if (!Store.AppendWorldEvent(Event,Error)) return false;
    return Transaction.Commit();
}

bool FOGNarrativeService::GetConversation(const FOGNarrativeCatalog& Catalog, const FOGNarrativeContext& C,
    const FOGContentId& Id, int64 Tick, FOGNarrativeConversationView& Out, FString& Error) const
{
    Out = FOGNarrativeConversationView(); Error.Reset();
    if (Tick < 0 || !FOGNarrativeCatalog::Validate(Catalog,Error)) return false;
    const auto* D = Catalog.Dialogues.FindByPredicate([&](const auto& V){return V.DialogueId == Id;});
    if (!D) return Fail(Error,TEXT("dialogue"),TEXT("unknown authored conversation"));
    FString Stamp; TArray<FString> Missing;
    if (!ContextCheck(Store,C,true,Tick,Stamp,Missing,Error) || !SubjectCheck(Store,D->Subject,C,true,Tick,Stamp,Missing,Error)) return false;
    FLocks Locks; bool LockFound = false; FOGProjectRecord BranchProject;
    if (!LoadLocks(Store,C.RulerId,Locks,LockFound,BranchProject,Error) ||
        !Conditions(Store,D->Conditions,C,Locks,Tick,Stamp,Missing,Error)) return false;
    if (!Missing.IsEmpty()) return Fail(Error,TEXT("dialogue"),FString::Join(Missing,TEXT("\n")));
    FOGProjectRecord P; FString Hash;
    if (!LoadDialogue(Store,C,Id,Out.State,P,Hash,Error)) return false;
    if (Out.State.bExists && Hash != DefinitionHash(*D)) return Fail(Error,TEXT("dialogue"),TEXT("content changed; explicit save migration required"));
    const auto NodeId = Out.State.bExists ? Out.State.NodeId : D->EntryNodeId;
    const auto* N = D->Nodes.FindByPredicate([&](const auto& V){return V.NodeId == NodeId;});
    if (!N) return Fail(Error,TEXT("dialogue"),TEXT("saved node is missing"));
    if (Out.State.bExists && (Out.State.bCompleted != N->bTerminal || Tick < Out.State.UpdatedWorldTick))
        return Fail(Error,TEXT("dialogue"),TEXT("saved node/status or world tick mismatch"));
    Out.DisplayName = D->DisplayName; Out.Text = N->Text;
    if (!Out.State.bExists || Out.State.bCompleted) return true; // Reading never starts or advances a scene.
    for (const auto& Choice : N->Choices)
    {
        FOGNarrativeCommand Command; Command.Context = C; Command.Kind = EOGNarrativeCommand::ChooseDialogue;
        Command.ContentId = Id; Command.ChoiceId = Choice.ChoiceId; FOGNarrativePreview Quote;
        if (!Preview(Catalog,Command,Tick,Quote,Error)) return false;
        FOGNarrativeChoiceView V; V.ChoiceId = Choice.ChoiceId; V.Text = Choice.Text;
        V.bEligible = Quote.bCanApply; V.ConfirmationToken = Quote.ConfirmationToken; V.MissingRequirements = Quote.MissingRequirements;
        Out.Choices.Add(MoveTemp(V));
    }
    return true;
}
