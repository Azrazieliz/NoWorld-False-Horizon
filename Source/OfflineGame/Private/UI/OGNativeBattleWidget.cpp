#include "UI/OGNativeBattleWidget.h"
#include "World/OGStartingRegionPresentation.h"
#include "Combat/OGNativeBattleCommands.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "EngineUtils.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
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
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"

namespace
{
FSlateFontInfo BattleFont(int32 Size,bool Title=false)
{
    return FSlateFontInfo(Title?FPaths::ProjectContentDir()/TEXT("Slate/Fonts/Cinzel-Bold.ttf"):
        FPaths::EngineContentDir()/TEXT("Slate/Fonts/Roboto-Medium.ttf"),static_cast<float>(Size));
}
const FLinearColor Gold(.82f,.73f,.57f,1);
const FLinearColor Pale(.93f,.91f,.95f,1);
}
void UOGNativeBattleButton::Invoke() { if (Screen) Screen->Activate(Command,TargetIndex); }
bool UOGNativeBattleWidget::IsForCharacter(const AOGWorldPrototypeCharacter* InCharacter) const { return Character.Get()==InCharacter; }
void UOGNativeBattleWidget::BindCharacter(AOGWorldPrototypeCharacter* InCharacter)
{
    if (Character.Get()==InCharacter) return;
    Character=InCharacter; Signature.Reset(); LastActionMessage.Reset(); Refresh();
}
void UOGNativeBattleWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!WidgetTree || WidgetTree->RootWidget) return;
    auto* Safe=WidgetTree->ConstructWidget<USafeZone>();
    Safe->SetVisibility(ESlateVisibility::SelfHitTestInvisible); WidgetTree->RootWidget=Safe;
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(); Safe->AddChild(Canvas);
    Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    auto Panel=[&](float Top,float Bottom,UCanvasPanelSlot*& Out)
    {
        auto* Frame=WidgetTree->ConstructWidget<UBorder>();
        Frame->SetBrushColor(FLinearColor(.011f,.008f,.024f,.94f));
        Frame->SetPadding(FMargin(16,12)); Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Out=Canvas->AddChildToCanvas(Frame); Out->SetAnchors(FAnchors(.02f,Top,.98f,Bottom));
        Out->SetOffsets(FMargin(0)); return Frame;
    };
    UCanvasPanelSlot* HeaderSlot=nullptr; auto* Header=Panel(.02f,.25f,HeaderSlot);
    auto* Column=WidgetTree->ConstructWidget<UVerticalBox>(); Header->AddChild(Column);
    Heading=WidgetTree->ConstructWidget<UTextBlock>(); Heading->SetFont(BattleFont(22,true));
    Heading->SetColorAndOpacity(Gold); Heading->SetAutoWrapText(true); Column->AddChildToVerticalBox(Heading);
    Resources=WidgetTree->ConstructWidget<UTextBlock>(); Resources->SetFont(BattleFont(16));
    Resources->SetColorAndOpacity(Pale); Resources->SetAutoWrapText(true);
    Column->AddChildToVerticalBox(Resources)->SetPadding(FMargin(0,4,0,8));
    Targets=WidgetTree->ConstructWidget<UScrollBox>(); Targets->SetOrientation(Orient_Horizontal);
    Column->AddChildToVerticalBox(Targets)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UCanvasPanelSlot* LowerSlot=nullptr; auto* Lower=Panel(.61f,.98f,LowerSlot); ActionPanelSlot=LowerSlot;
    auto* LowerColumn=WidgetTree->ConstructWidget<UVerticalBox>(); Lower->AddChild(LowerColumn);
    Feedback=WidgetTree->ConstructWidget<UTextBlock>(); Feedback->SetFont(BattleFont(16));
    Feedback->SetColorAndOpacity(Gold); Feedback->SetAutoWrapText(true);
    LowerColumn->AddChildToVerticalBox(Feedback)->SetPadding(FMargin(0,0,0,8));
    ActionScroll=WidgetTree->ConstructWidget<UScrollBox>();
    LowerColumn->AddChildToVerticalBox(ActionScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Commands=WidgetTree->ConstructWidget<UWrapBox>(); Commands->SetExplicitWrapSize(true);
    Commands->SetInnerSlotPadding(FVector2D(8,8)); ActionScroll->AddChild(Commands);
    Refresh();
}
void UOGNativeBattleWidget::AddButton(UPanelWidget* Parent,const FString& Label,const FString& Hint,
    FName Command,int32 TargetIndex,bool Enabled,bool Selected,float Width)
{
    auto* Bounds=WidgetTree->ConstructWidget<USizeBox>();
    Bounds->SetWidthOverride(Width); Bounds->SetMinDesiredHeight(68);
    Parent->AddChild(Bounds);
    auto* Button=WidgetTree->ConstructWidget<UOGNativeBattleButton>();
    Button->Screen=this; Button->Command=Command; Button->TargetIndex=TargetIndex;
    Button->SetIsFocusable(false); Button->SetIsEnabled(Enabled); Button->SetToolTipText(FText::FromString(Hint));
    Button->SetBackgroundColor(Selected?FLinearColor(.26f,.17f,.31f,1):FLinearColor(.07f,.045f,.105f,1));
    Button->OnClicked.AddDynamic(Button,&UOGNativeBattleButton::Invoke); Bounds->AddChild(Button);
    auto* Text=WidgetTree->ConstructWidget<UTextBlock>(); Text->SetFont(BattleFont(17));
    Text->SetAutoWrapText(true); Text->SetText(FText::FromString(Label));
    Text->SetColorAndOpacity(Selected?Gold:Enabled?Pale:FLinearColor(.49f,.47f,.53f,1));
    if (auto* Slot=Cast<UButtonSlot>(Button->AddChild(Text)))
    { Slot->SetPadding(FMargin(12,10)); Slot->SetVerticalAlignment(VAlign_Center); }
}
void UOGNativeBattleWidget::Refresh()
{
    if (!Heading || !Character.IsValid()) return;
    auto* Body=Character.Get();
    const auto* GI=Body->GetGameInstance();
    const auto* Core=GI?GI->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    const auto* Session=GI?GI->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    const auto* Battle=Core?Core->GetOwnedLuminousTurnBattle():nullptr;
    if (!Core || !Core->IsCoreReady() || !Battle || !Session ||
        Core->GetOwnedBattleRulerId()!=Session->BoundRulerId ||
        Battle->GetState().Status!=EOGTurnBattleStatus::Running)
    { SetVisibility(ESlateVisibility::Collapsed); return; }
    const auto& State=Battle->GetState();
    const auto* Source=State.Units.FindByPredicate([Session](const FOGCombatUnitState& U)
        { return U.UnitEntityId==Session->SelectedManifestationId; });
    if (!Source) { SetVisibility(ESlateVisibility::Collapsed); return; }
    int32 Resource=0,Charge=0,OwnerActions=0;
    Battle->ReadLuminousCommandResources(Source->UnitEntityId,Resource,Charge,OwnerActions);
    FString Key=State.BattleId.ToString()+Source->UnitEntityId.ToString()+
        FString::Printf(TEXT("|%lld|%d|%d|%lld|%d|%d|"),static_cast<long long>(Battle->GetLog().GetNextSequence()),
            Resource,Charge,static_cast<long long>(Core->GetCanonicalWorldTick()/1000),
            FMath::RoundToInt(ViewWidth),FMath::RoundToInt(ViewHeight))+LastActionMessage;
    for (const auto& Id:Body->GetSelectedCanonicalTurnTargets()) Key+=Id.ToString();
    if (Body->HasTargetHudProjectionForHud()) Key+=Body->GetTargetHudProjectionForHud().TargetEntityId.ToString();
    if (Key==Signature) return;
    Signature=Key;
    FOGNativeBattleCommandSet Set; FString Error;
    if (!Body->ReadOwnedLuminousBattleCommands(Set,Error))
    { Heading->SetText(FText::FromString(TEXT("ENCOUNTER"))); Feedback->SetText(FText::FromString(Error)); Commands->ClearChildren(); return; }
    const int32 Acting=Battle->SelectNextActingUnitIndex();
    const bool Own=State.Units.IsValidIndex(Acting) && State.Units[Acting].UnitEntityId==Source->UnitEntityId &&
        Source->CanAct() && Body->IsFoundationWorldInputAllowed();
    Heading->SetText(FText::FromString(Own?TEXT("YOUR TURN"):TEXT("OPPONENT / ALLY TURN")));
    Resources->SetText(FText::FromString(FString::Printf(TEXT("HP %s / %s   |   %s %d / %d   |   Ultimate %d%% / %d%%"),
        *Source->CurrentHp.ToDebugString(),*Source->Stats.MaxHp.ToDebugString(),*Set.ResourceLabel,Resource,Set.ResourceCap,Charge,Set.ChargeCap)));
    Feedback->SetText(FText::FromString(LastActionMessage.IsEmpty()?
        TEXT("Choose a primary target. Area effects include every eligible field participant."):LastActionMessage));
    const float TargetOffset=Targets->GetScrollOffset(),ActionOffset=ActionScroll->GetScrollOffset();
    Targets->ClearChildren(); Commands->ClearChildren();
    // One actor pass per refresh; expose only existing native battle bodies.
    TMap<FOGEntityId,AActor*> Bodies; TSet<FOGEntityId> AmbiguousBodies;
    for (TActorIterator<AActor> It(GetWorld());It;++It)
    {
        AActor* Actor=*It; if (!IsValid(Actor)) continue;
        const auto* Binding=Actor->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
        if (!Binding || Binding->GetBoundBattleId()!=State.BattleId || !Binding->GetBoundUnitId().IsValid()) continue;
        const auto Id=Binding->GetBoundUnitId();
        if (Bodies.Contains(Id)) AmbiguousBodies.Add(Id);
        else Bodies.Add(Id,Actor);
    }
    if (!Bodies.Contains(Source->UnitEntityId)) Bodies.Add(Source->UnitEntityId,Body);
    for (int32 I=0;I<State.Units.Num();++I)
    {
        const auto& Unit=State.Units[I]; if (Unit.Presence!=EOGCombatPresence::Active) continue;
        const bool Selected=Body->IsOwnedBattleTargetSelected(Unit.UnitEntityId);
        const FString Side=Unit.UnitEntityId==Source->UnitEntityId?TEXT("SELF"):Unit.TeamIndex==Source->TeamIndex?TEXT("ALLY"):TEXT("OPPONENT");
        const bool Embodied=Bodies.Contains(Unit.UnitEntityId) && !AmbiguousBodies.Contains(Unit.UnitEntityId);
        const FString Suffix=!Embodied?TEXT(" / CHARACTER UNAVAILABLE"):Selected?TEXT(" / SELECTED"):TEXT("");
        AddButton(Targets,FString::Printf(TEXT("%s %d%s"),*Side,I+1,*Suffix),
            TEXT("Select this actual battle participant."),NAME_None,I,Own && Unit.CanAct() && Embodied,Selected,176);
    }
    const bool Portrait=ViewWidth<ViewHeight;
    const float Available=FMath::Max(200.f,ViewWidth*.96f-32.f);
    Commands->SetWrapSize(Available);
    const float Width=Portrait?Available:FMath::Max(180.f,(Available-16.f)/3.f);
    TArray<FOGEntityId> Selected=Body->GetSelectedCanonicalTurnTargets();
    if (Selected.IsEmpty() && Body->HasTargetHudProjectionForHud()) Selected.Add(Body->GetTargetHudProjectionForHud().TargetEntityId);
    for (const auto& Command:Set.Commands)
    {
        FOGResolvedCombatAction Proposal,Expanded; Command.BuildAction(*Source,Selected,Proposal);
        FString Reason; bool Enabled=Own && Battle->ValidateResolvedAction(Proposal,Expanded,Reason);
        if (Enabled) for (const auto& Id:Expanded.TargetUnitIds)
            if (!Bodies.Contains(Id) || AmbiguousBodies.Contains(Id))
            { Enabled=false; Reason=TEXT("Waiting for the battlefield characters."); break; }
        if (!Own) Reason=TEXT("Available on this character's next turn.");
        if (Own && !Enabled && Expanded.TargetUnitIds.IsEmpty()) Reason=TEXT("Select a primary target.");
        const FString Label=Command.Label+(Own && !Enabled?TEXT("\n")+Reason:FString());
        AddButton(Commands,Label,Enabled?Command.Hint:Reason,Command.Id,INDEX_NONE,Enabled,false,Width);
    }
    AddButton(Commands,TEXT("Pause"),TEXT("Open the existing pause controls."),TEXT("screen_pause"),INDEX_NONE,true,false,Width);
    AddButton(Commands,TEXT("Settings"),TEXT("Open the existing game settings."),TEXT("screen_settings"),INDEX_NONE,true,false,Width);
    AddButton(Commands,TEXT("Ruler"),TEXT("Return to the existing Ruler interface."),TEXT("screen_ruler"),INDEX_NONE,true,false,Width);
    Targets->SetScrollOffset(TargetOffset); ActionScroll->SetScrollOffset(ActionOffset);
}
void UOGNativeBattleWidget::Activate(FName Command,int32 TargetIndex)
{
    if (!Character.IsValid()) return;
    if (Command==TEXT("screen_pause")) { Character->SetFoundationPauseMenuOpen(true); return; }
    if (Command==TEXT("screen_settings")) { Character->SetFoundationSettingsOpen(true); return; }
    if (Command==TEXT("screen_ruler"))
    {
        if (auto* Player=Cast<APlayerController>(Character->GetController()))
            if (auto* Hud=Cast<AOGWorldPresentationHud>(Player->GetHUD())) Hud->NotifyHitBoxClick(TEXT("OG.World.Ruler"));
        return;
    }
    FString Error; const bool Accepted=TargetIndex!=INDEX_NONE?
        Character->SelectOwnedLuminousBattleTarget(TargetIndex,Error):
        Character->SubmitApprovedLuminousWorldCommand(Command,Error);
    LastActionMessage=Accepted?FString():Error;
    Signature.Reset(); Refresh();
}
void UOGNativeBattleWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    const auto Size=Geometry.GetLocalSize(); ViewWidth=Size.X; ViewHeight=Size.Y;
    if (ActionPanelSlot) ActionPanelSlot->SetAnchors(FAnchors(.02f,Size.X<Size.Y?.55f:.61f,.98f,.98f));
    RefreshElapsed+=DeltaSeconds;
    if (RefreshElapsed>=.2f) { RefreshElapsed=0; Refresh(); }
}
