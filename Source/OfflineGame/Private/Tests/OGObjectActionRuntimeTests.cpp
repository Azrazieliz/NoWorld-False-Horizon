#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Objects/OGObjectActionService.h"
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
        auto* Package=CreatePackage(*(TEXT("/Game/NoWorld/Definitions/Automation/ObjectActions_")+Tag));
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
        FOGObjectActionResourceChange Cost; Cost.ResourceId=ObjectTestId(TEXT("currency")); Cost.Amount=-7; Craft->Resources={Cost};
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
#endif
