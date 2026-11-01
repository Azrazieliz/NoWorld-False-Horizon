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
    if (!GetWorld() || !GetWorld()->IsGameWorld() ||
        !IsLocallyControlled() || !IsFoundationWorldInputAllowed())
        return;
    const AOGLuminousV3PlayableCharacter* Luminous =
        Cast<AOGLuminousV3PlayableCharacter>(this);
    const AOGContentPlayableCharacter* ContentCharacter = Cast<AOGContentPlayableCharacter>(this);
    if (!Luminous && !ContentCharacter) return;

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
    const FOGCombatUnitState* Owned = State.Units.FindByPredicate(
        [Session](const FOGCombatUnitState& U)
        { return U.UnitEntityId == Session->SelectedManifestationId; });
    if (!Owned || !((Luminous && Luminous->IsBoundToOwnedLuminousCombatUnit(
        Session->BoundRulerId, Session->SelectedManifestationId, Owned->SkillSet.CurrentForm)) ||
        (ContentCharacter && ContentCharacter->IsBoundToOwnedCombatUnit(
            Session->BoundRulerId, Session->SelectedManifestationId, Owned->SkillSet.CurrentForm)))) return;

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
    // All choices are already authored abilities on the actual participating
    // unit. The approved resolver owns targeting, AoE expansion, costs,
    // status effects, health, turn delay, and every world consequence.
    TArray<FOGContentId> Authored;
    Authored.Reserve(Acting.SkillSet.ActiveSkills.Num()+Acting.SkillSet.GetUltimateSkills().Num());
    for (const FOGContentId& Id : Acting.SkillSet.ActiveSkills)
        if (Id.IsValid() && !Authored.Contains(Id)) { FString Missing; if (Battle->HasNativeActionExecutor(Acting,Id,Missing)) Authored.Add(Id); }
    for (const auto& Ultimate:Acting.SkillSet.GetUltimateSkills())
        if (!Authored.Contains(Ultimate))
        { FString Missing; if (Battle->HasNativeActionExecutor(Acting,Ultimate,Missing)) Authored.Add(Ultimate); }
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
        if (Unit.CanAct() && Unit.TeamIndex == Acting.TeamIndex)
            Candidates.Add(&Unit);

    // Rotation chooses among genuine available abilities without hardcoding
    // any opponent identity, invented kit, numeric scaling or target ceiling.
    const int32 Start = static_cast<int32>(
        static_cast<uint64>(FMath::Max<int64>(0, State.CurrentActionValue)) %
        static_cast<uint64>(Authored.Num()));
    for (int32 Offset = 0; Offset < Authored.Num(); ++Offset)
    {
        const FOGContentId Skill =
            Authored[(Start + Offset) % Authored.Num()];
        for (const FOGCombatUnitState* Focus : Candidates)
        {
            FOGResolvedCombatAction Proposal;
            Proposal.SourceUnitId = Acting.UnitEntityId;
            Proposal.SkillId = Skill;
            Proposal.TargetUnitIds.Add(Focus->UnitEntityId);
            Proposal.ActionDelay = FMath::Max<int64>(1, Acting.DefaultActionDelay);
            if (Skill == Acting.SkillSet.UltimateSkill)
            {
                const FString Form = Acting.SkillSet.CurrentForm.ToString();
                if (Form == TEXT("character:luminous_valentine.version_ln_base"))
                    Proposal.ResolutionJson = TEXT("{\"branch\":\"day\"}");
                else if (Form == TEXT("character:luminous_valentine.version_alter_v1"))
                    Proposal.ResolutionJson = TEXT("{\"branch\":\"ruin\"}");
            }
            FOGResolvedCombatAction Expanded;
            bool bArea = false;
            FString Error;
            if (!Battle->ExpandApprovedFieldTargets(
                Proposal, Expanded, bArea, Error))
                continue;
            FName Semantic = Battle->ResolveNativeAbilityMotion(
                Acting, Expanded.SkillId);
            if (Semantic.IsNone())
            {
                FOGNativeBattleCommandSet AuthoredCommands;
                FString CommandError;
                if (FOGNativeBattleCommands::BuildFor(
                        Acting, AuthoredCommands, CommandError))
                {
                    for (const FOGNativeBattleCommand& Command
                        : AuthoredCommands.Commands)
                    {
                        if (Command.SkillId != Expanded.SkillId ||
                            Command.bFantasm)
                            continue;
                        if (!Command.Branch.IsNone() &&
                            !Proposal.ResolutionJson.Contains(
                                FString::Printf(TEXT("\\"branch\\":\\"%s\\""),
                                    *Command.Branch.ToString().ToLower())))
                            continue;
                        if (!Command.Mode.IsNone() &&
                            !Proposal.ResolutionJson.Contains(
                                FString::Printf(TEXT("\\"mode\\":\\"%s\\""),
                                    *Command.Mode.ToString().ToLower())))
                            continue;
                        Semantic = Command.MotionSemantic();
                        break;
                    }
                }
            }
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
                TEXT("Gate12 non-player original action staged: %s skill %s"),
                *ActingId.ToString(), *Skill.ToString());
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
