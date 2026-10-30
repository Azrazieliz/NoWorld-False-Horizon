#include "World/OGWorldSimulationReceiver.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Interaction/OGInteractionParticipantComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGCanonicalCharacterPresentation.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Combat/OGWorldActionRuntime.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CoreDelegates.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
bool FailBody(FString& Error, const TCHAR* Message) { Error = Message; return false; }
UOGGameCoreSubsystem* CurrentCore(const UActorComponent* Component)
{
    const auto* World = Component ? Component->GetWorld() : nullptr;
    auto* Game = World ? World->GetGameInstance() : nullptr;
    return Game ? Game->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
}
bool HasExistingBodyActivity(APawn* Pawn, UOGGameCoreSubsystem* Core, const FOGEntityId& Entity)
{
    if (const auto* Participant = Pawn->FindComponentByClass<UOGInteractionParticipantComponent>();
        Participant && Participant->IsInInteractionPresentation()) return true;
    const FOGTurnBattle* Battle = Core->GetOwnedLuminousTurnBattle();
    return Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running &&
        Battle->GetState().Units.ContainsByPredicate(
            [&Entity](const FOGCombatUnitState& Unit) { return Unit.UnitEntityId == Entity; });
}
bool SamePosition(const FOGWorldPresenceRecord& A, const FOGWorldPresenceRecord& B)
{
    return A.LocationId == B.LocationId && A.MovementContext == B.MovementContext &&
        A.LocalPosition == B.LocalPosition && A.UpdatedWorldTick == B.UpdatedWorldTick;
}
}

UOGWorldSimulationDefinition::UOGWorldSimulationDefinition() { System = TEXT("world_simulation"); }
bool UOGWorldSimulationDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System != TEXT("world_simulation") || EntityKind.IsNone() ||
        MovementContext.IsNone() || RequiredStateValues.IsEmpty() ||
        !FMath::IsFinite(RefreshIntervalSeconds) || RefreshIntervalSeconds <= 0.f ||
        CheckpointIntervalWorldTicks <= 0)
        return FailBody(Error, TEXT("World bodies require source-authored kind/state selectors, context and positive refresh/checkpoint cadence."));
    if (bControlledPresence)
    {
        if (!ScheduleIdField.IsNone() || !ArrivalStateField.IsNone() || !ArrivalStateValue.IsEmpty() ||
            AcceptanceRadiusCm != 0.f)
            return FailBody(Error, TEXT("Controlled presence must omit AI schedule, arrival and acceptance-radius policy."));
    }
    else if (ScheduleIdField.IsNone() || !FMath::IsFinite(AcceptanceRadiusCm) || AcceptanceRadiusCm <= 0.f)
        return FailBody(Error, TEXT("AI schedules require source-authored schedule identity and positive physical acceptance radius."));
    for (const auto& Entry : RequiredStateValues)
        if (Entry.Key.IsNone()) return FailBody(Error, TEXT("Availability selectors need exact canonical field names."));
    for (const auto& Entry : AbsentStateValues)
        if (Entry.Key.IsNone()) return FailBody(Error, TEXT("Absence selectors need exact canonical field names."));
    if ((ArrivalStateField.IsNone() && !ArrivalStateValue.IsEmpty()) ||
        (!ArrivalStateField.IsNone() && ArrivalStateField == ScheduleIdField) || ArrivalStateField == TEXT("system_binding_id") ||
        ArrivalStateField == TEXT("diagnosticFixture"))
        return FailBody(Error, TEXT("Arrival state cannot rewrite schedule identity, binding composition or diagnostic provenance."));
    return true;
}

bool UOGWorldSimulationBodyRegistry::CanClaim(const FOGEntityId& Entity,
    const UOGWorldSimulationReceiver* Receiver) const
{
    const auto* Existing = Bodies.Find(Entity);
    return Entity.IsValid() && Receiver && (!Existing || !Existing->IsValid() ||
        Existing->Get() == Receiver || Existing->Get()->GetOwner() == Receiver->GetOwner());
}
bool UOGWorldSimulationBodyRegistry::Claim(const FOGEntityId& Entity, UOGWorldSimulationReceiver* Receiver)
{
    const auto* Existing = Bodies.Find(Entity);
    if (!Entity.IsValid() || !Receiver || (Existing && Existing->IsValid() && Existing->Get() != Receiver)) return false;
    Bodies.Add(Entity, Receiver); return true;
}
bool UOGWorldSimulationBodyRegistry::Owns(const FOGEntityId& Entity, const UOGWorldSimulationReceiver* Receiver) const
{
    const auto* Existing = Bodies.Find(Entity);
    return Existing && Existing->Get() == Receiver;
}
void UOGWorldSimulationBodyRegistry::Release(const FOGEntityId& Entity, const UOGWorldSimulationReceiver* Receiver)
{
    if (Owns(Entity, Receiver)) Bodies.Remove(Entity);
}

void UOGWorldSimulationBodyRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    DeactivateHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &ThisClass::HandleSaveBoundary);
    BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &ThisClass::HandleSaveBoundary);
    TearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::HandleWorldTearDown);
}
void UOGWorldSimulationBodyRegistry::Deinitialize()
{
    HandleSaveBoundary();
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    FWorldDelegates::OnWorldBeginTearDown.Remove(TearDownHandle);
    Bodies.Reset(); Super::Deinitialize();
}
void UOGWorldSimulationBodyRegistry::HandleSaveBoundary()
{
    FString Error;
    if (!CheckpointControlledBodies(Error))
        UE_LOG(LogTemp, Warning, TEXT("Controlled World boundary retained earlier canonical checkpoints: %s"), *Error);
}
void UOGWorldSimulationBodyRegistry::HandleWorldTearDown(UWorld* World)
{
    if (World == GetWorld()) HandleSaveBoundary();
}
bool UOGWorldSimulationBodyRegistry::CheckpointControlledBodies(FString& Error)
{
    Error.Reset(); bool Success = true;
    // Snapshot: committed notifications may synchronously release/rebind a body.
    TArray<TWeakObjectPtr<UOGWorldSimulationReceiver>> Snapshot;
    Bodies.GenerateValueArray(Snapshot);
    for (const auto& Weak : Snapshot)
        if (auto* Body = Weak.Get(); Body && Body->IsBodyActive() && Body->IsControlledPresence() &&
            Body->bHasAccepted && !Body->bRestoreRequired && !Body->bControlledSuspended)
        {
            FString Failure;
            if (!Body->CheckpointControlledPresence(Failure))
            {
                Success = false;
                Error += FString::Printf(TEXT("%s: %s\n"), *Body->BoundContext.EntityId.ToString(), *Failure);
            }
        }
    return Success;
}
void UOGWorldSimulationBodyRegistry::HandleLocationUnregistering(UOGWorldSimulationLocationComponent* Frame)
{
    TArray<TWeakObjectPtr<UOGWorldSimulationReceiver>> Snapshot;
    Bodies.GenerateValueArray(Snapshot);
    for (const auto& Weak : Snapshot)
        if (auto* Body = Weak.Get(); Body && Body->IsControlledPresence() && Body->LocationFrame.Get() == Frame)
        {
            // Called before Super::OnUnregister removes the old frame.
            Body->HandleControlledBoundary();
            Body->InvalidateControlledPresence(); Body->LocationFrame.Reset();
        }
}
void UOGWorldSimulationLocationComponent::OnUnregister()
{
    if (auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr)
        Registry->HandleLocationUnregistering(this);
    Super::OnUnregister();
}

UOGWorldSimulationReceiver::UOGWorldSimulationReceiver()
{
    AcceptedSystem = TEXT("world_simulation");
    ExclusiveChannels.Add(TEXT("world.navigation")); ExclusiveChannels.Add(TEXT("world.presence"));
    PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.bStartWithTickEnabled = false;
}
bool UOGWorldSimulationReceiver::PrepareDefinition_Implementation(UDataAsset* Asset,
    const FOGDefinitionBindingContext& Context, FString& Error)
{
    bWaitingForNativePrerequisites = false;
    auto* D = Cast<UOGWorldSimulationDefinition>(Asset);
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* AI = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
    auto* Core = CurrentCore(this); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (Prepared || bActive || !D || !D->ValidateDefinition(Error)) return false;
    // Establish valid canonical/source authority first. Missing native parts
    // must not turn malformed ownership/schema into an automatic retry.
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() || Store->IsWriteTransactionActive())
        return FailBody(Error, TEXT("Canonical World is not ready for body binding."));
    if (!FOGFoundationWorldSimulationRuntime(*Store).Read(*D, Context.EntityId,
        Core->GetCanonicalWorldTick(), Accepted, Error)) return false;
    if (!Pawn || !Pawn->GetMovementComponent())
    { bWaitingForNativePrerequisites = true; return FailBody(Error, TEXT("World presence needs its existing physical pawn and movement component.")); }
    // A controlled body may prepare before possession, without any physical write.
    if (!D->bControlledPresence &&
        (Pawn->IsPlayerControlled() || !AI || !AI->GetPathFollowingComponent()))
    { bWaitingForNativePrerequisites = true; return FailBody(Error, TEXT("World schedule needs its existing non-player pawn, movement component and AIController.")); }
    if (!Registry || !Registry->CanClaim(Context.EntityId, this))
    { bWaitingForNativePrerequisites = true; return FailBody(Error, TEXT("Another streamed body already owns this canonical entity.")); }
    if (!FindLocationFrame(Accepted.Presence.LocationId, Error))
    { bWaitingForNativePrerequisites = true; return false; }
    Prepared = D; BoundContext = Context; BoundCore = Core; return true;
}
void UOGWorldSimulationReceiver::ActivateDefinition_Implementation()
{
    if (bActive || !Prepared || !BoundCore.IsValid() || !GetWorld()) return;
    auto* Registry = GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>();
    if (!Registry || !Registry->Claim(BoundContext.EntityId, this))
    { bWaitingForNativePrerequisites = true; Status = TEXT("Canonical body claim was superseded during binding."); return; }
    if (!bCapturedBodyFlags)
    { bOriginalHidden = GetOwner()->IsHidden(); bOriginalCollision = GetOwner()->GetActorEnableCollision(); }
    bCapturedBodyFlags = true; bRestoreRequired = true; bSuppressOnRelease = false;
    bActive = true; bHasAccepted = false;
    if (!Prepared->bControlledPresence || !HasControlledPreemption()) SetBodySuppressed(true);
    ReleaseHandle = BoundCore->OnCanonicalRuntimeReleasing.AddUObject(this, &ThisClass::HandleRuntimeReleasing);
    EntityFeed = BoundCore->GetEntityEventFeed(BoundContext.EntityId);
    if (EntityFeed) EntityHandle = EntityFeed->AddUObject(this, &ThisClass::HandleCommitted);
    if (Prepared->bControlledPresence)
    {
        if (auto* Pawn = Cast<APawn>(GetOwner()))
            Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::HandleControllerChanged);
        ControlledParticipant = GetOwner()->FindComponentByClass<UOGInteractionParticipantComponent>();
        if (ControlledParticipant.IsValid())
            InteractionHandle = ControlledParticipant->OnSignificanceChanged.AddUObject(this, &ThisClass::HandleInteractionSignificance);
        BattleHandle = BoundCore->OnNativeBattleStateCommitted.AddUObject(this, &ThisClass::HandleBattleCommitted);
    }
    SetComponentTickInterval(Prepared->RefreshIntervalSeconds); SetComponentTickEnabled(true);
    // Staged activation has no canonical writes or movement. First tick re-reads authority.
}
bool UOGWorldSimulationReceiver::FindLocationFrame(const FOGEntityId& Location, FString& Error)
{
    UOGWorldSimulationLocationComponent* Found = nullptr;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (*It == GetOwner()) continue;
        TArray<UOGWorldSimulationLocationComponent*> Frames; It->GetComponents(Frames);
        for (auto* Frame : Frames)
            if (Frame->IsRegistered() && Frame->LocationId == Location)
            {
                if (Found) return FailBody(Error, TEXT("Streamed Location has ambiguous physical coordinate frames."));
                Found = Frame;
            }
    }
    if (!Found || !Found->GetComponentTransform().IsValid() ||
        !Found->GetComponentScale().Equals(FVector::OneVector))
    { LocationFrame.Reset(); return FailBody(Error, TEXT("Existing Location needs one streamed, unscaled canonical coordinate frame.")); }
    LocationFrame = Found; return true;
}
void UOGWorldSimulationReceiver::SetBodySuppressed(bool Suppressed)
{
    if (!bActive || !GetOwner()) return;
    GetOwner()->SetActorHiddenInGame(Suppressed || bOriginalHidden);
    GetOwner()->SetActorEnableCollision(!Suppressed && bOriginalCollision);
}
void UOGWorldSimulationReceiver::WatchSchedule(const FOGEntityId& Schedule)
{
    if (ObservedSchedule == Schedule) return;
    if (ScheduleFeed) ScheduleFeed->Remove(ScheduleHandle);
    ScheduleFeed.Reset(); ScheduleHandle.Reset(); ObservedSchedule = Schedule;
    if (Schedule.IsValid() && BoundCore.IsValid())
    {
        ScheduleFeed = BoundCore->GetEntityEventFeed(Schedule);
        if (ScheduleFeed) ScheduleHandle = ScheduleFeed->AddUObject(this, &ThisClass::HandleCommitted);
    }
}
void UOGWorldSimulationReceiver::HandleCommitted(const TArray<FOGWorldEvent>&)
{
    // Read current truth next frame. Events never carry a second simulation or replay effects.
    if (bActive) SetComponentTickInterval(0.f);
}
void UOGWorldSimulationReceiver::HandleMoveCompleted(FAIRequestID RequestId, const FPathFollowingResult& Result)
{
    if (!bActive) return;
    if (!MoveRequestId || static_cast<uint32>(RequestId) != MoveRequestId)
    {
        // A later owner may finish before our refresh/release callback runs.
        if (bArrivalPending) { bArrivalPending=false; CompletedMoveRequestId=0; bRestoreRequired=true; }
        return;
    }
    CompletedMoveRequestId = static_cast<uint32>(RequestId);
    MoveRequestId = 0; bArrivalPending = Result.IsSuccess();
    if (!bArrivalPending) Status = TEXT("Native path failed or was interrupted; canonical schedule remains active.");
    SetComponentTickInterval(0.f);
}
void UOGWorldSimulationReceiver::StopOwnedMove()
{
    const uint32 Request = MoveRequestId; MoveRequestId = 0; CompletedMoveRequestId = 0; bArrivalPending = false;
    if (auto* AI = MoveController.Get())
    {
        auto* Path = AI->GetPathFollowingComponent();
        if (Path) Path->OnRequestFinished.RemoveAll(this);
        if (Request && Path && static_cast<uint32>(Path->GetCurrentRequestId()) == Request) AI->StopMovement();
    }
    MoveController.Reset();
}
bool UOGWorldSimulationReceiver::OwnsCheckpointRequest(uint32 Request,bool Arrival,uint32 Completed,
    uint32 Current,bool CurrentValid,bool Idle)
{
    if (Request) return CurrentValid && Current==Request;
    return Arrival && Idle && (!CurrentValid || (Completed && Current==Completed));
}
bool UOGWorldSimulationReceiver::SavePhysicalCheckpoint(bool Arrived, FString& Error)
{
    auto* Core = BoundCore.Get(); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Pawn = Cast<APawn>(GetOwner()); auto* Frame = LocationFrame.Get();
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (!bActive || !bHasAccepted || bRestoreRequired || !Prepared || Prepared->bControlledPresence || !Core || CurrentCore(this) != Core ||
        Core->GetCanonicalWorldTick()<0 || !Store || !Store->IsOpen() || !Pawn || Pawn->IsPlayerControlled() ||
        MoveController.Get() != Pawn->GetController() ||
        HasExistingBodyActivity(Pawn, Core, BoundContext.EntityId) ||
        !Frame || !Frame->IsRegistered() || !Frame->GetComponentTransform().IsValid() ||
        !Frame->GetComponentScale().Equals(FVector::OneVector) ||
        Frame->LocationId != Accepted.Presence.LocationId ||
        !Registry || !Registry->Owns(BoundContext.EntityId, this))
        return FailBody(Error, TEXT("Current canonical lifetime, body claim or Location frame is unavailable."));
    const auto* AI = MoveController.Get();
    const auto* Path = AI ? AI->GetPathFollowingComponent() : nullptr;
    if (!AI || !Path || !OwnsCheckpointRequest(MoveRequestId,bArrivalPending,CompletedMoveRequestId,
        static_cast<uint32>(Path->GetCurrentRequestId()),Path->GetCurrentRequestId().IsValid(),
        Path->GetStatus()==EPathFollowingStatus::Idle))
    { bRestoreRequired=true; return FailBody(Error, TEXT("A superseded native movement request cannot checkpoint this body.")); }
    const int64 Tick = Core->GetCanonicalWorldTick();
    bool Changed = false; FOGFoundationWorldSimulationRuntime Runtime(*Store);
    if (!Runtime.Checkpoint(*Prepared, Accepted,
        Frame->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation()),
        Tick, Arrived, Changed, Error)) return false;
    if (!Runtime.Read(*Prepared, BoundContext.EntityId, Tick, Accepted, Error)) return false;
    LastCheckpointTick = Tick; return true;
}
void UOGWorldSimulationReceiver::Refresh()
{
    if (IsControlledPresence()) { RefreshControlled(); return; }
    auto* Core = BoundCore.Get(); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Pawn = Cast<APawn>(GetOwner()); auto* AI = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (!Prepared || !Core || CurrentCore(this) != Core || !Core->IsCoreReady() ||
        !Store || !Store->IsOpen() || Store->IsWriteTransactionActive() ||
        !Pawn || Pawn->IsPlayerControlled() || !Registry || !Registry->Owns(BoundContext.EntityId, this))
    { StopOwnedMove(); Status = TEXT("Current canonical AI body authority is unavailable."); return; }
    const int64 Tick = Core->GetCanonicalWorldTick();
    FOGFoundationWorldBodyState Current; FString Error;
    if (!FOGFoundationWorldSimulationRuntime(*Store).Read(*Prepared, BoundContext.EntityId, Tick, Current, Error) ||
        !FindLocationFrame(Current.Presence.LocationId, Error))
    { StopOwnedMove(); SetBodySuppressed(true); bHasAccepted = false; Status = Error; return; }
    WatchSchedule(Current.bHasSchedule ? Current.Schedule.ScheduleId : FOGEntityId());
    const bool Restore = bRestoreRequired || !bHasAccepted || !SamePosition(Accepted.Presence, Current.Presence) ||
        (!Accepted.bAvailable && Current.bAvailable);
    if (bHasAccepted && !FOGFoundationWorldSimulationRuntime::SameAuthority(Accepted, Current)) StopOwnedMove();
    if (MoveController.IsValid() && MoveController.Get() != AI) StopOwnedMove();
    Accepted = MoveTemp(Current); bHasAccepted = true;
    if (Restore) { bRestoreRequired=true; SetBodySuppressed(true); }
    else if (Accepted.bAbsent) SetBodySuppressed(true);
    auto* CurrentPath = AI ? AI->GetPathFollowingComponent() : nullptr;
    const bool OwnsPath = MoveRequestId && CurrentPath &&
        static_cast<uint32>(CurrentPath->GetCurrentRequestId()) == MoveRequestId;
    if (HasExistingBodyActivity(Pawn, Core, BoundContext.EntityId) ||
        (CurrentPath && CurrentPath->GetStatus() != EPathFollowingStatus::Idle && !OwnsPath))
    {
        StopOwnedMove(); bHasAccepted = false;
        Status = TEXT("Existing interaction, encounter or native movement owns the real body."); return;
    }
    if (Restore)
    {
        StopOwnedMove();
        // Rehydration only. Actual route execution below always uses native path following.
        if (!Pawn->SetActorLocation(LocationFrame->GetComponentTransform().TransformPosition(Accepted.Presence.LocalPosition),
            false, nullptr, ETeleportType::TeleportPhysics))
        {
            SetBodySuppressed(true); bHasAccepted = false;
            Status = TEXT("Actual body could not restore its canonical physical checkpoint."); return;
        }
        bRestoreRequired=false; LastCheckpointTick = Accepted.Presence.UpdatedWorldTick;
    }
    SetBodySuppressed(Accepted.bAbsent);
    if (!Accepted.bAvailable || !Accepted.bHasSchedule || Tick < Accepted.Schedule.ScheduledStartWorldTick ||
        Accepted.Schedule.Status == TEXT("completed"))
    { StopOwnedMove(); Status.Reset(); return; }
    if (!AI || !CurrentPath)
    { StopOwnedMove(); Status = TEXT("Existing NPC AI controller/path follower is unavailable."); return; }
    if (Accepted.Schedule.Status == TEXT("scheduled"))
    {
        FOGFoundationWorldSimulationRuntime Runtime(*Store);
        if (!Runtime.ActivateDue(*Prepared, Accepted, Tick, Error) ||
            !Runtime.Read(*Prepared, BoundContext.EntityId, Tick, Accepted, Error))
        { StopOwnedMove(); Status = Error; return; }
    }
    if (bArrivalPending)
    {
        if (!SavePhysicalCheckpoint(true, Error)) { StopOwnedMove(); Status = Error; return; }
        StopOwnedMove(); Status.Reset(); return;
    }
    auto* Path = AI->GetPathFollowingComponent();
    if (!Path) { StopOwnedMove(); Status = TEXT("Existing AI path follower is unavailable."); return; }
    if (MoveRequestId)
    {
        if (static_cast<uint32>(Path->GetCurrentRequestId()) != MoveRequestId)
        { StopOwnedMove(); Status = TEXT("Another native action owns the AI path follower."); return; }
        if (Tick - LastCheckpointTick >= Prepared->CheckpointIntervalWorldTicks &&
            !SavePhysicalCheckpoint(false, Error))
        { StopOwnedMove(); Status = Error; }
        return;
    }
    if (Path->GetStatus() != EPathFollowingStatus::Idle)
    { Status = TEXT("Waiting for the existing native movement owner."); return; }
    StopOwnedMove(); MoveController = AI;
    Path->OnRequestFinished.AddUObject(this, &ThisClass::HandleMoveCompleted);
    FAIMoveRequest Request;
    Request.SetGoalLocation(LocationFrame->GetComponentTransform().TransformPosition(Accepted.Destination.LocalPosition));
    Request.SetAcceptanceRadius(Prepared->AcceptanceRadiusCm);
    Request.SetReachTestIncludesAgentRadius(false); Request.SetReachTestIncludesGoalRadius(false);
    Request.SetAllowPartialPath(false); Request.SetUsePathfinding(true); Request.SetProjectGoalLocation(false);
    const auto Result = AI->MoveTo(Request);
    if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
    { MoveRequestId = static_cast<uint32>(Result.MoveId); Status.Reset(); }
    else if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    { CompletedMoveRequestId=static_cast<uint32>(Result.MoveId); bArrivalPending=true; }
    else { StopOwnedMove(); Status = TEXT("No complete native navigation path; schedule remains pending."); }
}
void UOGWorldSimulationReceiver::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    if (!bActive || !Prepared) return;
    SetComponentTickInterval(Prepared->RefreshIntervalSeconds); Refresh();
}
void UOGWorldSimulationReceiver::ReleaseDefinition_Implementation()
{
    if (bActive && IsControlledPresence()) HandleControlledBoundary();
    const bool RestoreFlags = !IsControlledPresence() || bSuppressOnRelease || !HasControlledPreemption();
    if (bActive && bHasAccepted && (MoveRequestId || bArrivalPending))
    {
        FString Error;
        if (!SavePhysicalCheckpoint(false, Error))
            UE_LOG(LogTemp, Warning, TEXT("World body boundary retained last canonical checkpoint: %s"), *Error);
    }
    StopOwnedMove(); SetComponentTickEnabled(false);
    if (auto* Pawn = Cast<APawn>(GetOwner()))
        Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandleControllerChanged);
    if (ControlledParticipant.IsValid()) ControlledParticipant->OnSignificanceChanged.Remove(InteractionHandle);
    ControlledParticipant.Reset(); InteractionHandle.Reset();
    if (BoundCore.IsValid()) BoundCore->OnNativeBattleStateCommitted.Remove(BattleHandle);
    BattleHandle.Reset(); ControlledController.Reset(); bControlledSuspended = false;
    if (EntityFeed) EntityFeed->Remove(EntityHandle);
    if (ScheduleFeed) ScheduleFeed->Remove(ScheduleHandle);
    EntityFeed.Reset(); ScheduleFeed.Reset(); EntityHandle.Reset(); ScheduleHandle.Reset(); ObservedSchedule = FOGEntityId();
    if (BoundCore.IsValid()) BoundCore->OnCanonicalRuntimeReleasing.Remove(ReleaseHandle);
    ReleaseHandle.Reset(); BoundCore.Reset();
    if (auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr)
        Registry->Release(BoundContext.EntityId, this);
    if (RestoreFlags) SetBodySuppressed(bSuppressOnRelease);
    bActive = false; bHasAccepted = false; bRestoreRequired=true;
    LocationFrame.Reset(); Prepared = nullptr; Accepted = FOGFoundationWorldBodyState(); LastCheckpointTick = -1;
    bWaitingForNativePrerequisites = false;
}
void UOGWorldSimulationReceiver::HandleRuntimeReleasing() { bSuppressOnRelease=true; ReleaseDefinition(); }
void UOGWorldSimulationReceiver::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseDefinition(); Super::EndPlay(Reason);
}

bool UOGWorldSimulationReceiver::ValidateControlledPawn(FString& Error, AController* DepartingController) const
{
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* Player = Cast<APlayerController>(DepartingController ? DepartingController : Pawn ? Pawn->GetController() : nullptr);
    if (!Pawn || !Pawn->GetMovementComponent() || !Player || !Player->IsLocalController() ||
        (DepartingController ? Pawn->GetController() == DepartingController :
        (!Pawn->IsLocallyControlled() || !Pawn->IsPlayerControlled() || Player->GetPawn() != Pawn)))
        return FailBody(Error, TEXT("Controlled presence requires this pawn's genuine local player possession or exact departing-controller boundary."));
    const auto* Binding = Pawn->FindComponentByClass<UOGSystemBindingComponent>();
    if (!Binding || Binding->CanonicalEntityId != BoundContext.EntityId)
        return FailBody(Error, TEXT("Physical pawn is not bound to this exact canonical system entity."));
    if (!Accepted.bHasControlledManifestation)
    {
        if (Accepted.LocalRulerId == BoundContext.EntityId) return true;
        return FailBody(Error, TEXT("Controlled non-Manifestation body must be the actual locally bound Ruler."));
    }
    // Canonical-release callbacks may already have torn down presentation.
    // Only a previously restored body can use this path; the transaction still
    // rechecks exact live owner, copy, version, availability and presence.
    if (bSuppressOnRelease && bHasAccepted && !bRestoreRequired) return true;
    if (const auto* Appearance = Pawn->FindComponentByClass<UOGCharacterAppearanceComponent>();
        Appearance && Appearance->MatchesOwnedManifestation(Accepted.LocalRulerId, BoundContext.EntityId)) return true;
    if (const auto* Presentation = Pawn->FindComponentByClass<UOGCanonicalCharacterPresentation>();
        Presentation && Presentation->HasProjection())
    {
        const auto& P = Presentation->GetProjection(); const auto& M = Accepted.ControlledManifestation;
        if (P.Context.EntityId == BoundContext.EntityId && P.bHasManifestation &&
            P.Manifestation.ManifestationId == M.ManifestationId &&
            P.Manifestation.OwningRulerId == M.OwningRulerId &&
            P.Manifestation.IdentityId == M.IdentityId &&
            P.Manifestation.ActiveVersionId == M.ActiveVersionId &&
            P.Manifestation.LifecycleState == M.LifecycleState) return true;
    }
    return FailBody(Error, TEXT("Owned presence needs the existing character adapter's exact live owner/Identity/Version binding."));
}
bool UOGWorldSimulationReceiver::HasControlledPreemption() const
{
    auto* Pawn = Cast<APawn>(GetOwner()); auto* Core = BoundCore.Get();
    if (!Pawn || !Core) return true;
    if (HasExistingBodyActivity(Pawn, Core, BoundContext.EntityId) ||
        Core->IsNativeEncounterAwaitingWorldRebind()) return true;
    if (const auto* Action = Pawn->FindComponentByClass<UOGWorldActionRuntimeComponent>();
        Action && Action->HasActiveAction()) return true;
    const auto* Controller = Pawn->GetController();
    const auto* Path = Controller ? Controller->FindComponentByClass<UPathFollowingComponent>() : nullptr;
    return Path && Path->GetStatus() != EPathFollowingStatus::Idle;
}
void UOGWorldSimulationReceiver::InvalidateControlledPresence()
{
    StopOwnedMove(); bRestoreRequired = true; bHasAccepted = false; ControlledController.Reset();
}
void UOGWorldSimulationReceiver::HandleInteractionSignificance(bool, bool)
{
    if (IsControlledPresence()) InvalidateControlledPresence();
}
void UOGWorldSimulationReceiver::HandleBattleCommitted(const FOGTurnBattleState& State, const FOGEntityId&)
{
    if (IsControlledPresence() && State.Units.ContainsByPredicate(
        [this](const FOGCombatUnitState& Unit) { return Unit.UnitEntityId == BoundContext.EntityId; }))
        InvalidateControlledPresence();
}
void UOGWorldSimulationReceiver::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
    if (!bActive || !IsControlledPresence() || Pawn != GetOwner() || OldController == NewController ||
        Pawn->GetController() != NewController) return;
    // Unreal emits this synchronous notification at the controller change.
    // The old accepted local controller is a narrowly scoped transfer fence;
    // a newly possessed or already invalidated/staged body cannot donate a pose.
    if (OldController && ControlledController.Get() == OldController && bHasAccepted && !bRestoreRequired)
    {
        FString Error;
        if (!SaveControlledCheckpoint(OldController, Error))
            UE_LOG(LogTemp, Warning, TEXT("Controlled handoff retained last canonical checkpoint: %s"), *Error);
    }
    InvalidateControlledPresence();
    if (bActive) SetComponentTickInterval(0.f);
}
bool UOGWorldSimulationReceiver::CheckpointControlledPresence(FString& Error)
{
    return SaveControlledCheckpoint(nullptr, Error);
}
bool UOGWorldSimulationReceiver::SaveControlledCheckpoint(AController* DepartingController, FString& Error)
{
    Error.Reset();
    auto* Core = BoundCore.Get(); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Pawn = Cast<APawn>(GetOwner()); auto* Frame = LocationFrame.Get();
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (!bActive || !IsControlledPresence() || bControlledSuspended || bControlledCheckpointing ||
        !bHasAccepted || bRestoreRequired || !Core || CurrentCore(this) != Core ||
        Core->GetCanonicalWorldTick() < 0 || !Store || !Store->IsOpen() || Store->IsWriteTransactionActive() ||
        !Registry || !Registry->Owns(BoundContext.EntityId, this))
        return FailBody(Error, TEXT("Controlled body has no restored, exclusive canonical checkpoint authority."));
    TGuardValue<bool> Checkpointing(bControlledCheckpointing, true);
    auto* ExpectedController = DepartingController ? DepartingController : Pawn ? Pawn->GetController() : nullptr;
    const auto* DepartingPath = DepartingController ? DepartingController->FindComponentByClass<UPathFollowingComponent>() : nullptr;
    if (!ValidateControlledPawn(Error, DepartingController) || ControlledController.Get() != ExpectedController ||
        HasControlledPreemption() || (DepartingPath && DepartingPath->GetStatus() != EPathFollowingStatus::Idle))
    {
        InvalidateControlledPresence();
        if (Error.IsEmpty()) Error = TEXT("Possession, foreign movement, interaction or encounter staging invalidated this physical sample.");
        return false;
    }
    // Re-resolve uniqueness, identity and transform at every save boundary.
    if (!Frame || !Frame->IsRegistered() || Frame->LocationId != Accepted.Presence.LocationId ||
        !FindLocationFrame(Accepted.Presence.LocationId, Error) || LocationFrame.Get() != Frame ||
        !Frame->GetComponentTransform().Equals(AcceptedFrameTransform))
    {
        InvalidateControlledPresence();
        if (Error.IsEmpty()) Error = TEXT("Location frame changed before the measured boundary; last canonical position retained.");
        return false;
    }
    TStrongObjectPtr<UOGWorldSimulationDefinition> Definition(Prepared.Get());
    const auto Entity = BoundContext.EntityId;
    FOGFoundationWorldSimulationRuntime Runtime(*Store); bool Changed = false;
    const int64 Tick = Core->GetCanonicalWorldTick();
    if (!Runtime.CheckpointControlled(*Definition, Accepted,
        Frame->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation()), Tick, Changed, Error))
    { InvalidateControlledPresence(); return false; }
    // Committed callbacks may have released/replaced this binding. Never
    // dereference a released definition or resurrect an old writer afterwards.
    if (!bActive || Prepared.Get() != Definition.Get() || BoundCore.Get() != Core ||
        !Registry->Owns(Entity, this) || bControlledSuspended || bRestoreRequired) return true;
    FOGFoundationWorldBodyState Current;
    if (!Runtime.Read(*Definition, Entity, Tick, Current, Error))
    { InvalidateControlledPresence(); return false; }
    Accepted = MoveTemp(Current); LastCheckpointTick = Tick; return true;
}
bool UOGWorldSimulationReceiver::SuspendControlledPresence(FString& Error)
{
    const bool Saved = CheckpointControlledPresence(Error);
    bControlledSuspended = true; InvalidateControlledPresence(); return Saved;
}
void UOGWorldSimulationReceiver::ResumeControlledPresence()
{
    bControlledSuspended = false; InvalidateControlledPresence();
    if (bActive) SetComponentTickInterval(0.f);
}
void UOGWorldSimulationReceiver::HandleControlledBoundary()
{
    if (!bActive || !IsControlledPresence() || bControlledSuspended || !bHasAccepted || bRestoreRequired) return;
    FString Error;
    if (!CheckpointControlledPresence(Error))
        UE_LOG(LogTemp, Warning, TEXT("Controlled World boundary retained last canonical checkpoint: %s"), *Error);
}
void UOGWorldSimulationReceiver::RefreshControlled()
{
    StopOwnedMove(); // This policy never borrows the AI path writer.
    auto* Core = BoundCore.Get(); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (bControlledSuspended || !Core || CurrentCore(this) != Core || !Core->IsCoreReady() ||
        !Store || !Store->IsOpen() || Store->IsWriteTransactionActive() ||
        !Pawn || !Registry || !Registry->Owns(BoundContext.EntityId, this))
    { InvalidateControlledPresence(); Status = TEXT("Controlled body is suspended or canonical lifetime is unavailable."); return; }
    auto* Participant = Pawn->FindComponentByClass<UOGInteractionParticipantComponent>();
    if (ControlledParticipant.Get() != Participant)
    {
        if (ControlledParticipant.IsValid()) ControlledParticipant->OnSignificanceChanged.Remove(InteractionHandle);
        ControlledParticipant = Participant; InteractionHandle.Reset();
        if (Participant) InteractionHandle = Participant->OnSignificanceChanged.AddUObject(this, &ThisClass::HandleInteractionSignificance);
        InvalidateControlledPresence();
    }
    // Check before hiding or restoring: never teleport a staged encounter pose.
    if (HasControlledPreemption())
    { InvalidateControlledPresence(); Status = TEXT("Existing movement, interaction or encounter owns the physical pose."); return; }
    const int64 Tick = Core->GetCanonicalWorldTick(); FString Error;
    FOGFoundationWorldBodyState Current;
    if (!FOGFoundationWorldSimulationRuntime(*Store).Read(*Prepared, BoundContext.EntityId, Tick, Current, Error))
    { InvalidateControlledPresence(); Status = Error; return; }
    const bool PresenceChanged = !bHasAccepted || !SamePosition(Accepted.Presence, Current.Presence) ||
        Accepted.LocalRulerId != Current.LocalRulerId ||
        Accepted.bHasControlledManifestation != Current.bHasControlledManifestation ||
        (Current.bHasControlledManifestation &&
        (Accepted.ControlledManifestation.IdentityId != Current.ControlledManifestation.IdentityId ||
        Accepted.ControlledManifestation.ActiveVersionId != Current.ControlledManifestation.ActiveVersionId));
    const auto* PreviousFrame = LocationFrame.Get();
    Accepted = MoveTemp(Current);
    if (!ValidateControlledPawn(Error))
    { InvalidateControlledPresence(); Status = Error; return; }
    if (!FindLocationFrame(Accepted.Presence.LocationId, Error))
    { InvalidateControlledPresence(); SetBodySuppressed(true); Status = Error; return; }
    const FTransform FrameTransform = LocationFrame->GetComponentTransform();
    const bool Restore = bRestoreRequired || PresenceChanged ||
        ControlledController.Get() != Pawn->GetController() || PreviousFrame != LocationFrame.Get() ||
        !FrameTransform.Equals(AcceptedFrameTransform);
    if (Accepted.bAbsent || !Accepted.bAvailable)
    {
        InvalidateControlledPresence(); SetBodySuppressed(Accepted.bAbsent);
        Status = TEXT("Canonical controlled presence is absent or unavailable."); return;
    }
    if (Restore)
    {
        SetBodySuppressed(true);
        if (!Pawn->SetActorLocation(FrameTransform.TransformPosition(Accepted.Presence.LocalPosition),
            false, nullptr, ETeleportType::TeleportPhysics))
        { InvalidateControlledPresence(); Status = TEXT("Controlled body could not restore its canonical Location-local position."); return; }
        AcceptedFrameTransform = FrameTransform; ControlledController = Pawn->GetController();
        bRestoreRequired = false; bHasAccepted = true; LastCheckpointTick = Tick;
    }
    SetBodySuppressed(false); Status.Reset();
    if (!Restore && Tick - LastCheckpointTick >= Prepared->CheckpointIntervalWorldTicks &&
        !CheckpointControlledPresence(Error)) Status = Error;
}
