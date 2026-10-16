#include "UI/OGGate12NativeRulerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "UI/OGGate12CharacterVisualCatalog.h"
#include "UI/OGGate12CharacterPreviewStage.h"
#include "UI/OGGate12CharacterPresentationPreferences.h"
#include "Runtime/OGPlayerProfileSettings.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
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
const FLinearColor Gate12RulerGold(0.72f, 0.62f, 0.44f, 1.0f);

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

void UOGGate12NativeRulerWidget::NativeDestruct()
{
    ReleasePreviewStage();
    Super::NativeDestruct();
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
    Heading->SetColorAndOpacity(FSlateColor(Gate12RulerGold));
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
    RosterGrid = nullptr;
    Content->ClearChildren();
    if (OpenIdentityIndex == INDEX_NONE || !bPrefer3DCharacterPresentation)
        ReleasePreviewStage();
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;

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

    AddMessage(Content, DestinationTitle(Session->CurrentDestination), 26, Gate12RulerGold);
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
            TEXT("No fabricated banner, pool, ticket, currency or secondary duplicate rewards."),
            16, TextMuted);
        AddAction(Content,
            bGachaHistoryExpanded ? TEXT("HIDE ACQUISITION HISTORY")
                                  : TEXT("VIEW ACQUISITION HISTORY"),
            TEXT("gacha-history-toggle"));
        if (bGachaHistoryExpanded)
        {
            FString GachaError;
            if (!Session->RefreshGachaHistory(GachaError))
            {
                AddMessage(Content, GachaError.IsEmpty()
                    ? TEXT("Canonical gacha history unavailable.")
                    : GachaError, 16, TextMuted);
            }
            else if (Session->GachaHistoryProjection.IsEmpty())
            {
                AddMessage(Content, TEXT("No recorded acquisition events."),
                    16, TextMuted);
            }
            else
            {
                for (const FOGGachaHistoryEntryViewModel& Entry :
                    Session->GachaHistoryProjection)
                {
                    AddMessage(Content,
                        FString::Printf(TEXT("%s  %s  %s  tick %lld"),
                            *Entry.IdentityId.ToString(),
                            *Entry.VersionId.ToString(),
                            *Entry.Rarity.ToString(),
                            Entry.WorldTick),
                        15, TextMain);
                }
            }
        }
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
    if (!Session || Session->OwnedRoster.IsEmpty())
    {
        AddMessage(Content, TEXT("No owned character Manifestations."), 18, TextMuted);
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
    RosterGrid->SetSlotPadding(FMargin(3.0f));
    UScaleBox* Responsive = WidgetTree->ConstructWidget<UScaleBox>(
        UScaleBox::StaticClass());
    Responsive->SetStretch(EStretch::ScaleToFit);
    Responsive->AddChild(RosterGrid);
    Content->AddChildToVerticalBox(Responsive);
    RebuildRosterGrid();
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

    // Exactly three uniform portrait cards in original approved proportions.
    constexpr float CardWidth = 144.0f;
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
                        Selected->ActiveVersionId, Selected->ManifestationId))
                {
                    Stage->SetLiveCapture(false);
                    RosterPreviewStages.Add(Stage);
                    CardTexture = Stage->GetLiveRenderTarget();
                }
                else if (IsValid(Stage))
                    Stage->Destroy();
            }
        }
        else if (!bCard3D && Entry && !Entry->OriginalArtwork2D.IsNull())
        {
            if (UTexture2D* OriginalArt = Entry->OriginalArtwork2D.LoadSynchronous())
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
                        : TEXT("ORIGINAL ART NOT INSTALLED"), 12, TextMuted);
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

    static const TCHAR* Sections[4] =
    {
        TEXT("REPORTS"), TEXT("CHRONICLE"), TEXT("CODEX"), TEXT("INTELLIGENCE")
    };
    for (int32 Index = 0; Index < 4; ++Index)
        AddAction(Content, Sections[Index],
            FString::Printf(TEXT("records/%d"), Index));

    FString Error;
    if ((RecordsSection == 0 || RecordsSection == 1 || RecordsSection == 3) &&
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
        if (Session->ReportProjection.IsEmpty())
        {
            AddMessage(Content, TEXT("No reports in this world."), 16, TextMuted);
        }
        for (int32 Index = 0; Index < Session->ReportProjection.Num(); ++Index)
        {
            const FOGReportRecord& Report = Session->ReportProjection[Index];
            AddMessage(Content,
                FString::Printf(TEXT("%s | priority %d | world tick %lld | %s"),
                    *Report.Category.ToString(), Report.Priority,
                    Report.CreatedWorldTick,
                    Report.bAcknowledged ? TEXT("READ") : TEXT("UNREAD")),
                16, Report.bAcknowledged ? TextMuted : TextMain);
            if (!Report.bAcknowledged)
                AddAction(Content, TEXT("ACKNOWLEDGE REPORT"),
                    FString::Printf(TEXT("report/ack/%d"), Index));
        }
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
                Selected->ActiveVersionId, Selected->ManifestationId))
        {
            ReleasePreviewStage();
            AddMessage(Parent, TEXT("Live skeletal 3D preview unavailable."), 15, TextMuted);
            return;
        }
        Brush.SetResourceObject(PreviewStage->GetLiveRenderTarget());
        Brush.ImageSize = FVector2D(512.0f, 768.0f);
    }
    else
    {
        if (Entry->OriginalArtwork2D.IsNull())
        {
            AddMessage(Parent, TEXT("Original 2D artwork not yet supplied."), 15, TextMuted);
            return;
        }
        UTexture2D* OriginalArt = Entry->OriginalArtwork2D.LoadSynchronous();
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
    UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->AddChild(Image);
    USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Height->SetHeightOverride(500.0f);
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
    AddMessage(Content,
        Identity.DisplayNameKey.IsEmpty()
            ? Identity.IdentityId.ToString() : Identity.DisplayNameKey,
        24, TextMain);
    AddMessage(Content,
        FString::Printf(TEXT("%d separately owned Manifestations"),
            Identity.ManifestationCount), 17, TextMuted);
    AddAction(Content, bPrefer3DCharacterPresentation ? TEXT("PRESENTATION: 3D (switch to 2D)") : TEXT("PRESENTATION: 2D (switch to 3D)"), TEXT("presentation/toggle"));
    AddCharacterVisual(Content, Identity, bPrefer3DCharacterPresentation);
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
    else if (Action == TEXT("roster/sort"))
    {
        RosterSortMode = (RosterSortMode + 1) % 3;
    }
    else if (Action == TEXT("gacha-history-toggle"))
    {
        bGachaHistoryExpanded = !bGachaHistoryExpanded;
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
    else if (Action.StartsWith(TEXT("report/ack/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(11));
        if (Session->ReportProjection.IsValidIndex(Index))
            Session->AcknowledgeOwnedReport(
                Session->ReportProjection[Index].ReportId, Error);
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
    else if (Action == TEXT("world"))
    {
        if (Session->EnterWorld(Error))
        {
            OnWorldPresentationRequested.Broadcast();
            return;
        }
    }

    LastUiError = Error;
    if (Error.IsEmpty()) RefreshFromCanonical();
    else BuildCurrentPage();
}
