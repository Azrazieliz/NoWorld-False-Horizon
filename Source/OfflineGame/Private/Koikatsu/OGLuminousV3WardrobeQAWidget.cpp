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

    UButton* Clothed = AddButton(TEXT("QA CLOTHED"), FLinearColor(0.1f, 0.19f, 0.3f));
    UButton* Underwear = AddButton(TEXT("QA UNDERWEAR"), FLinearColor(0.21f, 0.15f, 0.25f));
    UButton* Nude = AddButton(TEXT("QA NUDE"), FLinearColor(0.25f, 0.16f, 0.18f));
    if (Clothed) Clothed->OnClicked.AddDynamic(this, &ThisClass::OnClothedClicked);
    if (Underwear) Underwear->OnClicked.AddDynamic(this, &ThisClass::OnUnderwearClicked);
    if (Nude) Nude->OnClicked.AddDynamic(this, &ThisClass::OnNudeClicked);

    // Real touch/gameplay playback of the four imported source Koikatsu clips.
    // Isolated native QA only; final Gate10 HUD remains untouched.
    UHorizontalBox* EmoteRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    if (!EmoteRow) return;
    UCanvasPanelSlot* EmoteSlot = Root->AddChildToCanvas(EmoteRow);
    EmoteSlot->SetAnchors(FAnchors(0.5f, 0.0f));
    EmoteSlot->SetAlignment(FVector2D(0.5f, 0.0f));
    EmoteSlot->SetPosition(FVector2D(0.0f, 90.0f));
    EmoteSlot->SetSize(FVector2D(660.0f, 62.0f));
    auto AddEmote = [this, EmoteRow](const TCHAR* Title) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!Button || !Label) return nullptr;
        Label->SetText(FText::FromString(Title));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Label->SetJustification(ETextJustify::Center);
        Button->SetBackgroundColor(FLinearColor(0.09f, 0.21f, 0.28f));
        Button->AddChild(Label);
        UHorizontalBoxSlot* Slot = EmoteRow->AddChildToHorizontalBox(Button);
        if (Slot)
        {
            Slot->SetPadding(FMargin(4.f, 3.f));
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
        return Button;
    };
    UButton* Thoughtful = AddEmote(TEXT("THOUGHTFUL"));
    UButton* Greeting = AddEmote(TEXT("GREETING"));
    UButton* Expressive = AddEmote(TEXT("EXPRESSIVE"));
    UButton* Bow = AddEmote(TEXT("BOW"));
    if (Thoughtful) Thoughtful->OnClicked.AddDynamic(this, &ThisClass::OnThoughtfulClicked);
    if (Greeting) Greeting->OnClicked.AddDynamic(this, &ThisClass::OnGreetingClicked);
    if (Expressive) Expressive->OnClicked.AddDynamic(this, &ThisClass::OnExpressiveClicked);
    if (Bow) Bow->OnClicked.AddDynamic(this, &ThisClass::OnBowClicked);
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

void UOGLuminousV3WardrobeQAWidget::PlayAuthenticEmote(int32 EmoteIndex)
{
    if (!TargetCharacter || EmoteIndex < 0 || EmoteIndex > 3) return;
    const bool bStarted = TargetCharacter->PlayGate09AuthoredEmote(
        static_cast<EOGLuminousV3AuthoredEmote>(EmoteIndex));
    UE_LOG(LogTemp, Display,
        TEXT("G12_LUMINOUS_TOUCH_EMOTE index=%d result=%s parts=%d error=%s"),
        EmoteIndex, bStarted ? TEXT("STARTED") : TEXT("REJECTED"),
        TargetCharacter->GetActiveLuminousPartCount(),
        *TargetCharacter->GetLastLuminousError());
}
void UOGLuminousV3WardrobeQAWidget::OnThoughtfulClicked() { PlayAuthenticEmote(0); }
void UOGLuminousV3WardrobeQAWidget::OnGreetingClicked() { PlayAuthenticEmote(1); }
void UOGLuminousV3WardrobeQAWidget::OnExpressiveClicked() { PlayAuthenticEmote(2); }
void UOGLuminousV3WardrobeQAWidget::OnBowClicked() { PlayAuthenticEmote(3); }
