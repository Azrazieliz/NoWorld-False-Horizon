#include "UI/OGGate12NativeRulerWidget.h"
#include "UI/OGGate12CharacterVisualCatalog.h"
#include "UI/OGNativeUiStyle.h"
#include "UI/OGReactiveUiSubsystem.h"
#include "Gacha/OGSharedGachaBannerDefinition.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGPlayerProfileSettings.h"
#include "World/OGStartingRegionPresentation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"

namespace
{
constexpr double GachaCueSeconds=.45;
constexpr double GachaCardSeconds=.85;

AOGWorldPresentationHud* AcquisitionHud(const UUserWidget& Screen)
{ return Screen.GetOwningPlayer()?Cast<AOGWorldPresentationHud>(Screen.GetOwningPlayer()->GetHUD()):nullptr; }
FSlateFontInfo AcquisitionFont(int32 Size,bool Display=false)
{
    return FSlateFontInfo(Display?FPaths::ProjectContentDir()/TEXT("Slate/Fonts/Cinzel-Bold.ttf"):
        FPaths::EngineContentDir()/TEXT("Slate/Fonts/Roboto-Medium.ttf"),static_cast<float>(Size));
}
FString PaymentText(const FOGGachaBatchPreview& Quote,bool Balances=false)
{
    TArray<FString> Parts;
    for (const auto& Line:Quote.Payments)
        if (Balances||Line.Spend>0)
            Parts.Add(FString::Printf(TEXT("%lld %s%s"),Balances?Line.Balance:Line.Spend,
                *Line.ResourceId.ToString(),Line.bTicket?TEXT(" (ticket)"):TEXT("")));
    return Parts.IsEmpty()?TEXT("No currency required"):FString::Join(Parts,TEXT("  +  "));
}
FLinearColor AcquisitionRarity(FName Tier)
{
    if (Tier==TEXT("R")) return FLinearColor(.94f,.81f,.32f,1);
    if (Tier==TEXT("SR")) return FLinearColor(.92f,.47f,.70f,1);
    if (Tier==TEXT("SSR")) return FLinearColor(.92f,.25f,.31f,1);
    if (Tier==TEXT("UR")) return FLinearColor(.38f,.59f,.92f,1);
    if (Tier==TEXT("LR")) return FLinearColor(.81f,.67f,.34f,1);
    if (Tier==TEXT("MR")) return FLinearColor(.77f,.80f,.86f,1);
    return FLinearColor(.59f,.59f,.65f,1);
}
}
void UOGGate12NativeRulerWidget::ClearGachaReview()
{
    PendingGachaReview=FOGGachaBatchPreview(); PendingGachaRequestId=FOGEntityId();
    GachaConfirmButton=nullptr; GachaReviewStatus=nullptr;
}
void UOGGate12NativeRulerWidget::FinishGachaReveal()
{
    GachaRevealEndsAt=0.0;
    bGachaRevealCue=false;
    GachaRevealedResults=GachaSavedResults.Num();
    GachaAnimatedReveal=nullptr;
}

bool UOGGate12NativeRulerWidget::AdvanceGachaReveal()
{
    if (!bGachaResultsExpanded || GachaRevealEndsAt<=0.0 || GachaSavedResults.IsEmpty()) return false;
    const double Now=FPlatformTime::Seconds();
    if (Now<GachaRevealEndsAt)
    {
        const double Duration=bGachaRevealCue?GachaCueSeconds:GachaCardSeconds;
        const float Progress=FMath::Clamp(static_cast<float>(1.0-(GachaRevealEndsAt-Now)/Duration),0.f,1.f);
        if (GachaAnimatedReveal)
        {
            const float Entry=FMath::InterpEaseOut(0.f,1.f,FMath::Clamp(Progress*2.9f,0.f,1.f),2.5f);
            GachaAnimatedReveal->SetRenderOpacity(FMath::Lerp(.22f,1.f,Entry));
            const float Scale=FMath::Lerp(bGachaRevealCue?.94f:.975f,1.f,Entry);
            GachaAnimatedReveal->SetRenderScale(FVector2D(Scale,Scale));
        }
        return false;
    }

    if (bGachaRevealCue && GachaSavedResults.IsValidIndex(GachaRevealedResults))
    {
        ++GachaRevealedResults;
        bGachaRevealCue=false;
        GachaRevealEndsAt=Now+GachaCardSeconds;
    }
    else if (GachaRevealedResults<GachaSavedResults.Num())
    {
        bGachaRevealCue=true;
        GachaRevealEndsAt=Now+GachaCueSeconds;
    }
    else FinishGachaReveal();

    BuildCurrentPage();
    return true;
}

void UOGGate12NativeRulerWidget::ResetGachaUi()
{
    if (GachaEventFeed) GachaEventFeed->Remove(GachaEventHandle);
    GachaEventFeed.Reset();GachaEventHandle.Reset();
    if (auto* Library=UOGDefinitionLibrary::Get()) Library->OnDefinitionsChanged.Remove(GachaLibraryHandle);
    GachaLibraryHandle.Reset();bGachaDefinitionsDirty=false;
    GachaContextRulerId=FOGEntityId();GachaContextBannerId=FOGContentId();
    VisibleGachaBannerIds.Reset();
    GachaNextScheduleTick = -1;
    ClearGachaReview();GachaQuotes.Reset();GachaSavedResults.Reset();GachaDrawButtons.Reset();GachaCostLabels.Reset();
    GachaWalletLabel=nullptr;GachaPityLabel=nullptr;GachaPityBar=nullptr;
    bGachaRatesExpanded=bGachaResultsExpanded=bGachaHistoryExpanded=false;
    GachaFeedback.Reset();bGachaDirty=true;
    FinishGachaReveal();
}
void UOGGate12NativeRulerWidget::HandleGachaEvents(const TArray<FOGWorldEvent>& Events)
{ if (!Events.IsEmpty()) bGachaDirty=true; } // Never read intermediate transaction state in a commit callback.
void UOGGate12NativeRulerWidget::ObserveGacha(UOGGate12RulerSessionSubsystem& Session)
{
    auto* Hud=AcquisitionHud(*this);
    const FOGContentId Banner=Hud&&Hud->HasActiveGachaPresentation()?Hud->GetActiveGachaBanner().BannerId:FOGContentId();
    if (GachaContextRulerId == Session.BoundRulerId)
    {
        if (GachaContextBannerId != Banner)
        {
            GachaContextBannerId = Banner;
            if (PendingGachaRequestId.IsValid())
            {
                ClearGachaReview();
                GachaFeedback = TEXT("The reviewed banner is no longer active. Select an available banner.");
            }
            GachaQuotes.Reset();
            bGachaRatesExpanded = false;
            bGachaDirty = true;
        }
        return;
    }
    ResetGachaUi();GachaContextRulerId=Session.BoundRulerId;GachaContextBannerId=Banner;
    if (auto* Library=UOGDefinitionLibrary::Get())
        GachaLibraryHandle=Library->OnDefinitionsChanged.AddWeakLambda(this,[this](int64){bGachaDefinitionsDirty=true;});
    if (auto* Core=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>():nullptr)
    {
        GachaEventFeed=Core->GetEntityEventFeed(GachaContextRulerId);
        if (GachaEventFeed) GachaEventHandle=GachaEventFeed->AddUObject(this,&ThisClass::HandleGachaEvents);
    }
}
void UOGGate12NativeRulerWidget::SynchronizeGachaUi()
{
    if (!GachaContextRulerId.IsValid()) return;
    auto* Session=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    if (!Session||!Session->IsBoundToStoredRuler()||Session->BoundRulerId!=GachaContextRulerId||
        Session->CurrentDestination!=EOGRulerPrimaryDestination::Gacha)
    { ResetGachaUi(); return; }
    if (AdvanceGachaReveal()) return;
    auto* Hud = AcquisitionHud(*this);
    auto* Core = GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    const int64 Tick = Core && Core->IsCoreReady() ? Core->GetCanonicalWorldTick() : -1;
    const bool bScheduleChanged = GachaNextScheduleTick >= 0 && Tick >= GachaNextScheduleTick;
    if (Hud && Tick >= 0 && (bGachaDefinitionsDirty || bScheduleChanged))
    {
        bGachaDefinitionsDirty = false;
        FString Before, After, Error;
        FOGGachaBannerDefinition::StaticStruct()->ExportText(
            Before, &Hud->GetActiveGachaBanner(), nullptr, nullptr, PPF_None, nullptr);
        if (!Hud->LoadInstalledGachaBannerForRuler(Error))
        {
            Hud->ClearActiveGachaPresentation();
        }
        FOGGachaBannerDefinition::StaticStruct()->ExportText(
            After, &Hud->GetActiveGachaBanner(), nullptr, nullptr, PPF_None, nullptr);

        TArray<FOGGachaBannerDefinition> Installed;
        FOGInstalledGachaBannerLibrary::Collect(Tick, Installed, Error, &GachaNextScheduleTick);
        TArray<FOGContentId> CurrentIds;
        for (const auto& Banner : Installed)
        {
            CurrentIds.Add(Banner.BannerId);
        }
        if (Before != After || CurrentIds != VisibleGachaBannerIds)
        {
            bGachaDirty = true;
            BuildCurrentPage();
            return;
        }
        // A theme-only change leaves the active controls and review in place.
    }
    if (!Hud||!Hud->HasActiveGachaPresentation()||Hud->GetActiveGachaBanner().BannerId!=GachaContextBannerId)
    {
        for (const auto& Button:GachaDrawButtons) if (Button) Button->SetIsEnabled(false);
        if (GachaConfirmButton) GachaConfirmButton->SetIsEnabled(false);
        if (GachaReviewStatus) GachaReviewStatus->SetText(FText::FromString(TEXT("This banner changed. Close this review and select an available banner.")));
        return;
    }
    if (GachaPityBar)
        if (auto* Style=UOGReactiveUiSubsystem::Find(this)) GachaPityBar->SetFillColorAndOpacity(Style->GetPalette().Accent);
    if (!bGachaDirty) return;
    auto* Store=Core?Core->GetWorldStore():nullptr;
    if (!Store||!Store->IsOpen()||Store->IsWriteTransactionActive()) return;
    bGachaDirty=false;GachaQuotes.SetNum(2);
    const auto& Banner=Hud->GetActiveGachaBanner(); FString Error;
    bool AnyQuote=false;
    for (int32 I=0;I<2;++I)
    {
        auto& Quote=GachaQuotes[I];
        const bool Read=FOGGachaService(*Store).PreviewBatch(Banner,GachaContextRulerId,
            Tick,I==0?1:10,Quote,Error);
        const bool Enabled=Read&&Quote.bCanPull;
        if (GachaDrawButtons.IsValidIndex(I)&&GachaDrawButtons[I]) GachaDrawButtons[I]->SetIsEnabled(Enabled&&!bGachaCommandInFlight);
        if (GachaCostLabels.IsValidIndex(I)&&GachaCostLabels[I])
            GachaCostLabels[I]->SetText(FText::FromString(Read?PaymentText(Quote):TEXT("Payment unavailable")));
        AnyQuote|=Read;
    }
    if (GachaWalletLabel) GachaWalletLabel->SetText(FText::FromString(AnyQuote?PaymentText(GachaQuotes[0],true):Error));
    if (GachaPityLabel&&AnyQuote)
        GachaPityLabel->SetText(FText::FromString(FString::Printf(
            TEXT("%s pity  %d / %d   |   Next top rarity %.2f%%%s"),
            *Banner.TopRarity.ToString(),GachaQuotes[0].Pity.PullsSinceTopRarity,Banner.HardPity,
            GachaQuotes[0].NextTopRarityChanceBps/100.0,
            GachaQuotes[0].Pity.bFeaturedGuarantee?TEXT("   |   Featured guarantee active"):TEXT(""))));
    if (GachaPityBar&&AnyQuote) GachaPityBar->SetPercent(Banner.HardPity>0?
        FMath::Clamp(static_cast<float>(GachaQuotes[0].Pity.PullsSinceTopRarity)/Banner.HardPity,0.f,1.f):0.f);
    if (PendingGachaRequestId.IsValid())
    {
        FOGGachaBatchPreview TicketQuote;
        const bool TicketReview=PendingGachaReview.BatchTicketId.IsValid();
        const int32 Index=PendingGachaReview.Count==10?1:0;
        const bool Read=!TicketReview || FOGGachaService(*Store).PreviewBatch(Banner,GachaContextRulerId,Tick,
            PendingGachaReview.Count,TicketQuote,Error,PendingGachaReview.BatchTicketId);
        const auto& Fresh=TicketReview?TicketQuote:GachaQuotes[Index];
        const bool Same=Read && Fresh.bCanPull && Fresh.ConfirmationToken==PendingGachaReview.ConfirmationToken;
        if (GachaConfirmButton) GachaConfirmButton->SetIsEnabled(Same&&!bGachaCommandInFlight);
        if (GachaReviewStatus) GachaReviewStatus->SetText(FText::FromString(Same?
            TEXT("Confirm the exact payment shown above."):TEXT("Balance, access, pity or banner changed. Review the draw again.")));
    }
}
void UOGGate12NativeRulerWidget::BuildGacha()
{
    if (auto* Hud = AcquisitionHud(*this))
    {
        FString Error;
        if (!Hud->LoadInstalledGachaBannerForRuler(Error))
        {
            Hud->ClearActiveGachaPresentation();
        }
    }
    BuildGachaPortrait();
}
void UOGGate12NativeRulerWidget::BuildGachaPortrait()
{
    if (!Gate10PortraitGachaCanvas||!WidgetTree) return;
    auto* Session=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    if (!Session||!Session->IsBoundToStoredRuler()) { ResetGachaUi();return; }
    ObserveGacha(*Session);
    Gate10PortraitGachaCanvas->ClearChildren();
    GachaAnimatedReveal=nullptr;
    GachaDrawButtons.Reset();GachaCostLabels.Reset();GachaReviewStatus=nullptr;GachaConfirmButton=nullptr;
    GachaWalletLabel=nullptr;GachaPityLabel=nullptr;GachaPityBar=nullptr;
    auto* Hud=AcquisitionHud(*this);
    const bool Available=Hud&&Hud->HasActiveGachaPresentation();
    const auto* Banner=Available?&Hud->GetActiveGachaBanner():nullptr;
    const FLinearColor Main(.92f,.91f,.94f,1),GachaMuted(.59f,.59f,.65f,1),Gold(.72f,.62f,.44f,1);
    auto Place=[this](UWidget* W,float X,float Y,float Width,float Height,int32 Z=0)
    {
        auto* Slot=Gate10PortraitGachaCanvas->AddChildToCanvas(W);
        Slot->SetAnchors(FAnchors(X/864.f,Y/1536.f,(X+Width)/864.f,(Y+Height)/1536.f));
        Slot->SetOffsets(FMargin(0)); Slot->SetZOrder(Z);
    };
    auto Text=[this,&Place](const FString& Value,float X,float Y,float W,float H,int32 Size,FLinearColor Color,bool Display=false)
    {
        auto* Label=WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Value)); Label->SetFont(AcquisitionFont(Size,Display));
        Label->SetColorAndOpacity(Color);Label->SetAutoWrapText(true);Label->SetJustification(ETextJustify::Center);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Label,X,Y,W,H);return Label;
    };
    auto Action=[this,&Place](const FString& Caption,const FString& Id,float X,float Y,float W,float H,bool Enabled,
        EOGNativeSurface Kind=EOGNativeSurface::Action,int32 Z=0)
    {
        auto* Button=WidgetTree->ConstructWidget<UOGGate12RulerActionButton>();
        Button->RulerWidget=this;Button->ActionId=Id;Button->SetIsEnabled(Enabled);
        Button->OnClicked.AddDynamic(Button,&UOGGate12RulerActionButton::InvokeAction);
        FOGNativeUiStyle::SkinButton(*Button);
        auto* Label=WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Caption));Label->SetFont(AcquisitionFont(Kind==EOGNativeSurface::PrimaryAction?27:19));
        Label->SetColorAndOpacity(FLinearColor(.92f,.91f,.94f,1));Label->SetJustification(ETextJustify::Center);Label->SetAutoWrapText(true);
        if (auto* Slot=Cast<UButtonSlot>(Button->AddChild(Label)))
        { Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(10,6)); }
        Place(FOGNativeUiStyle::Surface(*WidgetTree,Button,Kind),X,Y,W,H,Z);return Button;
    };
    FOGPlayerProfileSettings Profile;FString ProfileError;
    if (!FOGPlayerProfileSettingsService::Load(FOGPlayerProfileSettingsService::DefaultProfilePath(),Profile,ProfileError))
        Profile.bSfwPresentation=true;
    if (Profile.bReducedMotion && GachaRevealEndsAt>0.0) FinishGachaReveal();
    auto Artwork=[this,&Profile](const FOGContentId& Identity,const FOGContentId& Version)->UWidget*
    {
        const auto* Visual=VisualCatalog?VisualCatalog->FindExact(Identity,Version):nullptr;
        if (!Visual) return nullptr;
        const auto& Source=Profile.bSfwPresentation?Visual->ApprovedSfwArtwork2D:Visual->OriginalArtwork2D;
        auto* Texture=Source.IsNull()?nullptr:Source.LoadSynchronous(); if (!Texture) return nullptr;
        auto* Image=WidgetTree->ConstructWidget<UImage>(); Image->SetBrushFromTexture(Texture,true);
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* Fit=WidgetTree->ConstructWidget<UScaleBox>(); Fit->SetStretch(EStretch::ScaleToFit);
        Fit->AddChild(Image);Fit->SetVisibility(ESlateVisibility::HitTestInvisible);return Fit;
    };
    auto Name=[Session](const FOGContentId& Id)
    {
        const auto* Identity=Session->OwnedRoster.FindByPredicate([&](const auto& I){return I.IdentityId==Id;});
        return Identity&&!Identity->DisplayNameKey.IsEmpty()?Identity->DisplayNameKey:Id.ToString();
    };
    // Retain the approved portrait anchors, draw locations and five-destination navigation.
    auto* Stage=WidgetTree->ConstructWidget<USizeBox>();
    Place(FOGNativeUiStyle::Surface(*WidgetTree,Stage,EOGNativeSurface::Hero),28,235,808,788);
    const FOGGachaPoolEntry* Featured=Banner?Banner->Entries.FindByPredicate([](const auto& E){return E.bFeatured&&E.Weight>0;}):nullptr;
    if (!Featured&&Banner) Featured=Banner->Entries.FindByPredicate([](const auto& E){return E.Weight>0;});
    if (Featured)
        if (auto* Art=Artwork(Featured->IdentityId,Featured->VersionId)) Place(Art,70,260,724,550);
    Text(TEXT("ACQUISITION"),50,48,550,52,32,Main,true);
    Text(Banner?FOGInstalledGachaBannerLibrary::DisplayName(Banner->BannerId):TEXT("NO ACTIVE BANNER"),
        90,812,684,112,35,Main,true);
    if (Featured) Text(Name(Featured->IdentityId),100,754,664,48,21,Main);
    GachaWalletLabel=Text(TEXT(""),45,115,600,88,18,GachaMuted);
    Action(TEXT("RATES"),TEXT("gacha-rates"),686,132,70,88,Available);
    Action(TEXT("HISTORY"),TEXT("gacha-history-toggle"),761,132,94,88,true);
    GachaPityLabel=Text(TEXT(""),65,930,734,60,17,GachaMuted);
    GachaPityBar=WidgetTree->ConstructWidget<UProgressBar>();
    GachaPityBar->SetFillColorAndOpacity(Gold);GachaPityBar->SetVisibility(ESlateVisibility::HitTestInvisible);
    Place(GachaPityBar,90,1000,684,4);
    TArray<FOGGachaBannerDefinition> Installed; FString Error;
    VisibleGachaBannerIds.Reset();
    GachaNextScheduleTick = -1;
    if (const auto* Core = GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>(); Core && Core->IsCoreReady())
    {
        FOGInstalledGachaBannerLibrary::Collect(
            Core->GetCanonicalWorldTick(), Installed, Error, &GachaNextScheduleTick);
        for (const auto& InstalledBanner : Installed)
        {
            VisibleGachaBannerIds.Add(InstalledBanner.BannerId);
        }
    }
    Action(TEXT("PREVIOUS"),TEXT("gacha-banner-previous"),68,1038,242,80,Installed.Num()>1);
    const int32 Index=Banner?Installed.IndexOfByPredicate([&](const auto& B){return B.BannerId==Banner->BannerId;}):INDEX_NONE;
    Text(Index==INDEX_NONE?TEXT("NO BANNER"):FString::Printf(TEXT("%d / %d"),Index+1,Installed.Num()),332,1055,242,54,21,GachaMuted);
    Action(TEXT("NEXT"),TEXT("gacha-banner-next"),595,1038,243,80,Installed.Num()>1);
    for (int32 I=0;I<2;++I)
    {
        const float X=I==0?50.f:511.f;
        GachaDrawButtons.Add(Action(I==0?TEXT("DRAW x1"):TEXT("DRAW x10"),
            I==0?TEXT("gacha-draw-1"):TEXT("gacha-draw-10"),X,1170,309,100,false,EOGNativeSurface::PrimaryAction));
        GachaCostLabels.Add(Text(TEXT(""),X-10,1119,329,49,17,GachaMuted));
    }
    Action(TEXT("DETAILS"),TEXT("gacha-rates"),151,1289,223,50,Available);
    if (!GachaSavedResults.IsEmpty()) Action(TEXT("LAST RESULTS"),TEXT("gacha-results"),488,1289,227,50,true);
    if (!GachaFeedback.IsEmpty()) Text(GachaFeedback,45,1341,774,48,17,GachaMuted);
    static const TCHAR* Destinations[]={TEXT("HOME"),TEXT("CHARACTERS"),TEXT("GACHA"),TEXT("TERRITORY"),TEXT("RECORDS")};
    for (int32 I=0;I<5;++I)
    {
        auto* B=Action(Destinations[I],FString::Printf(TEXT("nav/%d"),I),10+171.f*I,1393,167,111,true,EOGNativeSurface::Navigation);
        if (auto* Overlay=Cast<UOverlay>(B->GetParent()))
            if (auto* Chrome=Cast<UOGGate12GothicChromeWidget>(Overlay->GetChildAt(0)))
                Chrome->Configure(Gold,I,I==2);
        if (auto* Caption=Cast<UTextBlock>(B->GetChildAt(0))) Caption->SetFont(AcquisitionFont(I==1||I==3?14:17));
        if (auto* S=Cast<UButtonSlot>(B->GetChildAt(0)->Slot)) { S->SetVerticalAlignment(VAlign_Bottom);S->SetPadding(FMargin(2,0,2,10)); }
    }
    const bool Review=PendingGachaRequestId.IsValid();
    if (Review||bGachaRatesExpanded||bGachaHistoryExpanded||bGachaResultsExpanded)
    {
        // A real input shield consumes taps outside the modal. It has no purchase handler.
        auto* Shield=WidgetTree->ConstructWidget<UButton>();FOGNativeUiStyle::SkinButton(*Shield);
        auto* Veil=WidgetTree->ConstructWidget<UBorder>();Veil->SetBrushColor(FLinearColor(0,0,0,.8f));
        Shield->AddChild(Veil);Place(Shield,0,0,864,1536,20);
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
        auto* Rows=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Rows);
        auto* ModalPadding=WidgetTree->ConstructWidget<UBorder>();ModalPadding->SetBrushColor(FLinearColor::Transparent);
        ModalPadding->SetPadding(FMargin(22,100,22,176));ModalPadding->AddChild(Scroll);
        Place(FOGNativeUiStyle::Surface(*WidgetTree,ModalPadding,EOGNativeSurface::Panel),58,250,748,1010,21);
        Action(TEXT("CLOSE"),TEXT("gacha-overlay-close"),614,272,156,64,true,EOGNativeSurface::Action,22);
        if (Review)
        {
            AddMessage(Rows,TEXT("CONFIRM ACQUISITION"),28,Main);
            AddMessage(Rows,FString::Printf(TEXT("%d draw%s"),PendingGachaReview.Count,PendingGachaReview.Count==1?TEXT(""):TEXT("s")),22,Main);
            for (const auto& Line:PendingGachaReview.Payments)
                if (Line.Spend>0)
                {
                    const auto* Ticket=Banner?Banner->CompatibleMultiPullTickets.FindByPredicate(
                        [&](const auto& Row){return Row.ResourceId==Line.ResourceId;}):nullptr;
                    const FString PaymentName=Ticket?Ticket->DisplayName.ToString():Line.ResourceId.ToString();
                    AddMessage(Rows,FString::Printf(TEXT("%s\n%lld  ->  %lld   (cost %lld)"),*PaymentName,
                        Line.Balance,Line.Balance-Line.Spend,Line.Spend),18,GachaMuted);
                }
            if (!PendingGachaReview.Payments.ContainsByPredicate([](const auto& L){return L.Spend>0;}))
                AddMessage(Rows,TEXT("No currency required."),20,Main);
            AddMessage(Rows,TEXT("Every acquired duplicate is an independent owned Manifestation."),18,GachaMuted);
            GachaReviewStatus=WidgetTree->ConstructWidget<UTextBlock>();
            GachaReviewStatus->SetAutoWrapText(true);GachaReviewStatus->SetFont(AcquisitionFont(18));GachaReviewStatus->SetColorAndOpacity(Muted);
            Rows->AddChildToVerticalBox(GachaReviewStatus)->SetPadding(FMargin(0,20));
            GachaConfirmButton=Action(TEXT("CONFIRM DRAW"),TEXT("gacha-confirm/")+PendingGachaRequestId.ToString(),
                108,1138,648,88,true,EOGNativeSurface::PrimaryAction,22);
            const FString ReviewAgain=PendingGachaReview.BatchTicketId.IsValid()
                ?TEXT("gacha-ticket/")+PendingGachaReview.BatchTicketId.ToString()
                :(PendingGachaReview.Count==10?TEXT("gacha-review-10"):TEXT("gacha-review-1"));
            Action(TEXT("REVIEW AGAIN"),ReviewAgain,108,1048,648,64,true,EOGNativeSurface::Action,22);
        }
        else if (bGachaRatesExpanded&&Banner)
        {
            AddMessage(Rows,TEXT("BANNER DETAILS"),28,Main);
            AddMessage(Rows,FOGInstalledGachaBannerLibrary::DisplayName(Banner->BannerId),20,Main);
            AddMessage(Rows,FString::Printf(TEXT("Soft pity starts at %d. Hard pity: %d.\nTop tier: %s"),
                Banner->SoftPityStart,Banner->HardPity,*Banner->TopRarity.ToString()),18,GachaMuted);
            if (!Banner->CompatibleMultiPullTickets.IsEmpty())
            {
                AddMessage(Rows,TEXT("COMPATIBLE MULTI-PULL TICKETS"),20,Main);
                auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
                auto* Store=Core?Core->GetWorldStore():nullptr;
                if (Store && Store->IsOpen())
                    for (const auto& Ticket:Banner->CompatibleMultiPullTickets)
                    {
                        FOGGachaBatchPreview Quote;
                        FString TicketError;
                        if (!FOGGachaService(*Store).PreviewBatch(*Banner,Session->BoundRulerId,
                            Core->GetCanonicalWorldTick(),Ticket.PullCount,Quote,TicketError,Ticket.ResourceId)) continue;
                        const FString Caption=Ticket.DisplayName.ToString()+FString::Printf(TEXT(" / %d draws"),Ticket.PullCount);
                        const int64 Balance=Quote.Payments.IsEmpty()?0:Quote.Payments[0].Balance;
                        if (Quote.bCanPull)
                            AddAction(Rows,Caption+FString::Printf(TEXT(" / %lld owned"),Balance),
                                TEXT("gacha-ticket/")+Ticket.ResourceId.ToString());
                        else
                            AddMessage(Rows,Caption+TEXT(" / ")+FString::Join(Quote.MissingRequirements,TEXT(" ")),17,GachaMuted);
                    }
            }
            AddMessage(Rows,TEXT("Source pool weights below are relative selection weights, not final draw percentages. The live pity panel shows the next top-rarity chance."),17,GachaMuted);
            for (const auto& E:Banner->Entries)
                AddMessage(Rows,FString::Printf(TEXT("%s  |  %s  |  weight %d%s"),
                    *Name(E.IdentityId),*E.Rarity.ToString(),E.Weight,E.bFeatured?TEXT("  |  Featured"):TEXT("")),18,Main);
        }
        else if (bGachaHistoryExpanded)
        {
            AddMessage(Rows,TEXT("ACQUISITION HISTORY"),28,Main);
            if (!Session->RefreshGachaHistory(Error)) AddMessage(Rows,Error,18,GachaMuted);
            else if (Session->GachaHistoryProjection.IsEmpty()) AddMessage(Rows,TEXT("No recorded acquisitions."),20,GachaMuted);
            else for (const auto& E:Session->GachaHistoryProjection)
                AddMessage(Rows,FString::Printf(TEXT("%s  |  %s  |  tick %lld"),*Name(E.IdentityId),*E.Rarity.ToString(),E.WorldTick),18,Main);
        }
        else if (bGachaResultsExpanded&&!GachaSavedResults.IsEmpty())
        {
            const bool Playing=GachaRevealEndsAt>0.0;
            const auto& Results=GachaSavedResults;
            AddMessage(Rows,Playing?TEXT("SUMMON RESULTS"):TEXT("ACQUISITION COMPLETE"),28,Main);
            if (Playing)
                AddMessage(Rows,FString::Printf(TEXT("%d / %d"),FMath::Min(
                    GachaRevealedResults+(bGachaRevealCue?1:0),Results.Num()),Results.Num()),19,GachaMuted);
            else AddMessage(Rows,TEXT("Saved to your roster."),18,GachaMuted);
            if (Playing && bGachaRevealCue && Results.IsValidIndex(GachaRevealedResults))
            {
                const auto& R=Results[GachaRevealedResults];
                auto* Cue=WidgetTree->ConstructWidget<UVerticalBox>();
                auto* Glow=WidgetTree->ConstructWidget<UBorder>();
                const FLinearColor Accent=AcquisitionRarity(R.Rarity);
                Glow->SetBrushColor(FLinearColor(Accent.R,Accent.G,Accent.B,.25f));
                Glow->SetPadding(FMargin(22.f,108.f));
                Glow->AddChild(Cue);
                AddMessage(Cue,R.bTopRarity?TEXT("A RADIANT MANIFESTATION"):TEXT("A MANIFESTATION APPROACHES"),25,Main);
                GachaAnimatedReveal=FOGNativeUiStyle::Surface(*WidgetTree,Glow,EOGNativeSurface::Hero);
                GachaAnimatedReveal->SetRenderOpacity(.22f);
                Rows->AddChildToVerticalBox(GachaAnimatedReveal)->SetPadding(FMargin(0,28));
            }
            const int32 First=Playing?FMath::Max(0,GachaRevealedResults-1):0;
            const int32 Last=Playing && bGachaRevealCue?First:FMath::Min(GachaRevealedResults,Results.Num());
            for (int32 I=First;I<Last;++I)
            {
                const auto& R=Results[I];
                auto* Card=WidgetTree->ConstructWidget<UVerticalBox>();
                if (auto* Art=Artwork(R.IdentityId,R.VersionId))
                {
                    auto* Bounds=WidgetTree->ConstructWidget<USizeBox>();
                    Bounds->SetHeightOverride(340); Bounds->AddChild(Art); Card->AddChildToVerticalBox(Bounds);
                }
                auto* Tier=WidgetTree->ConstructWidget<UTextBlock>();Tier->SetText(FText::FromName(R.Rarity));
                Tier->SetColorAndOpacity(AcquisitionRarity(R.Rarity));Tier->SetFont(AcquisitionFont(24,true));
                if (auto* Style=UOGReactiveUiSubsystem::Find(this)) Style->PreserveOriginal(Tier);
                Card->AddChildToVerticalBox(Tier);
                AddMessage(Card,Name(R.IdentityId),23,Main);
                AddMessage(Card,R.bDuplicateIdentity?TEXT("INDEPENDENT COPY"):TEXT("NEW IDENTITY"),17,GachaMuted);
                if (!Playing && R.ManifestationId.IsValid())
                    AddAction(Card,TEXT("OPEN OWNED COPY"),TEXT("gacha-copy/")+R.ManifestationId.ToString());
                auto* Surface=FOGNativeUiStyle::Surface(*WidgetTree,Card,EOGNativeSurface::Hero);
                Rows->AddChildToVerticalBox(Surface)->SetPadding(FMargin(0,12));
                if (Playing) { GachaAnimatedReveal=Surface; GachaAnimatedReveal->SetRenderOpacity(.22f); }
            }
            if (Playing)
            {
                Action(TEXT("REVEAL NEXT"),TEXT("gacha-reveal-next"),100,1144,310,78,true,EOGNativeSurface::Action,22);
                Action(TEXT("SKIP TO RESULTS"),TEXT("gacha-reveal-all"),454,1144,310,78,true,EOGNativeSurface::PrimaryAction,22);
            }
        }
    }
    bGachaDirty=true;SynchronizeGachaUi();
}
void UOGGate12NativeRulerWidget::HandleGachaAction(const FString& Action,UOGGate12RulerSessionSubsystem& Session)
{
    if (bGachaCommandInFlight||Session.CurrentDestination!=EOGRulerPrimaryDestination::Gacha||!Session.IsBoundToStoredRuler()) return;
    auto* Hud=AcquisitionHud(*this);FString Error;
    if (!Hud) {GachaFeedback=TEXT("Acquisition is unavailable.");BuildCurrentPage();return;}
    if (Action==TEXT("gacha-banner-next")||Action==TEXT("gacha-banner-previous"))
    {
        ClearGachaReview();bGachaRatesExpanded=bGachaHistoryExpanded=bGachaResultsExpanded=false;
        if (!Hud->StepInstalledGachaBanner(Action==TEXT("gacha-banner-next")?1:-1,Error)) GachaFeedback=Error;
    }
    else if (Action==TEXT("gacha-draw-1")||Action==TEXT("gacha-draw-10")||
        Action==TEXT("gacha-review-1")||Action==TEXT("gacha-review-10"))
    {
        const int32 Count=Action.EndsWith(TEXT("-10"))?10:1;
        ClearGachaReview();bGachaRatesExpanded=bGachaHistoryExpanded=bGachaResultsExpanded=false;
        auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
        auto* Store=Core?Core->GetWorldStore():nullptr;
        FOGGachaBatchPreview Quote;
        if (Hud->HasActiveGachaPresentation()&&Store&&
            FOGGachaService(*Store).PreviewBatch(Hud->GetActiveGachaBanner(),Session.BoundRulerId,Core->GetCanonicalWorldTick(),Count,Quote,Error)&&Quote.bCanPull)
        { PendingGachaReview=MoveTemp(Quote);PendingGachaRequestId=FOGEntityId::NewId();GachaFeedback.Reset(); }
        else GachaFeedback=Error.IsEmpty()?FString::Join(Quote.MissingRequirements,TEXT(" ")):Error;
    }
    else if (Action.StartsWith(TEXT("gacha-ticket/")))
    {
        ClearGachaReview(); bGachaRatesExpanded=bGachaHistoryExpanded=bGachaResultsExpanded=false;
        const FOGContentId Id(Action.Mid(UE_ARRAY_COUNT(TEXT("gacha-ticket/"))-1));
        const auto* Banner=Hud->HasActiveGachaPresentation()?&Hud->GetActiveGachaBanner():nullptr;
        const auto* Ticket=Banner?Banner->CompatibleMultiPullTickets.FindByPredicate([&](const auto& Row){return Row.ResourceId==Id;}):nullptr;
        auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
        auto* Store=Core?Core->GetWorldStore():nullptr;
        FOGGachaBatchPreview Quote;
        if (Ticket && Store && FOGGachaService(*Store).PreviewBatch(*Banner,Session.BoundRulerId,
            Core->GetCanonicalWorldTick(),Ticket->PullCount,Quote,Error,Id) && Quote.bCanPull)
        { PendingGachaReview=MoveTemp(Quote); PendingGachaRequestId=FOGEntityId::NewId(); GachaFeedback.Reset(); }
        else GachaFeedback=Error.IsEmpty()?(Quote.MissingRequirements.IsEmpty()?FString(TEXT("This ticket is unavailable.")):
            FString::Join(Quote.MissingRequirements,TEXT(" "))):Error;
    }
    else if (Action.StartsWith(TEXT("gacha-confirm/")))
    {
        if (!PendingGachaRequestId.IsValid()||Action!=TEXT("gacha-confirm/")+PendingGachaRequestId.ToString()||
            PendingGachaReview.RulerId!=Session.BoundRulerId) return;
        TGuardValue<bool> Guard(bGachaCommandInFlight,true);
        const auto Review=PendingGachaReview;ClearGachaReview(); // Consume the UI request before any commit callback.
        if (Hud->ExecuteReviewedGachaDraw(Review.Count,Review.ConfirmationToken,Error,Review.BatchTicketId))
        {
            bGachaRatesExpanded=bGachaHistoryExpanded=false;bGachaResultsExpanded=true;
            GachaSavedResults=Hud->GetLastGachaResults();
            GachaRevealedResults=0;
            bGachaRevealCue=!GachaSavedResults.IsEmpty();
            GachaRevealEndsAt=bGachaRevealCue?FPlatformTime::Seconds()+GachaCueSeconds:0.0;
            GachaFeedback=TEXT("Acquisition saved.");
        }
        else GachaFeedback=Error;
    }
    else if (Action==TEXT("gacha-reveal-next") && GachaRevealEndsAt>0.0)
    {
        if (GachaRevealedResults<GachaSavedResults.Num())
        {
            ++GachaRevealedResults;
            bGachaRevealCue=false;
            GachaRevealEndsAt=FPlatformTime::Seconds()+GachaCardSeconds;
        }
        else FinishGachaReveal();
    }
    else if (Action==TEXT("gacha-reveal-all")) FinishGachaReveal();
    else if (Action.StartsWith(TEXT("gacha-copy/")) && bGachaResultsExpanded && GachaRevealEndsAt<=0.0)
    {
        FGuid Copy;
        if (!FGuid::Parse(Action.Mid(UE_ARRAY_COUNT(TEXT("gacha-copy/"))-1),Copy)) return;
        const auto* Result=GachaSavedResults.FindByPredicate([&](const auto& R){return R.ManifestationId==FOGEntityId(Copy);});
        if (!Result || !Session.SelectOwnedManifestation(Result->IdentityId,Result->ManifestationId,Error))
            GachaFeedback=Error.IsEmpty()?TEXT("This result is no longer selectable."):Error;
    }
    else if (Action==TEXT("gacha-results")&&!GachaSavedResults.IsEmpty()) {ClearGachaReview();bGachaRatesExpanded=bGachaHistoryExpanded=false;bGachaResultsExpanded=true;}
    else if (Action==TEXT("gacha-rates")) {ClearGachaReview();bGachaRatesExpanded=!bGachaRatesExpanded;bGachaHistoryExpanded=bGachaResultsExpanded=false;}
    else if (Action==TEXT("gacha-history-toggle")) {ClearGachaReview();bGachaHistoryExpanded=!bGachaHistoryExpanded;bGachaRatesExpanded=bGachaResultsExpanded=false;}
    else if (Action==TEXT("gacha-overlay-close"))
    {ClearGachaReview();FinishGachaReveal();bGachaRatesExpanded=bGachaHistoryExpanded=bGachaResultsExpanded=false;}
    else return;
    bGachaDirty=true;BuildCurrentPage();
}
