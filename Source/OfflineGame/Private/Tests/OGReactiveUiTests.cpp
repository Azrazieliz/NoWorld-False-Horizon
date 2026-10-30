#if WITH_DEV_AUTOMATION_TESTS
#include "UI/OGReactiveUiSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"

struct FOGReactiveUiTestAccess
{
    static void Track(UOGReactiveUiSubsystem& S,UUserWidget* Owner,UWidget* W) { S.Track(Owner,W); }
    static void Apply(UOGReactiveUiSubsystem& S,const FOGReactiveUiPalette& Palette) { S.Current=Palette; S.Apply(); }
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGReactiveUiCanonicalSelectors,
    "OfflineGame.UI.Reactive.CanonicalSelectorsAndVisualProgression",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGReactiveUiCanonicalSelectors::RunTest(const FString&)
{
    FOGReactiveUiContext Context; Context.OwnedLevel=90;
    const auto Unavailable=UOGReactiveUiSubsystem::ResolveDefault(Context);
    FOGReactiveUiContext Empty;
    TestTrue(TEXT("Unavailable canonical state cannot imply an unlock or advanced presentation"),
        Unavailable.NearlyEquals(UOGReactiveUiSubsystem::ResolveDefault(Empty)));
    Context.bCanonicalReady=true; Context.OwnedLevel=1;
    const auto Early=UOGReactiveUiSubsystem::ResolveDefault(Context);
    Context.OwnedLevel=90; Context.RulerClasses={FOGContentId(TEXT("test:ruler.source_class"))};
    Context.RulerTitle=TEXT("test_source_title");
    const auto Advanced=UOGReactiveUiSubsystem::ResolveDefault(Context);
    TestTrue(TEXT("Actual progression and Ruler capability projection change visual intensity"),Advanced.Glow>Early.Glow);
    FOGReactiveUiRule Rule; Rule.MinimumOwnedLevel=5; Rule.RequiredRulerTitle=Context.RulerTitle;
    Rule.RequiredRulerClass=Context.RulerClasses[0]; Rule.RequiredRulerScope.Add(TEXT("active_power"),TEXT("true"));
    Rule.RequiredLocationKind=TEXT("test_original_location");
    Context.LocationKind=Rule.RequiredLocationKind; Context.RulerScopeJson=TEXT("{\"active_power\":true}");
    TestTrue(TEXT("Installed selectors match exact source class/title/location/scope"),Rule.Matches(Context));
    Context.RulerScopeJson=TEXT("{\"active_power\":false}");
    TestFalse(TEXT("Revoked source power immediately stops matching"),Rule.Matches(Context));
    Context.RulerScopeJson=TEXT("{}");
    TestFalse(TEXT("Missing source evidence is never fabricated"),Rule.Matches(Context));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGReactiveUiEnvironmentAndPolicy,
    "OfflineGame.UI.Reactive.LiveEnvironmentAndDeterministicPolicy",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGReactiveUiEnvironmentAndPolicy::RunTest(const FString&)
{
    FOGReactiveUiContext Context; Context.bCanonicalReady=true; Context.bWorldMode=true;
    Context.MovementContext=TEXT("Ground");
    const auto Ground=UOGReactiveUiSubsystem::ResolveDefault(Context);
    Context.NativeMovementContext=TEXT("Underwater");
    TestFalse(TEXT("Genuine native water context changes the same layout's palette"),
        Ground.Accent.Equals(UOGReactiveUiSubsystem::ResolveDefault(Context).Accent));
    auto* Definition=NewObject<UOGReactiveUiThemeDefinition>();
    Definition->DefinitionId=FOGContentId(TEXT("test:ui.theme"));
    // Base definition provenance remains enforced separately by its existing library tests.
    FOGReactiveUiPalette Invalid; Invalid.Glow=-1;
    TestFalse(TEXT("Invalid visual intensity is rejected"),Invalid.IsFinite());
    FOGReactiveUiRule Rule; Rule.RequiredMovementContext=TEXT("Underwater");
    TestTrue(TEXT("Native context is available to installed presentation selectors"),Rule.Matches(Context));
    Context.NativeMovementContext=NAME_None;
    TestFalse(TEXT("Removing native context restores canonical condition matching"),Rule.Matches(Context));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGReactiveUiPreservesWidgetsAndSemanticColours,
    "OfflineGame.UI.Reactive.NoRebuildNoArtTintAndCriticalColourOwnership",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGReactiveUiPreservesWidgetsAndSemanticColours::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    if (!TestNotNull(TEXT("Native World"),World)) return false;
    auto* Driver=World->GetSubsystem<UOGReactiveUiSubsystem>();
    if (!TestNotNull(TEXT("World-owned presentation driver"),Driver)) { World->DestroyWorld(false); return false; }
    auto* Owner=NewObject<UUserWidget>(World);
    auto* Layout=NewObject<UVerticalBox>(); auto* Title=NewObject<UTextBlock>();
    const FLinearColor Original(.72f,.62f,.44f,1); Title->SetColorAndOpacity(FSlateColor(Original));
    auto* Slot=Layout->AddChildToVerticalBox(Title);
    auto* Art=NewObject<UImage>(); const FLinearColor ArtColour(.91f,.85f,.82f,1); Art->SetColorAndOpacity(ArtColour);
    FOGReactiveUiTestAccess::Track(*Driver,Owner,Title); FOGReactiveUiTestAccess::Track(*Driver,Owner,Art);
    auto* Imported=NewObject<UImage>(Owner,TEXT("Gate10GachaOrnamentNotControls")); Imported->SetColorAndOpacity(ArtColour);
    FOGReactiveUiTestAccess::Track(*Driver,Owner,Imported);
    auto* MRFrame=NewObject<UBorder>(); const FLinearColor MRBlack(.035f,.035f,.040f,1);
    MRFrame->SetBrushColor(MRBlack); Driver->PreserveOriginal(MRFrame);
    FOGReactiveUiTestAccess::Track(*Driver,Owner,MRFrame);
    auto* MRBadge=NewObject<UTextBlock>(); const FLinearColor MRSilver(.77f,.80f,.86f,1);
    MRBadge->SetColorAndOpacity(FSlateColor(MRSilver)); Driver->PreserveOriginal(MRBadge);
    FOGReactiveUiTestAccess::Track(*Driver,Owner,MRBadge);
    Driver->SetReducedMotion(true);
    TestTrue(TEXT("Existing reduced-motion preference suppresses chrome movement"),Driver->IsMotionReduced()&&Driver->GetEffectPhase()==0);
    FOGReactiveUiPalette Palette; Palette.Accent=FLinearColor(.3f,.7f,.8f,1);
    FOGReactiveUiTestAccess::Apply(*Driver,Palette);
    TestTrue(TEXT("Accent changes in place"),Title->GetColorAndOpacity().GetSpecifiedColor().Equals(Palette.Accent));
    TestTrue(TEXT("Layout, child identity and slot stay intact"),Layout->GetChildAt(0)==Title && Title->Slot.Get()==Slot);
    TestTrue(TEXT("Original artwork pixels receive no automatic tint"),Art->GetColorAndOpacity().Equals(ArtColour));
    TestTrue(TEXT("An imported image with ornament in its name retains source colours"),Imported->GetColorAndOpacity().Equals(ArtColour));
    TestTrue(TEXT("Explicit MR semantic frame is preserved"),MRFrame->GetBrushColor().Equals(MRBlack));
    TestTrue(TEXT("Explicit MR silver rarity badge is preserved"),MRBadge->GetColorAndOpacity().GetSpecifiedColor().Equals(MRSilver));
    const FLinearColor Warning(.88f,.57f,.43f,1); Title->SetColorAndOpacity(FSlateColor(Warning));
    Palette.Accent=FLinearColor(.8f,.6f,.4f,1); FOGReactiveUiTestAccess::Apply(*Driver,Palette);
    TestTrue(TEXT("A gameplay-owned warning colour wins over theme styling"),
        Title->GetColorAndOpacity().GetSpecifiedColor().Equals(Warning));
    Driver->UntrackTree(Owner);
    TestTrue(TEXT("Release preserves a later gameplay-owned colour"),
        Title->GetColorAndOpacity().GetSpecifiedColor().Equals(Warning));
    World->DestroyWorld(false);
    return true;
}
#endif
