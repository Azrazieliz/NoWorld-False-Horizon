#if WITH_DEV_AUTOMATION_TESTS
#include "Content/OGDefinitionLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDefinitionCatalogPrecisionTest,
    "OfflineGame.Definitions.Catalogs.PreserveExactNumbers",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGDefinitionCatalogPrecisionTest::RunTest(const FString&)
{
    FString Out,Error;
    const FString Base=TEXT("{\"schema\":\"test.v1\",\"entries\":[{\"cost\":9223372036854775807}]}");
    const FString Add=TEXT("{\"schema\":\"test.v1\",\"entries\":[{\"magnitude\":\"7.123456789012345e900\"}]}");
    TestTrue(TEXT("Merge complete catalogs"),UOGDefinitionLibrary::MergeCatalogDocuments(Base,{Add},{TEXT("entries")},Out,Error));
    TestTrue(TEXT("int64 amount remains exact text"),Out.Contains(TEXT("9223372036854775807")));
    TestTrue(TEXT("large-number representation remains exact"),Out.Contains(TEXT("7.123456789012345e900")));
    TestTrue(TEXT("both records preserved"),Out.Contains(TEXT("},{")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDefinitionCatalogRejectionTest,
    "OfflineGame.Definitions.Catalogs.RejectAmbiguousOrMismatchedDocuments",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGDefinitionCatalogRejectionTest::RunTest(const FString&)
{
    FString Out,Error;
    const FString Base=TEXT("{\"schema\":\"test.v1\",\"entries\":[]}");
    TestFalse(TEXT("Duplicate nested keys rejected"),
        UOGDefinitionLibrary::MergeCatalogDocuments(Base,{TEXT("{\"schema\":\"test.v1\",\"entries\":[{\"id\":1,\"id\":2}]}")},{TEXT("entries")},Out,Error));
    TestTrue(TEXT("Failed merge publishes no document"),Out.IsEmpty());
    TestFalse(TEXT("Schema mismatch rejected"),
        UOGDefinitionLibrary::MergeCatalogDocuments(Base,{TEXT("{\"schema\":\"other.v1\",\"entries\":[]}")},{TEXT("entries")},Out,Error));
    TestFalse(TEXT("Unsupported root fields cannot disappear during merge"),
        UOGDefinitionLibrary::MergeCatalogDocuments(Base,{TEXT("{\"schema\":\"test.v1\",\"entries\":[],\"other\":1}")},{TEXT("entries")},Out,Error));
    TestFalse(TEXT("Object cannot masquerade as array"),
        UOGDefinitionLibrary::MergeCatalogDocuments(Base,{TEXT("{\"schema\":\"test.v1\",\"entries\":{}}")},{TEXT("entries")},Out,Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDefinitionCatalogMetadataTest,
    "OfflineGame.Definitions.Catalogs.PreserveOriginalSourceCalibration",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGDefinitionCatalogMetadataTest::RunTest(const FString&)
{
    FString Out,Error;
    const FString Base=TEXT("{\"schema\":\"test.source.v1\",\"calibration_basis\":\"original\",\"coverage\":{\"exact\":9223372036854775807},\"clips\":[],\"routes\":[]}");
    const FString Add=TEXT("{\"schema\":\"test.source.v1\",\"clips\":[{\"id\":\"source\"}],\"routes\":[]}");
    const TArray<FString> Arrays={TEXT("clips"),TEXT("routes")},Metadata={TEXT("calibration_basis"),TEXT("coverage")};
    TestTrue(TEXT("New source records use the original calibration metadata"),
        UOGDefinitionLibrary::MergeCatalogDocuments(Base,{Add},Arrays,Out,Error,Metadata));
    TestTrue(TEXT("Original calibration is retained"),Out.Contains(TEXT("\"calibration_basis\":\"original\"")));
    TestTrue(TEXT("Provenance numbers never pass through floating point"),Out.Contains(TEXT("9223372036854775807")));
    TestFalse(TEXT("A source fragment cannot replace calibration"),
        UOGDefinitionLibrary::MergeCatalogDocuments(Base,{TEXT("{\"schema\":\"test.source.v1\",\"calibration_basis\":\"different\",\"clips\":[],\"routes\":[]}")},Arrays,Out,Error,Metadata));
    TestTrue(TEXT("Failed metadata composition publishes no partial catalog"),Out.IsEmpty());
    return true;
}
#endif
