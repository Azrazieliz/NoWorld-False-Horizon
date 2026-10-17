#include "UI/OGGate12NativeRulerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "UI/OGGate12CharacterVisualCatalog.h"
#include "UI/OGGate12ExploredMapWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/OGGate12CharacterPreviewStage.h"
#include "UI/OGGate12CharacterPresentationPreferences.h"
#include "Runtime/OGPlayerProfileSettings.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "World/OGStartingRegionPresentation.h"

namespace
{
const FLinearColor TextMain(0.90f, 0.90f, 0.89f, 1.0f);
const FLinearColor TextMuted(0.55f, 0.58f, 0.61f, 1.0f);
const FLinearColor Gate12RulerGold(0.72f, 0.62f, 0.44f, 1.0f);

struct FNativeRarityColors
{
    FLinearColor Frame;
    FLinearColor Badge;
    bool bKnown = false;
};

FNativeRarityColors ResolveRarity(FName Tier)
{
    // Frozen six-tier Gate10 palette; no fabricated/universal Absolute tier.
    // Matte outer lines only. Metallic treatment is reserved for insignia.
    if (Tier == TEXT("R"))   return { FLinearColor(.90f,.72f,.16f,1), FLinearColor(.94f,.81f,.32f,1), true };
    if (Tier == TEXT("SR"))  return { FLinearColor(.87f,.34f,.60f,1), FLinearColor(.92f,.47f,.70f,1), true };
    if (Tier == TEXT("SSR")) return { FLinearColor(.78f,.16f,.22f,1), FLinearColor(.92f,.25f,.31f,1), true };
    if (Tier == TEXT("UR"))  return { FLinearColor(.22f,.40f,.82f,1), FLinearColor(.38f,.59f,.92f,1), true };
    if (Tier == TEXT("LR"))  return { FLinearColor(.68f,.70f,.72f,1), FLinearColor(.81f,.67f,.34f,1), true };
    if (Tier == TEXT("MR"))  return { FLinearColor(.035f,.035f,.040f,1), FLinearColor(.77f,.80f,.86f,1), true };
    // Unknown and Absolute require authored source; neutral fallback only.
    return { FLinearColor(.13f,.14f,.16f,1), TextMuted, false };
}

FString DestinationTitle(EOGRulerPrimaryDestination Dest)
{
    switch (Dest)
    {
    case EOGRulerPrimaryDestination::Home: return TEXT("HOME");
    case EOGRulerPrimaryDestination::Characters: return TEXT("CHARACTERS");
    case EOGRulerPrimaryDestination::Gacha: return TEXT("GACHA");
    case EOGRulerPrimaryDestination::Territory: return TEXT("TERRITORY");
    case EOGRulerPrimaryDestination::Records: return TEXT("RECORDS");
    }
    return TEXT("UNKNOWN");
}
}

void UOGGate12RulerActionButton::InvokeAction()
{
    if (RulerWidget) RulerWidget->InvokeAction(ActionId);
}

void UOGGate12NativeRulerWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildRoot();
}

void UOGGate12NativeRulerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshFromCanonical();
}

void UOGGate12NativeRulerWidget::NativeDestruct()
{
    ReleasePreviewStage();
    Super::NativeDestruct();
}

void UOGGate12NativeRulerWidget::ShowWorldTransitionError(const FString& Error)
{
    LastUiError = Error;
    BuildCurrentPage();
}

void UOGGate12NativeRulerWidget::SetVisualCatalog(UOGGate12CharacterVisualCatalog* InCatalog)
{
    ReleasePreviewStage();
    VisualCatalog = InCatalog;
    RefreshFromCanonical();
}

void UOGGate12NativeRulerWidget::ReleaseRosterPreviewStages()
{
    for (AOGGate12CharacterPreviewStage* Stage : RosterPreviewStages)
        if (IsValid(Stage)) Stage->Destroy();
    RosterPreviewStages.Reset();
}

void UOGGate12NativeRulerWidget::ReleasePreviewStage()
{
    ReleaseRosterPreviewStages();
    if (IsValid(PreviewStage)) PreviewStage->Destroy();
    PreviewStage = nullptr;
}

void UOGGate12NativeRulerWidget::BuildRoot()
{
    if (!WidgetTree || WidgetTree->RootWidget) return;

    USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>(
        USafeZone::StaticClass(), TEXT("RulerSafeArea"));
    Screen = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("RulerScreen"));
    UBorder* Background = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RulerBackground"));
    if (!Safe || !Screen || !Background) return;

    WidgetTree->RootWidget = Safe;
    Safe->AddChild(Background);
    Background->SetBrushColor(FLinearColor(.003f, .003f, .005f, 1.0f));
    // Frozen Gate10's illustration is a non-interactive scene under a real
    // session-bound widget tree. No currency, art character or menu label from
    // the mock-up becomes game state or a fake hotspot.
    UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), TEXT("Gate10RulerLiveLayers"));
    Background->AddChild(Layers);
    FrozenSceneArtwork = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("Gate10RulerSceneArtOnly"));
    FrozenSceneArtwork->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* BackSlot = Layers->AddChildToOverlay(FrozenSceneArtwork);
    BackSlot->SetHorizontalAlignment(HAlign_Fill);
    BackSlot->SetVerticalAlignment(VAlign_Fill);
    UOverlaySlot* UiSlot = Layers->AddChildToOverlay(Screen);
    UiSlot->SetHorizontalAlignment(HAlign_Fill);
    UiSlot->SetVerticalAlignment(VAlign_Fill);

    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("GameTitle"));
    Heading->SetText(FText::FromString(TEXT("NoWorld: False Horizon")));
    Heading->SetColorAndOpacity(FSlateColor(Gate12RulerGold));
    FSlateFontInfo HeadingFont = Heading->GetFont();
    HeadingFont.Size = 24;
    Heading->SetFont(HeadingFont);
    UBorder* StatusBar = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RulerStatusOrnamentalStrip"));
    StatusBar->SetBrushColor(FLinearColor(.010f,.010f,.013f,.94f));
    StatusBar->SetPadding(FMargin(8.f,7.f));
    StatusBar->AddChild(Heading);
    UVerticalBoxSlot* HeadingSlot = Screen->AddChildToVerticalBox(StatusBar);
    HeadingSlot->SetPadding(FMargin(7.0f, 5.0f, 7.0f, 5.0f));

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("RulerScrollableContent"));
    UVerticalBoxSlot* ScrollSlot = Screen->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ScrollSlot->SetPadding(FMargin(16.0f, 4.0f, 16.0f, 4.0f));
    Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CanonicalPageBody"));
    Scroll->AddChild(Content);

    NavigationBar = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("RulerFiveDestinations"));
    UBorder* BottomFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("Gate10BottomNavigationOrnament"));
    BottomFrame->SetBrushColor(FLinearColor(.009f,.009f,.012f,.96f));
    BottomFrame->SetPadding(FMargin(3.f,5.f));
    BottomFrame->AddChild(NavigationBar);
    UVerticalBoxSlot* NavSlot = Screen->AddChildToVerticalBox(BottomFrame);
    NavSlot->SetPadding(FMargin(5.0f, 5.0f, 5.0f, 6.0f));
}

void UOGGate12NativeRulerWidget::RefreshFrozenScene(
    EOGRulerPrimaryDestination Destination)
{
    if (!FrozenSceneArtwork) return;
    // Gate10 corrected 2026-10-17 authority: every character, location and
    // environment in the frozen mockups is illustrative and MUST be discarded.
    // Never import cropped or retouched mockup screenshots as game art.
    FrozenSceneArtwork->SetOpacity(0.f);
    LoadedSceneTexture = nullptr;
    LoadedSceneSource.Reset();

    if (Destination != EOGRulerPrimaryDestination::Home || !VisualCatalog)
        return;

    const UGameInstance* GI = GetGameInstance();
    const UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || !Session->IsBoundToStoredRuler())
        return;

    // Use only exact source/Registry bindings for an actually OWNED unit.
    // Empty worlds intentionally retain the invariant black background.
    for (const FOGRosterIdentityViewModel& Identity : Session->OwnedRoster)
    {
        const FOGRosterManifestationViewModel* Owned =
            Identity.Manifestations.FindByPredicate(
                [&Identity](const FOGRosterManifestationViewModel& Candidate)
                {
                    return Candidate.ManifestationId ==
                        Identity.SelectedManifestationId;
                });
        if (!Owned || !Owned->ManifestationId.IsValid())
            continue;

        const FOGGate12CharacterVisualEntry* Approved =
            VisualCatalog->FindExact(Identity.IdentityId, Owned->ActiveVersionId);
        if (!Approved)
            continue;

        FOGPlayerProfileSettings Profile;
        FString ProfileError;
        if (!FOGPlayerProfileSettingsService::Load(
                FOGPlayerProfileSettingsService::DefaultProfilePath(),
                Profile, ProfileError))
        {
            // Fail closed: unknown privacy profile never exposes adult art.
            Profile.bSfwPresentation = true;
        }
        const TSoftObjectPtr<UTexture2D>& Artwork =
            Profile.bSfwPresentation ? Approved->ApprovedSfwArtwork2D
                                     : Approved->OriginalArtwork2D;
        UTexture2D* Texture = Artwork.IsNull()
            ? nullptr : Artwork.LoadSynchronous();
        if (!Texture)
            continue;

        // Art is purely visual and non-interactive; the approved 5-destination
        // native UMG navigation and canonical state remain authoritative.
        FrozenSceneArtwork->SetBrushFromTexture(Texture, true);
        FrozenSceneArtwork->SetOpacity(.75f);
        return;
    }
}

void UOGGate12NativeRulerWidget::AddMessage(
    UVerticalBox* Parent, const FString& Value, int32 FontSize, FLinearColor Tint)
{
    if (!Parent || !WidgetTree) return;
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Value));
    Label->SetColorAndOpacity(FSlateColor(Tint));
    Label->SetAutoWrapText(true);
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = FontSize;
    Label->SetFont(Font);
    UVerticalBoxSlot* LayoutSlot = Parent->AddChildToVerticalBox(Label);
    LayoutSlot->SetPadding(FMargin(8.0f, 7.0f));
}

void UOGGate12NativeRulerWidget::AddAction(
    UVerticalBox* Parent, const FString& Title, const FString& Action, bool bEnabled)
{
    if (!Parent || !WidgetTree) return;
    UOGGate12RulerActionButton* Button =
        WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
            UOGGate12RulerActionButton::StaticClass());
    Button->RulerWidget = this;
    Button->ActionId = Action;
    Button->SetIsEnabled(bEnabled);
    Button->SetBackgroundColor(
        bEnabled ? FLinearColor(0.13f, 0.14f, 0.16f, 1.0f)
                 : FLinearColor(0.045f, 0.046f, 0.052f, 1.0f));
    Button->OnClicked.AddDynamic(Button, &UOGGate12RulerActionButton::InvokeAction);
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Title));
    Label->SetColorAndOpacity(FSlateColor(bEnabled ? TextMain : TextMuted));
    Label->SetJustification(ETextJustify::Center);
    Button->AddChild(Label);
    UVerticalBoxSlot* LayoutSlot = Parent->AddChildToVerticalBox(Button);
    LayoutSlot->SetPadding(FMargin(5.0f, 4.0f, 5.0f, 4.0f));
}

void UOGGate12NativeRulerWidget::BuildNav()
{
    if (!NavigationBar || !WidgetTree) return;
    NavigationBar->ClearChildren();
    // Approved Gate10 five-column bottom nav: tall insignia, fine top/bottom
    // rails, actual clickable destinations, invariant black base. Sample
    // accents in the mockup are NOT a universal hard-coded violet theme.
    static const TCHAR* Titles[5] =
    {
        TEXT("HOME"), TEXT("CHARACTERS"), TEXT("GACHA"),
        TEXT("TERRITORY"), TEXT("RECORDS")
    };
    static const TCHAR* Marks[5] =
    {
        TEXT("✧"), TEXT("◇"), TEXT("✦"), TEXT("♜"), TEXT("▤")
    };
    const UGameInstance* GI = GetGameInstance();
    const UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    // Contextual UI metal/insignia tint comes only from an actually selected
    // and owned Manifestation's canonical rarity. Unknown/default is neutral,
    // never a fabricated player's power, world, location or progression.
    FLinearColor ContextAccent(.70f,.69f,.74f,1.f);
    if (Session && Session->SelectedManifestationId.IsValid())
    {
        for (const FOGRosterIdentityViewModel& Identity : Session->OwnedRoster)
        {
            const FOGRosterManifestationViewModel* Selected =
                Identity.Manifestations.FindByPredicate(
                    [Session](const FOGRosterManifestationViewModel& M)
                    {
                        return M.ManifestationId == Session->SelectedManifestationId;
                    });
            if (Selected)
            {
                const FNativeRarityColors Rarity =
                    ResolveRarity(Selected->CurrentRarity);
                if (Rarity.bKnown) ContextAccent = Rarity.Frame;
                break;
            }
        }
    }
    for (int32 Index = 0; Index < 5; ++Index)
    {
        UOGGate12RulerActionButton* Button =
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        const bool bSelected = Session &&
            static_cast<int32>(Session->CurrentDestination) == Index;
        Button->RulerWidget = this;
        Button->ActionId = FString::Printf(TEXT("nav/%d"), Index);
        Button->SetBackgroundColor(
            bSelected ? FLinearColor(.057f,.052f,.068f,1.f)
                      : FLinearColor(.006f,.006f,.009f,1.f));
        Button->OnClicked.AddDynamic(
            Button, &UOGGate12RulerActionButton::InvokeAction);
        UVerticalBox* NativeInsignia =
            WidgetTree->ConstructWidget<UVerticalBox>(
                UVerticalBox::StaticClass());
        UBorder* TopRule = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        TopRule->SetBrushColor(bSelected
            ? ContextAccent : FLinearColor(.27f,.26f,.31f,1.f));
        USizeBox* ThinTopRule = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass());
        ThinTopRule->SetHeightOverride(1.f);
        ThinTopRule->AddChild(TopRule);
        NativeInsignia->AddChildToVerticalBox(ThinTopRule);
        UTextBlock* Icon = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Icon->SetText(FText::FromString(Marks[Index]));
        Icon->SetJustification(ETextJustify::Center);
        Icon->SetColorAndOpacity(FSlateColor(bSelected
            ? ContextAccent : FLinearColor(.57f,.55f,.61f,1.f)));
        FSlateFontInfo IconFont = Icon->GetFont();
        IconFont.Size = 24;
        Icon->SetFont(IconFont);
        NativeInsignia->AddChildToVerticalBox(Icon)
            ->SetPadding(FMargin(0.f,5.f,0.f,0.f));
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Label->SetText(FText::FromString(Titles[Index]));
        Label->SetJustification(ETextJustify::Center);
        Label->SetColorAndOpacity(FSlateColor(bSelected
            ? FLinearColor(.91f,.90f,.92f,1.f)
            : FLinearColor(.65f,.63f,.68f,1.f)));
        FSlateFontInfo LabelFont = Label->GetFont();
        LabelFont.Size = 11;
        Label->SetFont(LabelFont);
        Label->SetAutoWrapText(false);
        NativeInsignia->AddChildToVerticalBox(Label)
            ->SetPadding(FMargin(0.f,0.f,0.f,5.f));
        Button->AddChild(NativeInsignia);
        UHorizontalBoxSlot* NavigationCell =
            NavigationBar->AddChildToHorizontalBox(Button);
        NavigationCell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NavigationCell->SetPadding(FMargin(1.f,0.f));
    }
}

void UOGGate12NativeRulerWidget::RefreshFromCanonical()
{
    if (!Screen) BuildRoot();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;

    if (Session)
    {
        FString Error;
        if (!Session->IsBoundToStoredRuler())
        {
            // Recovery/import can replace the canonical runtime after this
            // widget is constructed. Attempt a READ-ONLY restore.
            Session->TryRestoreLocalRuler(Error);
        }
        if (Session->IsBoundToStoredRuler())
        {
            if (!Session->RefreshCanonicalProjections(Error))
                LastUiError = Error;
            else
                LastUiError.Reset();
        }
        else if (!Error.IsEmpty())
        {
            LastUiError = Error;
        }
    }
    BuildCurrentPage();
}

void UOGGate12NativeRulerWidget::BuildCurrentPage()
{
    if (!Content) return;
    RosterGrid = nullptr;
    TerritoryMapWidget = nullptr;
    Content->ClearChildren();
    if (OpenIdentityIndex == INDEX_NONE || !bPrefer3DCharacterPresentation)
        ReleasePreviewStage();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    RefreshFrozenScene(Session ? Session->CurrentDestination
                              : EOGRulerPrimaryDestination::Home);

    if (!Session || !Session->IsBoundToStoredRuler())
    {
        AddMessage(Content, TEXT("NO CANONICAL RULER SELECTED"), 25, Gate12RulerGold);
        AddMessage(Content,
            TEXT("A local player must exist in the current canonical world. "
                 "Creating one does not grant characters, territory, currency or banners."),
            18, TextMuted);
        AddAction(Content, TEXT("CONTINUE EXISTING RULER"), TEXT("restore-ruler"));
        AddAction(Content, TEXT("CREATE LOCAL RULER"), TEXT("create-ruler"));
        BuildNav();
        return;
    }

    AddMessage(Content, DestinationTitle(Session->CurrentDestination), 26, Gate12RulerGold);
    if (!LastUiError.IsEmpty()) AddMessage(Content, LastUiError, 16, TextMuted);
    switch (Session->CurrentDestination)
    {
    case EOGRulerPrimaryDestination::Home:
        BuildHome();
        break;
    case EOGRulerPrimaryDestination::Characters:
        if (OpenIdentityIndex == INDEX_NONE) BuildRoster();
        else BuildCharacterDetail();
        break;
    case EOGRulerPrimaryDestination::Gacha:
        BuildGacha();
        break;
    case EOGRulerPrimaryDestination::Territory:
        BuildTerritory();
        break;
    case EOGRulerPrimaryDestination::Records:
        BuildRecords();
        break;
    }
    BuildNav();
}


void UOGGate12NativeRulerWidget::BuildHome()
{
    if (!Content || !WidgetTree) return;
    UGameInstance* GI = GetGameInstance();
    const UOGGate12RulerSessionSubsystem* Session =
        GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || !Session->IsBoundToStoredRuler()) return;

    // Frozen Gate10 portrait Ruler Home: full-height character-first stage,
    // narrow left utility rail and three stacked right-side actions.
    // An actual Registry-approved OWNED character, if any, is already painted
    // by RefreshFrozenScene BELOW these non-interactive and UI-only layers.
    // An empty roster remains a correct pure-black stage, never a fake hero.
    UBorder* HeroOuter = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("Gate10HomeStageFineOuterRail"));
    HeroOuter->SetBrushColor(FLinearColor(.21f,.20f,.24f,.71f));
    HeroOuter->SetPadding(FMargin(1.f));
    UBorder* HeroBlack = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("Gate10HomeStageTransparentBlack"));
    HeroBlack->SetBrushColor(FLinearColor(.009f,.008f,.014f,.56f));
    HeroOuter->AddChild(HeroBlack);
    USizeBox* HeroHeight = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("Gate10HomePortraitStageHeight"));
    HeroHeight->SetMinDesiredHeight(425.f);
    HeroHeight->AddChild(HeroOuter);
    Content->AddChildToVerticalBox(HeroHeight)->SetPadding(FMargin(0.f,0.f,0.f,5.f));

    UHorizontalBox* MainStage = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("Gate10HomeLeftStageRightLayout"));
    HeroBlack->SetPadding(FMargin(3.f));
    HeroBlack->AddChild(MainStage);

    UVerticalBox* LeftRail = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("Gate10HomeSideNavigation"));
    UHorizontalBoxSlot* LeftCell = MainStage->AddChildToHorizontalBox(LeftRail);
    LeftCell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    static const TCHAR* RailNames[4] = {
        TEXT("WORLD"), TEXT("CHARACTERS"), TEXT("TERRITORY"), TEXT("RECORDS")
    };
    static const TCHAR* RailActions[4] = {
        TEXT("world"), TEXT("nav/1"), TEXT("nav/3"), TEXT("nav/4")
    };
    static const TCHAR* RailMarks[4] = {
        TEXT("✧"), TEXT("◇"), TEXT("♜"), TEXT("▤")
    };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        UOGGate12RulerActionButton* Action =
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        Action->RulerWidget=this;
        Action->ActionId=RailActions[Index];
        Action->OnClicked.AddDynamic(
            Action, &UOGGate12RulerActionButton::InvokeAction);
        Action->SetBackgroundColor(FLinearColor(.015f,.014f,.021f,.94f));
        UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        UTextBlock* Mark=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Mark->SetText(FText::FromString(RailMarks[Index]));
        Mark->SetJustification(ETextJustify::Center);
        Mark->SetColorAndOpacity(FSlateColor(FLinearColor(.72f,.70f,.77f,1.f)));
        FSlateFontInfo MF=Mark->GetFont(); MF.Size=24; Mark->SetFont(MF);
        Inner->AddChildToVerticalBox(Mark)->SetPadding(FMargin(0.f,2.f));
        UTextBlock* Label=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Label->SetText(FText::FromString(RailNames[Index]));
        Label->SetJustification(ETextJustify::Center);
        Label->SetColorAndOpacity(FSlateColor(TextMain));
        FSlateFontInfo LF=Label->GetFont(); LF.Size=10; Label->SetFont(LF);
        Inner->AddChildToVerticalBox(Label);
        Action->AddChild(Inner);
        LeftRail->AddChildToVerticalBox(Action)
            ->SetPadding(FMargin(2.f,3.f,2.f,10.f));
    }
    // The central stage deliberately has no false silhouette/character art.
    USizeBox* CharacterStage = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("Gate10HomeOnlyActualCharacterStage"));
    MainStage->AddChildToHorizontalBox(CharacterStage)
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UVerticalBox* RightRail = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("Gate10HomeRightActions"));
    MainStage->AddChildToHorizontalBox(RightRail)
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    // Unlike mocked example quest/banner tiles, these 3 controls map
    // directly to real existing Foundation functionality.
    AddMessage(RightRail, TEXT("RULER"), 20,
        FLinearColor(.88f,.86f,.90f,1.f));
    AddMessage(RightRail,
        FString::Printf(TEXT("%d OWNED IDENTITIES"),
            Session->OwnedRoster.Num()), 12, TextMuted);
    AddAction(RightRail, TEXT("ENTER WORLD"), TEXT("world"));
    AddAction(RightRail, TEXT("CHARACTERS"), TEXT("nav/1"));
    AddAction(RightRail, TEXT("RECORDS"), TEXT("nav/4"));

    UBorder* BottomRibbon=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("Gate10HomeCanonicalStateRibbon"));
    BottomRibbon->SetBrushColor(FLinearColor(.018f,.016f,.024f,.96f));
    BottomRibbon->SetPadding(FMargin(6.f,7.f));
    UHorizontalBox* Facts=WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass());
    BottomRibbon->AddChild(Facts);
    auto AddFact=[this,Facts](const FString& Caption)
    {
        UTextBlock* Item=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Item->SetText(FText::FromString(Caption));
        Item->SetColorAndOpacity(FSlateColor(FLinearColor(.74f,.72f,.77f,1.f)));
        Item->SetAutoWrapText(true);
        FSlateFontInfo Font=Item->GetFont(); Font.Size=11; Item->SetFont(Font);
        Facts->AddChildToHorizontalBox(Item)
            ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    };
    AddFact(FString::Printf(TEXT("%d TERRITORIES"),
        Session->TerritoryProjection.Territories.Num()));
    AddFact(FString::Printf(TEXT("%d UNREAD REPORTS"),
        Session->HomeProjection.UnacknowledgedReportCount));
    Content->AddChildToVerticalBox(BottomRibbon);
}

void UOGGate12NativeRulerWidget::HandleRosterSearchChanged(
    const FText& NewText)
{
    RosterSearchQuery = NewText.ToString();
    RebuildRosterGrid();
}

void UOGGate12NativeRulerWidget::BuildRoster()
{
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || Session->OwnedRoster.IsEmpty())
    {
        AddMessage(Content, TEXT("No owned character Manifestations."), 18, TextMuted);
#if !UE_BUILD_SHIPPING
        AddAction(Content,
            bDevelopmentQaViewerOpen
                ? TEXT("CLOSE DEVELOPMENT 3D SOURCE VIEWER")
                : TEXT("VIEW EXISTING LUMINOUS 3D QA SOURCE (NOT OWNED)"),
            bDevelopmentQaViewerOpen ? TEXT("qa/close") : TEXT("qa/open"));
        if (bDevelopmentQaViewerOpen)
            BuildDevelopmentQaCharacterViewer();
#endif
        return;
    }
    UEditableTextBox* Search = WidgetTree->ConstructWidget<UEditableTextBox>(
        UEditableTextBox::StaticClass());
    Search->SetHintText(FText::FromString(TEXT("Search owned characters")));
    Search->SetText(FText::FromString(RosterSearchQuery));
    Search->OnTextChanged.AddDynamic(
        this, &UOGGate12NativeRulerWidget::HandleRosterSearchChanged);
    Content->AddChildToVerticalBox(Search);
    const TCHAR* SortModeName = RosterSortMode == 1 ? TEXT("LEVEL")
        : RosterSortMode == 2 ? TEXT("OWNED COPIES") : TEXT("NAME");
    AddAction(Content,
        FString::Printf(TEXT("SORT: %s"), SortModeName), TEXT("roster/sort"));

    RosterGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(
        UUniformGridPanel::StaticClass());
    RosterGrid->SetSlotPadding(FMargin(3.0f));
    UScaleBox* Responsive = WidgetTree->ConstructWidget<UScaleBox>(
        UScaleBox::StaticClass());
    Responsive->SetStretch(EStretch::ScaleToFit);
    Responsive->AddChild(RosterGrid);
    Content->AddChildToVerticalBox(Responsive);
    RebuildRosterGrid();
}

void UOGGate12NativeRulerWidget::BuildDevelopmentQaCharacterViewer()
{
#if !UE_BUILD_SHIPPING
    if (!Content || !WidgetTree || !GetWorld() ||
        !GetWorld()->IsGameWorld())
        return;

    AddMessage(Content,
        TEXT("DEVELOPMENT QA — real imported CLOTHED Luminous V3. "
             "This is NOT a character you own, NOT a Registry package, "
             "NOT a canonical Identity or a gameplay acquisition."),
        16, Gate12RulerGold);

    UClass* QaClass = LoadClass<AActor>(nullptr,
        TEXT("/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/"
             "BP_LuminousValentineV3_CLOTHED_RuntimeQA."
             "BP_LuminousValentineV3_CLOTHED_RuntimeQA_C"));
    if (!AOGGate12CharacterPreviewStage::IsDevelopmentQaClassAllowed(QaClass))
    {
        AddMessage(Content, TEXT("Real QA source actor not installed."), 16, TextMuted);
        return;
    }

    ReleasePreviewStage();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    PreviewStage = GetWorld()->SpawnActor<AOGGate12CharacterPreviewStage>(
        AOGGate12CharacterPreviewStage::StaticClass(),
        FVector(0.f, 0.f, -200000.f), FRotator::ZeroRotator, Spawn);
    if (!IsValid(PreviewStage) ||
        !PreviewStage->InitializeDevelopmentQaPreview(QaClass))
    {
        ReleasePreviewStage();
        AddMessage(Content,
            TEXT("The actual 45-part source could not initialize. "
                 "No substitute model was displayed."),
            16, TextMuted);
        return;
    }
    // This fixture exists only in non-Shipping builds. Newly loaded real
    // 45-part meshes may have outstanding GPU shader/texture compilation;
    // freezing the first frame produces an empty black render target.
    // Keep the development-only preview live while its page is visible.
    // BuildCurrentPage()/ReleasePreviewStage destroys it on navigation.
    // Canonical production thumbnails retain their separate frozen policy.
    PreviewStage->SetLiveCapture(true);
    FSlateBrush Brush;
    Brush.SetResourceObject(PreviewStage->GetLiveRenderTarget());
    Brush.ImageSize = FVector2D(512.f, 768.f);
    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    Image->SetBrush(Brush);
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->AddChild(Image);
    USizeBox* View = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    View->SetHeightOverride(500.f);
    View->AddChild(Fit);
    Content->AddChildToVerticalBox(View);
    AddMessage(Content,
        TEXT("Existing experimental source, not an accepted production "
             "visual binding. No canonical save data is changed."),
        14, TextMuted);
#endif
}

void UOGGate12NativeRulerWidget::RebuildRosterGrid()
{
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || Session->OwnedRoster.IsEmpty())
    {
        AddMessage(Content, TEXT("No owned character Manifestations."), 18, TextMuted);
        return;
    }

    if (!RosterGrid) return;
    ReleaseRosterPreviewStages();
    RosterGrid->ClearChildren();
    // A search changes the grid contents only; the keyboard never loses focus.
    TMap<FString, bool> Cached3DPreferences;
    FString PreferenceError;
    FOGPlayerProfileSettings CardProfile;
    if (!FOGPlayerProfileSettingsService::Load(
            FOGPlayerProfileSettingsService::DefaultProfilePath(),
            CardProfile, PreferenceError))
    {
        UE_LOG(LogTemp, Warning, TEXT("Character card privacy profile unavailable: %s"),
            *PreferenceError);
        // A corrupt/unknown Privacy state must not expose unapproved artwork.
        CardProfile.bSfwPresentation = true;
    }
    const bool bCardSfwPresentation = CardProfile.bSfwPresentation;
    if (!FOGGate12CharacterPresentationPreferences::LoadAll(
            FOGPlayerProfileSettingsService::DefaultProfilePath(),
            Cached3DPreferences, PreferenceError))
    {
        UE_LOG(LogTemp, Warning, TEXT("Character card preferences unavailable: %s"),
            *PreferenceError);
    }
    TArray<int32> VisibleIndices;
    for (int32 Index = 0; Index < Session->OwnedRoster.Num(); ++Index)
    {
        const FOGRosterIdentityViewModel& Identity = Session->OwnedRoster[Index];
        bool bMatches = RosterSearchQuery.IsEmpty() ||
            Identity.DisplayNameKey.Contains(RosterSearchQuery,
                ESearchCase::IgnoreCase) ||
            Identity.IdentityId.ToString().Contains(RosterSearchQuery,
                ESearchCase::IgnoreCase);
        for (const FOGRosterManifestationViewModel& M : Identity.Manifestations)
        {
            bMatches |= M.BuildLabel.Contains(RosterSearchQuery,
                ESearchCase::IgnoreCase);
        }
        if (bMatches) VisibleIndices.Add(Index);
    }
    auto LevelOf = [Session](int32 Index) -> int32
    {
        const FOGRosterIdentityViewModel& Identity = Session->OwnedRoster[Index];
        const FOGRosterManifestationViewModel* Selected =
            Identity.Manifestations.FindByPredicate(
                [&Identity](const FOGRosterManifestationViewModel& M)
                {
                    return M.ManifestationId == Identity.SelectedManifestationId;
                });
        return Selected ? Selected->Level : 0;
    };
    VisibleIndices.Sort([this, Session, &LevelOf](int32 A, int32 B)
    {
        const FOGRosterIdentityViewModel& Left = Session->OwnedRoster[A];
        const FOGRosterIdentityViewModel& Right = Session->OwnedRoster[B];
        if (RosterSortMode == 1 && LevelOf(A) != LevelOf(B))
            return LevelOf(A) > LevelOf(B);
        if (RosterSortMode == 2 &&
            Left.ManifestationCount != Right.ManifestationCount)
            return Left.ManifestationCount > Right.ManifestationCount;
        const FString L = Left.DisplayNameKey.IsEmpty()
            ? Left.IdentityId.ToString() : Left.DisplayNameKey;
        const FString R = Right.DisplayNameKey.IsEmpty()
            ? Right.IdentityId.ToString() : Right.DisplayNameKey;
        const int32 Compared = L.Compare(R, ESearchCase::IgnoreCase);
        return Compared == 0
            ? Left.IdentityId.ToString().Compare(
                Right.IdentityId.ToString(), ESearchCase::CaseSensitive) < 0
            : Compared < 0;
    });

    // Exactly three uniform portrait cards in original approved proportions.
    constexpr float CardWidth = 144.0f;
    const float CardHeight = CardWidth / GetApprovedRosterCardRatio();
    for (int32 Position = 0; Position < VisibleIndices.Num(); ++Position)
    {
        const int32 Index = VisibleIndices[Position];
        const FOGRosterIdentityViewModel& Identity = Session->OwnedRoster[Index];
        USizeBox* Fixed = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Fixed->SetWidthOverride(CardWidth);
        Fixed->SetHeightOverride(CardHeight);

        UOGGate12RulerActionButton* Button =
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        Button->RulerWidget = this;
        Button->ActionId = FString::Printf(TEXT("identity/%d"), Index);
        Button->OnClicked.AddDynamic(Button, &UOGGate12RulerActionButton::InvokeAction);
        Button->SetBackgroundColor(FLinearColor(0.07f, 0.075f, 0.09f, 1.0f));

        const FOGRosterManifestationViewModel* Selected =
            Identity.Manifestations.FindByPredicate(
                [&Identity](const FOGRosterManifestationViewModel& Candidate)
                {
                    return Candidate.ManifestationId == Identity.SelectedManifestationId;
                });
        const FName Rarity = Selected ? Selected->CurrentRarity : NAME_None;
        const FNativeRarityColors RarityColors = ResolveRarity(Rarity);
        UBorder* AngularFrameBase = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        AngularFrameBase->SetBrushColor(RarityColors.Frame);
        AngularFrameBase->SetPadding(FMargin(1.5f));

        UVerticalBox* CardBody = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        if (RarityColors.bKnown)
            AddMessage(CardBody, Rarity.ToString(), 16, RarityColors.Badge);
        const FOGGate12CharacterVisualEntry* Entry = Selected && VisualCatalog
            ? VisualCatalog->FindExact(Identity.IdentityId,
                Selected->ActiveVersionId) : nullptr;
        const bool bCard3D = Selected && Cached3DPreferences.FindRef(
            FOGGate12CharacterPresentationPreferences::MakeKey(
                Session->BoundRulerId, Identity.IdentityId,
                Selected->ManifestationId));
        UTexture* CardTexture = nullptr;
        FVector2D CardImageSize(512.0f, 768.0f);
        if (bCard3D && Entry && !Entry->GenuineCharacterPreviewClass.IsNull()
            && RosterPreviewStages.Num() < 6 && GetWorld())
        {
            TSubclassOf<AActor> PreviewClass =
                Entry->GenuineCharacterPreviewClass.LoadSynchronous();
            if (PreviewClass)
            {
                FActorSpawnParameters Spawn;
                Spawn.SpawnCollisionHandlingOverride =
                    ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                AOGGate12CharacterPreviewStage* Stage =
                    GetWorld()->SpawnActor<AOGGate12CharacterPreviewStage>(
                        AOGGate12CharacterPreviewStage::StaticClass(),
                        FVector(static_cast<float>(Position * 400),
                                0.0f, -200000.0f),
                        FRotator::ZeroRotator, Spawn);
                if (IsValid(Stage) &&
                    Stage->InitializeApprovedPreview(PreviewClass,
                        Session->BoundRulerId, Identity.IdentityId,
                        Selected->ActiveVersionId, Selected->ManifestationId,
                        bCardSfwPresentation))
                {
                    Stage->SetLiveCapture(false);
                    RosterPreviewStages.Add(Stage);
                    CardTexture = Stage->GetLiveRenderTarget();
                }
                else if (IsValid(Stage))
                    Stage->Destroy();
            }
        }
        else if (!bCard3D && Entry)
        {
            const TSoftObjectPtr<UTexture2D>& Artwork =
                bCardSfwPresentation ? Entry->ApprovedSfwArtwork2D
                                     : Entry->OriginalArtwork2D;
            if (UTexture2D* OriginalArt = Artwork.LoadSynchronous())
            {
                CardTexture = OriginalArt;
                CardImageSize = FVector2D(
                    OriginalArt->GetSizeX(), OriginalArt->GetSizeY());
            }
        }
        if (CardTexture)
        {
            UImage* Portrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
            FSlateBrush ArtBrush;
            ArtBrush.SetResourceObject(CardTexture);
            ArtBrush.ImageSize = CardImageSize;
            Portrait->SetBrush(ArtBrush);
            UScaleBox* FitArt = WidgetTree->ConstructWidget<UScaleBox>(
                UScaleBox::StaticClass());
            FitArt->SetStretch(EStretch::ScaleToFit);
            FitArt->AddChild(Portrait);
            UVerticalBoxSlot* PictureSlot = CardBody->AddChildToVerticalBox(FitArt);
            PictureSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
        else
        {
            UVerticalBoxSlot* Spacer = CardBody->AddChildToVerticalBox(
                WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()));
            Spacer->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            AddMessage(CardBody,
                bCard3D ? TEXT("3D PREVIEW UNAVAILABLE - OPEN CARD")
                : bCardSfwPresentation ? TEXT("SFW ART NOT INSTALLED")
                                       : TEXT("ORIGINAL ART NOT INSTALLED"),
                12, TextMuted);
        }
        AddMessage(CardBody,
            Identity.DisplayNameKey.IsEmpty()
                ? Identity.IdentityId.ToString() : Identity.DisplayNameKey,
            16, TextMain);
        AddMessage(CardBody,
            FString::Printf(TEXT("%d manifestations"), Identity.ManifestationCount),
            12, Gate12RulerGold);
        Button->AddChild(CardBody);
        AngularFrameBase->AddChild(Button);
        Fixed->AddChild(AngularFrameBase);
        RosterGrid->AddChildToUniformGrid(Fixed, Position / 3, Position % 3);
    }
}


void UOGGate12NativeRulerWidget::BuildGacha()
{
    if (!Content || !WidgetTree) return;
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session =
        GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || !Session->IsBoundToStoredRuler()) return;

    // Gate10 frozen 04 visual structure: upper right utility controls, full
    // vertical showcase, compact banner rail, two primary draw buttons and
    // two utility actions. All content remains canonical, NEVER borrowed
    // from the mockup's fictional banner, currency or character.
    UHorizontalBox* Utilities = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("GachaUpperRightUtilityBar"));
    Content->AddChildToVerticalBox(Utilities)->SetPadding(FMargin(5.f,1.f,5.f,9.f));
    {
        UTextBlock* Section=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("GachaSectionTitle"));
        Section->SetText(FText::FromString(TEXT("GACHA")));
        Section->SetColorAndOpacity(FSlateColor(FLinearColor(.81f,.79f,.85f,1.f)));
        FSlateFontInfo F=Section->GetFont();F.Size=19;Section->SetFont(F);
        Utilities->AddChildToHorizontalBox(Section)->SetSize(
            FSlateChildSize(ESlateSizeRule::Fill));
    }
    UVerticalBox* Options=WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("GachaRatesAndHistory"));
    Utilities->AddChildToHorizontalBox(Options);
    // Gate07 must install rate details before this control can be used.
    AddAction(Options, TEXT("RATES"), TEXT("gacha-rates"), false);
    AddAction(Options, bGachaHistoryExpanded ? TEXT("CLOSE HISTORY")
        : TEXT("HISTORY"), TEXT("gacha-history-toggle"));

    UBorder* BannerEdge=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("GachaShowcaseAngularOuterRail"));
    BannerEdge->SetBrushColor(FLinearColor(.34f,.32f,.39f,1.f));
    BannerEdge->SetPadding(FMargin(1.f));
    UBorder* BannerFace=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("GachaShowcaseBlackStage"));
    BannerFace->SetBrushColor(FLinearColor(.006f,.006f,.011f,.85f));
    BannerFace->SetPadding(FMargin(10.f));
    BannerEdge->AddChild(BannerFace);
    USizeBox* BannerHeight=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("GachaTallPortraitShowcase"));
    BannerHeight->SetMinDesiredHeight(335.f);
    BannerHeight->AddChild(BannerEdge);
    Content->AddChildToVerticalBox(BannerHeight)->SetPadding(FMargin(5.f,1.f,5.f,8.f));
    UVerticalBox* Showcase=WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CanonicalGachaShowcase"));
    BannerFace->AddChild(Showcase);
    AddMessage(Showcase, Session->bRealGachaAccessUnlocked
        ? TEXT("ACQUISITION") : TEXT("ACQUISITION LOCKED"), 21,
        FLinearColor(.86f,.84f,.88f,1.f));
    AddMessage(Showcase, Session->bApprovedBannerProvided
        ? TEXT("A verified banner is available from the canonical world.")
        : TEXT("No approved active banner is installed in this world."),
        16, TextMuted);
    // Banner is never invented. Until an authoritative current banner +
    // asset source is present, this intentionally remains an empty dark
    // art stage; it is not a mockup crop.
    USizeBox* BlankArt=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("CanonicalBannerArtOnly"));
    BlankArt->SetMinDesiredHeight(180.f);
    Showcase->AddChildToVerticalBox(BlankArt)
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UBorder* SelectorEdge=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("GachaBannerSelectorThinRail"));
    SelectorEdge->SetBrushColor(FLinearColor(.39f,.36f,.42f,1.f));
    SelectorEdge->SetPadding(FMargin(1.f));
    UBorder* SelectorFace=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass());
    SelectorFace->SetBrushColor(FLinearColor(.014f,.012f,.020f,.96f));
    SelectorFace->SetPadding(FMargin(8.f,11.f));
    SelectorEdge->AddChild(SelectorFace);
    UTextBlock* Selector=WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("LiveBannerSelectorLabel"));
    Selector->SetText(FText::FromString(Session->bApprovedBannerProvided
        ? TEXT("VERIFIED BANNER") : TEXT("NO APPROVED BANNERS")));
    Selector->SetColorAndOpacity(FSlateColor(TextMain));
    Selector->SetJustification(ETextJustify::Center);
    FSlateFontInfo Font=Selector->GetFont();Font.Size=13;Selector->SetFont(Font);
    SelectorFace->AddChild(Selector);
    Content->AddChildToVerticalBox(SelectorEdge)
        ->SetPadding(FMargin(7.f,3.f,7.f,11.f));

    UHorizontalBox* DrawActions=WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("GachaTwoNativeDrawActions"));
    Content->AddChildToVerticalBox(DrawActions)->SetPadding(FMargin(3.f,2.f,3.f,4.f));
    // No native pull UI until the Gate07-approved banner, payment and
    // transactional pull binding are all present. Disabled controls make the
    // frozen structure visible without granting fictional acquisitions.
    const bool bCanDraw=false;
    for (int32 I=0;I<2;++I)
    {
        UVerticalBox* Wrapper=WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        DrawActions->AddChildToHorizontalBox(Wrapper)
            ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        AddAction(Wrapper, I==0 ? TEXT("DRAW  ×1") : TEXT("DRAW  ×10"),
            I==0 ? TEXT("gacha-draw-1") : TEXT("gacha-draw-10"), bCanDraw);
    }
    UHorizontalBox* BottomUtilities=WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("GachaDetailsAndExchange"));
    Content->AddChildToVerticalBox(BottomUtilities);
    UVerticalBox* Details=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UVerticalBox* Exchange=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    BottomUtilities->AddChildToHorizontalBox(Details)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BottomUtilities->AddChildToHorizontalBox(Exchange)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    AddAction(Details,TEXT("DETAILS"),TEXT("gacha-details"),false);
    AddAction(Exchange,TEXT("EXCHANGE"),TEXT("gacha-exchange"),false);

    if (!bGachaHistoryExpanded) return;
    FString Error;
    if (!Session->RefreshGachaHistory(Error))
    {
        AddMessage(Content, Error.IsEmpty() ? TEXT("Acquisition history unavailable.") : Error, 14, TextMuted);
        return;
    }
    if (Session->GachaHistoryProjection.IsEmpty())
    {
        AddMessage(Content, TEXT("No recorded acquisitions."), 15, TextMuted);
        return;
    }
    for (const FOGGachaHistoryEntryViewModel& Entry : Session->GachaHistoryProjection)
        AddMessage(Content, FString::Printf(TEXT("%s  %s  tick %lld"),
            *Entry.IdentityId.ToString(),*Entry.Rarity.ToString(),Entry.WorldTick),
            14, TextMain);
}

void UOGGate12NativeRulerWidget::BuildTerritory()
{
    UGameInstance* GI = GetGameInstance();
    const UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session) return;

    const FOGTerritoryViewModel& Territory = Session->TerritoryProjection;
    AddMessage(Content,
        FString::Printf(TEXT("%d canonical territory holdings"),
            Territory.Territories.Num()), 19, TextMain);


    // A real interactive known-trail viewport, not the fictional landscape
    // from the Gate10 concept. Future accepted Gate02 map tiles can overlay
    // this coordinate-true viewport without replacing its pan/zoom input.
    AddMessage(Content, TEXT("KNOWN EXPLORATION"), 17, Gate12RulerGold);
    TerritoryMapWidget = WidgetTree->ConstructWidget<UOGGate12ExploredMapWidget>(
        UOGGate12ExploredMapWidget::StaticClass(), TEXT("ActualKnownTerritoryViewport"));
    if (const UWorld* World = GetWorld())
    {
        const APlayerController* PC = World->GetFirstPlayerController();
        const AOGWorldPrototypeCharacter* Pawn =
            PC ? Cast<AOGWorldPrototypeCharacter>(PC->GetPawn()) : nullptr;
        if (Pawn)
        {
            const FVector Position = Pawn->GetActorLocation();
            TerritoryMapWidget->SetKnownWorldTrail(
                Pawn->GetVisitedMapTrailForHud(),
                FVector2D(Position.X,Position.Y));
        }
    }
    UBorder* MapFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("TerritoryNativeMapFrame"));
    MapFrame->SetBrushColor(FLinearColor(.37f,.33f,.42f,1.f));
    MapFrame->SetPadding(FMargin(1.f));
    MapFrame->AddChild(TerritoryMapWidget);
    USizeBox* MapHeight = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),TEXT("TerritoryMapViewportHeight"));
    MapHeight->SetMinDesiredHeight(340.f);
    MapHeight->AddChild(MapFrame);
    Content->AddChildToVerticalBox(MapHeight)->SetPadding(FMargin(3.f,4.f,3.f,5.f));
    UHorizontalBox* MapTools=WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(),TEXT("FrozenTerritoryMapTools"));
    Content->AddChildToVerticalBox(MapTools);
    for (const TPair<FString,FString>& Tool :
        {TPair<FString,FString>(TEXT("＋"),TEXT("map/zoom-in")),
         TPair<FString,FString>(TEXT("－"),TEXT("map/zoom-out")),
         TPair<FString,FString>(TEXT("CENTER"),TEXT("map/center"))})
    {
        UVerticalBox* ToolCell=WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        MapTools->AddChildToHorizontalBox(ToolCell)
            ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        AddAction(ToolCell,Tool.Key,Tool.Value);
    }
    if (!TerritoryMapWidget->HasKnownTrail())
        AddMessage(Content,
            TEXT("No mapped path has been discovered in this world yet."),
            13, TextMuted);
    else
        AddMessage(Content,
            TEXT("Known traversed path only. Drag to pan; pinch controls and scroll to zoom. Undiscovered geography remains hidden."),
            12, TextMuted);
    if (Territory.Territories.IsEmpty())
    {
        AddMessage(Content,
            TEXT("No territorial holdings in this world. "
                 "World ownership and geography have not been invented."),
            17, TextMuted);
        return;
    }

    // Existing authoritative view-model overlays are navigation/filter state;
    // no fictional coordinates, world map tiles or terrain are generated.
    AddMessage(Content, TEXT("ANALYTICAL OVERLAY"), 16, Gate12RulerGold);
    for (int32 Index = 0; Index < Territory.AnalyticalOverlays.Num(); ++Index)
    {
        const FName Overlay = Territory.AnalyticalOverlays[Index];
        AddAction(Content,
            FString::Printf(TEXT("%s%s"),
                Index == SelectedTerritoryOverlayIndex ? TEXT("> ") : TEXT(""),
                *Overlay.ToString()),
            FString::Printf(TEXT("overlay/%d"), Index));
    }

    AddMessage(Content, TEXT("MY TERRITORIES"), 16, Gate12RulerGold);
    const FOGTerritorySummaryViewModel* Chosen = nullptr;
    for (int32 Index = 0; Index < Territory.Territories.Num(); ++Index)
    {
        const FOGTerritorySummaryViewModel& Entry = Territory.Territories[Index];
        if (Entry.TerritoryId == SelectedTerritoryId ||
            (!SelectedTerritoryId.IsValid() && Index == 0))
            Chosen = &Entry;

        AddAction(Content,
            FString::Printf(TEXT("%s  %s  (%s)%s"),
                Entry.bMainTerritory ? TEXT("MAIN") : TEXT("REGION"),
                *Entry.TerritoryId.ToString(),
                *Entry.ControlState.ToString(),
                Entry.bContested ? TEXT(" [CONTESTED]") : TEXT("")),
            FString::Printf(TEXT("territory/%d"), Index));
    }

    // The selected GUID may belong to a world that was replaced through
    // recovery. Never show stale details; fall back to a real current row.
    if (!Chosen && !Territory.Territories.IsEmpty())
        Chosen = &Territory.Territories[0];

    if (Chosen)
    {
        AddMessage(Content, TEXT("SELECTED HOLDING"), 18, Gate12RulerGold);
        AddMessage(Content,
            FString::Printf(TEXT("Territory ID: %s"), *Chosen->TerritoryId.ToString()),
            15, TextMain);
        AddMessage(Content,
            FString::Printf(TEXT("Control: %s / Domain: %s"),
                *Chosen->ControlState.ToString(), *Chosen->DomainState.ToString()),
            15, TextMuted);
        if (Chosen->RootLocationId.IsValid())
            AddMessage(Content,
                FString::Printf(TEXT("Root location: %s"),
                    *Chosen->RootLocationId.ToString()), 15, TextMuted);
        if (Chosen->bContested)
            AddMessage(Content, TEXT("Control is contested."), 15, Gate12RulerGold);
    }

    AddMessage(Content,
        TEXT("Interactive geographic imagery will appear only when a verified "
             "Gate 02 location/terrain map is installed. No placeholder coordinates."),
        14, TextMuted);
}

void UOGGate12NativeRulerWidget::BuildRecords()
{
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session) return;

    // Frozen Gate10 Records: a 2x2 native UMG section board with independent
    // real buttons and a canonical detail panel. The illustrated mockup
    // content is not carried over. Exact black frame/panel geometry is live.
    static const TCHAR* Titles[4] = {
        TEXT("REPORTS"), TEXT("CHRONICLE"), TEXT("CODEX"), TEXT("INTELLIGENCE")
    };
    static const TCHAR* IconMarks[4] = {
        TEXT("✧"), TEXT("♜"), TEXT("◇"), TEXT("◈")
    };
    static const TCHAR* Subtitles[4] = {
        TEXT("AUTHORITATIVE REPORTS"), TEXT("KNOWN HISTORY"),
        TEXT("DISCOVERED KNOWLEDGE"), TEXT("KNOWN INTELLIGENCE")
    };
    UBorder* HeaderFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RecordsGothicTitleRail"));
    HeaderFrame->SetBrushColor(FLinearColor(.18f,.17f,.20f,.78f));
    HeaderFrame->SetPadding(FMargin(0.f,1.f,0.f,1.f));
    UBorder* HeaderDark = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RecordsGothicTitleBlack"));
    HeaderDark->SetBrushColor(FLinearColor(.008f,.008f,.012f,.98f));
    HeaderFrame->AddChild(HeaderDark);
    UTextBlock* Head = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("RecordsAuthoredTitle"));
    Head->SetText(FText::FromString(TEXT("R E C O R D S")));
    Head->SetJustification(ETextJustify::Center);
    Head->SetColorAndOpacity(FSlateColor(FLinearColor(.87f,.85f,.88f,1.f)));
    FSlateFontInfo HF = Head->GetFont();
    HF.Size = 27;
    Head->SetFont(HF);
    HeaderDark->SetPadding(FMargin(5.f,16.f,5.f,16.f));
    HeaderDark->AddChild(Head);
    Content->AddChildToVerticalBox(HeaderFrame)->SetPadding(FMargin(0.f,4.f,0.f,13.f));

    UUniformGridPanel* Board = WidgetTree->ConstructWidget<UUniformGridPanel>(
        UUniformGridPanel::StaticClass(), TEXT("FrozenRecordsFourSectionBoard"));
    Board->SetSlotPadding(FMargin(3.f));
    Content->AddChildToVerticalBox(Board);
    for (int32 Index=0; Index<4; ++Index)
    {
        const bool bActive = RecordsSection == Index;
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        Frame->SetBrushColor(bActive
            ? FLinearColor(.55f,.49f,.57f,1.f)
            : FLinearColor(.23f,.22f,.27f,1.f));
        Frame->SetPadding(FMargin(1.f));
        UOGGate12RulerActionButton* Button =
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        Button->RulerWidget = this;
        Button->ActionId = FString::Printf(TEXT("records/%d"), Index);
        Button->SetBackgroundColor(FLinearColor(.010f,.009f,.014f,.97f));
        Button->OnClicked.AddDynamic(
            Button, &UOGGate12RulerActionButton::InvokeAction);
        UVerticalBox* Face = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        UTextBlock* Crest = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Crest->SetText(FText::FromString(IconMarks[Index]));
        Crest->SetJustification(ETextJustify::Center);
        Crest->SetColorAndOpacity(FSlateColor(bActive
            ? FLinearColor(.87f,.85f,.91f,1.f)
            : FLinearColor(.55f,.53f,.61f,1.f)));
        FSlateFontInfo CF = Crest->GetFont(); CF.Size=24; Crest->SetFont(CF);
        Face->AddChildToVerticalBox(Crest)->SetPadding(FMargin(0.f,9.f,0.f,0.f));
        UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Title->SetText(FText::FromString(Titles[Index]));
        Title->SetJustification(ETextJustify::Center);
        Title->SetColorAndOpacity(FSlateColor(TextMain));
        FSlateFontInfo TF = Title->GetFont(); TF.Size=16; Title->SetFont(TF);
        Face->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f,2.f));
        UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Sub->SetText(FText::FromString(Subtitles[Index]));
        Sub->SetJustification(ETextJustify::Center);
        Sub->SetAutoWrapText(true);
        Sub->SetColorAndOpacity(FSlateColor(TextMuted));
        FSlateFontInfo SF = Sub->GetFont(); SF.Size=9; Sub->SetFont(SF);
        Face->AddChildToVerticalBox(Sub)->SetPadding(FMargin(6.f,2.f,6.f,9.f));
        Button->AddChild(Face);
        Frame->AddChild(Button);
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass());
        Size->SetMinDesiredHeight(130.f);
        Size->AddChild(Frame);
        Board->AddChildToUniformGrid(Size, Index/2, Index%2);
    }
    UBorder* DetailFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RecordsCanonicalDetailFrame"));
    DetailFrame->SetBrushColor(FLinearColor(.31f,.30f,.34f,1.f));
    DetailFrame->SetPadding(FMargin(1.f));
    UBorder* DetailDark = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RecordsCanonicalDetailBlack"));
    DetailDark->SetBrushColor(FLinearColor(.009f,.009f,.013f,.95f));
    DetailDark->SetPadding(FMargin(8.f,10.f));
    DetailFrame->AddChild(DetailDark);
    UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("RecordsActualKnowledgeDetail"));
    DetailDark->AddChild(Detail);
    Content->AddChildToVerticalBox(DetailFrame)->SetPadding(FMargin(3.f,13.f,3.f,10.f));
    AddMessage(Detail, Titles[FMath::Clamp(RecordsSection,0,3)], 20, TextMain);

    FString Error;
    if ((RecordsSection == 0 || RecordsSection == 1 || RecordsSection == 3) &&
        !Session->RefreshRecordsDetail(Error))
    {
        AddMessage(Detail, Error.IsEmpty()
            ? TEXT("Canonical Records unavailable.") : Error, 16, TextMuted);
        return;
    }
    switch (RecordsSection)
    {
    case 0:
        AddMessage(Detail,
            FString::Printf(TEXT("%d verified reports / %d unacknowledged"),
                Session->RecordsProjection.ReportCount,
                Session->RecordsProjection.UnacknowledgedReportCount),
            18, TextMain);
        if (Session->ReportProjection.IsEmpty())
        {
            AddMessage(Detail, TEXT("No reports in this world."), 16, TextMuted);
        }
        for (int32 Index = 0; Index < Session->ReportProjection.Num(); ++Index)
        {
            const FOGReportRecord& Report = Session->ReportProjection[Index];
            AddMessage(Detail,
                FString::Printf(TEXT("%s | priority %d | world tick %lld | %s"),
                    *Report.Category.ToString(), Report.Priority,
                    Report.CreatedWorldTick,
                    Report.bAcknowledged ? TEXT("READ") : TEXT("UNREAD")),
                16, Report.bAcknowledged ? TextMuted : TextMain);
            if (!Report.bAcknowledged)
                AddAction(Detail, TEXT("ACKNOWLEDGE REPORT"),
                    FString::Printf(TEXT("report/ack/%d"), Index));
        }
        break;
    case 1:
        if (Session->ChronicleProjection.IsEmpty())
            AddMessage(Detail, TEXT("No known Chronicle entries."), 16, TextMuted);
        for (const FOGChronicleEntryViewModel& Event : Session->ChronicleProjection)
            AddMessage(Detail,
                FString::Printf(TEXT("%s  /  world tick %lld"),
                    *Event.EventType.ToString(), Event.WorldTick),
                16, TextMain);
        break;
    case 2:
        AddMessage(Detail,
            TEXT("No verified installed Codex entries are available. "
                 "Unknown lore remains unknown."),
            16, TextMuted);
        break;
    case 3:
        if (Session->IntelligenceProjection.IsEmpty())
            AddMessage(Detail, TEXT("No known Intelligence entries."), 16, TextMuted);
        for (const FOGIntelligenceEntryViewModel& Entry : Session->IntelligenceProjection)
        {
            // BuildIntelligence applies owner knowledge semantics; do not
            // reveal the value JSON of unknown/outdated intelligence.
            AddMessage(Detail,
                FString::Printf(TEXT("%s / %s"),
                    *Entry.FactKey.ToString(),
                    Entry.Knowledge.KnowledgeState ==
                        EOGUiKnowledgeState::Confirmed
                        ? TEXT("Confirmed") : TEXT("Unconfirmed")),
                16, TextMain);
        }
        break;
    }
}

void UOGGate12NativeRulerWidget::LoadSelectedPresentationPreference(
    const UOGGate12RulerSessionSubsystem* Session, FString& OutError)
{
    OutError.Reset();
    bPrefer3DCharacterPresentation = false;
    if (!Session || !Session->IsBoundToStoredRuler() ||
        !Session->OwnedRoster.IsValidIndex(OpenIdentityIndex))
        return;

    const FOGRosterIdentityViewModel& Identity =
        Session->OwnedRoster[OpenIdentityIndex];
    const FOGRosterManifestationViewModel* Owned =
        Identity.Manifestations.FindByPredicate(
            [&Identity](const FOGRosterManifestationViewModel& M)
            {
                return M.ManifestationId == Identity.SelectedManifestationId;
            });
    if (!Owned || Owned->bLocked) return;

    FOGGate12CharacterPresentationPreferences::Load(
        FOGPlayerProfileSettingsService::DefaultProfilePath(),
        Session->BoundRulerId, Identity.IdentityId,
        Owned->ManifestationId, bPrefer3DCharacterPresentation, OutError);
}

void UOGGate12NativeRulerWidget::AddCharacterVisual(
    UVerticalBox* Parent, const FOGRosterIdentityViewModel& Identity,
    bool bThreeDimensional)
{
    ReleasePreviewStage();
    if (!Parent || !VisualCatalog || !GetWorld())
    {
        AddMessage(Parent, TEXT("Character visual package is not installed."), 15, TextMuted);
        return;
    }

    const UGameInstance* GI = GetGameInstance();
    const UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || !Session->IsBoundToStoredRuler())
    {
        AddMessage(Parent, TEXT("No verified Ruler for character preview."), 15, TextMuted);
        return;
    }

    const FOGRosterManifestationViewModel* Selected =
        Identity.Manifestations.FindByPredicate(
            [&Identity](const FOGRosterManifestationViewModel& Item)
            {
                return Item.ManifestationId == Identity.SelectedManifestationId;
            });
    if (!Selected)
    {
        AddMessage(Parent, TEXT("No verified selected Manifestation."), 15, TextMuted);
        return;
    }

    const FOGGate12CharacterVisualEntry* Entry =
        VisualCatalog->FindExact(Identity.IdentityId, Selected->ActiveVersionId);
    if (!Entry)
    {
        AddMessage(Parent, TEXT("No unambiguous approved visual package for this version."), 15, TextMuted);
        return;
    }

    FOGPlayerProfileSettings DetailProfile;
    FString ProfileError;
    if (!FOGPlayerProfileSettingsService::Load(
            FOGPlayerProfileSettingsService::DefaultProfilePath(),
            DetailProfile, ProfileError))
        DetailProfile.bSfwPresentation = true;
    const bool bDetailSfwPresentation = DetailProfile.bSfwPresentation;

    FSlateBrush Brush;
    if (bThreeDimensional)
    {
        if (Entry->GenuineCharacterPreviewClass.IsNull())
        {
            AddMessage(Parent, TEXT("Genuine 3D character preview not yet installed."), 15, TextMuted);
            return;
        }
        TSubclassOf<AActor> PreviewClass = Entry->GenuineCharacterPreviewClass.LoadSynchronous();
        if (!PreviewClass)
        {
            AddMessage(Parent, TEXT("Approved character 3D preview failed to load."), 15, TextMuted);
            return;
        }
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        PreviewStage = GetWorld()->SpawnActor<AOGGate12CharacterPreviewStage>(
            AOGGate12CharacterPreviewStage::StaticClass(),
            FVector(0.0f, 0.0f, -200000.0f), FRotator::ZeroRotator, Spawn);
        if (!IsValid(PreviewStage) || !PreviewStage->InitializeApprovedPreview(
                PreviewClass, Session->BoundRulerId, Identity.IdentityId,
                Selected->ActiveVersionId, Selected->ManifestationId,
                bDetailSfwPresentation))
        {
            ReleasePreviewStage();
            AddMessage(Parent, TEXT("Live skeletal 3D preview unavailable."), 15, TextMuted);
            return;
        }
        Brush.SetResourceObject(PreviewStage->GetLiveRenderTarget());
        Brush.ImageSize = FVector2D(512.0f, 768.0f);
    }
    else
    {
        const TSoftObjectPtr<UTexture2D>& Artwork =
            bDetailSfwPresentation ? Entry->ApprovedSfwArtwork2D
                                   : Entry->OriginalArtwork2D;
        if (Artwork.IsNull())
        {
            AddMessage(Parent,
                bDetailSfwPresentation
                ? TEXT("Privacy-safe original illustration not yet supplied.")
                : TEXT("Original 2D artwork not yet supplied."), 15, TextMuted);
            return;
        }
        UTexture2D* OriginalArt = Artwork.LoadSynchronous();
        if (!OriginalArt)
        {
            AddMessage(Parent, TEXT("Original 2D artwork failed to load."), 15, TextMuted);
            return;
        }
        Brush.SetResourceObject(OriginalArt);
        Brush.ImageSize = FVector2D(
            static_cast<float>(OriginalArt->GetSizeX()),
            static_cast<float>(OriginalArt->GetSizeY()));
    }

    // ScaleToFit displays the complete artwork/model without cropping or bars.
    UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    Image->SetBrush(Brush);
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->AddChild(Image);
    USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Height->SetHeightOverride(500.0f);
    Height->AddChild(Fit);
    Parent->AddChildToVerticalBox(Height);
}

void UOGGate12NativeRulerWidget::BuildCharacterDetail()
{
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || !Session->OwnedRoster.IsValidIndex(OpenIdentityIndex))
    {
        OpenIdentityIndex = INDEX_NONE;
        BuildRoster();
        return;
    }
    const FOGRosterIdentityViewModel& Identity = Session->OwnedRoster[OpenIdentityIndex];
    AddAction(Content, TEXT("BACK TO ROSTER"), TEXT("back"));
    AddMessage(Content,
        Identity.DisplayNameKey.IsEmpty()
            ? Identity.IdentityId.ToString() : Identity.DisplayNameKey,
        24, TextMain);
    AddMessage(Content,
        FString::Printf(TEXT("%d separately owned Manifestations"),
            Identity.ManifestationCount), 17, TextMuted);
    AddAction(Content, bPrefer3DCharacterPresentation ? TEXT("PRESENTATION: 3D (switch to 2D)") : TEXT("PRESENTATION: 2D (switch to 3D)"), TEXT("presentation/toggle"));
    AddCharacterVisual(Content, Identity, bPrefer3DCharacterPresentation);
    for (int32 Index = 0; Index < Identity.Manifestations.Num(); ++Index)
    {
        const FOGRosterManifestationViewModel& Manifestation =
            Identity.Manifestations[Index];
        FString Label = FString::Printf(
            TEXT("%s  /  level %d  /  %s%s"),
            *Manifestation.BuildLabel,
            Manifestation.Level,
            *Manifestation.CurrentRarity.ToString(),
            Manifestation.bLocked ? TEXT("  [LOCKED]") : TEXT(""));
        AddAction(Content, Label, FString::Printf(TEXT("select/%d"), Index),
            !Manifestation.bLocked);
    }
    const FOGRosterManifestationViewModel* Selected =
        Identity.Manifestations.FindByPredicate(
            [&Identity](const FOGRosterManifestationViewModel& M)
            {
                return M.ManifestationId == Identity.SelectedManifestationId;
            });
    if (VisualCatalog && Selected)
    {
        const FOGGate12CharacterVisualEntry* Art =
            VisualCatalog->FindExact(Identity.IdentityId,
                Selected->ActiveVersionId);
        if (Art && !Art->WardrobeStateJsonField.IsNone() &&
            !Art->ApprovedWardrobeChoices.IsEmpty())
        {
            AddMessage(Content, TEXT("APPROVED WARDROBE"), 18, Gate12RulerGold);
            for (int32 Index = 0; Index < Art->ApprovedWardrobeChoices.Num(); ++Index)
                AddAction(Content, Art->ApprovedWardrobeChoices[Index].ToString(),
                    FString::Printf(TEXT("wardrobe/%d"), Index),
                    !Selected->bLocked);
        }
    }
    AddMessage(Content,
        TEXT("Wardrobe choices write only the selected owned character's "
             "canonical outfit. Privacy is a separate render-only setting."),
        15, TextMuted);
}

void UOGGate12NativeRulerWidget::InvokeAction(const FString& Action)
{
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session)
    {
        LastUiError = TEXT("Canonical Ruler service not loaded.");
        BuildCurrentPage();
        return;
    }

    FString Error;
    if (Action == TEXT("restore-ruler"))
    {
        if (!Session->TryRestoreLocalRuler(Error))
            LastUiError = Error.IsEmpty()
                ? TEXT("No saved local Ruler is available in this world.")
                : Error;
        else
            LastUiError.Reset();
        RefreshFromCanonical();
        return;
    }
    if (Action == TEXT("create-ruler"))
    {
        if (!Session->CreateLocalRuler(Error))
            LastUiError = Error;
        else
            LastUiError.Reset();
        RefreshFromCanonical();
        return;
    }
    if (Action == TEXT("presentation/toggle"))
    {
        if (!Session->IsBoundToStoredRuler() ||
            !Session->OwnedRoster.IsValidIndex(OpenIdentityIndex))
            Error = TEXT("Select an actually owned character before changing presentation.");
        else
        {
            const FOGRosterIdentityViewModel& Identity =
                Session->OwnedRoster[OpenIdentityIndex];
            const FOGRosterManifestationViewModel* Owned =
                Identity.Manifestations.FindByPredicate(
                    [&Identity](const FOGRosterManifestationViewModel& M)
                    {
                        return M.ManifestationId == Identity.SelectedManifestationId;
                    });
            if (!Owned || Owned->bLocked)
                Error = TEXT("A verified unlocked Manifestation is required.");
            else
            {
                const bool bNext = !bPrefer3DCharacterPresentation;
                if (FOGGate12CharacterPresentationPreferences::Save(
                    FOGPlayerProfileSettingsService::DefaultProfilePath(),
                    Session->BoundRulerId, Identity.IdentityId,
                    Owned->ManifestationId, bNext, Error))
                    bPrefer3DCharacterPresentation = bNext;
            }
        }
    }
#if !UE_BUILD_SHIPPING
    else if (Action == TEXT("qa/open") ||
             Action == TEXT("qa/close"))
    {
        // A diagnostic viewer is not a Manifestation or canonical owner.
        bDevelopmentQaViewerOpen = (Action == TEXT("qa/open"));
    }
#endif
    else if (Action == TEXT("roster/sort"))
    {
        RosterSortMode = (RosterSortMode + 1) % 3;
    }
    else if (Action == TEXT("gacha-history-toggle"))
    {
        bGachaHistoryExpanded = !bGachaHistoryExpanded;
    }
    else if (Action.StartsWith(TEXT("map/")))
    {
        if (TerritoryMapWidget)
        {
            if (Action == TEXT("map/zoom-in")) TerritoryMapWidget->StepZoom(1.3f);
            else if (Action == TEXT("map/zoom-out")) TerritoryMapWidget->StepZoom(1.f/1.3f);
            else if (Action == TEXT("map/center")) TerritoryMapWidget->RecenterView();
        }
        return;
    }
    else if (Action.StartsWith(TEXT("territory/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(10));
        if (Session->TerritoryProjection.Territories.IsValidIndex(Index))
            SelectedTerritoryId =
                Session->TerritoryProjection.Territories[Index].TerritoryId;
    }
    else if (Action.StartsWith(TEXT("overlay/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(8));
        if (Session->TerritoryProjection.AnalyticalOverlays.IsValidIndex(Index))
            SelectedTerritoryOverlayIndex = Index;
    }
    else if (Action.StartsWith(TEXT("wardrobe/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(9));
        if (VisualCatalog && Session->OwnedRoster.IsValidIndex(OpenIdentityIndex))
        {
            const FOGRosterIdentityViewModel& Identity =
                Session->OwnedRoster[OpenIdentityIndex];
            const FOGRosterManifestationViewModel* Selected =
                Identity.Manifestations.FindByPredicate(
                    [&Identity](const FOGRosterManifestationViewModel& M)
                    {
                        return M.ManifestationId == Identity.SelectedManifestationId;
                    });
            if (Selected)
            {
                const FOGGate12CharacterVisualEntry* Art =
                    VisualCatalog->FindExact(Identity.IdentityId,
                        Selected->ActiveVersionId);
                if (Art && Art->ApprovedWardrobeChoices.IsValidIndex(Index))
                    Session->SelectOwnedWardrobeChoice(
                        Identity.IdentityId, Selected->ManifestationId,
                        VisualCatalog, Art->ApprovedWardrobeChoices[Index], Error);
                else Error = TEXT("Unrecognized wardrobe choice.");
            }
            else Error = TEXT("No verified selected Manifestation.");
        }
        else Error = TEXT("A verified character visual catalog is required.");
    }
    else if (Action.StartsWith(TEXT("report/ack/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(11));
        if (Session->ReportProjection.IsValidIndex(Index))
            Session->AcknowledgeOwnedReport(
                Session->ReportProjection[Index].ReportId, Error);
    }
    else if (Action.StartsWith(TEXT("records/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(8));
        if (Index < 0 || Index >= 4) return;
        RecordsSection = Index;
    }
    else if (Action.StartsWith(TEXT("nav/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(4));
        if (Index < 0 || Index >= 5) return;
        OpenIdentityIndex = INDEX_NONE;
        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);
    }
    else if (Action.StartsWith(TEXT("identity/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(9));
        if (Session->OwnedRoster.IsValidIndex(Index))
        {
            OpenIdentityIndex = Index;
            LoadSelectedPresentationPreference(Session, Error);
        }
    }
    else if (Action.StartsWith(TEXT("select/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(7));
        if (Session->OwnedRoster.IsValidIndex(OpenIdentityIndex))
        {
            const FOGRosterIdentityViewModel& Identity = Session->OwnedRoster[OpenIdentityIndex];
            if (Identity.Manifestations.IsValidIndex(Index) &&
                Session->SelectOwnedManifestation(Identity.IdentityId,
                    Identity.Manifestations[Index].ManifestationId, Error))
                LoadSelectedPresentationPreference(Session, Error);
        }
    }
    else if (Action == TEXT("back"))
    {
        OpenIdentityIndex = INDEX_NONE;
    }
    else if (Action == TEXT("world"))
    {
        // A verified local Ruler can explore the world using the existing
        // prototype Pawn even before acquiring any character. Requiring an
        // owned Manifestation here made the visible ENTER WORLD button inert
        // in new empty worlds, although the legacy Canvas path worked.
        // If a real owned Manifestation IS selected, fail closed unless its
        // exact approved class/version binding is available.
        bool bApprovedWorldPawn = !Session->SelectedManifestationId.IsValid();
        if (!bApprovedWorldPawn)
        {
            TSubclassOf<AOGWorldPrototypeCharacter> ApprovedPlayable;
            FOGContentId ApprovedVersion;
            bApprovedWorldPawn = Session->ResolveSelectedOwnedWorldPawn(
                VisualCatalog, ApprovedPlayable, ApprovedVersion, Error) &&
                ApprovedPlayable != nullptr;
        }
        if (bApprovedWorldPawn && Session->EnterWorld(Error))
        {
            OnWorldPresentationRequested.Broadcast();
            return;
        }
        if (!bApprovedWorldPawn && Error.IsEmpty())
            Error = TEXT("Selected character has no approved World actor.");
    }

    LastUiError = Error;
    if (Error.IsEmpty()) RefreshFromCanonical();
    else BuildCurrentPage();
}
