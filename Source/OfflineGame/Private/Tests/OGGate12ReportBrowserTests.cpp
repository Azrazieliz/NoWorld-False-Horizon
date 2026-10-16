#include "Runtime/OGReportService.h"
#include "Runtime/OGGate12LocalRulerBootstrap.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Persistence/OGSQLiteWorldStore.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12ReportOwnershipAndAckTest,
    "OfflineGame.Gate12.Ruler.ReportBrowsingAcknowledgementAndReload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12ReportOwnershipAndAckTest::RunTest(const FString&)
{
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Db = FPaths::Combine(Directory, TEXT("records.db"));
    FString Error;
    FOGEntityId Owner, ReportId;
    bool bPassed = true;
    {
        FOGSQLiteWorldStore Store;
        if (!TestTrue(TEXT("Isolated Records SQLite opens"), Store.Open(Db, Error)))
            return false;
        if (!TestTrue(TEXT("Real Ruler created in isolated test world"),
                FOGGate12LocalRulerBootstrap::Create(Store, 5, Owner, Error)))
            return false;
        FOGReportService Reports(Store);
        bPassed &= TestTrue(TEXT("Persist real owned report"),
            Reports.CreateReport(Owner, FOGEntityId(), FName(TEXT("threat")),
                91, 9, TEXT("{\"origin\":\"test fixture\"}"), ReportId, Error));
        TArray<FOGReportRecord> Owned;
        bPassed &= TestTrue(TEXT("List by verified Ruler"),
            Store.ListReportsByOwner(Owner, Owned, Error));
        bPassed &= TestEqual(TEXT("One real report, not a fabricated label"),
            Owned.Num(), 1);
        if (Owned.Num() == 1)
        {
            bPassed &= TestTrue(TEXT("Owner ID matches"),
                Owned[0].OwnerEntityId == Owner);
            bPassed &= TestFalse(TEXT("Unread before acknowledgement"),
                Owned[0].bAcknowledged);
        }
        bPassed &= TestFalse(TEXT("Acknowledgement cannot predate report"),
            Reports.AcknowledgeReport(ReportId, 8, Error));
        bPassed &= TestTrue(TEXT("Acknowledgement uses canonical time"),
            Reports.AcknowledgeReport(ReportId, 10, Error));
        bool bFound = false;
        FOGReportRecord After;
        bPassed &= TestTrue(TEXT("Acknowledged record can be read"),
            Store.TryReadReport(ReportId, bFound, After, Error));
        bPassed &= TestTrue(TEXT("Acknowledged report exists"), bFound);
        if (bFound)
        {
            bPassed &= TestTrue(TEXT("Read-state truly persisted"), After.bAcknowledged);
            bPassed &= TestEqual(TEXT("Correct acknowledgement tick"),
                After.AcknowledgedWorldTick, int64(10));
        }
        Store.Close();
    }
    {
        FOGSQLiteWorldStore Store;
        bPassed &= TestTrue(TEXT("Cold reopen retains SQLite database"),
            Store.Open(Db, Error));
        TArray<FOGReportRecord> Reloaded;
        bPassed &= TestTrue(TEXT("List after cold restart"),
            Store.ListReportsByOwner(Owner, Reloaded, Error));
        bPassed &= TestEqual(TEXT("Same single report after restart"),
            Reloaded.Num(), 1);
        if (Reloaded.Num() == 1)
            bPassed &= TestTrue(TEXT("Acknowledgement survived restart"),
                Reloaded[0].bAcknowledged);
        TArray<FOGReportRecord> UnknownOwnerReports;
        bPassed &= TestTrue(TEXT("Another owner has a valid empty report list"),
            Store.ListReportsByOwner(FOGEntityId::NewId(),
                UnknownOwnerReports, Error));
        bPassed &= TestEqual(TEXT("Other owner cannot enumerate private reports"),
            UnknownOwnerReports.Num(), 0);
        Store.Close();
    }
    IFileManager::Get().DeleteDirectory(*Directory, false, true);
    return bPassed;
}
#endif
