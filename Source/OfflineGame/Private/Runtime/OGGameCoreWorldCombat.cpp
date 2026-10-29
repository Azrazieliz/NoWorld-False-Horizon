#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGWorldAbilityExecutionComponent.h"
#include "Combat/OGCombatActionJournal.h"
#include "Combat/OGCombatVitalityService.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "World/OGStartingRegionPresentation.h"

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
        !FOGCombatVitalityService(*WorldStore).Commit(Before,Candidate.GetState(),ActiveCombatRulerId,Event,Error) ||
        (Candidate.GetState().Status==EOGTurnBattleStatus::Completed &&
         !FOGCombatRewardService(*WorldStore).ApplyCommittedVictory(Candidate.GetState(),ActiveCombatRulerId,Event,Outcome,Error)) ||
        !PersistNativeEncounter(Candidate,ActiveCombatRulerId,Event,Cause,false,Error) ||
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
    if (bNativeRecoveryAwaitingWorld) { Error=TEXT("Recovered World encounter is reconnecting its native bodies."); return false; }
    if (OwnedLuminousTurnBattle->GetState().ExecutionMode==Mode) return true;
    if (!CanSetOwnedCombatExecutionMode(Mode,Error)) return false;
    TGuardValue<bool> Guard(bSubmittingNativeAction,true); FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    if (!Candidate.SetExecutionMode(Mode,Error)) return false;
    if (!CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_mode_changed"),nullptr,INDEX_NONE,false,Error)) return false;
    LastWorldCombatSeconds=-1; WorldCombatFractionalMilliseconds=0; return true;
}
bool UOGGameCoreSubsystem::BeginOwnedWorldAbility(const FOGResolvedCombatAction& Root,FString& Error)
{
    if (bNativeRecoveryAwaitingWorld) { Error=TEXT("Recovered World encounter is reconnecting its native bodies."); return false; }
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
    if (bNativeRecoveryAwaitingWorld) { Error=TEXT("Recovered World encounter is reconnecting its native bodies."); return false; }
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
    TArray<TWeakObjectPtr<AOGWorldPrototypeCharacter>> Dodged;
    const auto& State=OwnedLuminousTurnBattle->GetState();
    const auto* Source=State.Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Cause.SourceUnitId;});
    if (Source && OwnedLuminousTurnBattle->IsWorldDamagingStep(*Source,Cause.SkillId,Step))
        Eligible.RemoveAll([&](const auto& Id)
        {
            const auto* Unit=State.Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Id;});
            auto* Body=Cast<AOGWorldPrototypeCharacter>(UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),State.BattleId,Id));
            if (Unit && Unit->TeamIndex!=Source->TeamIndex && Body && Body->IsDodgeInvulnerable())
            { Dodged.Add(Body); return true; }
            return false;
        });
    FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    if (!Candidate.ApplyWorldAbilityStep(CastId,Step,Eligible,Error)) return false;
    Cause.TargetUnitIds=Eligible;
    if (!CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_impact_resolved"),&Cause,Step,false,Error)) return false;
    for (const auto& Body:Dodged) if (Body.IsValid()) Body->NotifyPerfectDodge();
    return true;
}
bool UOGGameCoreSubsystem::CancelOwnedWorldAbility(const FOGEntityId& CastId,FString& Error)
{
    if (bNativeRecoveryAwaitingWorld) { Error=TEXT("Recovered World encounter is reconnecting its native bodies."); return false; }
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
    if (bNativeRecoveryAwaitingWorld)
    { Error=TEXT("Recovered World encounter waits for native bodies and cast runners."); return false; }
    TArray<FOGEntityId> MissingBodies;
    for (const auto& Entry:NativeWorldCastRequests)
    {
        const auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(World,OwnedLuminousTurnBattle->GetState().BattleId,Entry.Value.SourceUnitId);
        const auto* Runner=Actor?Actor->FindComponentByClass<UOGWorldAbilityExecutionComponent>():nullptr;
        // A streamed replacement cannot inherit another actor's accepted cast or fabricate missed contacts.
        if (!Runner || Runner->GetActiveCastId()!=Entry.Key) MissingBodies.Add(Entry.Key);
    }
    // Commit separately: later clock events must not erase cancellation owner-action reactions/rewards.
    MissingBodies.Sort([](const auto& A,const auto& B){return A.ToString()<B.ToString();});
    for (const auto& Id:MissingBodies) if (!CancelOwnedWorldAbility(Id,Error)) return false;
    const auto Current=OwnedLuminousTurnBattle->GetState();
    if (Current.Status!=EOGTurnBattleStatus::Running)
    { LastWorldCombatSeconds=Now; WorldCombatFractionalMilliseconds=0; return true; }
    if (Whole>MAX_int64-Current.WorldElapsedMilliseconds) { Error=TEXT("World gameplay clock exhausted."); return false; }
    TGuardValue<bool> Guard(bSubmittingNativeAction,true); FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    const int64 Sequence=Candidate.GetLog().GetNextSequence();
    if (Candidate.GetState().Status==EOGTurnBattleStatus::Running &&
        !Candidate.AdvanceWorldCombatClock(Current.WorldElapsedMilliseconds+Whole,Error)) return false;
    if (Candidate.GetLog().GetNextSequence()!=Sequence || Candidate.GetState().Status!=Current.Status)
    {
        if (!CommitWorldCombatCandidate(MoveTemp(Candidate),TEXT("combat.world_clock_advanced"),nullptr,INDEX_NONE,false,Error)) return false;
    }
    else *OwnedLuminousTurnBattle=MoveTemp(Candidate); // Cursor-only updates do not create per-frame database events.
    const int64 Consumed=OwnedLuminousTurnBattle->GetState().WorldElapsedMilliseconds-Current.WorldElapsedMilliseconds;
    LastWorldCombatSeconds=Now; WorldCombatFractionalMilliseconds=Delta-Consumed; return true;
}
bool UOGGameCoreSubsystem::TickWorldCombat(float)
{
    BindWorldEventFeed(); FlushCommittedWorldEvents();
    if (bCoreReady && !bReleasingCanonicalRuntime && !bSubmittingNativeAction)
    {
        FString Error;
        if (bNativeRecoveryAwaitingWorld && !CompleteNativeWorldRecovery(Error))
        {
            if (Error!=LastWorldCombatClockError) UE_LOG(LogTemp,Warning,TEXT("Native World recovery waits: %s"),*Error);
            LastWorldCombatClockError=Error; return true;
        }
        if (!AdvanceOwnedWorldCombatTime(Error))
        {
            if (Error!=LastWorldCombatClockError)
                UE_LOG(LogTemp,Warning,TEXT("Native World combat waits for a valid clock/state: %s"),*Error);
            LastWorldCombatClockError=Error;
        }
        else LastWorldCombatClockError.Reset();
    }
    return true;
}

bool UOGGameCoreSubsystem::ValidateOwnedWorldParticipants(const FOGTurnBattleState& State,const FOGEntityId& Ruler,FString& Error) const
{
    if (!WorldStore || !WorldStore->IsOpen() || !Ruler.IsValid())
    { Error=TEXT("World party validation requires its stored Ruler."); return false; }
    int32 OwnedActive=0;
    for (const auto& Unit:State.Units) if (Unit.CanAct())
    {
        bool Owned=Unit.UnitEntityId==Ruler;
        if (!Owned)
        {
            bool Found=false; FOGCharacterManifestationRecord M;
            if (!WorldStore->TryReadCharacterManifestation(Unit.UnitEntityId,Found,M,Error)) return false;
            Owned=Found && M.OwningRulerId==Ruler;
        }
        if (Owned) ++OwnedActive;
    }
    if (OwnedActive>3) { Error=TEXT("World Mode requires the protagonist and at most two companions."); return false; }
    return true;
}
bool UOGGameCoreSubsystem::CanSetOwnedCombatExecutionMode(EOGCombatExecutionMode Mode,FString& Error) const
{
    Error.Reset();
    if (!bCoreReady || bReleasingCanonicalRuntime || !OwnedLuminousTurnBattle)
    { Error=TEXT("No live canonical encounter is available."); return false; }
    if (Mode==EOGCombatExecutionMode::World &&
        !ValidateOwnedWorldParticipants(OwnedLuminousTurnBattle->GetState(),ActiveCombatRulerId,Error)) return false;
    FOGTurnBattle Candidate=*OwnedLuminousTurnBattle;
    return Candidate.SetExecutionMode(Mode,Error);
}
