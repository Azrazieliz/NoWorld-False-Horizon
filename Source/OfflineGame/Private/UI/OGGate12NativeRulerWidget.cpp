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

    NavigationBar = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("RulerFiveDestinations"));
    UVerticalBoxSlot* NavSlot = Screen->AddChildToVerticalBox(NavigationBar);
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
        UHorizontalBoxSlot* LayoutSlot = NavigationBar->AddChildToHorizontalBox(Button);
        LayoutSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LayoutSlot->SetPadding(FMargin(2.0f));
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
    Content->ClearChildren();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;

    if (!Session || !Session->IsBoundToStoredRuler())
    {
        AddMessage(Content, TEXT("NO CANONICAL RULER SELECTED"), 25, Gold);
        AddMessage(Content,
            TEXT("A local player must exist in the current canonical world. "
                 "Creating one does not grant characters, territory, currency or banners."),
            18, TextMuted);
        AddAction(Content, TEXT("CREATE LOCAL RULER"), TEXT("create-ruler"));
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
        BuildTerritory();
        break;
    case EOGRulerPrimaryDestination::Records:
        BuildRecords();
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
    AddMessage(Content, TEXT("ANALYTICAL OVERLAY"), 16, Gold);
    for (int32 Index = 0; Index < Territory.AnalyticalOverlays.Num(); ++Index)
    {
        const FName Overlay = Territory.AnalyticalOverlays[Index];
        AddAction(Content,
            FString::Printf(TEXT("%s%s"),
                Index == SelectedTerritoryOverlayIndex ? TEXT("> ") : TEXT(""),
                *Overlay.ToString()),
            FString::Printf(TEXT("overlay/%d"), Index));
    }

    AddMessage(Content, TEXT("MY TERRITORIES"), 16, Gold);
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

    if (Chosen)
    {
        AddMessage(Content, TEXT("SELECTED HOLDING"), 18, Gold);
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
            AddMessage(Content, TEXT("Control is contested."), 15, Gold);
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

    static const TCHAR* Sections[4] =
    {
        TEXT("REPORTS"), TEXT("CHRONICLE"), TEXT("CODEX"), TEXT("INTELLIGENCE")
    };
    for (int32 Index = 0; Index < 4; ++Index)
        AddAction(Content, Sections[Index],
            FString::Printf(TEXT("records/%d"), Index));

    FString Error;
    if ((RecordsSection == 1 || RecordsSection == 3) &&
        !Session->RefreshRecordsDetail(Error))
    {
        AddMessage(Content, Error.IsEmpty()
            ? TEXT("Canonical Records unavailable.") : Error, 16, TextMuted);
        return;
    }
    switch (RecordsSection)
    {
    case 0:
        AddMessage(Content,
            FString::Printf(TEXT("%d verified reports / %d unacknowledged"),
                Session->RecordsProjection.ReportCount,
                Session->RecordsProjection.UnacknowledgedReportCount),
            18, TextMain);
        if (Session->RecordsProjection.ReportCount <= 0)
            AddMessage(Content, TEXT("No reports in this world."), 16, TextMuted);
        else
            AddMessage(Content,
                TEXT("Report count is authoritative. Detailed report browser "
                     "requires installed report projection; no invented reports."),
                16, TextMuted);
        break;
    case 1:
        if (Session->ChronicleProjection.IsEmpty())
            AddMessage(Content, TEXT("No known Chronicle entries."), 16, TextMuted);
        for (const FOGChronicleEntryViewModel& Event : Session->ChronicleProjection)
            AddMessage(Content,
                FString::Printf(TEXT("%s  /  world tick %lld"),
                    *Event.EventType.ToString(), Event.WorldTick),
                16, TextMain);
        break;
    case 2:
        AddMessage(Content,
            TEXT("No verified installed Codex entries are available. "
                 "Unknown lore remains unknown."),
            16, TextMuted);
        break;
    case 3:
        if (Session->IntelligenceProjection.IsEmpty())
            AddMessage(Content, TEXT("No known Intelligence entries."), 16, TextMuted);
        for (const FOGIntelligenceEntryViewModel& Entry : Session->IntelligenceProjection)
        {
            // BuildIntelligence applies owner knowledge semantics; do not
            // reveal the value JSON of unknown/outdated intelligence.
            AddMessage(Content,
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
    if (Action == TEXT("create-ruler"))
    {
        if (!Session->CreateLocalRuler(Error))
            LastUiError = Error;
        else
            LastUiError.Reset();
        RefreshFromCanonical();
        return;
    }
    if (Action.StartsWith(TEXT("territory/")))
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
