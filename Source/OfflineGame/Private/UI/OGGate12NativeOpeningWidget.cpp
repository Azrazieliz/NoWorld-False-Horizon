#include "UI/OGGate12NativeOpeningWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
const FLinearColor GothicVoid(.006f,.006f,.011f,1.0f);
const FLinearColor DarkSlate(.019f,.018f,.026f,.98f);
const FLinearColor OrnamentGold(.71f,.56f,.33f,1.0f);
const FLinearColor OpeningTextColor(.91f,.89f,.85f,1.0f);
const FLinearColor Muted(.51f,.50f,.53f,1.0f);
}

void UOGGate12OpeningActionButton::InvokeOpeningAction()
{
    if (Opening) Opening->DispatchNativeAction(Action);
}

void UOGGate12NativeOpeningWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildRoot();
}

void UOGGate12NativeOpeningWidget::BuildRoot()
{
    if (!WidgetTree || WidgetTree->RootWidget) return;
    USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>(
        USafeZone::StaticClass(), TEXT("OpeningSafeArea"));
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("OpeningNativeCanvas"));
    if (!Safe || !Canvas) return;
    WidgetTree->RootWidget = Safe;
    Safe->AddChild(Canvas);

    // The frozen concept PNG is NEVER drawn as an interactive UI. This
    // authored native layer is intentionally incomplete until a genuine
    // Gate02/Gate09 gothic scene and the approved character idle are bound.
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("OpeningSceneBackdrop"));
    Backdrop->SetBrushColor(GothicVoid);
    UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));
    BackdropSlot->SetOffsets(FMargin(0.f,0.f,0.f,0.f));

    // Gate10's approved opening is an ANIMATED interface, not the old ten
    // exploratory images. Author this real native UMG scene and its independent
    // moving atmosphere without inserting illustrative mockup art.
    UCanvasPanel* Stage = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("OpeningAnimatedSceneStage"));
    Stage->SetVisibility(ESlateVisibility::HitTestInvisible);
    StageSlot = Canvas->AddChildToCanvas(Stage);
    StageSlot->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));
    StageSlot->SetOffsets(FMargin(0.f));
    auto Layer = [this, Stage](const TCHAR* Name, FAnchors Anchors,
                                FLinearColor Tint) -> UCanvasPanelSlot*
    {
        UBorder* Shape = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(Name));
        Shape->SetVisibility(ESlateVisibility::HitTestInvisible);
        Shape->SetBrushColor(Tint);
        UCanvasPanelSlot* Slot = Stage->AddChildToCanvas(Shape);
        Slot->SetAnchors(Anchors);
        Slot->SetOffsets(FMargin(0.f));
        return Slot;
    };
    // Architectural throne silhouette; visual scene is generated from editable
    // UMG geometry so neither screenshot text nor characters are embedded.
    Layer(TEXT("DistantLeftVault"), FAnchors(.05f,.04f,.12f,.83f),
          FLinearColor(.028f,.026f,.035f,.92f));
    Layer(TEXT("DistantRightVault"), FAnchors(.88f,.04f,.95f,.83f),
          FLinearColor(.028f,.026f,.035f,.92f));
    Layer(TEXT("LeftVaultEtching"), FAnchors(.118f,.04f,.120f,.75f),
          FLinearColor(.24f,.22f,.25f,.29f));
    Layer(TEXT("RightVaultEtching"), FAnchors(.880f,.04f,.882f,.75f),
          FLinearColor(.24f,.22f,.25f,.29f));
    Layer(TEXT("DeepThroneBack"), FAnchors(.414f,.20f,.586f,.67f),
          FLinearColor(.017f,.016f,.023f,.98f));
    Layer(TEXT("ThroneRimLeft"), FAnchors(.411f,.20f,.415f,.66f),
          FLinearColor(.31f,.29f,.33f,.48f));
    Layer(TEXT("ThroneRimRight"), FAnchors(.585f,.20f,.589f,.66f),
          FLinearColor(.31f,.29f,.33f,.48f));
    Layer(TEXT("VacantThroneSeat"), FAnchors(.393f,.56f,.607f,.63f),
          FLinearColor(.012f,.012f,.017f,1.f));
    Layer(TEXT("ThroneArmLeft"), FAnchors(.375f,.49f,.411f,.67f),
          FLinearColor(.027f,.026f,.035f,1.f));
    Layer(TEXT("ThroneArmRight"), FAnchors(.589f,.49f,.625f,.67f),
          FLinearColor(.027f,.026f,.035f,1.f));
    Layer(TEXT("CathedralFloorRim"), FAnchors(.10f,.81f,.90f,.814f),
          FLinearColor(.29f,.28f,.34f,.34f));
    // Animated fog-light volume, authored separately from the throne shape.
    ThroneSilhouetteSlot = Layer(
        TEXT("MovingThroneLightColumn"), FAnchors(.49f,.15f,.51f,.67f),
        FLinearColor(.38f,.36f,.47f,.12f));
    AmbientHalo = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("AnimatedThroneHalo"));
    AmbientHalo->SetVisibility(ESlateVisibility::HitTestInvisible);
    AmbientHalo->SetBrushColor(FLinearColor(.13f,.12f,.17f,.18f));
    UCanvasPanelSlot* HaloSlot = Stage->AddChildToCanvas(AmbientHalo);
    HaloSlot->SetAnchors(FAnchors(.35f,.22f,.65f,.52f));
    HaloSlot->SetOffsets(FMargin(0.f));
    // Native particles animate deterministically with reduced-motion support.
    // They are ambient UI effects, NEVER six-tier rarity frame VFX.
    for (int32 Index = 0; Index < 13; ++Index)
    {
        UBorder* Ember = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass());
        Ember->SetVisibility(ESlateVisibility::HitTestInvisible);
        Ember->SetBrushColor(FLinearColor(.64f,.61f,.73f,.30f));
        UCanvasPanelSlot* EmberSlot = Stage->AddChildToCanvas(Ember);
        EmberSlot->SetAnchors(FAnchors(.15f + .056f * Index,
            .40f + .025f * (Index % 4)));
        EmberSlot->SetAlignment(FVector2D(.5f,.5f));
        EmberSlot->SetSize(FVector2D(Index % 3 == 0 ? 3.f : 2.f,
                                     Index % 2 == 0 ? 8.f : 5.f));
        EmberSlots.Add(EmberSlot);
        Embers.Add(Ember);
    }

    // Accent is an independent material-ready layer, not text burned into art.
    AtmosphericAccent = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("ThroneAtmosphereGoldAccent"));
    AtmosphericAccent->SetBrushColor(FLinearColor(.29f,.22f,.13f,.18f));
    UCanvasPanelSlot* AccentSlot = Canvas->AddChildToCanvas(AtmosphericAccent);
    AccentSlot->SetAnchors(FAnchors(.48f,.08f,.52f,.63f));
    AccentSlot->SetOffsets(FMargin(0.f,0.f,0.f,0.f));

    // The frozen Gate10 04 composition separates the central title from the
    // lower interactive controls. Keep actual game title editable UMG text;
    // the approved reference screenshot is never baked into the UI layer.
    UTextBlock* NativeTitle = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("ActualGameTitle"));
    NativeTitle->SetText(FText::FromString(TEXT("NoWorld: False Horizon")));
    NativeTitle->SetJustification(ETextJustify::Center);
    NativeTitle->SetAutoWrapText(true);
    NativeTitle->SetColorAndOpacity(FSlateColor(OrnamentGold));
    FSlateFontInfo TitleFont = NativeTitle->GetFont();
    TitleFont.Size = 32;
    NativeTitle->SetFont(TitleFont);
    TitleWidthConstraint = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("OpeningTitleWidthConstraint"));
    TitleWidthConstraint->SetWidthOverride(320.f);
    TitleWidthConstraint->AddChild(NativeTitle);
    TitleSlot = Canvas->AddChildToCanvas(TitleWidthConstraint);
    TitleSlot->SetAutoSize(true);
    TitleSlot->SetAnchors(FAnchors(.5f,.60f));
    TitleSlot->SetAlignment(FVector2D(.5f,.5f));

    UBorder* MenuFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("OpeningControlsFrame"));
    MenuFrame->SetBrushColor(DarkSlate);
    MenuFrame->SetPadding(FMargin(14.f,14.f,14.f,16.f));
    Menu = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("OpeningRealButtons"));
    MenuFrame->AddChild(Menu);
    // Constrain actual Slate desired size before Canvas auto-layout. Long
    // recovery/clear-confirmation captions otherwise push real buttons past
    // portrait edges, or off the right side of the landscape safe area.
    MenuWidthConstraint = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("OpeningViewportWidthConstraint"));
    MenuWidthConstraint->AddChild(MenuFrame);
    MenuWidthConstraint->SetWidthOverride(320.f);
    MenuSlot = Canvas->AddChildToCanvas(MenuWidthConstraint);
    MenuSlot->SetAutoSize(true);
    MenuSlot->SetAlignment(FVector2D(.5f,1.0f));
    MenuSlot->SetAnchors(FAnchors(.5f,.94f));
    MenuSlot->SetPosition(FVector2D(0.f,0.f));

    RebuildMenu();
}

void UOGGate12NativeOpeningWidget::RefreshOpening(
    bool bExistingOwnedRuler, bool bHasWorldSave,
    bool bRecoverable, bool bImportAvailable,
    bool bInReducedMotion, FName CurrentPage)
{
    const bool bChanged =
        bHasRuler != bExistingOwnedRuler || bHasSave != bHasWorldSave ||
        bHasRecovery != bRecoverable || bHasImport != bImportAvailable ||
        bReducedMotion != bInReducedMotion || Page != CurrentPage;
    bHasRuler = bExistingOwnedRuler;
    bHasSave = bHasWorldSave;
    bHasRecovery = bRecoverable;
    bHasImport = bImportAvailable;
    bReducedMotion = bInReducedMotion;
    Page = CurrentPage;
    if (bChanged) RebuildMenu();
}

void UOGGate12NativeOpeningWidget::AddAction(
    const FString& Caption, FName Action, bool bEnabled, bool bPrimary)
{
    if (!WidgetTree || !Menu) return;
    UOGGate12OpeningActionButton* Button =
        WidgetTree->ConstructWidget<UOGGate12OpeningActionButton>(
            UOGGate12OpeningActionButton::StaticClass());
    if (!Button) return;
    Button->Opening = this;
    Button->Action = Action;
    Button->SetIsEnabled(bEnabled);
    Button->SetBackgroundColor(!bEnabled
        ? FLinearColor(.028f,.029f,.031f,1.f)
        : bPrimary ? FLinearColor(.16f,.115f,.065f,.98f)
                   : FLinearColor(.036f,.033f,.039f,.98f));
    Button->OnClicked.AddDynamic(Button,
        &UOGGate12OpeningActionButton::InvokeOpeningAction);
    UTextBlock* TextLabel =
        WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TextLabel->SetText(FText::FromString(Caption));
    TextLabel->SetJustification(ETextJustify::Center);
    TextLabel->SetAutoWrapText(true);
    TextLabel->SetColorAndOpacity(FSlateColor(
        bEnabled ? (bPrimary ? OrnamentGold : OpeningTextColor) : Muted));
    FSlateFontInfo Font = TextLabel->GetFont();
    Font.Size = bPrimary ? 23 : 18;
    TextLabel->SetFont(Font);
    Button->AddChild(TextLabel);
    UVerticalBoxSlot* AddedButtonSlot = Menu->AddChildToVerticalBox(Button);
    AddedButtonSlot->SetPadding(FMargin(5.f,7.f,5.f,7.f));
}

void UOGGate12NativeOpeningWidget::RebuildMenu()
{
    if (!Menu || !WidgetTree) return;
    Menu->ClearChildren();

    if (Page == TEXT("confirm_clear"))
    {
        UTextBlock* Notice = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("WorldClearWarning"));
        Notice->SetText(FText::FromString(
            TEXT("Start a new world? Your existing world is replaced only AFTER a recovery copy is preserved.")));
        Notice->SetAutoWrapText(true);
        Notice->SetColorAndOpacity(FSlateColor(OpeningTextColor));
        Menu->AddChildToVerticalBox(Notice)->SetPadding(FMargin(8.f,9.f));
        AddAction(TEXT("KEEP CURRENT WORLD"), TEXT("OG.Opening.ClearCancel"),true);
        AddAction(TEXT("CREATE RECOVERY COPY AND START NEW"),
                  TEXT("OG.Opening.ClearConfirm"),true,true);
        return;
    }

    // Canonical opening resume requires both a stored local Ruler and an
    // existing world save; a Ruler profile alone is not a resumable world.
    const bool bWorldFilePresent = IFileManager::Get().FileExists(
        *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("OfflineGame"),
                         TEXT("WorldState.db")));
    // A corrupt or outdated file is still user data and cannot be silently
    // discarded simply because read-only validation rejected Continue.
    if (bWorldFilePresent && !bHasSave)
    {
        UTextBlock* Notice = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("UnverifiedWorldSaveNotice"));
        Notice->SetText(FText::FromString(
            TEXT("Existing world data could not be verified. Continue is unavailable; preserve or recover it before starting a replacement.")));
        Notice->SetAutoWrapText(true);
        Notice->SetColorAndOpacity(FSlateColor(OpeningTextColor));
        Menu->AddChildToVerticalBox(Notice)->SetPadding(FMargin(7.f,9.f));
    }
    AddAction(TEXT("CONTINUE"), TEXT("OG.Opening.Continue"),
              bHasRuler && bHasSave, true);
    AddAction(TEXT("NEW GAME"), TEXT("new-game"), true);
    AddAction(TEXT("SETTINGS"), TEXT("OG.Settings.Toggle"), true);
    AddAction(TEXT("DATA RECOVERY"), TEXT("OG.Opening.Recover"),
              bHasRecovery || bHasImport || bHasSave || bWorldFilePresent);
}

void UOGGate12NativeOpeningWidget::DispatchNativeAction(FName Action)
{
    if (Action == TEXT("new-game"))
    {
        // Existing worlds are NEVER silently cleared. The only destructive
        // path delegates to the already-tested real Core backup+confirm flow.
        // An orphaned world database also requires the confirmed backup path;
        // absence of a local Ruler must not silently bypass recovery safety.
        const bool bWorldFilePresent = IFileManager::Get().FileExists(
            *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("OfflineGame"),
                             TEXT("WorldState.db")));
        Action = (bHasRuler || bHasSave || bWorldFilePresent)
            ? TEXT("OG.Opening.Clear") : TEXT("OG.Opening.Ruler");
    }
    OnOpeningAction.Broadcast(Action);
}

void UOGGate12NativeOpeningWidget::NativeTick(
    const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    const FVector2D Size = MyGeometry.GetLocalSize();
    if (MenuSlot && Size.X > 0 && Size.Y > 0)
    {
        // Separate portrait and landscape placement, same canonical actions.
        const bool bWide = Size.X > Size.Y;
        // SafeZone dimensions are authoritative here; neither a hard-coded
        // screen size nor a frozen reference screenshot drives touch layout.
        if (MenuWidthConstraint)
        {
            const float DesiredWidth = FMath::Min(
                Size.X - 24.f, bWide ? Size.X * .39f : Size.X * .90f);
            MenuWidthConstraint->SetWidthOverride(FMath::Max(150.f, DesiredWidth));
        }
        MenuSlot->SetAnchors(
            bWide ? FAnchors(.77f,.83f) : FAnchors(.5f,.94f));
        MenuSlot->SetAlignment(FVector2D(.5f,1.f));
        if (TitleSlot && TitleWidthConstraint)
        {
            const float Width = FMath::Max(150.f, FMath::Min(
                Size.X - 24.f, bWide ? Size.X * .40f : Size.X * .92f));
            TitleWidthConstraint->SetWidthOverride(Width);
            TitleSlot->SetAnchors(
                bWide ? FAnchors(.76f,.27f) : FAnchors(.5f,.57f));
        }
    }
    // Continuous native ambient animation; no baked PNG or cinematic.
    // Reduced-motion preferences freeze all moving layers.
    const bool bWideStage = Size.X > Size.Y;
    if (StageSlot && Size.X > 0.f && Size.Y > 0.f &&
        (!bStageLayoutInitialized || bStageWasWide != bWideStage))
    {
        bStageWasWide = bWideStage;
        bStageLayoutInitialized = true;
        StageSlot->SetAnchors(bWideStage
            ? FAnchors(0.f,0.f,.56f,1.f)
            : FAnchors(0.f,0.f,1.f,1.f));
        StageSlot->SetOffsets(FMargin(0.f));
    }
    const float Phase = bReducedMotion || !GetWorld()
        ? 0.f : static_cast<float>(GetWorld()->GetRealTimeSeconds());
    if (AtmosphericAccent)
    {
        const float Alpha = bReducedMotion ? .08f :
            .055f + .035f * (1.f + FMath::Sin(Phase * .43f));
        AtmosphericAccent->SetBrushColor(
            FLinearColor(.46f,.44f,.52f,Alpha));
    }
    if (AmbientHalo)
    {
        const float Glow = bReducedMotion ? .09f :
            .065f + .09f * (.5f + .5f * FMath::Sin(Phase * .31f));
        AmbientHalo->SetBrushColor(
            FLinearColor(.24f,.22f,.31f,Glow));
    }
    if (ThroneSilhouetteSlot)
    {
        const float Sway = bReducedMotion ? 0.f :
            3.f * FMath::Sin(Phase * .21f);
        ThroneSilhouetteSlot->SetPosition(FVector2D(Sway,0.f));
    }
    for (int32 Index = 0; Index < Embers.Num(); ++Index)
    {
        if (!Embers[Index]) continue;
        const float Offset = Index * 1.17f;
        const float DriftX = bReducedMotion ? 0.f :
            14.f * FMath::Sin(Phase * .27f + Offset);
        const float DriftY = bReducedMotion ? 0.f :
            -FMath::Fmod(Phase * (5.2f + .48f * Index)
                         + Index * 16.7f, 150.f);
        Embers[Index]->SetRenderTranslation(FVector2D(DriftX, DriftY));
        const float Opacity = bReducedMotion ? .09f :
            .13f + .25f * (.5f + .5f *
                           FMath::Sin(Phase * .59f + Offset));
        Embers[Index]->SetBrushColor(
            FLinearColor(.69f,.67f,.75f,Opacity));
    }
}
