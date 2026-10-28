// Native commands for the actual owned character in the rendered World encounter.
#include "World/OGStartingRegionPresentation.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Characters/OGContentCharacterActors.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Combat/OGNativeBattleCommands.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
UOGSharedMotionControllerComponent* BattleMotion(AActor* Actor)
{
    if (!Actor) return nullptr;
    if (auto* Appearance=Actor->FindComponentByClass<UOGCharacterAppearanceComponent>())
        if (auto* Motion=Appearance->GetMotionController()) return Motion;
    return Actor->FindComponentByClass<UOGSharedMotionControllerComponent>();
}
}
bool AOGWorldPrototypeCharacter::HasNativeOwnedLuminousEncounter() const
{
    const auto* Core=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    const auto* Battle=Core?Core->GetOwnedLuminousTurnBattle():nullptr;
    return Battle && Battle->GetState().Status==EOGTurnBattleStatus::Running;
}
bool AOGWorldPrototypeCharacter::ReadOwnedLuminousBattleCommands(
    FOGNativeBattleCommandSet& Out,FString& Error) const
{
    Out=FOGNativeBattleCommandSet(); Error.Reset();
    const auto* Core=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    const auto* Session=GetGameInstance()?GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    const auto* Battle=Core?Core->GetOwnedLuminousTurnBattle():nullptr;
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !IsLocallyControlled() ||
        !Core || !Core->IsCoreReady() || !Session || !Session->IsBoundToStoredRuler() ||
        Core->GetOwnedBattleRulerId()!=Session->BoundRulerId || !Battle)
    { Error=TEXT("This character has no locally owned encounter."); return false; }
    const auto* Unit=Battle->GetState().Units.FindByPredicate(
        [Session](const FOGCombatUnitState& U) { return U.UnitEntityId==Session->SelectedManifestationId; });
    const auto* Legacy=Cast<AOGLuminousV3PlayableCharacter>(this);
    const auto* Content=Cast<AOGContentPlayableCharacter>(this);
    if (!Unit || !((Legacy && Legacy->IsBoundToOwnedLuminousCombatUnit(Session->BoundRulerId,Unit->UnitEntityId,Unit->SkillSet.CurrentForm)) ||
        (Content && Content->IsBoundToOwnedCombatUnit(Session->BoundRulerId,Unit->UnitEntityId,Unit->SkillSet.CurrentForm))))
    { Error=TEXT("Battle controls require the actual possessed owned character."); return false; }
    FOGResolvedSkillSet Owned;
    if (!Core->ReadOwnedCharacterSkills(Unit->UnitEntityId,Session->BoundRulerId,Owned,Error) ||
        Owned.CurrentForm!=Unit->SkillSet.CurrentForm)
    { if (Error.IsEmpty()) Error=TEXT("The stored character Version changed."); return false; }
    return FOGNativeBattleCommands::BuildFor(*Unit,Out,Error);
}
bool AOGWorldPrototypeCharacter::SubmitApprovedLuminousWorldAction(int32 Slot,FName Branch,FString& Error)
{
    FOGNativeBattleCommandSet Set;
    if (!ReadOwnedLuminousBattleCommands(Set,Error)) return false;
    const auto* Choice=Set.Commands.FindByPredicate([Slot,Branch](const FOGNativeBattleCommand& C)
        { return !C.bFantasm && C.ActiveSlot==Slot && C.Branch==Branch && C.Mode.IsNone(); });
    if (!Choice) { Error=TEXT("This form has no approved command for that skill or branch."); return false; }
    return SubmitApprovedLuminousWorldCommand(Choice->Id,Error);
}
bool AOGWorldPrototypeCharacter::SubmitApprovedLuminousWorldCommand(FName CommandId,FString& Error)
{
    Error.Reset();
    if (!IsFoundationWorldInputAllowed())
    { Error=TEXT("World battle input is currently suspended."); return false; }
    FOGNativeBattleCommandSet Set;
    if (!ReadOwnedLuminousBattleCommands(Set,Error)) return false;
    const auto* Command=Set.Commands.FindByPredicate([CommandId](const FOGNativeBattleCommand& C) { return C.Id==CommandId; });
    if (!Command) { Error=TEXT("The selected command is not part of this approved kit."); return false; }
    auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    const auto* Session=GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>();
    const auto* Encounter=Core->GetOwnedLuminousTurnBattle();
    const FOGEntityId BattleId=Encounter->GetState().BattleId;
    const FOGEntityId SourceId=Session->SelectedManifestationId;
    const auto* Source=Encounter->GetState().Units.FindByPredicate(
        [&SourceId](const FOGCombatUnitState& U) { return U.UnitEntityId==SourceId; });
    if (!Source) { Error=TEXT("The owned battle participant is unavailable."); return false; }
    TArray<FOGEntityId> Selected;
    if (SelectedCanonicalBattleId==BattleId) Selected=SelectedCanonicalTurnTargets;
    if (Selected.IsEmpty() && HasTargetHudProjectionForHud() && GetTargetHudProjectionForHud().TargetEntityId.IsValid())
        Selected.Add(GetTargetHudProjectionForHud().TargetEntityId);
    FOGResolvedCombatAction Proposed,Action;
    Command->BuildAction(*Source,Selected,Proposed);
    if (!Encounter->ValidateResolvedAction(Proposed,Action,Error)) return false;
    for (const auto& Id:Action.TargetUnitIds)
    {
        const auto* Target=Encounter->GetState().Units.FindByPredicate(
            [&Id](const FOGCombatUnitState& U) { return U.UnitEntityId==Id; });
        if (!Target || !Target->CanAct())
        { Error=TEXT("Select a living participant currently on the battlefield."); return false; }
        if (Id!=SourceId && !UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),BattleId,Id))
        { Error=TEXT("A required battle character is unavailable in the World."); return false; }
    }
    // Include self-costs, self-restoration, ongoing effects and unselected AoE
    // participants in feedback; direct selected targets are not the full result.
    TMap<FOGEntityId,FOGLargeNumber> Before;
    for (const auto& Unit:Encounter->GetState().Units) Before.Add(Unit.UnitEntityId,Unit.CurrentHp);
    const FName Semantic=Command->MotionSemantic();
    const bool UseAuthoredMap=Command->Branch.IsNone() && Command->Mode.IsNone();
    const FOGContentId Skill=Command->SkillId;
    Action.ActionId=FOGEntityId::NewId();
    if (!Core->ApplyApprovedLuminousResolvedAction(Action,Error)) return false;
    ClearSelectedCanonicalTurnTargets();
    const auto* Updated=Core->GetOwnedLuminousTurnBattle();
    if (!Updated || Updated->GetState().BattleId!=BattleId) return true;
    AActor* FirstTarget=nullptr;
    auto* PC=Cast<APlayerController>(GetController());
    auto* Hud=PC?Cast<AOGWorldPresentationHud>(PC->GetHUD()):nullptr;
    for (const auto& Unit:Updated->GetState().Units)
    {
        AActor* Body=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),BattleId,Unit.UnitEntityId);
        if (!Body && Unit.UnitEntityId==SourceId) Body=this;
        if (!FirstTarget && Body && Action.TargetUnitIds.Contains(Unit.UnitEntityId)) FirstTarget=Body;
        const auto* Old=Before.Find(Unit.UnitEntityId);
        if (!Body || !Old || FOGLargeNumber::Compare(*Old,Unit.CurrentHp)==0) continue;
        if (Hud) Hud->RegisterCanonicalCombatImpact(Body,*Old,Unit.CurrentHp);
        if (auto* Motion=BattleMotion(Body))
        {
            if (!Unit.IsAlive()) Motion->PlayAction(TEXT("Combat.Reaction.Death"),TEXT("Combat"),false,1);
            else Motion->EnqueueCombatCue(FOGLargeNumber::Compare(Unit.CurrentHp,*Old)<0?
                FName(TEXT("Combat.Reaction.Hit")):FName(TEXT("Combat.Reaction.Heal")),BattleId,Action.ActionId,Unit.UnitEntityId);
        }
    }
    if (FirstTarget && FirstTarget!=this)
    {
        const FVector Delta=FirstTarget->GetActorLocation()-GetActorLocation();
        if (Delta.SizeSquared2D()>FMath::Square(5.f)) SetActorRotation(FRotator(0,Delta.Rotation().Yaw,0));
    }
    const auto* SurvivingSource=Updated->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==SourceId;});
    if (!SurvivingSource || !SurvivingSource->IsAlive()) return true;
    const auto* BoundBody=FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
    if (BoundBody && BoundBody->IsBoundTo(BattleId,SourceId)) return true;
    if (auto* Legacy=Cast<AOGLuminousV3PlayableCharacter>(this)) Legacy->PlaySharedGameplayAction(Semantic);
    if (auto* Content=Cast<AOGContentPlayableCharacter>(this); Content && Content->Appearance)
    {
        const FName* Authored=UseAuthoredMap?Content->CombatActionSemantics.Find(Skill):nullptr;
        const FName Key=Authored?*Authored:Semantic;
        auto* Motion=Content->Appearance->GetMotionController();
        if (!Motion || !Motion->IsPlayingAction() || Motion->CurrentAction!=Key)
        { FString MotionError; Content->Appearance->PlaySemanticAction(Key,TEXT("Combat"),false,1,NAME_None,MotionError); }
    }
    return true;
}
