#include "UI/OGGate12NativeOpeningWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

namespace
{
const FLinearColor GothicVoid(.006f,.006f,.011f,1.0f);
const FLinearColor DarkSlate(.019f,.018f,.026f,.98f);
const FLinearColor OrnamentGold(.71f,.56f,.33f,1.0f);
const FLinearColor Text(.91f,.89f,.85f,1.0f);
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

    UBorder* MenuFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("OpeningControlsFrame"));
    MenuFrame->SetBrushColor(DarkSlate);
    MenuFrame->SetPadding(FMargin(14.f,14.f,14.f,16.f));
    Menu = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("OpeningRealButtons"));
    MenuFrame->AddChild(Menu);
    MenuSlot = Canvas->AddChildToCanvas(MenuFrame);
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
        bEnabled ? (bPrimary ? OrnamentGold : Text) : Muted));
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
    UTextBlock* Title =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("ActualGameTitle"));
    Title->SetText(FText::FromString(TEXT("NoWorld: False Horizon")));
    Title->SetJustification(ETextJustify::Center);
    Title->SetColorAndOpacity(FSlateColor(OrnamentGold));
    FSlateFontInfo Font = Title->GetFont();
    Font.Size = 27;
    Title->SetFont(Font);
    Menu->AddChildToVerticalBox(Title)->SetPadding(FMargin(9.f,14.f,9.f,18.f));

    if (Page == TEXT("confirm_clear"))
    {
        UTextBlock* Notice = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(), TEXT("WorldClearWarning"));
        Notice->SetText(FText::FromString(
            TEXT("Start a new world? Your existing world is replaced only AFTER a recovery copy is preserved.")));
        Notice->SetAutoWrapText(true);
        Notice->SetColorAndOpacity(FSlateColor(Text));
        Menu->AddChildToVerticalBox(Notice)->SetPadding(FMargin(8.f,9.f));
        AddAction(TEXT("KEEP CURRENT WORLD"), TEXT("OG.Opening.ClearCancel"),true);
        AddAction(TEXT("CREATE RECOVERY COPY AND START NEW"),
                  TEXT("OG.Opening.ClearConfirm"),true,true);
        return;
    }

    AddAction(TEXT("CONTINUE"), TEXT("OG.Opening.Continue"), bHasRuler, true);
    AddAction(TEXT("NEW GAME"), TEXT("new-game"), true);
    AddAction(TEXT("SETTINGS"), TEXT("OG.Settings.Toggle"), true);
    AddAction(TEXT("DATA RECOVERY"), TEXT("OG.Opening.Recover"),
              bHasRecovery || bHasImport || bHasSave);
}

void UOGGate12NativeOpeningWidget::DispatchNativeAction(FName Action)
{
    if (Action == TEXT("new-game"))
    {
        // Existing worlds are NEVER silently cleared. The only destructive
        // path delegates to the already-tested real Core backup+confirm flow.
        Action = bHasRuler ? TEXT("OG.Opening.Clear")
                           : TEXT("OG.Opening.Ruler");
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
        MenuSlot->SetAnchors(
            bWide ? FAnchors(.81f,.72f) : FAnchors(.5f,.94f));
        MenuSlot->SetAlignment(FVector2D(.5f,1.f));
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
