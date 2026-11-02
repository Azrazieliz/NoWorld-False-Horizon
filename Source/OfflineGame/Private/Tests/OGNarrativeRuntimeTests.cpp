#include "Narrative/OGNarrativeService.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Persistence/OGSQLiteWorldStore.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
// Explicitly synthetic test-only content; never installed story or canon.
FString FixtureJson()
{
    return TEXT(R"JSON({"schema":"noworld.narrative.v1","quests":[{
      "quest_id":"test:quest.fixture","display_name":"TEST ONLY quest","description":"Synthetic fixture",
      "objectives":[{"objective_id":"test:objective.fixture","display_name":"TEST ONLY objective","event_type":"test.narrative.evidence","subject":"ruler","required_count":2}],
      "accept_effects":{"costs":[{"resource_id":"test:resource.fixture","display_name":"TEST ONLY resource","amount":"3"}]},
      "completion_effects":{"rewards":[{"resource_id":"test:resource.fixture","display_name":"TEST ONLY resource","amount":"7"}]}
    }],"dialogues":[{
      "dialogue_id":"test:dialogue.fixture","display_name":"TEST ONLY dialogue",
      "subject":{"entity_kind":"test_npc","actor_id":"test:actor.fixture","state_equals":{"life_state":"alive"}},
      "entry_node_id":"test:node.entry","nodes":[
        {"node_id":"test:node.entry","text":"TEST ONLY prompt","choices":[
          {"choice_id":"test:choice.left","text":"TEST ONLY left","next_node_id":"test:node.end","effects":{"branch_locks":[{"branch_id":"test:branch.fixture","value_id":"test:value.left"}]}},
          {"choice_id":"test:choice.right","text":"TEST ONLY right","next_node_id":"test:node.end","effects":{"branch_locks":[{"branch_id":"test:branch.fixture","value_id":"test:value.right"}]}}
        ]},
        {"node_id":"test:node.end","text":"TEST ONLY end","terminal":true,"choices":[]}
      ]
    }]})JSON");
}

struct FFixture
{
    FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Narrative"), FGuid::NewGuid().ToString());
    FString Path = FPaths::Combine(Directory, TEXT("world.db"));
    FOGSQLiteWorldStore Store;
    FOGNarrativeCatalog Catalog;
    FOGNarrativeContext Context;
    FOGContentId QuestId{TEXT("test:quest.fixture")};
    FOGContentId DialogueId{TEXT("test:dialogue.fixture")};
    FOGContentId ResourceId{TEXT("test:resource.fixture")};
    ~FFixture() { Store.Close(); IFileManager::Get().DeleteDirectory(*Directory, false, true); }
    bool Open(FString& Error)
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
        if (!Store.Open(Path, Error) || !FOGNarrativeCatalog::Parse(FixtureJson(), Catalog, Error)) return false;
        Context.RulerId = FOGEntityId::NewId(); Context.SubjectId = FOGEntityId::NewId(); Context.LocationId = FOGEntityId::NewId();
        FOGLocationRecord Location; Location.LocationId = Context.LocationId; Location.Kind = FName(TEXT("test_location"));
        if (!Store.UpsertEntity(Context.RulerId, FName(TEXT("ruler")), 0, TEXT("{}"), Error) ||
            !SetSubject(TEXT("alive"), Error) || !Store.UpsertLocation(Location, 0, Error)) return false;
        for (const auto& Id : {Context.RulerId, Context.SubjectId})
        {
            FOGWorldPresenceRecord Presence; Presence.EntityId = Id; Presence.LocationId = Context.LocationId;
            if (!Store.UpsertWorldPresence(Presence, Error)) return false;
        }
        return Store.SetResourceBalance(Context.RulerId, ResourceId, 10, Error);
    }
    bool SetSubject(const TCHAR* Life, FString& Error)
    {
        return Store.UpsertEntity(Context.SubjectId, FName(TEXT("test_npc")), 0,
            FString::Printf(TEXT("{\"narrative_actor_id\":\"test:actor.fixture\",\"life_state\":\"%s\"}"), Life), Error);
    }
    FOGNarrativeCommand Command(EOGNarrativeCommand Kind) const
    {
        FOGNarrativeCommand C; C.Context = Context; C.Kind = Kind;
        C.ContentId = Kind == EOGNarrativeCommand::StartDialogue || Kind == EOGNarrativeCommand::ChooseDialogue ? DialogueId : QuestId;
        return C;
    }
    bool Execute(const FOGNarrativeCommand& C, int64 Tick, FString& Error)
    {
        FOGNarrativeService Service(Store); FOGNarrativePreview Preview; bool Already = false;
        return Service.Preview(Catalog, C, Tick, Preview, Error) && Preview.bCanApply &&
            Service.Apply(Catalog, C, Preview.ConfirmationToken, FOGEntityId::NewId(), Tick, Already, Error);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNarrativeCatalogContractTest,
    "OfflineGame.Narrative.Catalog.StrictTypedContract",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNarrativeCatalogContractTest::RunTest(const FString&)
{
    FOGNarrativeCatalog C; FString Error;
    TestTrue(TEXT("Synthetic authored catalog parses"), FOGNarrativeCatalog::Parse(FixtureJson(), C, Error));
    TestTrue(TEXT("Empty approved content is valid"), FOGNarrativeCatalog::Parse(
        TEXT("{\"schema\":\"noworld.narrative.v1\",\"quests\":[],\"dialogues\":[]}"), C, Error));
    for (const TCHAR* Bad : {TEXT("0"), TEXT("-1"), TEXT("01"), TEXT("1e3"), TEXT("9223372036854775808")})
        TestFalse(TEXT("Resource amount must be exact positive int64 decimal string"),
            FOGNarrativeCatalog::Parse(FixtureJson().Replace(TEXT("\"amount\":\"3\""),
                *FString::Printf(TEXT("\"amount\":\"%s\""), Bad)), C, Error));
    TestFalse(TEXT("Duplicate JSON member rejected"), FOGNarrativeCatalog::Parse(
        FixtureJson().Replace(TEXT("\"required_count\":2"), TEXT("\"required_count\":2,\"required_count\":3")), C, Error));
    TestFalse(TEXT("Rumor confidence is not a condition"), FOGNarrativeCatalog::Parse(
        FixtureJson().Replace(TEXT("\"subject\":\"ruler\""), TEXT("\"subject\":\"ruler\",\"minimum_confidence_bps\":1")), C, Error));
    TestFalse(TEXT("Dialogue loops cannot farm rewards"), FOGNarrativeCatalog::Parse(
        FixtureJson().Replace(TEXT("\"terminal\":true,\"choices\":[]"),
            TEXT("\"terminal\":false,\"choices\":[{\"choice_id\":\"test:choice.loop\",\"text\":\"TEST ONLY loop\",\"next_node_id\":\"test:node.entry\"}]")), C, Error));
    TestTrue(TEXT("Reachable loop fails specifically on cycle validation"), Error.Contains(TEXT("cycles")));
    TestFalse(TEXT("Generic NPC kind alone cannot authorize dialogue"), FOGNarrativeCatalog::Parse(
        FixtureJson().Replace(TEXT("\"actor_id\":\"test:actor.fixture\","), TEXT("")), C, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNarrativeQuestPersistenceTest,
    "OfflineGame.Narrative.Quest.CanonicalEvidenceRestartAndIdempotency",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNarrativeQuestPersistenceTest::RunTest(const FString&)
{
    FFixture F; FString Error;
    if (!TestTrue(TEXT("Open SQLite fixture"), F.Open(Error))) { AddError(Error); return false; }
    FOGNarrativeService Service(F.Store);
    FOGWorldEvent BeforeAcceptance; BeforeAcceptance.EventId = FOGEntityId::NewId();
    BeforeAcceptance.EventType = FName(TEXT("test.narrative.evidence")); BeforeAcceptance.PrimaryEntity = F.Context.RulerId; BeforeAcceptance.WorldTick = 5;
    TestTrue(TEXT("Canonical same-tick event existed before acceptance"), F.Store.AppendWorldEvent(BeforeAcceptance,Error));
    TestTrue(TEXT("Accept authored quest and debit its cost"), F.Execute(F.Command(EOGNarrativeCommand::AcceptQuest), 5, Error));
    FOGKnowledgeFactRecord Rumor; Rumor.OwnerEntityId = F.Context.RulerId; Rumor.SubjectEntityId = F.Context.SubjectId;
    Rumor.FactKey = FName(TEXT("test.narrative.evidence")); Rumor.ValueJson = TEXT("true");
    TestTrue(TEXT("Persist subjective rumor"), F.Store.UpsertKnowledgeFact(Rumor, Error));
    FOGNarrativePreview Quote;
    TestTrue(TEXT("Preview incomplete quest"), Service.Preview(F.Catalog, F.Command(EOGNarrativeCommand::CompleteQuest), 6, Quote, Error));
    TestFalse(TEXT("Rumor cannot complete an objective"), Quote.bCanApply);
    auto Advance = F.Command(EOGNarrativeCommand::AdvanceQuest);
    Advance.EvidenceEventIds.Add(BeforeAcceptance.EventId);
    for (int32 Index = 0; Index < 2; ++Index)
    {
        FOGWorldEvent Event; Event.EventId = FOGEntityId::NewId(); Event.EventType = Rumor.FactKey;
        Event.PrimaryEntity = F.Context.RulerId; Event.WorldTick = Index == 0 ? 5 : 8;
        TestTrue(TEXT("Append canonical objective evidence"), F.Store.AppendWorldEvent(Event, Error));
        Advance.EvidenceEventIds.Add(Event.EventId);
    }
    TestTrue(TEXT("Credit two distinct canonical events"), F.Execute(Advance, 9, Error));
    TestTrue(TEXT("Preview duplicate evidence safely"), Service.Preview(F.Catalog, Advance, 9, Quote, Error));
    TestFalse(TEXT("Evidence cannot count twice"), Quote.bCanApply);
    F.Store.Close();
    TestTrue(TEXT("Restart same canonical SQLite world"), F.Store.Open(F.Path, Error));
    FOGNarrativeQuestState State;
    TestTrue(TEXT("Read quest after restart"), Service.ReadQuest(F.Context.RulerId, F.QuestId, State, Error));
    const auto* Credits = State.CreditedEvents.Find(FOGContentId(TEXT("test:objective.fixture")));
    TestTrue(TEXT("Distinct objective evidence persisted"), Credits && Credits->Num() == 2);
    TestTrue(TEXT("Preaccept same-tick evidence is excluded"), Credits && !Credits->Contains(BeforeAcceptance.EventId));
    TestTrue(TEXT("Postaccept same-tick evidence is credited"), Credits && Credits->Contains(Advance.EvidenceEventIds[1]));
    auto Complete = F.Command(EOGNarrativeCommand::CompleteQuest);
    if (!TestTrue(TEXT("Preview completion"), Service.Preview(F.Catalog, Complete, 10, Quote, Error)) || !Quote.bCanApply) return false;
    const auto RequestId = FOGEntityId::NewId(); bool Already = false;
    TestTrue(TEXT("Apply authored reward"), Service.Apply(F.Catalog, Complete, Quote.ConfirmationToken, RequestId, 10, Already, Error));
    TestFalse(TEXT("First completion is new"), Already);
    TestTrue(TEXT("Exact request retry succeeds"), Service.Apply(F.Catalog, Complete, Quote.ConfirmationToken, RequestId, 11, Already, Error));
    TestTrue(TEXT("Retry recognized from persistent event receipt"), Already);
    bool Found = false; int64 Balance = 0;
    TestTrue(TEXT("Read canonical balance"), F.Store.TryReadResourceBalance(F.Context.RulerId, F.ResourceId, Found, Balance, Error));
    TestEqual(TEXT("One cost and exactly one reward"), Balance, 14LL);
    TestFalse(TEXT("Reusing request for a different command is rejected"),
        Service.Apply(F.Catalog, Advance, Quote.ConfirmationToken, RequestId, 12, Already, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNarrativeDialogueBranchTest,
    "OfflineGame.Narrative.Dialogue.SubjectDeathStalenessAndBranchLocks",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNarrativeDialogueBranchTest::RunTest(const FString&)
{
    FFixture F; FString Error;
    if (!TestTrue(TEXT("Open dialogue fixture"), F.Open(Error))) { AddError(Error); return false; }
    FOGNarrativeService Service(F.Store); FOGNarrativePreview Quote;
    FOGNarrativeConversationView View;
    TestTrue(TEXT("Conversation view is read-only before start"), Service.GetConversation(F.Catalog,F.Context,F.DialogueId,1,View,Error));
    TestFalse(TEXT("View does not create persistent conversation"), View.State.bExists);
    TestTrue(TEXT("Start eligible authored dialogue"), F.Execute(F.Command(EOGNarrativeCommand::StartDialogue), 1, Error));
    auto Choice = F.Command(EOGNarrativeCommand::ChooseDialogue); Choice.ChoiceId = FOGContentId(TEXT("test:choice.left"));
    TestTrue(TEXT("Preview authored choice"), Service.Preview(F.Catalog, Choice, 2, Quote, Error));
    TestTrue(TEXT("Canonical NPC death"), F.SetSubject(TEXT("dead"), Error));
    bool Already = false;
    TestFalse(TEXT("Dead NPC invalidates previously eligible choice"),
        Service.Apply(F.Catalog, Choice, Quote.ConfirmationToken, FOGEntityId::NewId(), 2, Already, Error));
    FOGNarrativeDialogueState State;
    TestTrue(TEXT("Read unchanged conversation"), Service.ReadDialogue(F.Context, F.DialogueId, State, Error));
    TestFalse(TEXT("Rejected choice did not complete dialogue"), State.bCompleted);
    TestTrue(TEXT("Explicit canonical state restoration"), F.SetSubject(TEXT("alive"), Error));
    TestFalse(TEXT("Changed entity revision makes old quote stale even when state returns"),
        Service.Apply(F.Catalog, Choice, Quote.ConfirmationToken, FOGEntityId::NewId(), 3, Already, Error));
    TestTrue(TEXT("Fresh choice applies and locks branch"), F.Execute(Choice, 3, Error));
    F.Store.Close();
    TestTrue(TEXT("Dialogue and shared branch locks survive restart"), F.Store.Open(F.Path, Error));
    auto Other = F.Catalog.Dialogues[0]; Other.DialogueId = FOGContentId(TEXT("test:dialogue.other"));
    F.Catalog.Dialogues.Add(Other);
    auto StartOther = F.Command(EOGNarrativeCommand::StartDialogue); StartOther.ContentId = Other.DialogueId;
    TestTrue(TEXT("Another authored dialogue can start"), F.Execute(StartOther, 4, Error));
    Choice.ContentId = Other.DialogueId; Choice.ChoiceId = FOGContentId(TEXT("test:choice.right"));
    TestTrue(TEXT("Preview incompatible branch"), Service.Preview(F.Catalog, Choice, 5, Quote, Error));
    TestFalse(TEXT("Persisted branch lock excludes alternative"), Quote.bCanApply);
    const auto SavedSubject = F.Context.SubjectId;
    F.Context.SubjectId = FOGEntityId::NewId();
    TestTrue(TEXT("Persist arbitrary NPC"), F.Store.UpsertEntity(F.Context.SubjectId, FName(TEXT("test_npc")), 0, TEXT("{}"), Error));
    Choice.Context = F.Context;
    TestTrue(TEXT("Preview arbitrary subject safely"), Service.Preview(F.Catalog, Choice, 5, Quote, Error));
    TestFalse(TEXT("Unbound NPC cannot execute authored dialogue"), Quote.bCanApply);
    F.Context.SubjectId = SavedSubject;
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNarrativeInteractorJournalTest,
    "OfflineGame.Narrative.Context.OwnedInteractorAndExplicitRemoteJournalPolicy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNarrativeInteractorJournalTest::RunTest(const FString&)
{
    FFixture F; FString Error;
    if (!TestTrue(TEXT("Open interactor fixture"), F.Open(Error))) { AddError(Error); return false; }
    FOGNarrativeService Service(F.Store);
    FOGLocationRecord Remote; Remote.LocationId = FOGEntityId::NewId(); Remote.Kind = FName(TEXT("test_remote"));
    TestTrue(TEXT("Persist other canonical location"), F.Store.UpsertLocation(Remote,0,Error));
    FOGWorldPresenceRecord RulerPresence; RulerPresence.EntityId = F.Context.RulerId; RulerPresence.LocationId = Remote.LocationId;
    TestTrue(TEXT("Ruler physical presence can differ from controlled copy"), F.Store.UpsertWorldPresence(RulerPresence,Error));
    FOGCharacterManifestationRecord M; M.ManifestationId = FOGEntityId::NewId(); M.OwningRulerId = F.Context.RulerId;
    M.IdentityId = FOGContentId(TEXT("test:identity.interactor")); M.ActiveVersionId = FOGContentId(TEXT("test:version.interactor"));
    M.CurrentRarity = FName(TEXT("R")); M.LifecycleState = FName(TEXT("active")); M.Level = 1;
    TestTrue(TEXT("Persist active owned interactor"), F.Store.UpsertCharacterManifestation(M,0,Error));
    FOGWorldPresenceRecord CopyPresence; CopyPresence.EntityId = M.ManifestationId; CopyPresence.LocationId = F.Context.LocationId;
    TestTrue(TEXT("Controlled copy is beside authored NPC"), F.Store.UpsertWorldPresence(CopyPresence,Error));
    F.Context.InteractorEntityId = M.ManifestationId;
    TestTrue(TEXT("Selected copy permits local dialogue despite ruler elsewhere"), F.Execute(F.Command(EOGNarrativeCommand::StartDialogue),1,Error));
    F.Catalog.Quests[0].Subject = F.Catalog.Dialogues[0].Subject;
    auto RemoteQuest = F.Catalog.Quests[0]; RemoteQuest.QuestId = FOGContentId(TEXT("test:quest.remote")); RemoteQuest.bAllowRemoteCompletion = true;
    F.Catalog.Quests.Add(RemoteQuest);
    TestTrue(TEXT("Accept subject-bound local quest"), F.Execute(F.Command(EOGNarrativeCommand::AcceptQuest),2,Error));
    auto AcceptRemote = F.Command(EOGNarrativeCommand::AcceptQuest); AcceptRemote.ContentId = RemoteQuest.QuestId;
    TestTrue(TEXT("Accept explicitly remote-completable quest locally"), F.Execute(AcceptRemote,2,Error));
    CopyPresence.LocationId = Remote.LocationId;
    TestTrue(TEXT("Move exact controlled copy away"), F.Store.UpsertWorldPresence(CopyPresence,Error));
    FOGNarrativePreview Quote; auto Choice = F.Command(EOGNarrativeCommand::ChooseDialogue); Choice.ChoiceId = FOGContentId(TEXT("test:choice.left"));
    TestTrue(TEXT("Preview remote dialogue"), Service.Preview(F.Catalog,Choice,3,Quote,Error));
    TestFalse(TEXT("Remote NPC choice cannot execute"), Quote.bCanApply);
    F.Context.InteractorEntityId = FOGEntityId(); F.Context.LocationId = FOGEntityId();
    auto Advance = F.Command(EOGNarrativeCommand::AdvanceQuest);
    for (int32 Index = 0; Index < 2; ++Index)
    {
        FOGWorldEvent E; E.EventId = FOGEntityId::NewId(); E.EventType = FName(TEXT("test.narrative.evidence"));
        E.PrimaryEntity = F.Context.RulerId; E.WorldTick = 3 + Index;
        TestTrue(TEXT("Canonical remote objective event"), F.Store.AppendWorldEvent(E,Error)); Advance.EvidenceEventIds.Add(E.EventId);
    }
    TestTrue(TEXT("Journal evidence needs no new local observation"), F.Execute(Advance,5,Error));
    Advance.ContentId = RemoteQuest.QuestId;
    TestTrue(TEXT("Evidence also advances explicitly remote quest"), F.Execute(Advance,5,Error));
    auto Complete = F.Command(EOGNarrativeCommand::CompleteQuest);
    TestTrue(TEXT("Preview default local turn-in from journal"), Service.Preview(F.Catalog,Complete,6,Quote,Error));
    TestFalse(TEXT("Default subject-bound completion remains local"), Quote.bCanApply);
    Complete.ContentId = RemoteQuest.QuestId;
    TestTrue(TEXT("Authored remote completion succeeds without presence or context location"), F.Execute(Complete,6,Error));
    FOGNarrativeQuestState State;
    TestTrue(TEXT("Accepted subject remains bound after remote completion"), Service.ReadQuest(F.Context.RulerId,RemoteQuest.QuestId,State,Error));
    TestTrue(TEXT("Remote completion is canonical and persistent"), State.bCompleted && State.SubjectId == F.Context.SubjectId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNarrativeAtomicFailureTest,
    "OfflineGame.Narrative.Transactions.StaleBalanceAndLateFailureRollback",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGNarrativeAtomicFailureTest::RunTest(const FString&)
{
    FFixture F; FString Error;
    if (!TestTrue(TEXT("Open rollback fixture"), F.Open(Error))) { AddError(Error); return false; }
    FOGNarrativeService Service(F.Store); auto Accept = F.Command(EOGNarrativeCommand::AcceptQuest);
    FOGNarrativePreview Quote; bool Already = false;
    TestTrue(TEXT("Persist wrong-kind owner for boundary check"), F.Store.UpsertEntity(F.Context.RulerId,FName(TEXT("test_npc")),0,TEXT("{}"),Error));
    TestTrue(TEXT("Preview non-ruler owner safely"), Service.Preview(F.Catalog,Accept,1,Quote,Error));
    TestFalse(TEXT("Only canonical rulers may own quests/rewards"), Quote.bCanApply);
    TestTrue(TEXT("Persist diagnostic ruler marker"), F.Store.UpsertEntity(F.Context.RulerId,FName(TEXT("ruler")),0,TEXT("{\"diagnosticFixture\":true}"),Error));
    TestTrue(TEXT("Preview diagnostic ruler safely"), Service.Preview(F.Catalog,Accept,1,Quote,Error));
    TestFalse(TEXT("Diagnostic fixtures cannot receive narrative rewards"), Quote.bCanApply);
    TestTrue(TEXT("Restore canonical ruler"), F.Store.UpsertEntity(F.Context.RulerId,FName(TEXT("ruler")),0,TEXT("{}"),Error));
    TestTrue(TEXT("Preview accept cost"), Service.Preview(F.Catalog,Accept,1,Quote,Error));
    TestTrue(TEXT("Balance changed after preview"), F.Store.SetResourceBalance(F.Context.RulerId,F.ResourceId,11,Error));
    TestFalse(TEXT("Stale balance quote cannot consume resources"),
        Service.Apply(F.Catalog,Accept,Quote.ConfirmationToken,FOGEntityId::NewId(),1,Already,Error));
    TestTrue(TEXT("Refresh quoted balance"), Service.Preview(F.Catalog,Accept,1,Quote,Error));
    TestTrue(TEXT("Inject failure at final canonical receipt"), F.Store.ExecuteSql(
        TEXT("CREATE TEMP TRIGGER test_narrative_receipt_failure BEFORE INSERT ON world_events WHEN NEW.event_type = 'noworld.narrative.command_applied' BEGIN SELECT RAISE(ABORT, 'test failure'); END;"),Error));
    const auto Request = FOGEntityId::NewId();
    TestTrue(TEXT("Caller owns an outer transaction"), F.Store.BeginTransaction(Error));
    TestFalse(TEXT("Late event failure rejects complete mutation"), Service.Apply(F.Catalog,Accept,Quote.ConfirmationToken,Request,1,Already,Error));
    TestTrue(TEXT("Caller transaction remains usable after nested rollback"), F.Store.CommitTransaction(Error));
    bool Found = false; int64 Balance = 0;
    TestTrue(TEXT("Read balance after rollback"), F.Store.TryReadResourceBalance(F.Context.RulerId,F.ResourceId,Found,Balance,Error));
    TestEqual(TEXT("Cost was rolled back"), Balance,11LL);
    FOGNarrativeQuestState State;
    TestTrue(TEXT("Read quest after rollback"), Service.ReadQuest(F.Context.RulerId,F.QuestId,State,Error));
    TestFalse(TEXT("Quest project was rolled back"), State.bExists);
    TestTrue(TEXT("Remove failure injection"), F.Store.ExecuteSql(TEXT("DROP TRIGGER test_narrative_receipt_failure;"),Error));
    TestTrue(TEXT("Same request can apply after rejected transaction"),
        Service.Apply(F.Catalog,Accept,Quote.ConfirmationToken,Request,1,Already,Error));
    TestFalse(TEXT("Rejected attempt did not leave a replay receipt"), Already);
    return true;
}
#endif
