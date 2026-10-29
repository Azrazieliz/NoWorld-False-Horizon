#if WITH_DEV_AUTOMATION_TESTS
#include "Persistence/OGSQLiteWorldStore.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
namespace
{
struct FCommittedEventFixture
{
    FOGSQLiteWorldStore Store; FString Path,Error;
    ~FCommittedEventFixture() { Store.Close(); if (!Path.IsEmpty()) IFileManager::Get().Delete(*Path); }
    bool Open()
    {
        const auto Folder=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation"));
        IFileManager::Get().MakeDirectory(*Folder,true);
        Path=FPaths::Combine(Folder,FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".eventfeed.db"));
        return Store.Open(Path,Error);
    }
    FOGWorldEvent Event()
    {
        FOGWorldEvent E; E.EventId=FOGEntityId::NewId(); E.EventType=TEXT("test.actual_committed_event");
        E.PrimaryEntity=FOGEntityId::NewId(); E.PayloadJson=TEXT("{}"); return E;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCommittedEventSavepointTest,
    "OfflineGame.Persistence.EventFeed.OnlySuccessfulOuterCommitPublishes",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCommittedEventSavepointTest::RunTest(const FString&)
{
    FCommittedEventFixture F; if (!F.Open()) return false;
    auto Feed=F.Store.GetCommittedEventFeed(); TArray<FOGWorldEvent> Seen; int32 Batches=0;
    const auto Handle=Feed->AddLambda([&](const TArray<FOGWorldEvent>& Batch){Seen.Append(Batch); ++Batches;});
    const auto A=F.Event(),B=F.Event(),C=F.Event(),D=F.Event();
    if (!F.Store.BeginTransaction(F.Error) || !F.Store.AppendWorldEvent(A,F.Error) ||
        !F.Store.BeginTransaction(F.Error) || !F.Store.AppendWorldEvent(B,F.Error) ||
        !F.Store.RollbackTransaction(F.Error) || !F.Store.AppendWorldEvent(C,F.Error) ||
        !F.Store.BeginTransaction(F.Error) || !F.Store.AppendWorldEvent(D,F.Error) ||
        !F.Store.CommitTransaction(F.Error)) return false;
    TestEqual(TEXT("Released inner savepoint does not publish gameplay feedback"),Batches,0);
    if (!F.Store.CommitTransaction(F.Error)) return false;
    TestEqual(TEXT("One actual outer commit publishes one batch"),Batches,1);
    if (!TestEqual(TEXT("Only retained events are published"),Seen.Num(),3)) return false;
    TestEqual(TEXT("First actual event retains order"),Seen[0].EventId,A.EventId);
    TestEqual(TEXT("Rolled-back inner event is absent"),Seen[1].EventId,C.EventId);
    TestEqual(TEXT("Committed inner event retains order"),Seen[2].EventId,D.EventId);
    Feed->Remove(Handle); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCommittedEventRollbackTest,
    "OfflineGame.Persistence.EventFeed.RollbackAndRejectedAppendNeverAnimate",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCommittedEventRollbackTest::RunTest(const FString&)
{
    FCommittedEventFixture F; if (!F.Open()) return false;
    auto Feed=F.Store.GetCommittedEventFeed(); int32 Count=0;
    const auto Handle=Feed->AddLambda([&](const TArray<FOGWorldEvent>& Batch){Count+=Batch.Num();});
    const auto A=F.Event();
    if (!F.Store.BeginTransaction(F.Error) || !F.Store.AppendWorldEvent(A,F.Error) || !F.Store.RollbackTransaction(F.Error)) return false;
    TestEqual(TEXT("A rolled-back transaction emits nothing"),Count,0);
    if (!F.Store.AppendWorldEvent(A,F.Error)) return false;
    TestEqual(TEXT("Successful autocommit emits its actual event"),Count,1);
    TestFalse(TEXT("A duplicate event is rejected"),F.Store.AppendWorldEvent(A,F.Error));
    TestEqual(TEXT("Rejected append emits no extra feedback"),Count,1);
    Feed->Remove(Handle); return true;
}
#endif
