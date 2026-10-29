#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Animation/OGSharedAnimationRoutes.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Combat/OGNativeBattleCommands.h"
#include "Combat/OGWorldAbilityExecutionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "Combat/OGTurnBattle.h"
#include "Interaction/OGInteractionParticipantComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Characters/OGContentCharacterActors.h"

UOGCanonicalCombatActorBindingComponent::UOGCanonicalCombatActorBindingComponent()
{
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickInterval=.25f;
}
void UOGCanonicalCombatActorBindingComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if (bAutoConnectCanonicalEncounters) { FString Error; RefreshCanonicalEncounter(Error); }
}
bool UOGCanonicalCombatActorBindingComponent::RefreshCanonicalEncounter(FString& Error)
{
    Error.Reset();
    if (!GetOwner() || !GetWorld() || !GetWorld()->GetGameInstance()) return false;
    auto* Core=GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    if (!Core || !Core->IsCoreReady() || !Core->GetOwnedLuminousTurnBattle() ||
        Core->GetOwnedLuminousTurnBattle()->GetState().Status!=EOGTurnBattleStatus::Running) return false;
    const auto& State=Core->GetOwnedLuminousTurnBattle()->GetState();
    // Existing owned-character adapters can bind without an interaction component.
    // Preserve their explicit, independently validated native binding.
    if (!bAutomaticBinding && BoundBattleId==State.BattleId && BoundUnitId.IsValid()) return true;
    const auto* Participant=GetOwner()->FindComponentByClass<UOGInteractionParticipantComponent>();
    const auto Entity=Participant?Participant->GetBoundCanonicalEntityId():FOGEntityId();
    if (BoundBattleId.IsValid() && (BoundBattleId!=State.BattleId || BoundUnitId!=Entity)) UnbindVisual();
    if (!Entity.IsValid()) return false;
    if (IsBoundTo(State.BattleId,Entity)) return true;
    const auto* Unit=State.Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Entity;});
    if (!Unit || !Unit->CanAct()) return false;
    FOGFoundationCharacterProjection Projection;
    if (!Participant->GetCurrentProjection(Projection,Error) || Projection.Context.EntityId!=Entity) return false;
    const bool Bound=BindActualNativeUnit(Unit->UnitEntityId,Unit->IdentityId,Error);
    if (Bound) bAutomaticBinding=true;
    return Bound;
}

bool UOGCanonicalCombatActorBindingComponent::BindActualNativeUnit(
    const FOGEntityId& UnitId, const FOGContentId& ApprovedIdentity,
    FString& OutError)
{
    OutError.Reset();
    UWorld* World=GetWorld();
    AActor* Owner=GetOwner();
    if (!World || !World->IsGameWorld() || !Owner ||
        !UnitId.IsValid() || !ApprovedIdentity.IsValid())
    {
        OutError=TEXT("Valid existing source character, World and unit ID required.");
        return false;
    }
    UGameInstance* GI=World->GetGameInstance();
    UOGGameCoreSubsystem* Core=GI
        ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const FOGTurnBattle* Battle=Core
        ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!Battle || Battle->GetState().Status!=EOGTurnBattleStatus::Running)
    {
        OutError=TEXT("No active authoritative GameCore encounter for this visual.");
        return false;
    }
    const FOGTurnBattleState& State=Battle->GetState();
    const FOGCombatUnitState* Unit=State.Units.FindByPredicate(
        [&UnitId](const FOGCombatUnitState& Candidate)
        { return Candidate.UnitEntityId==UnitId; });
    if (!Unit || !Unit->CanAct() || Unit->IdentityId!=ApprovedIdentity)
    {
        OutError=TEXT("Actor Identity cannot be bound to a nonmatching reserve/defeated/unknown canonical unit.");
        return false;
    }
    // Reject mock actors: the physical participant must have actual native
    // source skeletal geometry. This creates no mesh, version or placeholder.
    bool bHasNativeBody=false;
    TArray<USkeletalMeshComponent*> Parts;
    Owner->GetComponents<USkeletalMeshComponent>(Parts);
    for (USkeletalMeshComponent* Part:Parts)
        if (Part && Part->GetSkeletalMeshAsset() &&
            Part->GetSkeletalMeshAsset()->GetSkeleton())
        { bHasNativeBody=true;break; }
    if (!bHasNativeBody)
    {
        OutError=TEXT("Source character has no valid native skeletal body.");
        return false;
    }
    // One actual world body per canonical unit; never duplicate actors while
    // streaming, and never confuse the player with an unrelated NPC.
    if (AActor* Other=FindActualWorldActor(World,State.BattleId,UnitId))
        if (Other!=Owner)
        {
            OutError=TEXT("This canonical battle unit is already embodied elsewhere.");
            return false;
        }
    // Existing native owned Luminous has stricter Registry/manifestation
    // checks. Do not bypass that through this actor visual binding.
    if (AOGLuminousV3PlayableCharacter* Luminous=
        Cast<AOGLuminousV3PlayableCharacter>(Owner))
    {
        // Owned Luminous cannot be rebound to a different unit merely by
        // passing a matching display Identity to this visual function.
        const UOGGate12RulerSessionSubsystem* Session=GI
            ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
        if (!Session || !Session->IsBoundToStoredRuler() ||
            !Luminous->IsBoundToOwnedLuminousCombatUnit(
                Session->BoundRulerId,UnitId,Unit->SkillSet.CurrentForm))
        {
            OutError=TEXT("Owned Luminous Registry/Manifestation binding differs from requested unit.");
            return false;
        }
    }
    if (const AOGContentPlayableCharacter* ContentCharacter = Cast<AOGContentPlayableCharacter>(Owner))
    {
        const UOGGate12RulerSessionSubsystem* Session = GI
            ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
        if (!Session || !Session->IsBoundToStoredRuler() ||
            !ContentCharacter->IsBoundToOwnedCombatUnit(Session->BoundRulerId, UnitId, Unit->SkillSet.CurrentForm))
        {
            OutError = TEXT("Content character Registry/Manifestation binding differs from requested unit.");
            return false;
        }
    }
    if (BoundBattleId!=State.BattleId || BoundUnitId!=UnitId) UnbindVisual();
    if (BoundCore.IsValid())
    { BoundCore->OnNativeCombatActionResolved.RemoveAll(this); BoundCore->OnNativeBattleStateCommitted.RemoveAll(this); BoundCore->OnCanonicalRuntimeReleasing.RemoveAll(this); }
    BoundCore=Core;
    Core->OnNativeCombatActionResolved.AddUObject(this,&ThisClass::HandleNativeAction);
    Core->OnNativeBattleStateCommitted.AddUObject(this,&ThisClass::HandleCommittedState);
    Core->OnCanonicalRuntimeReleasing.AddUObject(this,&ThisClass::UnbindVisual);
    BoundBattleId=State.BattleId; bAutomaticBinding=false;
    BoundUnitId=UnitId; LastVisualHp=Unit->CurrentHp; bHasVisualHp=true;
    if (!Owner->FindComponentByClass<UOGWorldActionRuntimeComponent>())
    { auto* Actions=NewObject<UOGWorldActionRuntimeComponent>(Owner); Owner->AddInstanceComponent(Actions); Actions->RegisterComponent(); }
    if (!Owner->FindComponentByClass<UOGWorldAbilityExecutionComponent>())
    { auto* Execution=NewObject<UOGWorldAbilityExecutionComponent>(Owner); Owner->AddInstanceComponent(Execution); Execution->RegisterComponent(); }
    // Encounter opening is committed before a body can bind to its live battle ID.
    // Catch up those factual source actions only while the opening is still current.
    if (Core->IsNativeOpeningPresentationCurrent())
        for (const auto& Reaction:Battle->GetLastNativeReactions())
            if (Reaction.bExecuted && Reaction.Action.SourceUnitId==UnitId) HandleNativeAction(Reaction.Action);
    return true;
}

void UOGCanonicalCombatActorBindingComponent::UnbindVisual()
{
    if (BoundCore.IsValid())
    { BoundCore->OnNativeCombatActionResolved.RemoveAll(this); BoundCore->OnNativeBattleStateCommitted.RemoveAll(this); BoundCore->OnCanonicalRuntimeReleasing.RemoveAll(this); }
    BoundCore.Reset();
    auto* Appearance=GetOwner()?GetOwner()->FindComponentByClass<UOGCharacterAppearanceComponent>():nullptr;
    auto* Motion=Appearance?Appearance->GetMotionController():nullptr;
    if (!Motion && GetOwner()) Motion=GetOwner()->FindComponentByClass<UOGSharedMotionControllerComponent>();
    if (Motion) Motion->CancelCombatActions(BoundBattleId);
    LastPresentedAction=FOGEntityId(); bHasVisualHp=false; bAutomaticBinding=false;
    BoundBattleId=FOGEntityId();
    BoundUnitId=FOGEntityId();
}

bool UOGCanonicalCombatActorBindingComponent::IsBoundTo(
    const FOGEntityId& BattleId, const FOGEntityId& UnitId) const
{
    return BattleId.IsValid() && UnitId.IsValid() &&
        BoundBattleId==BattleId && BoundUnitId==UnitId;
}

AActor* UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(
    UWorld* World, const FOGEntityId& BattleId, const FOGEntityId& UnitId)
{
    if (!World || !BattleId.IsValid() || !UnitId.IsValid())
        return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor=*It;
        if (!IsValid(Actor)) continue;
        const UOGCanonicalCombatActorBindingComponent* Binding=
            Actor->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
        if (Binding && Binding->IsBoundTo(BattleId,UnitId))
            return Actor;
    }
    return nullptr;
}

void UOGCanonicalCombatActorBindingComponent::EndPlay(
    const EEndPlayReason::Type Reason)
{
    UnbindVisual();
    Super::EndPlay(Reason);
}

void UOGCanonicalCombatActorBindingComponent::HandleNativeAction(const FOGResolvedCombatAction& Action)
{
    if (!BoundCore.IsValid() || Action.SourceUnitId!=BoundUnitId || !Action.ActionId.IsValid() || LastPresentedAction==Action.ActionId) return;
    const auto* Battle=BoundCore->GetOwnedLuminousTurnBattle();
    if (!Battle || Battle->GetState().BattleId!=BoundBattleId) return;
    const auto* Unit=Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==BoundUnitId;});
    if (!Unit || !Unit->IsAlive()) return;
    if (auto* Routes=GetOwner()->FindComponentByClass<UOGSharedAnimationReceiver>())
    {
        bool Handled=false; FString Error;
        if (!Routes->PresentAcceptedCombat(Action,*Battle,Handled,Error)) { UE_LOG(LogTemp,Warning,TEXT("%s"),*Error); return; }
        if (Handled) { LastPresentedAction=Action.ActionId; return; }
    }
    FName Semantic=Battle->ResolveNativeAbilityMotion(*Unit,Action.SkillId);
    if (Semantic.IsNone())
    {
        FOGNativeBattleCommandSet Set; FString Error; TSharedPtr<FJsonObject> Context; FString Branch,Mode;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Action.ResolutionJson),Context);
        if (Context) { Context->TryGetStringField(TEXT("branch"),Branch); Context->TryGetStringField(TEXT("mode"),Mode); }
        if (FOGNativeBattleCommands::BuildFor(*Unit,Set,Error))
            for (const auto& Command:Set.Commands)
                if (Command.SkillId==Action.SkillId && Command.Branch==FName(*Branch) && Command.Mode==FName(*Mode))
                { Semantic=Command.MotionSemantic(); break; }
    }
    auto* Appearance=GetOwner()?GetOwner()->FindComponentByClass<UOGCharacterAppearanceComponent>():nullptr;
    auto* Motion=Appearance?Appearance->GetMotionController():nullptr;
    if (!Motion && GetOwner()) Motion=GetOwner()->FindComponentByClass<UOGSharedMotionControllerComponent>();
    if (Motion && !Semantic.IsNone())
    {
        const bool Presented=Battle->GetState().ExecutionMode==EOGCombatExecutionMode::World && Action.InterruptPriority==0?
            Motion->StartImmediateCombatAction(Semantic,BoundBattleId,Action.ActionId):
            Motion->EnqueueCombatAction(Semantic,BoundBattleId,Action.ActionId);
        if (Presented) LastPresentedAction=Action.ActionId;
    }
}

void UOGCanonicalCombatActorBindingComponent::HandleCommittedState(const FOGTurnBattleState& State,const FOGEntityId& Receipt)
{
    if (State.BattleId!=BoundBattleId || !GetOwner()) return;
    const auto* Unit=State.Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==BoundUnitId;}); if (!Unit) return;
    const auto Before=LastVisualHp; const bool Changed=bHasVisualHp && Before!=Unit->CurrentHp;
    LastVisualHp=Unit->CurrentHp; bHasVisualHp=true;
    if (!Changed || State.ExecutionMode!=EOGCombatExecutionMode::World) return;
    if (auto* Player=UGameplayStatics::GetPlayerController(this,0))
        if (auto* Hud=Cast<AOGWorldPresentationHud>(Player->GetHUD())) Hud->RegisterCanonicalCombatImpact(GetOwner(),Before,Unit->CurrentHp);
    auto* Appearance=GetOwner()->FindComponentByClass<UOGCharacterAppearanceComponent>();
    auto* Motion=Appearance?Appearance->GetMotionController():GetOwner()->FindComponentByClass<UOGSharedMotionControllerComponent>();
    if (!Motion) return;
    if (!Unit->IsAlive()) Motion->PlayAction(TEXT("Combat.Reaction.Death"),TEXT("Combat"),false,1);
    else Motion->EnqueueCombatCue(FOGLargeNumber::Compare(Unit->CurrentHp,Before)<0?
        FName(TEXT("Combat.Reaction.Hit")):FName(TEXT("Combat.Reaction.Heal")),State.BattleId,Receipt,BoundUnitId);
}
