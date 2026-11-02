#include "Animation/OGSharedAnimationRoutes.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Interaction/OGInteractionParticipantComponent.h"
#include "Combat/OGWorldActionRuntime.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "World/OGTraversalFramework.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/SecureHash.h"

namespace
{
FString RouteKey(FName System,FName Event) { return System.ToString()+TEXT("|")+Event.ToString(); }
float Sample(const FOGSharedMorphTrack& Track,double Time)
{
    if (Time<=Track.Keys[0].TimeSeconds) return Track.Keys[0].Weight;
    for (int32 I=1;I<Track.Keys.Num();++I) if (Time<=Track.Keys[I].TimeSeconds)
    {
        const auto& A=Track.Keys[I-1]; const auto& B=Track.Keys[I];
        return FMath::Lerp(A.Weight,B.Weight,static_cast<float>((Time-A.TimeSeconds)/(B.TimeSeconds-A.TimeSeconds)));
    }
    return Track.Keys.Last().Weight;
}
}
bool FOGSharedAutomaticAnimationRule::Matches(const FOGSharedMotionEnvironment& E) const
{
    if (E.Speed<MinimumSpeed || (MaximumSpeed>=0 && E.Speed>MaximumSpeed)) return false;
    for (FName Capability:RequiredCapabilities) if (!E.Capabilities.Contains(Capability)) return false;
    switch (Condition)
    {
    case EOGSharedMovementCondition::Any: return true;
    case EOGSharedMovementCondition::Grounded: return E.Mode==EOGSharedMovementCondition::Grounded && !E.bFalling;
    case EOGSharedMovementCondition::Stationary: return !E.bFalling && E.Speed<=KINDA_SMALL_NUMBER;
    case EOGSharedMovementCondition::Moving: return E.Speed>KINDA_SMALL_NUMBER;
    case EOGSharedMovementCondition::Rising: return E.bFalling && E.VerticalSpeed>0;
    case EOGSharedMovementCondition::Falling: return E.bFalling && E.VerticalSpeed<=0;
    default: return E.Mode==Condition;
    }
}
const FOGSharedSystemAnimationRoute* UOGSharedAnimationRoutes::FindRoute(FName SourceSystem,FName Event) const
{ return Routes.FindByPredicate([&](const auto& Route){return Route.SourceSystem==SourceSystem && Route.Event==Event;}); }
bool UOGSharedAnimationRoutes::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("animation_routes") || OriginalSourceReference.TrimStartAndEnd().IsEmpty() || Routes.IsEmpty())
    { Error=TEXT("Shared animation routes require their original source and actual subsystem events."); return false; }
    TSet<FString> Keys;
    for (const auto& R:Routes)
    {
        const auto Key=RouteKey(R.SourceSystem,R.Event);
        if (R.SourceSystem.IsNone() || R.Event.IsNone() || Keys.Contains(Key) ||
            static_cast<uint8>(R.Kind)>static_cast<uint8>(EOGSharedAnimationKind::Expression) ||
            !FMath::IsFinite(R.DurationSeconds) || !FMath::IsFinite(R.ContactSeconds))
        { Error=TEXT("Animation routes require distinct subsystem/event keys and finite clocks."); return false; }
        Keys.Add(Key);
        if (R.Kind==EOGSharedAnimationKind::Body)
        {
            if (R.Action.IsNone() || R.Context.IsNone() || !R.MorphTracks.IsEmpty())
            { Error=TEXT("Body routes require the shared source semantic and context."); return false; }
        }
        else
        {
            if (R.DurationSeconds<=0 || R.MorphTracks.IsEmpty() || R.ContactSeconds>R.DurationSeconds)
            { Error=TEXT("Expression routes require original curves and duration."); return false; }
            TSet<FString> Tracks;
            for (const auto& T:R.MorphTracks)
            {
                const auto TrackKey=RouteKey(T.Layer,T.Morph);
                if (T.Layer.IsNone() || T.Morph.IsNone() || T.Keys.IsEmpty() || Tracks.Contains(TrackKey))
                { Error=TEXT("Expression tracks require distinct source channels and keys."); return false; }
                Tracks.Add(TrackKey); float Previous=-1;
                for (const auto& K:T.Keys)
                {
                    if (!FMath::IsFinite(K.TimeSeconds) || !FMath::IsFinite(K.Weight) || K.TimeSeconds<0 ||
                        K.TimeSeconds<=Previous || K.TimeSeconds>R.DurationSeconds || K.Weight < -1 || K.Weight>1)
                    { Error=TEXT("Expression curves require ordered finite source keys within their duration."); return false; }
                    Previous=K.TimeSeconds;
                }
            }
        }
    }
    TSet<FName> Ids;
    for (const auto& R:AutomaticRules)
    {
        const auto* Route=FindRoute(R.SourceSystem,R.Event);
        if (R.RuleId.IsNone() || Ids.Contains(R.RuleId) || !Route || Route->Kind!=EOGSharedAnimationKind::Body ||
            static_cast<uint8>(R.Condition)>static_cast<uint8>(EOGSharedMovementCondition::Vehicle) ||
            !FMath::IsFinite(R.MinimumSpeed) || !FMath::IsFinite(R.MaximumSpeed) || R.MinimumSpeed<0 ||
            (R.MaximumSpeed>=0 && R.MaximumSpeed<R.MinimumSpeed))
        { Error=TEXT("Automatic animation rules require a body route and valid native movement conditions."); return false; }
        Ids.Add(R.RuleId);
    }
    return true;
}
bool UOGSharedAnimationRoutes::SelectAutomatic(const FOGSharedMotionEnvironment& E,int32& Index,FString& Error) const
{
    Index=INDEX_NONE; Error.Reset(); const FOGSharedAutomaticAnimationRule* Best=nullptr; bool Ambiguous=false;
    for (const auto& Rule:AutomaticRules) if (Rule.Matches(E))
    {
        if (Best && Rule.Priority==Best->Priority && RouteKey(Rule.SourceSystem,Rule.Event)!=RouteKey(Best->SourceSystem,Best->Event))
        Ambiguous=true;
        if (!Best || Rule.Priority>Best->Priority) { Best=&Rule; Ambiguous=false; }
    }
    if (Ambiguous) { Error=TEXT("Automatic animation rules have ambiguous equal-priority matches."); return false; }
    if (Best) Index=Routes.IndexOfByPredicate([&](const auto& R){return R.SourceSystem==Best->SourceSystem && R.Event==Best->Event;});
    return true;
}
UOGSharedAnimationReceiver::UOGSharedAnimationReceiver()
{
    AcceptedSystem=TEXT("animation_routes"); ExclusiveChannels={TEXT("animation.automatic")};
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.bStartWithTickEnabled=false;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
}
bool UOGSharedAnimationReceiver::ResolveMotion(FString& Error)
{
    TArray<USkeletalMeshComponent*> Parts; UOGSharedMotionControllerComponent* Actual=nullptr;
    if (!UOGInteractionParticipantComponent::DiscoverNativeBinding(GetOwner(),Parts,Actual,Error) || !Actual) return false;
    if (Motion.Get()!=Actual)
    {
        StopOwnedBody();
        for (const auto& Expression:Expressions) ClearExpression(Expression);
        if (Motion.IsValid()) { Motion->OnAuthoredContact.RemoveAll(this); Motion->OnMotionFinished.RemoveAll(this); Motion->RemoveTickPrerequisiteComponent(this); }
        Motion=Actual; Actual->OnAuthoredContact.AddUniqueDynamic(this,&ThisClass::HandleContact);
        Actual->OnMotionFinished.AddUniqueDynamic(this,&ThisClass::HandleFinished); Actual->AddTickPrerequisiteComponent(this);
    }
    return true;
}
bool UOGSharedAnimationReceiver::PrepareDefinition_Implementation(UDataAsset* Asset,const FOGDefinitionBindingContext&,FString& Error)
{
    const auto* Routes=Cast<UOGSharedAnimationRoutes>(Asset);
    return Routes && Routes->ValidateDefinition(Error) && ResolveMotion(Error);
}
void UOGSharedAnimationReceiver::ActivateDefinition_Implementation()
{
    ++LifecycleGeneration; SetActive(true); SetComponentTickEnabled(true); Status.Reset();
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    if (auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr)
    {
        EntityFeed=Core->GetEntityEventFeed(BoundContext.EntityId);
        if (EntityFeed) EntityFeedHandle=EntityFeed->AddUObject(this,&ThisClass::HandleCommittedWorldEvents);
    }
}
bool UOGSharedAnimationReceiver::BodyIsOwned() const
{ return Motion.IsValid() && BodyRoute!=INDEX_NONE && Motion->IsPlayingAction() && Motion->GetPlaybackGeneration()==BodyGeneration; }
bool UOGSharedAnimationReceiver::IsGameplayBodyBusy() const
{
    if (const auto* Participant=GetOwner()->FindComponentByClass<UOGInteractionParticipantComponent>();
        Participant && Participant->IsInInteractionPresentation()) return true;
    if (const auto* Action=GetOwner()->FindComponentByClass<UOGWorldActionRuntimeComponent>(); Action && Action->HasActiveAction()) return true;
    return Motion.IsValid() && (Motion->IsSceneTimeLocked() || (Motion->IsPlayingAction() && !BodyIsOwned() &&
        !Motion->CurrentAction.ToString().StartsWith(TEXT("Traversal."))));
}
void UOGSharedAnimationReceiver::StopOwnedBody()
{
    if (BodyIsOwned()) Motion->StopAction();
    BodyRoute=INDEX_NONE; BodyRequest=FOGEntityId(); bAutomaticBody=false;
}
FName UOGSharedAnimationReceiver::ExpressionLayer(const FExpression& E,FName Original) const
{ return FName(*(GetName()+TEXT("|")+E.Request.ToString()+TEXT("|")+Original.ToString())); }
void UOGSharedAnimationReceiver::ClearExpression(const FExpression& E)
{
    const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition);
    if (Motion.IsValid() && D && D->Routes.IsValidIndex(E.Route))
        for (const auto& Track:D->Routes[E.Route].MorphTracks) Motion->ClearMorphLayer(ExpressionLayer(E,Track.Layer));
}
bool UOGSharedAnimationReceiver::ApplyExpression(const FExpression& E,double Time,FString& Error)
{
    const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition);
    if (!D || !D->Routes.IsValidIndex(E.Route) || !Motion.IsValid()) return false;
    for (const auto& Track:D->Routes[E.Route].MorphTracks)
        if (!Motion->SetSourceMorphLayer(ExpressionLayer(E,Track.Layer),Track.Morph,Sample(Track,Time)))
        { Error=TEXT("The source expression channel is absent from this actual rig."); return false; }
    return true;
}
bool UOGSharedAnimationReceiver::PlaySystemEvent(FName System,FName Event,const FOGEntityId& Request,FString& Error)
{
    Error.Reset(); const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition);
    const auto* R=D?D->FindRoute(System,Event):nullptr; const auto Key=RouteKey(System,Event);
    if (!IsActive() || !Request.IsValid() || !R || !ResolveMotion(Error))
    { if (Error.IsEmpty()) Error=TEXT("Animation event requires its installed shared route and actual rig."); return false; }
    if (const auto* Prior=Receipts.Find(Request))
    { if (*Prior==Key) return true; Error=TEXT("An animation request ID cannot be reused for another event."); return false; }
    const int32 Index=D->Routes.IndexOfByPredicate([&](const auto& V){return &V==R;});
    if (R->Kind==EOGSharedAnimationKind::Expression)
    {
        FExpression E; E.Route=Index; E.Request=Request;
        if (!ApplyExpression(E,0,Error)) { ClearExpression(E); return false; }
        Expressions.Add(E);
    }
    else
    {
        if (IsGameplayBodyBusy()) { Error=TEXT("The current gameplay action owns body playback."); return false; }
        if (!Motion->CanPlayAction(R->Action,R->Context,false,1,NAME_None) || !Motion->PlayAction(R->Action,R->Context,false,1,NAME_None))
        { Error=TEXT("Shared source motion is unavailable for this rig and context."); return false; }
        BodyRoute=Index; BodyRequest=Request; BodyGeneration=Motion->GetPlaybackGeneration(); bAutomaticBody=false;
    }
    Receipts.Add(Request,Key); return true;
}
void UOGSharedAnimationReceiver::StopSystemEvent(const FOGEntityId& Request)
{
    const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition); if (!D) return;
    const uint64 Lifecycle=LifecycleGeneration;
    if (BodyRequest==Request && D->Routes.IsValidIndex(BodyRoute))
    {
        const auto R=D->Routes[BodyRoute]; StopOwnedBody();
        OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("cancelled"));
        if (!IsActive() || LifecycleGeneration!=Lifecycle || Cast<UOGSharedAnimationRoutes>(BoundDefinition)!=D) return;
    }
    const int32 At=Expressions.IndexOfByPredicate([&](const auto& E){return E.Request==Request;});
    if (At!=INDEX_NONE)
    {
        const auto E=Expressions[At]; const auto R=D->Routes[E.Route]; ClearExpression(E); Expressions.RemoveAt(At);
        OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("cancelled"));
    }
}
void UOGSharedAnimationReceiver::HandleContact(FName Action,FName)
{
    const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition);
    if (D && Motion.IsValid())
        if (const auto* Index=CombatRoutes.Find(Motion->GetActiveCombatReceipt()); Index && D->Routes.IsValidIndex(*Index))
        {
            const auto R=D->Routes[*Index]; const auto Request=Motion->GetActiveCombatReceipt();
            OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("contact")); return;
        }
    if (D && BodyIsOwned() && D->Routes[BodyRoute].Action==Action && BodyRequest.IsValid())
    { const auto R=D->Routes[BodyRoute]; OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,BodyRequest,TEXT("contact")); }
}
void UOGSharedAnimationReceiver::HandleFinished(FName Action,FName)
{
    const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition);
    if (D && Motion.IsValid())
        if (const auto* Index=CombatRoutes.Find(Motion->GetFinishingCombatReceipt()); Index && D->Routes.IsValidIndex(*Index))
        {
            const auto R=D->Routes[*Index]; const auto Request=Motion->GetFinishingCombatReceipt(); CombatRoutes.Remove(Request);
            OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("completed")); return;
        }
    if (!D || !D->Routes.IsValidIndex(BodyRoute) || !Motion.IsValid() || D->Routes[BodyRoute].Action!=Action ||
        Motion->GetPlaybackGeneration()!=BodyGeneration+1 || Motion->IsPlayingAction()) return;
    const auto R=D->Routes[BodyRoute]; const auto Request=BodyRequest; BodyRoute=INDEX_NONE; BodyRequest=FOGEntityId(); bAutomaticBody=false;
    if (Request.IsValid()) OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("completed"));
}
bool UOGSharedAnimationReceiver::DrivesAutomaticMotion(const AActor* Actor)
{
    const auto* Receiver=Actor?Actor->FindComponentByClass<UOGSharedAnimationReceiver>():nullptr;
    const auto* D=Receiver?Cast<UOGSharedAnimationRoutes>(Receiver->BoundDefinition):nullptr;
    return Receiver && Receiver->IsActive() && D && !D->AutomaticRules.IsEmpty();
}
void UOGSharedAnimationReceiver::ReleaseDefinition_Implementation()
{
    ++LifecycleGeneration; SetComponentTickEnabled(false); SetActive(false); StopOwnedBody();
    if (EntityFeed) EntityFeed->Remove(EntityFeedHandle);
    EntityFeed.Reset(); EntityFeedHandle.Reset(); PendingSystemEvents.Reset();
    for (const auto& E:Expressions) ClearExpression(E);
    Expressions.Reset(); Receipts.Reset(); CombatRoutes.Reset();
    if (Motion.IsValid()) { Motion->OnAuthoredContact.RemoveAll(this); Motion->OnMotionFinished.RemoveAll(this); Motion->RemoveTickPrerequisiteComponent(this); }
    Motion.Reset();
}
void UOGSharedAnimationReceiver::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Tick,Function);
    const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition);
    FString Error; if (!IsActive() || !D || !ResolveMotion(Error)) { Status=Error; return; }
    const uint64 Lifecycle=LifecycleGeneration;
    TArray<FOGEntityId> Interrupted;
    for (const auto& Entry:CombatRoutes) if (!Motion->IsCombatMotionPending(Entry.Key)) Interrupted.Add(Entry.Key);
    for (const auto& Request:Interrupted)
    {
        const int32 Index=CombatRoutes.FindChecked(Request); CombatRoutes.Remove(Request);
        if (D->Routes.IsValidIndex(Index))
        { const auto R=D->Routes[Index]; OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("interrupted")); }
        if (!IsActive() || LifecycleGeneration!=Lifecycle || Cast<UOGSharedAnimationRoutes>(BoundDefinition)!=D) return;
    }
    if (BodyRoute!=INDEX_NONE && !BodyIsOwned())
    {
        const auto Route=D->Routes[BodyRoute]; const auto Request=BodyRequest;
        BodyRoute=INDEX_NONE; BodyRequest=FOGEntityId(); bAutomaticBody=false;
        if (Request.IsValid()) OnSystemAnimation.Broadcast(Route.SourceSystem,Route.Event,Request,TEXT("interrupted"));
        if (!IsActive() || LifecycleGeneration!=Lifecycle || Cast<UOGSharedAnimationRoutes>(BoundDefinition)!=D) return;
    }
    TArray<FOGEntityId> ExpressionIds; for (const auto& E:Expressions) ExpressionIds.Add(E.Request);
    for (const auto& Request:ExpressionIds)
    {
        int32 At=Expressions.IndexOfByPredicate([&](const auto& V){return V.Request==Request;}); if (At==INDEX_NONE) continue;
        auto& E=Expressions[At]; const auto R=D->Routes[E.Route];
        E.Elapsed+=FMath::Max(0.f,Delta);
        if (!ApplyExpression(E,FMath::Min<double>(E.Elapsed,R.DurationSeconds),Error))
        {
            const auto Failed=E; ClearExpression(Failed); Expressions.RemoveAt(At); Status=Error;
            OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("interrupted"));
            if (!IsActive() || LifecycleGeneration!=Lifecycle || Cast<UOGSharedAnimationRoutes>(BoundDefinition)!=D) return;
            continue;
        }
        if (!E.ContactSent && R.ContactSeconds>=0 && E.Elapsed>=R.ContactSeconds)
        { E.ContactSent=true; OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("contact")); }
        if (!IsActive() || LifecycleGeneration!=Lifecycle || Cast<UOGSharedAnimationRoutes>(BoundDefinition)!=D) return;
        At=Expressions.IndexOfByPredicate([&](const auto& V){return V.Request==Request;});
        if (At!=INDEX_NONE && Expressions[At].Elapsed>=R.DurationSeconds)
        { const auto Finished=Expressions[At]; ClearExpression(Finished); Expressions.RemoveAt(At);
          OnSystemAnimation.Broadcast(R.SourceSystem,R.Event,Request,TEXT("completed")); }
        if (!IsActive() || LifecycleGeneration!=Lifecycle || Cast<UOGSharedAnimationRoutes>(BoundDefinition)!=D) return;
    }
    TArray<FOGEntityId> ReadyEvents; for (const auto& Pending:PendingSystemEvents) ReadyEvents.Add(Pending.Request);
    for (const auto& Request:ReadyEvents)
    {
        const int32 At=PendingSystemEvents.IndexOfByPredicate([&](const auto& E){return E.Request==Request;});
        if (At==INDEX_NONE) continue; const auto Pending=PendingSystemEvents[At];
        const auto* Route=D->FindRoute(Pending.System,Pending.Event);
        if (!Route) { PendingSystemEvents.RemoveAt(At); continue; }
        if (Route->Kind==EOGSharedAnimationKind::Body && (IsGameplayBodyBusy() || (BodyIsOwned() && !bAutomaticBody))) continue;
        PendingSystemEvents.RemoveAt(At);
        if (!PlaySystemEvent(Pending.System,Pending.Event,Pending.Request,Error)) Status=Error;
        if (!IsActive() || LifecycleGeneration!=Lifecycle || Cast<UOGSharedAnimationRoutes>(BoundDefinition)!=D) return;
    }
    if (IsGameplayBodyBusy() || (BodyIsOwned() && !bAutomaticBody)) return;
    const auto* Character=Cast<ACharacter>(GetOwner()); if (!Character) return;
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    const auto* Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    const auto* Battle=Core?Core->GetOwnedLuminousTurnBattle():nullptr;
    if (Battle && Battle->GetState().Status==EOGTurnBattleStatus::Running)
    {
        const auto* Unit=Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==BoundContext.EntityId;});
        if (Battle->GetState().ExecutionMode==EOGCombatExecutionMode::Turn || (Unit && !Unit->CanAct()))
        { if (bAutomaticBody) StopOwnedBody(); return; }
    }
    FOGSharedMotionEnvironment E; E.Speed=Character->GetVelocity().Size2D(); E.VerticalSpeed=Character->GetVelocity().Z;
    E.Capabilities=Motion->Capabilities;
    const auto* Movement=Character->GetCharacterMovement(); E.bFalling=Movement && Movement->IsFalling();
    if (Movement && Movement->IsSwimming()) E.Mode=EOGSharedMovementCondition::Swimming;
    else if (Movement && Movement->IsFlying()) E.Mode=EOGSharedMovementCondition::Flying;
    if (const auto* Traversal=Character->FindComponentByClass<UOGTraversalCapabilityComponent>())
    {
        switch (Traversal->GetTraversalMode())
        {
        case EOGTraversalMode::Climbing: E.Mode=EOGSharedMovementCondition::Climbing; break;
        case EOGTraversalMode::Swimming: E.Mode=EOGSharedMovementCondition::Swimming; break;
        case EOGTraversalMode::Diving: E.Mode=EOGSharedMovementCondition::Diving; break;
        case EOGTraversalMode::Flying: E.Mode=EOGSharedMovementCondition::Flying; break;
        case EOGTraversalMode::Mounted: E.Mode=EOGSharedMovementCondition::Mounted; break;
        case EOGTraversalMode::Vehicle: E.Mode=EOGSharedMovementCondition::Vehicle; break;
        default: break;
        }
    }
    int32 Route=INDEX_NONE;
    if (!D->SelectAutomatic(E,Route,Error)) { Status=Error; return; }
    if (Route==INDEX_NONE) { if (bAutomaticBody) StopOwnedBody(); return; }
    const auto& R=D->Routes[Route];
    if (BodyIsOwned() && BodyRoute==Route) return;
    if (Motion->CanPlayAction(R.Action,R.Context,false,1,NAME_None) && Motion->PlayAction(R.Action,R.Context,false,1,NAME_None))
    { BodyRoute=Route; BodyRequest=FOGEntityId(); BodyGeneration=Motion->GetPlaybackGeneration(); bAutomaticBody=true; Status.Reset(); }
    else Status=TEXT("Automatic source motion is not available for the connected rig.");
}

void UOGSharedAnimationReceiver::EndPlay(const EEndPlayReason::Type Reason)
{ ReleaseDefinition(); Super::EndPlay(Reason); }

bool UOGSharedAnimationReceiver::PresentAcceptedCombat(const FOGResolvedCombatAction& Action,const FOGTurnBattle& Battle,
    bool& Handled,FString& Error)
{
    Handled=false; Error.Reset(); const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition);
    const auto* Route=D?D->FindRoute(TEXT("combat"),FName(*Action.SkillId.ToString())):nullptr;
    if (!IsActive() || !Route || Action.SourceUnitId!=BoundContext.EntityId) return true;
    if (!Action.ActionId.IsValid() || !ResolveMotion(Error)) return false;
    const auto* Unit=Battle.GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==BoundContext.EntityId;});
    if (!Unit || !Unit->IsAlive()) return true;
    if (Route->Kind==EOGSharedAnimationKind::Expression)
        return PlaySystemEvent(Route->SourceSystem,Route->Event,Action.ActionId,Error); // Body presentation retains its existing source path.
    const auto Key=RouteKey(Route->SourceSystem,Route->Event);
    if (const auto* Prior=Receipts.Find(Action.ActionId))
    {
        if (*Prior!=Key) { Error=TEXT("Combat presentation request conflicts with another system event."); return false; }
        Handled=true; return true;
    }
    const bool Played=Battle.GetState().ExecutionMode==EOGCombatExecutionMode::World && Action.InterruptPriority==0?
        Motion->StartImmediateCombatAction(Route->Action,Battle.GetState().BattleId,Action.ActionId):
        Motion->EnqueueCombatAction(Route->Action,Battle.GetState().BattleId,Action.ActionId);
    if (!Played) { Error=TEXT("Accepted combat route could not resolve its shared source motion."); return false; }
    Receipts.Add(Action.ActionId,Key);
    CombatRoutes.Add(Action.ActionId,D->Routes.IndexOfByPredicate([&](const auto& R){return &R==Route;}));
    Handled=true; return true;
}

void UOGSharedAnimationReceiver::HandleCommittedWorldEvents(const TArray<FOGWorldEvent>& Events)
{
    const auto* D=Cast<UOGSharedAnimationRoutes>(BoundDefinition); if (!IsActive() || !D) return;
    for (const auto& Event:Events)
    {
        if (Event.PrimaryEntity!=BoundContext.EntityId && !Event.RelatedEntities.Contains(BoundContext.EntityId)) continue;
        const auto* Route=D->FindRoute(TEXT("world_event"),Event.EventType); if (!Route) continue;
        const FString Key=TEXT("animation.event|")+Event.EventId.ToString()+TEXT("|")+BoundContext.EntityId.ToString()+TEXT("|")+RouteKey(Route->SourceSystem,Route->Event);
        FGuid Guid; FGuid::ParseExact(FMD5::HashAnsiString(*Key),EGuidFormats::Digits,Guid); const FOGEntityId Request(Guid);
        if (Receipts.Contains(Request) || PendingSystemEvents.ContainsByPredicate([&](const auto& P){return P.Request==Request;})) continue;
        FPendingEvent Pending; Pending.System=Route->SourceSystem; Pending.Event=Route->Event; Pending.Request=Request;
        PendingSystemEvents.Add(Pending);
    }
}
