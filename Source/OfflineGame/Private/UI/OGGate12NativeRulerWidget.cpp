#include "UI/OGReactiveUiSubsystem.h"
#include "Misc/ScopeExit.h"
#include "UI/OGGate12NativeRulerWidget.h"
#include "Gacha/OGSharedGachaBannerDefinition.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "UI/OGGate12CharacterVisualCatalog.h"
#include "UI/OGGate12ExploredMapWidget.h"
#include "UI/OGGate12GothicChromeWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/OGGate12CharacterPreviewStage.h"
#include "UI/OGGate12CharacterPresentationPreferences.h"
#include "Runtime/OGPlayerProfileSettings.h"
#include "Components/Image.h"
#include "Styling/SlateBrush.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "World/OGStartingRegionPresentation.h"

namespace
{
const FLinearColor TextMain(0.90f, 0.90f, 0.89f, 1.0f);
const FLinearColor TextMuted(0.55f, 0.58f, 0.61f, 1.0f);
const FLinearColor Gate12RulerGold(0.72f, 0.62f, 0.44f, 1.0f);

// A single approved-display typography hierarchy across native Ruler pages.
// The OFL Cinzel faces ship in Content/Slate/Fonts (packaged as UFS), never
// depend on Windows fonts. Keep paragraphs legible with engine-provided
// medium sans; Cinzel is reserved for headings, labels and ornamental actions.
FSlateFontInfo Gate10Type(int32 Size, bool bDisplay = false,
    bool bEmphasis = false)
{
    const FString FontFile = bDisplay
        ? FPaths::ProjectContentDir() / TEXT("Slate/Fonts/")
            / (bEmphasis ? TEXT("Cinzel-Bold.ttf") : TEXT("Cinzel-Regular.ttf"))
        : FPaths::EngineContentDir()
            / TEXT("Slate/Fonts/")
            / (bEmphasis ? TEXT("Roboto-Bold.ttf") : TEXT("Roboto-Medium.ttf"));
    return FSlateFontInfo(FontFile, static_cast<float>(Size));
}

void CenterNativeButton(UOGGate12RulerActionButton* Button, UWidget* Child)
{
    if (!Button || !Child) return;
    if (UButtonSlot* Slot = Cast<UButtonSlot>(Button->AddChild(Child)))
    {
        Slot->SetHorizontalAlignment(HAlign_Fill);
        Slot->SetVerticalAlignment(VAlign_Center);
        Slot->SetPadding(FMargin(4.f,3.f));
    }
}

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

// Default Slate button brushes made black art appear as four grey tiles.
// Keep native mouse/touch, keyboard and action delegates; only omit the
// builtin filled brush so our native Gothic frames render above them.
UOGGate12RulerActionButton::UOGGate12RulerActionButton(
    const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    FSlateBrush Empty;
    Empty.DrawAs = ESlateBrushDrawType::NoDrawType;
    WidgetStyle.SetNormal(Empty);
    WidgetStyle.SetHovered(Empty);
    WidgetStyle.SetPressed(Empty);
    WidgetStyle.SetDisabled(Empty);
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
    if (UOGGameCoreSubsystem* Core = GetGameInstance()
            ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr)
    {
        Core->OnCanonicalRuntimeReleasing.RemoveAll(this);
        Core->OnCanonicalRuntimeReleasing.AddUObject(
            this, &UOGGate12NativeRulerWidget::ResetCharacterDevelopment);
        Core->OnCanonicalRuntimeReleasing.AddUObject(
            this, &UOGGate12NativeRulerWidget::ResetInventoryUi);
        Core->OnCanonicalRuntimeReleasing.AddUObject(
            this, &UOGGate12NativeRulerWidget::ResetNarrativePage);
    }
    RefreshFromCanonical();
}

void UOGGate12NativeRulerWidget::NativeDestruct()
{
    ResetNarrativePage();
    ResetInventoryUi();
    ResetCharacterDevelopment();
    if (UOGGameCoreSubsystem* Core = GetGameInstance()
            ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr)
        Core->OnCanonicalRuntimeReleasing.RemoveAll(this);
    ReleasePreviewStage();
    Super::NativeDestruct();
}

void UOGGate12NativeRulerWidget::NativeTick(
    const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!Gate10FigmaGachaChrome) return;
    const UGameInstance* GI = GetGameInstance();
    const UOGGate12RulerSessionSubsystem* Session =
        GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    const bool bInventoryContextChanged = SynchronizeInventoryContext(Session);
    const bool bNarrativeContextChanged = NarrativePageRulerId.IsValid() &&
        (!Session || !Session->IsBoundToStoredRuler() || Session->BoundRulerId != NarrativePageRulerId);
    if (bNarrativeContextChanged) ResetNarrativePage();
    if (SynchronizeDevelopmentContext(Session) || bInventoryContextChanged || bNarrativeContextChanged)
        BuildCurrentPage();
    const FVector2D ViewportSize = MyGeometry.GetLocalSize();
    // Ruler has NO landscape presentation. During the device's asynchronous
    // rotation from World, hide the whole Ruler surface until portrait fits.
    // In portrait, use the user-approved Gacha canvas or native other pages.
    const bool bPortrait = ViewportSize.Y > ViewportSize.X;
    if (bPortrait && FullPageEmptyCharactersStage)
    {
        // No one-third-screen Character panel on tall phone displays.
        // The safe-zone root has its own top title and bottom nav; this
        // stage owns the entire remaining scrolling content height.
        const float FullHeight=FMath::Max(1130.f,ViewportSize.Y-212.f);
        if (FMath::Abs(FullHeight-LastCharactersViewportHeight)>5.f)
        {
            FullPageEmptyCharactersStage->SetHeightOverride(FullHeight);
            LastCharactersViewportHeight=FullHeight;
        }
    }
    const bool bGachaVisible = bPortrait && Session &&
        Session->CurrentDestination == EOGRulerPrimaryDestination::Gacha;
    bGachaChromeWasVisible = bGachaVisible;
    if (Gate10PortraitGachaCanvas)
        Gate10PortraitGachaCanvas->SetVisibility(
            bGachaVisible ? ESlateVisibility::Visible
                          : ESlateVisibility::Collapsed);
    if (Screen)
        Screen->SetVisibility(
            !bPortrait || bGachaVisible ? ESlateVisibility::Hidden
                                        : ESlateVisibility::Visible);
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
    LivePreviewCutoutMaterial = nullptr;
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
    // The imported Figma layer contains ONLY approved-layout ornaments,
    // with a transparent center and no fictional art, buttons or player data.
    UOverlaySlot* UiSlot = Layers->AddChildToOverlay(Screen);
    UiSlot->SetHorizontalAlignment(HAlign_Fill);
    UiSlot->SetVerticalAlignment(VAlign_Fill);

    // Gate10 native Gacha is anchored DIRECTLY to the live viewport.
    // Normalized Canvas slots remove the zero-desired-size ScaleBox defect.
    // No landscape Ruler fallback exists: World alone owns landscape.
    Gate10PortraitGachaCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("Gate10GachaTrueInteractiveCanvas"));
    Gate10PortraitGachaCanvas->SetVisibility(ESlateVisibility::Collapsed);
    UOverlaySlot* GachaSlot =
        Layers->AddChildToOverlay(Gate10PortraitGachaCanvas);
    GachaSlot->SetHorizontalAlignment(HAlign_Fill);
    GachaSlot->SetVerticalAlignment(VAlign_Fill);
    Gate10FigmaGachaChrome = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("Gate10FigmaGachaDecorOnly"));
    Gate10FigmaGachaChrome->SetVisibility(ESlateVisibility::HitTestInvisible);

    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("GameTitle"));
    Heading->SetText(FText::FromString(TEXT("NoWorld: False Horizon")));
    Heading->SetColorAndOpacity(FSlateColor(Gate12RulerGold));
    Heading->SetJustification(ETextJustify::Center);
    Heading->SetFont(Gate10Type(28,true,true));
    UBorder* StatusBar = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RulerStatusOrnamentalStrip"));
    StatusBar->SetBrushColor(FLinearColor(.010f,.010f,.013f,.94f));
    StatusBar->SetPadding(FMargin(8.f,7.f));
    StatusBar->AddChild(Heading);
    UVerticalBoxSlot* HeadingSlot = Screen->AddChildToVerticalBox(StatusBar);
    HeadingSlot->SetPadding(FMargin(7.0f, 5.0f, 7.0f, 5.0f));

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("RulerScrollableContent"));
    // Keep every real roster, Records and Territory page scrollable, but
    // never expose Slate's bright platform-default scrollbar in the
    // approved dark Ruler presentation (observed as a white right rail).
    Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
    UVerticalBoxSlot* ScrollSlot = Screen->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ScrollSlot->SetPadding(FMargin(9.0f, 4.0f, 9.0f, 4.0f));
    Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CanonicalPageBody"));
    Scroll->AddChild(Content);

    NavigationBar = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("RulerFiveDestinations"));
    UBorder* BottomFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("Gate10BottomNavigationOrnament"));
    BottomFrame->SetBrushColor(FLinearColor(.009f,.009f,.012f,.96f));
    BottomFrame->SetPadding(FMargin(3.f,5.f));
    UOverlay* NavLayers=WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(),TEXT("GachaDerivedFiveNativeNavLayer"));
    // The imported shared raster had an opaque WHITE backing and enlarged
    // filigree crossing all five native hitboxes. Do not draw it. Keep the
    // approved Gacha asset untouched, use separately clipped native dark
    // metal chrome for each real navigation destination below.
    UOverlaySlot* NativeButtons=NavLayers->AddChildToOverlay(NavigationBar);
    NativeButtons->SetHorizontalAlignment(HAlign_Fill);
    NativeButtons->SetVerticalAlignment(VAlign_Fill);
    BottomFrame->AddChild(NavLayers);
    USizeBox* NativeNavHeight = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),TEXT("Gate10FiveNativeButtonsFullHeight"));
    NativeNavHeight->SetMinDesiredHeight(126.f);
    NativeNavHeight->AddChild(BottomFrame);
    UVerticalBoxSlot* NavSlot = Screen->AddChildToVerticalBox(NativeNavHeight);
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
    const bool bChapter = FontSize >= 20 ||
        Value == TEXT("KNOWN EXPLORATION") ||
        Value == TEXT("MY TERRITORIES") ||
        Value == TEXT("ANALYTICAL OVERLAY");
    const int32 ReadableSize = FontSize < 20
        ? FMath::Max(23, FMath::RoundToInt(FontSize*1.42f))
        : FMath::Max(28, FMath::RoundToInt(FontSize*1.20f));
    Label->SetFont(Gate10Type(ReadableSize,bChapter,bChapter));
    // Empty-state messages belong in the centered Gothic page composition.
    // Longer live report text and metadata remain left-aligned for scanning.
    const bool bEmptyState = Value.StartsWith(TEXT("No ")) ||
        Value.StartsWith(TEXT("NO ")) ||
        Value.StartsWith(TEXT("0 "));
    Label->SetJustification(bChapter || bEmptyState
        ? ETextJustify::Center : ETextJustify::Left);
    UVerticalBoxSlot* LayoutSlot = Parent->AddChildToVerticalBox(Label);
    LayoutSlot->SetHorizontalAlignment(HAlign_Fill);
    LayoutSlot->SetPadding(FMargin(10.f,8.f));
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
    Button->SetBackgroundColor(FLinearColor(0.f,0.f,0.f,0.f));
    Button->OnClicked.AddDynamic(Button, &UOGGate12RulerActionButton::InvokeAction);
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Title));
    Label->SetColorAndOpacity(FSlateColor(bEnabled ? TextMain : TextMuted));
    Label->SetJustification(ETextJustify::Center);
    Label->SetFont(Gate10Type(25,true,true));
    CenterNativeButton(Button,Label);
    // The accepted Gate10 ornament is drawn as native vector Slate chrome,
    // with the actual clickable button independently layered above it.
    UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass());
    UOGGate12GothicChromeWidget* Frame =
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass());
    Frame->Configure(bEnabled
        ? FLinearColor(.61f,.58f,.64f,1.f)
        : FLinearColor(.30f,.29f,.34f,1.f),INDEX_NONE,false);
    UOverlaySlot* TouchButton=Layers->AddChildToOverlay(Button);
    TouchButton->SetHorizontalAlignment(HAlign_Fill);
    TouchButton->SetVerticalAlignment(VAlign_Fill);
    Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* TouchChrome=Layers->AddChildToOverlay(Frame);
    TouchChrome->SetHorizontalAlignment(HAlign_Fill);
    TouchChrome->SetVerticalAlignment(VAlign_Fill);
    USizeBox* TouchHeight=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass());
    TouchHeight->SetMinDesiredHeight(62.f);
    TouchHeight->AddChild(Layers);
    UVerticalBoxSlot* LayoutSlot =
        Parent->AddChildToVerticalBox(TouchHeight);
    LayoutSlot->SetPadding(FMargin(3.f,2.f,3.f,2.f));
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
        TEXT("âœ§"), TEXT("â—‡"), TEXT("âœ¦"), TEXT("â™œ"), TEXT("â–¤")
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
        Button->SetBackgroundColor(FLinearColor(0.f,0.f,0.f,0.f));
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
        // Native Slate filigree paints the selected insignia beneath this
        // reserved accessibility-safe vertical glyph space.
        Icon->SetText(FText::FromString(TEXT(" ")));
        Icon->SetJustification(ETextJustify::Center);
        Icon->SetColorAndOpacity(FSlateColor(bSelected
            ? ContextAccent : FLinearColor(.57f,.55f,.61f,1.f)));
        FSlateFontInfo IconFont = Icon->GetFont();
        IconFont.Size = 31;
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
        // Long destination captions must stay inside their actual fifth
        // of the screen; preserve readable point size without clipping.
        // 360px wide S26-like portrait fixture gives only ~70px per
        // destination: preserve all five full labels without text crossing
        // native metal cell borders. The icon supplies visual emphasis.
        const int32 CaptionSize =
            (Index==1 || Index==3) ? 14 : 18;
        Label->SetFont(Gate10Type(CaptionSize,true,true));
        Label->SetAutoWrapText(false);
        Label->SetMinDesiredWidth(0.f);
        UVerticalBoxSlot* InsigniaTitle =
            NativeInsignia->AddChildToVerticalBox(Label);
        InsigniaTitle->SetPadding(FMargin(0.f,2.f,0.f,8.f));
        InsigniaTitle->SetHorizontalAlignment(HAlign_Fill);
        CenterNativeButton(Button,NativeInsignia);
        UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(
            UOverlay::StaticClass());
        UOGGate12GothicChromeWidget* Gothic =
            WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
                UOGGate12GothicChromeWidget::StaticClass());
        Gothic->Configure(bSelected ? ContextAccent
            : FLinearColor(.49f,.46f,.54f,1.f),Index,bSelected);
        UOverlaySlot* RealNavHit=Layers->AddChildToOverlay(Button);
        RealNavHit->SetHorizontalAlignment(HAlign_Fill);
        RealNavHit->SetVerticalAlignment(VAlign_Fill);
        Gothic->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* BoundedNavChrome=Layers->AddChildToOverlay(Gothic);
        BoundedNavChrome->SetHorizontalAlignment(HAlign_Fill);
        BoundedNavChrome->SetVerticalAlignment(VAlign_Fill);
        UHorizontalBoxSlot* NavigationCell =
            NavigationBar->AddChildToHorizontalBox(Layers);
        NavigationCell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NavigationCell->SetPadding(FMargin(1.f,0.f));
    }
}

void UOGGate12NativeRulerWidget::RefreshFromCanonical()
{
    // An explicit refresh may follow recovery of a world with the same IDs.
    // Require a fresh review rather than keeping a previous world's quote.
    ResetCharacterDevelopment();
    ClearInventoryReview();
    ClearNarrativeReview();
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
    ON_SCOPE_EXIT { if (auto* Theme=UOGReactiveUiSubsystem::Find(this)) Theme->TrackTree(this); };

    if (!Content) return;
    RosterGrid = nullptr;
    TerritoryMapWidget = nullptr;
    FullPageEmptyCharactersStage = nullptr;
    LastCharactersViewportHeight = 0.f;
    Content->ClearChildren();
    if (OpenIdentityIndex == INDEX_NONE || !bPrefer3DCharacterPresentation)
        ReleasePreviewStage();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    SynchronizeDevelopmentContext(Session);
    SynchronizeInventoryContext(Session);
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

    // Common Gate10 gothic chapter heading on all five actual Ruler pages.
    // Same black/silver angular chrome as the approved Gacha screen;
    // destination symbols are neutral UI navigation, not invented lore.
    UOverlay* HeadingLayers=WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), TEXT("Gate10SharedRulerChapterHeader"));
    UOGGate12GothicChromeWidget* HeadingFrame=
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass());
    HeadingFrame->Configure(FLinearColor(.65f,.59f,.75f,1.f),
        static_cast<int32>(Session->CurrentDestination),true);
    HeadingLayers->AddChildToOverlay(HeadingFrame);
    UTextBlock* HeadingText=WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(),TEXT("CanonicalDestinationChapterTitle"));
    HeadingText->SetText(FText::FromString(
        DestinationTitle(Session->CurrentDestination)));
    HeadingText->SetJustification(ETextJustify::Center);
    HeadingText->SetColorAndOpacity(FSlateColor(
        FLinearColor(.88f,.83f,.95f,1.f)));
    HeadingText->SetFont(Gate10Type(42,true,true));
    UOverlaySlot* HeadingTextSlot=HeadingLayers->AddChildToOverlay(HeadingText);
    HeadingTextSlot->SetHorizontalAlignment(HAlign_Fill);
    HeadingTextSlot->SetVerticalAlignment(VAlign_Center);
    USizeBox* HeadingHeight=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass());
    HeadingHeight->SetMinDesiredHeight(112.f);
    HeadingHeight->AddChild(HeadingLayers);
    if (Session->CurrentDestination != EOGRulerPrimaryDestination::Home)
        Content->AddChildToVerticalBox(HeadingHeight)
            ->SetPadding(FMargin(4.f,0.f,4.f,8.f));
    if (!LastUiError.IsEmpty()) AddMessage(Content, LastUiError, 16, TextMuted);
    if (bInventoryOpen)
    {
        BuildInventory();
        BuildNav();
        return;
    }
    switch (Session->CurrentDestination)
    {
    case EOGRulerPrimaryDestination::Home:
        AddAction(Content, TEXT("INVENTORY & EQUIPMENT"), TEXT("inventory/open"));
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
    const UGameInstance* GI = GetGameInstance();
    const UOGGate12RulerSessionSubsystem* Session =
        GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || !Session->IsBoundToStoredRuler()) return;

    // One native viewport-anchored Home stage, using the accepted Gacha
    // screen's permanent obsidian, silver borders and restrained typography.
    // A full-height grey placeholder and a second nav rail are prohibited.
    USizeBox* Bounds=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),TEXT("Gate10HomeOneSceneNotThreeColumns"));
    Bounds->SetMinDesiredWidth(390.f);
    // The 505-unit stage rendered only 216 physical pixels on the real
    // portrait fixture. Reserve the actual available tall viewport rather
    // than reproducing a short banner inside an otherwise empty Ruler page.
    Bounds->SetHeightOverride(1250.f);
    UCanvasPanel* Scene=WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(),TEXT("Gate10HomeAnchoredPortraitScene"));
    Bounds->AddChild(Scene);
    Content->AddChildToVerticalBox(Bounds)->SetPadding(FMargin(2.f,3.f,2.f,4.f));

    auto Position=[Scene](UWidget* Widget,float X,float Y,
        float X2,float Y2)
    {
        UCanvasPanelSlot* Slot=Scene->AddChildToCanvas(Widget);
        Slot->SetAnchors(FAnchors(X,Y,X2,Y2));
        Slot->SetOffsets(FMargin(0.f));
    };
    UBorder* Void=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),TEXT("Gate10HomeObsidianVoid"));
    // Owned source-backed character art can remain visible behind Home.
    // Do not manufacture a portrait for an empty Registry roster.
    Void->SetBrushColor(FLinearColor(.003f,.003f,.007f,
        Session->OwnedRoster.IsEmpty() ? .92f : .17f));
    Position(Void,0.f,0.f,1.f,1.f);
    UOGGate12GothicChromeWidget* HomeRails =
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass(),
            TEXT("Gate10HomeSharedGachaOrnamentalRails"));
    HomeRails->Configure(FLinearColor(.58f,.56f,.63f,1.f),
        INDEX_NONE,false);
    HomeRails->SetVisibility(ESlateVisibility::HitTestInvisible);
    Position(HomeRails,.016f,.011f,.983f,.994f);

    // The former Home wallpaper embedded three obsolete duplicate actions.
    // Do NOT flatten, mask with opaque panels or present unclickable chrome.
    // Use the ALREADY APPROVED Opening 04 throne architecture as a subtle
    // full-width environment backdrop. Canonical content remains a separate
    // native interactive layer; no character image or ownership is fabricated.
    if (UTexture2D* HomeArt=LoadObject<UTexture2D>(nullptr,
        TEXT("/Game/UI/Gate10/Opening/T_Gate10_VacantThroneBackdrop_V1."
             "T_Gate10_VacantThroneBackdrop_V1")))
    {
        UImage* Environment=WidgetTree->ConstructWidget<UImage>(
            UImage::StaticClass(),TEXT("Gate10HomeApprovedThroneEnvironment"));
        Environment->SetBrushFromTexture(HomeArt,true);
        Environment->SetVisibility(ESlateVisibility::HitTestInvisible);
        Environment->SetOpacity(Session->OwnedRoster.IsEmpty() ? .56f : .26f);
        Position(Environment,0.f,.08f,1.f,.90f);
    }

    auto TextAt=[this,&Position](const FString& String,
        const TCHAR* Key,float X,float Y,float X2,float Y2,
        int32 Size,const FLinearColor& Ink)
    {
        UTextBlock* Text=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),FName(Key));
        Text->SetText(FText::FromString(String));
        Text->SetJustification(ETextJustify::Center);
        Text->SetAutoWrapText(true);
        Text->SetColorAndOpacity(FSlateColor(Ink));
        Text->SetFont(Gate10Type(Size,
            Size>=22, Size>=40));
        Position(Text,X,Y,X2,Y2);
    };
    const FLinearColor Silver(.84f,.81f,.90f,1.f);
    const FLinearColor Dim(.56f,.53f,.64f,1.f);
    // Ruler is the canonical account/world actor, NOT a Character
    // Manifestation. Never show opaque persistence IDs or promote empty
    // ownership records to the Home screen.
    TextAt(TEXT("RULER MODE"),TEXT("Gate10HomeActualModeTitle"),
        .075f,.045f,.92f,.13f,45,Silver);
    TextAt(TEXT("WORLD OVERVIEW"),TEXT("Gate10HomeCanonicalOverview"),
        .12f,.535f,.88f,.61f,27,Silver);

    auto NativeAction=[this,&Position](const TCHAR* Caption,
        const TCHAR* Action,float Y)
    {
        UOverlay* Surface=WidgetTree->ConstructWidget<UOverlay>(
            UOverlay::StaticClass());
        UBorder* Back=WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        Back->SetBrushColor(FLinearColor(.014f,.010f,.026f,.96f));
        UOverlaySlot* BackFill=Surface->AddChildToOverlay(Back);
        BackFill->SetHorizontalAlignment(HAlign_Fill);
        BackFill->SetVerticalAlignment(VAlign_Fill);
        UOGGate12GothicChromeWidget* Frame=
            WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
                UOGGate12GothicChromeWidget::StaticClass());
        Frame->Configure(FLinearColor(.74f,.65f,.83f,1.f),
            INDEX_NONE,false);
        Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* FrameFill=Surface->AddChildToOverlay(Frame);
        FrameFill->SetHorizontalAlignment(HAlign_Fill);
        FrameFill->SetVerticalAlignment(VAlign_Fill);
        UOGGate12RulerActionButton* Button=
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        Button->RulerWidget=this;
        Button->ActionId=Action;
        Button->SetBackgroundColor(FLinearColor(0.f,0.f,0.f,0.f));
        Button->OnClicked.AddDynamic(
            Button,&UOGGate12RulerActionButton::InvokeAction);
        UTextBlock* Label=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Label->SetText(FText::FromString(Caption));
        Label->SetJustification(ETextJustify::Center);
        // The three Figma chrome actions are narrow. Center compact,
        // single-line text inside each full-surface native hit target.
        Label->SetAutoWrapText(false);
        Label->SetColorAndOpacity(FSlateColor(
            FLinearColor(.87f,.84f,.93f,1.f)));
        // Captured 26 pt ENTER WORLD clipped left by 4 pixels; 24 pt
        // stays readable and remains entirely within the approved rail.
        Label->SetFont(Gate10Type(24,true,true));
        CenterNativeButton(Button,Label);
        UOverlaySlot* ButtonFill=Surface->AddChildToOverlay(Button);
        ButtonFill->SetHorizontalAlignment(HAlign_Fill);
        ButtonFill->SetVerticalAlignment(VAlign_Fill);
        Position(Surface,.17f,Y,.83f,Y+.095f);
    };
    // World is the only Home CTA. Characters/Gacha remain in the permanent
    // five-way nav; duplicate destinations are removed, not merely hidden.
    NativeAction(TEXT("ENTER WORLD"),TEXT("world"),.735f);

    UBorder* Rule=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass());
    Rule->SetBrushColor(FLinearColor(.38f,.34f,.46f,.74f));
    Position(Rule,.06f,.917f,.94f,.919f);
    TextAt(FString::Printf(TEXT("%d TERRITORIES"),
        Session->TerritoryProjection.Territories.Num()),
        TEXT("HomeRealTerritoryCount"),.055f,.933f,.46f,.975f,18,Dim);
    TextAt(FString::Printf(TEXT("%d UNREAD REPORTS"),
        Session->HomeProjection.UnacknowledgedReportCount),
        TEXT("HomeActualReportCount"),.52f,.933f,.96f,.975f,18,Dim);
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
    // The protagonist is always available independently of gacha ownership.
    // This is NOT a character Manifestation and is never counted as one.
    if (Session && Session->IsBoundToStoredRuler())
    {
        AddMessage(Content, TEXT("RULER â€” ORIGINAL PLAYER"), 22, TextMain);
        AddAction(Content, TEXT("CONTROL THE RULER"), TEXT("select-ruler"));
        AddAction(Content, TEXT("INTERACTIONS"), TEXT("interaction/open"));
    }
    if (!Session || Session->OwnedRoster.IsEmpty())
    {
        // No invented portrait, no stand-alone rotating QA tab in the
        // production roster. The per-owned-Manifestation 2D/3D switch is
        // already implemented in BuildCharacterDetail().
        USizeBox* EmptyStage=WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),TEXT("CanonicalEmptyOwnedRosterStage"));
        // Characters is its OWN full-viewport destination, not a 620-unit
        // inset inside Home. The viewport is ~1250 Slate layout units on
        // our 432x700 portrait fixture (DPI ~0.5). Fill it completely.
        EmptyStage->SetMinDesiredHeight(1130.f);
        FullPageEmptyCharactersStage=EmptyStage;
        UOverlay* EmptyLayers=WidgetTree->ConstructWidget<UOverlay>(
            UOverlay::StaticClass(),TEXT("CharacterEmptyFullscreenLayers"));
        UBorder* Black=WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        Black->SetBrushColor(FLinearColor(.004f,.003f,.009f,1.f));
        UOverlaySlot* BlackFill=EmptyLayers->AddChildToOverlay(Black);
        BlackFill->SetHorizontalAlignment(HAlign_Fill);
        BlackFill->SetVerticalAlignment(VAlign_Fill);
        // Preserve the approved atmospheric artwork: this is scenery, not
        // an invented hero or a duplicate of a non-owned Manifestation.
        if (UTexture2D* ApprovedBack=LoadObject<UTexture2D>(nullptr,
            TEXT("/Game/UI/Gate10/Opening/T_Gate10_VacantThroneBackdrop_V1."
                 "T_Gate10_VacantThroneBackdrop_V1")))
        {
            UImage* AtmosphericArtwork=WidgetTree->ConstructWidget<UImage>(
                UImage::StaticClass(),TEXT("CharacterFullPortraitBackdrop"));
            AtmosphericArtwork->SetBrushFromTexture(ApprovedBack,true);
            AtmosphericArtwork->SetOpacity(.38f);
            AtmosphericArtwork->SetVisibility(ESlateVisibility::HitTestInvisible);
            UOverlaySlot* ArtFill=EmptyLayers->AddChildToOverlay(AtmosphericArtwork);
            ArtFill->SetHorizontalAlignment(HAlign_Fill);
            ArtFill->SetVerticalAlignment(VAlign_Fill);
        }
        UOGGate12GothicChromeWidget* Frame=
            WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
                UOGGate12GothicChromeWidget::StaticClass());
        Frame->Configure(FLinearColor(.48f,.43f,.57f,.72f),INDEX_NONE,false);
        Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* FrameSlot=EmptyLayers->AddChildToOverlay(Frame);
        FrameSlot->SetHorizontalAlignment(HAlign_Fill);
        FrameSlot->SetVerticalAlignment(VAlign_Fill);
        UVerticalBox* Copy=WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        AddMessage(Copy,TEXT("NO OWNED CHARACTERS"),31,TextMain);
        AddMessage(Copy,
            TEXT("Acquired characters appear here."),
            18,TextMuted);
        UOverlaySlot* CopySlot=EmptyLayers->AddChildToOverlay(Copy);
        CopySlot->SetHorizontalAlignment(HAlign_Fill);
        CopySlot->SetVerticalAlignment(VAlign_Center);
        EmptyStage->AddChild(EmptyLayers);
        UVerticalBoxSlot* FullPage=Content->AddChildToVerticalBox(EmptyStage);
        FullPage->SetHorizontalAlignment(HAlign_Fill);
        FullPage->SetPadding(FMargin(1.f,2.f,1.f,4.f));
#if !UE_BUILD_SHIPPING
        // The automated noncanonical source integrity fixture still needs
        // its direct qa/open entry point. It is intentionally NOT a visible
        // roster action or a fictional owned Character card.
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
    RosterGrid->SetSlotPadding(FMargin(5.f,7.f));
    UScaleBox* Responsive = WidgetTree->ConstructWidget<UScaleBox>(
        UScaleBox::StaticClass());
    Responsive->SetStretch(EStretch::ScaleToFit);
    Responsive->SetStretchDirection(EStretchDirection::DownOnly);
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
        TEXT("DEVELOPMENT QA â€” real imported CLOTHED Luminous V3. "
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
    static const TCHAR* UiCutoutPath=TEXT(
        "/Game/UI/Gate10/Materials/M_Gate10_CharacterCaptureAlphaUI."
        "M_Gate10_CharacterCaptureAlphaUI");
    if (UMaterialInterface* Mat=LoadObject<UMaterialInterface>(
        nullptr,UiCutoutPath))
    {
        LivePreviewCutoutMaterial=UMaterialInstanceDynamic::Create(Mat,this);
        LivePreviewCutoutMaterial->SetTextureParameterValue(
            TEXT("LiveScene"),PreviewStage->GetLiveRenderTarget());
        Image->SetBrushFromMaterial(LivePreviewCutoutMaterial);
    }
    else
    {
        Image->SetBrush(Brush);
        UE_LOG(LogTemp,Warning,
            TEXT("Original character preview alpha-compositing asset missing"));
    }
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->AddChild(Image);
    USizeBox* View = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    View->SetMinDesiredWidth(380.f);
    View->SetMinDesiredHeight(590.f);
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

    // Exactly three TALL character portraits filling the full portrait
    // page. 144 Slate units previously used barely half its available
    // width; 262 x 3 plus gutters fits the 840-unit content viewport.
    constexpr float CardWidth = 262.0f;
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



void UOGGate12NativeRulerWidget::BuildGachaPortrait()
{
    if (!Gate10PortraitGachaCanvas || !WidgetTree) return;
    Gate10PortraitGachaCanvas->ClearChildren();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session =
        GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || !Session->IsBoundToStoredRuler()) return;

    AOGWorldPresentationHud* Hud = GetOwningPlayer()
        ? Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()) : nullptr;
    const bool bCanSummon = Hud && Hud->HasActiveGachaPresentation() &&
        Session->bRealGachaAccessUnlocked;

    UImage* Chrome = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("Gate10GachaOrnamentNotControls"));
    Gate10FigmaGachaChrome = Chrome;
    Chrome->SetVisibility(ESlateVisibility::HitTestInvisible);
    // Cooked UTexture2D imported directly from Figma, NOT runtime PNG
    // downloaded from the network and NOT an entire flattened interactive UI.
    static const TCHAR* ChromePath =
        TEXT("/Game/UI/Gate10/Chrome/T_Gate10_Gacha_VectorChrome_V2."
             "T_Gate10_Gacha_VectorChrome_V2");
    UTexture2D* Art = LoadObject<UTexture2D>(nullptr, ChromePath);
    if (Art) Chrome->SetBrushFromTexture(Art, true);
    UCanvasPanelSlot* ChromeSlot =
        Gate10PortraitGachaCanvas->AddChildToCanvas(Chrome);
    ChromeSlot->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));
    ChromeSlot->SetOffsets(FMargin(0.f));

    // Small independent text/button controls exactly track design coordinates.
    // All app data comes from the stored Ruler; zero banner remains zero.
    const FLinearColor Main(.94f,.91f,.98f,1.f);
    const FLinearColor GachaMuted(.73f,.68f,.79f,1.f);
    auto TextAt = [this](FString Value,FString Name,
        float X,float Y,float W,float H,int32 Size,
        FLinearColor Ink) -> UTextBlock*
    {
        UTextBlock* Label=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),FName(*Name));
        Label->SetText(FText::FromString(Value));
        Label->SetJustification(ETextJustify::Center);
        Label->SetAutoWrapText(true);
        Label->SetColorAndOpacity(FSlateColor(Ink));
        // Preserve Figma's exact anchors and decorative chrome; elevate
        // only real text legibility and the display family.
        Label->SetFont(Gate10Type(
            FMath::Max(17,FMath::RoundToInt(Size*.82f)),
            Size>=24,Size>=38));
        UCanvasPanelSlot* Slot=
            Gate10PortraitGachaCanvas->AddChildToCanvas(Label);
        // Explicit normalized multi-anchor rectangle in the actual viewport.
        // No centered ScaleBox or unscaled pixel offsets can clip the page.
        Slot->SetAnchors(FAnchors(
            X/864.f,Y/1536.f,(X+W)/864.f,(Y+H)/1536.f));
        Slot->SetOffsets(FMargin(0.f));
        return Label;
    };
    auto ActionAt = [this](FString Name,FString Action,
        float X,float Y,float W,float H,
        bool Enabled,int32 Size) -> UOGGate12RulerActionButton*
    {
        UOGGate12RulerActionButton* Button =
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        Button->RulerWidget=this;
        Button->ActionId=Action;
        Button->SetIsEnabled(Enabled);
        Button->SetBackgroundColor(FLinearColor(0.f,0.f,0.f,0.f));
        Button->OnClicked.AddDynamic(
            Button,&UOGGate12RulerActionButton::InvokeAction);
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Text->SetText(FText::FromString(Name));
        Text->SetJustification(ETextJustify::Center);
        Text->SetAutoWrapText(true);
        Text->SetColorAndOpacity(FSlateColor(
            Enabled ? FLinearColor(.91f,.87f,.96f,1.f)
                    : FLinearColor(.57f,.54f,.65f,1.f)));
        Text->SetFont(Gate10Type(
            FMath::Max(18,FMath::RoundToInt(Size*.86f)),
            true,Size>=30));
        CenterNativeButton(Button,Text);
        UCanvasPanelSlot* Slot =
            Gate10PortraitGachaCanvas->AddChildToCanvas(Button);
        Slot->SetAnchors(FAnchors(
            X/864.f,Y/1536.f,(X+W)/864.f,(Y+H)/1536.f));
        Slot->SetOffsets(FMargin(0.f));
        return Button;
    };
    const FString RulerId=Session->BoundRulerId.ToString();
    TextAt(TEXT("RULER"),TEXT("ActualRulerIdentityLabel"),
        112.f,55.f,170.f,35.f,25,Main);
    TextAt(FString::Printf(TEXT("ID %s"),*RulerId.Left(12)),
        TEXT("ActualStoredRulerID"),112.f,89.f,176.f,26.f,18,GachaMuted);
    ActionAt(TEXT("RATES"),TEXT("gacha-rates"),686.f,132.f,70.f,88.f,bCanSummon,18);
    ActionAt(TEXT("HISTORY"),TEXT("gacha-history-toggle"),
        761.f,132.f,94.f,88.f,true,18);

    if (!Hud || !Hud->HasActiveGachaPresentation())
        TextAt(TEXT("NO ACTIVE BANNER"),
            TEXT("AuthoritativeNoActiveBanner"),155.f,830.f,555.f,96.f,48,Main);
    else
        TextAt(FOGInstalledGachaBannerLibrary::DisplayName(Hud->GetActiveGachaBanner().BannerId),
            TEXT("AuthoritativeInstalledBannerId"),108.f,812.f,640.f,118.f,35,Main);

    TArray<FOGGachaBannerDefinition> InstalledBanners; FString BannerError;
    if (Hud) Hud->ListInstalledGachaBanners(InstalledBanners,BannerError);
    ActionAt(TEXT("PREVIOUS"),TEXT("gacha-banner-previous"),68.f,1038.f,242.f,80.f,InstalledBanners.Num()>1,21);
    const int32 BannerIndex=Hud?InstalledBanners.IndexOfByPredicate([&](const auto& B)
        {return B.BannerId==Hud->GetActiveGachaBanner().BannerId;}):INDEX_NONE;
    TextAt(BannerIndex==INDEX_NONE?TEXT("NO BANNER"):FString::Printf(TEXT("%d / %d"),BannerIndex+1,InstalledBanners.Num()),
        TEXT("RegisteredBannerPosition"),332.f,1038.f,242.f,80.f,25,Main);
    ActionAt(TEXT("NEXT"),TEXT("gacha-banner-next"),595.f,1038.f,243.f,80.f,InstalledBanners.Num()>1,21);
    // A pull must NEVER fire before Gate07's actual banner/economy transaction
    // has been bound. No invented pity/tickets/currency are displayed.
    ActionAt(TEXT("DRAW Ã—1"),TEXT("gacha-draw-1"),
             50.f,1170.f,309.f,100.f,bCanSummon,37);
    ActionAt(TEXT("DRAW Ã—10"),TEXT("gacha-draw-10"),
             511.f,1170.f,311.f,100.f,bCanSummon,37);
    ActionAt(TEXT("DETAILS"),TEXT("gacha-rates"),
             151.f,1289.f,223.f,50.f,bCanSummon,24);
    ActionAt(TEXT("EXCHANGE"),TEXT("gacha-exchange"),
             488.f,1289.f,227.f,50.f,false,24);

    static const TCHAR* Destinations[] =
    {TEXT("HOME"),TEXT("CHARACTERS"),TEXT("GACHA"),
     TEXT("TERRITORY"),TEXT("RECORDS")};
    for (int32 Index=0; Index<5; ++Index)
        ActionAt(Destinations[Index],
                 FString::Printf(TEXT("nav/%d"),Index),
                 10.f+171.f*Index,1393.f,167.f,111.f,true,
                 Index==1 ? 19 : 22);

    if (bGachaResultsExpanded || bGachaRatesExpanded)
    {
        const bool bShowRates = bGachaRatesExpanded;
        UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),TEXT("CanonicalGachaResultOrRatesPanel"));
        Panel->SetBrushColor(FLinearColor(.008f,.005f,.016f,.985f));
        Panel->SetPadding(FMargin(18.f));
        UCanvasPanelSlot* PortraitCanvasSlot=Gate10PortraitGachaCanvas->AddChildToCanvas(Panel);
        PortraitCanvasSlot->SetAnchors(FAnchors(100.f/864.f,250.f/1536.f,
                                 765.f/864.f,1020.f/1536.f));
        PortraitCanvasSlot->SetOffsets(FMargin(0.f));
        UScrollBox* Scroll=WidgetTree->ConstructWidget<UScrollBox>(
            UScrollBox::StaticClass());
        Panel->AddChild(Scroll);
        UVerticalBox* Rows=WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        Scroll->AddChild(Rows);
        if (bShowRates && Hud && Hud->HasActiveGachaPresentation())
        {
            const FOGGachaBannerDefinition& Banner=Hud->GetActiveGachaBanner();
            AddMessage(Rows,TEXT("AUTHORITATIVE BANNER RATES"),26,Main);
            AddMessage(Rows,Banner.BannerId.ToString(),17,GachaMuted);
            int64 Total=0;
            for (const FOGGachaPoolEntry& E:Banner.Entries)
                Total+=FMath::Max(0,E.Weight);
            for (const FOGGachaPoolEntry& E:Banner.Entries)
            {
                const double Rate=Total>0
                    ? 100.0*static_cast<double>(E.Weight)/static_cast<double>(Total)
                    : 0.0;
                AddMessage(Rows,FString::Printf(TEXT("%s / %s / %.3f%%%s"),
                    *E.IdentityId.ToString(),*E.Rarity.ToString(),Rate,
                    E.bFeatured?TEXT(" / FEATURED"):TEXT("")),17,Main);
            }
            AddMessage(Rows,TEXT("Displayed weights are BASE rates. Persisted pity and featured guarantees apply to individual draws."),17,GachaMuted);
        }
        else if (Hud)
        {
            AddMessage(Rows,TEXT("ACTUAL SUMMON RESULTS"),26,Main);
            for (const FOGGachaPullResult& Result:Hud->GetLastGachaResults())
                AddMessage(Rows,FString::Printf(
                    TEXT("%s / %s / %s%s"),
                    *Result.IdentityId.ToString(),*Result.Rarity.ToString(),
                    *Result.ManifestationId.ToString(),
                    Result.bDuplicateIdentity?TEXT(" / INDEPENDENT COPY"):TEXT("")),
                    17,Main);
        }
        ActionAt(TEXT("CLOSE"),TEXT("gacha-overlay-close"),
                 629.f,256.f,125.f,53.f,true,19);
        return;
    }
    if (!bGachaHistoryExpanded) return;
    // This panel is the existing canonical acquisition history projection.
    // It never fabricates banner records when there are none.
    UBorder* HistorySurface=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),TEXT("ActualCanonicalGachaHistoryPanel"));
    HistorySurface->SetBrushColor(FLinearColor(.015f,.012f,.022f,.98f));
    HistorySurface->SetPadding(FMargin(15.f));
    UCanvasPanelSlot* HistorySlot =
        Gate10PortraitGachaCanvas->AddChildToCanvas(HistorySurface);
    HistorySlot->SetAnchors(FAnchors(
        118.f/864.f,300.f/1536.f,748.f/864.f,890.f/1536.f));
    HistorySlot->SetOffsets(FMargin(0.f));
    UScrollBox* Scroll=WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass());
    HistorySurface->AddChild(Scroll);
    UVerticalBox* Rows=WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass());
    Scroll->AddChild(Rows);
    AddMessage(Rows,TEXT("ACQUISITION HISTORY"),30,Main);
    FString Error;
    if (!Session->RefreshGachaHistory(Error))
        AddMessage(Rows,Error.IsEmpty()
            ? TEXT("Canonical history is unavailable.") : Error,19,GachaMuted);
    else if (Session->GachaHistoryProjection.IsEmpty())
        AddMessage(Rows,TEXT("No recorded acquisitions."),21,GachaMuted);
    else
        for (const FOGGachaHistoryEntryViewModel& Entry :
             Session->GachaHistoryProjection)
            AddMessage(Rows,FString::Printf(TEXT("%s Â· %s Â· tick %lld"),
                *Entry.IdentityId.ToString(),*Entry.Rarity.ToString(),
                Entry.WorldTick),17,Main);
    ActionAt(TEXT("CLOSE"),TEXT("gacha-history-toggle"),
        626.f,304.f,120.f,48.f,true,19);
}

void UOGGate12NativeRulerWidget::BuildGacha()
{
    if (auto* BannerHud=GetOwningPlayer()?Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()):nullptr)
    { FString Error; BannerHud->LoadInstalledGachaBannerForRuler(Error); }
    BuildGachaPortrait();
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
    AddAction(Options, TEXT("RATES"), TEXT("gacha-rates"), true);
    AddAction(Options, bGachaHistoryExpanded ? TEXT("CLOSE HISTORY")
        : TEXT("HISTORY"), TEXT("gacha-history-toggle"));

    UBorder* BannerEdge=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("GachaShowcaseAngularOuterRail"));
    BannerEdge->SetBrushColor(FLinearColor(.34f,.32f,.39f,1.f));
    BannerEdge->SetPadding(FMargin(1.f));
    UBorder* BannerFace=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("GachaShowcaseBlackStage"));
    BannerFace->SetBrushColor(FLinearColor(.004f,.004f,.008f,1.f));
    BannerFace->SetPadding(FMargin(10.f));
    UOverlay* BannerLayers=WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(),TEXT("GachaOrnamentAndBannerStage"));
    BannerLayers->AddChildToOverlay(BannerFace);
    UOGGate12GothicChromeWidget* BannerEtch=
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass());
    BannerEtch->Configure(FLinearColor(.56f,.53f,.63f,1.f),INDEX_NONE,false);
    BannerLayers->AddChildToOverlay(BannerEtch);
    BannerEdge->AddChild(BannerLayers);
    USizeBox* BannerHeight=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("GachaTallPortraitShowcase"));
    BannerHeight->SetMinDesiredHeight(770.f);
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
    UVerticalBox* BannerSelection=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    BannerSelection->AddChildToVerticalBox(Selector);
    auto* SelectionHud=GetOwningPlayer()?Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()):nullptr;
    TArray<FOGGachaBannerDefinition> AvailableBanners; FString SelectionError;
    if (SelectionHud) SelectionHud->ListInstalledGachaBanners(AvailableBanners,SelectionError);
    AddAction(BannerSelection,TEXT("PREVIOUS BANNER"),TEXT("gacha-banner-previous"),AvailableBanners.Num()>1);
    AddAction(BannerSelection,TEXT("NEXT BANNER"),TEXT("gacha-banner-next"),AvailableBanners.Num()>1);
    SelectorFace->AddChild(BannerSelection);
    Content->AddChildToVerticalBox(SelectorEdge)
        ->SetPadding(FMargin(7.f,3.f,7.f,11.f));

    UHorizontalBox* DrawActions=WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("GachaTwoNativeDrawActions"));
    Content->AddChildToVerticalBox(DrawActions)->SetPadding(FMargin(3.f,2.f,3.f,4.f));
    // No native pull UI until the Gate07-approved banner, payment and
    // transactional pull binding are all present. Disabled controls make the
    // frozen structure visible without granting fictional acquisitions.
    const AOGWorldPresentationHud* Hud=GetOwningPlayer()
        ? Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()) : nullptr;
    const bool bCanDraw=Hud && Hud->HasActiveGachaPresentation() &&
        Session->bRealGachaAccessUnlocked;
    for (int32 I=0;I<2;++I)
    {
        UVerticalBox* Wrapper=WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        DrawActions->AddChildToHorizontalBox(Wrapper)
            ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        AddAction(Wrapper, I==0 ? TEXT("DRAW  Ã—1") : TEXT("DRAW  Ã—10"),
            I==0 ? TEXT("gacha-draw-1") : TEXT("gacha-draw-10"), bCanDraw);
    }
    UHorizontalBox* BottomUtilities=WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("GachaDetailsAndExchange"));
    Content->AddChildToVerticalBox(BottomUtilities);
    UVerticalBox* Details=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UVerticalBox* Exchange=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    BottomUtilities->AddChildToHorizontalBox(Details)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BottomUtilities->AddChildToHorizontalBox(Exchange)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    AddAction(Details,TEXT("DETAILS"),TEXT("gacha-rates"),bCanDraw);
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
    AddMessage(Content, TEXT("KNOWN EXPLORATION"), 20, Gate12RulerGold);
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
    MapFrame->SetBrushColor(FLinearColor(.008f,.007f,.014f,1.f));
    MapFrame->SetPadding(FMargin(1.f));
    UOverlay* MapLayers=WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(),TEXT("TerritoryRealMapAndOrnament"));
    // A UserWidget containing only an unbounded Border can otherwise have
    // zero desired size. Allocate real paint geometry for the graticule and
    // accurate known exploration trail; never fake terrain/map discoveries.
    USizeBox* MapPaintBounds=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),TEXT("TerritoryRealMapSlatePaintBounds"));
    MapPaintBounds->SetMinDesiredWidth(380.f);
    MapPaintBounds->SetMinDesiredHeight(710.f);
    MapPaintBounds->AddChild(TerritoryMapWidget);
    UOverlaySlot* MapFill=MapLayers->AddChildToOverlay(MapPaintBounds);
    MapFill->SetHorizontalAlignment(HAlign_Fill);
    MapFill->SetVerticalAlignment(VAlign_Fill);
    UOGGate12GothicChromeWidget* MapEtch=
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass());
    MapEtch->Configure(FLinearColor(.58f,.54f,.61f,1.f),INDEX_NONE,false);
    MapEtch->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* MapEtchFill=MapLayers->AddChildToOverlay(MapEtch);
    MapEtchFill->SetHorizontalAlignment(HAlign_Fill);
    MapEtchFill->SetVerticalAlignment(VAlign_Fill);
    if (!TerritoryMapWidget->HasKnownTrail())
    {
        UTextBlock* Unexplored=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),TEXT("RealUnknownGeographyState"));
        Unexplored->SetText(FText::FromString(TEXT("UNDISCOVERED WORLD")));
        Unexplored->SetJustification(ETextJustify::Center);
        Unexplored->SetColorAndOpacity(FSlateColor(
            FLinearColor(.57f,.55f,.63f,.89f)));
        Unexplored->SetFont(Gate10Type(29,true,true));
        Unexplored->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* Empty=MapLayers->AddChildToOverlay(Unexplored);
        Empty->SetHorizontalAlignment(HAlign_Center);
        Empty->SetVerticalAlignment(VAlign_Center);
    }
    MapFrame->AddChild(MapLayers);
    USizeBox* MapHeight = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),TEXT("TerritoryMapViewportHeight"));
    MapHeight->SetMinDesiredHeight(710.f);
    MapHeight->AddChild(MapFrame);
    Content->AddChildToVerticalBox(MapHeight)->SetPadding(FMargin(3.f,4.f,3.f,5.f));
    UHorizontalBox* MapTools=WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(),TEXT("FrozenTerritoryMapTools"));
    Content->AddChildToVerticalBox(MapTools);
    for (const TPair<FString,FString>& Tool :
        {TPair<FString,FString>(TEXT("+"),TEXT("map/zoom-in")),
         TPair<FString,FString>(TEXT("-"),TEXT("map/zoom-out")),
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
            TEXT("No territories are owned in this world."),
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
    UOGNarrativeSessionSubsystem* Narrative = GI ? GI->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    if (bNarrativeJournalOpen || (Narrative && Narrative->HasOpenSubject()))
    {
        BuildNarrativeJournal();
        return;
    }
    AddAction(Content, TEXT("QUEST JOURNAL"), TEXT("narrative/open"));

    // Frozen Gate10 Records: a 2x2 native UMG section board with independent
    // real buttons and a canonical detail panel. The illustrated mockup
    // content is not carried over. Exact black frame/panel geometry is live.
    static const TCHAR* Titles[4] = {
        TEXT("REPORTS"), TEXT("CHRONICLE"), TEXT("CODEX"), TEXT("INTELLIGENCE")
    };
    static const TCHAR* IconMarks[4] = {
        TEXT("âœ§"), TEXT("â™œ"), TEXT("â—‡"), TEXT("â—ˆ")
    };
    static const TCHAR* Subtitles[4] = {
        TEXT("AUTHORITATIVE REPORTS"), TEXT("KNOWN HISTORY"),
        TEXT("DISCOVERED KNOWLEDGE"), TEXT("KNOWN INTELLIGENCE")
    };
    // The common approved Gate10 page chapter header is already painted
    // by BuildCurrentPage. Do not duplicate a giant RECORDS title above
    // the four native sections and push meaningful content offscreen.
    UUniformGridPanel* Board = WidgetTree->ConstructWidget<UUniformGridPanel>(
        UUniformGridPanel::StaticClass(), TEXT("FrozenRecordsFourSectionBoard"));
    Board->SetSlotPadding(FMargin(7.f,9.f));
    // The four cards are a navigation HUB, not persistent tabs above a
    // miniature detail panel. Each selected section owns the whole page.
    if (RecordsSection < 0)
    {
        Content->AddChildToVerticalBox(Board);
    for (int32 Index=0; Index<4; ++Index)
    {
        const bool bActive = false;
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        // Preserve the Gacha's restrained obsidian + etched metal instead of
        // four opaque grey placeholders that obscured the approved layout.
        Frame->SetBrushColor(bActive
            ? FLinearColor(.003f,.002f,.006f,1.f)
            : FLinearColor(.001f,.001f,.002f,1.f));
        Frame->SetPadding(FMargin(1.f));
        Frame->SetHorizontalAlignment(HAlign_Fill);
        Frame->SetVerticalAlignment(VAlign_Fill);
        UOGGate12RulerActionButton* Button =
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        Button->RulerWidget = this;
        Button->ActionId = FString::Printf(TEXT("records/%d"), Index);
        Button->SetBackgroundColor(FLinearColor(0.f,0.f,0.f,0.f));
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
        Crest->SetFont(Gate10Type(53,false,true));
        UVerticalBoxSlot* CrestSlot=Face->AddChildToVerticalBox(Crest);
        CrestSlot->SetPadding(FMargin(0.f,10.f,0.f,2.f));
        CrestSlot->SetHorizontalAlignment(HAlign_Fill);
        UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Title->SetText(FText::FromString(Titles[Index]));
        Title->SetJustification(ETextJustify::Center);
        Title->SetColorAndOpacity(FSlateColor(TextMain));
        Title->SetFont(Gate10Type(
            Index==3 ? 26 : 31,true,true));
        UVerticalBoxSlot* TitleSlot=Face->AddChildToVerticalBox(Title);
        TitleSlot->SetPadding(FMargin(0.f,4.f));
        TitleSlot->SetHorizontalAlignment(HAlign_Fill);
        UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Sub->SetText(FText::FromString(Subtitles[Index]));
        Sub->SetJustification(ETextJustify::Center);
        Sub->SetAutoWrapText(true);
        Sub->SetWrapTextAt(212.f);
        Sub->SetColorAndOpacity(FSlateColor(TextMuted));
        Sub->SetFont(Gate10Type(18,false,true));
        UVerticalBoxSlot* SubtitleSlot=Face->AddChildToVerticalBox(Sub);
        SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
        SubtitleSlot->SetPadding(FMargin(6.f,4.f,6.f,6.f));
        UTextBlock* RealCount=WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        FString RealSummary;
        if (Index==0)
            RealSummary=FString::Printf(TEXT("%d VERIFIED"),
                Session->RecordsProjection.ReportCount);
        else if (Index==1)
            RealSummary=FString::Printf(TEXT("%d KNOWN"),
                Session->ChronicleProjection.Num());
        else if (Index==2)
            RealSummary=TEXT("NO VERIFIED CODEX PACKAGE");
        else
            RealSummary=FString::Printf(TEXT("%d KNOWN"),
                Session->IntelligenceProjection.Num());
        RealCount->SetText(FText::FromString(RealSummary));
        RealCount->SetJustification(ETextJustify::Center);
        RealCount->SetAutoWrapText(true);
        RealCount->SetWrapTextAt(212.f);
        RealCount->SetColorAndOpacity(FSlateColor(
            FLinearColor(.68f,.66f,.74f,1.f)));
        RealCount->SetFont(Gate10Type(22,false,true));
        UVerticalBoxSlot* CountSlot=Face->AddChildToVerticalBox(RealCount);
        CountSlot->SetHorizontalAlignment(HAlign_Fill);
        CountSlot->SetPadding(FMargin(6.f,12.f,6.f,9.f));
        // Real native text stays interactive, while the Gacha-style black
        // panel prevents the default UButton Slate grey from filling the
        // frozen Records card. The etching must paint ABOVE the hit surface.
        UBorder* BlackCard=WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        BlackCard->SetBrushColor(bActive
            ? FLinearColor(.005f,.003f,.008f,1.f)
            : FLinearColor(.0015f,.0013f,.003f,1.f));
        BlackCard->SetVerticalAlignment(VAlign_Center);
        BlackCard->SetHorizontalAlignment(HAlign_Fill);
        BlackCard->SetPadding(FMargin(6.f,10.f,6.f,10.f));
        BlackCard->AddChild(Face);
        CenterNativeButton(Button,BlackCard);
        UOverlay* OrnamentedRecord=WidgetTree->ConstructWidget<UOverlay>(
            UOverlay::StaticClass());
        UOGGate12GothicChromeWidget* Etching=
            WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
                UOGGate12GothicChromeWidget::StaticClass());
        Etching->Configure(bActive
            ? FLinearColor(.68f,.65f,.74f,1.f)
            : FLinearColor(.43f,.40f,.48f,1.f),INDEX_NONE,bActive);
        UOverlaySlot* RecordButtonFill=
            OrnamentedRecord->AddChildToOverlay(Button);
        RecordButtonFill->SetHorizontalAlignment(HAlign_Fill);
        RecordButtonFill->SetVerticalAlignment(VAlign_Fill);
        // Decoration is render-only. Put above the button so the native
        // Slate button brushes cannot occlude the shared gothic filigree.
        Etching->SetVisibility(ESlateVisibility::HitTestInvisible);
        UOverlaySlot* RecordEtchingFill=
            OrnamentedRecord->AddChildToOverlay(Etching);
        RecordEtchingFill->SetHorizontalAlignment(HAlign_Fill);
        RecordEtchingFill->SetVerticalAlignment(VAlign_Fill);
        Frame->AddChild(OrnamentedRecord);
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass());
        // Every Records tile MUST have identical actual geometry; minimum
        // desired height leaves mixed-length text cards unequal on screen.
        Size->SetHeightOverride(400.f);
        Size->AddChild(Frame);
        UUniformGridSlot* RecordCell=
            Board->AddChildToUniformGrid(Size, Index/2, Index%2);
        RecordCell->SetHorizontalAlignment(HAlign_Fill);
        RecordCell->SetVerticalAlignment(VAlign_Fill);
    }
        return;
    }

    AddAction(Content, TEXT("BACK TO RECORDS"), TEXT("records/back"));
    UBorder* DetailFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RecordsCanonicalDetailFrame"));
    DetailFrame->SetBrushColor(FLinearColor(.002f,.0015f,.004f,1.f));
    DetailFrame->SetPadding(FMargin(1.f));
    UBorder* DetailDark = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("RecordsCanonicalDetailBlack"));
    DetailDark->SetBrushColor(FLinearColor(.0012f,.0010f,.002f,1.f));
    DetailDark->SetVerticalAlignment(VAlign_Center);
    DetailDark->SetPadding(FMargin(8.f,10.f));
    UOverlay* DetailLayers=WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(),TEXT("RecordsCanonicalDetailGothicLayers"));
    DetailLayers->AddChildToOverlay(DetailDark);
    UOGGate12GothicChromeWidget* DetailRails=
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass());
    DetailRails->Configure(FLinearColor(.56f,.54f,.65f,1.f),
        INDEX_NONE,false);
    DetailRails->SetVisibility(ESlateVisibility::HitTestInvisible);
    DetailLayers->AddChildToOverlay(DetailRails);
    DetailFrame->AddChild(DetailLayers);
    UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("RecordsActualKnowledgeDetail"));
    DetailDark->AddChild(Detail);
    USizeBox* DetailHeight = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("RecordsGachaLanguageDetailedPanel"));
    // Long Reports/Chronicle/Intelligence histories are independent scrolling
    // pages, not clipped to the 380px footer of a four-card overview.
    DetailHeight->SetMinDesiredHeight(980.f);
    DetailHeight->AddChild(DetailFrame);
    Content->AddChildToVerticalBox(DetailHeight)
        ->SetPadding(FMargin(7.f,13.f,7.f,10.f));
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
        // Detail view remains live; roster thumbnails alone stay frozen for
        // mobile performance. Source animation/turntable can now advance.
        PreviewStage->SetLiveCapture(true);
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
    if (bThreeDimensional && IsValid(PreviewStage))
    {
        static const TCHAR* UiCutoutPath=TEXT(
            "/Game/UI/Gate10/Materials/M_Gate10_CharacterCaptureAlphaUI."
            "M_Gate10_CharacterCaptureAlphaUI");
        if (UMaterialInterface* Mat=LoadObject<UMaterialInterface>(
            nullptr,UiCutoutPath))
        {
            LivePreviewCutoutMaterial=UMaterialInstanceDynamic::Create(Mat,this);
            LivePreviewCutoutMaterial->SetTextureParameterValue(
                TEXT("LiveScene"),PreviewStage->GetLiveRenderTarget());
            Image->SetBrushFromMaterial(LivePreviewCutoutMaterial);
        }
    }
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->AddChild(Image);
    USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    // Character visuals belong at near-full-page scale in their own
    // detail card, not a 380-unit rotating side panel.
    Height->SetMinDesiredWidth(780.f);
    Height->SetMinDesiredHeight(1040.f);
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
    AddAction(Content, TEXT("INTERACTIONS"), TEXT("interaction/open"));
    AddMessage(Content,
        Identity.DisplayNameKey.IsEmpty()
            ? Identity.IdentityId.ToString() : Identity.DisplayNameKey,
        24, TextMain);
    AddMessage(Content,
        FString::Printf(TEXT("%d separately owned Manifestations"),
            Identity.ManifestationCount), 17, TextMuted);
    // The 2D/3D switch is PART OF this owned character's visual card.
    // It never appears as a free-floating Characters-page control.
    UBorder* PortraitSurface=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),TEXT("OwnedCharacterFullscreenVisualCard"));
    PortraitSurface->SetBrushColor(FLinearColor(.005f,.004f,.011f,1.f));
    PortraitSurface->SetPadding(FMargin(8.f,10.f));
    UVerticalBox* VisualCardContents=WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(),TEXT("OwnedCharacterVisualAndLocalControls"));
    PortraitSurface->AddChild(VisualCardContents);
    AddCharacterVisual(VisualCardContents, Identity,
        bPrefer3DCharacterPresentation);
    AddAction(VisualCardContents,
        bPrefer3DCharacterPresentation
            ? TEXT("3D VIEW  /  SELECT 2D")
            : TEXT("2D VIEW  /  SELECT 3D"),
        TEXT("presentation/toggle"));
    UVerticalBoxSlot* VisualPanel=Content->AddChildToVerticalBox(PortraitSurface);
    VisualPanel->SetPadding(FMargin(4.f,6.f,4.f,12.f));
    VisualPanel->SetHorizontalAlignment(HAlign_Fill);
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
    BuildCharacterDevelopment(Identity, Selected);
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
        ResetInventoryUi();
        ResetCharacterDevelopment();
        LastUiError = TEXT("Canonical Ruler service not loaded.");
        BuildCurrentPage();
        return;
    }

    if (Action.StartsWith(TEXT("narrative/")))
    {
        ClearDevelopmentReview();
        ResetInventoryUi();
        HandleNarrativeAction(Action);
        return;
    }
    ResetNarrativePage();
    if (UOGNarrativeSessionSubsystem* Narrative = GI->GetSubsystem<UOGNarrativeSessionSubsystem>()) Narrative->CloseSubject();
    if (Action.StartsWith(TEXT("inventory/")))
    {
        ClearDevelopmentReview();
        DevelopmentFeedback.Reset();
        DevelopmentRefreshWarning.Reset();
        HandleInventoryAction(Action, *Session);
        return;
    }
    ResetInventoryUi();
    if (Action.StartsWith(TEXT("development/")))
    {
        HandleDevelopmentAction(Action, *Session);
        return;
    }
    // Includes all navigation, copy selection, wardrobe and world transitions.
    ClearDevelopmentReview();
    DevelopmentFeedback.Reset();
    DevelopmentRefreshWarning.Reset();
    bDevelopmentFeedbackError = false;

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
    else if (Action == TEXT("select-ruler"))
    {
        if (Session->SelectRulerForWorld(Error))
        {
            OpenIdentityIndex = INDEX_NONE;
            bPrefer3DCharacterPresentation = false;
            ReleasePreviewStage();
        }
    }
    else if (Action == TEXT("roster/sort"))
    {
        RosterSortMode = (RosterSortMode + 1) % 3;
    }
    else if (Action == TEXT("gacha-banner-next") || Action == TEXT("gacha-banner-previous"))
    {
        auto* Hud=GetOwningPlayer()?Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()):nullptr;
        if (!Hud) Error=TEXT("Native acquisition presentation is unavailable.");
        else Hud->StepInstalledGachaBanner(Action==TEXT("gacha-banner-next")?1:-1,Error);
        bGachaRatesExpanded=bGachaResultsExpanded=bGachaHistoryExpanded=false;
    }
    else if (Action == TEXT("gacha-history-toggle"))
    {
        bGachaRatesExpanded = false;
        bGachaResultsExpanded = false;
        bGachaHistoryExpanded = !bGachaHistoryExpanded;
    }
    else if (Action == TEXT("gacha-rates"))
    {
        bGachaHistoryExpanded = false;
        bGachaResultsExpanded = false;
        bGachaRatesExpanded = !bGachaRatesExpanded;
    }
    else if (Action == TEXT("gacha-overlay-close"))
    {
        bGachaRatesExpanded = false;
        bGachaResultsExpanded = false;
        bGachaHistoryExpanded = false;
    }
    else if (Action == TEXT("gacha-draw-1") ||
             Action == TEXT("gacha-draw-10"))
    {
        AOGWorldPresentationHud* Hud=GetOwningPlayer()
            ? Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()) : nullptr;
        if (!Hud || !Session->bRealGachaAccessUnlocked ||
            !Hud->HasActiveGachaPresentation())
            Error=TEXT("No eligible installed banner or unlocked Ruler.");
        else if (Hud->ExecuteActiveGachaDraw(
                     Action == TEXT("gacha-draw-10")?10:1,Error))
        {
            bGachaRatesExpanded = false;
            bGachaHistoryExpanded = false;
            bGachaResultsExpanded = true;
        }
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
    else if (Action == TEXT("records/back"))
    {
        RecordsSection = -1;
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
        RecordsSection = -1;
        bGachaRatesExpanded = false;
        bGachaHistoryExpanded = false;
        bGachaResultsExpanded = false;
        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);
        if (Error.IsEmpty() && Index == 2 && GetOwningPlayer())
        {
            if (AOGWorldPresentationHud* Hud =
                Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()))
            {
                FString BannerError;
                Hud->LoadInstalledGachaBannerForRuler(BannerError);
                // No catalog is the legitimate empty state, not a reason to
                // block Ruler navigation or invent a featured banner.
            }
        }
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
    else if (Action == TEXT("interaction/open"))
    {
        auto* Hud = GetOwningPlayer() ? Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()) : nullptr;
        if (Hud && Hud->ShowNativeInteractionSurface(Error)) return;
        if (!Hud) Error = TEXT("World interaction presentation is unavailable.");
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
