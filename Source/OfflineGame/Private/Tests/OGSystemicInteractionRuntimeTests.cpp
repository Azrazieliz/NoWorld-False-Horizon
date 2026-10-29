#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/OGSystemicInteractionRuntime.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "Runtime/OGCanonicalClockRuntime.h"
#include "Components/BoxComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <limits>

namespace
{
struct FInteractionFixture
{
    FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
    FString DatabasePath = FPaths::Combine(Directory, TEXT("interaction.db"));
    FOGSQLiteWorldStore Store;
    FOGEntityId Ruler = FOGEntityId::NewId();
    FOGEntityId Location = FOGEntityId::NewId();
    FOGEntityId ContextEvent = FOGEntityId::NewId();
    FOGEntityId Memory = FOGEntityId::NewId();
    FOGInteractionEntry Entry;
    FString Error;
    bool Open(int32 Count = 13)
    {
        if (!Store.Open(DatabasePath, Error)) return false;
        FOGLocationRecord Place; Place.LocationId = Location; Place.Kind = TEXT("test_context");
        if (!Store.UpsertLocation(Place, 0, Error)) return false;
        FOGWorldEvent Context; Context.EventId = ContextEvent;
        Context.EventType = TEXT("test.authored_context"); Context.PrimaryEntity = Ruler;
        Context.PayloadJson = TEXT("{\"authored\":true}");
        Entry.Context = TEXT("authored_extension"); Entry.ContextEventId = ContextEvent;
        Entry.LocationId = Location;
        for (int32 Index = 0; Index < Count; ++Index)
        {
            FOGInteractionParticipant Participant;
            Participant.Character.EntityId = Index == 0 ? Ruler : FOGEntityId::NewId();
            Participant.Character.OwnerEntityId = Participant.Character.EntityId;
            Participant.Role = Index == 0 ? FName(TEXT("player")) : FName(TEXT("participant"));
            Participant.RigFamilyId = FOGContentId(TEXT("test:rig.neutral"));
            if (!Store.UpsertEntity(Participant.Character.EntityId,
                Index == 0 ? FName(TEXT("ruler")) : FName(TEXT("npc")), 0,
                TEXT("{\"history\":\"retained\",\"progression\":17}"), Error)) return false;
            FOGWorldPresenceRecord Presence; Presence.EntityId = Participant.Character.EntityId;
            Presence.LocationId = Location; Presence.MovementContext = TEXT("ground");
            Presence.LocalPosition = FVector(Index * 20.0, 10, 0);
            if (!Store.UpsertWorldPresence(Presence, Error)) return false;
            Entry.Participants.Add(Participant);
        }
        FOGEntityClassRecord Class; Class.OwnerEntityId = Ruler;
        Class.ClassId = FOGContentId(TEXT("test:class.neutral"));
        Class.RecognizedWorldTick = 2; Class.UpdatedWorldTick = 3;
        Class.StateJson = TEXT("{\"mastery\":\"retained\"}");
        if (!Store.UpsertEntityClass(Class, Error)) return false;
        return Store.AppendWorldEvent(Context, Error) &&
            Store.SetResourceBalance(Ruler, FOGContentId(TEXT("test:resource.unrelated")), 29, Error);
    }
    FOGInteractionActionGraph GraphWithConsequences() const
    {
        auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
        FOGKnowledgeFactRecord Fact; Fact.OwnerEntityId = Ruler;
        Fact.SubjectEntityId = Entry.Participants.Last().Character.EntityId;
        Fact.FactKey = TEXT("neutral_handoff_observed"); Fact.SourceEntityId = Fact.SubjectEntityId;
        Fact.ValueJson = TEXT("{\"observed\":\"neutral_handoff\"}");
        Fact.LearnedWorldTick = 5;
        Graph.Nodes[0].Consequences.Knowledge.Add(Fact);
        FOGSemanticMemoryRecord Record; Record.MemoryId = Memory; Record.OwnerEntityId = Ruler;
        Record.SubjectEntityId = Fact.SubjectEntityId;
        Record.MemoryTypeId = FOGContentId(TEXT("test:memory.neutral_handoff"));
        Record.SalienceBps = 5000; Record.StateJson = TEXT("{\"authored\":\"handoff\"}");
        Graph.Nodes[0].Consequences.Memories.Add(Record);
        FOGWorldPresenceRecord Presence; Presence.EntityId = Ruler;
        Presence.LocationId = Location; Presence.LocalPosition = FVector(50, 60, 0);
        Presence.MovementContext = TEXT("ground");
        Graph.Nodes[0].Consequences.Presence.Add(Presence);
        return Graph;
    }
    bool Events(TArray<FOGWorldEvent>& Out)
    {
        if (!Store.ListWorldEvents(Ruler, NAME_None, false, 1000, Out, Error)) return false;
        Out.RemoveAll([](const FOGWorldEvent& Event)
            { return !Event.EventType.ToString().StartsWith(TEXT("interaction.")); });
        return true;
    }
    ~FInteractionFixture()
    {
        Store.Close(); IFileManager::Get().DeleteDirectory(*Directory, false, true);
    }
};

struct FInteractionPresentationProbe : IOGSystemicInteractionPresentation
{
    TSet<FOGEntityId> Projected;
    TSet<FOGEntityId> Significant;
    int32 CharacterCalls = 0;
    int32 RestoreCalls = 0;
    int32 ConstraintCalls = 0;
    bool bPrivacy = false;
    bool bPrepared=true,bNativeReady=true;
    TArray<double> PresentationTimes;
    virtual bool PrepareActionPresentation(const FOGInteractionActionNode&,const TArray<FOGInteractionParticipantState>&,bool,FString& Error) override
    { if (!bPrepared) Error=TEXT("Source adapter unavailable"); return bPrepared; }
    virtual bool AdvanceActionPresentation(const FOGInteractionActionNode&,const TArray<FOGInteractionParticipantState>&,double Time,bool,FString& Error) override
    { PresentationTimes.Add(Time); if (!bNativeReady) Error=TEXT("Source adapter interrupted"); return bNativeReady; }
    TFunction<void()> OnCharacter;
    int32 StopCalls = 0;
    virtual void ApplyCharacter(const FOGInteractionParticipantState& State) override
    { ++CharacterCalls; Projected.Add(State.Binding.Character.EntityId); if (OnCharacter) OnCharacter(); }
    virtual void SetSignificance(const FOGEntityId& Entity, bool, bool) override
    { Significant.Add(Entity); }
    virtual void ApplyConstraint(const FOGInteractionConstraint&,
        const TArray<FOGInteractionParticipantState>&) override { ++ConstraintCalls; }
    virtual void ApplyAnimation(const FOGEntityId&, UObject*) override {}
    virtual void ApplyReaction(const FOGEntityId&, UObject*, UObject*) override {}
    virtual void SetPrivacy(bool bValue) override { bPrivacy = bValue; }
    virtual void StopSharedScene() override { ++StopCalls; }
    virtual void RestoreGameplay() override { ++RestoreCalls; }
};

TSharedPtr<FJsonObject> Payload(const FOGWorldEvent& Event)
{
    TSharedPtr<FJsonObject> Object;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Event.PayloadJson), Object);
    return Object;
}
FString WithoutGeneratedSessionId(const FOGWorldEvent& Event)
{
    const auto Object = Payload(Event);
    if (!Object.IsValid()) return TEXT("invalid JSON");
    // Only this runtime-generated identifier is excluded. Every authored field,
    // participant binding, context, reason and payload field remains compared.
    Object->RemoveField(TEXT("session_id"));
    FString Json; FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
    return Json;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSystemicInteractionBindingsTest,
    "OfflineGame.Foundation.Interaction.CanonicalRulerVariableArityAndGraphReferences",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGSystemicInteractionBindingsTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open shared canonical fixture"), F.Open())) return false;
    auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    FInteractionPresentationProbe Presentation;
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    Runtime.SetPresentationBridge(&Presentation);
    auto RejectEntry = [&](const TCHAR* Label, const FOGInteractionEntry& Entry,
        const FOGInteractionActionGraph& Candidate)
    {
        TestFalse(Label, Runtime.Enter(Entry, Candidate, 5, F.Error));
        TestFalse(TEXT("Rejected entry stays inactive"), Runtime.IsActive());
        TestEqual(TEXT("Rejected entry leaves no logical participants"), Runtime.GetParticipants().Num(), 0);
        TestEqual(TEXT("Rejected entry cannot take character presentation ownership"), Presentation.CharacterCalls, 0);
    };
    auto Bad = F.Entry; Bad.Participants[0].Role = TEXT("participant");
    RejectEntry(TEXT("Ruler cannot be a nonplayer participant"), Bad, Graph);
    Bad = F.Entry; Bad.Participants[1].Role = TEXT("player");
    RejectEntry(TEXT("Non-Ruler cannot take the physical player role"), Bad, Graph);
    Bad = F.Entry; Bad.Participants.RemoveAt(0);
    RejectEntry(TEXT("Missing canonical player rejected"), Bad, Graph);
    Bad = F.Entry; Bad.Participants.Add(F.Entry.Participants[0]);
    RejectEntry(TEXT("Duplicate canonical player rejected"), Bad, Graph);
    Bad = F.Entry; Bad.Participants[2].Character.EntityId = Bad.Participants[1].Character.EntityId;
    RejectEntry(TEXT("Duplicate persistent participant rejected"), Bad, Graph);
    auto Broken = Graph; Broken.StartNode = TEXT("absent");
    RejectEntry(TEXT("Missing graph start rejected"), F.Entry, Broken);
    Broken = Graph; Broken.Nodes[0].NextNodes.Add(TEXT("absent"));
    RejectEntry(TEXT("Missing graph transition rejected"), F.Entry, Broken);
    Broken = Graph; Broken.Nodes[0].RequiredRoles.Add(TEXT("absent_role"));
    RejectEntry(TEXT("Unbound action role rejected"), F.Entry, Broken);
    Broken = Graph;
    const FOGInteractionActionNode DuplicateNode = Broken.Nodes[0];
    Broken.Nodes.Add(DuplicateNode);
    RejectEntry(TEXT("Duplicate action node rejected"), F.Entry, Broken);
    FOGInteractionConstraint Constraint;
    Constraint.ParticipantRole = TEXT("player"); Constraint.TargetRole = TEXT("absent_role");
    Broken = Graph; Broken.Nodes[0].Constraints.Add(Constraint);
    RejectEntry(TEXT("Unbound constraint target rejected"), F.Entry, Broken);
    Constraint.TargetRole = TEXT("participant");
    Constraint.TargetOffset.X = std::numeric_limits<double>::quiet_NaN();
    Broken = Graph; Broken.Nodes[0].Constraints.Add(Constraint);
    RejectEntry(TEXT("Nonfinite constraint placement rejected"), F.Entry, Broken);
    FOGInteractionAnimationBinding Animation;
    Animation.Role = TEXT("absent_role"); Animation.RigFamilyId = FOGContentId(TEXT("test:rig.neutral"));
    Broken = Graph; Broken.Nodes[0].AnimationBindings.Add(Animation);
    RejectEntry(TEXT("Unbound animation role rejected"), F.Entry, Broken);
    TArray<FOGWorldEvent> Events;
    TestTrue(TEXT("Read events after rejections"), F.Events(Events));
    TestEqual(TEXT("Invalid bindings and graphs create no interaction event"), Events.Num(), 0);

    Constraint.TargetOffset = FVector(10, 0, 0);
    Graph.Nodes[0].Constraints.Add(Constraint);
    Animation.Role = TEXT("participant");
    // Empty optional assets are intentional; logical action validity is independent.
    Graph.Nodes[0].AnimationBindings.Add(Animation);
    if (!TestTrue(TEXT("Thirteen persistent participants bind without an architectural cap"),
        Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    TestEqual(TEXT("Every logical participant retained"), Runtime.GetParticipants().Num(), 13);
    TestEqual(TEXT("All current character projections reach the ordinary bridge"), Presentation.Projected.Num(), 13);
    TestEqual(TEXT("Significance does not drop logical participants"), Presentation.Significant.Num(), 13);
    TestTrue(TEXT("Bound constraints reach the presentation extension"), Presentation.ConstraintCalls > 0);
    int32 Players = 0;
    for (int32 Index = 0; Index < Runtime.GetParticipants().Num(); ++Index)
    {
        const auto& State = Runtime.GetParticipants()[Index];
        TestTrue(TEXT("Participant order and canonical identity remain bound"),
            State.Binding.Character.EntityId == F.Entry.Participants[Index].Character.EntityId);
        TestTrue(TEXT("Current canonical history reaches presentation"), State.Character.EntityStateJson.Contains(TEXT("retained")));
        TestFalse(TEXT("Optional presentation absence is reported safely"), State.bHasOptionalPresentation);
        if (State.Binding.Role == TEXT("player"))
        { ++Players; TestTrue(TEXT("Only canonical Ruler is player"), State.Binding.Character.EntityId == F.Ruler); }
    }
    TestEqual(TEXT("Exactly one Ruler player role"), Players, 1);
    TestTrue(TEXT("Read accepted entry event"), F.Events(Events));
    if (TestEqual(TEXT("One accepted entry event"), Events.Num(), 1))
    {
        const auto Object = Payload(Events[0]);
        if (TestTrue(TEXT("Entry payload is structured"), Object.IsValid()))
        {
            TestEqual(TEXT("Extension context is provenance rather than a whitelist"),
                Object->GetStringField(TEXT("context")), F.Entry.Context.ToString());
            TestEqual(TEXT("Authored context event retained"), Object->GetStringField(TEXT("context_event_id")), F.ContextEvent.ToString());
            TestEqual(TEXT("Authored location retained"), Object->GetStringField(TEXT("location_id")), F.Location.ToString());
            TestEqual(TEXT("All bindings persist in canonical payload"), Object->GetArrayField(TEXT("participants")).Num(), 13);
        }
    }
    TestTrue(TEXT("Optional absence permits action completion"), Runtime.Advance(1, 6, F.Error));
    TestTrue(TEXT("Optional absence permits ordinary exit"), Runtime.Exit(TEXT("player_exit"), 7, F.Error));
    TestEqual(TEXT("Bridge restored once on native exit"), Presentation.RestoreCalls, 1);
    Runtime.SetPresentationBridge(nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSystemicInteractionClockPersistenceTest,
    "OfflineGame.Foundation.Interaction.ActionsSequenceZeroPacingAndSharedClockPersistence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGSystemicInteractionClockPersistenceTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open canonical fixture"), F.Open())) return false;
    FOGCanonicalClockPolicy Policy;
    Policy.PolicyId = TEXT("test.interaction_clock"); Policy.Version = 1;
    Policy.RateDenominator = 1000; Policy.WorldRateNumerator = 2;
    Policy.RulerRateNumerator = 1; Policy.OfflineRateNumerator = 1;
    Policy.TurnLocalRateDivisor = 2; Policy.MaxDueActionsPerPump = 4;
    Policy.CalendarId = FOGContentId(TEXT("test:calendar.interaction"));
    Policy.LocalTicksPerDay = 10; Policy.DaysPerMonth = 3;
    FOGCanonicalClockRuntime Clock(F.Store);
    if (!TestTrue(TEXT("Existing canonical clock owns the same Store"), Clock.LoadOrCreate(Policy, 1000, F.Error))) return false;
    Clock.ResetOnlineAnchor(10);
    {
        FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
        const auto Graph = F.GraphWithConsequences();
        if (!TestTrue(TEXT("Enter neutral authored action graph"), Runtime.Enter(F.Entry, Graph, Clock.GetCanonicalWorldTick(), F.Error))) return false;
        TestFalse(TEXT("Cannot replace an unfinished action"), Runtime.SelectAction(TEXT("handoff"), F.Error));
        TestFalse(TEXT("Unknown sequence node rejected atomically"), Runtime.SetSequence({FName(TEXT("handoff")), FName(TEXT("absent"))}, F.Error));
        TestFalse(TEXT("Negative pacing rejected"), Runtime.SetPacing(-1, F.Error));
        TestFalse(TEXT("Nonfinite pacing rejected"), Runtime.SetPacing(std::numeric_limits<double>::infinity(), F.Error));
        TestTrue(TEXT("Player can pause action pacing"), Runtime.SetPacing(0, F.Error));
        const int64 Before = Clock.GetCanonicalWorldTick();
        TestTrue(TEXT("Canonical Core clock pump is independent of action pacing"), Clock.PumpOnline(13, 4000, F.Error));
        TestTrue(TEXT("Canonical world advances during zero pacing"), Clock.GetCanonicalWorldTick() > Before);
        TestTrue(TEXT("Paused action accepts independently advanced canonical tick"), Runtime.Advance(100, Clock.GetCanonicalWorldTick(), F.Error));
        TestEqual(TEXT("Zero pacing leaves action uncompleted"), Runtime.GetCurrentNode(), FName(TEXT("align")));
        TArray<FOGWorldEvent> Events; TestTrue(TEXT("Read paused interaction events"), F.Events(Events));
        TestEqual(TEXT("Pause cannot manufacture completed-action events"), Events.Num(), 1);
        TestTrue(TEXT("Valid sequence may be chosen by player"), Runtime.SetSequence({FName(TEXT("handoff")), FName(TEXT("align"))}, F.Error));
        TestTrue(TEXT("Player resumes pacing"), Runtime.SetPacing(1, F.Error));
        TestTrue(TEXT("Authored consequence commits at canonical clock tick"), Runtime.Advance(1, Clock.GetCanonicalWorldTick(), F.Error));
        TestEqual(TEXT("Queued graph transition begins handoff"), Runtime.GetCurrentNode(), FName(TEXT("handoff")));
        TestTrue(TEXT("Independent clock continues during handoff"), Clock.PumpOnline(14, 5000, F.Error));
        TestTrue(TEXT("Handoff completes and next player-selected action begins"), Runtime.Advance(2, Clock.GetCanonicalWorldTick(), F.Error));
        TestEqual(TEXT("Sequence returns to align"), Runtime.GetCurrentNode(), FName(TEXT("align")));
        TestTrue(TEXT("Safe interruption persists on shared clock"), Runtime.Exit(TEXT("application_deactivated"), Clock.GetCanonicalWorldTick(), F.Error));
    }
    F.Store.Close();
    if (!TestTrue(TEXT("Reopen same canonical database"), F.Store.Open(F.DatabasePath, F.Error))) return false;
    TArray<FOGWorldEvent> Events; TestTrue(TEXT("Read persisted enter, two completions and exit"), F.Events(Events));
    TestEqual(TEXT("No replay store or invented action event"), Events.Num(), 4);
    int32 Entered = 0, Completed = 0, Exited = 0;
    FOGEntityId ConsequenceEvent;
    for (const auto& Event : Events)
    {
        if (Event.EventType == TEXT("interaction.entered")) ++Entered;
        if (Event.EventType == TEXT("interaction.action_completed"))
        {
            ++Completed;
            if (Payload(Event)->GetStringField(TEXT("reason")) == TEXT("align")) ConsequenceEvent = Event.EventId;
        }
        if (Event.EventType == TEXT("interaction.exited"))
        { ++Exited; TestEqual(TEXT("Interruption reason persists"), Payload(Event)->GetStringField(TEXT("reason")), FString(TEXT("application_deactivated"))); }
    }
    TestEqual(TEXT("Exactly one canonical entry"), Entered, 1);
    TestEqual(TEXT("Exactly two completed actions"), Completed, 2);
    TestEqual(TEXT("Exactly one canonical exit"), Exited, 1);
    bool Found = false; FOGKnowledgeFactRecord Fact;
    TestTrue(TEXT("Read causally authored knowledge after restart"), F.Store.TryReadKnowledgeFact(F.Ruler,
        TEXT("neutral_handoff_observed"), F.Entry.Participants.Last().Character.EntityId, Found, Fact, F.Error));
    TestTrue(TEXT("Authored knowledge survives restart"), Found);
    TestTrue(TEXT("Knowledge source is the actual completed-action event"), Fact.SourceEventId == ConsequenceEvent);
    TestEqual(TEXT("Consequence uses independently committed world tick"), Fact.UpdatedWorldTick, int64(6));
    FOGSemanticMemoryRecord Memory;
    TestTrue(TEXT("Read authored semantic memory"), F.Store.TryReadSemanticMemory(F.Memory, Found, Memory, F.Error));
    TestTrue(TEXT("Memory shares the same causal event"), Found && Memory.SourceEventId == ConsequenceEvent);
    FOGWorldPresenceRecord Presence;
    TestTrue(TEXT("Read persisted authored presence"), F.Store.TryReadWorldPresence(F.Ruler, Found, Presence, F.Error));
    TestTrue(TEXT("Authored world consequence persists"), Found && Presence.LocalPosition.Equals(FVector(50, 60, 0)));
    TestTrue(TEXT("Reload sole canonical clock from same Store"), Clock.LoadOrCreate(Policy, 5000, F.Error));
    TestEqual(TEXT("Interaction never forks or rewinds canonical time"), Clock.GetCanonicalWorldTick(), int64(8));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSystemicInteractionPrivacyParityTest,
    "OfflineGame.Foundation.Interaction.PrivacySubstitutionPreservesAuthoritativeParity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGSystemicInteractionPrivacyParityTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open canonical parity fixture"), F.Open())) return false;
    const auto Graph = F.GraphWithConsequences();
    TArray<FOGWorldEvent> Visible;
    FOGKnowledgeFactRecord VisibleFact; FOGSemanticMemoryRecord VisibleMemory;
    FOGWorldPresenceRecord VisiblePresence;
    FString InitialJson; FName InitialKind; int64 InitialRevision = 0; bool Found = false;
    TestTrue(TEXT("Capture existing canonical history and progression"), F.Store.TryReadEntity(F.Ruler,
        Found, InitialKind, InitialJson, InitialRevision, F.Error));
    TArray<FOGEntityClassRecord> InitialClasses;
    TestTrue(TEXT("Capture actual canonical class progression"), F.Store.ListEntityClasses(F.Ruler, InitialClasses, F.Error));
    for (int32 Pass = 0; Pass < 2; ++Pass)
    {
        FInteractionPresentationProbe Presentation;
        FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler); Runtime.SetPresentationBridge(&Presentation);
        auto Entry = F.Entry; Entry.bPrivacyPresentation = Pass == 1;
        if (!TestTrue(TEXT("Visible and private entry accept identical logical scene"), Runtime.Enter(Entry, Graph, 5, F.Error))) return false;
        const FName Current = Runtime.GetCurrentNode();
        Runtime.SetPrivacyPresentation(Pass == 0);
        Runtime.SetPrivacyPresentation(Pass == 1);
        TestEqual(TEXT("Substitution leaves selected action intact"), Runtime.GetCurrentNode(), Current);
        TestEqual(TEXT("Substitution leaves player pacing intact"), Runtime.GetPacing(), 1.0);
        TestEqual(TEXT("Substitution retains every participant"), Runtime.GetParticipants().Num(), 13);
        for (int32 Index = 0; Index < Runtime.GetParticipants().Num(); ++Index)
            TestTrue(TEXT("Substitution cannot replace canonical bindings"),
                Runtime.GetParticipants()[Index].Binding.Character.EntityId == Entry.Participants[Index].Character.EntityId);
        TestEqual(TEXT("Bridge receives only presentation choice"), Presentation.bPrivacy, Pass == 1);
        TestTrue(TEXT("Same authored action completes under both presentations"), Runtime.Advance(1, 6, F.Error));
        TestTrue(TEXT("Same authored transition remains selectable"), Runtime.SelectAction(TEXT("handoff"), F.Error));
        TestTrue(TEXT("Same action sequence completes under both presentations"), Runtime.Advance(2, 8, F.Error));
        TestTrue(TEXT("Both presentations return through same exit"), Runtime.Exit(TEXT("player_exit"), 9, F.Error));
        Runtime.SetPresentationBridge(nullptr);
        TArray<FOGWorldEvent> Events; TestTrue(TEXT("Read shared canonical event ledger"), F.Events(Events));
        Events.Sort([](const auto& A, const auto& B)
        { return A.WorldTick == B.WorldTick ? A.EventType.ToString() < B.EventType.ToString() : A.WorldTick < B.WorldTick; });
        FOGKnowledgeFactRecord Fact; FOGSemanticMemoryRecord Memory; FOGWorldPresenceRecord Presence;
        TestTrue(TEXT("Read identical authored fact"), F.Store.TryReadKnowledgeFact(F.Ruler,
            TEXT("neutral_handoff_observed"), Entry.Participants.Last().Character.EntityId, Found, Fact, F.Error));
        TestTrue(TEXT("Authored fact exists"), Found);
        TestTrue(TEXT("Read identical authored memory"), F.Store.TryReadSemanticMemory(F.Memory, Found, Memory, F.Error));
        TestTrue(TEXT("Authored memory exists"), Found);
        TestTrue(TEXT("Read identical authored presence"), F.Store.TryReadWorldPresence(F.Ruler, Found, Presence, F.Error));
        TestTrue(TEXT("Authored presence exists"), Found);
        if (Pass == 0)
        { Visible = Events; VisibleFact = Fact; VisibleMemory = Memory; VisiblePresence = Presence; }
        else
        {
            TSet<FOGEntityId> PriorIds; for (const auto& Event : Visible) PriorIds.Add(Event.EventId);
            Events.RemoveAll([&PriorIds](const auto& Event) { return PriorIds.Contains(Event.EventId); });
            if (TestEqual(TEXT("Privacy creates identical event categories and counts"), Events.Num(), Visible.Num()))
                for (int32 Index = 0; Index < Events.Num(); ++Index)
                {
                    TestEqual(TEXT("Privacy preserves event category"), Events[Index].EventType, Visible[Index].EventType);
                    TestEqual(TEXT("Privacy preserves canonical event timestamp"), Events[Index].WorldTick, Visible[Index].WorldTick);
                    TestTrue(TEXT("Privacy preserves event authority"), Events[Index].PrimaryEntity == Visible[Index].PrimaryEntity);
                    TestTrue(TEXT("Privacy preserves participant causality"), Events[Index].RelatedEntities == Visible[Index].RelatedEntities);
                    TestEqual(TEXT("Privacy preserves Chronicle flags"), Events[Index].bChronicleEligible, Visible[Index].bChronicleEligible);
                    TestEqual(TEXT("Privacy preserves every authored payload field"), WithoutGeneratedSessionId(Events[Index]), WithoutGeneratedSessionId(Visible[Index]));
                    const auto Object = Payload(Events[Index]);
                    TestFalse(TEXT("Privacy preference cannot leak into canonical payload"), Object->HasField(TEXT("privacy")) || Object->HasField(TEXT("privacy_presentation")));
                }
            TestEqual(TEXT("Same authored knowledge outcome"), Fact.ValueJson, VisibleFact.ValueJson);
            TestEqual(TEXT("Same knowledge confidence"), Fact.ConfidenceBps, VisibleFact.ConfidenceBps);
            TestEqual(TEXT("Same knowledge belief"), Fact.BeliefState, VisibleFact.BeliefState);
            TestEqual(TEXT("Same learned history tick"), Fact.LearnedWorldTick, VisibleFact.LearnedWorldTick);
            TestEqual(TEXT("Same consequence timestamp"), Fact.UpdatedWorldTick, VisibleFact.UpdatedWorldTick);
            TestTrue(TEXT("Same knowledge source individual"), Fact.SourceEntityId == VisibleFact.SourceEntityId);
            TestEqual(TEXT("Same semantic history outcome"), Memory.StateJson, VisibleMemory.StateJson);
            TestEqual(TEXT("Same semantic history type"), Memory.MemoryTypeId.ToString(), VisibleMemory.MemoryTypeId.ToString());
            TestEqual(TEXT("Same semantic salience"), Memory.SalienceBps, VisibleMemory.SalienceBps);
            TestTrue(TEXT("Same persistent world outcome"), Presence.LocationId == VisiblePresence.LocationId && Presence.LocalPosition.Equals(VisiblePresence.LocalPosition));
            // SourceEventId is generated, but must identify the matching causal completion in each run.
            const auto* Action = Events.FindByPredicate([](const auto& Event)
                { return Event.EventType == TEXT("interaction.action_completed") && Event.WorldTick == 6; });
            TestTrue(TEXT("Privacy knowledge and memory retain actual canonical causality"),
                Action && Fact.SourceEventId == Action->EventId && Memory.SourceEventId == Action->EventId);
        }
        FString Json; FName Kind; int64 Revision = 0;
        TestTrue(TEXT("Read retained authoritative character"), F.Store.TryReadEntity(F.Ruler, Found, Kind, Json, Revision, F.Error));
        TestEqual(TEXT("History and progression never rewritten by presentation"), Json, InitialJson);
        TestEqual(TEXT("Character kind remains canonical"), Kind, InitialKind);
        TestEqual(TEXT("Existing entity metadata remains canonical"), Revision, InitialRevision);
        TArray<FOGEntityClassRecord> Classes;
        TestTrue(TEXT("Read actual canonical class progression"), F.Store.ListEntityClasses(F.Ruler, Classes, F.Error));
        if (TestEqual(TEXT("Privacy and neutral actions preserve class count"), Classes.Num(), InitialClasses.Num()))
            for (int32 Index = 0; Index < Classes.Num(); ++Index)
            {
                TestTrue(TEXT("Canonical class identity and owner retained"), Classes[Index].OwnerEntityId == InitialClasses[Index].OwnerEntityId && Classes[Index].ClassId == InitialClasses[Index].ClassId);
                TestEqual(TEXT("Canonical attained tier retained"), Classes[Index].AttainedTier, InitialClasses[Index].AttainedTier);
                TestEqual(TEXT("Canonical expression retained"), Classes[Index].CurrentExpressionState, InitialClasses[Index].CurrentExpressionState);
                TestEqual(TEXT("Canonical recognition history retained"), Classes[Index].RecognizedWorldTick, InitialClasses[Index].RecognizedWorldTick);
                TestEqual(TEXT("Canonical progression timestamp retained"), Classes[Index].UpdatedWorldTick, InitialClasses[Index].UpdatedWorldTick);
                TestEqual(TEXT("Canonical progression state retained"), Classes[Index].StateJson, InitialClasses[Index].StateJson);
            }
        int64 Balance = 0;
        TestTrue(TEXT("Read existing unrelated resource"), F.Store.TryReadResourceBalance(F.Ruler,
            FOGContentId(TEXT("test:resource.unrelated")), Found, Balance, F.Error));
        TestEqual(TEXT("No universal interaction reward economy"), Balance, int64(29));
    }
    return true;
}

namespace
{
struct FInteractionTestWorld
{
    UWorld* World = nullptr;
    FInteractionTestWorld()
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(EWorldType::Game, true, NAME_None, nullptr, true,
            ERHIFeatureLevel::SM5, &Values, false);
    }
    ~FInteractionTestWorld() { if (World) World->DestroyWorld(true); }
    void AddFloor()
    {
        AActor* Floor = World->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(Box); Floor->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(5000, 5000, 50));
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->SetCollisionObjectType(ECC_WorldStatic); Box->RegisterComponent();
        Floor->SetActorLocation(FVector(0, 0, -50));
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSystemicInteractionGameplayRestoreTest,
    "OfflineGame.Foundation.Interaction.StagingFailureInterruptionAndFailedExitRestoreGameplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGSystemicInteractionGameplayRestoreTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open canonical embodied fixture"), F.Open(1))) return false;
    FInteractionTestWorld W;
    if (!TestNotNull(TEXT("Isolated staging world"), W.World)) return false;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACharacter* Actor = W.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FVector(700, 100, 200), FRotator(0, 35, 0), Spawn);
    APlayerController* Controller = W.World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Native participant body"), Actor) || !TestNotNull(TEXT("Native return controller"), Controller)) return false;
    Controller->SpawnPlayerCameraManager();
    if (!TestNotNull(TEXT("Synthetic controller has native camera manager"), Controller->PlayerCameraManager.Get())) return false;
    auto* Movement = Actor->GetCharacterMovement();
    Movement->SetMovementMode(MOVE_Flying); Movement->Velocity = FVector(5, 6, 7);
    const FTransform Before = Actor->GetActorTransform(); const FVector Velocity = Movement->Velocity;
    Controller->SetViewTarget(Actor); Controller->SetControlRotation(FRotator(10, 25, 0));
    const FRotator ControlRotation = Controller->GetControlRotation();
    // Prior suppression belongs to ordinary gameplay and must survive our exit.
    Controller->SetIgnoreMoveInput(true);
    F.Entry.World = W.World; F.Entry.ReturnController = Controller;
    F.Entry.Participants[0].Actor = Actor;
    FInteractionPresentationProbe Presentation;
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    Runtime.SetPresentationBridge(&Presentation);
    const auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    TestFalse(TEXT("No physical floor fails staging safely"), Runtime.Enter(F.Entry, Graph, 5, F.Error));
    TestFalse(TEXT("Failed staging never activates"), Runtime.IsActive());
    TestTrue(TEXT("Failed staging preserves original body transform"), Actor->GetActorTransform().Equals(Before));
    TestTrue(TEXT("Failed staging preserves original velocity"), Movement->Velocity.Equals(Velocity));
    TestTrue(TEXT("Failed staging preserves original movement mode"), Movement->MovementMode == MOVE_Flying);
    TestFalse(TEXT("Failed staging preserves visibility"), Actor->IsHidden());
    TestTrue(TEXT("Failed staging preserves existing move suppression"), Controller->IsMoveInputIgnored());
    TestFalse(TEXT("Failed staging does not own look suppression"), Controller->IsLookInputIgnored());
    TestTrue(TEXT("Failed staging preserves ordinary camera"), Controller->GetViewTarget() == Actor);
    TestEqual(TEXT("Failed staging does not touch ordinary character presentation"), Presentation.CharacterCalls, 0);
    TArray<FOGWorldEvent> Events; TestTrue(TEXT("Read failed staging events"), F.Events(Events));
    TestEqual(TEXT("Failed staging persists no entry"), Events.Num(), 0);
    W.AddFloor();
    if (!TestTrue(TEXT("Nearby clear floor permits embodied staging"), Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    TestTrue(TEXT("Staging owns ordinary movement temporarily"), Movement->MovementMode == MOVE_None);
    TestTrue(TEXT("Staging owns look suppression"), Controller->IsLookInputIgnored());
    Runtime.SetPrivacyPresentation(true); TestTrue(TEXT("Privacy masks actual body"), Actor->IsHidden());
    Runtime.SetPrivacyPresentation(false); TestFalse(TEXT("Privacy reversal preserves saved visibility"), Actor->IsHidden());
    TestTrue(TEXT("Player camera uses native camera hook"), Runtime.SetCamera(FTransform(FRotator(0, 180, 0), FVector(400, 0, 200)), F.Error));
    TestTrue(TEXT("Interaction camera temporarily becomes view target"), Controller->GetViewTarget() != Actor);
    const FTransform Staged = Actor->GetActorTransform();
    TestFalse(TEXT("Invalid restaging rejected without corrupting current placement"),
        Runtime.Restage(FVector(std::numeric_limits<double>::quiet_NaN(), 0, 0), F.Error));
    TestTrue(TEXT("Restaging failure preserves current safe placement"), Actor->GetActorTransform().Equals(Staged));
    TestTrue(TEXT("Native application-context interruption exits cleanly"), Runtime.Exit(TEXT("application_deactivated"), 6, F.Error));
    TestFalse(TEXT("Interrupted interaction inactive"), Runtime.IsActive());
    TestTrue(TEXT("Interruption restores original transform"), Actor->GetActorTransform().Equals(Before));
    TestTrue(TEXT("Interruption restores velocity"), Movement->Velocity.Equals(Velocity));
    TestTrue(TEXT("Interruption restores movement mode"), Movement->MovementMode == MOVE_Flying);
    TestFalse(TEXT("Interruption restores original visibility"), Actor->IsHidden());
    TestTrue(TEXT("Interruption preserves move suppression owned by ordinary gameplay"), Controller->IsMoveInputIgnored());
    TestFalse(TEXT("Interruption releases only its own look suppression"), Controller->IsLookInputIgnored());
    TestTrue(TEXT("Interruption restores ordinary view target"), Controller->GetViewTarget() == Actor);
    TestTrue(TEXT("Interruption restores ordinary control rotation"), Controller->GetControlRotation().Equals(ControlRotation));
    TestEqual(TEXT("Interruption restores character bridge once"), Presentation.RestoreCalls, 1);
    TestTrue(TEXT("Entry works again after clean interruption"), Runtime.Enter(F.Entry, Graph, 7, F.Error));
    Runtime.SetPrivacyPresentation(true);
    TestFalse(TEXT("Unavailable canonical clock reports failed exit persistence"), Runtime.Exit(TEXT("core_unavailable"), -1, F.Error));
    TestFalse(TEXT("Failed exit persistence cannot trap active runtime"), Runtime.IsActive());
    TestTrue(TEXT("Failed exit persistence still restores body"), Actor->GetActorTransform().Equals(Before));
    TestFalse(TEXT("Failed exit persistence still restores visibility"), Actor->IsHidden());
    TestTrue(TEXT("Failed exit persistence still restores movement"), Movement->MovementMode == MOVE_Flying && Movement->Velocity.Equals(Velocity));
    TestTrue(TEXT("Failed exit persistence preserves ordinary move ownership"), Controller->IsMoveInputIgnored());
    TestFalse(TEXT("Failed exit persistence releases look ownership"), Controller->IsLookInputIgnored());
    TestTrue(TEXT("Failed exit persistence restores ordinary camera"), Controller->GetViewTarget() == Actor);
    TestEqual(TEXT("Every active session restores character bridge once"), Presentation.RestoreCalls, 2);
    Runtime.SetPresentationBridge(nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionStateFreshPatchTest,
    "OfflineGame.Foundation.Interaction.State.FreshRoleRelativeNpcPatches",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGInteractionStateFreshPatchTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open ordinary NPC fixture"), F.Open(2))) return false;
    const auto Npc = F.Entry.Participants[1].Character.EntityId;
    FOGInteractionPresentationPatch Appearance;
    Appearance.Role = TEXT("participant");
    Appearance.OutfitMergeJson = TEXT("{\"layers\":{\"outer\":\"updated\"},\"slots\":[\"new\"],\"nullable\":null}");
    Appearance.VariantMergeJson = TEXT("{\"hairstyle_id\":\"test:hair.selected\"}");
    FOGInteractionAdultContextPatch Context;
    Context.Role = Appearance.Role; Context.MutableContextMergeJson = TEXT("{\"expression\":{\"current\":\"neutral\"}}");
    auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    Graph.bMaturePresentation = true; Graph.DisplayName = TEXT("Neutral state fixture");
    Graph.Nodes[0].Consequences.PresentationPatches.Add(Appearance);
    Graph.Nodes[0].Consequences.AdultContextPatches.Add(Context);
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    if (!TestTrue(TEXT("Classification adds no gameplay gate"), Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    // Deliberately update the canonical records AFTER the entry projection.
    const FString NpcJson = TEXT("{\"history\":\"fresh\",\"precise\":9007199254740993,\"foundation_presentation\":{\"skin_id\":\"test:skin.current\",\"outfit_state_json\":\"{\\\"layers\\\":{\\\"inner\\\":\\\"retained\\\"},\\\"damage_bps\\\":17}\",\"variant_state_json\":\"{\\\"other\\\":true}\",\"extension\":\"preserved\"}}");
    TestTrue(TEXT("Persist fresh ordinary NPC state"), F.Store.UpsertEntity(Npc, TEXT("npc"), 6, NpcJson, F.Error));
    FOGCharacterAdultRuntimeStateRecord Adult; Adult.CharacterEntityId = Npc;
    Adult.CurrentProfileVariantId = FOGContentId(TEXT("test:profile.current"));
    Adult.MutableContextStateJson = TEXT("{\"expression\":{\"other\":true},\"precise\":9007199254740993}");
    TestTrue(TEXT("Persist fresh context independently"), F.Store.UpsertCharacterAdultRuntimeState(Adult, F.Error));
    TestTrue(TEXT("Partial progress has no patches"), Runtime.Advance(0.5, 7, F.Error));
    bool Found = false; FName Kind; FString Json; int64 Revision = 0;
    TestTrue(TEXT("Read uncompleted NPC"), F.Store.TryReadEntity(Npc, Found, Kind, Json, Revision, F.Error));
    TestEqual(TEXT("Patches wait for actual completion"), Json, NpcJson);
    Runtime.SetPrivacyPresentation(true);
    if (!TestTrue(TEXT("Complete canonical patches with privacy substitution"), Runtime.Advance(0.5, 8, F.Error))) return false;
    FOGFoundationCharacterProjection Projection;
    TestTrue(TEXT("Ordinary character projection sees patches"), FOGFoundationCharacterRuntime(F.Store).Project(F.Entry.Participants[1].Character, Projection, F.Error));
    TestTrue(TEXT("No Manifestation is required for NPC presentation"), !Projection.bHasManifestation && Projection.bHasPresentation);
    TestEqual(TEXT("Unset optional skin preserves fresh selection"), Projection.Presentation.SelectedSkinId.ToString(), FString(TEXT("test:skin.current")));
    TestTrue(TEXT("Nested outfit sibling retained"), Projection.Presentation.OutfitStateJson.Contains(TEXT("retained")));
    TestTrue(TEXT("Authored outfit field updated"), Projection.Presentation.OutfitStateJson.Contains(TEXT("updated")));
    TestTrue(TEXT("Hairstyle flows through existing variant JSON"), Projection.Presentation.PresentationVariantStateJson.Contains(TEXT("test:hair.selected")));
    TestTrue(TEXT("Untouched variant retained"), Projection.Presentation.PresentationVariantStateJson.Contains(TEXT("other")));
    TestTrue(TEXT("Fresh entity history and presentation extensions retained"), Projection.EntityStateJson.Contains(TEXT("fresh")) && Projection.EntityStateJson.Contains(TEXT("preserved")));
    TestTrue(TEXT("Untouched large JSON numbers remain exact"), Projection.EntityStateJson.Contains(TEXT("9007199254740993")) && Projection.AdultContext.MutableContextStateJson.Contains(TEXT("9007199254740993")));
    TestEqual(TEXT("Fresh profile selection retained"), Projection.AdultContext.CurrentProfileVariantId.ToString(), Adult.CurrentProfileVariantId.ToString());
    TestTrue(TEXT("Nested expression sibling retained"), Projection.AdultContext.MutableContextStateJson.Contains(TEXT("other")));
    TestEqual(TEXT("State uses actual completion tick"), Projection.AdultContext.UpdatedWorldTick, int64(8));
    TestTrue(TEXT("Idle advances cannot repeat completion"), Runtime.Advance(10, 9, F.Error));
    TestTrue(TEXT("Exit never reapplies patches"), Runtime.Exit(TEXT("interrupted"), 10, F.Error));
    TArray<FOGWorldEvent> Events; TestTrue(TEXT("Read event ledger"), F.Events(Events));
    TestEqual(TEXT("One entry, one completion, one exit"), Events.Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionStateRollbackRetryTest,
    "OfflineGame.Foundation.Interaction.State.RollbackFreshRetryAndExit",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGInteractionStateRollbackRetryTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open transaction fixture"), F.Open(1))) return false;
    auto Graph = F.GraphWithConsequences();
    FOGInteractionPresentationPatch Appearance; Appearance.Role = TEXT("player");
    Appearance.VariantMergeJson = TEXT("{\"hairstyle_id\":\"test:hair.updated\"}");
    Graph.Nodes[0].Consequences.PresentationPatches.Add(Appearance);
    FOGInteractionAdultContextPatch Context; Context.Role = TEXT("player");
    Context.MutableContextMergeJson = TEXT("{\"expression\":\"neutral\"}");
    Graph.Nodes[0].Consequences.AdultContextPatches.Add(Context);
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    if (!TestTrue(TEXT("Enter before external state corruption"), Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    FOGCharacterAdultRuntimeStateRecord Adult; Adult.CharacterEntityId = F.Ruler;
    Adult.MutableContextStateJson = TEXT("{\"duplicate\":1,\"duplicate\":2}");
    TestTrue(TEXT("Store accepts legacy invalid payload for negative fixture"), F.Store.UpsertCharacterAdultRuntimeState(Adult, F.Error));
    TestFalse(TEXT("Malformed fresh context aborts all completion writes"), Runtime.Advance(1, 6, F.Error));
    TestEqual(TEXT("Uncommitted action remains retryable"), Runtime.GetCurrentNode(), Graph.StartNode);
    TArray<FOGWorldEvent> Events; F.Events(Events);
    TestEqual(TEXT("Failed completion rolls back its event"), Events.Num(), 1);
    bool Found = false; FName Kind; FString Json; int64 Revision = 0;
    F.Store.TryReadEntity(F.Ruler, Found, Kind, Json, Revision, F.Error);
    TestFalse(TEXT("Earlier presentation write rolled back"), Json.Contains(TEXT("foundation_presentation")));
    Adult.MutableContextStateJson = TEXT("{\"external\":true}");
    TestTrue(TEXT("Repair canonical context"), F.Store.UpsertCharacterAdultRuntimeState(Adult, F.Error));
    TestTrue(TEXT("Retry rereads fresh records and commits once"), Runtime.Advance(0, 7, F.Error));
    TestTrue(TEXT("Idle retry cannot duplicate consequences"), Runtime.Advance(0, 7, F.Error));
    FOGCharacterAdultRuntimeStateRecord Result;
    F.Store.TryReadCharacterAdultRuntimeState(F.Ruler, Found, Result, F.Error);
    TestTrue(TEXT("Retry preserves repair made after failure"), Found && Result.MutableContextStateJson.Contains(TEXT("external")));
    TestTrue(TEXT("Clean exit after successful retry"), Runtime.Exit(TEXT("interrupted"), 8, F.Error));
    F.Events(Events); TestEqual(TEXT("Retry produces one canonical completion"), Events.Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionStateValidationTest,
    "OfflineGame.Foundation.Interaction.State.StrictJsonAndUnambiguousRoleValidation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGInteractionStateValidationTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open validation fixture"), F.Open())) return false;
    const TArray<FString> Invalid = {
        TEXT(""), TEXT("[]"), TEXT("null"), TEXT("{\"x\":1,\"x\":2}"),
        TEXT("{\"x\":1,\"\\u0078\":2}"), TEXT("{} {}"), TEXT("{\"x\":NaN}"),
        TEXT("{\"x\":1e999}"), TEXT("{\"x\":01}"), TEXT("{\"x\":true,}"),
        TEXT("{/*comment*/\"x\":true}"), TEXT("{\"x\":\"\\q\"}"),
        TEXT("{\"expression\":{\"eligible\":true}}"), TEXT("{\"consent\":true}"),
        TEXT("{\"affection\":100}"), TEXT("{\"active_version_id\":\"test:version.forbidden\"}")
    };
    for (const FString& BadJson : Invalid)
    {
        auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
        FOGInteractionAdultContextPatch Patch; Patch.Role = TEXT("player"); Patch.MutableContextMergeJson = BadJson;
        Graph.Nodes[0].Consequences.AdultContextPatches.Add(Patch);
        FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
        TestFalse(TEXT("Invalid authored JSON fails before entry"), Runtime.Enter(F.Entry, Graph, 5, F.Error));
        TestFalse(TEXT("Invalid graph does not own gameplay"), Runtime.IsActive());
    }
    auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    FOGInteractionPresentationPatch Patch; Patch.Role = TEXT("participant");
    Graph.Nodes[0].Consequences.PresentationPatches.Add(Patch);
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    TestFalse(TEXT("Group roles cannot silently select arbitrary NPC"), Runtime.Enter(F.Entry, Graph, 5, F.Error));
    Graph.Nodes[0].Consequences.PresentationPatches[0].Role = TEXT("missing");
    TestFalse(TEXT("Missing role rejected"), Runtime.Enter(F.Entry, Graph, 5, F.Error));
    TArray<FOGWorldEvent> Events; F.Events(Events);
    TestEqual(TEXT("Invalid patch graphs persist no event"), Events.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionStateCompletionRecoveryTest,
    "OfflineGame.Foundation.Interaction.State.CommittedCompletionQueueRecoveryAndMonotonicClock",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGInteractionStateCompletionRecoveryTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open completion fixture"), F.Open(1))) return false;
    FInteractionPresentationProbe Presentation;
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler); Runtime.SetPresentationBridge(&Presentation);
    const auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    if (!TestTrue(TEXT("Enter queue fixture"), Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    TestTrue(TEXT("Active-node available actions follow source edges"), Runtime.GetAvailableActions().Contains(FName(TEXT("handoff"))));
    TestTrue(TEXT("Queue transition"), Runtime.SetSequence({FName(TEXT("handoff"))}, F.Error));
    TestEqual(TEXT("Read-only queue excludes the active node"), Runtime.GetQueuedActions().Num(), 1);
    TestFalse(TEXT("Backward canonical tick rejected before progress"), Runtime.Advance(1, 4, F.Error));
    const int32 Stops = Presentation.StopCalls;
    Presentation.OnCharacter = [&F]() { F.Store.Close(); };
    TestFalse(TEXT("Post-commit next-node refresh failure reported"), Runtime.Advance(1, 6, F.Error));
    TestTrue(TEXT("Committed action cannot be retried as unfinished"), Runtime.IsAwaitingAction());
    TestTrue(TEXT("Awaiting available actions follow last completed source edges"), Runtime.GetAvailableActions().Contains(FName(TEXT("handoff"))));
    TestTrue(TEXT("Completed action presentation stops while awaiting recovery"), Presentation.StopCalls > Stops);
    Presentation.OnCharacter = nullptr;
    TestTrue(TEXT("Restore canonical store"), F.Store.Open(F.DatabasePath, F.Error));
    TestTrue(TEXT("Later advance recovers queued transition"), Runtime.Advance(0, 7, F.Error));
    TestEqual(TEXT("Queued action no longer stranded"), Runtime.GetCurrentNode(), FName(TEXT("handoff")));
    TArray<FOGWorldEvent> Events; F.Events(Events);
    TestEqual(TEXT("Recovery never repeats committed completion event"), Events.Num(), 2);
    TestTrue(TEXT("Pause does not freeze canonical tick"), Runtime.SetPacing(0, F.Error) && Runtime.Advance(100, 10, F.Error));
    TestFalse(TEXT("Backward exit refuses stale event but restores gameplay"), Runtime.Exit(TEXT("stale_clock"), 9, F.Error));
    TestFalse(TEXT("Failed exit still cleans up"), Runtime.IsActive());
    TestEqual(TEXT("Inactive sessions expose no action rows"), Runtime.GetAvailableActions().Num(), 0);
    TestEqual(TEXT("Bridge restored exactly once"), Presentation.RestoreCalls, 1);
    Runtime.SetPresentationBridge(nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionStateManifestationPatchTest,
    "OfflineGame.Foundation.Interaction.State.ManifestationPresentationKeepsCanonicalDevelopment",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGInteractionStateManifestationPatchTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open Manifestation fixture"), F.Open(2))) return false;
    const auto Id = F.Entry.Participants[1].Character.EntityId;
    FOGCharacterManifestationRecord Manifestation;
    Manifestation.ManifestationId = Id; Manifestation.OwningRulerId = F.Ruler;
    Manifestation.IdentityId = FOGContentId(TEXT("test:identity.exact"));
    Manifestation.ActiveVersionId = FOGContentId(TEXT("test:version.exact"));
    Manifestation.Level = 17; Manifestation.ProgressionStateJson = TEXT("{\"development\":\"retained\"}");
    if (!TestTrue(TEXT("Persist actual canonical Manifestation"), F.Store.UpsertCharacterManifestation(Manifestation, 0, F.Error))) return false;
    F.Entry.Participants[1].Character.ManifestationId = Id;
    F.Entry.Participants[1].Character.OwnerEntityId = F.Ruler;
    FOGManifestationPresentationStateRecord Presentation; Presentation.ManifestationId = Id;
    Presentation.SelectedSkinId = FOGContentId(TEXT("test:skin.previous"));
    Presentation.OutfitStateJson = TEXT("{\"damage_bps\":300,\"layers\":{\"inner\":true}}");
    Presentation.PresentationVariantStateJson = TEXT("{\"hairstyle_id\":\"test:hair.previous\",\"other\":true}");
    TestTrue(TEXT("Persist existing presentation"), F.Store.UpsertManifestationPresentationState(Presentation, F.Error));
    auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    FOGInteractionPresentationPatch Patch; Patch.Role = TEXT("participant");
    Patch.SelectedSkinId = FOGContentId(); // Explicit empty selection differs from unset.
    Patch.OutfitMergeJson = TEXT("{\"layers\":{\"outer\":true}}");
    Graph.Nodes[0].Consequences.PresentationPatches.Add(Patch);
    Patch.SelectedSkinId.Reset(); Patch.OutfitMergeJson = TEXT("{}");
    Patch.VariantMergeJson = TEXT("{\"hairstyle_id\":\"test:hair.current\"}");
    Graph.Nodes[0].Consequences.PresentationPatches.Add(Patch);
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    if (!TestTrue(TEXT("Enter canonical Manifestation graph"), Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    TestTrue(TEXT("Complete ordered sparse patches"), Runtime.Advance(1, 6, F.Error));
    bool Found = false; FOGManifestationPresentationStateRecord Result;
    F.Store.TryReadManifestationPresentationState(Id, Found, Result, F.Error);
    TestTrue(TEXT("Explicit empty selection clears only skin"), Found && Result.SelectedSkinId.IsEmpty());
    TestTrue(TEXT("Both ordered patches compose with existing state"), Result.OutfitStateJson.Contains(TEXT("inner")) && Result.OutfitStateJson.Contains(TEXT("outer")) && Result.OutfitStateJson.Contains(TEXT("300")));
    TestTrue(TEXT("Other variant data survives hairstyle change"), Result.PresentationVariantStateJson.Contains(TEXT("other")) && Result.PresentationVariantStateJson.Contains(TEXT("test:hair.current")));
    FOGCharacterManifestationRecord Actual;
    F.Store.TryReadCharacterManifestation(Id, Found, Actual, F.Error);
    TestTrue(TEXT("Ownership, Identity and Version cannot be granted or replaced by appearance patches"), Found && Actual.OwningRulerId == Manifestation.OwningRulerId && Actual.IdentityId == Manifestation.IdentityId && Actual.ActiveVersionId == Manifestation.ActiveVersionId);
    TestEqual(TEXT("Canonical level untouched"), Actual.Level, Manifestation.Level);
    TestEqual(TEXT("Canonical development untouched"), Actual.ProgressionStateJson, Manifestation.ProgressionStateJson);
    TestTrue(TEXT("Exit canonical graph"), Runtime.Exit(TEXT("finished"), 7, F.Error));
    F.Store.Close(); TestTrue(TEXT("Reopen sole canonical store"), F.Store.Open(F.DatabasePath, F.Error));
    F.Store.TryReadManifestationPresentationState(Id, Found, Result, F.Error);
    TestTrue(TEXT("Patch survives canonical reload"), Found && Result.PresentationVariantStateJson.Contains(TEXT("test:hair.current")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionStateAuthoredPlacementTest,
    "OfflineGame.Foundation.Interaction.State.AuthoredRoleOffsetsAndObserverStaging",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGInteractionStateAuthoredPlacementTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open variable-arity placement fixture"), F.Open())) return false;
    F.Entry.Participants[1].Role = TEXT("focus");
    F.Entry.PreferredLocation = FVector(300, 400, 100);
    auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    FOGInteractionRolePlacement Player; Player.Role = TEXT("player");
    Player.RelativeToAnchor = FTransform(FRotator(0, 20, 0), FVector(15, 0, 7), FVector(3));
    FOGInteractionRolePlacement Focus; Focus.Role = TEXT("focus");
    Focus.RelativeToAnchor = FTransform(FRotator(0, -20, 0), FVector(45, 0, 27));
    Graph.Nodes[0].RolePlacements = {Player, Focus};
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    if (!TestTrue(TEXT("Authored source offsets support 13 logical participants"), Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    const auto& States = Runtime.GetParticipants();
    TestEqual(TEXT("Observers remain in authoritative state"), States.Num(), 13);
    TestTrue(TEXT("Source role offsets remain together"), (States[1].StagingTransform.GetLocation() - States[0].StagingTransform.GetLocation()).Equals(FVector(30, 0, 20)));
    TestTrue(TEXT("Source role orientation retained"), States[0].StagingTransform.GetRotation().Equals(Player.RelativeToAnchor.GetRotation()));
    TestTrue(TEXT("Authored scale never resizes canonical body"), States[0].StagingTransform.GetScale3D().Equals(FVector::OneVector));
    for (int32 I = 2; I < States.Num(); ++I)
        TestTrue(TEXT("Observers are outside the authored source group"), FVector::Dist2D(States[I].StagingTransform.GetLocation(), F.Entry.PreferredLocation) > 100);
    TestTrue(TEXT("Restage authored group"), Runtime.Restage(FVector(600, 700, 200), F.Error));
    TestTrue(TEXT("Restaging retains relative source offsets"), (States[1].StagingTransform.GetLocation() - States[0].StagingTransform.GetLocation()).Equals(FVector(30, 0, 20)));
    TestTrue(TEXT("Exit placement fixture"), Runtime.Exit(TEXT("finished"), 6, F.Error));
    Graph.Nodes[0].RolePlacements.Add(Player);
    TestFalse(TEXT("Duplicate placement role fails before entry"), Runtime.Enter(F.Entry, Graph, 7, F.Error));
    Graph.Nodes[0].RolePlacements.Pop();
    Graph.Nodes[0].RolePlacements[0].Role = TEXT("participant");
    TestFalse(TEXT("Ambiguous observer group cannot be one source track"), Runtime.Enter(F.Entry, Graph, 7, F.Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionStateEmbodiedPlacementTest,
    "OfflineGame.Foundation.Interaction.State.SourceGroupFloorFitScaleAndFailedRestage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGInteractionStateEmbodiedPlacementTest::RunTest(const FString& Parameters)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open embodied source fixture"), F.Open(2))) return false;
    FInteractionTestWorld W;
    if (!TestNotNull(TEXT("Embodied source world"), W.World)) return false;
    W.AddFloor();
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACharacter* Player = W.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FVector(500, 0, 200), FRotator::ZeroRotator, Spawn);
    ACharacter* Focus = W.World->SpawnActor<ACharacter>(ACharacter::StaticClass(), FVector(900, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Player body"), Player) || !TestNotNull(TEXT("Focus body"), Focus)) return false;
    Player->SetActorScale3D(FVector(1.5));
    const auto OriginalPlayer = Player->GetActorTransform(); const auto OriginalFocus = Focus->GetActorTransform();
    F.Entry.World = W.World; F.Entry.Participants[0].Actor = Player; F.Entry.Participants[1].Actor = Focus;
    auto Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    FOGInteractionRolePlacement A; A.Role = TEXT("player");
    FOGInteractionRolePlacement B; B.Role = TEXT("participant");
    B.RelativeToAnchor = FTransform(FRotator(0, 40, 0), FVector(40, 0, 20));
    Graph.Nodes[0].RolePlacements = {A, B};
    FOGSystemicInteractionRuntime Runtime(F.Store, F.Ruler);
    if (!TestTrue(TEXT("Nearby floor fits source group atomically"), Runtime.Enter(F.Entry, Graph, 5, F.Error))) return false;
    TestTrue(TEXT("Common floor correction preserves exact source offsets"), (Focus->GetActorLocation() - Player->GetActorLocation()).Equals(FVector(40, 0, 20)));
    TestTrue(TEXT("Canonical actor scale survives staging"), Player->GetActorScale3D().Equals(OriginalPlayer.GetScale3D()));
    const auto StagedPlayer = Player->GetActorTransform(); const auto StagedFocus = Focus->GetActorTransform();
    TestFalse(TEXT("No floor within bounded search fails restaging"), Runtime.Restage(FVector(1000000, 1000000, 0), F.Error));
    TestTrue(TEXT("Failed candidate never moves either source role"), Player->GetActorTransform().Equals(StagedPlayer) && Focus->GetActorTransform().Equals(StagedFocus));
    TestTrue(TEXT("Source group exits cleanly"), Runtime.Exit(TEXT("interrupted"), 6, F.Error));
    TestTrue(TEXT("Exit restores both original transforms and scale"), Player->GetActorTransform().Equals(OriginalPlayer) && Focus->GetActorTransform().Equals(OriginalFocus));
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionNativePresentationClockTest,
    "OfflineGame.Foundation.Interaction.NativeClockAndUnavailablePresentation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGInteractionNativePresentationClockTest::RunTest(const FString&)
{
    FInteractionFixture F;
    if (!TestTrue(TEXT("Open canonical fixture"),F.Open(2))) return false;
    FOGSystemicInteractionRuntime Runtime(F.Store,F.Ruler); FInteractionPresentationProbe Probe;
    Runtime.SetPresentationBridge(&Probe); const auto Graph=F.GraphWithConsequences();
    Probe.bPrepared=false;
    TestFalse(TEXT("Missing source adapter rejects entry"),Runtime.Enter(F.Entry,Graph,0,F.Error));
    TArray<FOGWorldEvent> Events;
    TestTrue(TEXT("Read rejected entry events"),F.Events(Events));
    TestEqual(TEXT("Source preflight cannot create an entered event"),Events.Num(),0);
    Probe.bPrepared=true;
    if (!TestTrue(TEXT("Prepared source action enters"),Runtime.Enter(F.Entry,Graph,0,F.Error))) return false;
    TestTrue(TEXT("Pause action pacing"),Runtime.SetPacing(0,F.Error));
    TestTrue(TEXT("Paused frame remains native-valid"),Runtime.Advance(10,1,F.Error));
    TestEqual(TEXT("Facial source clock stays paused"),Probe.PresentationTimes.Last(),0.0);
    TestTrue(TEXT("Set half pacing"),Runtime.SetPacing(.5,F.Error));
    TestTrue(TEXT("Advance actual action source time"),Runtime.Advance(1,2,F.Error));
    TestEqual(TEXT("Native presentation shares paced action time"),Probe.PresentationTimes.Last(),.5);
    Runtime.SetPrivacyPresentation(true); Runtime.SetPrivacyPresentation(false);
    TestEqual(TEXT("Unmasking resumes existing source phase"),Probe.PresentationTimes.Last(),.5);
    Probe.bNativeReady=false;
    TestFalse(TEXT("Interrupted source cannot complete logical action"),Runtime.Advance(10,3,F.Error));
    TestTrue(TEXT("Read interrupted events"),F.Events(Events));
    TestEqual(TEXT("No phantom completed-action event"),Events.Num(),1);
    bool Found=false; FOGKnowledgeFactRecord Fact;
    TestTrue(TEXT("Read withheld consequence"),F.Store.TryReadKnowledgeFact(F.Ruler,
        TEXT("neutral_handoff_observed"),F.Entry.Participants.Last().Character.EntityId,Found,Fact,F.Error));
    TestFalse(TEXT("Unavailable native presentation grants no completion consequence"),Found);
    Probe.bNativeReady=true;
    TestTrue(TEXT("Native source recovers at retained phase"),Runtime.Advance(1,4,F.Error));
    TestTrue(TEXT("Read recovered events"),F.Events(Events));
    TestEqual(TEXT("Recovery produces exactly one completion"),Events.Num(),2);
    TestTrue(TEXT("Close fixture interaction"),Runtime.Exit(TEXT("test_finished"),4,F.Error));
    return true;
}
#endif
