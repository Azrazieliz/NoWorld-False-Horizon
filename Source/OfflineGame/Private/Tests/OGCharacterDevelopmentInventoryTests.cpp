#include "Progression/OGCharacterDevelopmentService.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Content/OGDefinitionLibrary.h"
#include "Objects/OGObjectActionDefinition.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
// Isolated automation data. No fixture bundle is saved, cooked or used as canon.
struct FDevelopmentInventoryFixture
{
    FOGSQLiteWorldStore Store;
    FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/DevelopmentInventory") /
        FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString Database = Directory / TEXT("world.sqlite");
    FString Error;
    FOGEntityId Ruler = FOGEntityId::NewId(), Copy = FOGEntityId::NewId();
    FOGEntityId OtherCopy = FOGEntityId::NewId(), Stranger = FOGEntityId::NewId();
    UOGDefinitionLibrary* Library = nullptr;
    UOGDefinitionBundle* Bundle = nullptr;
    FOGCharacterDevelopmentDefinition Offer;

    bool Initialize()
    {
        Library = UOGDefinitionLibrary::Get();
        if (!Library) return false;
        const FString Tag = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        auto Id = [&Tag](const TCHAR* Kind)
            { return FOGContentId(FString(TEXT("test:development_inventory.")) + Kind + TEXT("_") + Tag); };
        const FOGContentId Object = Id(TEXT("object")), Resource = Id(TEXT("resource"));
        const FOGContentId Identity = Id(TEXT("identity")), Version = Id(TEXT("version"));
        const FOGContentId Development = Id(TEXT("offer")), Rank = Id(TEXT("rank")), Quality = Id(TEXT("quality"));
        auto* Package = CreatePackage(*(TEXT("/Game/NoWorld/Definitions/Automation/DevelopmentInventory_") + Tag));
        auto* Material = NewObject<UOGObjectDefinition>(Package, TEXT("Material"), RF_Public | RF_Standalone);
        Material->DefinitionId = Object;
        Material->DisplayName = FText::FromString(TEXT("Automation material"));
        Material->OriginalSourceReference = TEXT("Isolated transaction regression fixture; not game content");
        Material->InitialRankId = Rank;
        Material->InitialQualityId = Quality;
        Bundle = NewObject<UOGDefinitionBundle>(Package, TEXT("Bundle"), RF_Public | RF_Standalone);
        Bundle->BundleId = Id(TEXT("bundle"));
        FOGDefinitionAssetEntry Entry;
        Entry.System = Material->System;
        Entry.Id = Material->DefinitionId;
        Entry.Asset = Material;
        Bundle->Assets.Add(Entry);
        FOGDefinitionCatalogFragment Fragment;
        Fragment.System = TEXT("development");
        Fragment.Json = FString::Printf(TEXT(R"JSON({
          "schema":"noworld.character_development.v1",
          "offers":[{
            "development_id":"%s","identity_id":"%s","version_id":"%s",
            "display_name":"Automation reinforcement",
            "costs":[{"resource_id":"%s","display_name":"Automation resource","amount":"100"}],
            "item_costs":[{"object_id":"%s","rank_id":"%s","quality_id":"%s","instance_count":2}],
            "result_level":2,"result_reinforcement_state":"fixture_reinforced",
            "stat_grants":{"attack":{"significand":"50","exponent10":0}}
          }]
        })JSON"), *Development.ToString(), *Identity.ToString(), *Version.ToString(),
            *Resource.ToString(), *Object.ToString(), *Rank.ToString(), *Quality.ToString());
        Bundle->Catalogs.Add(Fragment);
        if (!Library->InstallBundle(Bundle, Error)) return false;
        TArray<FOGCharacterDevelopmentDefinition> Installed;
        if (!FOGCharacterDevelopmentCatalog::LoadInstalled(Installed, Error)) return false;
        const auto* Found = Installed.FindByPredicate([&](const auto& D) { return D.DevelopmentId == Development; });
        if (!Found) return false;
        Offer = *Found;
        IFileManager::Get().MakeDirectory(*Directory, true);
        if (!Store.Open(Database, Error) ||
            !Store.UpsertEntity(Ruler, TEXT("ruler"), 0, TEXT("{}"), Error) ||
            !Store.UpsertEntity(Stranger, TEXT("ruler"), 0, TEXT("{}"), Error) ||
            !Store.SetResourceBalance(Ruler, Resource, 500, Error)) return false;
        for (const auto& CopyId : {Copy, OtherCopy})
        {
            FOGCharacterManifestationRecord M;
            M.ManifestationId = CopyId;
            M.OwningRulerId = Ruler;
            M.IdentityId = Identity;
            M.ActiveVersionId = Version;
            M.CurrentRarity = TEXT("SR");
            if (!Store.UpsertEntity(CopyId, TEXT("manifestation"), 0, TEXT("{}"), Error) ||
                !Store.UpsertCharacterManifestation(M, 0, Error)) return false;
        }
        return true;
    }
    bool AddItem(const FOGEntityId& Owner, FOGItemInstanceRecord& Item)
    {
        Item.ItemId = FOGEntityId::NewId();
        Item.OwnerEntityId = Owner;
        Item.DefinitionId = Offer.ItemCosts[0].ObjectId;
        Item.CurrentRankId = Offer.ItemCosts[0].RankId;
        Item.QualityId = Offer.ItemCosts[0].QualityId;
        Item.HistoryStateJson = TEXT("{\"fixture_provenance\":\"preserved\"}");
        return Store.UpsertItemInstance(Item, 0, Error);
    }
    int64 Balance()
    {
        bool Found = false;
        int64 Value = -1;
        if (!Store.TryReadResourceBalance(Ruler, Offer.Costs[0].ResourceId, Found, Value, Error) || !Found) return -1;
        return Value;
    }
    ~FDevelopmentInventoryFixture()
    {
        if (Library && Bundle)
        {
            FString Ignored;
            Library->RemoveBundle(Bundle->BundleId, Ignored);
        }
        Store.Close();
        IFileManager::Get().DeleteDirectory(*Directory, false, true);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentInventoryBundleTransactionTest,
    "OfflineGame.Progression.Development.Inventory.BundleDiscoveryAtomicReplayAndReopen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGDevelopmentInventoryBundleTransactionTest::RunTest(const FString&)
{
    FDevelopmentInventoryFixture F;
    if (!TestTrue(TEXT("Install isolated object and development fragment"), F.Initialize())) return false;
    FOGItemInstanceRecord A, B, Equipped, Foreign, WrongQuality;
    if (!F.AddItem(F.Ruler, A) || !F.AddItem(F.OtherCopy, B) ||
        !F.AddItem(F.Ruler, Equipped) || !F.AddItem(F.Stranger, Foreign) || !F.AddItem(F.Ruler, WrongQuality)) return false;
    WrongQuality.QualityId = FOGContentId(TEXT("test:development_inventory.other_quality"));
    if (!F.Store.UpsertItemInstance(WrongQuality, 0, F.Error)) return false;
    FOGEquipmentBindingRecord Binding;
    Binding.WearerEntityId = F.Copy;
    Binding.SlotId = FOGContentId(TEXT("test:development_inventory.slot"));
    Binding.ItemId = Equipped.ItemId;
    if (!F.Store.UpsertEquipmentBinding(Binding, F.Error)) return false;
    FOGCharacterDevelopmentService Service(F.Store);
    FOGCharacterDevelopmentPreview Quote;
    if (!TestTrue(TEXT("Discover usable offer without character-specific code"), Service.Preview(F.Ruler, F.Copy, F.Offer, Quote, F.Error))) return false;
    if (!TestTrue(TEXT("Actual item and resource costs are available"), Quote.bCanApply) ||
        !TestEqual(TEXT("One physical input"), Quote.ItemCosts.Num(), 1)) return false;
    TestEqual(TEXT("Only exact unequipped owned instances count"), Quote.ItemCosts[0].AvailableInstances, 2);
    TestEqual(TEXT("Two exact instances selected"), Quote.ItemCosts[0].SelectedInstances.Num(), 2);
    const auto Request = FOGEntityId::NewId();
    bool Already = false;
    if (!TestTrue(TEXT("Confirm atomic strengthening"), Service.Apply(F.Ruler, F.Copy, F.Offer,
        Quote.ConfirmationToken, Request, 1, Already, F.Error))) return false;
    TestFalse(TEXT("First request is new"), Already);
    TestTrue(TEXT("Second tap reuses durable receipt"), Service.Apply(F.Ruler, F.Copy, F.Offer,
        Quote.ConfirmationToken, Request, 1, Already, F.Error) && Already);
    TestFalse(TEXT("Different request cannot repeat the same offer"), Service.Apply(F.Ruler, F.Copy, F.Offer,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 1, Already, F.Error));
    TestEqual(TEXT("Currency charged once"), F.Balance(), int64(400));
    F.Store.Close();
    if (!TestTrue(TEXT("Reopen durable world"), F.Store.Open(F.Database, F.Error))) return false;
    for (const auto& Original : {A, B})
    {
        bool Found = false;
        FOGItemInstanceRecord Saved;
        if (!F.Store.TryReadItemInstance(Original.ItemId, Found, Saved, F.Error) || !Found) return false;
        TestFalse(TEXT("Consumed original is no longer owned"), Saved.OwnerEntityId.IsValid());
        TestTrue(TEXT("Provenance survives consumption"), Saved.HistoryStateJson.Contains(TEXT("fixture_provenance")) &&
            Saved.HistoryStateJson.Contains(Request.ToString()));
    }
    for (const auto& Original : {Equipped, Foreign, WrongQuality})
    {
        bool Found = false;
        FOGItemInstanceRecord Saved;
        if (!F.Store.TryReadItemInstance(Original.ItemId, Found, Saved, F.Error) || !Found) return false;
        TestTrue(TEXT("Excluded items retain ownership"), Saved.OwnerEntityId == Original.OwnerEntityId);
    }
    bool Found = false;
    FOGCharacterManifestationRecord M;
    if (!F.Store.TryReadCharacterManifestation(F.Copy, Found, M, F.Error) || !Found) return false;
    TestEqual(TEXT("Strengthening survives reopen"), M.Level, 2);
    FOGCombatStats Stats;
    Stats.Attack = FOGLargeNumber::FromInt64(100);
    TestTrue(TEXT("Durable grant projects after restart"), Service.ApplyEarnedStats(F.Copy, Stats, F.Error) &&
        Stats.Attack == FOGLargeNumber::FromInt64(150));
    TestEqual(TEXT("Durable wallet remains charged once"), F.Balance(), int64(400));
    TestTrue(TEXT("Retry after restart remains idempotent"), Service.Apply(F.Ruler, F.Copy, F.Offer,
        Quote.ConfirmationToken, Request, 2, Already, F.Error) && Already);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentInventoryStaleTest,
    "OfflineGame.Progression.Development.Inventory.StaleOwnershipModifiersAndInsufficientAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGDevelopmentInventoryStaleTest::RunTest(const FString&)
{
    FDevelopmentInventoryFixture F;
    if (!F.Initialize()) return false;
    FOGItemInstanceRecord A, B;
    if (!F.AddItem(F.Ruler, A) || !F.AddItem(F.Ruler, B)) return false;
    FOGCharacterDevelopmentService Service(F.Store);
    FOGCharacterDevelopmentPreview Quote;
    if (!Service.Preview(F.Ruler, F.Copy, F.Offer, Quote, F.Error)) return false;
    FOGItemModifierRecord Modifier;
    Modifier.ItemId = A.ItemId;
    Modifier.ModifierId = FOGContentId(TEXT("test:development_inventory.changed_modifier"));
    if (!F.Store.UpsertItemModifier(Modifier, F.Error)) return false;
    bool Already = false;
    TestFalse(TEXT("Changed selected-item modifier invalidates review"), Service.Apply(F.Ruler, F.Copy, F.Offer,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 1, Already, F.Error));
    if (!Service.Preview(F.Ruler, F.Copy, F.Offer, Quote, F.Error)) return false;
    A.OwnerEntityId = F.Stranger;
    if (!F.Store.UpsertItemInstance(A, 1, F.Error)) return false;
    TestFalse(TEXT("Ownership transfer invalidates review"), Service.Apply(F.Ruler, F.Copy, F.Offer,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 1, Already, F.Error));
    TestEqual(TEXT("Stale reviews never debit"), F.Balance(), int64(500));
    if (!Service.Preview(F.Ruler, F.Copy, F.Offer, Quote, F.Error)) return false;
    TestFalse(TEXT("Missing instance blocks confirmation"), Quote.bCanApply);
    A.OwnerEntityId = F.Ruler;
    if (!F.Store.UpsertItemInstance(A, 1, F.Error) ||
        !F.Store.SetResourceBalance(F.Ruler, F.Offer.Costs[0].ResourceId, 99, F.Error) ||
        !Service.Preview(F.Ruler, F.Copy, F.Offer, Quote, F.Error)) return false;
    TestFalse(TEXT("Physical inputs cannot substitute for insufficient currency"), Quote.bCanApply);
    TestFalse(TEXT("Insufficient assets reject command"), Service.Apply(F.Ruler, F.Copy, F.Offer,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 2, Already, F.Error));
    bool Found = false;
    FOGItemInstanceRecord Saved;
    if (!F.Store.TryReadItemInstance(B.ItemId, Found, Saved, F.Error) || !Found) return false;
    TestTrue(TEXT("Failed confirmation leaves inputs owned"), Saved.OwnerEntityId == F.Ruler);
    TestEqual(TEXT("Insufficient currency is untouched"), F.Balance(), int64(99));
    // Destination state can also change independently after a valid review.
    if (!F.Store.SetResourceBalance(F.Ruler, F.Offer.Costs[0].ResourceId, 500, F.Error)) return false;
    auto WithUnlocks = F.Offer;
    FOGDevelopmentRouteRequirement Grant;
    Grant.RouteId = FOGContentId(TEXT("test:development_inventory.route"));
    Grant.NodeId = FOGContentId(TEXT("test:development_inventory.node"));
    Grant.State = TEXT("completed");
    WithUnlocks.GrantedRoutes.Add(Grant);
    const FOGContentId FormId(TEXT("test:development_inventory.form"));
    WithUnlocks.GrantedForms.Add(FormId);
    if (!Service.Preview(F.Ruler, F.Copy, WithUnlocks, Quote, F.Error) || !Quote.bCanApply) return false;
    FOGManifestationRouteNodeRecord Route;
    Route.ManifestationId = F.Copy;
    Route.RouteId = Grant.RouteId;
    Route.NodeId = Grant.NodeId;
    if (!F.Store.UpsertManifestationRouteNode(Route, F.Error)) return false;
    TestFalse(TEXT("Changed destination route invalidates review"), Service.Apply(F.Ruler, F.Copy, WithUnlocks,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 2, Already, F.Error));
    if (!Service.Preview(F.Ruler, F.Copy, WithUnlocks, Quote, F.Error) || !Quote.bCanApply) return false;
    FOGManifestationFormRecord Form;
    Form.ManifestationId = F.Copy;
    Form.FormId = FormId;
    Form.State = TEXT("locked");
    if (!F.Store.UpsertManifestationForm(Form, F.Error)) return false;
    TestFalse(TEXT("Changed destination form invalidates review"), Service.Apply(F.Ruler, F.Copy, WithUnlocks,
        Quote.ConfirmationToken, FOGEntityId::NewId(), 2, Already, F.Error));
    TestEqual(TEXT("Rejected state previews did not debit"), F.Balance(), int64(500));
    auto Missing = F.Offer;
    Missing.ItemCosts[0].ObjectId = FOGContentId(TEXT("test:development_inventory.uninstalled_object"));
    if (!Service.Preview(F.Ruler, F.Copy, Missing, Quote, F.Error)) return false;
    TestFalse(TEXT("Missing definition is an explicit dependency"), Quote.bCanApply);
    TestTrue(TEXT("Dependency diagnostic identifies the object"), FString::Join(Quote.MissingRequirements, TEXT(" ")).Contains(
        Missing.ItemCosts[0].ObjectId.ToString()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentInventoryRollbackTest,
    "OfflineGame.Progression.Development.Inventory.LateFailureRollsBackAllDebits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGDevelopmentInventoryRollbackTest::RunTest(const FString&)
{
    FDevelopmentInventoryFixture F;
    if (!F.Initialize()) return false;
    FOGItemInstanceRecord A, B;
    if (!F.AddItem(F.Ruler, A) || !F.AddItem(F.Ruler, B)) return false;
    FOGInventoryContainerRecord Container;
    Container.ContainerId = FOGEntityId::NewId();
    Container.OwnerEntityId = F.Ruler;
    Container.ContainerTypeId = FOGContentId(TEXT("test:development_inventory.container"));
    if (!F.Store.UpsertInventoryContainer(Container, 0, F.Error)) return false;
    FOGContainerContentRecord Content;
    Content.ContainerId = Container.ContainerId;
    Content.ItemId = A.ItemId;
    if (!F.Store.UpsertContainerContent(Content, F.Error)) return false;
    FOGManifestationReinforcementRecord Reinforcement;
    Reinforcement.ManifestationId = F.Copy;
    Reinforcement.UpdatedWorldTick = 10;
    if (!F.Store.UpsertManifestationReinforcement(Reinforcement, F.Error)) return false;
    FOGCharacterDevelopmentService Service(F.Store);
    FOGCharacterDevelopmentPreview Quote;
    if (!Service.Preview(F.Ruler, F.Copy, F.Offer, Quote, F.Error) || !Quote.bCanApply) return false;
    const auto Request = FOGEntityId::NewId();
    bool Already = false;
    TestFalse(TEXT("History-time failure after inventory writes rejects transaction"), Service.Apply(
        F.Ruler, F.Copy, F.Offer, Quote.ConfirmationToken, Request, 1, Already, F.Error));
    TestEqual(TEXT("Earlier currency debit rolled back"), F.Balance(), int64(500));
    for (const auto& Original : {A, B})
    {
        bool Found = false;
        FOGItemInstanceRecord Saved;
        if (!F.Store.TryReadItemInstance(Original.ItemId, Found, Saved, F.Error) || !Found) return false;
        TestTrue(TEXT("Earlier consumption rolled back"), Saved.OwnerEntityId == Original.OwnerEntityId &&
            Saved.HistoryStateJson == Original.HistoryStateJson);
    }
    TArray<FOGContainerContentRecord> Contents;
    if (!F.Store.ListContainerContents(Container.ContainerId, Contents, F.Error)) return false;
    TestTrue(TEXT("Container link rolled back"), Contents.ContainsByPredicate([&](const auto& C) { return C.ItemId == A.ItemId; }));
    bool Found = false;
    FOGCharacterManifestationRecord M;
    if (!F.Store.TryReadCharacterManifestation(F.Copy, Found, M, F.Error) || !Found) return false;
    TestEqual(TEXT("Failed command did not strengthen"), M.Level, 1);
    FOGWorldEvent Event;
    if (!F.Store.TryReadWorldEvent(Request, Found, Event, F.Error)) return false;
    TestFalse(TEXT("Failed command has no success receipt"), Found);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGDevelopmentInventoryCatalogTest,
    "OfflineGame.Progression.Development.Inventory.CatalogRejectsAmbiguousInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGDevelopmentInventoryCatalogTest::RunTest(const FString&)
{
    FDevelopmentInventoryFixture F;
    if (!F.Initialize()) return false;
    const FString Json = F.Bundle->Catalogs[0].Json;
    auto Reject = [this](const FString& Text)
    {
        TArray<FOGCharacterDevelopmentDefinition> Out;
        FString Error;
        TestFalse(TEXT("Malformed inventory cost rejected"), FOGCharacterDevelopmentCatalog::Parse(Text, Out, Error));
        TestTrue(TEXT("Malformed offer publishes no partial rows"), Out.IsEmpty() && !Error.IsEmpty());
    };
    Reject(Json.Replace(TEXT("\"instance_count\":2"), TEXT("\"instance_count\":0")));
    Reject(Json.Replace(TEXT("\"instance_count\":2"), TEXT("\"instance_count\":1.5")));
    Reject(Json.Replace(TEXT("\"instance_count\":2"), TEXT("\"instance_count\":2147483648")));
    Reject(Json.Replace(TEXT("\"instance_count\":2"), TEXT("\"instance_count\":\"2\"")));
    auto D = F.Offer;
    const auto Duplicate = D.ItemCosts[0];
    D.ItemCosts.Add(Duplicate);
    TestFalse(TEXT("One instance selector cannot be charged twice"), FOGCharacterDevelopmentCatalog::ValidateDefinition(D, F.Error));
    return true;
}
#endif
