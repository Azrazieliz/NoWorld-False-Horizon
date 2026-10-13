#include "UI/OGGate12NativeRulerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
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

namespace
{
const FLinearColor TextMain(0.90f, 0.90f, 0.89f, 1.0f);
const FLinearColor TextMuted(0.55f, 0.58f, 0.61f, 1.0f);
const FLinearColor Gold(0.72f, 0.62f, 0.44f, 1.0f);

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
    Background->SetBrushColor(FLinearColor(0.012f, 0.013f, 0.018f, 1.0f));
    Background->AddChild(Screen);

    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("GameTitle"));
    Heading->SetText(FText::FromString(TEXT("NoWorld: False Horizon")));
    Heading->SetColorAndOpacity(FSlateColor(Gold));
    FSlateFontInfo HeadingFont = Heading->GetFont();
    HeadingFont.Size = 24;
    Heading->SetFont(HeadingFont);
    UVerticalBoxSlot* HeadingSlot = Screen->AddChildToVerticalBox(Heading);
    HeadingSlot->SetPadding(FMargin(20.0f, 18.0f, 20.0f, 12.0f));

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("RulerScrollableContent"));
    UVerticalBoxSlot* ScrollSlot = Screen->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ScrollSlot->SetPadding(FMargin(16.0f, 4.0f, 16.0f, 4.0f));
    Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CanonicalPageBody"));
    Scroll->AddChild(Content);

    Navigation = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("RulerFiveDestinations"));
    UVerticalBoxSlot* NavSlot = Screen->AddChildToVerticalBox(Navigation);
    NavSlot->SetPadding(FMargin(6.0f, 8.0f, 6.0f, 18.0f));
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
    UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Label);
    Slot->SetPadding(FMargin(8.0f, 7.0f));
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
    UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Button);
    Slot->SetPadding(FMargin(5.0f, 4.0f, 5.0f, 4.0f));
}

void UOGGate12NativeRulerWidget::BuildNav()
{
    if (!Navigation || !WidgetTree) return;
    Navigation->ClearChildren();
    static const TCHAR* Titles[5] =
    {
        TEXT("Home"), TEXT("Characters"), TEXT("Gacha"),
        TEXT("Territory"), TEXT("Records")
    };

    for (int32 Index = 0; Index < 5; ++Index)
    {
        UOGGate12RulerActionButton* Button =
            WidgetTree->ConstructWidget<UOGGate12RulerActionButton>(
                UOGGate12RulerActionButton::StaticClass());
        Button->RulerWidget = this;
        Button->ActionId = FString::Printf(TEXT("nav/%d"), Index);
        Button->SetBackgroundColor(FLinearColor(0.07f, 0.07f, 0.075f, 1.0f));
        Button->OnClicked.AddDynamic(Button, &UOGGate12RulerActionButton::InvokeAction);
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(FText::FromString(Titles[Index]));
        Label->SetJustification(ETextJustify::Center);
        Label->SetColorAndOpacity(FSlateColor(TextMain));
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = 13;
        Label->SetFont(Font);
        Button->AddChild(Label);
        UHorizontalBoxSlot* Slot = Navigation->AddChildToHorizontalBox(Button);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(2.0f));
    }
}

void UOGGate12NativeRulerWidget::RefreshFromCanonical()
{
    if (!Screen) BuildRoot();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;

    if (Session && Session->IsBoundToStoredRuler())
    {
        FString Error;
        if (!Session->RefreshCanonicalProjections(Error))
            LastUiError = Error;
        else
            LastUiError.Reset();
    }
    BuildCurrentPage();
}

void UOGGate12NativeRulerWidget::BuildCurrentPage()
{
    if (!Content) return;
    Content->ClearChildren();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;

    if (!Session || !Session->IsBoundToStoredRuler())
    {
        AddMessage(Content, TEXT("NO CANONICAL RULER SELECTED"), 25, Gold);
        AddMessage(Content,
            TEXT("A real existing Ruler/world must be loaded before any roster, "
                 "gacha balance or territory can be shown. Nothing has been fabricated."),
            18, TextMuted);
        BuildNav();
        return;
    }

    AddMessage(Content, DestinationTitle(Session->CurrentDestination), 26, Gold);
    if (!LastUiError.IsEmpty()) AddMessage(Content, LastUiError, 16, TextMuted);
    switch (Session->CurrentDestination)
    {
    case EOGRulerPrimaryDestination::Home:
        AddMessage(Content, TEXT("CANONICAL WORLD"), 20, TextMain);
        AddMessage(Content,
            FString::Printf(TEXT("%d owned character identities"),
                Session->OwnedRoster.Num()), 18, TextMuted);
        AddMessage(Content,
            FString::Printf(TEXT("%d persistent territories"),
                Session->TerritoryProjection.Territories.Num()), 18, TextMuted);
        AddMessage(Content,
            FString::Printf(TEXT("%d unread reports"),
                Session->HomeProjection.UnacknowledgedReportCount), 18, TextMuted);
        AddAction(Content, TEXT("ENTER WORLD"), TEXT("world"));
        break;
    case EOGRulerPrimaryDestination::Characters:
        if (OpenIdentityIndex == INDEX_NONE) BuildRoster();
        else BuildCharacterDetail();
        break;
    case EOGRulerPrimaryDestination::Gacha:
        AddMessage(Content,
            Session->bRealGachaAccessUnlocked
                ? TEXT("No approved live banner is available.")
                : TEXT("Gacha is locked by the canonical world state."),
            19, TextMuted);
        AddMessage(Content,
            TEXT("No fabricated banner, pool, ticket, currency or duplicate chip rewards."),
            16, TextMuted);
        break;
    case EOGRulerPrimaryDestination::Territory:
        AddMessage(Content,
            FString::Printf(TEXT("%d actual controlled territories"),
                Session->TerritoryProjection.Territories.Num()), 20);
        if (Session->TerritoryProjection.Territories.IsEmpty())
            AddMessage(Content, TEXT("No territory holdings in current world."), 18, TextMuted);
        break;
    case EOGRulerPrimaryDestination::Records:
        AddMessage(Content,
            FString::Printf(TEXT("%d reports"),
                Session->RecordsProjection.ReportCount), 20);
        AddMessage(Content,
            TEXT("Reports / Chronicle / Codex / Intelligence consume the existing "
                 "canonical records services; no fabricated historical entries."),
            17, TextMuted);
        break;
    }
    BuildNav();
}

void UOGGate12NativeRulerWidget::BuildRoster()
{
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Session || Session->OwnedRoster.IsEmpty())
    {
        AddMessage(Content, TEXT("No owned character Manifestations."), 18, TextMuted);
        return;
    }

    // Fixed 3-column equal-size grid; never a chip/reward tile. Scale the
    // entire grid for narrow portrait devices rather than resizing one card.
    constexpr float CardWidth = 144.0f;
    const float CardHeight = CardWidth / GetApprovedRosterCardRatio();
    UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(
        UUniformGridPanel::StaticClass());
    Grid->SetSlotPadding(FMargin(3.0f));
    UScaleBox* Responsive = WidgetTree->ConstructWidget<UScaleBox>(
        UScaleBox::StaticClass());
    Responsive->SetStretch(EStretch::ScaleToFit);
    Responsive->AddChild(Grid);
    Content->AddChildToVerticalBox(Responsive);

    for (int32 Index = 0; Index < Session->OwnedRoster.Num(); ++Index)
    {
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
        UVerticalBoxSlot* Spacer = CardBody->AddChildToVerticalBox(
            WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()));
        Spacer->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        // Artwork is deliberately not invented or substituted with a generic character.
        AddMessage(CardBody, TEXT("ART NOT INSTALLED"), 12, TextMuted);
        AddMessage(CardBody,
            Identity.DisplayNameKey.IsEmpty()
                ? Identity.IdentityId.ToString() : Identity.DisplayNameKey,
            16, TextMain);
        AddMessage(CardBody,
            FString::Printf(TEXT("%d manifestations"), Identity.ManifestationCount),
            12, Gold);
        Button->AddChild(CardBody);
        AngularFrameBase->AddChild(Button);
        Fixed->AddChild(AngularFrameBase);
        Grid->AddChildToUniformGrid(Fixed, Index / 3, Index % 3);
    }
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
    AddMessage(Content,
        TEXT("Appearance choices use the real character detail and canonical state. "
             "Privacy only changes the effective render, never actual clothing."),
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
    if (Action.StartsWith(TEXT("nav/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(4));
        if (Index < 0 || Index >= 5) return;
        OpenIdentityIndex = INDEX_NONE;
        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);
    }
    else if (Action.StartsWith(TEXT("identity/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(9));
        if (Session->OwnedRoster.IsValidIndex(Index)) OpenIdentityIndex = Index;
    }
    else if (Action.StartsWith(TEXT("select/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(7));
        if (Session->OwnedRoster.IsValidIndex(OpenIdentityIndex))
        {
            const FOGRosterIdentityViewModel& Identity = Session->OwnedRoster[OpenIdentityIndex];
            if (Identity.Manifestations.IsValidIndex(Index))
                Session->SelectOwnedManifestation(Identity.IdentityId,
                    Identity.Manifestations[Index].ManifestationId, Error);
        }
    }
    else if (Action == TEXT("back"))
    {
        OpenIdentityIndex = INDEX_NONE;
    }
    else if (Action == TEXT("world"))
    {
        if (Session->EnterWorld(Error))
        {
            OnWorldPresentationRequested.Broadcast();
            return;
        }
    }

    LastUiError = Error;
    RefreshFromCanonical();
}
