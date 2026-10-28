#include "Content/OGSystemBindingComponent.h"
#include "Characters/OGContentCharacterActors.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Interaction/OGInteractionParticipantComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGNativeBattleCommands.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"

AOGContentPlayableCharacter::AOGContentPlayableCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Appearance = CreateDefaultSubobject<UOGCharacterAppearanceComponent>(TEXT("ContentAppearance"));
    SystemBindings = CreateDefaultSubobject<UOGSystemBindingComponent>(TEXT("SharedSystemBindings"));
    CombatBody = CreateDefaultSubobject<UOGCanonicalCombatActorBindingComponent>(TEXT("CanonicalCombatBody"));
    if (GetMesh()) Appearance->BodyComponentNamesToReplace.Add(GetMesh()->GetFName());
}

bool AOGContentPlayableCharacter::BindOwnedWorldCharacter_Implementation(
    const FOGEntityId& Ruler, const FOGContentId& Identity, const FOGContentId& Version,
    const FOGEntityId& Manifestation, bool bSfw)
{
    FString Error;
    if (!Appearance || !Appearance->BindOwnedManifestation(Ruler, Identity, Version, Manifestation, bSfw, Error))
    {
        UE_LOG(LogTemp, Warning, TEXT("Character appearance binding failed: %s"), *Error);
        return false;
    }
    if (BoundCore.IsValid()) BoundCore->OnNativeCombatActionResolved.RemoveAll(this);
    UOGGameCoreSubsystem* Core = GetGameInstance() ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    BoundCore = Core;
    BoundManifestation = Manifestation;
    BoundRuler = Ruler;
    BoundIdentity = Identity;
    BoundVersion = Version;
    FailedTraversalAction = NAME_None;
    LastPresentedActionId = FOGEntityId();
    LastMotionController.Reset();
    if (Core) Core->OnNativeCombatActionResolved.AddUObject(this, &ThisClass::HandleResolvedCombatAction);
    if (auto* Participant = FindComponentByClass<UOGInteractionParticipantComponent>())
    {
        auto* Motion = Appearance->GetMotionController();
        const FString Rig = Motion ? Motion->RigProfile.ToString().ToLower() : FString();
        const FOGContentId Family = FOGContentId(Rig).IsValid() ? FOGContentId(Rig) : FOGContentId(TEXT("rig:") + Rig);
        if (!Participant->BindActualNativeParticipant(Manifestation, Family, Error))
        { UE_LOG(LogTemp, Warning, TEXT("Native participant binding unavailable: %s"), *Error); return false; }
    }
    if (SystemBindings && !SystemBindings->BindCanonicalEntity(Manifestation,Error))
        UE_LOG(LogTemp,Warning,TEXT("Optional shared character bindings unavailable: %s"),*Error);
    return true;
}

bool AOGContentPlayableCharacter::IsBoundToOwnedCombatUnit(const FOGEntityId& Ruler,
    const FOGEntityId& Manifestation, const FOGContentId& Version) const
{
    return Appearance && Appearance->MatchesOwnedManifestation(Ruler, Manifestation) &&
        Ruler == BoundRuler && Manifestation == BoundManifestation && Version == BoundVersion;
}

void AOGContentPlayableCharacter::HandleResolvedCombatAction(const FOGResolvedCombatAction& Action)
{
    if (Action.SourceUnitId != BoundManifestation || !Appearance ||
        !Appearance->MatchesOwnedManifestation(BoundRuler, BoundManifestation)) return;
    if (!Action.ActionId.IsValid() || Action.ActionId == LastPresentedActionId) return;
    const FOGTurnBattle* Battle = BoundCore.IsValid() ? BoundCore->GetOwnedLuminousTurnBattle() : nullptr;
    const FOGCombatUnitState* Unit = Battle ? Battle->GetState().Units.FindByPredicate(
        [this](const FOGCombatUnitState& U) { return U.UnitEntityId == BoundManifestation; }) : nullptr;
    if (!Unit) return;
    TSharedPtr<FJsonObject> Resolution;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Action.ResolutionJson), Resolution) || !Resolution) return;
    FString Branch,Mode;
    if (Resolution->HasField(TEXT("branch")) && !Resolution->TryGetStringField(TEXT("branch"), Branch)) return;
    if (Resolution->HasField(TEXT("mode")) && !Resolution->TryGetStringField(TEXT("mode"), Mode)) return;
    const bool bUltimate = Action.SkillId == Unit->SkillSet.UltimateSkill;
    FOGNativeBattleCommandSet Commands; FString CommandError;
    bool Fantasm=false;
    if (FOGNativeBattleCommands::BuildFor(*Unit,Commands,CommandError))
        for (const auto& Command:Commands.Commands) if (Command.bFantasm && Command.SkillId==Action.SkillId) Fantasm=true;
    const FName* Authored = Branch.IsEmpty() && Mode.IsEmpty() ? CombatActionSemantics.Find(Action.SkillId) : nullptr;
    FString Key = FString(Fantasm ? TEXT("Combat.Fantasm.") : bUltimate ? TEXT("Combat.Ultimate.") : TEXT("Combat.Skill.")) + Action.SkillId.ToString();
    if (bUltimate && !Branch.IsEmpty()) Key += TEXT(".") + Branch.ToLower();
    if (!Mode.IsEmpty()) Key += TEXT(".") + Mode.ToLower();
    const FName Registered=Battle->ResolveNativeAbilityMotion(*Unit,Action.SkillId);
    const FName Semantic = Authored ? *Authored : !Registered.IsNone() ? Registered : FName(*Key);
    FString Error;
    if (!Appearance->PlaySemanticAction(Semantic, TEXT("Combat"), false, 1, NAME_None, Error))
        UE_LOG(LogTemp, Warning, TEXT("Source combat motion %s is unavailable: %s"), *Semantic.ToString(), *Error);
    else LastPresentedActionId = Action.ActionId;
}

void AOGContentPlayableCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (const auto* Participant = FindComponentByClass<UOGInteractionParticipantComponent>(); Participant && Participant->IsInInteractionPresentation()) return;
    if (!Appearance || !Appearance->MatchesOwnedManifestation(BoundRuler, BoundManifestation)) return;
    const FOGTurnBattle* Battle = BoundCore.IsValid() ? BoundCore->GetOwnedLuminousTurnBattle() : nullptr;
    if (Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running && CombatBody)
    {
        const FOGCombatUnitState* Unit = Battle->GetState().Units.FindByPredicate(
            [this](const FOGCombatUnitState& U) { return U.UnitEntityId == BoundManifestation; });
        if (Unit && Unit->CanAct() && !CombatBody->IsBoundTo(Battle->GetState().BattleId, BoundManifestation))
        {
            FString Error;
            CombatBody->BindActualNativeUnit(BoundManifestation, BoundIdentity, Error);
        }
        // Turn animation is driven by accepted combat events rather than
        // ground locomotion overwriting a visible attack or defeat.
        UOGSharedMotionControllerComponent* Motion = Appearance->GetMotionController();
        if (Motion && Motion->IsPlayingAction() && Motion->CurrentAction.ToString().StartsWith(TEXT("Traversal.")))
            Motion->StopAction();
        return;
    }
    UOGSharedMotionControllerComponent* Controller = Appearance->GetMotionController();
    if (!Controller) return;
    if (LastMotionController.Get() != Controller)
    {
        LastMotionController = Controller;
        FailedTraversalAction = NAME_None;
    }
    if (Controller->IsPlayingAction() &&
        !Controller->CurrentAction.ToString().StartsWith(TEXT("Traversal."))) return;
    const UCharacterMovementComponent* Movement = GetCharacterMovement();
    const float Speed = GetVelocity().Size2D();
    FName Action(TEXT("Traversal.Idle"));
    if (Movement && Movement->IsSwimming()) Action = Speed > 1.f ? TEXT("Traversal.Swim") : TEXT("Traversal.SwimIdle");
    else if (Movement && Movement->IsFlying()) Action = GetVelocity().Size() > 1.f ? TEXT("Traversal.Fly") : TEXT("Traversal.FlightHover");
    else if (Movement && Movement->IsFalling()) Action = GetVelocity().Z > 0.f ? TEXT("Traversal.Jump") : TEXT("Traversal.Fall");
    else if (Speed > 1.f) Action = RunAnimationSpeedCmPerSecond > 0.f && Speed >= RunAnimationSpeedCmPerSecond
        ? TEXT("Traversal.Run") : TEXT("Traversal.Walk");
    if ((!Controller->IsPlayingAction() || Controller->CurrentAction != Action) && FailedTraversalAction != Action)
    {
        FString Error;
        if (Appearance->PlaySemanticAction(Action, TEXT("Traversal"), false, 1, NAME_None, Error))
            FailedTraversalAction = NAME_None;
        else
        {
            FailedTraversalAction = Action;
            UE_LOG(LogTemp, Warning, TEXT("Source traversal motion %s is unavailable: %s"), *Action.ToString(), *Error);
        }
    }
}

void AOGContentPlayableCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if (BoundCore.IsValid()) BoundCore->OnNativeCombatActionResolved.RemoveAll(this);
    BoundCore.Reset();
    Super::EndPlay(Reason);
}

AOGContentCharacterPreviewActor::AOGContentCharacterPreviewActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AppearanceRoot"));
    Appearance = CreateDefaultSubobject<UOGCharacterAppearanceComponent>(TEXT("ContentAppearance"));
    SetActorEnableCollision(false);
}

bool AOGContentCharacterPreviewActor::ConfigureOwnedPreview_Implementation(
    const FOGEntityId& Ruler, const FOGContentId& Identity, const FOGContentId& Version,
    const FOGEntityId& Manifestation, bool bSfw)
{
    FString Error;
    const bool Result = Appearance && Appearance->BindOwnedManifestation(
        Ruler, Identity, Version, Manifestation, bSfw, Error);
    if (!Result) UE_LOG(LogTemp, Warning, TEXT("Character preview binding failed: %s"), *Error);
    return Result;
}
