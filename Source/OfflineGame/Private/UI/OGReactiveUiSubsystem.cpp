#include "UI/OGReactiveUiSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Content/OGSystemBindingComponent.h"
#include "Runtime/OGCanonicalCharacterPresentation.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGPlayerProfileSettings.h"
#include "Progression/OGRankService.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "Combat/OGTurnBattle.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool FiniteColor(const FLinearColor& C)
{ return FMath::IsFinite(C.R)&&FMath::IsFinite(C.G)&&FMath::IsFinite(C.B)&&FMath::IsFinite(C.A); }
bool MatchJson(const FString& Text,const TMap<FName,FString>& Required)
{
    if (Required.IsEmpty()) return true;
    TSharedPtr<FJsonObject> Json;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json) || !Json) return false;
    for (const auto& Pair:Required)
    {
        const auto* Field=Json->TryGetField(Pair.Key.ToString()); if (!Field || !Field->IsValid()) return false;
        FString Value;
        switch ((*Field)->Type)
        {
        case EJson::String: Value=(*Field)->AsString(); break;
        case EJson::Boolean: Value=(*Field)->AsBool()?TEXT("true"):TEXT("false"); break;
        case EJson::Number: Value=LexToString((*Field)->AsNumber()); break;
        case EJson::Null: Value=TEXT("null"); break;
        default: return false;
        }
        if (Value!=Pair.Value) return false;
    }
    return true;
}
FOGReactiveUiPalette Blend(const FOGReactiveUiPalette& A,const FOGReactiveUiPalette& B,float T)
{
    FOGReactiveUiPalette R;
    R.Accent=FMath::Lerp(A.Accent,B.Accent,T); R.Rim=FMath::Lerp(A.Rim,B.Rim,T);
    R.Surface=FMath::Lerp(A.Surface,B.Surface,T); R.Text=FMath::Lerp(A.Text,B.Text,T);
    R.Muted=FMath::Lerp(A.Muted,B.Muted,T); R.Glow=FMath::Lerp(A.Glow,B.Glow,T);
    R.MotionRate=FMath::Lerp(A.MotionRate,B.MotionRate,T); R.TransitionSeconds=B.TransitionSeconds;
    return R;
}
bool ReadWidgetColor(UWidget* W,FLinearColor& C)
{
    if (auto* Text=Cast<UTextBlock>(W))
    { if (!Text->GetColorAndOpacity().IsColorSpecified()) return false; C=Text->GetColorAndOpacity().GetSpecifiedColor(); return true; }
    if (auto* Border=Cast<UBorder>(W)) { C=Border->GetBrushColor(); return true; }
    if (auto* Button=Cast<UButton>(W)) { C=Button->GetBackgroundColor(); return true; }
    if (auto* Image=Cast<UImage>(W)) { C=Image->GetColorAndOpacity(); return true; }
    return false;
}
void WriteWidgetColor(UWidget* W,const FLinearColor& C)
{
    if (auto* Text=Cast<UTextBlock>(W)) Text->SetColorAndOpacity(FSlateColor(C));
    else if (auto* Border=Cast<UBorder>(W)) Border->SetBrushColor(C);
    else if (auto* Button=Cast<UButton>(W)) Button->SetBackgroundColor(C);
    else if (auto* Image=Cast<UImage>(W)) Image->SetColorAndOpacity(C);
}
}
bool FOGReactiveUiPalette::IsFinite() const
{
    return FiniteColor(Accent)&&FiniteColor(Rim)&&FiniteColor(Surface)&&FiniteColor(Text)&&FiniteColor(Muted)&&
        FMath::IsFinite(Glow)&&Glow>=0&&FMath::IsFinite(MotionRate)&&MotionRate>=0&&
        FMath::IsFinite(TransitionSeconds)&&TransitionSeconds>=0;
}
bool FOGReactiveUiPalette::NearlyEquals(const FOGReactiveUiPalette& O) const
{
    return Accent.Equals(O.Accent,.0001f)&&Rim.Equals(O.Rim,.0001f)&&Surface.Equals(O.Surface,.0001f)&&
        Text.Equals(O.Text,.0001f)&&Muted.Equals(O.Muted,.0001f)&&FMath::IsNearlyEqual(Glow,O.Glow,.0001f)&&
        FMath::IsNearlyEqual(MotionRate,O.MotionRate,.0001f);
}
bool FOGReactiveUiRule::Matches(const FOGReactiveUiContext& C) const
{
    const FName Movement=C.NativeMovementContext.IsNone()?C.MovementContext:C.NativeMovementContext;
    return C.bCanonicalReady && C.OwnedLevel>=MinimumOwnedLevel &&
        (RequiredRulerTitle.IsNone()||RequiredRulerTitle==C.RulerTitle) &&
        (RequiredRulerClass.IsEmpty()||C.RulerClasses.Contains(RequiredRulerClass)) &&
        (RequiredLocationKind.IsNone()||RequiredLocationKind==C.LocationKind) &&
        (RequiredMovementContext.IsNone()||RequiredMovementContext==Movement) &&
        (!bRequireCombat||C.bCombat) && MatchJson(C.RulerScopeJson,RequiredRulerScope) &&
        MatchJson(C.LocationStateJson,RequiredLocationState);
}
UOGReactiveUiThemeDefinition::UOGReactiveUiThemeDefinition() { System=TEXT("ui_theme"); }
bool UOGReactiveUiThemeDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("ui_theme")) { Error=TEXT("Native UI themes require their UI presentation system namespace."); return false; }
    TSet<int32> Priorities;
    for (const auto& Rule:Rules)
    {
        if (System!=TEXT("ui_theme") || Rule.MinimumOwnedLevel<0 || !Rule.Palette.IsFinite() || Priorities.Contains(Rule.Priority))
        { Error=TEXT("UI rules require finite visual tokens, valid selectors and distinct deterministic priorities."); return false; }
        Priorities.Add(Rule.Priority);
    }
    return true;
}
FOGReactiveUiPalette UOGReactiveUiSubsystem::ResolveDefault(const FOGReactiveUiContext& C)
{
    FOGReactiveUiPalette P;
    if (!C.bCanonicalReady) return P;
    const float Progress=FMath::Clamp(FMath::Loge(1.f+FMath::Max(0,C.OwnedLevel-1))/8.f,0.f,1.f);
    const bool Sovereign=!C.RulerTitle.IsNone()&&C.RulerTitle!=TEXT("none");
    const float Power=FMath::Clamp(C.RulerClasses.Num()*.08f+(Sovereign?.16f:0.f),0.f,.5f);
    // Pure UI art tuning: these visual curves are never rank, GP, level-cap or unlock rules.
    P.Accent=FMath::Lerp(FLinearColor(.65f,.64f,.73f,1),FLinearColor(.80f,.68f,.43f,1),
        FMath::Clamp(.35f+Progress*.4f+Power,0.f,1.f));
    const FName Movement=C.NativeMovementContext.IsNone()?C.MovementContext:C.NativeMovementContext;
    if (C.bWorldMode && Movement==TEXT("Underwater")) P.Accent=FMath::Lerp(P.Accent,FLinearColor(.28f,.71f,.79f,1),.55f);
    else if (C.bWorldMode && Movement==TEXT("Sky")) P.Accent=FMath::Lerp(P.Accent,FLinearColor(.48f,.65f,.87f,1),.4f);
    else if (C.bWorldMode && Movement==TEXT("Underground")) P.Accent=FMath::Lerp(P.Accent,FLinearColor(.76f,.53f,.35f,1),.35f);
    P.Rim=FMath::Lerp(FLinearColor(.32f,.32f,.39f,1),P.Accent,.22f+Progress*.12f);
    P.Glow=.12f+Progress*.2f+Power*.2f+(C.bCombat?.12f:0.f);
    P.MotionRate=C.bCombat?.32f:.12f+Progress*.08f;
    return P;
}
UOGReactiveUiSubsystem* UOGReactiveUiSubsystem::Find(const UObject* Object)
{
    auto* World=Object?Object->GetWorld():nullptr;
    return World&&World->IsGameWorld()?World->GetSubsystem<UOGReactiveUiSubsystem>():nullptr;
}
bool UOGReactiveUiSubsystem::IsTickable() const
{ return Super::IsTickable()&&!IsTemplate()&&GetWorld()&&GetWorld()->IsGameWorld(); }
void UOGReactiveUiSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FOGPlayerProfileSettings Profile; FString Error;
    if (FOGPlayerProfileSettingsService::Load(FOGPlayerProfileSettingsService::DefaultProfilePath(),Profile,Error))
        SetReducedMotion(Profile.bReducedMotion);
    ProfileHandle=FOGPlayerProfileSettingsService::OnSettingsChanged().AddWeakLambda(this,
        [this](const FOGPlayerProfileSettings& Changed){SetReducedMotion(Changed.bReducedMotion);});
    if (auto* Library=UOGDefinitionLibrary::Get())
        LibraryHandle=Library->OnDefinitionsChanged.AddUObject(this,&ThisClass::HandleDefinitions);
    SpawnHandle=GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this,&ThisClass::HandleActorSpawned));
}
void UOGReactiveUiSubsystem::Disconnect()
{
    if (RulerFeed) RulerFeed->Remove(RulerHandle);
    if (SubjectFeed) SubjectFeed->Remove(SubjectHandle);
    if (WorldBodyFeed) WorldBodyFeed->Remove(BodyHandle);
    if (LocationFeed) LocationFeed->Remove(LocationHandle);
    RulerFeed.Reset(); SubjectFeed.Reset(); WorldBodyFeed.Reset(); LocationFeed.Reset();
    RulerHandle.Reset(); SubjectHandle.Reset(); BodyHandle.Reset(); LocationHandle.Reset();
    if (Core.IsValid())
    { Core->OnCanonicalRuntimeReleasing.Remove(ReleaseHandle); Core->OnNativeCombatActionResolved.Remove(ActionHandle);
      Core->OnNativeBattleStateCommitted.Remove(BattleHandle); }
    Core.Reset(); ReleaseHandle.Reset(); ActionHandle.Reset(); BattleHandle.Reset();
}
void UOGReactiveUiSubsystem::Deinitialize()
{
    Disconnect();
    FOGPlayerProfileSettingsService::OnSettingsChanged().Remove(ProfileHandle);
    if (auto* Library=UOGDefinitionLibrary::Get()) Library->OnDefinitionsChanged.Remove(LibraryHandle);
    GetWorld()->RemoveOnActorSpawnedHandler(SpawnHandle);
    for (const auto& Pair:Bindings)
        if (auto* W=Pair.Key.Get())
        { FLinearColor Now; if (ReadWidgetColor(W,Now)&&Now.Equals(Pair.Value.Last,.001f)) WriteWidgetColor(W,Pair.Value.Base); }
    for (const auto& B:Materials) if (auto* M=B.Material.Get())
    {
        if (M->K2_GetVectorParameterValue(B.Accent).Equals(B.LastAccent,.001f)) M->SetVectorParameterValue(B.Accent,B.OriginalAccent);
        if (FMath::IsNearlyEqual(M->K2_GetScalarParameterValue(B.Glow),B.LastGlow,.001f)) M->SetScalarParameterValue(B.Glow,B.OriginalGlow);
    }
    Bindings.Reset(); Materials.Reset(); OriginalWidgets.Reset(); PendingTrees.Reset(); OnPaletteChanged.Clear();
    Super::Deinitialize();
}
void UOGReactiveUiSubsystem::Observe(UOGGameCoreSubsystem* NewCore,const FOGReactiveUiContext& Keys)
{
    if (Core.Get()==NewCore && Context.RulerId==Keys.RulerId && Context.SubjectId==Keys.SubjectId &&
        Context.WorldBodyId==Keys.WorldBodyId) return;
    Disconnect(); Context=FOGReactiveUiContext();
    Context.RulerId=Keys.RulerId; Context.SubjectId=Keys.SubjectId; Context.WorldBodyId=Keys.WorldBodyId;
    Context.bWorldMode=Keys.bWorldMode; Context.bCombat=Keys.bCombat; Core=NewCore; bDirty=true;
    if (!NewCore) return;
    auto Watch=[this,NewCore](const FOGEntityId& Id,TSharedPtr<FOGWorldStoreCommittedEvents>& Feed,FDelegateHandle& Handle)
    { if (Id.IsValid()) { Feed=NewCore->GetEntityEventFeed(Id); if (Feed) Handle=Feed->AddUObject(this,&ThisClass::HandleEvents); } };
    Watch(Context.RulerId,RulerFeed,RulerHandle);
    if (Context.SubjectId!=Context.RulerId) Watch(Context.SubjectId,SubjectFeed,SubjectHandle);
    if (Context.WorldBodyId!=Context.RulerId&&Context.WorldBodyId!=Context.SubjectId) Watch(Context.WorldBodyId,WorldBodyFeed,BodyHandle);
    ReleaseHandle=NewCore->OnCanonicalRuntimeReleasing.AddUObject(this,&ThisClass::HandleRelease);
    ActionHandle=NewCore->OnNativeCombatActionResolved.AddUObject(this,&ThisClass::HandleAction);
    BattleHandle=NewCore->OnNativeBattleStateCommitted.AddUObject(this,&ThisClass::HandleBattle);
}
void UOGReactiveUiSubsystem::HandleEvents(const TArray<FOGWorldEvent>& Events) { if (!Events.IsEmpty()) bDirty=true; }
void UOGReactiveUiSubsystem::HandleRelease()
{ Disconnect(); Context=FOGReactiveUiContext(); ActionPulse=0; bDirty=true; }
void UOGReactiveUiSubsystem::HandleDefinitions(int64) { bThemeDirty=true; bStyleDirty=true; }
void UOGReactiveUiSubsystem::HandleAction(const FOGResolvedCombatAction& Action)
{
    if (!Context.RulerId.IsValid()) return;
    if (Action.SourceUnitId==Context.SubjectId||Action.SourceUnitId==Context.RulerId||
        (Core.IsValid()&&Core->GetOwnedBattleRulerId()==Context.RulerId))
        ActionPulse=.6f; // Interface activity only: no hit, damage, FX contact or reward is inferred.
}
void UOGReactiveUiSubsystem::HandleBattle(const FOGTurnBattleState&,const FOGEntityId& Ruler)
{ if (Ruler==Context.RulerId) bDirty=true; }
void UOGReactiveUiSubsystem::ReadProjection()
{
    auto* Store=Core.IsValid()?Core->GetWorldStore():nullptr;
    if (!Store||!Store->IsOpen()||Store->IsWriteTransactionActive()) return;
    bDirty=false; FOGReactiveUiContext C=Context; C.bCanonicalReady=false;
    C.OwnedLevel=0; C.RulerClasses.Reset(); C.RulerTitle=TEXT("none");
    C.LocationId=FOGEntityId(); C.LocationKind=NAME_None; C.MovementContext=NAME_None;
    C.RulerScopeJson=TEXT("{}"); C.LocationStateJson=TEXT("{}");
    FString Error,State; FName Kind; bool Found=false; int64 Revision=0;
    auto Fail=[&](){ Context=C; Status=Error; };
    if (!C.RulerId.IsValid()) { Context=C; Status.Reset(); return; }
    if (!Store->TryReadEntity(C.RulerId,Found,Kind,State,Revision,Error)) { Fail(); return; }
    if (!Found||Kind!=TEXT("ruler")) { Error=TEXT("No canonical Ruler for reactive presentation."); Fail(); return; }
    FOGRulerSovereigntyStateRecord Sovereignty;
    if (!Store->TryReadRulerSovereigntyState(C.RulerId,Found,Sovereignty,Error)) { Fail(); return; }
    if (Found) { C.RulerTitle=Sovereignty.CurrentTitle; C.RulerScopeJson=Sovereignty.ScopeStateJson; }
    TArray<FOGEntityClassRecord> Classes;
    if (!Store->ListEntityClasses(C.RulerId,Classes,Error)) { Fail(); return; }
    for (const auto& Entry:Classes) if (Entry.ClassId.IsValid()) C.RulerClasses.AddUnique(Entry.ClassId);
    if (C.SubjectId==C.RulerId)
    {
        FOGResolvedRankProjection Rank;
        if (!FOGRankService(*Store).ResolveEffectiveRank(C.RulerId,Found,Rank,Error)) { Fail(); return; }
        if (Found) C.OwnedLevel=FMath::Max(0,Rank.Level);
    }
    else if (C.SubjectId.IsValid())
    {
        FOGCharacterManifestationRecord M;
        if (!Store->TryReadCharacterManifestation(C.SubjectId,Found,M,Error)) { Fail(); return; }
        if (Found&&M.OwningRulerId==C.RulerId&&M.LifecycleState==TEXT("active")) C.OwnedLevel=FMath::Max(0,M.Level);
    }
    FOGWorldPresenceRecord Presence;
    if (!Store->TryReadWorldPresence(C.WorldBodyId,Found,Presence,Error)) { Fail(); return; }
    if (Found)
    {
        C.LocationId=Presence.LocationId; C.MovementContext=Presence.MovementContext;
        FOGLocationRecord Location;
        if (!Store->TryReadLocation(C.LocationId,Found,Location,Error)) { Fail(); return; }
        if (Found) C.LocationKind=Location.Kind;
        if (!Store->TryReadEntity(C.LocationId,Found,Kind,State,Revision,Error)) { Fail(); return; }
        if (Found) C.LocationStateJson=State; // Only source-authored existing fields can satisfy installed rules.
    }
    if (Context.LocationId!=C.LocationId)
    {
        if (LocationFeed) LocationFeed->Remove(LocationHandle);
        LocationFeed.Reset(); LocationHandle.Reset();
        if (C.LocationId.IsValid())
        { LocationFeed=Core->GetEntityEventFeed(C.LocationId); if (LocationFeed) LocationHandle=LocationFeed->AddUObject(this,&ThisClass::HandleEvents); }
    }
    C.bCanonicalReady=true; Context=MoveTemp(C); Status.Reset();
}
void UOGReactiveUiSubsystem::HandleActorSpawned(AActor* Actor)
{
    if (!Actor) return; TArray<UDirectionalLightComponent*> Lights; Actor->GetComponents(Lights);
    if (!Lights.IsEmpty()) bLightDirty=true;
}
void UOGReactiveUiSubsystem::DiscoverLight()
{
    bLightDirty=false; ULightComponent* Found=nullptr; bool Ambiguous=false;
    for (TActorIterator<AActor> It(GetWorld());It;++It)
    {
        TArray<UDirectionalLightComponent*> Lights; It->GetComponents(Lights);
        for (auto* Light:Lights) if (Light&&Light->IsRegistered())
        { if (Found) Ambiguous=true; else Found=Light; }
    }
    EnvironmentLight=Ambiguous?nullptr:Found; // Never guess which sun/moon is the source.
}
void UOGReactiveUiSubsystem::SetEnvironmentLight(ULightComponent* Light)
{ if (!Light||Light->GetWorld()==GetWorld()) { EnvironmentLight=Light; bLightDirty=false; } }
void UOGReactiveUiSubsystem::SelectInstalledTheme(FOGContentId Id) { ThemeId=MoveTemp(Id); bThemeDirty=true; bStyleDirty=true; }
void UOGReactiveUiSubsystem::SetReducedMotion(bool Reduced) { bReducedMotion=Reduced; if (Reduced) Phase=0; }
bool UOGReactiveUiSubsystem::BindMaterial(UMaterialInstanceDynamic* M,FName Accent,FName Glow)
{
    if (!IsValid(M)||Accent.IsNone()||Glow.IsNone()) return false;
    if (Materials.ContainsByPredicate([M](const auto& B){return B.Material.Get()==M;})) return true;
    TArray<FMaterialParameterInfo> VectorInfo,ScalarInfo; TArray<FGuid> VectorIds,ScalarIds;
    M->GetAllVectorParameterInfo(VectorInfo,VectorIds); M->GetAllScalarParameterInfo(ScalarInfo,ScalarIds);
    if (!VectorInfo.ContainsByPredicate([Accent](const auto& I){return I.Name==Accent;}) ||
        !ScalarInfo.ContainsByPredicate([Glow](const auto& I){return I.Name==Glow;})) return false;
    FMaterialBinding B; B.Material=M; B.Accent=Accent; B.Glow=Glow;
    B.OriginalAccent=M->K2_GetVectorParameterValue(Accent); B.OriginalGlow=M->K2_GetScalarParameterValue(Glow);
    B.LastAccent=Current.Accent; B.LastGlow=Current.Glow;
    Materials.Add(B); M->SetVectorParameterValue(Accent,Current.Accent); M->SetScalarParameterValue(Glow,Current.Glow); return true;
}
UOGReactiveUiSubsystem::ERole UOGReactiveUiSubsystem::Classify(UWidget* W,const FLinearColor& C) const
{
    const FString Name=W->GetName();
    if (OriginalWidgets.Contains(W)||C.A<.15f||Name.Contains(TEXT("Rarity"))||Name.Contains(TEXT("Badge"))) return ERole::Original;
    if (Cast<UImage>(W)) return ERole::Original; // Imported artwork never opts in through a filename.
    if (Cast<UBorder>(W)||Cast<UButton>(W))
        return FMath::Max3(C.R,C.G,C.B)<.16f && FMath::Max3(C.R,C.G,C.B)-FMath::Min3(C.R,C.G,C.B)<.06f
            ? ERole::Surface:ERole::Original;
    if (Cast<UTextBlock>(W))
    {
        if (C.Equals(FLinearColor(.72f,.62f,.44f,C.A),.008f) ||
            C.Equals(FLinearColor(.82f,.73f,.57f,C.A),.008f) ||
            C.Equals(FLinearColor(.83f,.69f,.40f,C.A),.008f)) return ERole::Accent;
        if (FMath::Max3(C.R,C.G,C.B)-FMath::Min3(C.R,C.G,C.B)<.13f)
            return C.GetLuminance()>.72f?ERole::Primary:ERole::Secondary;
    }
    return ERole::Original; // Semantic rarity, warning, HP and faction colours remain authored.
}
void UOGReactiveUiSubsystem::PreserveOriginal(UWidget* W)
{
    if (!W) return;
    OriginalWidgets.Add(W);
    if (auto* B=Bindings.Find(W))
    { FLinearColor Now; if (ReadWidgetColor(W,Now)&&Now.Equals(B->Last,.001f)) WriteWidgetColor(W,B->Base); Bindings.Remove(W); }
}
void UOGReactiveUiSubsystem::Track(UUserWidget* Owner,UWidget* W)
{
    if (!W||!Owner||OriginalWidgets.Contains(W)) return;
    if (Bindings.Contains(W)) return;
    FLinearColor Base; if (!ReadWidgetColor(W,Base)) return;
    FBinding B; B.Owner=Owner; B.Widget=W; B.Base=Base; B.Last=Base; B.Role=Classify(W,Base);
    if (Cast<UImage>(W)&&B.Role==ERole::Original) return;
    Bindings.Add(W,B);
}
void UOGReactiveUiSubsystem::TrackTree(UUserWidget* Owner)
{
    if (!Owner||Owner->GetWorld()!=GetWorld()||!Owner->GetRootWidget()) return;
    if (bApplying) { PendingTrees.Add(Owner); return; }
    for (auto It=OriginalWidgets.CreateIterator();It;++It) if (!It->IsValid()) It.RemoveCurrent();
    TSet<UWidget*> Seen;
    TArray<UWidget*> Pending; Pending.Add(Owner->GetRootWidget());
    while (!Pending.IsEmpty())
    {
        auto* W=Pending.Pop(EAllowShrinking::No); if (!W||Seen.Contains(W)) continue;
        Seen.Add(W); Track(Owner,W);
        if (auto* Panel=Cast<UPanelWidget>(W))
            for (int32 I=0;I<Panel->GetChildrenCount();++I) Pending.Add(Panel->GetChildAt(I));
        if (auto* Nested=Cast<UUserWidget>(W)) Pending.Add(Nested->GetRootWidget());
    }
    for (auto It=Bindings.CreateIterator();It;++It)
        if (!It.Key().IsValid()||(It.Value().Owner.Get()==Owner&&!Seen.Contains(It.Key().Get()))) It.RemoveCurrent();
    Apply();
}
void UOGReactiveUiSubsystem::UntrackTree(UUserWidget* Owner)
{
    for (auto It=Bindings.CreateIterator();It;++It)
        if (!It.Key().IsValid()||It.Value().Owner.Get()==Owner)
        { if (auto* W=It.Key().Get()) { FLinearColor Now;
            if (ReadWidgetColor(W,Now)&&Now.Equals(It.Value().Last,.001f)) WriteWidgetColor(W,It.Value().Base); }
          It.RemoveCurrent(); }
}
FLinearColor UOGReactiveUiSubsystem::AccentFor(const FLinearColor& Base,float Strength) const
{ auto C=FMath::Lerp(Base,Current.Accent,FMath::Clamp(Strength,0.f,1.f)); C.A=Base.A; return C; }
FLinearColor UOGReactiveUiSubsystem::Resolve(const FBinding& B) const
{
    FLinearColor C=B.Base;
    switch (B.Role)
    {
    case ERole::Primary: C=FMath::Lerp(C,Current.Text,.45f); break;
    case ERole::Secondary: C=FMath::Lerp(C,Current.Muted,.3f); break;
    case ERole::Accent: C=Current.Accent; break;
    case ERole::Surface: C=FMath::Lerp(C,Current.Surface,.55f); break;
    case ERole::Ornament: C=AccentFor(C,.28f); break;
    default: break;
    }
    C.A=B.Base.A; return C;
}
void UOGReactiveUiSubsystem::Apply()
{
    if (bApplying) return; TGuardValue<bool> Applying(bApplying,true);
    for (auto It=Bindings.CreateIterator();It;++It)
    {
        auto* W=It.Key().Get(); auto& B=It.Value(); if (!W||!B.Owner.IsValid()) { It.RemoveCurrent(); continue; }
        FLinearColor Now; if (!ReadWidgetColor(W,Now)) { It.RemoveCurrent(); continue; }
        // Gameplay may have changed an existing label to a critical/disabled colour.
        if (!Now.Equals(B.Last,.001f)) { B.Base=Now; B.Role=Classify(W,Now); }
        const auto Next=Resolve(B); if (!Now.Equals(Next,.0001f)) WriteWidgetColor(W,Next); B.Last=Next;
    }
    for (int32 I=Materials.Num()-1;I>=0;--I)
        if (auto* M=Materials[I].Material.Get())
        { M->SetVectorParameterValue(Materials[I].Accent,Current.Accent); M->SetScalarParameterValue(Materials[I].Glow,Current.Glow);
          Materials[I].LastAccent=Current.Accent; Materials[I].LastGlow=Current.Glow; }
        else Materials.RemoveAtSwap(I);
}
void UOGReactiveUiSubsystem::Tick(float Delta)
{
    if (!FMath::IsFinite(Delta)||Delta<0) return;
    auto* Game=GetWorld()->GetGameInstance(); auto* Ready=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    if (Ready&&!Ready->IsCoreReady()) Ready=nullptr;
    auto* Session=Game?Game->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    auto* Modes=Game?Game->GetSubsystem<UOGPresentationModeSubsystem>():nullptr;
    FOGReactiveUiContext Keys; Keys.bWorldMode=Modes&&Modes->GetMode()==EOGPresentationMode::World;
    if (Session&&Session->IsBoundToStoredRuler())
    { Keys.RulerId=Session->BoundRulerId; Keys.SubjectId=Session->SelectedManifestationId.IsValid()?Session->SelectedManifestationId:Keys.RulerId; }
    auto* Player=GetWorld()->GetFirstPlayerController(); APawn* Pawn=Player?Player->GetPawn().Get():nullptr;
    Keys.WorldBodyId=Keys.RulerId;
    if (Pawn)
    {
        if (auto Binding=Pawn->FindComponentByClass<UOGSystemBindingComponent>();Binding&&Binding->CanonicalEntityId.IsValid()) Keys.WorldBodyId=Binding->CanonicalEntityId;
        else if (auto P=Pawn->FindComponentByClass<UOGCanonicalCharacterPresentation>();P&&P->HasProjection()) Keys.WorldBodyId=P->GetProjection().Context.EntityId;
    }
    const auto* Battle=Ready?Ready->GetOwnedLuminousTurnBattle():nullptr;
    Keys.bCombat=Keys.RulerId.IsValid()&&Battle&&Ready->GetOwnedBattleRulerId()==Keys.RulerId&&Battle->GetState().Status==EOGTurnBattleStatus::Running;
    const bool ModeChanged=Context.bWorldMode!=Keys.bWorldMode||Context.bCombat!=Keys.bCombat;
    Observe(Ready,Keys); Context.bWorldMode=Keys.bWorldMode; Context.bCombat=Keys.bCombat;
    if (bDirty) { ReadProjection(); bStyleDirty=true; }
    bStyleDirty|=ModeChanged;
    const FName BeforeMovement=Context.NativeMovementContext;
    Context.NativeMovementContext=NAME_None;
    if (Keys.bWorldMode)
        if (const auto* Character=Cast<ACharacter>(Pawn))
            if (const auto* Movement=Character->GetCharacterMovement())
            { if (Movement->IsSwimming()) Context.NativeMovementContext=TEXT("Underwater");
              else if (Movement->MovementMode==MOVE_Flying) Context.NativeMovementContext=TEXT("Sky"); }
    bStyleDirty|=BeforeMovement!=Context.NativeMovementContext;
    if (bThemeDirty)
    {
        ActiveTheme=nullptr; bThemeDirty=false; bStyleDirty=true;
        if (auto* Library=UOGDefinitionLibrary::Get())
        {
            if (ThemeId.IsValid()) ActiveTheme=Cast<UOGReactiveUiThemeDefinition>(Library->Resolve(TEXT("ui_theme"),ThemeId));
            else { TArray<UDataAsset*> Definitions; Library->List(TEXT("ui_theme"),UOGReactiveUiThemeDefinition::StaticClass(),Definitions);
                if (Definitions.Num()==1) ActiveTheme=Cast<UOGReactiveUiThemeDefinition>(Definitions[0]); }
            FString Error; if (ActiveTheme&&!ActiveTheme->ValidateDefinition(Error)) { ActiveTheme=nullptr; Status=Error; }
        }
    }
    if (bStyleDirty)
    {
        bStyleDirty=false; bHasAuthoredPalette=false;
        if (ActiveTheme)
        {
            const FOGReactiveUiRule* Best=nullptr;
            for (const auto& Rule:ActiveTheme->Rules)
                if (Rule.Matches(Context)&&(!Best||Rule.Priority>Best->Priority)) Best=&Rule;
            if (Best) { AuthoredPalette=Best->Palette; bHasAuthoredPalette=true; }
        }
    }
    Target=bHasAuthoredPalette?AuthoredPalette:ResolveDefault(Context);
    if (bLightDirty) DiscoverLight();
    if (auto* Light=EnvironmentLight.Get();Light&&Light->IsRegistered())
    { auto Environment=Light->GetLightColor().GetClamped(); Environment.A=1;
      Target.Accent=FMath::Lerp(Target.Accent,Target.Accent*Environment,.18f); }
    ActionPulse=FMath::Max(0.f,ActionPulse-Delta*1.8f);
    if (!bReducedMotion) Target.Glow+=ActionPulse*.16f;
    const float Alpha=Target.TransitionSeconds<=SMALL_NUMBER?1.f:1.f-FMath::Exp(-Delta*6.f/Target.TransitionSeconds);
    auto Next=Blend(Current,Target,Alpha);
    if (!bReducedMotion) Phase=FMath::Fmod(Phase+Delta*Current.MotionRate*2.f*PI,2.f*PI);
    PropertyRefreshSeconds+=Delta;
    if (!Current.NearlyEquals(Next))
    { Current=Next; PropertyRefreshSeconds=0; Apply(); OnPaletteChanged.Broadcast(Current); }
    else if (PropertyRefreshSeconds>=.1f)
    { PropertyRefreshSeconds=0; Apply(); } // Reconcile gameplay-owned label colours; no Store reads or tree rebuilding.
    if (!PendingTrees.IsEmpty())
    { auto Pending=MoveTemp(PendingTrees); PendingTrees.Reset(); for (const auto& Owner:Pending) if (Owner.IsValid()) TrackTree(Owner.Get()); }
}
