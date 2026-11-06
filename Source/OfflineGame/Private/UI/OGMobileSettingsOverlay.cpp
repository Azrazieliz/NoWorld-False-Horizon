#include "UI/OGMobileSettingsOverlay.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/OGSemanticAudioRuntime.h"
#include "Styling/CoreStyle.h"
#include "World/OGStartingRegionPresentation.h"

void UOGMobileSettingsOverlay::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;

    auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    SettingsButton = WidgetTree->ConstructWidget<UButton>();
    SettingsButton->SetBackgroundColor(FLinearColor(.11f, .09f, .15f, 1.f));
    auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(NSLOCTEXT("OGMobileSettings", "Open", "Settings"));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 18));
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Label->SetVisibility(ESlateVisibility::HitTestInvisible);
    SettingsButton->AddChild(Label);
    SettingsButton->OnClicked.AddDynamic(this, &ThisClass::OpenSettings);
    auto* SettingsSlot = Root->AddChildToCanvas(SettingsButton);
    SettingsSlot->SetAnchors(FAnchors(1.f, 0.f));
    SettingsSlot->SetAlignment(FVector2D(1.f, 0.f));
    SettingsSlot->SetPosition(FVector2D(-18.f, 16.f));
    SettingsSlot->SetSize(FVector2D(116.f, 48.f));
    SettingsButton->SetVisibility(ESlateVisibility::Collapsed);

    auto* CaptionArea = WidgetTree->ConstructWidget<UOverlay>();
    CaptionArea->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* CaptionSlot = Root->AddChildToCanvas(CaptionArea);
    CaptionSlot->SetAnchors(FAnchors(.20f, .55f, .80f, .82f));
    CaptionSlot->SetOffsets(FMargin(0.f));
    CaptionPanel = WidgetTree->ConstructWidget<UBorder>();
    CaptionPanel->SetPadding(FMargin(12.f, 6.f));
    CaptionPanel->SetVisibility(ESlateVisibility::Collapsed);
    auto* CaptionAlignment = CaptionArea->AddChildToOverlay(CaptionPanel);
    CaptionAlignment->SetHorizontalAlignment(HAlign_Fill);
    CaptionAlignment->SetVerticalAlignment(VAlign_Bottom);
    CaptionLine = WidgetTree->ConstructWidget<UTextBlock>();
    CaptionLine->SetAutoWrapText(true);
    CaptionLine->SetJustification(ETextJustify::Center);
    CaptionLine->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CaptionPanel->SetContent(CaptionLine);
}

void UOGMobileSettingsOverlay::UpdatePresentation(bool bShowSettings, const FOGSemanticCaptionProjection& Caption)
{
    if (!SettingsButton || !CaptionPanel || !CaptionLine) return;
    SettingsButton->SetVisibility(bShowSettings ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    CaptionPanel->SetVisibility(Caption.bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (!Caption.bVisible) return;

    const FText Text = Caption.Speaker.IsEmpty() ? Caption.Text : FText::Format(
        NSLOCTEXT("OGMobileSettings", "CaptionWithSpeaker", "{0}: {1}"), Caption.Speaker, Caption.Text);
    if (!CaptionLine->GetText().EqualTo(Text)) CaptionLine->SetText(Text);

    const float Size = (Caption.Presentation == TEXT("large") ? 28.f : 22.f) *
        (Caption.Readability == TEXT("large_text") ? 1.12f : 1.f);
    const bool bHighContrast = Caption.Presentation == TEXT("high_contrast") ||
        Caption.Readability == TEXT("high_contrast");
    if (CaptionFontSize != Size || bContrastCaption != bHighContrast)
    {
        CaptionFontSize = Size;
        bContrastCaption = bHighContrast;
        CaptionLine->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::RoundToInt(Size)));
        CaptionPanel->SetBrushColor(FLinearColor(.01f, .015f, .02f, bHighContrast ? .98f : .80f));
    }
}

void UOGMobileSettingsOverlay::OpenSettings()
{
    if (auto* PC = GetOwningPlayer())
        if (auto* Hud = Cast<AOGWorldPresentationHud>(PC->GetHUD()))
            Hud->NotifyHitBoxClick(TEXT("OG.Settings.Toggle"));
}
