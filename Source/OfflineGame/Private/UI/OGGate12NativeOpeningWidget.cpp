#include "UI/OGGate12NativeOpeningWidget.h"
#include "UI/OGGate12GothicChromeWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "UI/OGGate12VacantThroneSceneWidget.h"
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

    // Editable Gothic architectural art is a noninteractive backdrop,
    // not a pasted UI screenshot. Slate-only animation remains above it.
    // All actions are independent native UMG buttons.
    const TCHAR* OriginalScenePath =
        TEXT("/Game/UI/Gate10/Opening/T_Gate10_VacantThroneBackdrop_V1."
             "T_Gate10_VacantThroneBackdrop_V1");
    const UTexture2D* ThroneArt =
        LoadObject<UTexture2D>(nullptr, OriginalScenePath);
    if (ThroneArt)
    {
        UImage* BackdropArt=WidgetTree->ConstructWidget<UImage>(
            UImage::StaticClass(),TEXT("Gate10VacantThroneApprovedArtBackdrop"));
        BackdropArt->SetVisibility(ESlateVisibility::HitTestInvisible);
        BackdropArt->SetBrushFromTexture(
            const_cast<UTexture2D*>(ThroneArt),true);
        UCanvasPanelSlot* ArtSlot=Canvas->AddChildToCanvas(BackdropArt);
        ArtSlot->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));
        ArtSlot->SetOffsets(FMargin(0.f));
    }

    // Keep real reduced-motion-aware ambient elements and the native
    // animated 04 Vacant Throne vector tracery over the backdrop.
    UCanvasPanel* Stage = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("OpeningAnimatedSceneStage"));
    Stage->SetVisibility(ESlateVisibility::HitTestInvisible);
    StageSlot = Canvas->AddChildToCanvas(Stage);
    StageSlot->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));
    StageSlot->SetOffsets(FMargin(0.f));
    // Gate10 opening-04 genuine vector stage replaces the previous flat
    // gray UMG primitives, retains independent 2D graphics and all real
    // opening controls. Never load illustrative ten-concept artwork.
    NativeThroneScene = WidgetTree->ConstructWidget<UOGGate12VacantThroneSceneWidget>(
        UOGGate12VacantThroneSceneWidget::StaticClass(),
        TEXT("Gate10AnimatedVacantThroneVectorScene"));
    NativeThroneScene->SetVisibility(ESlateVisibility::HitTestInvisible);
    // Do not superimpose a second opaque throne on the imported source.
    // Subtle moving tracery preserves live HSR-style UI motion.
    NativeThroneScene->SetRenderOpacity(ThroneArt ? .16f : 1.f);
    UCanvasPanelSlot* VectorLayer=Stage->AddChildToCanvas(NativeThroneScene);
    VectorLayer->SetAnchors(FAnchors(0.f,0.f,1.f,1.f));
    VectorLayer->SetOffsets(FMargin(0.f));

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
    UCanvasPanelSlot* AccentSlot = Stage->AddChildToCanvas(AtmosphericAccent);
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
    TitleFont.Size = 38;
    NativeTitle->SetFont(TitleFont);
    TitleWidthConstraint = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("OpeningTitleWidthConstraint"));
    TitleWidthConstraint->SetWidthOverride(320.f);
    TitleWidthConstraint->AddChild(NativeTitle);
    TitleSlot = Canvas->AddChildToCanvas(TitleWidthConstraint);
    TitleSlot->SetAutoSize(true);
    TitleSlot->SetAnchors(FAnchors(.5f,.78f));
    TitleSlot->SetAlignment(FVector2D(.5f,.5f));

    UBorder* MenuFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("OpeningControlsFrame"));
    MenuFrame->SetBrushColor(FLinearColor(0.f,0.f,0.f,0.f));
    MenuFrame->SetPadding(FMargin(4.f,2.f,4.f,2.f));
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
    MenuSlot->SetAnchors(FAnchors(.5f,.92f));
    MenuSlot->SetPosition(FVector2D(0.f,0.f));

    // Actual native utility actions, circular ornamental buttons at upper
    // right. They never replace Settings/Recovery gameplay handlers.
    AddTopUtilityIcon(Canvas,TEXT("OG.Settings.Toggle"),TEXT("⚙"),
        TEXT("Settings"),.965f,SettingsIcon);
    AddTopUtilityIcon(Canvas,TEXT("OG.Opening.Recover"),TEXT("↺"),
        TEXT("Data recovery"),.835f,RecoveryIcon);
    RebuildMenu();
}

void UOGGate12NativeOpeningWidget::AddTopUtilityIcon(
    UCanvasPanel* Canvas,FName Action,const FString& Glyph,
    const FString& Description,float RightAnchor,
    TObjectPtr<UOGGate12OpeningActionButton>& OutButton)
{
    if (!Canvas || !WidgetTree) return;
    UOGGate12OpeningActionButton* Button =
        WidgetTree->ConstructWidget<UOGGate12OpeningActionButton>(
            UOGGate12OpeningActionButton::StaticClass());
    Button->Opening=this;
    Button->Action=Action;
    Button->SetToolTipText(FText::FromString(Description));
    Button->OnClicked.AddDynamic(
        Button,&UOGGate12OpeningActionButton::InvokeOpeningAction);
    const FSlateRoundedBoxBrush Normal(
        FLinearColor(.009f,.007f,.018f,.94f),23.f,
        FLinearColor(.65f,.60f,.74f,.89f),1.2f);
    const FSlateRoundedBoxBrush Hover(
        FLinearColor(.10f,.06f,.15f,.99f),23.f,
        FLinearColor(.82f,.78f,.91f,1.f),1.6f);
    FButtonStyle Style=Button->GetStyle();
    Style.SetNormal(Normal);Style.SetHovered(Hover);
    Style.SetPressed(Hover);
    Button->SetStyle(Style);
    UTextBlock* Mark=WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass());
    Mark->SetText(FText::FromString(Glyph));
    Mark->SetJustification(ETextJustify::Center);
    Mark->SetColorAndOpacity(FSlateColor(OpeningTextColor));
    FSlateFontInfo Font=Mark->GetFont(); Font.Size=23;Mark->SetFont(Font);
    Button->AddChild(Mark);
    UCanvasPanelSlot* UtilityCanvasSlot=Canvas->AddChildToCanvas(Button);
    UtilityCanvasSlot->SetAnchors(FAnchors(RightAnchor,.035f));
    UtilityCanvasSlot->SetAlignment(FVector2D(1.f,0.f));
    UtilityCanvasSlot->SetSize(FVector2D(43.f,43.f));
    OutButton=Button;
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
    // Recovery remains available to inspect/import external backups even
    // before an account has created its first local world.
    if (RecoveryIcon) RecoveryIcon->SetIsEnabled(true);
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
    Button->SetBackgroundColor(FLinearColor(0.f,0.f,0.f,0.f));
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
    Font.Size = bPrimary ? 36 : 29;
    TextLabel->SetFont(Font);
    Button->AddChild(TextLabel);
    UOverlay* Decorations = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass());
    UBorder* OpaqueBlack = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass());
    OpaqueBlack->SetBrushColor(FLinearColor(.006f,.005f,.010f,.97f));
    Decorations->AddChildToOverlay(OpaqueBlack);
    UOGGate12GothicChromeWidget* LiveFrame =
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass());
    LiveFrame->Configure(!bEnabled
        ? FLinearColor(.24f,.23f,.27f,1.f)
        : bPrimary
            ? FLinearColor(.78f,.68f,.45f,1.f)
            : FLinearColor(.56f,.54f,.61f,1.f),
        INDEX_NONE,bPrimary && bEnabled);
    Decorations->AddChildToOverlay(LiveFrame);
    Decorations->AddChildToOverlay(Button);
    USizeBox* ControlHeight=WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass());
    ControlHeight->SetMinDesiredHeight(bPrimary ? 69.f : 60.f);
    ControlHeight->AddChild(Decorations);
    UVerticalBoxSlot* AddedButtonSlot =
        Menu->AddChildToVerticalBox(ControlHeight);
    AddedButtonSlot->SetPadding(FMargin(3.f,4.f,3.f,4.f));
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
            TEXT("WORLD SAVE UNVERIFIED — RECOVERY REQUIRED")));
        Notice->SetAutoWrapText(true);
        Notice->SetColorAndOpacity(FSlateColor(Muted));
        Notice->SetJustification(ETextJustify::Center);
        FSlateFontInfo WarningFont=Notice->GetFont();
        WarningFont.Size=13;
        Notice->SetFont(WarningFont);
        Menu->AddChildToVerticalBox(Notice)->SetPadding(FMargin(3.f,1.f));
    }
    // A gacha account is not a disposable single-player slot.
    // Existing worlds resume safely; new Rulers enter the established
    // Ruler flow without inventing a destructive NEW GAME option.
    if (bHasRuler && bHasSave)
        AddAction(TEXT("CONTINUE"),TEXT("OG.Opening.Continue"),true,true);
    else
        // Fresh install: enter the already-existing Ruler workflow.
        // Unverified on-disk world: fail closed until Recovery, never replace.
        AddAction(TEXT("ENTER"),TEXT("OG.Opening.Ruler"),
                  !bWorldFilePresent,true);
}

void UOGGate12NativeOpeningWidget::DispatchNativeAction(FName Action)
{
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
            bWide ? FAnchors(.77f,.89f) : FAnchors(.5f,.92f));
        MenuSlot->SetAlignment(FVector2D(.5f,1.f));
        if (TitleSlot && TitleWidthConstraint)
        {
            const float Width = FMath::Max(150.f, FMath::Min(
                Size.X - 24.f, bWide ? Size.X * .40f : Size.X * .92f));
            TitleWidthConstraint->SetWidthOverride(Width);
            TitleSlot->SetAnchors(
                bWide ? FAnchors(.76f,.69f) : FAnchors(.5f,.68f));
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
    if (NativeThroneScene)
        NativeThroneScene->UpdateAtmosphere(Phase,bReducedMotion);
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
