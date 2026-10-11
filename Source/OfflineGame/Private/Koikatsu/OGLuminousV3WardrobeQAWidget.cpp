#include "Koikatsu/OGLuminousV3WardrobeQAWidget.h"

#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"

void UOGLuminousV3WardrobeQAWidget::SetTargetCharacter(
    AOGLuminousV3PlayableCharacter* InCharacter)
{
    TargetCharacter = InCharacter;
}

void UOGLuminousV3WardrobeQAWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!Root || !Row) return;
    WidgetTree->RootWidget = Root;
    UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Row);
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f));
    PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
    PanelSlot->SetPosition(FVector2D(0.0f, 30.0f));
    PanelSlot->SetSize(FVector2D(480.0f, 54.0f));

    auto AddButton = [this, Row](const TCHAR* Title, const FLinearColor& Shade) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!Button || !Label) return nullptr;
        Label->SetText(FText::FromString(Title));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Label->SetJustification(ETextJustify::Center);
        Button->SetBackgroundColor(Shade);
        Button->AddChild(Label);
        UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Button);
        if (Slot)
        {
            Slot->SetPadding(FMargin(4.f, 3.f));
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }
        return Button;
    };

    UButton* Clothed = AddButton(TEXT("CLOTHED"), FLinearColor(0.1f, 0.19f, 0.3f));
    UButton* Underwear = AddButton(TEXT("UNDERWEAR"), FLinearColor(0.21f, 0.15f, 0.25f));
    UButton* Nude = AddButton(TEXT("NUDE"), FLinearColor(0.25f, 0.16f, 0.18f));
    if (Clothed) Clothed->OnClicked.AddDynamic(this, &ThisClass::OnClothedClicked);
    if (Underwear) Underwear->OnClicked.AddDynamic(this, &ThisClass::OnUnderwearClicked);
    if (Nude) Nude->OnClicked.AddDynamic(this, &ThisClass::OnNudeClicked);
}

void UOGLuminousV3WardrobeQAWidget::ChangeWardrobe(int32 StateIndex)
{
    if (!TargetCharacter || StateIndex < 0 || StateIndex > 2) return;
    const EOGLuminousV3Wardrobe State =
        static_cast<EOGLuminousV3Wardrobe>(StateIndex);
    const bool bChanged = TargetCharacter->SetLuminousWardrobe(State, false);
    UE_LOG(LogTemp, Display, TEXT("G12_LUMINOUS_TOUCH_WARDROBE_UI=%d result=%s parts=%d"),
        StateIndex, bChanged ? TEXT("PASS") : TEXT("FAIL"),
        TargetCharacter->GetActiveLuminousPartCount());
}

void UOGLuminousV3WardrobeQAWidget::OnClothedClicked() { ChangeWardrobe(0); }
void UOGLuminousV3WardrobeQAWidget::OnUnderwearClicked() { ChangeWardrobe(1); }
void UOGLuminousV3WardrobeQAWidget::OnNudeClicked() { ChangeWardrobe(2); }
