#include "World/OGWorldSimulationReceiver.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"

namespace
{
bool FailBody(FString& Error, const TCHAR* Message) { Error = Message; return false; }
UOGGameCoreSubsystem* CurrentCore(const UActorComponent* Component)
{
    const auto* World = Component ? Component->GetWorld() : nullptr;
    auto* Game = World ? World->GetGameInstance() : nullptr;
    return Game ? Game->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
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
    if (System != TEXT("world_simulation") || EntityKind.IsNone() || ScheduleIdField.IsNone() ||
        MovementContext.IsNone() || RequiredStateValues.IsEmpty() ||
        !FMath::IsFinite(AcceptanceRadiusCm) || AcceptanceRadiusCm <= 0.f ||
        !FMath::IsFinite(RefreshIntervalSeconds) || RefreshIntervalSeconds <= 0.f ||
        CheckpointIntervalWorldTicks <= 0)
        return FailBody(Error, TEXT("World simulation requires source-authored kind, schedule/state selectors, context and positive physical/refresh/checkpoint tolerances."));
    for (const auto& Entry : RequiredStateValues)
        if (Entry.Key.IsNone()) return FailBody(Error, TEXT("Availability selectors need exact canonical field names."));
    for (const auto& Entry : AbsentStateValues)
        if (Entry.Key.IsNone()) return FailBody(Error, TEXT("Absence selectors need exact canonical field names."));
    if ((ArrivalStateField.IsNone() && !ArrivalStateValue.IsEmpty()) ||
        ArrivalStateField == ScheduleIdField || ArrivalStateField == TEXT("system_binding_id") ||
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

UOGWorldSimulationReceiver::UOGWorldSimulationReceiver()
{
    AcceptedSystem = TEXT("world_simulation");
    ExclusiveChannels.Add(TEXT("world.navigation")); ExclusiveChannels.Add(TEXT("world.presence"));
    PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.bStartWithTickEnabled = false;
}
bool UOGWorldSimulationReceiver::PrepareDefinition_Implementation(UDataAsset* Asset,
    const FOGDefinitionBindingContext& Context, FString& Error)
{
    auto* D = Cast<UOGWorldSimulationDefinition>(Asset);
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* AI = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
    auto* Core = CurrentCore(this); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (Prepared || bActive || !D || !D->ValidateDefinition(Error)) return false;
    if (!Pawn || Pawn->IsPlayerControlled() || !Pawn->GetMovementComponent() || !AI || !AI->GetPathFollowingComponent())
        return FailBody(Error, TEXT("World schedule needs its existing non-player pawn, movement component and AIController."));
    if (!Registry || !Registry->CanClaim(Context.EntityId, this))
        return FailBody(Error, TEXT("Another streamed body already owns this canonical entity."));
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() || Store->IsWriteTransactionActive())
        return FailBody(Error, TEXT("Canonical World is not ready for body binding."));
    if (!FOGFoundationWorldSimulationRuntime(*Store).Read(*D, Context.EntityId,
        Core->GetCanonicalWorldTick(), Accepted, Error) || !FindLocationFrame(Accepted.Presence.LocationId, Error)) return false;
    Prepared = D; BoundContext = Context; BoundCore = Core; return true;
}
void UOGWorldSimulationReceiver::ActivateDefinition_Implementation()
{
    if (bActive || !Prepared || !BoundCore.IsValid() || !GetWorld()) return;
    auto* Registry = GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>();
    if (!Registry || !Registry->Claim(BoundContext.EntityId, this))
    { Status = TEXT("Canonical body claim was superseded during binding."); return; }
    bOriginalHidden = GetOwner()->IsHidden(); bOriginalCollision = GetOwner()->GetActorEnableCollision();
    bActive = true; bHasAccepted = false;
    ReleaseHandle = BoundCore->OnCanonicalRuntimeReleasing.AddUObject(this, &ThisClass::HandleRuntimeReleasing);
    EntityFeed = BoundCore->GetEntityEventFeed(BoundContext.EntityId);
    if (EntityFeed) EntityHandle = EntityFeed->AddUObject(this, &ThisClass::HandleCommitted);
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
    if (!bActive || !MoveRequestId || static_cast<uint32>(RequestId) != MoveRequestId) return;
    MoveRequestId = 0; bArrivalPending = Result.IsSuccess();
    if (!bArrivalPending) Status = TEXT("Native path failed or was interrupted; canonical schedule remains active.");
    SetComponentTickInterval(0.f);
}
void UOGWorldSimulationReceiver::StopOwnedMove()
{
    const uint32 Request = MoveRequestId; MoveRequestId = 0; bArrivalPending = false;
    if (auto* AI = MoveController.Get())
    {
        auto* Path = AI->GetPathFollowingComponent();
        if (Path) Path->OnRequestFinished.RemoveAll(this);
        if (Request && Path && static_cast<uint32>(Path->GetCurrentRequestId()) == Request) AI->StopMovement();
    }
    MoveController.Reset();
}
bool UOGWorldSimulationReceiver::SavePhysicalCheckpoint(bool Arrived, FString& Error)
{
    auto* Core = BoundCore.Get(); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Pawn = Cast<APawn>(GetOwner()); auto* Frame = LocationFrame.Get();
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (!bActive || !bHasAccepted || !Prepared || !Core || CurrentCore(this) != Core ||
        !Core->IsCoreReady() || !Store || !Store->IsOpen() || !Pawn || Pawn->IsPlayerControlled() ||
        !Frame || !Frame->IsRegistered() || Frame->LocationId != Accepted.Presence.LocationId ||
        !Registry || !Registry->Owns(BoundContext.EntityId, this))
        return FailBody(Error, TEXT("Current canonical lifetime, body claim or Location frame is unavailable."));
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
    auto* Core = BoundCore.Get(); auto* Store = Core ? Core->GetWorldStore() : nullptr;
    auto* Pawn = Cast<APawn>(GetOwner()); auto* AI = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
    auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr;
    if (!Prepared || !Core || CurrentCore(this) != Core || !Core->IsCoreReady() ||
        !Store || !Store->IsOpen() || Store->IsWriteTransactionActive() ||
        !Pawn || Pawn->IsPlayerControlled() || !AI || !Registry || !Registry->Owns(BoundContext.EntityId, this))
    { StopOwnedMove(); Status = TEXT("Current canonical AI body authority is unavailable."); return; }
    const int64 Tick = Core->GetCanonicalWorldTick();
    FOGFoundationWorldBodyState Current; FString Error;
    if (!FOGFoundationWorldSimulationRuntime(*Store).Read(*Prepared, BoundContext.EntityId, Tick, Current, Error) ||
        !FindLocationFrame(Current.Presence.LocationId, Error))
    { StopOwnedMove(); SetBodySuppressed(true); bHasAccepted = false; Status = Error; return; }
    WatchSchedule(Current.bHasSchedule ? Current.Schedule.ScheduleId : FOGEntityId());
    const bool Restore = !bHasAccepted || !SamePosition(Accepted.Presence, Current.Presence) ||
        (!Accepted.bAvailable && Current.bAvailable);
    if (bHasAccepted && !FOGFoundationWorldSimulationRuntime::SameAuthority(Accepted, Current)) StopOwnedMove();
    if (MoveController.IsValid() && MoveController.Get() != AI) StopOwnedMove();
    Accepted = MoveTemp(Current); bHasAccepted = true;
    SetBodySuppressed(Accepted.bAbsent);
    if (Restore)
    {
        StopOwnedMove();
        // Rehydration only. Actual route execution below always uses native path following.
        Pawn->SetActorLocation(LocationFrame->GetComponentTransform().TransformPosition(Accepted.Presence.LocalPosition),
            false, nullptr, ETeleportType::TeleportPhysics);
        LastCheckpointTick = Accepted.Presence.UpdatedWorldTick;
    }
    if (!Accepted.bAvailable || !Accepted.bHasSchedule || Tick < Accepted.Schedule.ScheduledStartWorldTick ||
        Accepted.Schedule.Status == TEXT("completed"))
    { StopOwnedMove(); Status.Reset(); return; }
    if (Accepted.Schedule.Status == TEXT("scheduled"))
    {
        FOGFoundationWorldSimulationRuntime Runtime(*Store);
        if (!Runtime.ActivateDue(*Prepared, Accepted, Tick, Error) ||
            !Runtime.Read(*Prepared, BoundContext.EntityId, Tick, Accepted, Error))
        { StopOwnedMove(); Status = Error; return; }
    }
    if (bArrivalPending)
    {
        bArrivalPending = false;
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
    else if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal) bArrivalPending = true;
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
    if (bActive && bHasAccepted && (MoveRequestId || bArrivalPending))
    {
        FString Error;
        if (!SavePhysicalCheckpoint(false, Error))
            UE_LOG(LogTemp, Warning, TEXT("World body boundary retained last canonical checkpoint: %s"), *Error);
    }
    StopOwnedMove(); SetComponentTickEnabled(false);
    if (EntityFeed) EntityFeed->Remove(EntityHandle);
    if (ScheduleFeed) ScheduleFeed->Remove(ScheduleHandle);
    EntityFeed.Reset(); ScheduleFeed.Reset(); EntityHandle.Reset(); ScheduleHandle.Reset(); ObservedSchedule = FOGEntityId();
    if (BoundCore.IsValid()) BoundCore->OnCanonicalRuntimeReleasing.Remove(ReleaseHandle);
    ReleaseHandle.Reset(); BoundCore.Reset();
    if (auto* Registry = GetWorld() ? GetWorld()->GetSubsystem<UOGWorldSimulationBodyRegistry>() : nullptr)
        Registry->Release(BoundContext.EntityId, this);
    SetBodySuppressed(false); bActive = false; bHasAccepted = false;
    LocationFrame.Reset(); Prepared = nullptr; Accepted = FOGFoundationWorldBodyState(); LastCheckpointTick = -1;
}
void UOGWorldSimulationReceiver::HandleRuntimeReleasing() { ReleaseDefinition(); }
void UOGWorldSimulationReceiver::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseDefinition(); Super::EndPlay(Reason);
}
