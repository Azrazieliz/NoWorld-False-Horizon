#include "UObject/Package.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Objects/OGObjectActionService.h"
#include "Combat/OGCombatRewardDefinition.h"
#include "Combat/OGCombatActionJournal.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "World/OGInventoryEquipmentService.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace
{
FOGContentId ObjectTestId(const TCHAR* S) { return FOGContentId(FString(TEXT("test:object_actions."))+S); }
struct FObjectActionFixture
{
    FString Directory=FPaths::ProjectSavedDir()/TEXT("Automation")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString Database=Directory/TEXT("object-actions.db"),Error;
    FOGSQLiteWorldStore Store;
    FOGEntityId Ruler=FOGEntityId::NewId(),Merchant=FOGEntityId::NewId(),Ingredient,Stock;
    UOGDefinitionLibrary* Library=nullptr;
    UOGDefinitionBundle* Bundle=nullptr;
    UOGObjectDefinition* Material=nullptr;
    UOGObjectDefinition* Product=nullptr;
    UOGObjectActionDefinition* Craft=nullptr;
    UOGObjectActionDefinition* Trade=nullptr;
    bool Open()
    {
        IFileManager::Get().MakeDirectory(*Directory,true); Library=UOGDefinitionLibrary::Get(); if (!Library) return false;
        const FString Tag=FGuid::NewGuid().ToString(EGuidFormats::Digits);
        UPackage* Package=CreatePackage(*(TEXT("/Game/NoWorld/Definitions/Automation/ObjectActions_")+Tag));
        Material=NewObject<UOGObjectDefinition>(Package,TEXT("Material"),RF_Public|RF_Standalone);
        Product=NewObject<UOGObjectDefinition>(Package,TEXT("Product"),RF_Public|RF_Standalone);
        Material->DefinitionId=ObjectTestId(TEXT("material")); Product->DefinitionId=ObjectTestId(TEXT("product"));
        Material->DisplayName=FText::FromString(TEXT("Fixture material")); Product->DisplayName=FText::FromString(TEXT("Fixture product"));
        Material->OriginalSourceReference=Product->OriginalSourceReference=TEXT("isolated native transaction fixture");
        Craft=NewObject<UOGObjectActionDefinition>(Package,TEXT("Craft"),RF_Public|RF_Standalone);
        Craft->DefinitionId=ObjectTestId(TEXT("craft")); Craft->Kind=EOGObjectActionKind::Craft;
        Craft->DisplayName=FText::FromString(TEXT("Fixture crafting")); Craft->OriginalSourceReference=TEXT("native crafting fixture");
        FOGObjectActionInput Input; Input.ObjectId=Material->DefinitionId; Craft->Inputs={Input};
        FOGObjectActionOutput Output; Output.ObjectId=Product->DefinitionId; Craft->Outputs={Output};
        FOGObjectActionResourceChange Cost; Cost.ResourceId=ObjectTestId(TEXT("currency")); Cost.DisplayName=FText::FromString(TEXT("Fixture currency")); Cost.Amount=-7; Craft->Resources={Cost};
        Trade=NewObject<UOGObjectActionDefinition>(Package,TEXT("Trade"),RF_Public|RF_Standalone);
        Trade->DefinitionId=ObjectTestId(TEXT("trade")); Trade->Kind=EOGObjectActionKind::Exchange;
        Trade->DisplayName=FText::FromString(TEXT("Fixture stocked exchange")); Trade->OriginalSourceReference=TEXT("native exchange fixture");
        Trade->CounterpartyEntityKind=TEXT("npc");
        FOGObjectActionInput Give; Give.ObjectId=Material->DefinitionId; Give.Disposition=EOGObjectInputDisposition::Transfer;
        FOGObjectActionInput Receive; Receive.ObjectId=Product->DefinitionId; Receive.From=EOGObjectActionParty::Counterparty;
        Receive.Disposition=EOGObjectInputDisposition::Transfer; Trade->Inputs={Give,Receive};
        Cost.Amount=-3; FOGObjectActionResourceChange Credit=Cost; Credit.Amount=3; Credit.Account=EOGObjectActionParty::Counterparty;
        Trade->Resources={Cost,Credit};
        Bundle=NewObject<UOGDefinitionBundle>(Package,TEXT("Bundle"),RF_Public|RF_Standalone);
        Bundle->BundleId=FOGContentId(TEXT("test:object_actions.bundle_")+Tag);
        for (auto* D:TArray<UOGReusableDefinition*>{Material,Product,Craft,Trade})
        { FOGDefinitionAssetEntry E; E.System=D->System; E.Id=D->DefinitionId; E.Asset=D; Bundle->Assets.Add(E); }
        return Library->InstallBundle(Bundle,Error) && Store.Open(Database,Error) &&
            Store.UpsertEntity(Ruler,TEXT("ruler"),0,TEXT("{}"),Error) &&
            Store.UpsertEntity(Merchant,TEXT("npc"),0,TEXT("{}"),Error) &&
            Store.SetResourceBalance(Ruler,ObjectTestId(TEXT("currency")),20,Error) &&
            Store.SetResourceBalance(Merchant,ObjectTestId(TEXT("currency")),10,Error) &&
            FOGInventoryEquipmentService(Store).CreateItem(Ruler,Material->DefinitionId,FOGContentId(),FOGContentId(),0,Ingredient,Error) &&
            FOGInventoryEquipmentService(Store).CreateItem(Merchant,Product->DefinitionId,FOGContentId(),FOGContentId(),0,Stock,Error);
    }
    int64 Balance(const FOGEntityId& Owner)
    { bool Found=false; int64 Value=0; Store.TryReadResourceBalance(Owner,ObjectTestId(TEXT("currency")),Found,Value,Error); return Value; }
    ~FObjectActionFixture()
    {
        if (Library && Bundle) { FString Ignored; Library->RemoveBundle(Bundle->BundleId,Ignored); }
        Store.Close(); IFileManager::Get().DeleteDirectory(*Directory,false,true);
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGObjectActionCraftTest,
    "OfflineGame.Objects.Crafting.DurableInstancesStaleQuotesAndRetry",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGObjectActionCraftTest::RunTest(const FString&)
{
    FObjectActionFixture F; if (!TestTrue(TEXT("Open isolated object system"),F.Open())) return false;
    FOGObjectActionService Service(F.Store); FOGObjectActionContext Context; Context.RulerId=F.Ruler;
    FOGObjectActionPreview Review;
    if (!TestTrue(TEXT("Review actual ingredients and currency"),Service.Preview(*F.Craft,Context,1,Review,F.Error))) return false;
    TestTrue(TEXT("Recipe is executable"),Review.bCanApply);
    if (!F.Store.SetResourceBalance(F.Ruler,ObjectTestId(TEXT("currency")),19,F.Error)) return false;
    bool Already=false; TArray<FOGEntityId> Created; const auto Request=FOGEntityId::NewId();
    TestFalse(TEXT("Changed wallet rejects stale review"),Service.Apply(*F.Craft,Review,Request,2,Already,Created,F.Error));
    TestEqual(TEXT("Rejected action does not spend currency"),F.Balance(F.Ruler),static_cast<int64>(19));
    if (!Service.Preview(*F.Craft,Context,2,Review,F.Error) ||
        !TestTrue(TEXT("Apply reviewed recipe"),Service.Apply(*F.Craft,Review,Request,2,Already,Created,F.Error))) return false;
    if (!TestEqual(TEXT("Creates one real output instance"),Created.Num(),1)) return false;
    const auto CreatedId=Created[0];
    TestEqual(TEXT("Spends exact authored cost"),F.Balance(F.Ruler),static_cast<int64>(12));
    TestTrue(TEXT("Retry acknowledges same durable result"),Service.Apply(*F.Craft,Review,Request,2,Already,Created,F.Error) && Already);
    TestTrue(TEXT("Retry preserves original created ID"),Created.Num()==1 && Created[0]==CreatedId);
    TestEqual(TEXT("Retry does not pay twice"),F.Balance(F.Ruler),static_cast<int64>(12));
    F.Store.Close(); if (!F.Store.Open(F.Database,F.Error)) return false;
    bool Found=false; FOGItemInstanceRecord Input,Output;
    if (!F.Store.TryReadItemInstance(F.Ingredient,Found,Input,F.Error) || !Found ||
        !F.Store.TryReadItemInstance(CreatedId,Found,Output,F.Error) || !Found) return false;
    TestFalse(TEXT("Consumed instance no longer belongs to inventory"),Input.OwnerEntityId.IsValid());
    TestTrue(TEXT("Consumed provenance is retained"),Input.HistoryStateJson.Contains(TEXT("consumed_by_object_action_event")));
    TestTrue(TEXT("Cold reopen retains genuine output ownership"),Output.OwnerEntityId==F.Ruler && Output.DefinitionId==F.Product->DefinitionId);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGObjectActionExchangeTest,
    "OfflineGame.Objects.Exchange.RealStockCurrencyConservationAndOfferRetry",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGObjectActionExchangeTest::RunTest(const FString&)
{
    FObjectActionFixture F; if (!F.Open()) return false;
    FOGObjectActionService Service(F.Store); FOGObjectActionContext Context; Context.RulerId=F.Ruler; Context.CounterpartyId=F.Merchant;
    FOGObjectActionPreview Review;
    if (!Service.Preview(*F.Trade,Context,1,Review,F.Error)) return false;
    TestFalse(TEXT("Merely knowing a counterparty never creates an offer"),Review.bCanApply);
    const auto OfferRequest=FOGEntityId::NewId();
    if (!Service.PublishOffer(*F.Trade,F.Merchant,F.Ruler,1,0,-1,OfferRequest,1,F.Error) ||
        !Service.Preview(*F.Trade,Context,1,Review,F.Error)) return false;
    bool Already=false; TArray<FOGEntityId> Created;
    if (!TestTrue(TEXT("Execute real stocked exchange"),Service.Apply(*F.Trade,Review,FOGEntityId::NewId(),1,Already,Created,F.Error))) return false;
    TestTrue(TEXT("Trade creates no replacement stock"),Created.IsEmpty());
    TestEqual(TEXT("Currency remains conserved"),F.Balance(F.Ruler)+F.Balance(F.Merchant),static_cast<int64>(30));
    bool Found=false; FOGItemInstanceRecord Given,Received;
    if (!F.Store.TryReadItemInstance(F.Ingredient,Found,Given,F.Error) || !Found ||
        !F.Store.TryReadItemInstance(F.Stock,Found,Received,F.Error) || !Found) return false;
    TestTrue(TEXT("Transfers actual original item identities"),Given.OwnerEntityId==F.Merchant && Received.OwnerEntityId==F.Ruler);
    if (!Service.PublishOffer(*F.Trade,F.Merchant,F.Ruler,1,0,-1,OfferRequest,1,F.Error) ||
        !Service.Preview(*F.Trade,Context,1,Review,F.Error)) return false;
    TestFalse(TEXT("Publication retry does not reopen exhausted offer"),Review.bCanApply);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGObjectActionExchangeDefinitionTest,
    "OfflineGame.Objects.Exchange.RejectsCurrencyAndStockFabrication",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGObjectActionExchangeDefinitionTest::RunTest(const FString&)
{
    FObjectActionFixture F; if (!F.Open()) return false;
    TestTrue(TEXT("Balanced exchange validates"),F.Trade->ValidateDefinition(F.Error));
    F.Trade->Resources[1].Amount=4;
    TestFalse(TEXT("An exchange cannot mint currency"),F.Trade->ValidateDefinition(F.Error));
    F.Trade->Resources[1].Amount=3;
    FOGObjectActionOutput Output; Output.ObjectId=F.Product->DefinitionId; F.Trade->Outputs={Output};
    TestFalse(TEXT("An exchange must transfer actual stock"),F.Trade->ValidateDefinition(F.Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGObjectActionRepairTest,
    "OfflineGame.Objects.Repair.PreservesInstanceAndCustomState",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGObjectActionRepairTest::RunTest(const FString&)
{
    FObjectActionFixture F; if (!F.Open()) return false;
    bool Found=false; FOGItemInstanceRecord Item;
    if (!F.Store.TryReadItemInstance(F.Ingredient,Found,Item,F.Error) || !Found) return false;
    Item.DurabilityStateJson=TEXT("{\"condition_bps\":2000,\"broken\":true,\"custom_finish\":\"retained\"}");
    if (!F.Store.UpsertItemInstance(Item,1,F.Error)) return false;
    auto* Repair=NewObject<UOGObjectActionDefinition>();
    Repair->DefinitionId=ObjectTestId(TEXT("repair")); Repair->DisplayName=FText::FromString(TEXT("Fixture repair"));
    Repair->OriginalSourceReference=TEXT("native repair fixture"); Repair->Kind=EOGObjectActionKind::Repair;
    Repair->SubjectObjectId=F.Material->DefinitionId; Repair->RepairToConditionBps=9000; Repair->Resources=F.Craft->Resources;
    FOGObjectActionContext Context; Context.RulerId=F.Ruler; Context.SelectedItemId=F.Ingredient;
    FOGObjectActionService Service(F.Store); FOGObjectActionPreview Review;
    if (!Service.Preview(*Repair,Context,1,Review,F.Error)) return false;
    TestTrue(TEXT("Repair preview shows actual before/after condition"),Review.bCanApply && Review.bSubjectWasBroken &&
        Review.RepairBeforeConditionBps==2000 && Review.RepairAfterConditionBps==9000);
    bool Already=false; TArray<FOGEntityId> Created;
    if (!Service.Apply(*Repair,Review,FOGEntityId::NewId(),1,Already,Created,F.Error)) return false;
    if (!F.Store.TryReadItemInstance(F.Ingredient,Found,Item,F.Error) || !Found) return false;
    TestTrue(TEXT("Repair preserves real identity and ownership"),Created.IsEmpty() && Item.ItemId==F.Ingredient && Item.OwnerEntityId==F.Ruler);
    TestTrue(TEXT("Repair retains unrelated condition data"),Item.DurabilityStateJson.Contains(TEXT("custom_finish")) &&
        Item.DurabilityStateJson.Contains(TEXT("retained")) && Item.DurabilityStateJson.Contains(TEXT("9000")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGObjectActionLateRollbackTest,
    "OfflineGame.Objects.Crafting.OutputIdentityCollisionRollsBackEarlierWrites",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGObjectActionLateRollbackTest::RunTest(const FString&)
{
    FObjectActionFixture F; if (!F.Open()) return false;
    FOGObjectActionService Service(F.Store); FOGObjectActionContext Context; Context.RulerId=F.Ruler;
    FOGObjectActionPreview Review; if (!Service.Preview(*F.Craft,Context,1,Review,F.Error)) return false;
    const auto Request=FOGEntityId::NewId();
    const FString Key=Request.ToString()+TEXT("|output|0"); FTCHARToUTF8 Bytes(*Key);
    FMD5 Hash; Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    uint8 Digest[16]; Hash.Final(Digest); FGuid Guid; FGuid::ParseExact(BytesToHex(Digest,16),EGuidFormats::Digits,Guid);
    if (!F.Store.UpsertEntity(FOGEntityId(Guid),TEXT("protected_fixture_entity"),0,TEXT("{}"),F.Error)) return false;
    bool Already=false; TArray<FOGEntityId> Created;
    TestFalse(TEXT("Existing output identity rejects the whole action"),Service.Apply(*F.Craft,Review,Request,1,Already,Created,F.Error));
    TestEqual(TEXT("Earlier wallet debit was rolled back"),F.Balance(F.Ruler),static_cast<int64>(20));
    bool Found=false; FOGItemInstanceRecord Item;
    if (!F.Store.TryReadItemInstance(F.Ingredient,Found,Item,F.Error) || !Found) return false;
    TestTrue(TEXT("Earlier ingredient consumption was rolled back"),Item.OwnerEntityId==F.Ruler);
    FOGWorldEvent Receipt; if (!F.Store.TryReadWorldEvent(Request,Found,Receipt,F.Error)) return false;
    TestFalse(TEXT("Failed action left no success receipt"),Found);
    TestTrue(TEXT("Failed action returns no fabricated outputs"),Created.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeVictoryRewardsTest,
    "OfflineGame.Combat.Rewards.DurableVictoryPaysActualEnemyLifeOnce",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeVictoryRewardsTest::RunTest(const FString&)
{
    FObjectActionFixture F; if (!F.Open()) return false;
    auto* Reward=NewObject<UOGCombatRewardDefinition>(F.Material->GetOutermost(),TEXT("VictoryReward"),RF_Public|RF_Standalone);
    Reward->DefinitionId=ObjectTestId(TEXT("victory_reward")); Reward->DisplayName=FText::FromString(TEXT("Fixture victory reward"));
    Reward->OriginalSourceReference=TEXT("native victory transaction fixture"); Reward->EnemyEntityKind=TEXT("npc");
    Reward->Objects=F.Craft->Outputs; auto Currency=F.Craft->Resources[0]; Currency.Amount=5; Reward->Resources={Currency};
    FOGDefinitionAssetEntry Entry; Entry.System=Reward->System; Entry.Id=Reward->DefinitionId; Entry.Asset=Reward;
    F.Bundle->Assets.Add(Entry); ++F.Bundle->Version;
    if (!F.Library->InstallBundle(F.Bundle,F.Error)) return false;
    const FString EnemyJson=TEXT("{\"combat_reward_definition_id\":\"")+Reward->DefinitionId.ToString()+TEXT("\"}");
    if (!F.Store.UpsertEntity(F.Merchant,TEXT("npc"),1,EnemyJson,F.Error)) return false;
    FOGCombatUnitState Player,Enemy; Player.UnitEntityId=F.Ruler; Enemy.UnitEntityId=F.Merchant;
    Player.TeamIndex=0; Enemy.TeamIndex=1; Player.Presence=EOGCombatPresence::Active; Enemy.Presence=EOGCombatPresence::Defeated;
    Player.CurrentHp=FOGLargeNumber::FromInt64(100);
    FOGTurnBattleState After; After.BattleId=FOGEntityId::NewId(); After.Status=EOGTurnBattleStatus::Completed; After.Units={Player,Enemy};
    auto Before=After; Before.Status=EOGTurnBattleStatus::Running; Before.Units[1].CurrentHp=FOGLargeNumber::FromInt64(10);
    Before.Units[1].Presence=EOGCombatPresence::Active;
    auto Commit=[&](FOGNativeBattleOutcome& Outcome)
    {
        FOGResolvedCombatAction Action; Action.ActionId=FOGEntityId::NewId(); Action.SourceUnitId=F.Ruler;
        Action.SkillId=ObjectTestId(TEXT("finishing_action")); Action.TargetUnitIds={F.Merchant}; Action.ActionDelay=100;
        FOGWorldEvent Completion;
        if (!FOGCombatActionJournal::BuildResolvedActionEvent(Before,After,Action,F.Ruler,2,Completion,F.Error) ||
            !F.Store.BeginTransaction(F.Error)) return false;
        if (!F.Store.AppendWorldEvent(Completion,F.Error) ||
            !FOGCombatRewardService(F.Store).ApplyCommittedVictory(After,F.Ruler,Completion,Outcome,F.Error) ||
            !F.Store.CommitTransaction(F.Error))
        { FString Ignored; F.Store.RollbackTransaction(Ignored); return false; }
        return true;
    };
    FOGNativeBattleOutcome Outcome;
    if (!TestTrue(TEXT("Commit finishing action and source rewards together"),Commit(Outcome))) return false;
    TestTrue(TEXT("Actual victory and actual created item recorded"),Outcome.bVictory && Outcome.RewardEventIds.Num()==1 && Outcome.CreatedItemIds.Num()==1);
    TestEqual(TEXT("Real wallet receives exact reward"),F.Balance(F.Ruler),static_cast<int64>(25));
    const auto GrantedItem=Outcome.CreatedItemIds[0];
    Before.BattleId=After.BattleId=FOGEntityId::NewId();
    if (!TestTrue(TEXT("A second encounter can finish against the same canonical life"),Commit(Outcome))) return false;
    TestTrue(TEXT("The same enemy life never pays twice"),Outcome.RewardEventIds.IsEmpty() && Outcome.CreatedItemIds.IsEmpty());
    TestEqual(TEXT("No repeat currency grant"),F.Balance(F.Ruler),static_cast<int64>(25));
    F.Store.Close(); if (!F.Store.Open(F.Database,F.Error)) return false;
    bool Found=false; FOGItemInstanceRecord Item;
    if (!F.Store.TryReadItemInstance(GrantedItem,Found,Item,F.Error)) return false;
    TestTrue(TEXT("Victory output survives cold reopen"),Found && Item.OwnerEntityId==F.Ruler && Item.DefinitionId==F.Product->DefinitionId);
    return true;
}
#endif
