#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGCombatActionJournal.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

bool UOGGameCoreSubsystem::CommitWorldCombatCandidate(FOGTurnBattle&& Candidate,FName Operation,
    const FOGResolvedCombatAction* Cause,int32 StepIndex,bool PublishRoot,FString& Error)
{
    if (!OwnedLuminousTurnBattle || !WorldStore || !WorldStore->IsOpen())
    { Error=TEXT("Canonical World combat is unavailable."); return false; }
    const auto Before=OwnedLuminousTurnBattle->GetState(); TArray<FOGWorldAbilityCastView> Casts; Candidate.GetWorldCasts(Casts);
    FOGWorldEvent Event;
    if (!FOGCombatActionJournal::BuildWorldRuntimeEvent(Before,Candidate.GetState(),Candidate.GetLastNativeReactions(),
        ActiveCombatRulerId,GetCanonicalWorldTick(),Operation,Cause,StepIndex,Casts,Event,Error) ||
        !WorldStore->BeginTransaction(Error)) return false;
    auto Fail=[&](){FString Ignored; WorldStore->RollbackTransaction(Ignored); return false;};
    FOGNativeBattleOutcome Outcome;
    if (!WorldStore->AppendWorldEvent(Event,Error) ||
        (Candidate.GetState().Status==EOGTurnBattleStatus::Completed &&
         !FOGCombatRewardService(*WorldStore).ApplyCommittedVictory(Candidate.GetState(),ActiveCombatRulerId,Event,Outcome,Error)) ||
        !WorldStore->CommitTransaction(Error)) return Fail();
    *OwnedLuminousTurnBattle=MoveTemp(Candidate); LastNativeBattleOutcome=MoveTemp(Outcome); LastNativeBattleReceipt=Event.EventId;
    const auto State=OwnedLuminousTurnBattle->GetState(); const auto Reactions=OwnedLuminousTurnBattle->GetLastNativeReactions();
    LastNativeCombatEvent=FOGNativeCombatEventProvenance(); LastNativeCombatEvent.EventId=Event.EventId;
    LastNativeCombatEvent.BattleId=State.BattleId; LastNativeCombatEvent.OwningRulerId=ActiveCombatRulerId;
    LastNativeCombatEvent.WorldTick=Event.WorldTick; LastNativeCombatEvent.BattleStatus=State.Status;
    if (Cause)
    { LastNativeCombatEvent.ActionId=Cause->ActionId; LastNativeCombatEvent.SourceUnitId=Cause->SourceUnitId;
      LastNativeCombatEvent.SkillId=Cause->SkillId; LastNativeCombatEvent.TargetUnitIds=Cause->TargetUnitIds; }
    if (Operation==TEXT("combat.world_cast_started") && Cause) CommittedNativeActionIds.Add(Cause->ActionId);
    for (const auto& R:Reactions) if (R.bExecuted) CommittedNativeActionIds.Add(R.Action.ActionId);
    for (auto It=NativeWorldCastRequests.CreateIterator();It;++It)
    { FOGWorldAbilityCastView View; if (!OwnedLuminousTurnBattle->ReadWorldCast(It.Key(),View)) It.RemoveCurrent(); }
    if (!LastNativeBattleOutcome.RewardEventIds.IsEmpty())
        if (auto* Session=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
            Session && Session->BoundRulerId==ActiveCombatRulerId)
        { FString Warning; if (!Session->RefreshCanonicalProjections(Warning)) UE_LOG(LogTemp,Warning,TEXT("World rewards saved; view refresh failed: %s"),*Warning); }
    OnNativeBattleStateCommitted.Broadcast(State,Event.EventId);
    if (Cause && PublishRoot) PublishNativeAction(*Cause,Event,State);
    for (const auto& R:Reactions) if (R.bExecuted) PublishNativeAction(R.Action,Event,State);
    return true;
}
bool UOGGameCoreSubsystem::SetOwnedCombatExecutionMode(EOGCombatExecutionMode Mode,FString& Error)
{
    Error.Reset();
    if (!IsInGameThread() || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime || !OwnedLuminousTurnBattle)
    { Error=TEXT("No live canonical encounter is available."); return false; }
    if (OwnedLuminousTurnBattle->GetState().ExecutionMode==Mode) return true;
    if (Mode==EOGCombatExecutionMode::World)
    {
        int32 OwnedActive=0;
        for (const auto& Unit:OwnedLuminousTurnBattle->GetState().Units) if (Unit.CanAct())
        {
            bool Owned=Unit.UnitEntityId==ActiveCombatRulerId;
            if (!Owned)
            {
                bool Found=false; FOGCharacterManifestationRecord M;
                if (!WorldStore->TryReadCharacterManifestation(Unit.UnitEntityId,Found,M,Error)) return false;
                Owned=Found && M.OwningRulerId==ActiveCombatRulerId;
            }
            if (Owned) ++OwnedActive;
        }
        if (OwnedActive>3) { Error=TEXT("World Mode requires the protagonist and at most two companions."); return false; }
    }
    TGuardValue<bool> Guard(bSubmittingNativeAction,true); FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    if (!Candidate.SetExecutionMode(Mode,Error)) return false;
    if (!CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_mode_changed"),nullptr,INDEX_NONE,false,Error)) return false;
    LastWorldCombatSeconds=-1; WorldCombatFractionalMilliseconds=0; return true;
}
bool UOGGameCoreSubsystem::BeginOwnedWorldAbility(const FOGResolvedCombatAction& Root,FString& Error)
{
    Error.Reset(); if (!AdvanceOwnedWorldCombatTime(Error)) return false;
    if (!IsInGameThread() || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime || !OwnedLuminousTurnBattle ||
        !Root.ActionId.IsValid() || CommittedNativeActionIds.Contains(Root.ActionId) ||
        !UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),OwnedLuminousTurnBattle->GetState().BattleId,Root.SourceUnitId))
    { Error=TEXT("World cast requires a fresh request and its actual bound native actor."); return false; }
    TGuardValue<bool> Guard(bSubmittingNativeAction,true); FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    if (!Candidate.BeginWorldAbility(Root,Error)) return false;
    // Keep immutable request context available before source-motion callbacks are published.
    NativeWorldCastRequests.Add(Root.ActionId,Root);
    if (!CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_cast_started"),&Root,INDEX_NONE,true,Error))
    { NativeWorldCastRequests.Remove(Root.ActionId); return false; }
    return true;
}
bool UOGGameCoreSubsystem::ApplyOwnedWorldAbilityStep(const FOGEntityId& CastId,int32 Step,
    const TArray<FOGEntityId>& Contacts,FString& Error)
{
    Error.Reset(); if (!AdvanceOwnedWorldCombatTime(Error)) return false;
    const auto* Request=NativeWorldCastRequests.Find(CastId);
    if (!IsInGameThread() || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime || !OwnedLuminousTurnBattle || !Request)
    { Error=TEXT("Native World impact has no accepted live cast."); return false; }
    TGuardValue<bool> Guard(bSubmittingNativeAction,true); auto Cause=*Request;
    TArray<FOGEntityId> Physical,Eligible;
    for (const auto& Id:Contacts)
        if (UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),OwnedLuminousTurnBattle->GetState().BattleId,Id))
            Physical.AddUnique(Id);
    if (!OwnedLuminousTurnBattle->FilterWorldContacts(Cause,Physical,Eligible,Error)) return false;
    FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    if (!Candidate.ApplyWorldAbilityStep(CastId,Step,Eligible,Error)) return false;
    Cause.TargetUnitIds=Eligible;
    return CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_impact_resolved"),&Cause,Step,false,Error);
}
bool UOGGameCoreSubsystem::CancelOwnedWorldAbility(const FOGEntityId& CastId,FString& Error)
{
    Error.Reset();
    const auto* Request=NativeWorldCastRequests.Find(CastId);
    if (!Request) return true;
    if (!IsInGameThread() || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime || !OwnedLuminousTurnBattle)
    { Error=TEXT("World cast cancellation has no live runtime."); return false; }
    TGuardValue<bool> Guard(bSubmittingNativeAction,true); const auto Cause=*Request;
    FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    if (!Candidate.CancelWorldAbility(CastId,Error)) return false;
    return CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_cast_cancelled"),&Cause,INDEX_NONE,false,Error);
}
bool UOGGameCoreSubsystem::AdvanceOwnedWorldCombatTime(FString& Error)
{
    Error.Reset();
    if (!bCoreReady || bReleasingCanonicalRuntime || !OwnedLuminousTurnBattle ||
        OwnedLuminousTurnBattle->GetState().ExecutionMode!=EOGCombatExecutionMode::World ||
        OwnedLuminousTurnBattle->GetState().Status!=EOGTurnBattleStatus::Running) return true;
    auto* World=GetWorld();
    if (!IsInGameThread() || bSubmittingNativeAction || !World || !World->IsGameWorld()) return true;
    const double Now=World->GetTimeSeconds();
    if (WorldCombatClockWorld.Get()!=World || LastWorldCombatSeconds<0 || Now<LastWorldCombatSeconds || World->IsPaused())
    { WorldCombatClockWorld=World; LastWorldCombatSeconds=Now; return true; }
    const double Delta=(Now-LastWorldCombatSeconds)*1000.0+WorldCombatFractionalMilliseconds;
    if (!FMath::IsFinite(Delta) || Delta<0 || Delta>static_cast<double>(MAX_int64))
    { Error=TEXT("World gameplay clock delta is invalid."); return false; }
    const int64 Whole=static_cast<int64>(Delta);
    if (!Whole) return true;
    const auto& Current=OwnedLuminousTurnBattle->GetState();
    if (Whole>MAX_int64-Current.WorldElapsedMilliseconds) { Error=TEXT("World gameplay clock exhausted."); return false; }
    TGuardValue<bool> Guard(bSubmittingNativeAction,true); FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    const int64 Sequence=Candidate.GetLog().GetNextSequence();
    TArray<FOGEntityId> MissingBodies;
    for (const auto& Entry:NativeWorldCastRequests)
        if (!UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(World,Current.BattleId,Entry.Value.SourceUnitId)) MissingBodies.Add(Entry.Key);
    for (const auto& Id:MissingBodies) if (!Candidate.CancelWorldAbility(Id,Error)) return false;
    if (Candidate.GetState().Status==EOGTurnBattleStatus::Running &&
        !Candidate.AdvanceWorldCombatClock(Current.WorldElapsedMilliseconds+Whole,Error)) return false;
    if (Candidate.GetLog().GetNextSequence()!=Sequence || Candidate.GetState().Status!=Current.Status)
    {
        if (!CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_clock_advanced"),nullptr,INDEX_NONE,false,Error)) return false;
    }
    else *OwnedLuminousTurnBattle=MoveTemp(Candidate); // Cursor-only updates do not create per-frame database events.
    LastWorldCombatSeconds=Now; WorldCombatFractionalMilliseconds=Delta-Whole; return true;
}
bool UOGGameCoreSubsystem::TickWorldCombat(float)
{
    if (bCoreReady && !bReleasingCanonicalRuntime && !bSubmittingNativeAction)
    {
        FString Error;
        if (!AdvanceOwnedWorldCombatTime(Error))
        { UE_LOG(LogTemp,Warning,TEXT("Native World combat waits for a valid clock/state: %s"),*Error); LastWorldCombatSeconds=-1; }
    }
    return true;
}
