#include "Progression/OGCharacterDevelopmentService.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Persistence/OGSQLiteWorldStore.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
struct FDevelopmentFixture
{
    FOGSQLiteWorldStore Store;
    FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Development"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
    FString Error;
    FOGEntityId Ruler = FOGEntityId::NewId();
    FOGEntityId CopyA = FOGEntityId::NewId();
    FOGEntityId CopyB = FOGEntityId::NewId();
    FOGContentId Identity{TEXT("fixture:character.development.identity")};
    FOGContentId Version{TEXT("fixture:character.development.version")};
    FOGContentId Resource{TEXT("fixture:resource.development")};

    bool Initialize()
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
        if (!Store.Open(FPaths::Combine(Directory, TEXT("world.sqlite")), Error) ||
            !Store.UpsertEntity(Ruler, TEXT("ruler"), 0, TEXT("{}"), Error) ||
            !Store.SetResourceBalance(Ruler, Resource, 500, Error)) return false;
        for (const auto& Id : {CopyA, CopyB})
        {
            FOGCharacterManifestationRecord M;
            M.ManifestationId = Id;
            M.OwningRulerId = Ruler;
            M.IdentityId = Identity;
            M.ActiveVersionId = Version;
            M.CurrentRarity = TEXT("SR");
            if (!Store.UpsertEntity(Id, TEXT("manifestation"), 0, TEXT("{}"), Error) ||
                !Store.UpsertCharacterManifestation(M, 0, Error)) return false;
        }
        return true;
    }
    FOGCharacterDevelopmentDefinition Definition() const
    {
        FOGCharacterDevelopmentDefinition D;
        D.DevelopmentId = FOGContentId(TEXT("fixture:development.first"));
        D.IdentityId = Identity;
        D.VersionId = Version;
        D.DisplayName = TEXT("Noncanonical development test");
        FOGDevelopmentCost Cost;
        Cost.ResourceId = Resource;
        Cost.DisplayName = TEXT("Test resource");
        Cost.Amount = 100;
        D.Costs.Add(Cost);
        D.ResultLevel = 2;
        D.RequiredRarity = TEXT("SR");
        D.ResultRarity = TEXT("SSR");
        D.ResultReinforcementState = TEXT("reinforced");
        D.StatGrants.Add(TEXT("attack"), FOGLargeNumber::FromInt64(50));
        D.StatGrants.Add(TEXT("max_hp"), FOGLargeNumber::FromInt64(200));
        return D;
    }
    ~FDevelopmentFixture()
    {
        Store.Close();
        IFileManager::Get().DeleteDirectory(*Directory, false, true);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCharacterDevelopmentTransactionTest,
    "OfflineGame.Progression.Development.OwnedAtomicIdempotentAndPersistent",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FOGCharacterDevelopmentTransactionTest::RunTest(const FString& Parameters)
{
    FDevelopmentFixture F;
    if (!TestTrue(TEXT("Canonical development fixture"), F.Initialize())) return false;
    FOGCharacterDevelopmentService Service(F.Store);
    const auto D = F.Definition();
    FOGCharacterDevelopmentPreview Quote;
    if (!TestTrue(TEXT("Preview owned upgrade"), Service.Preview(F.Ruler, F.CopyA, D, Quote, F.Error))) return false;
    TestTrue(TEXT("Resources satisfy authored cost"), Quote.bCanApply);
    TestEqual(TEXT("Exact cost preview"), Quote.Costs[0].Balance, int64(500));
    const FOGEntityId Request = FOGEntityId::NewId();
    bool Already = false;
    TestFalse(TEXT("Another ruler cannot spend the owner's resources"),
        Service.Apply(FOGEntityId::NewId(), F.CopyA, D, Quote.ConfirmationToken, Request, 1, Already, F.Error));
    TestTrue(TEXT("Confirm commits development"), Service.Apply(F.Ruler, F.CopyA, D, Quote.ConfirmationToken, Request, 1, Already, F.Error));
    TestFalse(TEXT("First application is new"), Already);
    TestTrue(TEXT("Same request is idempotent"), Service.Apply(F.Ruler, F.CopyA, D, Quote.ConfirmationToken, Request, 1, Already, F.Error));
    TestTrue(TEXT("Retry returns existing receipt"), Already);
    bool Found = false;
    int64 Balance = 0;
    F.Store.TryReadResourceBalance(F.Ruler, F.Resource, Found, Balance, F.Error);
    TestEqual(TEXT("Resource charged exactly once"), Balance, int64(400));
    FOGCharacterManifestationRecord M;
    F.Store.TryReadCharacterManifestation(F.CopyA, Found, M, F.Error);
    TestEqual(TEXT("Level actually advances"), M.Level, 2);
    TestEqual(TEXT("Rarity actually advances"), M.CurrentRarity, FName(TEXT("SSR")));
    F.Store.TryReadCharacterManifestation(F.CopyB, Found, M, F.Error);
    TestEqual(TEXT("Independent duplicate keeps its level"), M.Level, 1);
    TestEqual(TEXT("Independent duplicate keeps its rarity"), M.CurrentRarity, FName(TEXT("SR")));
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = F.CopyA;
    Unit.IdentityId = F.Identity;
    Unit.SkillSet.CurrentForm = F.Version;
    Unit.Stats.Attack = FOGLargeNumber::FromInt64(100);
    Unit.Stats.MaxHp = FOGLargeNumber::FromInt64(1000);
    Unit.CurrentHp = FOGLargeNumber::FromInt64(600);
    TestTrue(TEXT("World/turn stat projection"), Service.ProjectCombatUnit(Unit, F.Error));
    TestTrue(TEXT("Earned attack affects actual combat stats"), Unit.Stats.Attack == FOGLargeNumber::FromInt64(150));
    TestTrue(TEXT("Earned Max HP affects actual combat capacity"), Unit.Stats.MaxHp == FOGLargeNumber::FromInt64(1200));
    TestTrue(TEXT("Upgrade projection does not heal wounds"), Unit.CurrentHp == FOGLargeNumber::FromInt64(600));
    TestTrue(TEXT("Refresh is valid"), Service.ProjectCombatUnit(Unit, F.Error));
    TestTrue(TEXT("Refresh does not stack bonuses"), Unit.Stats.Attack == FOGLargeNumber::FromInt64(150));
    Unit.CurrentHp = FOGLargeNumber::FromInt64(1400);
    TestTrue(TEXT("Refresh authored overflow HP"), Service.ProjectCombatUnit(Unit, F.Error));
    TestTrue(TEXT("Development preserves overflow HP"), Unit.CurrentHp == FOGLargeNumber::FromInt64(1400));
    Unit.Stats.Attack = FOGLargeNumber::FromInt64(300);
    Unit.bHasDevelopmentStatProjection = false; // fresh content/equipment result
    TestTrue(TEXT("Project fresh equipment build"), Service.ProjectCombatUnit(Unit, F.Error));
    TestTrue(TEXT("Fresh equipment is retained with earned grants"), Unit.Stats.Attack == FOGLargeNumber::FromInt64(350));
    F.Store.Close();
    if (!TestTrue(TEXT("Reopen real canonical save"), F.Store.Open(FPaths::Combine(F.Directory, TEXT("world.sqlite")), F.Error))) return false;
    FOGCombatStats Reloaded;
    Reloaded.Attack = FOGLargeNumber::FromInt64(100);
    TestTrue(TEXT("Persisted grants restore after restart"), FOGCharacterDevelopmentService(F.Store).ApplyEarnedStats(F.CopyA, Reloaded, F.Error));
    TestTrue(TEXT("Restored strength is identical"), Reloaded.Attack == FOGLargeNumber::FromInt64(150));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCharacterDevelopmentStaleAndProofTest,
    "OfflineGame.Progression.Development.StaleQuotesAndCanonicalProofs",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FOGCharacterDevelopmentStaleAndProofTest::RunTest(const FString& Parameters)
{
    FDevelopmentFixture F;
    if (!TestTrue(TEXT("Canonical fixture"), F.Initialize())) return false;
    auto D = F.Definition();
    FOGCharacterDevelopmentService Service(F.Store);
    FOGCharacterDevelopmentPreview Quote;
    TestTrue(TEXT("Preview before wallet change"), Service.Preview(F.Ruler, F.CopyA, D, Quote, F.Error));
    F.Store.SetResourceBalance(F.Ruler, F.Resource, 450, F.Error);
    bool Already = false;
    TestFalse(TEXT("Changed balance requires renewed confirmation"), Service.Apply(F.Ruler, F.CopyA, D,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 1, Already, F.Error));
    bool Found = false;
    int64 Balance = 0;
    F.Store.TryReadResourceBalance(F.Ruler, F.Resource, Found, Balance, F.Error);
    TestEqual(TEXT("Rejected quote consumes nothing"), Balance, int64(450));
    FOGDevelopmentFactRequirement Proof;
    Proof.FactKey = TEXT("fixture.victory_recorded");
    Proof.DisplayName = TEXT("Recorded victory required");
    D.RequiredFacts.Add(Proof);
    TestTrue(TEXT("Project unmet proof"), Service.Preview(F.Ruler, F.CopyA, D, Quote, F.Error));
    TestFalse(TEXT("No invented feat from UI"), Quote.bCanApply);
    FOGKnowledgeFactRecord Rumor;
    Rumor.OwnerEntityId = F.Ruler;
    Rumor.SubjectEntityId = F.CopyA;
    Rumor.FactKey = Proof.FactKey;
    Rumor.ValueJson = TEXT("{\"victory\":true}");
    F.Store.UpsertKnowledgeFact(Rumor, F.Error);
    Service.Preview(F.Ruler, F.CopyA, D, Quote, F.Error);
    TestFalse(TEXT("Belief cannot substitute for physical proof"), Quote.bCanApply);
    FOGWorldEvent Victory;
    Victory.EventId = FOGEntityId::NewId();
    Victory.PrimaryEntity = F.CopyA;
    Victory.EventType = Proof.FactKey;
    Victory.WorldTick = 1;
    Victory.PayloadJson = TEXT("{}");
    F.Store.AppendWorldEvent(Victory, F.Error);
    TestTrue(TEXT("Read real proof"), Service.Preview(F.Ruler, F.CopyA, D, Quote, F.Error));
    TestTrue(TEXT("Canonical victory unlocks authored choice"), Quote.bCanApply);
    auto Changed = D;
    Changed.StatGrants[TEXT("attack")] = FOGLargeNumber::FromInt64(90);
    TestFalse(TEXT("Content changed after review is rejected"), Service.Apply(F.Ruler, F.CopyA, Changed,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 2, Already, F.Error));
    FOGEntityRankStateRecord Rank;
    Rank.EntityId = F.CopyA;
    Rank.AttainedRankId = FOGContentId(TEXT("fixture:rank.test"));
    Rank.PeakRankId = Rank.AttainedRankId;
    TestTrue(TEXT("Record created after quote"), F.Store.UpsertEntityRankState(Rank, F.Error));
    TestFalse(TEXT("New rank record invalidates quote"), Service.Apply(F.Ruler, F.CopyA, D,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 2, Already, F.Error));
    Service.Preview(F.Ruler, F.CopyA, D, Quote, F.Error);
    Rank.AttainedLevel = 3;
    Rank.PeakLevel = 3;
    Rank.UpdatedWorldTick = 2;
    F.Store.UpsertEntityRankState(Rank, F.Error);
    TestFalse(TEXT("Changed rank state invalidates quote"), Service.Apply(F.Ruler, F.CopyA, D,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 3, Already, F.Error));
    return true;
}
#endif
