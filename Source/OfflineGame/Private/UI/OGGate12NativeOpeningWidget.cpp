#include "UI/OGGate12NativeOpeningWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
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
    if (AtmosphericAccent)
    {
        const float Phase = bReducedMotion || !GetWorld()
            ? 0.f : static_cast<float>(GetWorld()->GetRealTimeSeconds());
        const float Alpha = bReducedMotion ? .15f :
            .14f + .07f * (1.f + FMath::Sin(Phase * .36f));
        AtmosphericAccent->SetBrushColor(
            FLinearColor(.34f,.25f,.15f,Alpha));
    }
}
