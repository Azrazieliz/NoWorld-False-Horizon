#include "Combat/OGTurnContactActionComponent.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UOGTurnContactActionComponent::UOGTurnContactActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UOGTurnContactActionComponent::IsSameLiveTurn(FString& Error) const
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
        Motion->CurrentAction != SourceSemantic ||
        Motion->GetActiveCombatReceipt() != StagedAction.ActionId)
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
    if (!Motion || !Motion->CanPlayAction(
            ApprovedMotionSemantic, TEXT("Combat"), false, 1, NAME_None))
    {
        OutError = TEXT("The actual skeletal character has no approved contact-bearing action clip.");
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
    // resource costs and persistence change ONLY in HandleSourceContact.
    StagedAction = MoveTemp(Expanded);
    BattleId = Battle->GetState().BattleId;
    SourceSemantic = ApprovedMotionSemantic;
    BoundCore = Core;
    BoundMotion = Motion;
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
    return true;
}

void UOGTurnContactActionComponent::HandleSourceContact(
    FName GameplayAction, FName MotionId)
{
    if (!bStaged || bResolving || GameplayAction != SourceSemantic ||
        MotionId.IsNone())
        return;
    FString Error;
    if (!IsSameLiveTurn(Error))
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
        Motion->CompleteCombatAction(Scope, Receipt);
}

void UOGTurnContactActionComponent::HandleSourceMotionFinished(
    UOGSharedMotionControllerComponent* Source,
    FName GameplayAction, FName MotionId)
{
    (void)MotionId;
    if (!bResolving && bStaged && Source == BoundMotion.Get() &&
        GameplayAction == SourceSemantic)
    {
        // An animation with no real contact may not consume a player's turn.
        UE_LOG(LogTemp, Warning,
            TEXT("Original combat animation finished without authored contact; action not executed."));
        DropStagedAction(false);
    }
}

void UOGTurnContactActionComponent::DropStagedAction(bool bStopSourcePlayback)
{
    UOGSharedMotionControllerComponent* Motion = BoundMotion.Get();
    const FOGEntityId OldScope = BattleId;
    const FOGEntityId OldReceipt = StagedAction.ActionId;
    bStaged = false;
    BoundCore.Reset();
    BoundMotion.Reset();
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
