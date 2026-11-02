// Source-backed opponents and allies act on the REAL owned Luminous battle
// timeline. This does not spawn encounters, create units, assign skills,
// invent world-level scaling, or use the diagnostic combat simulator.
#include "World/OGStartingRegionPresentation.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Characters/OGContentCharacterActors.h"
#include "Combat/OGNativeTurnAgentComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGNativeBattleCommands.h"
#include "Combat/OGTurnContactActionComponent.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
UOGSharedMotionControllerComponent* OpponentMotion(AActor* Actor)
{
    if (!Actor) return nullptr;
    if (const auto* Appearance=Actor->FindComponentByClass<UOGCharacterAppearanceComponent>())
        if (auto* Motion=Appearance->GetMotionController()) return Motion;
    return Actor->FindComponentByClass<UOGSharedMotionControllerComponent>();
}
}


void AOGWorldPrototypeCharacter::AdvanceCanonicalNonPlayerBattleTurn()
{
    // Autonomous canonical turns are not player movement/camera input.
    // A native combat UMG overlay may suppress player controls while time
    // continues. Only an actual game pause or leaving rendered World suspends
    // this physical, source-contact-bound opponent presentation.
    if (!GetWorld() || !GetWorld()->IsGameWorld() ||
        !IsLocallyControlled() || GetWorld()->IsPaused())
        return;
    if (const auto* Mode=GetGameInstance()
            ? GetGameInstance()->GetSubsystem<UOGPresentationModeSubsystem>() : nullptr;
        Mode && Mode->GetMode()!=EOGPresentationMode::World) return;
    UGameInstance* GI = GetGameInstance();
    UOGGameCoreSubsystem* Core = GI ?
        GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    UOGGate12RulerSessionSubsystem* Session = GI ?
        GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Core || !Session || !Session->IsBoundToStoredRuler()) return;
    const FOGTurnBattle* Battle = Core->GetOwnedLuminousTurnBattle();
    if (!Battle || Battle->GetState().Status != EOGTurnBattleStatus::Running)
        return;
    const FOGTurnBattleState& State = Battle->GetState();
    // Real-time World actions belong to UOGNativeTurnAgentComponent and
    // UOGWorldAbilityExecutionComponent, never this turn-contact fallback.
    if (State.ExecutionMode!=EOGCombatExecutionMode::Turn) return;
    // This is the same canonical source/possessed-body authority used by
    // native player turn input. A genuine bound Ruler pawn can therefore keep
    // opponents acting; this requires neither a selected Luminous copy nor
    // a manufactured owned character.
    FOGEntityId Controlled; FString ControlError;
    if (!ReadControlledCanonicalBattleUnit(Controlled,ControlError)) return;
    const FOGCombatUnitState* Owned = State.Units.FindByPredicate(
        [&Controlled](const FOGCombatUnitState& U)
        { return U.UnitEntityId==Controlled; });
    if (!Owned) return;

    const double Now = GetWorld()->GetTimeSeconds();
    if (AutonomousCombatBattleId != State.BattleId)
    {
        AutonomousCombatBattleId = State.BattleId;
        NextCanonicalOpponentActionAt = Now + 0.75;
    }
    const int32 NextIndex = Battle->SelectNextActingUnitIndex();
    if (!State.Units.IsValidIndex(NextIndex)) return;
    const FOGCombatUnitState& Acting = State.Units[NextIndex];
    const FOGEntityId ActingId = Acting.UnitEntityId;
    const FOGEntityId ActualBattleId = State.BattleId;
    if (Acting.UnitEntityId == Owned->UnitEntityId)
    {
        NextCanonicalOpponentActionAt = Now + 0.75;
        return; // Never auto-play a real player's actions.
    }
    if (Now < NextCanonicalOpponentActionAt) return;
    NextCanonicalOpponentActionAt = Now + 1.20;
    AActor* ExistingSourceBody = UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(
        GetWorld(), ActualBattleId, ActingId);
    if (!IsValid(ExistingSourceBody)) return; // Do not execute an invisible, unembodied opponent.
    if (ExistingSourceBody->FindComponentByClass<UOGNativeTurnAgentComponent>())
        return; // Its exact authored policy owns decisions; never race or bypass it with this fallback.

    const int64 Sequence=Battle->GetLog().GetNextSequence(),Revision=Battle->GetNativeAbilityRevision();
    if (MissingExecutorUnit==ActingId && MissingExecutorSequence==Sequence && MissingExecutorRevision==Revision) return;
    // The non-player uses the SAME executable, source-specific command list
    // as a player-controlled character. Abilities are not identified solely
    // by their SkillId: an Ultimate may have distinct authorized branches,
    // target rules, costs, semantic motion and settlement behavior.
    FOGNativeBattleCommandSet NativeCommands;
    FString NativeCommandError;
    if (!FOGNativeBattleCommands::BuildFor(
            Acting, NativeCommands, NativeCommandError))
    {
        MissingExecutorUnit=ActingId;
        MissingExecutorSequence=Sequence;
        MissingExecutorRevision=Revision;
        UE_LOG(LogTemp, Warning,
            TEXT("Non-player %s lacks an installed source-authored command set: %s"),
            *ActingId.ToString(), *NativeCommandError);
        return;
    }
    TArray<const FOGNativeBattleCommand*> Authored;
    Authored.Reserve(NativeCommands.Commands.Num());
    for (const FOGNativeBattleCommand& Command : NativeCommands.Commands)
    {
        FString ExecutorError;
        if (Command.SkillId.IsValid() &&
            Battle->HasNativeActionExecutor(Acting,Command.SkillId,ExecutorError))
            Authored.Add(&Command);
    }
    if (Authored.IsEmpty())
    {
        MissingExecutorUnit=ActingId; MissingExecutorSequence=Sequence; MissingExecutorRevision=Revision;
        UE_LOG(LogTemp,Warning,TEXT("Actual opponent %s waits for registered source ability content; no empty action was accepted."),*ActingId.ToString());
        return;
    }

    // Prefer living opponents for offensive actions. For authored support
    // skills try eligible allies as well; the canonical skill preflight
    // remains the ONLY authority for whether a target is legal.
    TArray<const FOGCombatUnitState*> Candidates;
    for (const FOGCombatUnitState& Unit : State.Units)
        if (Unit.CanAct() && Unit.TeamIndex != Acting.TeamIndex)
            Candidates.Add(&Unit);
    for (const FOGCombatUnitState& Unit : State.Units)
        if ((Unit.CanAct() ||
            (Unit.Presence == EOGCombatPresence::Defeated && !Unit.IsAlive())) &&
            Unit.TeamIndex == Acting.TeamIndex)
            Candidates.Add(&Unit);

    // Rotation chooses among genuine available abilities without hardcoding
    // any opponent identity, invented kit, numeric scaling or target ceiling.
    const int32 Start = static_cast<int32>(
        static_cast<uint64>(FMath::Max<int64>(0, State.CurrentActionValue)) %
        static_cast<uint64>(Authored.Num()));
    for (int32 Offset = 0; Offset < Authored.Num(); ++Offset)
    {
        const FOGNativeBattleCommand& Command =
            *Authored[(Start + Offset) % Authored.Num()];
        const FOGContentId Skill=Command.SkillId;
        for (const FOGCombatUnitState* Focus : Candidates)
        {
            FOGResolvedCombatAction Proposal;
            TArray<FOGEntityId> FocusIds;
            FocusIds.Add(Focus->UnitEntityId);
            Command.BuildAction(Acting,FocusIds,Proposal);
            FOGResolvedCombatAction Expanded;
            bool bArea = false;
            FString Error;
            if (!Battle->ExpandApprovedFieldTargets(
                Proposal, Expanded, bArea, Error))
                continue;
            // Keep the exact skill branch and its original clip together.
            // Generic character definitions may provide a source-registered
            // native semantic; otherwise use this command's authored mapping.
            // Preserve the exact authored Ultimate branch and mode for
            // both opponent turn staging and the visible action playback.
            const FName Semantic=FOGNativeBattleCommands::MotionForResolvedAction(
                Acting,Expanded,Battle->ResolveNativeAbilityMotion(
                    Acting,Expanded.SkillId));
            if (Semantic.IsNone())
                continue; // Never resolve source damage without its actual motion.

            UOGTurnContactActionComponent* Staging =
                ExistingSourceBody->FindComponentByClass<
                    UOGTurnContactActionComponent>();
            if (!Staging)
            {
                Staging = NewObject<UOGTurnContactActionComponent>(
                    ExistingSourceBody);
                if (!Staging)
                    continue;
                ExistingSourceBody->AddInstanceComponent(Staging);
                Staging->RegisterComponent();
            }
            if (Staging->IsAwaitingAuthoredContact())
                return; // The original character is already mid-strike.
            if (!Staging->StageActualTurnAction(
                    Expanded, Semantic, Error))
                continue;

            // Gameplay settlement and HP reactions happen at the original
            // clip's authored contact. No immediate invisible enemy attack.
            UE_LOG(LogTemp, Display,
                TEXT("Gate12 non-player original action staged: %s command %s skill %s"),
                *ActingId.ToString(), *Command.Id.ToString(), *Skill.ToString());
            return;
        }
    }
    // The real unit cannot use any of its current authored skills. Do not
    // invent an auto-basic, omit a cooldown or skip an authoritative turn.
    UE_LOG(LogTemp, Warning,
        TEXT("Gate12 non-player turn has no legal authored action for %s; preserving canonical timeline"),
        *Acting.UnitEntityId.ToString());
    NextCanonicalOpponentActionAt = Now + 2.5;
}
