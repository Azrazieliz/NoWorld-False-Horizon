#include "Progression/OGCharacterDevelopmentService.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace
{
// Synthetic test-only definitions, never installed character content or canon.
FString AuthoredFixture()
{
    return TEXT(R"JSON({
      "schema": "noworld.character_development.v1",
      "offers": [{
        "development_id": "test:development.catalog_fixture",
        "identity_id": "test:identity.catalog_fixture",
        "version_id": "test:version.catalog_fixture",
        "display_name": "TEST ONLY development",
        "description": "Synthetic parser fixture; not canon.",
        "minimum_level": 5,
        "required_rarity": "R",
        "required_reinforcement_state": "fixture_before",
        "prerequisite_development_ids": [],
        "excluded_development_ids": [],
        "required_routes": [{"route_id": "test:route.fixture", "node_id": "test:node.fixture", "state": "fixture_before"}],
        "required_facts": [{"fact_key": "test.event.fixture", "display_name": "TEST ONLY event", "subject": "manifestation"}],
        "costs": [{"resource_id": "test:resource.fixture", "display_name": "TEST ONLY resource", "amount": "9007199254740993"}],
        "result_level": 6,
        "result_rarity": "SR",
        "result_reinforcement_state": "fixture_after",
        "completes_reinforcement": true,
        "stat_grants": {"attack": {"significand": "987654321", "exponent10": 1000}, "speed": {"significand": "12", "exponent10": 0}},
        "granted_routes": [{"route_id": "test:route.fixture", "node_id": "test:node.fixture", "state": "fixture_after"}],
        "granted_forms": ["test:form.fixture"]
      }]
    })JSON");
}

FOGCharacterDevelopmentDefinition GraphFixture(const TCHAR* Id)
{
    FOGCharacterDevelopmentDefinition D;
    D.DevelopmentId = FOGContentId(Id);
    D.IdentityId = FOGContentId(TEXT("test:identity.catalog_graph"));
    D.VersionId = FOGContentId(TEXT("test:version.catalog_graph"));
    D.DisplayName = TEXT("TEST ONLY graph node");
    D.ResultLevel = 2;
    return D;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentCatalogAuthoredTest,
    "OfflineGame.Progression.DevelopmentCatalog.AuthoredAndEmptyInstalled",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FOGDevelopmentCatalogAuthoredTest::RunTest(const FString&)
{
    TArray<FOGCharacterDevelopmentDefinition> Definitions;
    FString Error = TEXT("stale error");
    if (!TestTrue(TEXT("Authored fixture parses"),
        FOGCharacterDevelopmentCatalog::Parse(AuthoredFixture(), Definitions, Error)))
    {
        AddError(Error);
        return false;
    }
    if (!TestEqual(TEXT("One explicit offer"), Definitions.Num(), 1))
        return false;
    const auto& D = Definitions[0];
    if (!TestEqual(TEXT("One explicit cost"), D.Costs.Num(), 1) ||
        !TestEqual(TEXT("One canonical event requirement"), D.RequiredFacts.Num(), 1))
        return false;
    TestTrue(TEXT("Success clears old errors"), Error.IsEmpty());
    TestEqual(TEXT("Cost above double precision is exact"), D.Costs[0].Amount, 9007199254740993LL);
    const FOGLargeNumber* Attack = D.StatGrants.Find(FName(TEXT("attack")));
    if (TestNotNull(TEXT("Attack stat parsed"), Attack))
    {
        TestEqual(TEXT("Scientific significand preserved"), Attack->Significand, 987654321LL);
        TestEqual(TEXT("Large exponent preserved"), Attack->Exponent10, 1000);
    }
    TestFalse(TEXT("Manifestation event subject"), D.RequiredFacts[0].bSubjectIsRuler);
    TestTrue(TEXT("Reinforcement completion parsed"), D.bCompletesReinforcement);
    TestEqual(TEXT("Explicit level outcome"), D.ResultLevel, 6);
    const FString RankCeilingFixture = AuthoredFixture().Replace(
        TEXT("\"minimum_level\": 5"), TEXT("\"minimum_level\": 100")).Replace(
        TEXT("\"result_level\": 6"), TEXT("\"result_level\": 100"));
    TestTrue(TEXT("Canonical same-rank Level 100 is accepted"),
        FOGCharacterDevelopmentCatalog::Parse(RankCeilingFixture, Definitions, Error));
    TestTrue(TEXT("Empty document parses and clears prior output"),
        FOGCharacterDevelopmentCatalog::Parse(
            TEXT("{\"schema\":\"noworld.character_development.v1\",\"offers\":[]}"), Definitions, Error));
    TestTrue(TEXT("Empty document has no offers"), Definitions.IsEmpty());
    TestTrue(TEXT("Installed catalog loads through UFS path"),
        FOGCharacterDevelopmentCatalog::LoadInstalled(Definitions, Error));
    TestTrue(TEXT("Shipped catalog intentionally has no offers"), Definitions.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentCatalogMalformedTest,
    "OfflineGame.Progression.DevelopmentCatalog.RejectsMalformedJsonAndNumbers",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FOGDevelopmentCatalogMalformedTest::RunTest(const FString&)
{
    const FString Fixture = AuthoredFixture();
    auto Reject = [this](const TCHAR* Label, const FString& Json)
    {
        TArray<FOGCharacterDevelopmentDefinition> Out;
        Out.AddDefaulted(); // A failure must not leave old or partial offers.
        FString Error;
        TestFalse(Label, FOGCharacterDevelopmentCatalog::Parse(Json, Out, Error));
        TestTrue(TEXT("Failure clears offers"), Out.IsEmpty());
        TestFalse(TEXT("Failure explains the offending content"), Error.IsEmpty());
    };
    Reject(TEXT("Malformed syntax"), TEXT("{"));
    Reject(TEXT("Missing offers"), TEXT("{\"schema\":\"noworld.character_development.v1\"}"));
    Reject(TEXT("Offers must be an array"), TEXT("{\"schema\":\"noworld.character_development.v1\",\"offers\":{}}"));
    Reject(TEXT("Schema is exact"), Fixture.Replace(TEXT("noworld.character_development.v1"), TEXT("noworld.character_development.v2")));
    Reject(TEXT("Duplicate JSON field"), Fixture.Replace(TEXT("\"result_level\": 6"), TEXT("\"result_level\": 6, \"result_level\": 7")));
    const FString Cost = TEXT("{\"resource_id\": \"test:resource.fixture\", \"display_name\": \"TEST ONLY resource\", \"amount\": \"9007199254740993\"}");
    Reject(TEXT("Duplicate canonical resource costs"), Fixture.Replace(*Cost, *(Cost + TEXT(", ") + Cost)));
    Reject(TEXT("Numeric cost rejected"), Fixture.Replace(TEXT("\"amount\": \"9007199254740993\""), TEXT("\"amount\": 10")));
    for (const TCHAR* Invalid : {TEXT("0"), TEXT("-1"), TEXT("+1"), TEXT("01"), TEXT(" 1"),
        TEXT("1.5"), TEXT("1e3"), TEXT(""), TEXT("9223372036854775808")})
        Reject(TEXT("Malformed decimal cost"), Fixture.Replace(TEXT("9007199254740993"), Invalid));
    Reject(TEXT("Fraction cannot round to a valid level"), Fixture.Replace(TEXT("\"minimum_level\": 5"), TEXT("\"minimum_level\": 5.00000000000000001")));
    Reject(TEXT("Minimum level cannot exceed rank ceiling"), Fixture.Replace(TEXT("\"minimum_level\": 5"), TEXT("\"minimum_level\": 101")));
    Reject(TEXT("Result level cannot exceed rank ceiling"), Fixture.Replace(TEXT("\"result_level\": 6"), TEXT("\"result_level\": 101")));
    Reject(TEXT("Integer exponent must be a number"), Fixture.Replace(TEXT("\"exponent10\": 1000"), TEXT("\"exponent10\": \"1000\"")));
    Reject(TEXT("Unsafe exponent rejected"), Fixture.Replace(TEXT("\"exponent10\": 1000"), TEXT("\"exponent10\": 2147483647")));
    Reject(TEXT("Significand must be a string"), Fixture.Replace(TEXT("\"significand\": \"987654321\""), TEXT("\"significand\": 987654321")));
    Reject(TEXT("Negative stat grant rejected"), Fixture.Replace(TEXT("987654321"), TEXT("-1")));
    Reject(TEXT("Overprecise significand rejected"), Fixture.Replace(TEXT("987654321"), TEXT("1234567891")));
    Reject(TEXT("Integer stat cannot grant a fraction"), Fixture.Replace(TEXT("\"significand\": \"12\", \"exponent10\": 0"), TEXT("\"significand\": \"12\", \"exponent10\": -1")));
    Reject(TEXT("Only exact safe stat keys accepted"), Fixture.Replace(TEXT("\"attack\""), TEXT("\"Attack\"")));
    Reject(TEXT("Subjective confidence is an unknown field"), Fixture.Replace(TEXT("\"subject\": \"manifestation\""), TEXT("\"subject\": \"manifestation\", \"minimum_confidence_bps\": 10000")));
    Reject(TEXT("Unknown event subject rejected"), Fixture.Replace(TEXT("\"subject\": \"manifestation\""), TEXT("\"subject\": \"observer\"")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentCatalogNativeRowTest,
    "OfflineGame.Progression.DevelopmentCatalog.NativeRowValidation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FOGDevelopmentCatalogNativeRowTest::RunTest(const FString&)
{
    TArray<FOGCharacterDevelopmentDefinition> Parsed;
    FString Error;
    if (!FOGCharacterDevelopmentCatalog::Parse(AuthoredFixture(), Parsed, Error) || Parsed.Num() != 1 ||
        Parsed[0].Costs.Num() != 1 || Parsed[0].GrantedForms.Num() != 1 || Parsed[0].GrantedRoutes.Num() != 1)
    {
        AddError(TEXT("Test fixture must parse: ") + Error);
        return false;
    }
    FOGCharacterDevelopmentDefinition D = Parsed[0];
    const auto DuplicateCost = D.Costs[0];
    D.Costs.Add(DuplicateCost);
    TestFalse(TEXT("Native callers cannot repeat a currency/material balance"),
        FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = Parsed[0];
    D.ResultLevel = D.MinimumLevel - 1;
    TestFalse(TEXT("Result level cannot be below minimum"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = Parsed[0];
    D.MinimumLevel = 101;
    TestFalse(TEXT("Native minimum obeys rank ceiling"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = Parsed[0];
    D.ResultLevel = 101;
    TestFalse(TEXT("Native result obeys rank ceiling"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = Parsed[0];
    D.RequiredRarity = FName(TEXT("MR"));
    TestFalse(TEXT("Known rarity cannot decrease"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D.RequiredRarity = FName(TEXT("test_unknown_grade"));
    TestFalse(TEXT("Unknown rarity cannot change"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D.ResultRarity = D.RequiredRarity;
    TestTrue(TEXT("Unknown rarity may remain unchanged"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = Parsed[0];
    const auto DuplicateForm = D.GrantedForms[0];
    D.GrantedForms.Add(DuplicateForm);
    TestFalse(TEXT("Duplicate forms rejected"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = Parsed[0];
    const auto DuplicateRoute = D.GrantedRoutes[0];
    D.GrantedRoutes.Add(DuplicateRoute);
    TestFalse(TEXT("Duplicate route/node rejected"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = Parsed[0];
    FOGLargeNumber UnsafeNativeGrant;
    UnsafeNativeGrant.Significand = 1;
    UnsafeNativeGrant.Exponent10 = MIN_int32 / 4;
    D.StatGrants.Add(FName(TEXT("attack")), UnsafeNativeGrant);
    TestFalse(TEXT("Native grant normalization must stay inside safe exponents"),
        FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D = GraphFixture(TEXT("test:development.row_only"));
    D.PrerequisiteDevelopmentIds.Add(FOGContentId(TEXT("test:development.unresolved")));
    TestTrue(TEXT("Row validation does not resolve graph references"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    D.PrerequisiteDevelopmentIds.Reset();
    D.ResultLevel = 0;
    TestFalse(TEXT("Empty outcomes rejected"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentCatalogGraphTest,
    "OfflineGame.Progression.DevelopmentCatalog.GraphAndTransitiveExclusions",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FOGDevelopmentCatalogGraphTest::RunTest(const FString&)
{
    auto Root = GraphFixture(TEXT("test:development.root"));
    auto Left = GraphFixture(TEXT("test:development.left"));
    auto Right = GraphFixture(TEXT("test:development.right"));
    auto Ancestor = GraphFixture(TEXT("test:development.ancestor"));
    Root.PrerequisiteDevelopmentIds = {Left.DevelopmentId, Right.DevelopmentId};
    Left.PrerequisiteDevelopmentIds = {Ancestor.DevelopmentId};
    FString Error;
    TestTrue(TEXT("Authored prerequisite DAG is valid"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right, Ancestor}, Error));
    Right.ExcludedDevelopmentIds = {Ancestor.DevelopmentId};
    TestFalse(TEXT("One branch cannot exclude another branch's ancestor"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right, Ancestor}, Error));
    TestTrue(TEXT("Contradiction diagnostic identifies exclusion"), Error.Contains(TEXT("excludes required offer")));
    Right.ExcludedDevelopmentIds.Reset();
    Root.ExcludedDevelopmentIds = {Ancestor.DevelopmentId};
    TestFalse(TEXT("Target cannot exclude an indirect prerequisite"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right, Ancestor}, Error));
    Root.ExcludedDevelopmentIds.Reset();
    Ancestor.PrerequisiteDevelopmentIds = {Root.DevelopmentId};
    TestFalse(TEXT("Prerequisite cycle rejected"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right, Ancestor}, Error));
    Ancestor.PrerequisiteDevelopmentIds.Reset();
    TestFalse(TEXT("Missing prerequisite rejected"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right}, Error));
    Ancestor.VersionId = FOGContentId(TEXT("test:version.other"));
    TestFalse(TEXT("Cross-version prerequisite rejected"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right, Ancestor}, Error));
    Ancestor.VersionId = Left.VersionId;
    TestFalse(TEXT("Duplicate offers rejected"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right, Ancestor, Ancestor}, Error));
    Ancestor.ExcludedDevelopmentIds = {Ancestor.DevelopmentId};
    TestFalse(TEXT("Self-exclusion rejected before graph traversal"),
        FOGCharacterDevelopmentCatalog::Validate({Root, Left, Right, Ancestor}, Error));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
