#include "UI/OGNativeInteractionWidget.h"
#include "UI/OGReactiveUiSubsystem.h"
#include "UI/OGNativeUiStyle.h"
#include "Misc/ScopeExit.h"


#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Interaction/OGFoundationInteractionHost.h"
#include "Misc/Paths.h"

namespace
{
FSlateFontInfo InteractionFont(int32 Size, bool Heading = false)
{
    return FSlateFontInfo(Heading
        ? FPaths::ProjectContentDir() / TEXT("Slate/Fonts/Cinzel-Bold.ttf")
        : FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Medium.ttf"), static_cast<float>(Size));
}
}

void UOGInteractionActionButton::Invoke() { if (Screen) Screen->Activate(Action); }

void UOGNativeInteractionWidget::BindHost(UOGFoundationInteractionHost* InHost)
{
    if (Host.Get() == InHost) return;
    Host = InHost; RowsSignature.Reset();
    if (Host.IsValid()) Host->Initialize();
    Refresh();
}

void UOGNativeInteractionWidget::NativeConstruct()
{
    ON_SCOPE_EXIT { if (auto* Theme=UOGReactiveUiSubsystem::Find(this)) Theme->TrackTree(this); };

    Super::NativeConstruct();
    if (!WidgetTree || WidgetTree->RootWidget) return;
    auto* Safe = WidgetTree->ConstructWidget<USafeZone>();
    WidgetTree->RootWidget = Safe;
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); Safe->AddChild(Canvas);
    auto* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(FLinearColor(.009f, .007f, .019f, .95f));
    Panel->SetPadding(FMargin(20));
    auto* Slot = Canvas->AddChildToCanvas(FOGNativeUiStyle::Surface(*WidgetTree,Panel));
    Slot->SetAnchors(FAnchors(.04f, .08f, .60f, .92f)); Slot->SetOffsets(FMargin(0));
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Panel->AddChild(Column);
    auto* Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("INTERACTION"))); Title->SetFont(InteractionFont(28, true));
    Title->SetColorAndOpacity(FSlateColor(FLinearColor(.82f, .73f, .57f, 1)));
    Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 12));
    Status = WidgetTree->ConstructWidget<UTextBlock>(); Status->SetAutoWrapText(true);
    Status->SetFont(InteractionFont(18)); Status->SetColorAndOpacity(FSlateColor(FLinearColor(.76f, .75f, .82f, 1)));
    Column->AddChildToVerticalBox(Status)->SetPadding(FMargin(0, 0, 0, 14));
    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Column->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Actions = WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Actions);
    RowsSignature.Reset(); Refresh();
}

void UOGNativeInteractionWidget::AddButton(FName Action, const FString& Label, bool Enabled)
{
    auto* Bounds = WidgetTree->ConstructWidget<USizeBox>(); Bounds->SetMinDesiredHeight(56);
    Actions->AddChildToVerticalBox(Bounds)->SetPadding(FMargin(0, 0, 0, 9));
    auto* Button = WidgetTree->ConstructWidget<UOGInteractionActionButton>();
    Button->Screen = this; Button->Action = Action; Button->SetIsEnabled(Enabled);
    Button->SetBackgroundColor(FLinearColor(.075f, .047f, .10f, 1));
    Button->OnClicked.AddDynamic(Button, &UOGInteractionActionButton::Invoke);
    FOGNativeUiStyle::SkinButton(*Button); Bounds->AddChild(FOGNativeUiStyle::Surface(*WidgetTree,Button,EOGNativeSurface::Action));
    auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Label)); Text->SetFont(InteractionFont(20)); Text->SetAutoWrapText(true);
    Text->SetColorAndOpacity(FSlateColor(Enabled ? FLinearColor(.9f, .88f, .93f, 1) : FLinearColor(.39f, .38f, .43f, 1)));
    if (auto* Child = Cast<UButtonSlot>(Button->AddChild(Text)))
    { Child->SetPadding(FMargin(12, 10)); Child->SetVerticalAlignment(VAlign_Center); }
}

void UOGNativeInteractionWidget::Refresh()
{
    const FString ThemeSignatureBefore=RowsSignature;
    ON_SCOPE_EXIT { if (RowsSignature!=ThemeSignatureBefore) if (auto* Theme=UOGReactiveUiSubsystem::Find(this)) Theme->TrackTree(this); };

    if (!Actions || !Status) return;
    if (!Host.IsValid()) { Status->SetText(FText::FromString(TEXT("World character unavailable."))); return; }
    Status->SetText(FText::FromString(Host->GetStatus()));
    const auto Rows = Host->GetRows(); FString Signature;
    for (const auto& Row : Rows) Signature += Row.Id.ToString() + TEXT("|") + Row.Label + (Row.bEnabled ? TEXT("1;") : TEXT("0;"));
    if (Signature == RowsSignature) return;
    RowsSignature = Signature; Actions->ClearChildren();
    for (const auto& Row : Rows) AddButton(Row.Id, Row.Label, Row.bEnabled);
    AddButton(TEXT("screen_close"), TEXT("Close controls"), true);
}

void UOGNativeInteractionWidget::Activate(FName Action)
{
    if (Action == TEXT("screen_close"))
    {
        if (Host.IsValid()) Host->Cancel();
        OnClosed.Broadcast(); return;
    }
    if (Host.IsValid() && Host->ActivateRow(Action) && Action.ToString().StartsWith(TEXT("object_offers.")))
    { OnClosed.Broadcast(); return; }
    Refresh();
}

void UOGNativeInteractionWidget::InterruptForCombat()
{
    if (Host.IsValid() && Host->IsActive()) Host->Cancel(TEXT("combat_started"));
}

void UOGNativeInteractionWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    // Use most of the portrait viewport and leave the staged scene visible in landscape.
    if (WidgetTree && WidgetTree->RootWidget)
        if (auto* Safe = Cast<USafeZone>(WidgetTree->RootWidget))
            if (auto* Canvas = Cast<UCanvasPanel>(Safe->GetChildAt(0)))
                if (auto* Slot = Cast<UCanvasPanelSlot>(Canvas->GetChildAt(0)->Slot))
                { const auto Size = Geometry.GetLocalSize(); Slot->SetAnchors(FAnchors(.04f, .08f, Size.X < Size.Y ? .96f : .60f, .92f)); }
    RefreshElapsed += DeltaSeconds;
    if (RefreshElapsed >= .15f) { RefreshElapsed = 0; Refresh(); }
}
