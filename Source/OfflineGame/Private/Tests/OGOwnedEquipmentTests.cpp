#include "World/OGOwnedEquipmentService.h"
#include "World/OGInventoryEquipmentService.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "Progression/OGCharacterDevelopmentService.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
struct FOwnedEquipmentFixture
{
    FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation") /
        FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString Database = Directory / TEXT("owned-equipment.db");
    FOGSQLiteWorldStore Store;
    FString Error;
    FOGEntityId Ruler = FOGEntityId::NewId();
    FOGCharacterManifestationRecord Copy;
    FOGEntityId Item, OtherItem;
    FOGContentId Slot = FOGContentId(TEXT("test:equipment.slot"));
    FOGEquipmentCatalog Catalog;
    bool Open()
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
        Copy.ManifestationId = FOGEntityId::NewId();
        Copy.OwningRulerId = Ruler;
        Copy.IdentityId = FOGContentId(TEXT("test:equipment.identity"));
        Copy.ActiveVersionId = FOGContentId(TEXT("test:equipment.version"));
        FOGAuthoredEquipmentSlot S;
        S.IdentityId = Copy.IdentityId; S.VersionId = Copy.ActiveVersionId;
        S.SlotId = Slot; S.DisplayName = TEXT("Test authored slot");
        Catalog.Slots.Add(S);
        FOGAuthoredEquipmentItem D;
        D.DefinitionId = FOGContentId(TEXT("test:equipment.item")); D.DisplayName = TEXT("Test authored item");
        Catalog.Items.Add(D);
        FOGAuthoredEquipmentRule R;
        R.IdentityId = Copy.IdentityId; R.VersionId = Copy.ActiveVersionId;
        R.SlotId = Slot; R.DefinitionId = D.DefinitionId;
        R.StatBonuses.Add(TEXT("attack"), FOGLargeNumber::FromInt64(10));
        R.StatBonuses.Add(TEXT("max_hp"), FOGLargeNumber::FromInt64(50));
        R.StatBonuses.Add(TEXT("speed"), FOGLargeNumber::FromInt64(3));
        Catalog.Rules.Add(R);
        return Store.Open(Database, Error) &&
            Store.UpsertEntity(Ruler, TEXT("ruler"), 0, TEXT("{}"), Error) &&
            Store.UpsertCharacterManifestation(Copy, 0, Error) &&
            FOGInventoryEquipmentService(Store).CreateItem(Ruler, D.DefinitionId,
                FOGContentId(), FOGContentId(), 1, Item, Error) &&
            FOGInventoryEquipmentService(Store).CreateItem(Ruler, D.DefinitionId,
                FOGContentId(), FOGContentId(), 1, OtherItem, Error);
    }
    bool Preview(const FOGEntityId& Candidate, bool bUnequip, FOGOwnedEquipmentPreview& Out)
    {
        return FOGOwnedEquipmentService(Store).Preview(Ruler, Copy.IdentityId,
            Copy.ManifestationId, Slot, Candidate, bUnequip, Catalog, Out, Error);
    }
    bool ReadBinding(bool& bFound, FOGEquipmentBindingRecord& Out)
    { return Store.TryReadEquipmentBinding(Copy.ManifestationId, Slot, bFound, Out, Error); }
    ~FOwnedEquipmentFixture()
    { Store.Close(); IFileManager::Get().DeleteDirectory(*Directory, false, true); }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedEquipmentAdmissionTest,
    "OfflineGame.Inventory.ExactRulesOwnershipAndStaleReview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedEquipmentAdmissionTest::RunTest(const FString&)
{
    FOwnedEquipmentFixture F;
    if (!TestTrue(TEXT("Open isolated canonical inventory"), F.Open())) return false;
    FOGOwnedEquipmentService Service(F.Store);
    FOGOwnedEquipmentPreview P;
    TestTrue(TEXT("Empty catalog still produces truthful unavailable review"), Service.Preview(F.Ruler,
        F.Copy.IdentityId, F.Copy.ManifestationId, F.Slot, F.Item, false, FOGEquipmentCatalog(), P, F.Error));
    TestFalse(TEXT("Empty catalog cannot authorize equipment"), P.bCanApply);
    TestFalse(TEXT("Another Ruler cannot review the character"), Service.Preview(FOGEntityId::NewId(),
        F.Copy.IdentityId, F.Copy.ManifestationId, F.Slot, F.Item, false, F.Catalog, P, F.Error));
    TestTrue(TEXT("Exact catalog admits the owned instance"), F.Preview(F.Item, false, P));
    TestTrue(TEXT("Exact admission is actionable"), P.bCanApply);
    FOGItemInstanceRecord Changed;
    bool bFound = false;
    TestTrue(TEXT("Read candidate for canonical change"), F.Store.TryReadItemInstance(F.Item, bFound, Changed, F.Error));
    Changed.DurabilityStateJson = TEXT("{\"condition_bps\":9000}");
    TestTrue(TEXT("Persist condition change after review"), F.Store.UpsertItemInstance(Changed, 2, F.Error));
    bool bAlready = false;
    TestFalse(TEXT("Condition change invalidates the quote inside transaction"),
        Service.Apply(P, F.Catalog, FOGEntityId::NewId(), 3, bAlready, F.Error));
    FOGEquipmentBindingRecord Binding;
    TestTrue(TEXT("Read slot after rejection"), F.ReadBinding(bFound, Binding));
    TestFalse(TEXT("Rejected command did not create equipment"), bFound);
    TestTrue(TEXT("Refresh valid quote"), F.Preview(F.Item, false, P));
    FOGEquipmentCatalog Different = F.Catalog;
    Different.Rules[0].StatBonuses[TEXT("attack")] = FOGLargeNumber::FromInt64(20);
    TestFalse(TEXT("Changed authored mechanics invalidate the quote"),
        Service.Apply(P, Different, FOGEntityId::NewId(), 3, bAlready, F.Error));
    const FOGEntityId OtherRuler = FOGEntityId::NewId();
    TestTrue(TEXT("Persist unrelated real owner"), F.Store.UpsertEntity(OtherRuler,
        TEXT("ruler"), 0, TEXT("{}"), F.Error));
    Changed.OwnerEntityId = OtherRuler;
    TestTrue(TEXT("Ownership changes after review"), F.Store.UpsertItemInstance(Changed, 3, F.Error));
    TestFalse(TEXT("Cannot confirm a transferred-away item"),
        Service.Apply(P, F.Catalog, FOGEntityId::NewId(), 4, bAlready, F.Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedEquipmentTransactionTest,
    "OfflineGame.Inventory.ReplacementReceiptUnequipAndColdReload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedEquipmentTransactionTest::RunTest(const FString&)
{
    FOwnedEquipmentFixture F;
    if (!TestTrue(TEXT("Open isolated canonical inventory"), F.Open())) return false;
    FOGOwnedEquipmentService Service(F.Store);
    FOGOwnedEquipmentPreview First, StaleReplacement;
    TestTrue(TEXT("Review first item"), F.Preview(F.Item, false, First));
    TestTrue(TEXT("Review second item while slot is empty"), F.Preview(F.OtherItem, false, StaleReplacement));
    const FOGEntityId Request = FOGEntityId::NewId();
    bool bAlready = false;
    TestTrue(TEXT("Apply exact reviewed equip"), Service.Apply(First, F.Catalog, Request, 2, bAlready, F.Error));
    TestFalse(TEXT("New transaction is not a receipt replay"), bAlready);
    TestTrue(TEXT("Same request acknowledges the committed transaction"),
        Service.Apply(First, F.Catalog, Request, 2, bAlready, F.Error));
    TestTrue(TEXT("Replay is identified without another binding change"), bAlready);
    TestFalse(TEXT("Changed current slot rejects stale replacement"),
        Service.Apply(StaleReplacement, F.Catalog, FOGEntityId::NewId(), 3, bAlready, F.Error));
    FOGOwnedEquipmentPreview Replacement;
    TestTrue(TEXT("Review actual occupied slot"), F.Preview(F.OtherItem, false, Replacement));
    TestTrue(TEXT("Replacement exposes displaced owned instance"), Replacement.bHasPreviousItem &&
        Replacement.PreviousItem.ItemId == F.Item);
    TestTrue(TEXT("Confirm reviewed replacement"), Service.Apply(Replacement, F.Catalog,
        FOGEntityId::NewId(), 3, bAlready, F.Error));
    FOGOwnedEquipmentPreview Removal;
    TestTrue(TEXT("Review exact currently equipped instance"), F.Preview(F.OtherItem, true, Removal));
    TestTrue(TEXT("Confirm canonical unequip"), Service.Apply(Removal, F.Catalog,
        FOGEntityId::NewId(), 4, bAlready, F.Error));
    F.Store.Close();
    TestTrue(TEXT("Cold reopen same database"), F.Store.Open(F.Database, F.Error));
    bool bFound = false;
    FOGEquipmentBindingRecord Binding;
    TestTrue(TEXT("Read equipment after reopen"), F.ReadBinding(bFound, Binding));
    TestFalse(TEXT("Unequip persisted"), bFound);
    FOGOwnedInventorySnapshot Inventory;
    TestTrue(TEXT("Read actual inventory after reopen"), Service.ReadInventory(F.Ruler,
        F.Copy.IdentityId, F.Copy.ManifestationId, F.Catalog, Inventory, F.Error));
    TestEqual(TEXT("Replacement and unequip never delete or duplicate owned items"), Inventory.Items.Num(), 2);
    TestTrue(TEXT("Original item history is preserved"), Inventory.Items.ContainsByPredicate(
        [&](const auto& I) { return I.Item.ItemId == F.Item && I.Item.HistoryStateJson.Contains(TEXT("created")); }));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedEquipmentProjectionTest,
    "OfflineGame.Inventory.RuntimeProjectionIsIdempotentAndPreservesHp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedEquipmentProjectionTest::RunTest(const FString&)
{
    FOwnedEquipmentFixture F;
    if (!TestTrue(TEXT("Open isolated canonical inventory"), F.Open())) return false;
    FOGOwnedEquipmentService Equipment(F.Store);
    FOGOwnedEquipmentPreview Review;
    TestTrue(TEXT("Review real owned instance"), F.Preview(F.Item, false, Review));
    bool bAlready = false;
    TestTrue(TEXT("Persist authored equipment"), Equipment.Apply(Review, F.Catalog,
        FOGEntityId::NewId(), 2, bAlready, F.Error));
    FOGManifestationReinforcementRecord Reinforcement;
    Reinforcement.ManifestationId = F.Copy.ManifestationId;
    Reinforcement.StateJson = TEXT("{\"development_stat_grants\":{\"attack\":{\"significand\":\"500000000\",\"exponent10\":-8}}}");
    TestTrue(TEXT("Persist earned contribution in the existing reinforcement record"),
        F.Store.UpsertManifestationReinforcement(Reinforcement, F.Error));
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = F.Copy.ManifestationId; Unit.IdentityId = F.Copy.IdentityId;
    Unit.SkillSet.CurrentForm = F.Copy.ActiveVersionId;
    Unit.Stats.MaxHp = FOGLargeNumber::FromInt64(100);
    Unit.Stats.Attack = FOGLargeNumber::FromInt64(20);
    Unit.Stats.Speed = 10;
    Unit.CurrentHp = FOGLargeNumber::FromInt64(37);
    FOGCharacterDevelopmentService Development(F.Store);
    for (int32 I = 0; I < 3; ++I)
        TestTrue(TEXT("Repeated equipment then development projection"),
            Equipment.ProjectCombatUnit(Unit, F.Catalog, F.Error) && Development.ProjectCombatUnit(Unit, F.Error));
    TestTrue(TEXT("Equipment plus earned stats apply exactly once"), Unit.Stats.Attack == FOGLargeNumber::FromInt64(35));
    TestTrue(TEXT("Authored HP contribution is reflected"), Unit.Stats.MaxHp == FOGLargeNumber::FromInt64(150));
    TestEqual(TEXT("Integral authored speed contribution"), Unit.Stats.Speed, 13);
    TestTrue(TEXT("Projection never heals or clips HP"), Unit.CurrentHp == FOGLargeNumber::FromInt64(37));
    TestTrue(TEXT("Review removal"), F.Preview(F.Item, true, Review));
    TestTrue(TEXT("Persist removal"), Equipment.Apply(Review, F.Catalog,
        FOGEntityId::NewId(), 3, bAlready, F.Error));
    TestTrue(TEXT("Refresh after removal"), Equipment.ProjectCombatUnit(Unit, F.Catalog, F.Error) &&
        Development.ProjectCombatUnit(Unit, F.Error));
    TestTrue(TEXT("Removed gear contribution disappears, earned grant remains"),
        Unit.Stats.Attack == FOGLargeNumber::FromInt64(25));
    TestTrue(TEXT("Removal still preserves current HP"), Unit.CurrentHp == FOGLargeNumber::FromInt64(37));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedEquipmentCatalogTest,
    "OfflineGame.Inventory.CatalogRejectsAmbiguousAndUnsupportedRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedEquipmentCatalogTest::RunTest(const FString&)
{
    FString Error;
    FOGEquipmentCatalog C;
    TestTrue(TEXT("An explicit empty installed catalog is valid"), FOGEquipmentCatalog::Parse(
        TEXT("{\"schema\":\"noworld.equipment.v1\",\"slots\":[],\"items\":[],\"rules\":[]}"), C, Error));
    TestFalse(TEXT("Unknown compatibility fields cannot silently loosen rules"), FOGEquipmentCatalog::Parse(
        TEXT("{\"schema\":\"noworld.equipment.v1\",\"slots\":[],\"items\":[],\"rules\":[],\"allow_all\":true}"), C, Error));
    TestFalse(TEXT("Duplicate JSON keys are rejected"), FOGEquipmentCatalog::Parse(
        TEXT("{\"schema\":\"noworld.equipment.v1\",\"slots\":[],\"items\":[],\"rules\":[],\"rules\":[]}"), C, Error));
    FOwnedEquipmentFixture F;
    if (!TestTrue(TEXT("Open strict catalog fixture"), F.Open())) return false;
    C = F.Catalog;
    const FOGAuthoredEquipmentRule Duplicate = C.Rules[0];
    C.Rules.Add(Duplicate);
    TestFalse(TEXT("Duplicate exact match cannot introduce priority ambiguity"), C.Validate(Error));
    C = F.Catalog;
    C.Rules[0].StatBonuses.Add(TEXT("invented_stat"), FOGLargeNumber::FromInt64(1));
    TestFalse(TEXT("Unsupported mechanics require an explicit native resolver"), C.Validate(Error));
    C = F.Catalog;
    C.Rules[0].VersionId = FOGContentId(TEXT("test:equipment.other_version"));
    TestFalse(TEXT("Rule cannot reference an unauthored version slot"), C.Validate(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedEquipmentTotalsPreviewTest,
    "OfflineGame.Inventory.AuthoritativeBeforeAfterReplacementAndRemoval",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedEquipmentTotalsPreviewTest::RunTest(const FString&)
{
    FOwnedEquipmentFixture F;
    if (!TestTrue(TEXT("Open isolated equipment preview store"), F.Open())) return false;
    FOGOwnedEquipmentService Equipment(F.Store);
    FOGCharacterDevelopmentService Development(F.Store);
    FOGManifestationReinforcementRecord Earned;
    Earned.ManifestationId = F.Copy.ManifestationId;
    Earned.StateJson = TEXT("{\"development_stat_grants\":{\"attack\":{\"significand\":\"500000000\",\"exponent10\":-8}}}");
    if (!TestTrue(TEXT("Persist existing earned grants"), F.Store.UpsertManifestationReinforcement(Earned, F.Error))) return false;
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = F.Copy.ManifestationId;
    Unit.IdentityId = F.Copy.IdentityId;
    Unit.SkillSet.CurrentForm = F.Copy.ActiveVersionId;
    Unit.Stats.MaxHp = FOGLargeNumber::FromInt64(100);
    Unit.Stats.Attack = FOGLargeNumber::FromInt64(20);
    Unit.Stats.Speed = 10;
    Unit.CurrentHp = FOGLargeNumber::FromInt64(37);
    FOGOwnedEquipmentPreview Equip;
    if (!TestTrue(TEXT("Review exact owned equip"), F.Preview(F.Item, false, Equip)) ||
        !TestTrue(TEXT("Compute before and after without writes"), Equipment.PreviewStats(Equip, Unit, F.Catalog, Equip, F.Error))) return false;
    TestTrue(TEXT("Before includes earned attack"), Equip.BeforeStats.Attack == FOGLargeNumber::FromInt64(25));
    TestTrue(TEXT("After includes gear and earned attack"), Equip.AfterStats.Attack == FOGLargeNumber::FromInt64(35));
    TestTrue(TEXT("After includes authored maximum HP"), Equip.AfterStats.MaxHp == FOGLargeNumber::FromInt64(150));
    TestEqual(TEXT("After includes authored integral speed"), Equip.AfterStats.Speed, 13);
    bool Found = false, Already = false;
    FOGEquipmentBindingRecord Binding;
    TestTrue(TEXT("Read slot after preview"), F.ReadBinding(Found, Binding));
    TestFalse(TEXT("Preview did not equip or reserve the item"), Found);
    TestTrue(TEXT("Preview did not mutate source stats or current HP"),
        Unit.Stats.Attack == FOGLargeNumber::FromInt64(20) && Unit.CurrentHp == FOGLargeNumber::FromInt64(37));
    const auto Request = FOGEntityId::NewId();
    if (!TestTrue(TEXT("Commit reviewed equip"), Equipment.Apply(Equip, F.Catalog, Request, 2, Already, F.Error)) ||
        !TestTrue(TEXT("Refresh real runtime stats"), Equipment.ProjectCombatUnit(Unit, F.Catalog, F.Error) &&
            Development.ProjectCombatUnit(Unit, F.Error))) return false;
    TestTrue(TEXT("Actual post-commit attack matches preview"), Unit.Stats.Attack == Equip.AfterStats.Attack);
    TestTrue(TEXT("Actual post-commit HP maximum matches preview"), Unit.Stats.MaxHp == Equip.AfterStats.MaxHp);
    TestTrue(TEXT("Receipt replay succeeds after slot changed"), Equipment.Apply(Equip, F.Catalog, Request, 3, Already, F.Error));
    TestTrue(TEXT("Duplicate command returns its receipt"), Already);
    // A genuinely different test rule makes replacement a decrease as well as an increase.
    FOGItemInstanceRecord Alternative;
    if (!TestTrue(TEXT("Read alternative instance"), F.Store.TryReadItemInstance(F.OtherItem, Found, Alternative, F.Error))) return false;
    Alternative.QualityId = FOGContentId(TEXT("test:equipment.alternative_quality"));
    if (!TestTrue(TEXT("Persist alternative test quality"), F.Store.UpsertItemInstance(Alternative, 3, F.Error))) return false;
    auto Rule = F.Catalog.Rules[0];
    Rule.QualityId = Alternative.QualityId;
    Rule.StatBonuses.Reset();
    Rule.StatBonuses.Add(TEXT("attack"), FOGLargeNumber::FromInt64(2));
    Rule.StatBonuses.Add(TEXT("speed"), FOGLargeNumber::FromInt64(1));
    F.Catalog.Rules.Add(Rule);
    Unit.CurrentHp = FOGLargeNumber::FromInt64(130); // Preserve overflow HP too.
    FOGOwnedEquipmentPreview Replacement;
    if (!TestTrue(TEXT("Review occupied slot"), F.Preview(F.OtherItem, false, Replacement)) ||
        !TestTrue(TEXT("Preview replacement from already projected unit"),
            Equipment.PreviewStats(Replacement, Unit, F.Catalog, Replacement, F.Error))) return false;
    TestTrue(TEXT("Replacement starts from actual totals"), Replacement.BeforeStats.Attack == FOGLargeNumber::FromInt64(35));
    TestTrue(TEXT("Replacement removes old contribution before adding new"), Replacement.AfterStats.Attack == FOGLargeNumber::FromInt64(27));
    TestTrue(TEXT("Replacement removes old maximum HP bonus"), Replacement.AfterStats.MaxHp == FOGLargeNumber::FromInt64(100));
    TestEqual(TEXT("Replacement does not stack speed"), Replacement.AfterStats.Speed, 11);
    if (!TestTrue(TEXT("Commit replacement"), Equipment.Apply(Replacement, F.Catalog, FOGEntityId::NewId(), 4, Already, F.Error)) ||
        !TestTrue(TEXT("Project actual replacement"), Equipment.ProjectCombatUnit(Unit, F.Catalog, F.Error) &&
            Development.ProjectCombatUnit(Unit, F.Error))) return false;
    TestTrue(TEXT("Committed replacement matches preview"), Unit.Stats.Attack == Replacement.AfterStats.Attack &&
        Unit.Stats.MaxHp == Replacement.AfterStats.MaxHp && Unit.Stats.Speed == Replacement.AfterStats.Speed);
    TestTrue(TEXT("Neither preview nor commit clips or heals overflow HP"), Unit.CurrentHp == FOGLargeNumber::FromInt64(130));
    FOGOwnedEquipmentPreview Removal;
    if (!TestTrue(TEXT("Review removal"), F.Preview(F.OtherItem, true, Removal)) ||
        !TestTrue(TEXT("Preview removal"), Equipment.PreviewStats(Removal, Unit, F.Catalog, Removal, F.Error))) return false;
    TestTrue(TEXT("Removal retains earned grants"), Removal.AfterStats.Attack == FOGLargeNumber::FromInt64(25));
    if (!TestTrue(TEXT("Commit removal"), Equipment.Apply(Removal, F.Catalog, FOGEntityId::NewId(), 5, Already, F.Error)) ||
        !TestTrue(TEXT("Project actual removal"), Equipment.ProjectCombatUnit(Unit, F.Catalog, F.Error) &&
            Development.ProjectCombatUnit(Unit, F.Error))) return false;
    TestTrue(TEXT("Actual removal matches preview"), Unit.Stats.Attack == Removal.AfterStats.Attack);
    TestTrue(TEXT("Removal preserves overflow HP"), Unit.CurrentHp == FOGLargeNumber::FromInt64(130));
    FOGOwnedInventorySnapshot Inventory;
    TestTrue(TEXT("Read inventory after all previews and commands"), Equipment.ReadInventory(F.Ruler, F.Copy.IdentityId,
        F.Copy.ManifestationId, F.Catalog, Inventory, F.Error));
    TestEqual(TEXT("No preview, replacement, retry or removal duplicates ownership"), Inventory.Items.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedEquipmentPreviewInvalidationTest,
    "OfflineGame.Inventory.TotalsRejectWrongBodyStaleGrantsAndOverflow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedEquipmentPreviewInvalidationTest::RunTest(const FString&)
{
    FOwnedEquipmentFixture F;
    if (!TestTrue(TEXT("Open isolated preview rejection store"), F.Open())) return false;
    FOGOwnedEquipmentService Service(F.Store);
    FOGOwnedEquipmentPreview Review;
    if (!TestTrue(TEXT("Review owned candidate"), F.Preview(F.Item, false, Review))) return false;
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = FOGEntityId::NewId();
    Unit.IdentityId = F.Copy.IdentityId;
    Unit.SkillSet.CurrentForm = F.Copy.ActiveVersionId;
    TestFalse(TEXT("Another body's resolved stats cannot supply totals"),
        Service.PreviewStats(Review, Unit, F.Catalog, Review, F.Error));
    Unit.UnitEntityId = F.Copy.ManifestationId;
    Unit.Stats.Speed = MAX_int32;
    TestFalse(TEXT("Overflow is rejected by the runtime's integral speed calculation"),
        Service.PreviewStats(Review, Unit, F.Catalog, Review, F.Error));
    TestFalse(TEXT("Failed preview never advertises resolved totals"), Review.bStatsKnown);
    FOGManifestationReinforcementRecord Earned;
    Earned.ManifestationId = F.Copy.ManifestationId;
    Earned.StateJson = TEXT("{\"development_stat_grants\":{\"attack\":{\"significand\":\"100000000\",\"exponent10\":-8}}}");
    TestTrue(TEXT("Earned stats change after review"), F.Store.UpsertManifestationReinforcement(Earned, F.Error));
    bool Already = false, Found = false;
    TestFalse(TEXT("Confirm rejects changed earned grants inside canonical transaction"),
        Service.Apply(Review, F.Catalog, FOGEntityId::NewId(), 2, Already, F.Error));
    FOGEquipmentBindingRecord Binding;
    TestTrue(TEXT("Read rejected slot"), F.ReadBinding(Found, Binding));
    TestFalse(TEXT("Rejected totals did not mutate equipment"), Found);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedEquipmentOtherSlotPreviewTest,
    "OfflineGame.Inventory.OtherSlotsInvalidateTotalsAndBrokenGearContributesNothing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedEquipmentOtherSlotPreviewTest::RunTest(const FString&)
{
    FOwnedEquipmentFixture F;
    if (!TestTrue(TEXT("Open isolated multi-slot store"), F.Open())) return false;
    auto SecondSlot = F.Catalog.Slots[0];
    SecondSlot.SlotId = FOGContentId(TEXT("test:equipment.second_slot"));
    SecondSlot.DisplayName = TEXT("Second test slot");
    F.Catalog.Slots.Add(SecondSlot);
    auto SecondRule = F.Catalog.Rules[0];
    SecondRule.SlotId = SecondSlot.SlotId;
    F.Catalog.Rules.Add(SecondRule);
    FOGOwnedEquipmentService Service(F.Store);
    bool Already = false, Found = false;
    FOGOwnedEquipmentPreview First, Second;
    if (!TestTrue(TEXT("Review first slot"), F.Preview(F.Item, false, First)) ||
        !TestTrue(TEXT("Equip first slot"), Service.Apply(First, F.Catalog, FOGEntityId::NewId(), 2, Already, F.Error)) ||
        !TestTrue(TEXT("Review second slot"), Service.Preview(F.Ruler, F.Copy.IdentityId, F.Copy.ManifestationId,
            SecondSlot.SlotId, F.OtherItem, false, F.Catalog, Second, F.Error)) ||
        !TestTrue(TEXT("Equip second slot"), Service.Apply(Second, F.Catalog, FOGEntityId::NewId(), 3, Already, F.Error))) return false;
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = F.Copy.ManifestationId;
    Unit.IdentityId = F.Copy.IdentityId;
    Unit.SkillSet.CurrentForm = F.Copy.ActiveVersionId;
    Unit.Stats.MaxHp = FOGLargeNumber::FromInt64(100);
    Unit.Stats.Attack = FOGLargeNumber::FromInt64(20);
    Unit.Stats.Speed = 10;
    Unit.CurrentHp = FOGLargeNumber::FromInt64(180);
    if (!TestTrue(TEXT("Resolve actual multi-slot projection"), Service.ProjectCombatUnit(Unit, F.Catalog, F.Error))) return false;
    FOGOwnedEquipmentPreview Removal;
    if (!TestTrue(TEXT("Review first-slot removal"), F.Preview(F.Item, true, Removal)) ||
        !TestTrue(TEXT("Preview first-slot removal"), Service.PreviewStats(Removal, Unit, F.Catalog, Removal, F.Error))) return false;
    TestTrue(TEXT("Before includes both working items"), Removal.BeforeStats.Attack == FOGLargeNumber::FromInt64(40));
    TestTrue(TEXT("After retains the other working item"), Removal.AfterStats.Attack == FOGLargeNumber::FromInt64(30));
    FOGItemInstanceRecord Broken;
    if (!TestTrue(TEXT("Read item in untouched slot"), F.Store.TryReadItemInstance(F.OtherItem, Found, Broken, F.Error))) return false;
    Broken.DurabilityStateJson = TEXT("{\"broken\":true,\"condition_bps\":0}");
    if (!TestTrue(TEXT("Persist unrelated slot condition change"), F.Store.UpsertItemInstance(Broken, 4, F.Error))) return false;
    TestFalse(TEXT("Changing another slot invalidates reviewed totals at commit"),
        Service.Apply(Removal, F.Catalog, FOGEntityId::NewId(), 5, Already, F.Error));
    if (!TestTrue(TEXT("Review current removal"), F.Preview(F.Item, true, Removal)) ||
        !TestTrue(TEXT("Recompute totals using canonical broken condition"),
            Service.PreviewStats(Removal, Unit, F.Catalog, Removal, F.Error))) return false;
    TestTrue(TEXT("Broken retained equipment supplies no attack"),
        Removal.BeforeStats.Attack == FOGLargeNumber::FromInt64(30) && Removal.AfterStats.Attack == FOGLargeNumber::FromInt64(20));
    TestTrue(TEXT("Broken retained equipment supplies no maximum HP"),
        Removal.AfterStats.MaxHp == FOGLargeNumber::FromInt64(100));
    if (!TestTrue(TEXT("Commit exact fresh removal"), Service.Apply(Removal, F.Catalog, FOGEntityId::NewId(), 5, Already, F.Error)) ||
        !TestTrue(TEXT("Refresh actual result"), Service.ProjectCombatUnit(Unit, F.Catalog, F.Error))) return false;
    TestTrue(TEXT("Multi-slot preview equals actual post-commit result"),
        Unit.Stats.Attack == Removal.AfterStats.Attack && Unit.Stats.MaxHp == Removal.AfterStats.MaxHp);
    TestTrue(TEXT("Canonical condition refresh never clips or heals overflow HP"),
        Unit.CurrentHp == FOGLargeNumber::FromInt64(180));
    return true;
}
#endif
