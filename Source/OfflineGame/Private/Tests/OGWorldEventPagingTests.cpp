#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Persistence/OGSQLiteWorldStore.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldEventPagingTest,
    "OfflineGame.Persistence.Events.KeysetPagingBeyondOneThousand",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FOGWorldEventPagingTest::RunTest(const FString& Parameters)
{
    struct FFixture
    {
        FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/EventPages") / FGuid::NewGuid().ToString();
        FOGSQLiteWorldStore Store;
        ~FFixture() { Store.Close(); IFileManager::Get().DeleteDirectory(*Directory, false, true); }
    } F;
    IFileManager::Get().MakeDirectory(*F.Directory, true);
    FString Error;
    const FOGEntityId Owner = FOGEntityId::NewId(), Other = FOGEntityId::NewId();
    if (!TestTrue(TEXT("Open isolated event ledger"), F.Store.Open(F.Directory / TEXT("events.db"), Error)) ||
        !TestTrue(TEXT("Create isolated event subject"), F.Store.UpsertEntity(Owner, TEXT("ruler"), 0, TEXT("{}"), Error)) ||
        !TestTrue(TEXT("Create isolated related subject"), F.Store.UpsertEntity(Other, TEXT("npc"), 0, TEXT("{}"), Error)) ||
        !TestTrue(TEXT("Start fixture transaction"), F.Store.BeginTransaction(Error))) return false;
    const FName Type(TEXT("test.paged_evidence"));
    int32 ExpectedChronicle = 0;
    for (int32 I = 0; I < 1030; ++I)
    {
        FOGWorldEvent E;
        E.EventId = FOGEntityId::NewId(); E.EventType = Type; E.WorldTick = I / 7;
        E.PrimaryEntity = I % 2 ? Other : Owner;
        if (I % 2) E.RelatedEntities.Add(Owner);
        E.PayloadJson = TEXT("{}"); E.bChronicleEligible = I % 3 == 0;
        if (E.bChronicleEligible) ++ExpectedChronicle;
        if (!TestTrue(TEXT("Append canonical fixture event"), F.Store.AppendWorldEvent(E, Error))) return false;
    }
    if (!TestTrue(TEXT("Commit fixture history"), F.Store.CommitTransaction(Error))) return false;
    TArray<FOGWorldEvent> Page;
    TestFalse(TEXT("Legacy bounded API still rejects an unbounded query"),
        F.Store.ListWorldEvents(Owner, Type, false, 1030, Page, Error));
    for (bool bChronicle : { false, true })
    {
        FOGWorldEventCursor Cursor;
        TSet<FOGEntityId> Seen;
        for (int32 Batch = 0; Batch < 20; ++Batch)
        {
            FOGWorldEventCursor Next;
            if (!TestTrue(TEXT("Read bounded keyset page"), F.Store.ListWorldEventsPage(Owner, Type, bChronicle, 137, Cursor, Page, Next, Error))) return false;
            for (const FOGWorldEvent& E : Page)
            {
                TestFalse(TEXT("Same-tick UUID tie-break does not duplicate evidence"), Seen.Contains(E.EventId));
                TestTrue(TEXT("Exact source/type scope preserved"), E.EventType == Type &&
                    (E.PrimaryEntity == Owner || E.RelatedEntities.Contains(Owner)));
                if (bChronicle) TestTrue(TEXT("Chronicle filter preserved"), E.bChronicleEligible);
                Seen.Add(E.EventId);
            }
            if (!Next.EventId.IsValid()) break;
            Cursor = Next;
        }
        TestEqual(TEXT("Every matching event remains reachable beyond newest window"), Seen.Num(), bChronicle ? ExpectedChronicle : 1030);
    }
    FOGWorldEventCursor Cursor, Next;
    TestTrue(TEXT("Read first page before concurrent append boundary"), F.Store.ListWorldEventsPage(Owner, Type, false, 137, Cursor, Page, Next, Error));
    FOGWorldEvent Newer;
    Newer.EventId = FOGEntityId::NewId(); Newer.EventType = Type; Newer.WorldTick = 9999;
    Newer.PrimaryEntity = Owner; Newer.PayloadJson = TEXT("{}");
    TestTrue(TEXT("Append newer canonical event between pages"), F.Store.AppendWorldEvent(Newer, Error));
    Cursor = Next;
    TestTrue(TEXT("Resume stable older page"), F.Store.ListWorldEventsPage(Owner, Type, false, 137, Cursor, Page, Next, Error));
    TestFalse(TEXT("Newer append cannot shift or re-enter an older page"), Page.ContainsByPredicate(
        [&Newer](const FOGWorldEvent& E) { return E.EventId == Newer.EventId; }));
    TestFalse(TEXT("Paged query still enforces the 1000-row bound"), F.Store.ListWorldEventsPage(Owner, Type, false, 1001, Cursor, Page, Next, Error));
    return true;
}

#endif
