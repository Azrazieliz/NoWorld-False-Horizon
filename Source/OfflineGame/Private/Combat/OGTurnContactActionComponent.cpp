#include "Combat/OGTurnContactActionComponent.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UOGTurnContactActionComponent::UOGTurnContactActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UOGTurnContactActionComponent::IsSameLiveTurn(
    FString& Error, bool bSourceFinished) const
{
    Error.Reset();
    UWorld* World = GetWorld();
    const UOGGameCoreSubsystem* Core = BoundCore.Get();
    const FOGTurnBattle* Battle =
        Core && Core->IsCoreReady() ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!World || !World->IsGameWorld() || !GetOwner() ||
        !bStaged || !Battle || !BattleId.IsValid() ||
        Battle->GetState().BattleId != BattleId ||
        Battle->GetState().Status != EOGTurnBattleStatus::Running ||
        Battle->GetState().ExecutionMode != EOGCombatExecutionMode::Turn)
    {
        Error = TEXT("The source-authored turn was interrupted or its encounter ended.");
        return false;
    }
    if (const APawn* ActualPawn = Cast<APawn>(GetOwner());
        ActualPawn && ActualPawn->GetController() != OriginalPawnController.Get())
    {
        Error = TEXT("The character was unpossessed or reassigned before contact.");
        return false;
    }
    const int32 Acting = Battle->SelectNextActingUnitIndex();
    if (!Battle->GetState().Units.IsValidIndex(Acting) ||
        Battle->GetState().Units[Acting].UnitEntityId != StagedAction.SourceUnitId)
    {
        Error = TEXT("The original turn actor changed before animation contact.");
        return false;
    }
    if (const auto* Body =
        GetOwner()->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>())
    {
        if (!Body->IsBoundTo(BattleId, StagedAction.SourceUnitId))
        {
            Error = TEXT("The staged action no longer belongs to its actual World character.");
            return false;
        }
    }
    UOGSharedMotionControllerComponent* Motion = BoundMotion.Get();
    if (!Motion || Motion->GetOwner() != GetOwner() ||
        (bSourceFinished
            ? Motion->GetFinishingCombatReceipt() != StagedAction.ActionId
            : (Motion->CurrentAction != SourceSemantic ||
               Motion->GetActiveCombatReceipt() != StagedAction.ActionId)))
    {
        Error = TEXT("The approved character motion was replaced before its authored contact.");
        return false;
    }
    return true;
}

bool UOGTurnContactActionComponent::StageActualTurnAction(
    const FOGResolvedCombatAction& Requested,
    FName ApprovedMotionSemantic, FString& OutError)
{
    OutError.Reset();
    if (bStaged || bResolving)
    {
        OutError = TEXT("The original character already has an unresolved turn action.");
        return false;
    }
    UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    UOGGameCoreSubsystem* Core = GI
        ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const FOGTurnBattle* Battle =
        Core && Core->IsCoreReady() ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!World || !World->IsGameWorld() || !GetOwner() ||
        !Battle || Battle->GetState().Status != EOGTurnBattleStatus::Running ||
        Battle->GetState().ExecutionMode != EOGCombatExecutionMode::Turn ||
        !Requested.SourceUnitId.IsValid() || !Requested.SkillId.IsValid() ||
        ApprovedMotionSemantic.IsNone())
    {
        OutError = TEXT("A genuine active turn, source skill and authored motion are required.");
        return false;
    }
    const int32 Acting = Battle->SelectNextActingUnitIndex();
    if (!Battle->GetState().Units.IsValidIndex(Acting) ||
        Battle->GetState().Units[Acting].UnitEntityId != Requested.SourceUnitId)
    {
        OutError = TEXT("This real character is not the current action-value actor.");
        return false;
    }
    if (const auto* Binding =
        GetOwner()->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
        Binding && !Binding->IsBoundTo(
            Battle->GetState().BattleId, Requested.SourceUnitId))
    {
        OutError = TEXT("The original visual character is bound to a different battle unit.");
        return false;
    }
    const auto* Appearance =
        GetOwner()->FindComponentByClass<UOGCharacterAppearanceComponent>();
    UOGSharedMotionControllerComponent* Motion = Appearance
        ? Appearance->GetMotionController() : nullptr;
    if (!Motion)
        Motion = GetOwner()->FindComponentByClass<UOGSharedMotionControllerComponent>();
    if (!Motion || !Motion->CanResolveTurnContactAction(ApprovedMotionSemantic))
    {
        OutError = TEXT("Original combat rig playback or authentic contact/completion evidence is missing.");
        return false;
    }

    FOGResolvedCombatAction Expanded;
    if (!Battle->ValidateResolvedAction(Requested, Expanded, OutError))
        return false;
    if (!Battle->HasNativeActionExecutor(
            Battle->GetState().Units[Acting], Expanded.SkillId, OutError))
        return false;
    if (!Expanded.ActionId.IsValid())
        Expanded.ActionId = FOGEntityId::NewId();
    if (!Expanded.ActionId.IsValid())
    {
        OutError = TEXT("Unable to allocate the actual action receipt.");
        return false;
    }

    // Playback is not a settlement. Charge, damage, heal, turn progression,
    // resource costs and persistence change ONLY upon an actual authored
    // contact, or the original sequence's natural completed event when
    // that approved sequence has no source contact marker.
    StagedAction = MoveTemp(Expanded);
    BattleId = Battle->GetState().BattleId;
    SourceSemantic = ApprovedMotionSemantic;
    BoundCore = Core;
    BoundMotion = Motion;
    OriginalPawnController = Cast<APawn>(GetOwner())
        ? Cast<APawn>(GetOwner())->GetController() : nullptr;
    bStaged = true;
    Motion->OnAuthoredContact.AddUniqueDynamic(
        this, &ThisClass::HandleSourceContact);
    Motion->OnNativeMotionFinished.AddUObject(
        this, &ThisClass::HandleSourceMotionFinished);

    if (!Motion->StartImmediateCombatAction(
            SourceSemantic, BattleId, StagedAction.ActionId))
    {
        OutError = TEXT("The actual approved source animation could not start.");
        DropStagedAction(false);
        return false;
    }
    bNaturalCompletionAuthorized=Motion->IsCurrentNaturalCompletionContact();
    return true;
}

void UOGTurnContactActionComponent::HandleSourceContact(
    FName GameplayAction, FName MotionId)
{
    ResolveSourceMotionEvent(GameplayAction, MotionId, false);
}

void UOGTurnContactActionComponent::ResolveSourceMotionEvent(
    FName GameplayAction, FName MotionId, bool bSourceFinished)
{
    if (!bStaged || bResolving || GameplayAction != SourceSemantic ||
        MotionId.IsNone())
        return;
    FString Error;
    if (!IsSameLiveTurn(Error, bSourceFinished))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Cancelled interrupted source action before contact: %s"),
            *Error);
        DropStagedAction(true);
        return;
    }
    UOGGameCoreSubsystem* Core = BoundCore.Get();
    if (!Core)
    {
        DropStagedAction(true);
        return;
    }
    const FOGTurnBattle* Battle = Core->GetOwnedLuminousTurnBattle();
    FOGResolvedCombatAction Expanded;
    if (!Battle || !Battle->ValidateResolvedAction(
            StagedAction, Expanded, Error))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Original turn source action rejected at contact: %s"), *Error);
        DropStagedAction(true);
        return;
    }

    // World streaming or defeat may remove a target after the player
    // pressed the button. Do not inflict invisible damage on a vanished
    // participant just because the earlier preview was legal.
    for (const FOGEntityId& Id : Expanded.TargetUnitIds)
    {
        const FOGCombatUnitState* Target =
            Battle->GetState().Units.FindByPredicate(
                [&Id](const FOGCombatUnitState& Unit)
                { return Unit.UnitEntityId == Id; });
        const bool bNeedsWorldBody = Target &&
            Target->Presence == EOGCombatPresence::Active && Target->IsAlive() &&
            Id != Expanded.SourceUnitId;
        if (!Target || (bNeedsWorldBody &&
            !UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(
                GetWorld(), BattleId, Id)))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Source contact aborted: the actual target is no longer embodied."));
            DropStagedAction(true);
            return;
        }
    }
    TGuardValue<bool> Guard(bResolving, true);
    const bool bLuminous =
        Battle->GetState().Units.ContainsByPredicate(
            [this](const FOGCombatUnitState& Unit)
            {
                return Unit.UnitEntityId == StagedAction.SourceUnitId &&
                    Unit.IdentityId ==
                        FOGContentId(TEXT("character:luminous_valentine.identity"));
            });
    const FOGEntityId Receipt = StagedAction.ActionId;
    const FOGEntityId Scope = BattleId;
    UOGSharedMotionControllerComponent* Motion = BoundMotion.Get();
    const bool bCommitted = bLuminous
        ? Core->ApplyApprovedLuminousResolvedAction(Expanded, Error)
        : Core->ApplyOwnedLuminousTurnAction(Expanded, Error);
    if (!bCommitted)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Authoritative contact had no game effect: %s"), *Error);
        DropStagedAction(true);
        return;
    }
    // The authoritative combat commit broadcasts exactly once to bound
    // physical actors, consequences, rewards and persisted WorldStore.
    // Do not generate visual-only HP damage here.
    DropStagedAction(false);
    if (Motion)
        if (!bSourceFinished)
            Motion->CompleteCombatAction(Scope, Receipt);
}

void UOGTurnContactActionComponent::HandleSourceMotionFinished(
    UOGSharedMotionControllerComponent* Source,
    FName GameplayAction, FName MotionId)
{
    if (!bResolving && bStaged && bNaturalCompletionAuthorized &&
        Source == BoundMotion.Get() && GameplayAction == SourceSemantic)
    {
        // This is the real end of a source-verified, nonlooping native
        // sequence. Original legacy clips often lack contact-notify data:
        // settle once only after that real movement finishes, not when a
        // fabricated clock or UI request says it ought to have hit.
        ResolveSourceMotionEvent(GameplayAction, MotionId, true);
    }
}

void UOGTurnContactActionComponent::DropStagedAction(bool bStopSourcePlayback)
{
    UOGSharedMotionControllerComponent* Motion = BoundMotion.Get();
    const FOGEntityId OldScope = BattleId;
    const FOGEntityId OldReceipt = StagedAction.ActionId;
    bStaged = false;
    bNaturalCompletionAuthorized = false;
    BoundCore.Reset();
    BoundMotion.Reset();
    OriginalPawnController.Reset();
    BattleId = FOGEntityId();
    StagedAction = FOGResolvedCombatAction();
    SourceSemantic = NAME_None;
    if (Motion)
    {
        Motion->OnAuthoredContact.RemoveAll(this);
        Motion->OnNativeMotionFinished.RemoveAll(this);
        if (bStopSourcePlayback)
            Motion->CancelCombatAction(OldScope, OldReceipt);
    }
}

void UOGTurnContactActionComponent::CancelUncommittedTurnAction()
{
    if (bStaged)
        DropStagedAction(true);
}

void UOGTurnContactActionComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    CancelUncommittedTurnAction();
    Super::EndPlay(EndPlayReason);
}
