#if WITH_DEV_AUTOMATION_TESTS
#include "Gacha/OGSharedGachaBannerDefinition.h"
#include "Misc/AutomationTest.h"
namespace
{
FOGInstalledScheduledGachaBanner ScheduledTestBanner(const TCHAR* Id)
{
    FOGInstalledScheduledGachaBanner S; S.bAcceptedInstalledAndActivated=true;
    S.BeginsAtWorldTick=10; S.EndsBeforeWorldTick=20;
    auto& B=S.Banner; B.BannerId=FOGContentId(Id); B.PityCategory=TEXT("test");
    B.CurrencyId=FOGContentId(TEXT("test:currency")); B.PullCost=1; B.TopRarity=TEXT("UR");
    B.HardPity=1; B.bFeaturedGuaranteeAfterMiss=false;
    FOGGachaPoolEntry E; E.IdentityId=FOGContentId(TEXT("test:character"));
    E.VersionId=FOGContentId(TEXT("test:version")); E.Rarity=TEXT("UR"); E.Weight=1; B.Entries.Add(E);
    return S;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedGachaScheduleTest,
    "OfflineGame.Gacha.SharedLibrary.WorldScheduleBoundariesAndActivation",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedGachaScheduleTest::RunTest(const FString&)
{
    const auto S=ScheduledTestBanner(TEXT("test:banner")); TArray<FOGGachaBannerDefinition> Out; FString Error;
    TestTrue(TEXT("Before opening is a valid empty selection"),FOGInstalledGachaBannerLibrary::FilterSchedules({S},9,Out,Error));
    TestEqual(TEXT("No early availability"),Out.Num(),0);
    TestTrue(TEXT("Opening boundary included"),FOGInstalledGachaBannerLibrary::FilterSchedules({S},10,Out,Error));
    TestEqual(TEXT("One active source banner"),Out.Num(),1);
    TestTrue(TEXT("Closing boundary is a valid empty selection"),FOGInstalledGachaBannerLibrary::FilterSchedules({S},20,Out,Error));
    TestEqual(TEXT("No availability at or after close"),Out.Num(),0);
    auto Disabled=S; Disabled.bAcceptedInstalledAndActivated=false;
    TestTrue(TEXT("Unactivated source remains unavailable"),FOGInstalledGachaBannerLibrary::FilterSchedules({Disabled},15,Out,Error));
    TestEqual(TEXT("Registration grants no acquisition eligibility"),Out.Num(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedGachaDuplicateTest,
    "OfflineGame.Gacha.SharedLibrary.RejectAmbiguousActiveBanner",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedGachaDuplicateTest::RunTest(const FString&)
{
    const auto S=ScheduledTestBanner(TEXT("test:duplicate")); TArray<FOGGachaBannerDefinition> Out; FString Error;
    TestFalse(TEXT("Two active sources cannot publish the same identity"),FOGInstalledGachaBannerLibrary::FilterSchedules({S,S},15,Out,Error));
    TestTrue(TEXT("Ambiguous selection returns no partial banner inventory"),Out.IsEmpty());
    auto Invalid=S; Invalid.EndsBeforeWorldTick=Invalid.BeginsAtWorldTick;
    TestFalse(TEXT("Invalid activated schedule is rejected"),FOGInstalledGachaBannerLibrary::FilterSchedules({Invalid},15,Out,Error));
    return true;
}
#endif
