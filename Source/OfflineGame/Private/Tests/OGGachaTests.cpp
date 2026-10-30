#include "Gacha/OGGachaService.h"
#include "Persistence/OGSQLiteWorldStore.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
FString MakeGachaTestDirectory()
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

FOGGachaBannerDefinition MakeSingleFeaturedTopBanner()
{
    FOGGachaBannerDefinition Banner;
    Banner.BannerId = FOGContentId(TEXT("test:banner.slice"));
    Banner.PityCategory = TEXT("standard");
    Banner.CurrencyId = FOGContentId(TEXT("test:currency.pull"));
    Banner.PullCost = 100;
    Banner.TopRarity = TEXT("UR");
    Banner.SoftPityStart = 0;
    Banner.HardPity = 1;
    // Single-top fixture cannot represent an independent limited 50/50;
    // the new production validation correctly rejects that dishonest pool.
    Banner.bFeaturedGuaranteeAfterMiss = false;

    FOGGachaPoolEntry Entry;
    Entry.IdentityId = FOGContentId(TEXT("test:character.alpha"));
    Entry.VersionId = FOGContentId(TEXT("test:character.alpha.base"));
    Entry.Rarity = TEXT("UR");
    Entry.Weight = 100;
    Entry.bFeatured = true;
    Banner.Entries.Add(Entry);
    return Banner;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGachaPersistenceAndDuplicateTest,
    "OfflineGame.Gacha.PersistentPityCurrencyAndDuplicateAcquisition",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)

bool FOGGachaPersistenceAndDuplicateTest::RunTest(const FString& Parameters)
{
    const FString Directory = MakeGachaTestDirectory();
    const FString DatabasePath = FPaths::Combine(Directory, TEXT("gacha.db"));
    IFileManager::Get().MakeDirectory(*Directory, true);

    const FOGEntityId RulerId = FOGEntityId::NewId();
    const FOGGachaBannerDefinition Banner = MakeSingleFeaturedTopBanner();
    FOGEntityId FirstManifestationId;
    FOGEntityId SecondManifestationId;

    {
        FOGSQLiteWorldStore Store;
        FString Error;
        TestTrue(TEXT("Open database"), Store.Open(DatabasePath, Error));
        TestEqual(TEXT("Schema version is 14"), Store.GetSchemaVersion(Error), 14);
        TestTrue(TEXT("Persist Ruler"), Store.UpsertEntity(
            RulerId, TEXT("ruler"), 0, TEXT("{}"), Error));
        TestTrue(TEXT("Seed pull currency"), Store.SetResourceBalance(
            RulerId, Banner.CurrencyId, 1000, Error));

        FOGRulerGachaAccessRecord Access;
        Access.RulerId = RulerId;
        Access.bPermanentlyUnlocked = true;
        Access.bHasUnlockedWorldTick = true;
        Access.UnlockedWorldTick = 0;
        Access.UpdatedWorldTick = 0;
        TestTrue(TEXT("Seed already-earned permanent gacha access"),
            Store.UpsertRulerGachaAccess(
                Access,
                Error));

        FOGGachaService Service(Store);
        FOGGachaPullResult First;
        TestTrue(TEXT("First pull succeeds"), Service.Pull(
            Banner, RulerId, 10, 101, First, Error));
        TestFalse(TEXT("First pull is not duplicate"), First.bDuplicateIdentity);
        TestTrue(TEXT("First pull is featured"), First.bFeatured);
        FirstManifestationId = First.ManifestationId;
        TestEqual(TEXT("First pull increments persistent total"),
            First.UpdatedState.TotalPulls, static_cast<int64>(1));

        FOGGachaPullResult Second;
        TestTrue(TEXT("Second pull succeeds"), Service.Pull(
            Banner, RulerId, 20, 202, Second, Error));
        TestTrue(TEXT("Second pull reports Identity already owned"), Second.bDuplicateIdentity);
        SecondManifestationId = Second.ManifestationId;
        TestTrue(TEXT("Repeat acquisition creates a distinct Manifestation"),
            SecondManifestationId != FirstManifestationId);

        TArray<FOGCharacterManifestationRecord> OwnedCopies;
        TestTrue(TEXT("List both same-Identity Manifestations"),
            Store.ListCharacterManifestationsByOwnerAndIdentity(
                RulerId,
                Banner.Entries[0].IdentityId,
                OwnedCopies,
                Error));
        TestEqual(TEXT("Two independent Manifestations are owned"),
            OwnedCopies.Num(), 2);
        if (OwnedCopies.Num() == 2)
        {
            TestEqual(TEXT("First acquisition ordinal"),
                OwnedCopies[0].AcquisitionOrdinal, 0);
            TestEqual(TEXT("Second acquisition ordinal"),
                OwnedCopies[1].AcquisitionOrdinal, 1);
            TestTrue(TEXT("Each Manifestation retains its own origin event"),
                OwnedCopies[0].OriginPullEventId.IsValid() &&
                OwnedCopies[1].OriginPullEventId.IsValid() &&
                OwnedCopies[0].OriginPullEventId != OwnedCopies[1].OriginPullEventId);
        }

        bool bKnown = false;
        int64 Balance = 0;
        TestTrue(TEXT("Read pull currency"), Store.TryReadResourceBalance(
            RulerId, Banner.CurrencyId, bKnown, Balance, Error));
        TestTrue(TEXT("Currency remains known"), bKnown);
        TestEqual(TEXT("Two costs deducted"), Balance, static_cast<int64>(800));
    }

    {
        FOGSQLiteWorldStore Store;
        FString Error;
        TestTrue(TEXT("Reopen database"), Store.Open(DatabasePath, Error));

        bool bFound = false;
        TArray<FOGCharacterManifestationRecord> OwnedCopies;
        TestTrue(TEXT("List Manifestations after restart"),
            Store.ListCharacterManifestationsByOwnerAndIdentity(
                RulerId,
                Banner.Entries[0].IdentityId,
                OwnedCopies,
                Error));
        TestEqual(TEXT("Both independent Manifestations survive restart"),
            OwnedCopies.Num(), 2);
        if (OwnedCopies.Num() == 2)
        {
            TestTrue(TEXT("First Manifestation ID survives restart"),
                OwnedCopies[0].ManifestationId == FirstManifestationId);
            TestTrue(TEXT("Second Manifestation ID survives restart"),
                OwnedCopies[1].ManifestationId == SecondManifestationId);
        }

        FOGGachaStateRecord State;
        TestTrue(TEXT("Read pity after restart"), Store.TryReadGachaState(
            RulerId, Banner.PityCategory, bFound, State, Error));
        TestTrue(TEXT("Pity survives restart"), bFound);
        TestEqual(TEXT("Total pulls survive restart"),
            State.TotalPulls, static_cast<int64>(2));
        TestEqual(TEXT("Top rarity resets pity"), State.PullsSinceTopRarity, 0);
    }

    IFileManager::Get().DeleteDirectory(*Directory, false, true);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGachaTicketFirstPaymentTest,
    "OfflineGame.Gacha.CompatibleTicketIsConsumedBeforeCurrency",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)

bool FOGGachaTicketFirstPaymentTest::RunTest(
    const FString& Parameters)
{
    const FString Directory =
        MakeGachaTestDirectory();
    const FString DatabasePath =
        FPaths::Combine(
            Directory,
            TEXT("gacha_ticket_first.db"));
    IFileManager::Get().MakeDirectory(
        *Directory,
        true);

    FOGSQLiteWorldStore Store;
    FString Error;
    TestTrue(
        TEXT("Open database"),
        Store.Open(
            DatabasePath,
            Error));

    const FOGEntityId RulerId =
        FOGEntityId::NewId();
    FOGGachaBannerDefinition Banner =
        MakeSingleFeaturedTopBanner();
    const FOGContentId TicketId(
        TEXT("test:ticket.standard"));
    Banner.CompatibleTicketIds.Add(
        TicketId);

    TestTrue(
        TEXT("Persist Ruler"),
        Store.UpsertEntity(
            RulerId,
            FName(TEXT("ruler")),
            0,
            TEXT("{}"),
            Error));

    FOGRulerGachaAccessRecord Access;
    Access.RulerId =
        RulerId;
    Access.bPermanentlyUnlocked =
        true;
    Access.bHasUnlockedWorldTick =
        true;
    TestTrue(
        TEXT("Persist earned permanent gacha access"),
        Store.UpsertRulerGachaAccess(
            Access,
            Error));

    TestTrue(
        TEXT("Seed one compatible ordinary ticket"),
        Store.SetResourceBalance(
            RulerId,
            TicketId,
            1,
            Error));
    TestTrue(
        TEXT("Seed pull currency"),
        Store.SetResourceBalance(
            RulerId,
            Banner.CurrencyId,
            1000,
            Error));

    FOGGachaService Service(
        Store);
    FOGGachaPullResult TicketPull;
    TestTrue(
        TEXT("Pull succeeds with compatible ticket"),
        Service.Pull(
            Banner,
            RulerId,
            10,
            111,
            TicketPull,
            Error));
    TestTrue(
        TEXT("Result records ticket payment"),
        TicketPull.bUsedTicket);
    TestTrue(
        TEXT("Result records exact ticket resource"),
        TicketPull.PaymentResourceId ==
            TicketId);

    bool bKnown = false;
    int64 Balance = -1;
    TestTrue(
        TEXT("Read ticket after pull"),
        Store.TryReadResourceBalance(
            RulerId,
            TicketId,
            bKnown,
            Balance,
            Error));
    TestTrue(
        TEXT("Compatible ticket was consumed first"),
        bKnown &&
        Balance == 0);

    bKnown = false;
    Balance = -1;
    TestTrue(
        TEXT("Read currency after ticket-funded pull"),
        Store.TryReadResourceBalance(
            RulerId,
            Banner.CurrencyId,
            bKnown,
            Balance,
            Error));
    TestTrue(
        TEXT("Currency is untouched while a ticket was available"),
        bKnown &&
        Balance == 1000);

    FOGGachaPullResult CurrencyPull;
    TestTrue(
        TEXT("Second pull falls back to currency after ticket depletion"),
        Service.Pull(
            Banner,
            RulerId,
            20,
            222,
            CurrencyPull,
            Error));
    TestFalse(
        TEXT("Fallback result is not marked ticket-funded"),
        CurrencyPull.bUsedTicket);
    TestTrue(
        TEXT("Fallback records pull currency resource"),
        CurrencyPull.PaymentResourceId ==
            Banner.CurrencyId);

    bKnown = false;
    Balance = -1;
    TestTrue(
        TEXT("Read currency after fallback pull"),
        Store.TryReadResourceBalance(
            RulerId,
            Banner.CurrencyId,
            bKnown,
            Balance,
            Error));
    TestTrue(
        TEXT("Fallback consumes configured currency cost"),
        bKnown &&
        Balance == 900);

    Store.Close();
    IFileManager::Get().DeleteDirectory(
        *Directory,
        false,
        true);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGachaD02NativeAdditiveProbabilityTest,
    "OfflineGame.Gacha.D02AdditivePityFeaturedPoolAndHonestTenPullLock",
    EAutomationTestFlags_ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)

bool FOGGachaD02NativeAdditiveProbabilityTest::RunTest(const FString&)
{
    // Self-contained, entirely noncanonical deterministic test pool.
    // Official D01 all-six MR-top rate profile totals 10,000 basis points.
    FOGGachaBannerDefinition Banner;
    Banner.BannerId = FOGContentId(TEXT("test:banner.all_six_mr_profile_A"));
    Banner.CurrencyId = FOGContentId(TEXT("test:currency.not_real"));
    Banner.PityCategory = TEXT("test:mr_limited");
    Banner.TopRarity = TEXT("MR");
    Banner.SoftPityStart = 50;
    Banner.SoftPityBonusPerPullBps = 500;
    Banner.HardPity = 70;
    Banner.bFeaturedGuaranteeAfterMiss = true;
    const FName Grades[] = {TEXT("R"),TEXT("SR"),TEXT("SSR"),
        TEXT("UR"),TEXT("LR"),TEXT("MR"),TEXT("MR")};
    const int32 Weights[] = {6500,1800,900,450,230,60,60};
    for (int32 Index=0;Index<7;++Index)
    {
        FOGGachaPoolEntry E;
        E.IdentityId = FOGContentId(FString::Printf(
            TEXT("test:character.d02_only_%d"),Index));
        E.VersionId = FOGContentId(FString::Printf(
            TEXT("test:version.d02_only_%d"),Index));
        E.Rarity = Grades[Index];
        E.Weight = Weights[Index];
        E.bFeatured = Index==5;
        Banner.Entries.Add(E);
    }
    FString Error;
    TestTrue(TEXT("Independent featured and standard top pool required"),
        FOGGachaService::ValidateBanner(Banner,Error));
    TestEqual(TEXT("Before 51 base MR chance 1.2%"),
        FOGGachaService::CalculateTopRarityChanceBps(Banner,50),120);
    TestEqual(TEXT("At 51 MR chance 1.2% +5 points =6.2%"),
        FOGGachaService::CalculateTopRarityChanceBps(Banner,51),620);
    TestEqual(TEXT("At 69 chance 96.2%"),
        FOGGachaService::CalculateTopRarityChanceBps(Banner,69),9620);
    TestEqual(TEXT("At 70 hard pity 100%"),
        FOGGachaService::CalculateTopRarityChanceBps(Banner,70),10000);
    Banner.Entries.Pop();
    TestFalse(TEXT("Reject dishonest one-sided featured 50/50 pool"),
        FOGGachaService::ValidateBanner(Banner,Error));
    FOGSQLiteWorldStore Store; // Not opened: ten-pull MUST reject before writes.
    FOGGachaService Service(Store);
    TArray<FOGGachaPullResult> Results;
    TestFalse(TEXT("Deferred SR+ ten-pull never charges or creates ownership"),
        Service.PullBatch(Banner,FOGEntityId::NewId(),0,42,10,Results,Error));
    TestTrue(TEXT("Ten-pull makes specific safety constraint visible"),
        Error.Contains(TEXT("SR+")));
    TestTrue(TEXT("Blocked ten-pull returned no Manifestations"),
        Results.IsEmpty());
    return true;
}

namespace
{
struct FNativeGachaQuoteFixture
{
    FString Directory=MakeGachaTestDirectory(),Path=FPaths::Combine(Directory,TEXT("quote.db"));
    FOGSQLiteWorldStore Store;
    FOGEntityId Ruler=FOGEntityId::NewId();
    FOGGachaBannerDefinition Banner=MakeSingleFeaturedTopBanner();
    FString Error;
    bool Open()
    {
        Banner.CompatibleTicketIds={FOGContentId(TEXT("test:ticket.first")),FOGContentId(TEXT("test:ticket.second"))};
        FOGRulerGachaAccessRecord Access; Access.RulerId=Ruler; Access.bPermanentlyUnlocked=true;
        Access.bHasUnlockedWorldTick=true;
        return Store.Open(Path,Error) && Store.UpsertEntity(Ruler,TEXT("ruler"),0,TEXT("{}"),Error) &&
            Store.UpsertRulerGachaAccess(Access,Error) && Store.SetResourceBalance(Ruler,Banner.CurrencyId,1000,Error) &&
            Store.SetResourceBalance(Ruler,Banner.CompatibleTicketIds[0],3,Error) &&
            Store.SetResourceBalance(Ruler,Banner.CompatibleTicketIds[1],2,Error);
    }
    int64 Balance(const FOGContentId& Resource)
    { bool Found=false; int64 Amount=-1; Store.TryReadResourceBalance(Ruler,Resource,Found,Amount,Error); return Amount; }
    ~FNativeGachaQuoteFixture() { Store.Close(); IFileManager::Get().DeleteDirectory(*Directory,false,true); }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGGachaReviewedPaymentTest,
    "OfflineGame.Gacha.NativeReview.ExactPaymentReadOnlyAndReplay",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGGachaReviewedPaymentTest::RunTest(const FString&)
{
    FNativeGachaQuoteFixture F; if (!F.Open()) { AddError(F.Error); return false; }
    FOGGachaService Service(F.Store); FOGGachaBatchPreview Quote,Later;
    if (!Service.PreviewBatch(F.Banner,F.Ruler,10,6,Quote,F.Error)) { AddError(F.Error); return false; }
    TestTrue(TEXT("Exact quote is confirmable"),Quote.bCanPull && !Quote.ConfirmationToken.IsEmpty());
    TestTrue(TEXT("Original ticket order precedes currency"),
        Quote.Payments.Num()==3 && Quote.Payments[0].Spend==3 && Quote.Payments[1].Spend==2 && Quote.Payments[2].Spend==100);
    bool Found=false; FOGGachaStateRecord Pity;
    TestTrue(TEXT("Review never writes an initial pity record"),
        F.Store.TryReadGachaState(F.Ruler,F.Banner.PityCategory,Found,Pity,F.Error) && !Found);
    TestEqual(TEXT("Review never charges"),F.Balance(F.Banner.CurrencyId),int64(1000));
    TestTrue(TEXT("World clock progress alone does not invalidate payment confirmation"),
        Service.PreviewBatch(F.Banner,F.Ruler,11,6,Later,F.Error) && Later.ConfirmationToken==Quote.ConfirmationToken);
    TArray<FOGGachaPullResult> Results;
    if (!TestTrue(TEXT("Reviewed payment commits real acquisition"),
        Service.PullBatchReviewed(F.Banner,F.Ruler,11,123,6,Quote.ConfirmationToken,Results,F.Error))) return false;
    TestEqual(TEXT("All actual results are retained"),Results.Num(),6);
    TestEqual(TEXT("Currency pays only the remaining pull"),F.Balance(F.Banner.CurrencyId),int64(900));
    TestEqual(TEXT("First tickets consumed"),F.Balance(F.Banner.CompatibleTicketIds[0]),int64(0));
    TestEqual(TEXT("Second tickets consumed"),F.Balance(F.Banner.CompatibleTicketIds[1]),int64(0));
    TestFalse(TEXT("Same reviewed command cannot purchase again after pity/account changes"),
        Service.PullBatchReviewed(F.Banner,F.Ruler,12,123,6,Quote.ConfirmationToken,Results,F.Error));
    TestEqual(TEXT("Replay never charges again"),F.Balance(F.Banner.CurrencyId),int64(900));
    TArray<FOGCharacterManifestationRecord> Owned;
    TestTrue(TEXT("Only the committed genuine copies remain owned"),
        F.Store.ListCharacterManifestationsByOwnerAndIdentity(F.Ruler,F.Banner.Entries[0].IdentityId,Owned,F.Error) && Owned.Num()==6);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGGachaReviewedStaleAndOuterTransactionTest,
    "OfflineGame.Gacha.NativeReview.StaleDefinitionWalletAndTransactionOwnership",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGGachaReviewedStaleAndOuterTransactionTest::RunTest(const FString&)
{
    FNativeGachaQuoteFixture F; if (!F.Open()) { AddError(F.Error); return false; }
    FOGGachaService Service(F.Store); FOGGachaBatchPreview Quote;
    if (!Service.PreviewBatch(F.Banner,F.Ruler,1,1,Quote,F.Error)) return false;
    TArray<FOGGachaPullResult> Results; auto Changed=F.Banner; ++Changed.Entries[0].Weight;
    TestFalse(TEXT("Changed original pool weights invalidate confirmation"),
        Service.PullBatchReviewed(Changed,F.Ruler,1,42,1,Quote.ConfirmationToken,Results,F.Error));
    if (!F.Store.SetResourceBalance(F.Ruler,F.Banner.CompatibleTicketIds[0],4,F.Error)) return false;
    TestFalse(TEXT("Changed displayed ticket balance invalidates confirmation"),
        Service.PullBatchReviewed(F.Banner,F.Ruler,1,42,1,Quote.ConfirmationToken,Results,F.Error));
    if (!Service.PreviewBatch(F.Banner,F.Ruler,1,1,Quote,F.Error) || !F.Store.BeginTransaction(F.Error)) return false;
    FOGGachaBatchPreview Nested;
    TestFalse(TEXT("A review cannot expose uncommitted outer writes"),
        Service.PreviewBatch(F.Banner,F.Ruler,1,1,Nested,F.Error));
    TestFalse(TEXT("A confirmed purchase cannot borrow another root transaction"),
        Service.PullBatchReviewed(F.Banner,F.Ruler,1,42,1,Quote.ConfirmationToken,Results,F.Error));
    TestTrue(TEXT("Rejection leaves the caller's outer transaction intact"),F.Store.IsWriteTransactionActive());
    F.Store.RollbackTransaction(F.Error);
    if (!Service.PreviewBatch(F.Banner,F.Ruler,1,10,Quote,F.Error)) return false;
    TestFalse(TEXT("Missing authored SR floor disables ten-draw confirmation"),Quote.bCanPull);
    return true;
}

#endif
